// SPDX-License-Identifier: MIT
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kChannels = 3;
constexpr int64_t kTokensPerChannel = 24;
constexpr int64_t kTokenCount = 72;
constexpr int64_t kLatent = 32;
constexpr int64_t kCells = 16;
constexpr int64_t kGroups = 8;
constexpr int64_t kProjectionInput = kTokensPerChannel * kLatent;

// These are test-owned copies of the sealed scientific contract.  The oracle
// below intentionally does not call the production plan, lift, or readout.
constexpr std::array<int64_t, kTokensPerChannel> kCellIds{
    0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7,
    8, 8, 9, 9, 9, 10, 10, 11, 12, 13, 14, 15};
constexpr std::array<int64_t, kCells> kCellExpected{
    2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1};
constexpr std::array<int64_t, kTokensPerChannel> kGroupIds{
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3,
    4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6, 7};

// Real graph rows are front-padded: H4/H10 observations occupy the suffix of
// the H30 carrier.  These are therefore the windows overlapping [26,30) and
// [20,30), respectively, in both time and frequency domains.
const std::vector<int64_t> kH4Positions{5, 6, 9, 10, 11,
                                        17, 18, 21, 22, 23};
const std::vector<int64_t> kH10Positions{4, 5, 6, 8, 9, 10, 11,
                                         16, 17, 18, 20, 21, 22, 23};
const std::vector<int64_t> kH30Positions{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

// torch::equal is numeric equality: it deliberately treats +0.0 and -0.0 as
// equal.  The complete-row contract is stronger, so compare tensor metadata
// first and then the raw logical element bytes after a lossless device copy.
[[nodiscard]] bool tensor_bitwise_exact(const torch::Tensor &left,
                                        const torch::Tensor &right) {
  if (left.defined() != right.defined()) {
    return false;
  }
  if (!left.defined()) {
    return true;
  }
  if (left.layout() != right.layout() ||
      left.scalar_type() != right.scalar_type() ||
      left.device() != right.device() || left.sizes() != right.sizes() ||
      left.strides() != right.strides() ||
      left.storage_offset() != right.storage_offset() ||
      left.element_size() != right.element_size()) {
    return false;
  }
  if (left.numel() == 0) {
    return true;
  }
  const auto left_bytes = left.detach().to(torch::kCPU).contiguous();
  const auto right_bytes = right.detach().to(torch::kCPU).contiguous();
  const auto byte_count = static_cast<std::size_t>(left.numel()) *
                          static_cast<std::size_t>(left.element_size());
  return std::memcmp(left_bytes.data_ptr(), right_bytes.data_ptr(),
                     byte_count) == 0;
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

[[nodiscard]] torch::Tensor independent_qpsm_cpu64() {
  constexpr uint64_t kTag = 0x7273736d5f74655fULL;
  torch::NoGradGuard no_grad;
  auto dense = torch::empty({kProjectionInput, kLatent}, torch::kFloat64);
  auto dense_a = dense.accessor<double, 2>();
  for (int64_t row = 0; row < kProjectionInput; ++row) {
    for (int64_t column = 0; column < kLatent; ++column) {
      const uint64_t key = splitmix64(
          kTag ^ splitmix64(static_cast<uint64_t>(row)) ^
          splitmix64(static_cast<uint64_t>(column) << 32U));
      dense_a[row][column] = signed_uniform(key);
    }
  }
  auto [q0, upper0] = at::linalg_qr(dense, "reduced");
  const auto signs0 = torch::where(upper0.diagonal().lt(0.0),
                                   -torch::ones_like(upper0.diagonal()),
                                   torch::ones_like(upper0.diagonal()));
  q0 = q0 * signs0.unsqueeze(0);

  auto basis = torch::zeros({kProjectionInput, kLatent}, torch::kFloat64);
  auto basis_a = basis.accessor<double, 2>();
  const double scale = 1.0 / std::sqrt(static_cast<double>(kTokensPerChannel));
  for (int64_t token = 0; token < kTokensPerChannel; ++token) {
    for (int64_t feature = 0; feature < kLatent; ++feature) {
      basis_a[token * kLatent + feature][feature] = scale;
    }
  }
  const auto contrast_seed =
      q0 - basis.matmul(basis.transpose(0, 1).matmul(q0));
  auto [contrast, upper] = at::linalg_qr(contrast_seed, "reduced");
  const auto signs = torch::where(upper.diagonal().lt(0.0),
                                  -torch::ones_like(upper.diagonal()),
                                  torch::ones_like(upper.diagonal()));
  contrast = contrast * signs.unsqueeze(0);
  return (basis / std::sqrt(static_cast<double>(kTokensPerChannel)) +
          std::sqrt(23.0 / 24.0) * contrast)
      .contiguous();
}

[[nodiscard]] const torch::Tensor &oracle_projection() {
  static const torch::Tensor projection = independent_qpsm_cpu64();
  return projection;
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

[[nodiscard]] mtf::mtf_token_metadata_t metadata(const torch::Device &device) {
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
    for (int64_t channel = 0; channel < kChannels; ++channel) {
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
  const auto values = torch::TensorOptions().dtype(dtype).device(device);
  const auto boolean =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  result.embeddings =
      (torch::arange(batch * kTokenCount * kLatent, values)
           .reshape({batch, kTokenCount, kLatent}) /
       1000.0)
          .contiguous();
  result.token_mask = torch::ones({batch, kTokenCount}, boolean);
  result.sample_valid_mask = torch::ones({batch}, boolean);
  result.channel_valid_mask = torch::ones({batch, kChannels}, boolean);
  result.metadata = metadata(device);
  result.pooled_by_channel =
      torch::zeros({batch, kChannels, kLatent}, values);
  return result;
}

struct metadata_record_t {
  int64_t source{0};
  int64_t domain{0};
  int64_t scale{0};
  int64_t start{0};
  int64_t width{0};
};

[[nodiscard]] std::array<std::array<int64_t, kTokensPerChannel>, kChannels>
ordered_indices(const mtf::mtf_token_metadata_t &input) {
  const auto starts = input.start_index.to(torch::kCPU).contiguous();
  const auto widths = input.width.to(torch::kCPU).contiguous();
  const auto scales = input.scale_id.to(torch::kCPU).contiguous();
  const auto channels = input.channel_id.to(torch::kCPU).contiguous();
  const auto domains = input.domain_id.to(torch::kCPU).contiguous();
  check(starts.numel() == kTokenCount && widths.numel() == kTokenCount &&
            scales.numel() == kTokenCount && channels.numel() == kTokenCount &&
            domains.numel() == kTokenCount,
        "oracle metadata width changed");
  const auto start_a = starts.accessor<int64_t, 1>();
  const auto width_a = widths.accessor<int64_t, 1>();
  const auto scale_a = scales.accessor<int64_t, 1>();
  const auto channel_a = channels.accessor<int64_t, 1>();
  const auto domain_a = domains.accessor<int64_t, 1>();
  std::array<std::array<int64_t, kTokensPerChannel>, kChannels> result{};
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    std::vector<metadata_record_t> records;
    for (int64_t source = 0; source < kTokenCount; ++source) {
      if (channel_a[source] == channel) {
        records.push_back({source, domain_a[source], scale_a[source],
                           start_a[source], width_a[source]});
      }
    }
    check(records.size() == kTokensPerChannel,
          "oracle channel does not contain 24 tokens");
    std::sort(records.begin(), records.end(), [](const auto &left,
                                                  const auto &right) {
      return std::tie(left.domain, left.scale, left.start, left.width) <
             std::tie(right.domain, right.scale, right.start, right.width);
    });
    for (int64_t position = 0; position < kTokensPerChannel; ++position) {
      result[static_cast<std::size_t>(channel)]
            [static_cast<std::size_t>(position)] =
          records[static_cast<std::size_t>(position)].source;
    }
  }
  return result;
}

struct oracle_output_t {
  torch::Tensor values{};     // CPU float64
  torch::Tensor valid_mask{}; // CPU bool
};

[[nodiscard]] oracle_output_t
independent_sparse_oracle(
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &source) {
  const auto embeddings =
      source.embeddings.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  const auto token_mask = source.token_mask.to(torch::kCPU).contiguous();
  const auto sample_mask =
      source.sample_valid_mask.to(torch::kCPU).contiguous();
  const auto channel_mask =
      source.channel_valid_mask.to(torch::kCPU).contiguous();
  const auto order = ordered_indices(source.metadata);
  const int64_t batch = embeddings.size(0);
  auto completed = torch::zeros(
      {batch, kChannels, kTokensPerChannel, kLatent}, torch::kFloat64);
  auto valid = torch::zeros({batch, kChannels}, torch::kBool);
  const auto embedding_a = embeddings.accessor<double, 3>();
  const auto token_a = token_mask.accessor<bool, 2>();
  const auto sample_a = sample_mask.accessor<bool, 1>();
  const auto channel_a = channel_mask.accessor<bool, 2>();
  auto completed_a = completed.accessor<double, 4>();
  auto valid_a = valid.accessor<bool, 2>();

  for (int64_t row = 0; row < batch; ++row) {
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      std::array<std::array<double, kLatent>, kCells> cell_mean{};
      std::array<bool, kCells> cell_supported{};
      for (int64_t cell = 0; cell < kCells; ++cell) {
        int64_t count = 0;
        for (int64_t position = 0; position < kTokensPerChannel; ++position) {
          if (kCellIds[static_cast<std::size_t>(position)] != cell) {
            continue;
          }
          const int64_t source_index =
              order[static_cast<std::size_t>(channel)]
                   [static_cast<std::size_t>(position)];
          if (!token_a[row][source_index]) {
            continue;
          }
          ++count;
          for (int64_t feature = 0; feature < kLatent; ++feature) {
            cell_mean[static_cast<std::size_t>(cell)]
                     [static_cast<std::size_t>(feature)] +=
                embedding_a[row][source_index][feature];
          }
        }
        cell_supported[static_cast<std::size_t>(cell)] = count > 0;
        if (count > 0) {
          for (int64_t feature = 0; feature < kLatent; ++feature) {
            cell_mean[static_cast<std::size_t>(cell)]
                     [static_cast<std::size_t>(feature)] /=
                static_cast<double>(count);
          }
        }
      }

      std::array<std::array<double, kLatent>, kGroups> group_mean{};
      std::array<int64_t, kGroups> group_position_count{};
      for (int64_t position = 0; position < kTokensPerChannel; ++position) {
        const int64_t cell = kCellIds[static_cast<std::size_t>(position)];
        if (!cell_supported[static_cast<std::size_t>(cell)]) {
          continue;
        }
        const int64_t group = kGroupIds[static_cast<std::size_t>(position)];
        ++group_position_count[static_cast<std::size_t>(group)];
        for (int64_t feature = 0; feature < kLatent; ++feature) {
          group_mean[static_cast<std::size_t>(group)]
                    [static_cast<std::size_t>(feature)] +=
              cell_mean[static_cast<std::size_t>(cell)]
                       [static_cast<std::size_t>(feature)];
        }
      }
      bool all_groups = true;
      for (int64_t group = 0; group < kGroups; ++group) {
        const int64_t count =
            group_position_count[static_cast<std::size_t>(group)];
        all_groups = all_groups && count > 0;
        if (count > 0) {
          for (int64_t feature = 0; feature < kLatent; ++feature) {
            group_mean[static_cast<std::size_t>(group)]
                      [static_cast<std::size_t>(feature)] /=
                static_cast<double>(count);
          }
        }
      }
      const bool computable = all_groups && sample_a[row] &&
                              channel_a[row][channel];
      valid_a[row][channel] = computable;
      if (!computable) {
        continue;
      }
      for (int64_t position = 0; position < kTokensPerChannel; ++position) {
        const int64_t cell = kCellIds[static_cast<std::size_t>(position)];
        const int64_t group = kGroupIds[static_cast<std::size_t>(position)];
        for (int64_t feature = 0; feature < kLatent; ++feature) {
          completed_a[row][channel][position][feature] =
              cell_supported[static_cast<std::size_t>(cell)]
                  ? cell_mean[static_cast<std::size_t>(cell)]
                             [static_cast<std::size_t>(feature)]
                  : group_mean[static_cast<std::size_t>(group)]
                              [static_cast<std::size_t>(feature)];
        }
      }
    }
  }
  auto values = completed.reshape({batch, kChannels, kProjectionInput})
                    .matmul(oracle_projection())
                    .contiguous();
  values = torch::where(valid.unsqueeze(-1), values, torch::zeros_like(values));
  return {.values = std::move(values), .valid_mask = std::move(valid)};
}

[[nodiscard]] mtf::mtf_serving_pool_output_t
candidate(const mtf::mtf_jepa_mae_vicreg_encode_output_t &source) {
  return mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
      config());
}

[[nodiscard]] mtf::mtf_serving_pool_output_t
complete_v1(const mtf::mtf_jepa_mae_vicreg_encode_output_t &source) {
  return mtf::select_mtf_serving_pool(
      source, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1, config());
}

void check_oracle(const mtf::mtf_serving_pool_output_t &actual,
                  const oracle_output_t &expected, double tolerance,
                  const std::string &context) {
  check(torch::equal(actual.valid_mask.to(torch::kCPU), expected.valid_mask),
        context + ": oracle mask mismatch");
  const auto translated = actual.values.to(torch::kCPU, torch::kFloat64);
  const double error =
      (translated - expected.values).abs().max().item<double>();
  check(error <= tolerance,
        context + ": oracle value error " + std::to_string(error));
}

void fill_channel_pattern(
    torch::Tensor &mask, int64_t row, int64_t channel,
    const std::vector<int64_t> &positions,
    const std::array<std::array<int64_t, kTokensPerChannel>, kChannels>
        &order) {
  auto mask_a = mask.accessor<bool, 2>();
  for (int64_t position = 0; position < kTokensPerChannel; ++position) {
    const int64_t source = order[static_cast<std::size_t>(channel)]
                                [static_cast<std::size_t>(position)];
    mask_a[row][source] = false;
  }
  for (const int64_t position : positions) {
    const int64_t source = order[static_cast<std::size_t>(channel)]
                                [static_cast<std::size_t>(position)];
    mask_a[row][source] = true;
  }
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_encode_output_t
actual_surface(torch::Dtype dtype, const torch::Device &device,
               int64_t batch = 1) {
  auto result = encoded(batch, dtype, device);
  auto mask = torch::zeros({batch, kTokenCount}, torch::kBool);
  const auto order = ordered_indices(metadata(torch::Device(torch::kCPU)));
  for (int64_t row = 0; row < batch; ++row) {
    fill_channel_pattern(mask, row, 0, kH4Positions, order);
    fill_channel_pattern(mask, row, 1, kH10Positions, order);
    fill_channel_pattern(mask, row, 2, kH30Positions, order);
  }
  result.token_mask = mask.to(device);
  return result;
}

void mask_position(mtf::mtf_jepa_mae_vicreg_encode_output_t &source,
                   int64_t row, int64_t channel, int64_t position,
                   bool value) {
  const auto order = ordered_indices(source.metadata);
  auto mask = source.token_mask.to(torch::kCPU).contiguous().clone();
  mask.accessor<bool, 2>()[row]
                          [order[static_cast<std::size_t>(channel)]
                                [static_cast<std::size_t>(position)]] = value;
  source.token_mask = mask.to(source.embeddings.device());
}

[[nodiscard]] mtf::mtf_token_metadata_t
permuted_metadata(const mtf::mtf_token_metadata_t &source,
                  const torch::Tensor &permutation) {
  return {.start_index = source.start_index.index_select(0, permutation),
          .width = source.width.index_select(0, permutation),
          .scale_id = source.scale_id.index_select(0, permutation),
          .channel_id = source.channel_id.index_select(0, permutation),
          .domain_id = source.domain_id.index_select(0, permutation)};
}

void test_raw_bitwise_comparator_canary() {
  const auto positive64 =
      torch::tensor(std::vector<double>{0.0, 1.0}, torch::kFloat64);
  const auto negative64 =
      torch::tensor(std::vector<double>{-0.0, 1.0}, torch::kFloat64);
  const auto positive32 =
      torch::tensor(std::vector<float>{0.0F, 1.0F}, torch::kFloat32);
  const auto negative32 =
      torch::tensor(std::vector<float>{-0.0F, 1.0F}, torch::kFloat32);
  check(torch::equal(positive64, negative64) &&
            torch::equal(positive32, negative32),
        "signed-zero canary no longer distinguishes numeric from bitwise "
        "equality");
  check(std::signbit(negative64.index({0}).item<double>()) &&
            std::signbit(negative32.index({0}).item<float>()),
        "signed-zero canary did not retain the negative sign bit");
  check(!tensor_bitwise_exact(positive64, negative64) &&
            !tensor_bitwise_exact(positive32, negative32),
        "raw comparator accepted +0.0 as byte-exact with -0.0");
  check(tensor_bitwise_exact(positive64, positive64.clone()) &&
            tensor_bitwise_exact(positive32, positive32.clone()),
        "raw comparator rejected an exact CPU clone");
  check(!tensor_bitwise_exact(positive64, positive32),
        "raw comparator ignored dtype");

  const auto strided =
      torch::arange(12, torch::kFloat64).reshape({3, 4}).transpose(0, 1);
  const auto contiguous = strided.contiguous();
  check(torch::equal(strided, contiguous) &&
            !tensor_bitwise_exact(strided, contiguous),
        "raw comparator ignored tensor strides");

  if (torch::cuda::is_available()) {
    const auto cuda = torch::Device(torch::kCUDA);
    const auto positive_cuda = positive32.to(cuda);
    const auto negative_cuda = negative32.to(cuda);
    check(!tensor_bitwise_exact(positive_cuda, negative_cuda),
          "raw comparator accepted CUDA +0.0 as byte-exact with -0.0");
    check(tensor_bitwise_exact(positive_cuda, positive_cuda.clone()),
          "raw comparator rejected an exact CUDA clone");
    check(!tensor_bitwise_exact(positive32, positive_cuda),
          "raw comparator ignored device");
  }
}

void test_suffix_feature_mask_tokenizer_surface() {
  using torch::indexing::Slice;
  torch::NoGradGuard no_grad;
  auto cfg = config();
  cfg.device = torch::Device(torch::kCPU);
  cfg.dtype = torch::kFloat32;
  mtf::TimeFrequencyViewBuilder tokenizer(cfg);
  tokenizer->eval();
  const auto input = torch::zeros({1, kChannels, 30, 9}, torch::kFloat32);
  auto feature_mask = torch::zeros({1, kChannels, 30, 9}, torch::kBool);
  feature_mask.index_put_({0, 0, Slice(26, 30), Slice()}, true);
  feature_mask.index_put_({0, 1, Slice(20, 30), Slice()}, true);
  feature_mask.index_put_({0, 2, Slice(), Slice()}, true);
  const auto tokenized = tokenizer->forward(input, feature_mask);
  const auto expected =
      actual_surface(torch::kFloat32, torch::Device(torch::kCPU));
  check(torch::equal(tokenized.token_mask, expected.token_mask),
        "front-padded suffix feature masks did not derive frozen H4/H10/H30 "
        "token support");
  check(torch::equal(tokenized.metadata.start_index,
                     expected.metadata.start_index) &&
            torch::equal(tokenized.metadata.width, expected.metadata.width) &&
            torch::equal(tokenized.metadata.scale_id,
                         expected.metadata.scale_id) &&
            torch::equal(tokenized.metadata.channel_id,
                         expected.metadata.channel_id) &&
            torch::equal(tokenized.metadata.domain_id,
                         expected.metadata.domain_id),
        "tokenizer metadata no longer matches the frozen 72-token layout");
  const auto grouped = mtf::detail::structured_cdsb_sparse_v1_lift(
      {.embeddings = torch::zeros({1, kTokenCount, kLatent}, torch::kFloat32),
       .pooled_embedding = torch::Tensor{},
       .pooled_by_channel = torch::zeros({1, kChannels, kLatent},
                                         torch::kFloat32),
       .pooled_time = torch::Tensor{},
       .pooled_frequency = torch::Tensor{},
       .token_mask = tokenized.token_mask,
       .sample_valid_mask = tokenized.token_mask.any(1),
       .channel_valid_mask = torch::ones({1, kChannels}, torch::kBool),
       .metadata = tokenized.metadata},
      cfg);
  check(torch::equal(grouped.grouped_token_mask.sum(2),
                     torch::tensor({{10, 14, 24}}, torch::kInt64)),
        "tokenizer-derived suffix support is not H4/H10/H30 = 10/14/24");
}

void test_policy_surface() {
  check(static_cast<int>(mtf::mtf_serving_pool_policy_t::all_tokens) == 0 &&
            static_cast<int>(mtf::mtf_serving_pool_policy_t::time_only) == 1 &&
            static_cast<int>(
                mtf::mtf_serving_pool_policy_t::frequency_only) == 2 &&
            static_cast<int>(
                mtf::mtf_serving_pool_policy_t::domain_balanced) == 3 &&
            static_cast<int>(
                mtf::mtf_serving_pool_policy_t::structured_cdsb_v1) == 4 &&
            static_cast<int>(
                mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1) ==
                5,
        "sparse policy was not append-only at ordinal 5");
  check(std::string(mtf::mtf_serving_pool_policy_name(
            mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1)) ==
            "structured_cdsb_sparse_v1",
        "sparse policy name changed");
  check(mtf::mtf_jepa_mae_vicreg_config_t{}.serving_pool_policy ==
            mtf::mtf_serving_pool_policy_t::all_tokens &&
            config().serving_pool_policy ==
                mtf::mtf_serving_pool_policy_t::all_tokens,
        "default or fixture policy is no longer all_tokens");
  check(mtf::mtf_jepa_mae_vicreg_spec_detail::parse_serving_pool_policy(
            " structured_CDSB_sparse_v1 ") ==
            mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        "parser did not recognize sparse policy");
  expect_throw(
      [] {
        (void)mtf::mtf_jepa_mae_vicreg_spec_detail::
            parse_serving_pool_policy("structured_cdsb_sparse_v2");
      },
      "parser accepted an unknown sparse policy");
  expect_throw(
      [] {
        (void)mtf::mtf_serving_pool_policy_name(
            static_cast<mtf::mtf_serving_pool_policy_t>(999));
      },
      "policy name accepted an unknown ordinal");

  auto configured = config();
  configured.serving_pool_policy =
      mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1;
  mtf::detail::validate_architecture_config(configured);
  configured.history_length = 29;
  expect_throw([&] { mtf::detail::validate_architecture_config(configured); },
               "sparse configured policy accepted unsupported H=29");
  expect_throw(
      [&] {
        (void)mtf::select_mtf_serving_pool(
            encoded(1, torch::kFloat64, torch::Device(torch::kCPU)),
            mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
            configured);
      },
      "sparse selector accepted unsupported configuration");
}

void check_actual_diagnostics(
    const mtf::detail::structured_cdsb_sparse_v1_lifted_t &lifted,
    const torch::Device &device) {
  const auto int_options =
      torch::TensorOptions().dtype(torch::kInt64).device(device);
  const auto bool_options =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  check(torch::equal(lifted.grouped_token_mask.sum(2),
                     torch::tensor({{10, 14, 24}}, int_options)),
        "actual H4/H10/H30 source-token counts are not 10/14/24");
  check(torch::equal(lifted.cell_support_mask.sum(2),
                     torch::tensor({{8, 12, 16}}, int_options)),
        "actual H4/H10/H30 supported-cell counts are not 8/12/16");
  check(torch::equal(lifted.repeated_support_position_count,
                     torch::tensor({{10, 18, 24}}, int_options)),
        "actual H4/H10/H30 repeated support is not 10/18/24");
  check(torch::equal(lifted.cell_source_token_count.sum(2),
                     torch::tensor({{10, 14, 24}}, int_options)),
        "cell source-token audit does not sum to grouped support");
  check(torch::equal(
            lifted.cell_source_token_count.index({0}),
            torch::tensor(
                {{0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 2, 0, 0, 1, 1, 1},
                 {0, 1, 2, 0, 1, 1, 1, 1, 0, 1, 2, 0, 1, 1, 1, 1},
                 {2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1}},
                int_options)),
        "suffix-derived H4/H10/H30 per-cell source counts changed");
  const std::vector<int64_t> expected(kCellExpected.begin(),
                                      kCellExpected.end());
  check(torch::equal(lifted.cell_expected_token_count,
                     torch::tensor(expected, int_options)),
        "cell expected-token counts changed");
  check(torch::equal(lifted.complete_mask,
                     torch::tensor({{false, false, true}}, bool_options)),
        "actual H4/H10/H30 complete mask changed");
  check(lifted.domain_scale_support_mask.all().item<bool>(),
        "actual sparse patterns do not cover every domain-scale group");
  check(lifted.values.sizes() ==
            torch::IntArrayRef({1, kChannels, kProjectionInput}) &&
            lifted.valid_mask.sizes() == torch::IntArrayRef({1, kChannels}) &&
            lifted.grouped_token_mask.sizes() ==
                torch::IntArrayRef({1, kChannels, kTokensPerChannel}) &&
            lifted.cell_support_mask.sizes() ==
                torch::IntArrayRef({1, kChannels, kCells}) &&
            lifted.cell_source_token_count.sizes() ==
                torch::IntArrayRef({1, kChannels, kCells}) &&
            lifted.domain_scale_support_mask.sizes() ==
                torch::IntArrayRef({1, kChannels, kGroups}),
        "sparse diagnostics shape changed");
  check(lifted.values.device() == device, "lift values left input device");
  check(lifted.valid_mask.device() == device,
        "valid_mask left input device");
  check(lifted.complete_mask.device() == device,
        "complete_mask left input device");
  check(lifted.grouped_token_mask.device() == device,
        "grouped_token_mask left input device");
  check(lifted.cell_support_mask.device() == device,
        "cell_support_mask left input device");
  check(lifted.cell_source_token_count.device() == device,
        "cell_source_token_count left input device");
  check(lifted.cell_expected_token_count.device() == device,
        "cell_expected_token_count left input device");
  check(lifted.domain_scale_support_mask.device() == device,
        "domain_scale_support_mask left input device");
  check(lifted.repeated_support_position_count.device() == device,
        "repeated_support_position_count left input device");
  check(lifted.values.is_contiguous() && lifted.valid_mask.is_contiguous() &&
            lifted.complete_mask.is_contiguous() &&
            lifted.grouped_token_mask.is_contiguous() &&
            lifted.cell_support_mask.is_contiguous() &&
            lifted.cell_source_token_count.is_contiguous() &&
            lifted.cell_expected_token_count.is_contiguous() &&
            lifted.domain_scale_support_mask.is_contiguous() &&
            lifted.repeated_support_position_count.is_contiguous(),
        "sparse diagnostics are not contiguous");
  check(lifted.grouped_token_mask.scalar_type() == torch::kBool &&
            lifted.cell_support_mask.scalar_type() == torch::kBool &&
            lifted.domain_scale_support_mask.scalar_type() == torch::kBool &&
            lifted.cell_source_token_count.scalar_type() == torch::kInt64 &&
            lifted.cell_expected_token_count.scalar_type() == torch::kInt64 &&
            lifted.repeated_support_position_count.scalar_type() ==
                torch::kInt64,
        "sparse diagnostic dtypes changed");
}

void test_actual_surface(torch::Dtype dtype, const torch::Device &device) {
  const double tolerance =
      dtype == torch::kFloat64 && device.is_cpu() ? 1.0e-12 : 2.0e-5;
  const auto source = actual_surface(dtype, device);
  const auto lifted = mtf::detail::structured_cdsb_sparse_v1_lift(
      source, config());
  check_actual_diagnostics(lifted, source.embeddings.device());
  const auto sparse = candidate(source);
  const auto legacy = complete_v1(source);
  check_oracle(sparse, independent_sparse_oracle(source), tolerance,
               "actual sparse surface");
  check(sparse.valid_mask.all().item<bool>(),
        "sparse policy did not make all actual channels computable");
  check(torch::equal(
            legacy.valid_mask,
            torch::tensor({{false, false, true}},
                          torch::TensorOptions().dtype(torch::kBool).device(
                              device))),
        "v1 no longer accepts only the complete H30 channel");
  check(tensor_bitwise_exact(sparse.values.index({0, 2}),
                             legacy.values.index({0, 2})) &&
            tensor_bitwise_exact(sparse.valid_mask.index({0, 2}),
                                 legacy.valid_mask.index({0, 2})),
        "complete H30 channel is not byte-exact with v1");
  check(sparse.values.sizes() == torch::IntArrayRef({1, 3, 32}) &&
            sparse.valid_mask.sizes() == torch::IntArrayRef({1, 3}) &&
            sparse.values.scalar_type() == dtype &&
            sparse.valid_mask.scalar_type() == torch::kBool &&
            sparse.values.device() == source.embeddings.device() &&
            sparse.valid_mask.device() == source.embeddings.device() &&
            sparse.values.is_contiguous() &&
            torch::isfinite(sparse.values).all().item<bool>(),
        "sparse public value/mask contract changed");
}

void test_mixed_complete_parity(torch::Dtype dtype,
                                const torch::Device &device) {
  auto source = encoded(3, dtype, device);
  auto mask = torch::zeros({3, kTokenCount}, torch::kBool);
  const auto order = ordered_indices(metadata(torch::Device(torch::kCPU)));
  fill_channel_pattern(mask, 0, 0, kH4Positions, order);
  fill_channel_pattern(mask, 0, 1, kH30Positions, order);
  fill_channel_pattern(mask, 0, 2, kH10Positions, order);
  fill_channel_pattern(mask, 1, 0, kH30Positions, order);
  fill_channel_pattern(mask, 1, 1, kH4Positions, order);
  fill_channel_pattern(mask, 1, 2, kH30Positions, order);
  fill_channel_pattern(mask, 2, 0, kH30Positions, order);
  fill_channel_pattern(mask, 2, 1, kH30Positions, order);
  fill_channel_pattern(mask, 2, 2, kH30Positions, order);
  source.token_mask = mask.to(device);

  const auto lifted = mtf::detail::structured_cdsb_sparse_v1_lift(
      source, config());
  const auto sparse = candidate(source);
  const auto legacy = complete_v1(source);
  check(lifted.complete_mask.any().item<bool>() &&
            lifted.complete_mask.logical_not().any().item<bool>(),
        "mixed batch does not contain complete and sparse channels");
  const auto complete_cpu = lifted.complete_mask.to(torch::kCPU).contiguous();
  const auto complete_a = complete_cpu.accessor<bool, 2>();
  for (int64_t row = 0; row < complete_cpu.size(0); ++row) {
    for (int64_t channel = 0; channel < complete_cpu.size(1); ++channel) {
      if (!complete_a[row][channel]) {
        continue;
      }
      check(tensor_bitwise_exact(sparse.values.index({row, channel}),
                                 legacy.values.index({row, channel})),
            "mixed-batch complete row is not byte-exact with v1");
      check(tensor_bitwise_exact(sparse.valid_mask.index({row, channel}),
                                 legacy.valid_mask.index({row, channel})),
            "mixed-batch complete mask is not byte-exact with v1");
    }
  }
}

[[nodiscard]] std::vector<mtf::mtf_jepa_mae_vicreg_encode_output_t>
sparse_variants(torch::Dtype dtype, const torch::Device &device) {
  std::vector<mtf::mtf_jepa_mae_vicreg_encode_output_t> result;
  auto single_missing = encoded(1, dtype, device);
  mask_position(single_missing, 0, 0, 0, false);
  result.push_back(std::move(single_missing));

  auto partial_cell = encoded(1, dtype, device);
  mask_position(partial_cell, 0, 0, 3, false);
  mask_position(partial_cell, 0, 0, 4, false);
  result.push_back(std::move(partial_cell));

  auto empty_cell = encoded(1, dtype, device);
  mask_position(empty_cell, 0, 0, 0, false);
  mask_position(empty_cell, 0, 0, 1, false);
  result.push_back(std::move(empty_cell));
  return result;
}

void test_sparse_variants(torch::Dtype dtype, const torch::Device &device) {
  const double tolerance =
      dtype == torch::kFloat64 && device.is_cpu() ? 1.0e-12 : 2.0e-5;
  int variant = 0;
  for (const auto &source : sparse_variants(dtype, device)) {
    const auto sparse = candidate(source);
    check(sparse.valid_mask.all().item<bool>(),
          "supported sparse variant became invalid");
    check_oracle(sparse, independent_sparse_oracle(source), tolerance,
                 "sparse variant " + std::to_string(variant));
    ++variant;
  }
}

void set_constant_embeddings(
    mtf::mtf_jepa_mae_vicreg_encode_output_t &source) {
  const auto meta = metadata(torch::Device(torch::kCPU));
  const auto channels = meta.channel_id.accessor<int64_t, 1>();
  auto values = torch::zeros({source.embeddings.size(0), kTokenCount, kLatent},
                             torch::kFloat64);
  auto value_a = values.accessor<double, 3>();
  for (int64_t row = 0; row < values.size(0); ++row) {
    for (int64_t token = 0; token < kTokenCount; ++token) {
      for (int64_t feature = 0; feature < kLatent; ++feature) {
        value_a[row][token][feature] =
            1.25 + 0.5 * static_cast<double>(channels[token]) +
            0.01 * static_cast<double>(feature);
      }
    }
  }
  source.embeddings = values.to(source.embeddings.device(),
                                source.embeddings.scalar_type());
}

void set_domain_scale_embeddings(
    mtf::mtf_jepa_mae_vicreg_encode_output_t &source) {
  const auto meta = metadata(torch::Device(torch::kCPU));
  const auto channels = meta.channel_id.accessor<int64_t, 1>();
  const auto domains = meta.domain_id.accessor<int64_t, 1>();
  const auto scales = meta.scale_id.accessor<int64_t, 1>();
  auto values = torch::zeros({source.embeddings.size(0), kTokenCount, kLatent},
                             torch::kFloat64);
  auto value_a = values.accessor<double, 3>();
  for (int64_t row = 0; row < values.size(0); ++row) {
    for (int64_t token = 0; token < kTokenCount; ++token) {
      const int64_t group = domains[token] * 4 + scales[token];
      for (int64_t feature = 0; feature < kLatent; ++feature) {
        value_a[row][token][feature] =
            0.1 * static_cast<double>(channels[token] + 1) +
            0.2 * static_cast<double>(group) +
            0.003 * static_cast<double>(feature);
      }
    }
  }
  source.embeddings = values.to(source.embeddings.device(),
                                source.embeddings.scalar_type());
}

void test_constant_canaries(torch::Dtype dtype,
                            const torch::Device &device) {
  const double tolerance =
      dtype == torch::kFloat64 && device.is_cpu() ? 1.0e-12 : 2.0e-5;
  auto patterns = sparse_variants(dtype, device);
  patterns.push_back(actual_surface(dtype, device));
  int pattern = 0;
  for (auto source : patterns) {
    set_constant_embeddings(source);
    const auto actual = candidate(source).values.to(torch::kCPU,
                                                     torch::kFloat64);
    auto expected = torch::zeros({1, kChannels, kLatent}, torch::kFloat64);
    auto expected_a = expected.accessor<double, 3>();
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      for (int64_t feature = 0; feature < kLatent; ++feature) {
        expected_a[0][channel][feature] =
            1.25 + 0.5 * static_cast<double>(channel) +
            0.01 * static_cast<double>(feature);
      }
    }
    const double error = (actual - expected).abs().max().item<double>();
    check(error <= tolerance,
          "constant preservation failed for pattern " +
              std::to_string(pattern));
    ++pattern;
  }

  patterns = sparse_variants(dtype, device);
  patterns.push_back(actual_surface(dtype, device));
  pattern = 0;
  for (auto source : patterns) {
    set_domain_scale_embeddings(source);
    auto complete = source;
    complete.token_mask = torch::ones_like(source.token_mask);
    const auto expected = independent_sparse_oracle(complete);
    check_oracle(candidate(source), expected, tolerance,
                 "domain-scale constant pattern " +
                     std::to_string(pattern));
    ++pattern;
  }
}

void test_missing_poison_and_upstream(torch::Dtype dtype,
                                      const torch::Device &device) {
  auto source = actual_surface(dtype, device);
  const auto baseline = candidate(source);
  auto poisoned = source;
  poisoned.embeddings = torch::where(
      source.token_mask.unsqueeze(-1), source.embeddings,
      torch::full_like(source.embeddings, 1.0e6));
  poisoned.pooled_by_channel = torch::full_like(
      source.pooled_by_channel, std::numeric_limits<double>::infinity());
  const auto poisoned_result = candidate(poisoned);
  check(torch::equal(baseline.values, poisoned_result.values) &&
            torch::equal(baseline.valid_mask, poisoned_result.valid_mask),
        "unsupported-token or pooled poison reached sparse output");

  const auto order = ordered_indices(source.metadata);
  auto unsupported_perturbation = source;
  unsupported_perturbation.embeddings = source.embeddings.clone();
  const int64_t unsupported = order[0][2]; // H4 cell 1 is unsupported.
  unsupported_perturbation.embeddings.index({0, unsupported}).add_(100.0);
  check(torch::equal(baseline.values,
                     candidate(unsupported_perturbation).values),
        "perturbation confined to unsupported cell was visible");

  auto supported_perturbation = source;
  supported_perturbation.embeddings = source.embeddings.clone();
  const int64_t supported = order[0][10]; // Required singleton group.
  supported_perturbation.embeddings.index({0, supported}).add_(0.5);
  check((baseline.values - candidate(supported_perturbation).values)
                .abs()
                .max()
                .item<double>() > 1.0e-6,
        "supported-cell perturbation was invisible");

  auto missing_group = source;
  mask_position(missing_group, 0, 0, 10, false);
  const auto missing_lift = mtf::detail::structured_cdsb_sparse_v1_lift(
      missing_group, config());
  const auto missing_result = candidate(missing_group);
  check(!missing_lift.domain_scale_support_mask.index({0, 0, 2})
             .item<bool>() &&
            !missing_result.valid_mask.index({0, 0}).item<bool>() &&
            missing_result.values.index({0, 0}).eq(0).all().item<bool>() &&
            missing_result.valid_mask.index({0, 1}).item<bool>() &&
            missing_result.valid_mask.index({0, 2}).item<bool>(),
        "missing domain-scale group did not fail exactly one channel closed");

  auto channel_invalid = source;
  channel_invalid.channel_valid_mask = source.channel_valid_mask.clone();
  channel_invalid.channel_valid_mask.index_put_({0, 1}, false);
  const auto channel_result = candidate(channel_invalid);
  check(!channel_result.valid_mask.index({0, 1}).item<bool>() &&
            channel_result.values.index({0, 1}).eq(0).all().item<bool>(),
        "upstream-invalid channel was not exactly zero");

  auto sample_invalid = source;
  sample_invalid.sample_valid_mask = source.sample_valid_mask.clone();
  sample_invalid.sample_valid_mask.index_put_({0}, false);
  const auto sample_result = candidate(sample_invalid);
  check(!sample_result.valid_mask.any().item<bool>() &&
            sample_result.values.eq(0).all().item<bool>(),
        "upstream-invalid sample was not exactly zero");

  auto all_invalid = source;
  all_invalid.token_mask.zero_();
  const auto invalid_result = candidate(all_invalid);
  check(!invalid_result.valid_mask.any().item<bool>() &&
            invalid_result.values.eq(0).all().item<bool>(),
        "all-invalid token input was not exactly zero");
}

void test_permutation_rng_and_purity(torch::Dtype dtype,
                                     const torch::Device &device) {
  const auto source = actual_surface(dtype, device);
  const auto embeddings_before = source.embeddings.clone();
  const auto token_before = source.token_mask.clone();
  const auto sample_before = source.sample_valid_mask.clone();
  const auto channel_before = source.channel_valid_mask.clone();
  const auto pooled_before = source.pooled_by_channel.clone();
  const auto start_before = source.metadata.start_index.clone();
  const auto width_before = source.metadata.width.clone();
  const auto scale_before = source.metadata.scale_id.clone();
  const auto meta_channel_before = source.metadata.channel_id.clone();
  const auto domain_before = source.metadata.domain_id.clone();
  const auto projection_before =
      mtf::detail::structured_cdsb_v1_projection_for(
          source.embeddings.options())
          .clone();

  torch::manual_seed(3107);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(3107);
  }
  const auto expected_cpu = torch::rand({17}, torch::kFloat32);
  torch::Tensor expected_device;
  if (device.is_cuda()) {
    expected_device = torch::rand(
        {17}, torch::TensorOptions().dtype(torch::kFloat32).device(device));
  }
  torch::manual_seed(3107);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(3107);
  }
  const auto baseline = candidate(source);
  const auto replay = candidate(source);
  const auto actual_cpu = torch::rand({17}, torch::kFloat32);
  torch::Tensor actual_device;
  if (device.is_cuda()) {
    actual_device = torch::rand(
        {17}, torch::TensorOptions().dtype(torch::kFloat32).device(device));
  }
  check(torch::equal(baseline.values, replay.values) &&
            torch::equal(baseline.valid_mask, replay.valid_mask),
        "repeated sparse call was not deterministic");
  check(torch::equal(expected_cpu, actual_cpu) &&
            (!device.is_cuda() ||
             torch::equal(expected_device, actual_device)),
        "sparse readout consumed CPU or CUDA RNG");
  check(torch::equal(source.embeddings, embeddings_before) &&
            torch::equal(source.token_mask, token_before) &&
            torch::equal(source.sample_valid_mask, sample_before) &&
            torch::equal(source.channel_valid_mask, channel_before) &&
            torch::equal(source.pooled_by_channel, pooled_before) &&
            torch::equal(source.metadata.start_index, start_before) &&
            torch::equal(source.metadata.width, width_before) &&
            torch::equal(source.metadata.scale_id, scale_before) &&
            torch::equal(source.metadata.channel_id, meta_channel_before) &&
            torch::equal(source.metadata.domain_id, domain_before),
        "sparse readout mutated encoded input");
  check(torch::equal(
            projection_before,
            mtf::detail::structured_cdsb_v1_projection_for(
                source.embeddings.options())),
        "sparse readout mutated fixed projection state");

  const auto permutation = torch::arange(
      kTokenCount - 1, -1, -1,
      torch::TensorOptions().dtype(torch::kInt64).device(device));
  auto permuted = source;
  permuted.embeddings = source.embeddings.index_select(1, permutation);
  permuted.token_mask = source.token_mask.index_select(1, permutation);
  permuted.metadata = permuted_metadata(source.metadata, permutation);
  const auto permuted_result = candidate(permuted);
  check(torch::equal(baseline.values, permuted_result.values) &&
            torch::equal(baseline.valid_mask, permuted_result.valid_mask),
        "joint token/metadata permutation changed sparse result");
}

void run_device_suite(torch::Dtype dtype, const torch::Device &device) {
  test_actual_surface(dtype, device);
  test_mixed_complete_parity(dtype, device);
  test_sparse_variants(dtype, device);
  test_constant_canaries(dtype, device);
  test_missing_poison_and_upstream(dtype, device);
  test_permutation_rng_and_purity(dtype, device);
}

void test_cuda_if_available() {
  if (!torch::cuda::is_available()) {
    std::cout << "cuda_float32_cases=skipped_unavailable\n";
    return;
  }
  run_device_suite(torch::kFloat32, torch::Device(torch::kCUDA));
  std::cout << "cuda_float32_cases=passed\n";
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    at::set_num_interop_threads(1);
    test_policy_surface();
    test_raw_bitwise_comparator_canary();
    test_suffix_feature_mask_tokenizer_surface();
    run_device_suite(torch::kFloat64, torch::Device(torch::kCPU));
    run_device_suite(torch::kFloat32, torch::Device(torch::kCPU));
    test_cuda_if_available();
    std::cout << "SRR-4 sparse structured readout mechanics passed\n";
    std::cout << "independent_domain_scale_oracle=true\n";
    std::cout << "actual_support_counts=10/14/24\n";
    std::cout << "actual_cell_counts=8/12/16\n";
    std::cout << "actual_repeated_support=10/18/24\n";
    std::cout << "suffix_feature_mask_tokenizer_derived=true\n";
    std::cout << "complete_v1_bytes_exact=true\n";
    std::cout << "signed_zero_byte_canary=passed\n";
    std::cout << "training_or_augmentation_used=false\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "SRR-4 sparse structured readout mechanics failure: "
              << error.what() << "\n";
    return 1;
  }
}
