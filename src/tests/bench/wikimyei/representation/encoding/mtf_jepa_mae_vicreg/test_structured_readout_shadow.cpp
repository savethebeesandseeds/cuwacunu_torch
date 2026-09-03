// SPDX-License-Identifier: MIT
#include "structured_readout_shadow.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
namespace srr = mtf::structured_readout_shadow;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double uniform01(uint64_t value) {
  return static_cast<double>(splitmix64(value) >> 11U) *
         (1.0 / 9007199254740992.0);
}

[[nodiscard]] double signed_uniform(uint64_t value) {
  return 2.0 * uniform01(value) - 1.0;
}

void mix_hash_value(uint64_t &hash, uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] uint64_t stable_tensor_hash(const torch::Tensor &input) {
  check(input.defined(), "cannot hash an undefined tensor");
  const auto contiguous = input.detach().to(torch::kCPU).contiguous();
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(contiguous.scalar_type()));
  mix_hash_value(hash, static_cast<uint64_t>(contiguous.dim()));
  for (const int64_t size : contiguous.sizes()) {
    mix_hash_value(hash, static_cast<uint64_t>(size));
  }
  const auto byte_count = static_cast<std::size_t>(contiguous.numel()) *
                          static_cast<std::size_t>(contiguous.element_size());
  const auto *bytes = static_cast<const uint8_t *>(contiguous.data_ptr());
  for (std::size_t index = 0; index < byte_count; ++index) {
    mix_hash_value(hash, static_cast<uint64_t>(bytes[index]));
  }
  return hash;
}

[[nodiscard]] torch::Tensor make_q0() {
  constexpr uint64_t kProjectionTag = 0x7273736d5f74655fULL;
  torch::NoGradGuard no_grad;
  auto dense = torch::empty(
      {srr::kProjectionInputWidth, srr::kProjectionOutputWidth},
      torch::kFloat64);
  auto values = dense.accessor<double, 2>();
  for (int64_t row = 0; row < dense.size(0); ++row) {
    for (int64_t column = 0; column < dense.size(1); ++column) {
      const uint64_t projection_key = splitmix64(
          kProjectionTag ^ splitmix64(static_cast<uint64_t>(row)) ^
          splitmix64(static_cast<uint64_t>(column) << 32U));
      values[row][column] = signed_uniform(projection_key);
    }
  }
  auto [projection, upper] = at::linalg_qr(dense, "reduced");
  const auto signs = torch::where(upper.diagonal().lt(0.0),
                                  -torch::ones_like(upper.diagonal()),
                                  torch::ones_like(upper.diagonal()));
  return (projection * signs.unsqueeze(0)).contiguous();
}

[[nodiscard]] torch::Tensor mean_basis() {
  auto basis = torch::zeros(
      {srr::kProjectionInputWidth, srr::kProjectionOutputWidth},
      torch::kFloat64);
  auto values = basis.accessor<double, 2>();
  const double scale = 1.0 / std::sqrt(srr::kTokensPerChannel);
  for (int64_t token = 0; token < srr::kTokensPerChannel; ++token) {
    for (int64_t feature = 0; feature < srr::kLatentDim; ++feature) {
      values[token * srr::kLatentDim + feature][feature] = scale;
    }
  }
  return basis;
}

[[nodiscard]] torch::Tensor make_qpsm(const torch::Tensor &q0) {
  torch::NoGradGuard no_grad;
  const auto basis = mean_basis();
  const auto contrast_seed =
      q0 - basis.matmul(basis.transpose(0, 1).matmul(q0));
  auto [contrast, upper] = at::linalg_qr(contrast_seed, "reduced");
  const auto signs = torch::where(upper.diagonal().lt(0.0),
                                  -torch::ones_like(upper.diagonal()),
                                  torch::ones_like(upper.diagonal()));
  contrast = contrast * signs.unsqueeze(0);
  return (basis / std::sqrt(srr::kTokensPerChannel) +
          std::sqrt(static_cast<double>(srr::kTokensPerChannel - 1) /
                    static_cast<double>(srr::kTokensPerChannel)) *
              contrast)
      .contiguous();
}

template <typename Function>
void expect_throw(Function &&function, const std::string &message) {
  bool threw = false;
  try {
    function();
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, message);
}

mtf::mtf_jepa_mae_vicreg_config_t config() {
  mtf::mtf_jepa_mae_vicreg_config_t result{};
  result.channel_count = srr::kChannelCount;
  result.history_length = srr::kHistoryLength;
  result.input_width = srr::kInputWidth;
  result.d_model = srr::kLatentDim;
  result.latent_dim = srr::kLatentDim;
  result.use_frequency_tokens = true;
  result.time_scales = {8, 16, 32, 64};
  result.scale_strides = {4, 8, 16, 32};
  result.serving_pool_policy = mtf::mtf_serving_pool_policy_t::all_tokens;
  return result;
}

mtf::mtf_token_metadata_t metadata(const torch::Device &device) {
  std::vector<int64_t> starts;
  std::vector<int64_t> widths;
  std::vector<int64_t> scales;
  std::vector<int64_t> channels;
  std::vector<int64_t> domains;
  starts.reserve(srr::kTokenCount);
  widths.reserve(srr::kTokenCount);
  scales.reserve(srr::kTokenCount);
  channels.reserve(srr::kTokenCount);
  domains.reserve(srr::kTokenCount);
  // These are the exact H=30 windows produced by scales 8/16/32/64 and
  // strides 4/8/16/32.  Deliberately emit domain/channel/scale order rather
  // than production channel-major order so canonicalization is still tested.
  const std::array<std::vector<std::pair<int64_t, int64_t>>,
                   srr::kScaleCount>
      windows{
          std::vector<std::pair<int64_t, int64_t>>{
              {0, 8}, {4, 8}, {8, 8}, {12, 8}, {16, 8}, {20, 8}, {22, 8}},
          std::vector<std::pair<int64_t, int64_t>>{
              {0, 16}, {8, 16}, {14, 16}},
          std::vector<std::pair<int64_t, int64_t>>{{0, 30}},
          std::vector<std::pair<int64_t, int64_t>>{{0, 30}}};
  for (int64_t domain = 0; domain < srr::kDomainCount; ++domain) {
    for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
      for (int64_t scale = 0; scale < srr::kScaleCount; ++scale) {
        const auto &scale_windows = windows[static_cast<std::size_t>(scale)];
        check(scale_windows.size() == static_cast<std::size_t>(
                                          srr::kTokensPerScale[
                                              static_cast<std::size_t>(scale)]),
              "frozen metadata fixture has the wrong scale cardinality");
        for (const auto &[start, width] : scale_windows) {
          starts.push_back(start);
          widths.push_back(width);
          scales.push_back(scale);
          channels.push_back(channel);
          domains.push_back(domain);
        }
      }
    }
  }
  const auto options =
      torch::TensorOptions().dtype(torch::kInt64).device(device);
  return {.start_index = torch::tensor(starts, options),
          .width = torch::tensor(widths, options),
          .scale_id = torch::tensor(scales, options),
          .channel_id = torch::tensor(channels, options),
          .domain_id = torch::tensor(domains, options)};
}

mtf::mtf_jepa_mae_vicreg_encode_output_t
encoded(int64_t batch_size, torch::Dtype dtype, const torch::Device &device) {
  mtf::mtf_jepa_mae_vicreg_encode_output_t result{};
  const auto options = torch::TensorOptions().dtype(dtype).device(device);
  result.embeddings =
      torch::arange(batch_size * srr::kTokenCount * srr::kLatentDim, options)
          .reshape({batch_size, srr::kTokenCount, srr::kLatentDim}) /
      1000.0;
  result.token_mask = torch::ones(
      {batch_size, srr::kTokenCount},
      torch::TensorOptions().dtype(torch::kBool).device(device));
  result.sample_valid_mask = torch::ones(
      {batch_size}, torch::TensorOptions().dtype(torch::kBool).device(device));
  result.channel_valid_mask = torch::ones(
      {batch_size, srr::kChannelCount},
      torch::TensorOptions().dtype(torch::kBool).device(device));
  result.metadata = metadata(device);
  return result;
}

torch::Tensor fixed_projection() {
  auto projection = torch::empty(
      {srr::kProjectionInputWidth, srr::kProjectionOutputWidth},
      torch::kFloat64);
  auto values = projection.accessor<double, 2>();
  for (int64_t row = 0; row < projection.size(0); ++row) {
    for (int64_t column = 0; column < projection.size(1); ++column) {
      const int64_t code = (row * 17 + column * 13 + 5) % 41;
      values[row][column] = (static_cast<double>(code) - 20.0) / 97.0;
    }
  }
  return projection;
}

torch::Tensor mean_preserving_projection() {
  auto projection = torch::zeros(
      {srr::kProjectionInputWidth, srr::kProjectionOutputWidth},
      torch::kFloat64);
  auto values = projection.accessor<double, 2>();
  for (int64_t position = 0; position < srr::kTokensPerChannel; ++position) {
    for (int64_t feature = 0; feature < srr::kLatentDim; ++feature) {
      values[position * srr::kLatentDim + feature][feature] =
          1.0 / static_cast<double>(srr::kTokensPerChannel);
    }
  }
  return projection;
}

torch::Tensor frozen_reference(
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &in,
    const torch::Tensor &projection);

void test_frozen_qpsm_contract() {
  const auto q0 = make_q0();
  const auto qpsm = make_qpsm(q0);
  const auto identity = torch::eye(srr::kProjectionOutputWidth, torch::kFloat64);
  const auto basis = mean_basis();
  const auto contrast =
      (qpsm - basis / std::sqrt(srr::kTokensPerChannel)) /
      std::sqrt(static_cast<double>(srr::kTokensPerChannel - 1) /
                static_cast<double>(srr::kTokensPerChannel));
  const double orthogonality_error =
      (qpsm.transpose(0, 1).matmul(qpsm) - identity)
          .abs()
          .max()
          .item<double>();
  const double contrast_mean_error =
      basis.transpose(0, 1).matmul(contrast).abs().max().item<double>();
  const double block_sum_error =
      (qpsm.reshape({srr::kTokensPerChannel, srr::kLatentDim,
                     srr::kProjectionOutputWidth})
           .sum(0) -
       identity)
          .abs()
          .max()
          .item<double>();

  check(q0.sizes() == torch::IntArrayRef(
                            {srr::kProjectionInputWidth,
                             srr::kProjectionOutputWidth}) &&
            q0.scalar_type() == torch::kFloat64 && q0.device().is_cpu() &&
            q0.is_contiguous() && torch::isfinite(q0).all().item<bool>(),
        "Q_0 tensor contract changed");
  check(qpsm.sizes() == torch::IntArrayRef(
                              {srr::kProjectionInputWidth,
                               srr::kProjectionOutputWidth}) &&
            qpsm.scalar_type() == torch::kFloat64 && qpsm.device().is_cpu() &&
            qpsm.is_contiguous() && torch::isfinite(qpsm).all().item<bool>(),
        "Q_psm tensor contract changed");
  check(stable_tensor_hash(q0) == 0xf8c9f35282de2ee0ULL,
        "Q_0 stable hash changed");
  check(stable_tensor_hash(qpsm) == 0xac8a43fd65b2c8a8ULL,
        "Q_psm stable hash changed");
  check(orthogonality_error <= 1.0e-10,
        "Q_psm orthogonality exceeded 1e-10");
  check(contrast_mean_error <= 1.0e-10,
        "Q_psm contrast leaked into the channel mean subspace");
  check(block_sum_error <= 1.0e-10,
        "Q_psm token-block sum no longer equals identity");

  const auto source = encoded(2, torch::kFloat64, torch::Device(torch::kCPU));
  const auto actual = srr::readout_cpu64(source, qpsm, config());
  const auto expected = frozen_reference(source, qpsm);
  check(torch::equal(actual.values, expected),
        "actual Q_psm readout lost byte identity with offline CDSB");
}

torch::Tensor frozen_reference(const mtf::mtf_jepa_mae_vicreg_encode_output_t &in,
                               const torch::Tensor &projection) {
  const auto plan = srr::build_plan(in.metadata);
  const auto values = in.embeddings.to(torch::kCPU, torch::kFloat64);
  std::vector<torch::Tensor> channels;
  channels.reserve(srr::kChannelCount);
  for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
    const auto &source =
        plan.ordered_token_indices[static_cast<std::size_t>(channel)];
    channels.push_back(values.index_select(
        1, torch::tensor(std::vector<int64_t>(source.begin(), source.end()),
                         torch::kInt64)));
  }
  const auto grouped = torch::stack(channels, 1);
  std::vector<torch::Tensor> means;
  means.reserve(srr::kCellCount);
  for (int64_t cell = 0; cell < srr::kCellCount; ++cell) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < srr::kTokensPerChannel; ++position) {
      if (srr::kFrozenCellIds[static_cast<std::size_t>(position)] == cell) {
        positions.push_back(position);
      }
    }
    means.push_back(
        grouped.index_select(2, torch::tensor(positions, torch::kInt64))
            .mean(2));
  }
  std::vector<torch::Tensor> lifted;
  lifted.reserve(srr::kTokensPerChannel);
  for (const int64_t cell : srr::kFrozenCellIds) {
    lifted.push_back(means[static_cast<std::size_t>(cell)]);
  }
  return torch::stack(lifted, 2)
      .reshape({values.size(0), srr::kChannelCount,
                srr::kProjectionInputWidth})
      .matmul(projection.to(torch::kFloat64))
      .contiguous();
}

mtf::mtf_token_metadata_t permute_metadata(
    const mtf::mtf_token_metadata_t &source, const torch::Tensor &permutation) {
  return {.start_index = source.start_index.index_select(0, permutation),
          .width = source.width.index_select(0, permutation),
          .scale_id = source.scale_id.index_select(0, permutation),
          .channel_id = source.channel_id.index_select(0, permutation),
          .domain_id = source.domain_id.index_select(0, permutation)};
}

void test_metadata_plan() {
  const auto plan = srr::build_plan(metadata(torch::Device(torch::kCPU)));
  for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
    check(plan.ordered_cell_ids[static_cast<std::size_t>(channel)] ==
              srr::kFrozenCellIds,
          "metadata cell IDs differ from frozen PSM-1 CDSB IDs");
    check(plan.cell_counts[static_cast<std::size_t>(channel)] ==
              srr::kFrozenCellCounts,
          "metadata cell cardinalities differ from PSM-1");
  }
}

void test_reference_and_permutation_equivalence() {
  const auto source = encoded(2, torch::kFloat64, torch::Device(torch::kCPU));
  const auto projection = fixed_projection();
  const auto actual = srr::readout_cpu64(source, projection, config());
  const auto expected = frozen_reference(source, projection);
  check(torch::allclose(actual.values, expected, 0.0, 1.0e-12),
        "CPU64 shadow differs from frozen CDSB partition/lift/projection");
  check(actual.valid_mask.all().item<bool>(),
        "fully observed reference unexpectedly became invalid");

  const auto permutation =
      torch::arange(srr::kTokenCount - 1, -1, -1, torch::kInt64);
  auto permuted = source;
  permuted.embeddings = source.embeddings.index_select(1, permutation);
  permuted.token_mask = source.token_mask.index_select(1, permutation);
  permuted.metadata = permute_metadata(source.metadata, permutation);
  const auto replay = srr::readout_cpu64(permuted, projection, config());
  check(torch::equal(actual.values, replay.values),
        "joint token/metadata permutation changed structured readout");
  check(torch::equal(actual.valid_mask, replay.valid_mask),
        "joint token/metadata permutation changed validity");
}

void test_partition_invariances_and_sensitivity() {
  const auto projection = fixed_projection();
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  const auto plan = srr::build_plan(source.metadata);
  const auto baseline = srr::readout(source, projection, config());
  auto within = source;
  within.embeddings = source.embeddings.clone();
  auto within_values = within.embeddings.accessor<double, 3>();
  const int64_t first = plan.ordered_token_indices[0][0];
  const int64_t second = plan.ordered_token_indices[0][1];
  const int64_t other_cell = plan.ordered_token_indices[0][2];
  for (int64_t feature = 0; feature < srr::kLatentDim; ++feature) {
    within_values[0][first][feature] += 8.0;
    within_values[0][second][feature] -= 8.0;
  }
  const auto within_result = srr::readout(within, projection, config());
  check(torch::allclose(baseline.values, within_result.values, 0.0, 1.0e-12),
        "zero-mean perturbation inside one cell changed readout");

  auto within_permuted = source;
  within_permuted.embeddings = source.embeddings.clone();
  const auto first_value =
      within_permuted.embeddings.select(1, first).clone();
  within_permuted.embeddings.select(1, first).copy_(
      within_permuted.embeddings.select(1, second));
  within_permuted.embeddings.select(1, second).copy_(first_value);
  const auto within_permuted_result =
      srr::readout(within_permuted, projection, config());
  check(torch::equal(baseline.values, within_permuted_result.values),
        "permuting values within one cell changed readout");

  auto across = source;
  across.embeddings = source.embeddings.clone();
  auto across_values = across.embeddings.accessor<double, 3>();
  for (int64_t feature = 0; feature < srr::kLatentDim; ++feature) {
    across_values[0][first][feature] += 1.0;
    across_values[0][other_cell][feature] -= 1.0;
  }
  const auto across_result = srr::readout(across, projection, config());
  check((baseline.values - across_result.values).abs().max().item<double>() >
            1.0e-6,
        "equal-opposite cross-bin signal change was not visible to readout");

  const auto lifted_once = srr::lift(source, config());
  const auto lifted_tokens = lifted_once.values.reshape(
      {source.embeddings.size(0), srr::kChannelCount,
       srr::kTokensPerChannel, srr::kLatentDim});
  auto partitioned = source;
  partitioned.embeddings = torch::zeros_like(source.embeddings);
  for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
    for (int64_t position = 0; position < srr::kTokensPerChannel; ++position) {
      const int64_t source_index =
          plan.ordered_token_indices[static_cast<std::size_t>(channel)]
                                    [static_cast<std::size_t>(position)];
      partitioned.embeddings.select(1, source_index)
          .copy_(lifted_tokens.select(1, channel).select(1, position));
    }
  }
  const auto lifted_twice = srr::lift(partitioned, config());
  check((lifted_once.values - lifted_twice.values)
                .abs()
                .max()
                .item<double>() <= 1.0e-12 &&
            torch::equal(lifted_once.valid_mask, lifted_twice.valid_mask),
        "structured partition is not idempotent");
}

void test_constant_preservation() {
  auto source = encoded(2, torch::kFloat64, torch::Device(torch::kCPU));
  source.embeddings.fill_(3.25);
  const auto result =
      srr::readout(source, mean_preserving_projection(), config());
  check(torch::allclose(result.values, torch::full_like(result.values, 3.25),
                        0.0, 1.0e-12),
        "constant token field was not preserved by mean-preserving projection");
}

void test_mask_semantics() {
  const auto projection = fixed_projection();
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  const auto plan = srr::build_plan(source.metadata);

  auto partial_cell = source;
  partial_cell.token_mask = source.token_mask.clone();
  partial_cell.token_mask.accessor<bool, 2>()[0]
                                          [plan.ordered_token_indices[0][0]] =
      false;
  const auto partial_result =
      srr::readout(partial_cell, projection, config());
  check(!partial_result.valid_mask[0][0].item<bool>(),
        "partially observed channel did not fail closed");
  check(partial_result.values[0][0].abs().max().item<double>() == 0.0,
        "partially observed channel output was not safely zeroed");
  check(partial_result.valid_mask[0][1].item<bool>() &&
            partial_result.valid_mask[0][2].item<bool>(),
        "one partial channel invalidated unrelated channels");

  auto missing_cell = source;
  missing_cell.token_mask = source.token_mask.clone();
  auto missing_mask = missing_cell.token_mask.accessor<bool, 2>();
  missing_mask[0][plan.ordered_token_indices[0][0]] = false;
  missing_mask[0][plan.ordered_token_indices[0][1]] = false;
  const auto missing_result =
      srr::readout(missing_cell, projection, config());
  check(!missing_result.valid_mask[0][0].item<bool>(),
        "channel with an empty cell was not marked invalid");
  check(missing_result.values[0][0].abs().max().item<double>() == 0.0,
        "invalid channel output was not safely zeroed");
  check(missing_result.valid_mask[0][1].item<bool>() &&
            missing_result.valid_mask[0][2].item<bool>(),
        "one channel's empty cell invalidated unrelated channels");

  auto upstream_invalid = source;
  upstream_invalid.channel_valid_mask = source.channel_valid_mask.clone();
  upstream_invalid.channel_valid_mask.accessor<bool, 2>()[0][2] = false;
  const auto upstream_result =
      srr::readout(upstream_invalid, projection, config());
  check(!upstream_result.valid_mask[0][2].item<bool>() &&
            upstream_result.values[0][2].abs().max().item<double>() == 0.0,
        "upstream channel validity was not respected");

  auto sample_invalid = source;
  sample_invalid.sample_valid_mask = source.sample_valid_mask.clone();
  sample_invalid.sample_valid_mask.accessor<bool, 1>()[0] = false;
  const auto sample_result =
      srr::readout(sample_invalid, projection, config());
  check(!sample_result.valid_mask.any().item<bool>() &&
            sample_result.values.abs().max().item<double>() == 0.0,
        "upstream sample invalidity did not fail every channel closed");
}

void test_dtype_device_shape_and_rng() {
  const auto projection = fixed_projection();
  const auto source = encoded(2, torch::kFloat32, torch::Device(torch::kCPU));
  torch::manual_seed(9041);
  const auto expected_next = torch::rand({11});
  torch::manual_seed(9041);
  const auto embeddings_before = source.embeddings.clone();
  const auto token_mask_before = source.token_mask.clone();
  const auto sample_mask_before = source.sample_valid_mask.clone();
  const auto channel_mask_before = source.channel_valid_mask.clone();
  const auto result = srr::readout(source, projection, config());
  const auto actual_next = torch::rand({11});
  check(torch::equal(expected_next, actual_next),
        "shadow readout consumed CPU RNG state");
  check(result.values.sizes() == torch::IntArrayRef({2, 3, 32}) &&
            result.valid_mask.sizes() == torch::IntArrayRef({2, 3}),
        "shadow output shape is not [B,3,32]/[B,3]");
  check(result.values.scalar_type() == torch::kFloat32 &&
            result.values.device().is_cpu(),
        "production-like path did not preserve CPU float32 dtype/device");
  check(torch::equal(source.embeddings, embeddings_before) &&
            torch::equal(source.token_mask, token_mask_before) &&
            torch::equal(source.sample_valid_mask, sample_mask_before) &&
            torch::equal(source.channel_valid_mask, channel_mask_before),
        "shadow readout mutated an input tensor");
  const auto audit = srr::readout_cpu64(source, projection, config());
  check(audit.values.scalar_type() == torch::kFloat64 &&
            audit.values.device().is_cpu(),
        "audit path is not CPU float64");

  if (torch::cuda::is_available()) {
    const auto cuda_source =
        encoded(2, torch::kFloat32, torch::Device(torch::kCUDA));
    const auto cuda_result = srr::readout(cuda_source, projection, config());
    check(cuda_result.values.scalar_type() == torch::kFloat32 &&
              cuda_result.values.device().is_cuda() &&
              cuda_result.valid_mask.device().is_cuda(),
          "production-like path did not preserve CUDA float32 dtype/device");
    const auto cuda_audit =
        srr::readout_cpu64(cuda_source, projection, config());
    check(cuda_audit.values.scalar_type() == torch::kFloat64 &&
              cuda_audit.values.device().is_cpu(),
          "CUDA audit translation did not end at CPU float64");
  }
}

void test_rejections() {
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  const auto projection = fixed_projection();

  expect_throw(
      [&] {
        auto bad = source;
        bad.embeddings = torch::Tensor{};
        (void)srr::readout(bad, projection, config());
      },
      "undefined embeddings were accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.embeddings = source.embeddings.narrow(1, 0, 71);
        (void)srr::readout(bad, projection, config());
      },
      "wrong embedding shape was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.embeddings = source.embeddings.to(torch::kInt64);
        (void)srr::readout(bad, projection, config());
      },
      "non-floating embeddings were accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.embeddings = source.embeddings.clone();
        bad.embeddings.fill_(std::numeric_limits<double>::quiet_NaN());
        (void)srr::readout(bad, projection, config());
      },
      "non-finite embeddings were accepted");
  expect_throw(
      [&] { (void)srr::readout(source, torch::Tensor{}, config()); },
      "undefined projection was accepted");
  expect_throw(
      [&] {
        (void)srr::readout(source, torch::zeros({768}, torch::kFloat64),
                           config());
      },
      "rank-one projection was accepted");
  expect_throw(
      [&] {
        (void)srr::readout(source, torch::zeros({767, 32}, torch::kFloat64),
                           config());
      },
      "wrong projection shape was accepted");
  expect_throw(
      [&] {
        (void)srr::readout(source, torch::zeros({768, 32}, torch::kInt64),
                           config());
      },
      "non-floating projection was accepted");
  expect_throw(
      [&] {
        (void)srr::readout(source, projection.to(torch::kFloat32), config());
      },
      "non-CPU64 fixed projection was accepted");
  expect_throw(
      [&] {
        const auto noncontiguous =
            projection.transpose(0, 1).contiguous().transpose(0, 1);
        check(!noncontiguous.is_contiguous(),
              "noncontiguous projection fixture became contiguous");
        (void)srr::readout(source, noncontiguous, config());
      },
      "non-contiguous fixed projection was accepted");
  expect_throw(
      [&] {
        auto trainable = projection.clone().set_requires_grad(true);
        (void)srr::readout(source, trainable, config());
      },
      "trainable projection was accepted by fixed shadow readout");
  expect_throw(
      [&] {
        auto nonfinite = projection.clone();
        nonfinite.fill_(std::numeric_limits<double>::infinity());
        (void)srr::readout(source, nonfinite, config());
      },
      "non-finite projection was accepted");
  if (torch::cuda::is_available()) {
    expect_throw(
        [&] { (void)srr::readout(source, projection.to(torch::kCUDA), config()); },
        "CUDA projection was accepted instead of fixed CPU64 projection");
  }

  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.channel_count = 2;
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong channel configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.history_length = 29;
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong history configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.d_model = 31;
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong encoder-width configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.latent_dim = 31;
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong latent-width configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.use_frequency_tokens = false;
        (void)srr::readout(source, projection, bad_config);
      },
      "time-only configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.time_scales = {8, 16, 32};
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong scale configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.scale_strides = {4, 8, 16, 31};
        (void)srr::readout(source, projection, bad_config);
      },
      "wrong stride configuration was accepted");
  expect_throw(
      [&] {
        auto bad_config = config();
        bad_config.serving_pool_policy =
            mtf::mtf_serving_pool_policy_t::domain_balanced;
        (void)srr::readout(source, projection, bad_config);
      },
      "non-baseline serving configuration was accepted");

  expect_throw(
      [&] {
        auto bad = source;
        bad.token_mask = torch::Tensor{};
        (void)srr::readout(bad, projection, config());
      },
      "undefined token mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.token_mask = source.token_mask.narrow(1, 0, 71);
        (void)srr::readout(bad, projection, config());
      },
      "wrong-shaped token mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.token_mask = source.token_mask.to(torch::kInt64);
        (void)srr::readout(bad, projection, config());
      },
      "non-bool token mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.sample_valid_mask = torch::Tensor{};
        (void)srr::readout(bad, projection, config());
      },
      "missing sample validity mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.sample_valid_mask = torch::ones({1, 1}, torch::kBool);
        (void)srr::readout(bad, projection, config());
      },
      "wrong-shaped sample validity mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.sample_valid_mask = source.sample_valid_mask.to(torch::kInt64);
        (void)srr::readout(bad, projection, config());
      },
      "non-bool sample validity mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.channel_valid_mask = torch::Tensor{};
        (void)srr::readout(bad, projection, config());
      },
      "missing channel validity mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.channel_valid_mask = source.channel_valid_mask.narrow(1, 0, 2);
        (void)srr::readout(bad, projection, config());
      },
      "wrong-shaped channel validity mask was accepted");
  expect_throw(
      [&] {
        auto bad = source;
        bad.channel_valid_mask = source.channel_valid_mask.to(torch::kInt64);
        (void)srr::readout(bad, projection, config());
      },
      "non-bool channel validity mask was accepted");
  if (torch::cuda::is_available()) {
    expect_throw(
        [&] {
          auto bad = source;
          bad.token_mask = source.token_mask.to(torch::kCUDA);
          (void)srr::readout(bad, projection, config());
        },
        "token mask on a different device was accepted");
  }

  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.start_index = torch::Tensor{};
        (void)srr::build_plan(bad);
      },
      "undefined metadata was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.width = source.metadata.width.narrow(0, 0, 71);
        (void)srr::build_plan(bad);
      },
      "wrong-sized metadata was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.scale_id = source.metadata.scale_id.to(torch::kFloat64);
        (void)srr::build_plan(bad);
      },
      "non-int64 metadata was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.channel_id = source.metadata.channel_id.clone();
        bad.channel_id.accessor<int64_t, 1>()[0] = 3;
        (void)srr::build_plan(bad);
      },
      "out-of-range channel was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.domain_id = source.metadata.domain_id.clone();
        bad.domain_id.accessor<int64_t, 1>()[0] = 2;
        (void)srr::build_plan(bad);
      },
      "out-of-range domain was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.scale_id = source.metadata.scale_id.clone();
        bad.scale_id.accessor<int64_t, 1>()[0] = 4;
        (void)srr::build_plan(bad);
      },
      "out-of-range scale was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.start_index = source.metadata.start_index.clone();
        bad.start_index.accessor<int64_t, 1>()[0] = -1;
        (void)srr::build_plan(bad);
      },
      "negative token start was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.width = source.metadata.width.clone();
        bad.width.accessor<int64_t, 1>()[0] = 0;
        (void)srr::build_plan(bad);
      },
      "non-positive token width was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        const auto plan = srr::build_plan(bad);
        bad.start_index = source.metadata.start_index.clone();
        auto starts = bad.start_index.accessor<int64_t, 1>();
        for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
          const int64_t token =
              plan.ordered_token_indices[static_cast<std::size_t>(channel)][0];
          starts[token] += 1;
        }
        (void)srr::build_plan(bad);
      },
      "shared but noncanonical positive start layout was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        const auto plan = srr::build_plan(bad);
        bad.width = source.metadata.width.clone();
        auto widths = bad.width.accessor<int64_t, 1>();
        for (int64_t channel = 0; channel < srr::kChannelCount; ++channel) {
          const int64_t token =
              plan.ordered_token_indices[static_cast<std::size_t>(channel)][0];
          widths[token] += 1;
        }
        (void)srr::build_plan(bad);
      },
      "shared but noncanonical positive width layout was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        bad.scale_id = source.metadata.scale_id.clone();
        bad.scale_id.accessor<int64_t, 1>()[0] = 1;
        (void)srr::build_plan(bad);
      },
      "wrong 7,3,1,1 scale cardinality was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        const auto plan = srr::build_plan(bad);
        const int64_t first = plan.ordered_token_indices[0][0];
        const int64_t second = plan.ordered_token_indices[0][1];
        bad.start_index = source.metadata.start_index.clone();
        bad.width = source.metadata.width.clone();
        bad.start_index.accessor<int64_t, 1>()[second] =
            bad.start_index.accessor<int64_t, 1>()[first];
        bad.width.accessor<int64_t, 1>()[second] =
            bad.width.accessor<int64_t, 1>()[first];
        (void)srr::build_plan(bad);
      },
      "ambiguous duplicate rank metadata was accepted");
  expect_throw(
      [&] {
        auto bad = source.metadata;
        const auto plan = srr::build_plan(bad);
        const int64_t token = plan.ordered_token_indices[1][0];
        bad.start_index = source.metadata.start_index.clone();
        bad.start_index.accessor<int64_t, 1>()[token] += 1;
        (void)srr::build_plan(bad);
      },
      "cross-channel metadata layout disagreement was accepted");
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    at::set_num_interop_threads(1);
    test_metadata_plan();
    test_frozen_qpsm_contract();
    test_reference_and_permutation_equivalence();
    test_partition_invariances_and_sensitivity();
    test_constant_preservation();
    test_mask_semantics();
    test_dtype_device_shape_and_rng();
    test_rejections();
    std::cout << "SRR-1 structured readout shadow tests passed\n";
    std::cout << "cells=16 tokens_per_channel=24 output_shape=[B,3,32]\n";
    std::cout << "training_or_augmentation_used=false\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "SRR-1 structured readout shadow test failure: "
              << error.what() << "\n";
    return 1;
  }
}
