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

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double signed_uniform(uint64_t value) {
  return 2.0 * static_cast<double>(splitmix64(value) >> 11U) *
             (1.0 / 9007199254740992.0) -
         1.0;
}

void mix_hash_value(uint64_t &hash, uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] uint64_t stable_tensor_hash(const torch::Tensor &input) {
  check(input.defined(), "cannot hash undefined tensor");
  const auto value = input.detach().to(torch::kCPU).contiguous();
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(value.scalar_type()));
  mix_hash_value(hash, static_cast<uint64_t>(value.dim()));
  for (const int64_t size : value.sizes()) {
    mix_hash_value(hash, static_cast<uint64_t>(size));
  }
  const auto byte_count = static_cast<std::size_t>(value.numel()) *
                          static_cast<std::size_t>(value.element_size());
  const auto *bytes = static_cast<const uint8_t *>(value.data_ptr());
  for (std::size_t index = 0; index < byte_count; ++index) {
    mix_hash_value(hash, bytes[index]);
  }
  return hash;
}

[[nodiscard]] torch::Tensor make_q0() {
  constexpr uint64_t kTag = 0x7273736d5f74655fULL;
  torch::NoGradGuard no_grad;
  auto dense = torch::empty(
      {srr::kProjectionInputWidth, srr::kProjectionOutputWidth},
      torch::kFloat64);
  auto values = dense.accessor<double, 2>();
  for (int64_t row = 0; row < dense.size(0); ++row) {
    for (int64_t column = 0; column < dense.size(1); ++column) {
      const uint64_t key = splitmix64(
          kTag ^ splitmix64(static_cast<uint64_t>(row)) ^
          splitmix64(static_cast<uint64_t>(column) << 32U));
      values[row][column] = signed_uniform(key);
    }
  }
  auto [q, upper] = at::linalg_qr(dense, "reduced");
  const auto signs = torch::where(upper.diagonal().lt(0.0),
                                  -torch::ones_like(upper.diagonal()),
                                  torch::ones_like(upper.diagonal()));
  return (q * signs.unsqueeze(0)).contiguous();
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

[[nodiscard]] torch::Tensor make_qpsm() {
  torch::NoGradGuard no_grad;
  const auto q0 = make_q0();
  const auto basis = mean_basis();
  const auto seed = q0 - basis.matmul(basis.transpose(0, 1).matmul(q0));
  auto [contrast, upper] = at::linalg_qr(seed, "reduced");
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

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t config() {
  mtf::mtf_jepa_mae_vicreg_config_t result{};
  result.channel_count = 3;
  result.history_length = 30;
  result.input_width = 9;
  result.d_model = 32;
  result.latent_dim = 32;
  result.use_frequency_tokens = true;
  result.time_scales = {8, 16, 32, 64};
  result.scale_strides = {4, 8, 16, 32};
  result.serving_pool_policy = mtf::mtf_serving_pool_policy_t::all_tokens;
  return result;
}

[[nodiscard]] mtf::mtf_token_metadata_t
metadata(const torch::Device &device) {
  const std::array<std::vector<std::pair<int64_t, int64_t>>, 4> windows{
      std::vector<std::pair<int64_t, int64_t>>{
          {0, 8}, {4, 8}, {8, 8}, {12, 8}, {16, 8}, {20, 8}, {22, 8}},
      std::vector<std::pair<int64_t, int64_t>>{
          {0, 16}, {8, 16}, {14, 16}},
      std::vector<std::pair<int64_t, int64_t>>{{0, 30}},
      std::vector<std::pair<int64_t, int64_t>>{{0, 30}}};
  std::vector<int64_t> starts;
  std::vector<int64_t> widths;
  std::vector<int64_t> scales;
  std::vector<int64_t> channels;
  std::vector<int64_t> domains;
  for (int64_t domain = 0; domain < 2; ++domain) {
    for (int64_t channel = 0; channel < 3; ++channel) {
      for (int64_t scale = 0; scale < 4; ++scale) {
        for (const auto &[start, width] :
             windows[static_cast<std::size_t>(scale)]) {
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

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_encode_output_t
encoded(int64_t batch, torch::Dtype dtype, const torch::Device &device) {
  mtf::mtf_jepa_mae_vicreg_encode_output_t result{};
  const auto options = torch::TensorOptions().dtype(dtype).device(device);
  result.embeddings =
      torch::arange(batch * 72 * 32, options).reshape({batch, 72, 32}) /
      1000.0;
  result.token_mask = torch::ones(
      {batch, 72}, torch::TensorOptions().dtype(torch::kBool).device(device));
  result.sample_valid_mask = torch::ones(
      {batch}, torch::TensorOptions().dtype(torch::kBool).device(device));
  result.channel_valid_mask = torch::ones(
      {batch, 3}, torch::TensorOptions().dtype(torch::kBool).device(device));
  result.metadata = metadata(device);
  result.pooled_by_channel = torch::full(
      {batch, 3, 32}, std::numeric_limits<double>::quiet_NaN(), options);
  return result;
}

[[nodiscard]] mtf::mtf_serving_pool_output_t
production(const mtf::mtf_jepa_mae_vicreg_encode_output_t &source,
           const mtf::mtf_jepa_mae_vicreg_config_t &cfg) {
  return mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1, cfg);
}

[[nodiscard]] mtf::mtf_serving_pool_output_t
shadow(const mtf::mtf_jepa_mae_vicreg_encode_output_t &source,
       const torch::Tensor &qpsm,
       const mtf::mtf_jepa_mae_vicreg_config_t &cfg) {
  return srr::readout(source, qpsm, cfg);
}

void check_exact(const mtf::mtf_serving_pool_output_t &left,
                 const mtf::mtf_serving_pool_output_t &right,
                 const std::string &context) {
  check(torch::equal(left.values, right.values),
        context + ": value bytes differ");
  check(torch::equal(left.valid_mask, right.valid_mask),
        context + ": valid-mask bytes differ");
}

[[nodiscard]] mtf::mtf_token_metadata_t permute_metadata(
    const mtf::mtf_token_metadata_t &source,
    const torch::Tensor &permutation) {
  return {.start_index = source.start_index.index_select(0, permutation),
          .width = source.width.index_select(0, permutation),
          .scale_id = source.scale_id.index_select(0, permutation),
          .channel_id = source.channel_id.index_select(0, permutation),
          .domain_id = source.domain_id.index_select(0, permutation)};
}

void test_policy_surface() {
  check(static_cast<int>(mtf::mtf_serving_pool_policy_t::all_tokens) == 0 &&
            static_cast<int>(mtf::mtf_serving_pool_policy_t::time_only) == 1 &&
            static_cast<int>(mtf::mtf_serving_pool_policy_t::frequency_only) ==
                2 &&
            static_cast<int>(mtf::mtf_serving_pool_policy_t::domain_balanced) ==
                3 &&
            static_cast<int>(
                mtf::mtf_serving_pool_policy_t::structured_cdsb_v1) == 4,
        "serving policy ordinals are not append-only");
  check(std::string(mtf::mtf_serving_pool_policy_name(
            mtf::mtf_serving_pool_policy_t::all_tokens)) == "all_tokens" &&
            std::string(mtf::mtf_serving_pool_policy_name(
                mtf::mtf_serving_pool_policy_t::time_only)) == "time_only" &&
            std::string(mtf::mtf_serving_pool_policy_name(
                mtf::mtf_serving_pool_policy_t::frequency_only)) ==
                "frequency_only" &&
            std::string(mtf::mtf_serving_pool_policy_name(
                mtf::mtf_serving_pool_policy_t::domain_balanced)) ==
                "domain_balanced" &&
            std::string(mtf::mtf_serving_pool_policy_name(
                mtf::mtf_serving_pool_policy_t::structured_cdsb_v1)) ==
                "structured_cdsb_v1",
        "serving policy names changed");
  check(config().serving_pool_policy ==
            mtf::mtf_serving_pool_policy_t::all_tokens,
        "test fixture does not preserve all_tokens as stored policy");
  expect_throw(
      [&] {
        (void)mtf::mtf_serving_pool_policy_name(
            static_cast<mtf::mtf_serving_pool_policy_t>(999));
      },
      "unknown serving policy name was accepted");
}

void test_projection_contract() {
  const auto q0 = make_q0();
  const auto qpsm = make_qpsm();
  const auto identity = torch::eye(32, torch::kFloat64);
  const auto basis = mean_basis();
  const auto contrast =
      (qpsm - basis / std::sqrt(24.0)) / std::sqrt(23.0 / 24.0);
  const double orthogonality =
      (qpsm.transpose(0, 1).matmul(qpsm) - identity)
          .abs()
          .max()
          .item<double>();
  const double mean_error =
      basis.transpose(0, 1).matmul(contrast).abs().max().item<double>();
  const double block_sum_error =
      (qpsm.reshape({24, 32, 32}).sum(0) - identity)
          .abs()
          .max()
          .item<double>();
  check(stable_tensor_hash(q0) == 0xf8c9f35282de2ee0ULL,
        "independent Q0 hash changed");
  check(stable_tensor_hash(qpsm) == 0xac8a43fd65b2c8a8ULL,
        "independent Qpsm hash changed");
  check(qpsm.sizes() == torch::IntArrayRef({768, 32}) &&
            qpsm.scalar_type() == torch::kFloat64 && qpsm.device().is_cpu() &&
            qpsm.is_contiguous() && !qpsm.requires_grad() &&
            torch::isfinite(qpsm).all().item<bool>(),
        "independent Qpsm tensor contract changed");
  check(orthogonality <= 1.0e-10 && mean_error <= 1.0e-10 &&
            block_sum_error <= 1.0e-10,
        "independent Qpsm invariant changed");
}

void test_projection_thread_independence() {
  at::set_num_threads(4);
  const int threads_before = at::get_num_threads();
#if AT_MKL_ENABLED()
  const int original_mkl_local_threads = ::MKL_Set_Num_Threads_Local(2);
#endif
  const auto projection = mtf::detail::structured_cdsb_v1_projection_for(
      torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  check(stable_tensor_hash(projection) == 0xac8a43fd65b2c8a8ULL,
        "production projection changed under a multi-threaded caller");
  check(at::get_num_threads() == threads_before,
        "production projection changed the caller's Torch thread count");
#if AT_MKL_ENABLED()
  const int restored_mkl_local_threads = ::MKL_Set_Num_Threads_Local(1);
  check(restored_mkl_local_threads == 2,
        "production projection did not restore the caller's MKL-local thread "
        "override");
  (void)::MKL_Set_Num_Threads_Local(original_mkl_local_threads);
#endif
  at::set_num_threads(1);
}

void test_cpu_parity_and_purity() {
  const auto qpsm = make_qpsm();
  for (const auto dtype : {torch::kFloat64, torch::kFloat32}) {
    auto source = encoded(2, dtype, torch::Device(torch::kCPU));
    const auto embeddings_before = source.embeddings.clone();
    const auto token_mask_before = source.token_mask.clone();
    const auto sample_mask_before = source.sample_valid_mask.clone();
    const auto channel_mask_before = source.channel_valid_mask.clone();
    const auto metadata_before = source.metadata.start_index.clone();

    torch::manual_seed(9127);
    const auto expected_next = torch::rand({19});
    torch::manual_seed(9127);
    const auto actual = production(source, config());
    const auto actual_next = torch::rand({19});
    const auto oracle = shadow(source, qpsm, config());
    const auto replay = production(source, config());

    check_exact(actual, oracle, "CPU production/shadow parity");
    check_exact(actual, replay, "CPU repeated production parity");
    check(torch::equal(expected_next, actual_next),
          "first production structured call consumed CPU RNG");
    check(actual.values.sizes() == torch::IntArrayRef({2, 3, 32}) &&
              actual.valid_mask.sizes() == torch::IntArrayRef({2, 3}) &&
              actual.values.scalar_type() == dtype &&
              actual.valid_mask.scalar_type() == torch::kBool &&
              actual.values.device().is_cpu() &&
              actual.valid_mask.device().is_cpu() &&
              actual.values.is_contiguous() &&
              torch::isfinite(actual.values).all().item<bool>() &&
              actual.valid_mask.all().item<bool>(),
          "CPU production output contract changed");
    check(torch::equal(source.embeddings, embeddings_before) &&
              torch::equal(source.token_mask, token_mask_before) &&
              torch::equal(source.sample_valid_mask, sample_mask_before) &&
              torch::equal(source.channel_valid_mask, channel_mask_before) &&
              torch::equal(source.metadata.start_index, metadata_before),
          "production structured readout mutated input");
  }

  auto explicit_config = config();
  explicit_config.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::domain_balanced;
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  check_exact(production(source, explicit_config),
              shadow(source, qpsm, config()),
              "explicit selector must not require stored structured policy");

  auto constant = source;
  constant.embeddings.fill_(3.25);
  const auto constant_result = production(constant, config());
  check(torch::allclose(constant_result.values,
                        torch::full_like(constant_result.values, 3.25), 0.0,
                        1.0e-12),
        "production Qpsm did not preserve a constant token field");
}

void test_metadata_and_partition_behavior() {
  const auto qpsm = make_qpsm();
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  const auto plan = srr::build_plan(source.metadata);
  for (int64_t channel = 0; channel < 3; ++channel) {
    check(plan.ordered_cell_ids[static_cast<std::size_t>(channel)] ==
              srr::kFrozenCellIds &&
              plan.cell_counts[static_cast<std::size_t>(channel)] ==
                  srr::kFrozenCellCounts,
          "accepted metadata no longer derives the frozen 16-cell plan");
  }
  const auto baseline = production(source, config());

  const auto permutation = torch::arange(71, -1, -1, torch::kInt64);
  auto permuted = source;
  permuted.embeddings = source.embeddings.index_select(1, permutation);
  permuted.token_mask = source.token_mask.index_select(1, permutation);
  permuted.metadata = permute_metadata(source.metadata, permutation);
  check_exact(baseline, production(permuted, config()),
              "joint token/metadata permutation");
  check_exact(production(permuted, config()),
              shadow(permuted, qpsm, config()),
              "permuted production/shadow parity");

  auto within = source;
  within.embeddings = source.embeddings.clone();
  const int64_t first = plan.ordered_token_indices[0][0];
  const int64_t second = plan.ordered_token_indices[0][1];
  const int64_t other_cell = plan.ordered_token_indices[0][2];
  auto within_values = within.embeddings.accessor<double, 3>();
  for (int64_t feature = 0; feature < 32; ++feature) {
    within_values[0][first][feature] += 8.0;
    within_values[0][second][feature] -= 8.0;
  }
  check(torch::allclose(baseline.values, production(within, config()).values,
                        0.0, 1.0e-12),
        "within-cell zero-mean perturbation changed production output");

  auto across = source;
  across.embeddings = source.embeddings.clone();
  auto across_values = across.embeddings.accessor<double, 3>();
  for (int64_t feature = 0; feature < 32; ++feature) {
    across_values[0][first][feature] += 1.0;
    across_values[0][other_cell][feature] -= 1.0;
  }
  check((baseline.values - production(across, config()).values)
                .abs()
                .max()
                .item<double>() > 1.0e-6,
        "cross-cell perturbation was invisible to production output");
}

void test_masks_and_pooled_poison() {
  const auto qpsm = make_qpsm();
  const auto source = encoded(2, torch::kFloat64, torch::Device(torch::kCPU));
  const auto plan = srr::build_plan(source.metadata);

  auto partial = source;
  partial.token_mask = source.token_mask.clone();
  partial.token_mask.index_put_({0, plan.ordered_token_indices[0][0]}, false);
  const auto partial_p = production(partial, config());
  check_exact(partial_p, shadow(partial, qpsm, config()),
              "partial-token mask parity");
  check(!partial_p.valid_mask.index({0, 0}).item<bool>() &&
            partial_p.values.index({0, 0}).eq(0).all().item<bool>() &&
            partial_p.valid_mask.index({0, 1}).item<bool>() &&
            partial_p.valid_mask.index({0, 2}).item<bool>(),
        "partial token did not invalidate only its owning channel");

  auto empty_cell = source;
  empty_cell.token_mask = source.token_mask.clone();
  empty_cell.token_mask.index_put_({0, plan.ordered_token_indices[0][0]},
                                   false);
  empty_cell.token_mask.index_put_({0, plan.ordered_token_indices[0][1]},
                                   false);
  const auto empty_p = production(empty_cell, config());
  check_exact(empty_p, shadow(empty_cell, qpsm, config()),
              "empty-cell mask parity");
  check(!empty_p.valid_mask.index({0, 0}).item<bool>() &&
            empty_p.values.index({0, 0}).eq(0).all().item<bool>(),
        "empty cell did not fail its channel closed");

  auto channel_invalid = source;
  channel_invalid.channel_valid_mask = source.channel_valid_mask.clone();
  channel_invalid.channel_valid_mask.index_put_({0, 2}, false);
  const auto channel_p = production(channel_invalid, config());
  check_exact(channel_p, shadow(channel_invalid, qpsm, config()),
              "upstream channel-mask parity");
  check(!channel_p.valid_mask.index({0, 2}).item<bool>() &&
            channel_p.values.index({0, 2}).eq(0).all().item<bool>(),
        "upstream invalid channel was not exactly zero");

  auto sample_invalid = source;
  sample_invalid.sample_valid_mask = source.sample_valid_mask.clone();
  sample_invalid.sample_valid_mask.index_put_({1}, false);
  const auto sample_p = production(sample_invalid, config());
  check_exact(sample_p, shadow(sample_invalid, qpsm, config()),
              "sample-mask parity");
  check(!sample_p.valid_mask.index({1}).any().item<bool>() &&
            sample_p.values.index({1}).eq(0).all().item<bool>(),
        "invalid sample was not exactly zero");

  auto all_invalid = source;
  all_invalid.token_mask.zero_();
  const auto all_p = production(all_invalid, config());
  check_exact(all_p, shadow(all_invalid, qpsm, config()),
              "all-invalid mask parity");
  check(!all_p.valid_mask.any().item<bool>() &&
            all_p.values.eq(0).all().item<bool>(),
        "all-invalid input did not produce exact zeros");

  auto different_poison = source;
  different_poison.pooled_by_channel =
      torch::full_like(source.pooled_by_channel,
                       std::numeric_limits<double>::infinity());
  check_exact(production(source, config()),
              production(different_poison, config()),
              "structured output depends on pooled_by_channel");
}

void test_legacy_policy_goldens() {
  mtf::mtf_jepa_mae_vicreg_config_t cfg{};
  cfg.channel_count = 2;
  cfg.use_frequency_tokens = true;
  mtf::mtf_jepa_mae_vicreg_encode_output_t source{};
  source.embeddings = torch::tensor({{{2.0, 4.0},
                                      {4.0, 6.0},
                                      {12.0, 20.0},
                                      {6.0, 8.0},
                                      {14.0, 16.0},
                                      {18.0, 20.0}}},
                                    torch::kFloat32);
  source.token_mask = torch::ones({1, 6}, torch::kBool);
  source.metadata.channel_id =
      torch::tensor({0, 0, 0, 1, 1, 1}, torch::kInt64);
  source.metadata.domain_id =
      torch::tensor({0, 0, 1, 0, 1, 1}, torch::kInt64);
  source.pooled_by_channel = mtf::detail::pooled_by_channel(
      source.embeddings, source.token_mask, source.metadata, cfg);
  source.channel_valid_mask =
      mtf::detail::channel_valid_mask(source.metadata, source.token_mask, cfg);

  const auto all = mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::all_tokens, cfg);
  const auto time = mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::time_only, cfg);
  const auto frequency = mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::frequency_only, cfg);
  const auto balanced = mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::domain_balanced, cfg);
  check(torch::equal(all.values, source.pooled_by_channel) &&
            torch::equal(all.valid_mask, source.channel_valid_mask),
        "all_tokens golden changed");
  check(torch::equal(time.values,
                     torch::tensor({{{3.0, 5.0}, {6.0, 8.0}}},
                                   torch::kFloat32)) &&
            time.valid_mask.all().item<bool>(),
        "time_only golden changed");
  check(torch::equal(frequency.values,
                     torch::tensor({{{12.0, 20.0}, {16.0, 18.0}}},
                                   torch::kFloat32)) &&
            frequency.valid_mask.all().item<bool>(),
        "frequency_only golden changed");
  check(torch::equal(balanced.values,
                     torch::tensor({{{7.5, 12.5}, {11.0, 13.0}}},
                                   torch::kFloat32)) &&
            balanced.valid_mask.all().item<bool>(),
        "domain_balanced golden changed");

  expect_throw(
      [&] {
        (void)production(source, cfg);
      },
      "structured policy accepted legacy C=2/N=6/D=2 layout");
  expect_throw(
      [&] {
        (void)mtf::select_mtf_serving_pool(
            source, static_cast<mtf::mtf_serving_pool_policy_t>(999), cfg);
      },
      "selector accepted unknown enum value");
}

void test_rejections() {
  const auto qpsm = make_qpsm();
  const auto source = encoded(1, torch::kFloat64, torch::Device(torch::kCPU));
  const auto expect_both = [&](auto mutate, const std::string &message) {
    auto bad = source;
    mutate(bad);
    expect_throw([&] { (void)production(bad, config()); },
                 "production accepted " + message);
    expect_throw([&] { (void)shadow(bad, qpsm, config()); },
                 "shadow accepted " + message);
  };

  expect_both([](auto &bad) { bad.embeddings = torch::Tensor{}; },
              "undefined embeddings");
  expect_both(
      [](auto &bad) { bad.embeddings = bad.embeddings.narrow(1, 0, 71); },
      "wrong embedding width");
  expect_both(
      [](auto &bad) { bad.embeddings = bad.embeddings.to(torch::kInt64); },
      "non-floating embeddings");
  expect_both(
      [](auto &bad) {
        bad.embeddings = bad.embeddings.clone();
        bad.embeddings.index_put_({0, 0, 0},
                                  std::numeric_limits<double>::quiet_NaN());
      },
      "non-finite embeddings");
  expect_both([](auto &bad) { bad.token_mask = torch::Tensor{}; },
              "undefined token mask");
  expect_both(
      [](auto &bad) { bad.token_mask = bad.token_mask.narrow(1, 0, 71); },
      "wrong token-mask width");
  expect_both(
      [](auto &bad) { bad.token_mask = bad.token_mask.to(torch::kInt64); },
      "non-bool token mask");
  expect_both([](auto &bad) { bad.sample_valid_mask = torch::Tensor{}; },
              "undefined sample mask");
  expect_both(
      [](auto &bad) {
        bad.sample_valid_mask = torch::ones({1, 1}, torch::kBool);
      },
      "wrong sample-mask rank");
  expect_both([](auto &bad) { bad.channel_valid_mask = torch::Tensor{}; },
              "undefined channel mask");
  expect_both(
      [](auto &bad) {
        bad.channel_valid_mask = bad.channel_valid_mask.narrow(1, 0, 2);
      },
      "wrong channel-mask width");
  expect_both([](auto &bad) { bad.metadata.start_index = torch::Tensor{}; },
              "undefined metadata");
  expect_both(
      [](auto &bad) {
        bad.metadata.width = bad.metadata.width.narrow(0, 0, 71);
      },
      "wrong metadata width");
  expect_both(
      [](auto &bad) {
        bad.metadata.scale_id = bad.metadata.scale_id.to(torch::kFloat64);
      },
      "non-int64 metadata");
  expect_both(
      [](auto &bad) {
        bad.metadata.channel_id = bad.metadata.channel_id.clone();
        bad.metadata.channel_id.index_put_({0}, 3);
      },
      "out-of-range channel id");
  expect_both(
      [](auto &bad) {
        bad.metadata.domain_id = bad.metadata.domain_id.clone();
        bad.metadata.domain_id.index_put_({0}, 2);
      },
      "out-of-range domain id");
  expect_both(
      [](auto &bad) {
        bad.metadata.scale_id = bad.metadata.scale_id.clone();
        bad.metadata.scale_id.index_put_({0}, 4);
      },
      "out-of-range scale id");
  expect_both(
      [](auto &bad) {
        bad.metadata.start_index = bad.metadata.start_index.clone();
        bad.metadata.start_index.index_put_({0}, -1);
      },
      "negative start");
  expect_both(
      [](auto &bad) {
        bad.metadata.width = bad.metadata.width.clone();
        bad.metadata.width.index_put_({0}, 0);
      },
      "non-positive width");
  expect_both(
      [](auto &bad) {
        const auto plan = srr::build_plan(bad.metadata);
        const int64_t first = plan.ordered_token_indices[0][0];
        const int64_t second = plan.ordered_token_indices[0][1];
        bad.metadata.start_index = bad.metadata.start_index.clone();
        bad.metadata.width = bad.metadata.width.clone();
        bad.metadata.start_index.index_put_(
            {second}, bad.metadata.start_index.index({first}));
        bad.metadata.width.index_put_({second}, bad.metadata.width.index({first}));
      },
      "duplicate ordering key");
  expect_both(
      [](auto &bad) {
        const auto plan = srr::build_plan(bad.metadata);
        const int64_t token = plan.ordered_token_indices[1][0];
        bad.metadata.start_index = bad.metadata.start_index.clone();
        bad.metadata.start_index.index_put_(
            {token},
            bad.metadata.start_index.index({token}).template item<int64_t>() +
                1);
      },
      "cross-channel layout disagreement");

  const auto expect_bad_config = [&](auto mutate, const std::string &message) {
    auto bad = config();
    mutate(bad);
    expect_throw([&] { (void)production(source, bad); },
                 "production accepted " + message);
  };
  expect_bad_config([](auto &bad) { bad.channel_count = 2; },
                    "wrong channel count");
  expect_bad_config([](auto &bad) { bad.history_length = 29; },
                    "wrong history length");
  expect_bad_config([](auto &bad) { bad.input_width = 8; },
                    "wrong input width");
  expect_bad_config([](auto &bad) { bad.d_model = 31; },
                    "wrong model width");
  expect_bad_config([](auto &bad) { bad.latent_dim = 31; },
                    "wrong latent width");
  expect_bad_config([](auto &bad) { bad.use_frequency_tokens = false; },
                    "frequency-free layout");
  expect_bad_config([](auto &bad) { bad.time_scales = {8, 16, 32}; },
                    "wrong scales");
  expect_bad_config([](auto &bad) { bad.scale_strides = {4, 8, 16, 31}; },
                    "wrong strides");

  auto configured = config();
  configured.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::structured_cdsb_v1;
  mtf::detail::validate_architecture_config(configured);
  configured.history_length = 31;
  expect_throw([&] { mtf::detail::validate_architecture_config(configured); },
               "configured structured policy accepted unsupported layout");
}

void test_cuda_parity_if_available() {
  if (!torch::cuda::is_available()) {
    std::cout << "cuda_cases=skipped_unavailable\n";
    return;
  }
  const auto qpsm = make_qpsm();
  for (const auto dtype : {torch::kFloat32, torch::kFloat64}) {
    const auto source = encoded(2, dtype, torch::Device(torch::kCUDA));
    const auto actual = production(source, config());
    const auto oracle = shadow(source, qpsm, config());
    const auto replay = production(source, config());
    check_exact(actual, oracle, "CUDA production/shadow parity");
    check_exact(actual, replay, "CUDA production repeat parity");
    check(actual.values.device().is_cuda() &&
              actual.valid_mask.device().is_cuda() &&
              actual.values.scalar_type() == dtype &&
              actual.values.is_contiguous(),
          "CUDA production output contract changed");
    const auto reference = srr::readout_cpu64(source, qpsm, config());
    const auto error =
        (actual.values.to(torch::kCPU, torch::kFloat64) - reference.values)
            .abs()
            .max()
            .item<double>();
    check(error <= 2.0e-5,
          "CUDA production exceeded frozen CPU64 translation bound");
  }
  std::cout << "cuda_cases=passed\n";
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    at::set_num_interop_threads(1);
    test_policy_surface();
    test_projection_thread_independence();
    test_projection_contract();
    test_cpu_parity_and_purity();
    test_metadata_and_partition_behavior();
    test_masks_and_pooled_poison();
    test_legacy_policy_goldens();
    test_rejections();
    test_cuda_parity_if_available();
    std::cout << "SRR-2 production structured readout mechanics passed\n";
    std::cout << "production_shadow_cpu64_bytes_exact=true\n";
    std::cout << "production_shadow_cpu32_bytes_exact=true\n";
    std::cout << "legacy_policy_goldens_exact=true\n";
    std::cout << "training_or_augmentation_used=false\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "SRR-2 production structured readout failure: "
              << error.what() << "\n";
    return 1;
  }
}
