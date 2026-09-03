// SPDX-License-Identifier: MIT

#include <ATen/CPUGeneratorImpl.h>
#include <ATen/cuda/CUDAGeneratorImpl.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/cuda.h>
#include <torch/torch.h>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "piaabo/digest/sha256.h"
#include "ujcamei/source/registry/types/data.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"
#include "wikimyei/representation/encoding/vicreg/channel_representation_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_batch.h"

namespace fs = std::filesystem;
namespace protocol = cuwacunu::kikijyeba::protocol;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
namespace repstream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace inference_detail = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

namespace {

constexpr int64_t kSeed = 31;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kLatentDim = 32;
constexpr int64_t kFeatureCount = 3 * kLatentDim;
constexpr int64_t kCloseFeatureIndex = 3;
constexpr int64_t kDevelopmentBegin = 0;
constexpr int64_t kDevelopmentEnd = 730;
constexpr int64_t kConfirmationBegin = 760;
constexpr int64_t kConfirmationEnd = 1088;
constexpr int64_t kFinalHoldoutBegin = 1088;
constexpr int64_t kFinalHoldoutEnd = 1170;
constexpr int64_t kCompactCellCount = 16;

constexpr const char *kSealedProtocolSha256 =
    "a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30";
constexpr const char *kExpectedConfigSha256 =
    "23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0";
constexpr uintmax_t kExpectedConfigSize = 4298;
constexpr const char *kExpectedRepresentationCheckpointSha256 =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr uintmax_t kExpectedRepresentationCheckpointSize = 853867;
constexpr const char *kExpectedDevelopmentBaselineSha256 =
    "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed";
constexpr uintmax_t kExpectedDevelopmentBaselineSize = 13648442;
constexpr const char *kExpectedConfirmationBaselineSha256 =
    "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7";
constexpr uintmax_t kExpectedConfirmationBaselineSize = 6133066;

constexpr std::array<int64_t, kChannelCount> kExpectedSourceTokenCount = {
    10, 14, 24};
constexpr std::array<int64_t, kChannelCount> kExpectedSupportedCellCount = {
    8, 12, 16};
constexpr std::array<int64_t, kChannelCount>
    kExpectedRepeatedSupportPositionCount = {10, 18, 24};
constexpr std::array<int64_t, kCompactCellCount> kExpectedCellTokenCount = {
    2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1};

struct Options {
  fs::path config_path{};
  fs::path representation_checkpoint_path{};
  fs::path output_dir{};
  int64_t begin{-1};
  int64_t end{-1};
  int64_t batch_ceiling{0};
  uintmax_t expected_baseline_size{0};
  std::string range_id{};
  std::string expected_baseline_sha256{};
};

struct GeneratorStateSnapshot {
  torch::Tensor cpu_state{};
  torch::Tensor cuda_state{};
};

struct ModuleStateSnapshot {
  std::vector<torch::Tensor> parameters{};
  std::vector<torch::Tensor> buffers{};
};

struct SupportAudit {
  std::array<int64_t, kChannelCount> row_count{};
  std::array<int64_t, kChannelCount> complete_row_count{};
  std::array<int64_t, kChannelCount> source_token_count_min{};
  std::array<int64_t, kChannelCount> source_token_count_max{};
  std::array<int64_t, kChannelCount> supported_cell_count_min{};
  std::array<int64_t, kChannelCount> supported_cell_count_max{};
  std::array<int64_t, kChannelCount> repeated_support_count_min{};
  std::array<int64_t, kChannelCount> repeated_support_count_max{};
  std::array<std::array<int64_t, kCompactCellCount>, kChannelCount>
      cell_source_count_min{};
  std::array<std::array<int64_t, kCompactCellCount>, kChannelCount>
      cell_source_count_max{};
  bool expected_cell_cardinality_exact{true};
  bool cell_support_semantics_exact{true};
  bool repeated_support_semantics_exact{true};
  bool domain_scale_coverage_exact{true};
};

struct Mechanics {
  int64_t source_batches{0};
  int64_t encoder_calls{0};
  int64_t anchors{0};
  int64_t public_rows{0};
  int64_t public_mask_cells{0};
  int64_t baseline_valid_cells{0};
  int64_t candidate_valid_cells{0};
  int64_t baseline_feature_rows{0};
  int64_t candidate_feature_rows{0};
  bool encoded_bytes_stable{true};
  bool public_contract_exact{true};
  bool public_masks_exact{true};
  bool invalid_zero_exact{true};
  bool outputs_finite{true};
  bool paired_adapter_contract_exact{true};
  bool input_data_unchanged{true};
  bool input_mask_unchanged{true};
  bool converted_data_bytes_stable{true};
  bool converted_feature_mask_bytes_stable{true};
  bool representation_parameters_unchanged{true};
  bool representation_buffers_unchanged{true};
  bool representation_eval_unchanged{true};
  bool cpu_rng_unchanged{true};
  bool cuda_rng_unchanged{true};
  std::string graph_order_fingerprint{};
  SupportAudit support{};
};

[[nodiscard]] std::string require_value(int argc, char **argv, int *index,
                                        const std::string &flag) {
  if (*index + 1 >= argc) {
    throw std::runtime_error("missing value for " + flag);
  }
  return argv[++(*index)];
}

[[nodiscard]] int64_t parse_i64(const std::string &text,
                                const std::string &flag) {
  std::size_t consumed = 0;
  const auto value = std::stoll(text, &consumed, 10);
  if (consumed != text.size()) {
    throw std::runtime_error("invalid integer for " + flag + ": " + text);
  }
  return value;
}

void classify_range(Options &options) {
  if (options.begin == kDevelopmentBegin && options.end == kDevelopmentEnd) {
    options.range_id = "development_refit_0_730";
    options.batch_ceiling = 12;
    options.expected_baseline_sha256 = kExpectedDevelopmentBaselineSha256;
    options.expected_baseline_size = kExpectedDevelopmentBaselineSize;
    return;
  }
  if (options.begin == kConfirmationBegin &&
      options.end == kConfirmationEnd) {
    options.range_id = "historical_confirmation_760_1088";
    options.batch_ceiling = 6;
    options.expected_baseline_sha256 = kExpectedConfirmationBaselineSha256;
    options.expected_baseline_size = kExpectedConfirmationBaselineSize;
    return;
  }
  throw std::runtime_error(
      "only frozen SRR-4 ranges [0,730) and [760,1088) are allowed; "
      "MDN/final holdout [1088,1170) access is forbidden");
}

[[nodiscard]] Options parse_options(int argc, char **argv) {
  Options out{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--config") {
      out.config_path = require_value(argc, argv, &index, argument);
    } else if (argument == "--input-representation-checkpoint") {
      out.representation_checkpoint_path =
          require_value(argc, argv, &index, argument);
    } else if (argument == "--output-dir") {
      out.output_dir = require_value(argc, argv, &index, argument);
    } else if (argument == "--anchor-index-begin") {
      out.begin = parse_i64(require_value(argc, argv, &index, argument),
                            argument);
    } else if (argument == "--anchor-index-end") {
      out.end = parse_i64(require_value(argc, argv, &index, argument), argument);
    } else {
      throw std::runtime_error("unknown or forbidden argument: " + argument);
    }
  }
  if (out.config_path.empty() || out.representation_checkpoint_path.empty() ||
      out.output_dir.empty() || out.begin < 0 || out.end <= out.begin) {
    throw std::runtime_error(
        "required exactly: --config ABS --input-representation-checkpoint "
        "ABS --output-dir ABS --anchor-index-begin N --anchor-index-end N");
  }
  classify_range(out);
  if (!out.config_path.is_absolute() ||
      !out.representation_checkpoint_path.is_absolute() ||
      !out.output_dir.is_absolute()) {
    throw std::runtime_error("all supplied paths must be absolute");
  }
  return out;
}

void require_new_output_dir(const fs::path &path) {
  if (fs::exists(path)) {
    if (!fs::is_directory(path) || !fs::is_empty(path)) {
      throw std::runtime_error("output directory is not new and empty: " +
                               path.string());
    }
    return;
  }
  fs::create_directories(path);
}

[[nodiscard]] std::string read_file_bytes(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to read authority bytes: " +
                             path.string());
  }
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] std::string sha256_file(const fs::path &path) {
  return cuwacunu::piaabo::digest::sha256_hex(read_file_bytes(path));
}

void require_authority(const fs::path &path, uintmax_t expected_size,
                       const std::string &expected_sha256, const char *kind) {
  if (!fs::is_regular_file(path) || fs::file_size(path) != expected_size) {
    throw std::runtime_error(std::string(kind) + " size mismatch");
  }
  const auto actual = sha256_file(path);
  if (actual != expected_sha256) {
    throw std::runtime_error(std::string(kind) + " SHA-256 mismatch: " +
                             actual);
  }
}

[[nodiscard]] bool tensor_contract_and_bytes_equal(
    const torch::Tensor &left, const torch::Tensor &right) {
  if (left.defined() != right.defined()) {
    return false;
  }
  if (!left.defined()) {
    return true;
  }
  if (left.scalar_type() != right.scalar_type() ||
      left.device() != right.device() || left.sizes() != right.sizes() ||
      left.strides() != right.strides() ||
      left.is_contiguous() != right.is_contiguous()) {
    return false;
  }
  const auto left_bytes = left.detach().to(torch::kCPU).contiguous();
  const auto right_bytes = right.detach().to(torch::kCPU).contiguous();
  const auto byte_count = static_cast<std::size_t>(left_bytes.numel()) *
                          static_cast<std::size_t>(left_bytes.element_size());
  return byte_count == 0 ||
         std::memcmp(left_bytes.data_ptr(), right_bytes.data_ptr(),
                     byte_count) == 0;
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_encode_output_t
clone_encoded(const mtf::mtf_jepa_mae_vicreg_encode_output_t &value) {
  const auto clone = [](const torch::Tensor &tensor) {
    return tensor.defined() ? tensor.clone() : torch::Tensor{};
  };
  mtf::mtf_jepa_mae_vicreg_encode_output_t out{};
  out.embeddings = clone(value.embeddings);
  out.pooled_embedding = clone(value.pooled_embedding);
  out.pooled_by_channel = clone(value.pooled_by_channel);
  out.pooled_time = clone(value.pooled_time);
  out.pooled_frequency = clone(value.pooled_frequency);
  out.token_mask = clone(value.token_mask);
  out.sample_valid_mask = clone(value.sample_valid_mask);
  out.channel_valid_mask = clone(value.channel_valid_mask);
  out.metadata.start_index = clone(value.metadata.start_index);
  out.metadata.width = clone(value.metadata.width);
  out.metadata.scale_id = clone(value.metadata.scale_id);
  out.metadata.channel_id = clone(value.metadata.channel_id);
  out.metadata.domain_id = clone(value.metadata.domain_id);
  return out;
}

[[nodiscard]] bool encoded_bytes_equal(
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &left,
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &right) {
  const std::array<std::pair<torch::Tensor, torch::Tensor>, 13> pairs = {{
      {left.embeddings, right.embeddings},
      {left.pooled_embedding, right.pooled_embedding},
      {left.pooled_by_channel, right.pooled_by_channel},
      {left.pooled_time, right.pooled_time},
      {left.pooled_frequency, right.pooled_frequency},
      {left.token_mask, right.token_mask},
      {left.sample_valid_mask, right.sample_valid_mask},
      {left.channel_valid_mask, right.channel_valid_mask},
      {left.metadata.start_index, right.metadata.start_index},
      {left.metadata.width, right.metadata.width},
      {left.metadata.scale_id, right.metadata.scale_id},
      {left.metadata.channel_id, right.metadata.channel_id},
      {left.metadata.domain_id, right.metadata.domain_id},
  }};
  return std::all_of(pairs.begin(), pairs.end(), [](const auto &pair) {
    return tensor_contract_and_bytes_equal(pair.first, pair.second);
  });
}

[[nodiscard]] GeneratorStateSnapshot
generator_snapshot(const torch::Device &device) {
  GeneratorStateSnapshot out{};
  out.cpu_state = at::detail::getDefaultCPUGenerator().get_state().clone();
  if (device.is_cuda()) {
    out.cuda_state =
        at::cuda::detail::getDefaultCUDAGenerator(device.index())
            .get_state()
            .clone();
  }
  return out;
}

[[nodiscard]] bool tensor_equal_optional(const torch::Tensor &left,
                                         const torch::Tensor &right) {
  return tensor_contract_and_bytes_equal(left, right);
}

template <typename ModuleHolderT>
[[nodiscard]] ModuleStateSnapshot snapshot_module(const ModuleHolderT &module) {
  ModuleStateSnapshot out{};
  for (const auto &parameter : module->parameters()) {
    out.parameters.push_back(
        parameter.detach().to(torch::kCPU).contiguous().clone());
  }
  for (const auto &buffer : module->buffers()) {
    out.buffers.push_back(buffer.detach().to(torch::kCPU).contiguous().clone());
  }
  return out;
}

template <typename ModuleHolderT>
[[nodiscard]] bool module_state_equal(const ModuleHolderT &module,
                                      const ModuleStateSnapshot &snapshot,
                                      bool parameters) {
  const auto current = parameters ? module->parameters() : module->buffers();
  const auto &expected = parameters ? snapshot.parameters : snapshot.buffers;
  if (current.size() != expected.size()) {
    return false;
  }
  for (std::size_t index = 0; index < current.size(); ++index) {
    const auto value = current[index].detach().to(torch::kCPU).contiguous();
    if (value.scalar_type() != expected[index].scalar_type() ||
        value.sizes() != expected[index].sizes() ||
        !tensor_contract_and_bytes_equal(value, expected[index])) {
      return false;
    }
  }
  return true;
}

template <typename ModuleHolderT>
[[nodiscard]] bool all_parameters_frozen(const ModuleHolderT &module) {
  for (const auto &parameter : module->parameters()) {
    if (parameter.requires_grad()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool public_contract_exact(
    const mtf::mtf_serving_pool_output_t &baseline,
    const mtf::mtf_serving_pool_output_t &candidate, int64_t rows) {
  return baseline.values.defined() && candidate.values.defined() &&
         baseline.valid_mask.defined() && candidate.valid_mask.defined() &&
         baseline.values.sizes() ==
             torch::IntArrayRef({rows, kChannelCount, kLatentDim}) &&
         candidate.values.sizes() == baseline.values.sizes() &&
         baseline.valid_mask.sizes() ==
             torch::IntArrayRef({rows, kChannelCount}) &&
         candidate.valid_mask.sizes() == baseline.valid_mask.sizes() &&
         baseline.values.scalar_type() == candidate.values.scalar_type() &&
         baseline.values.device() == candidate.values.device() &&
         baseline.valid_mask.scalar_type() == torch::kBool &&
         candidate.valid_mask.scalar_type() == torch::kBool &&
         baseline.valid_mask.device() == candidate.valid_mask.device() &&
         baseline.values.is_contiguous() && candidate.values.is_contiguous() &&
         baseline.valid_mask.is_contiguous() &&
         candidate.valid_mask.is_contiguous();
}

[[nodiscard]] bool invalid_values_zero(
    const mtf::mtf_serving_pool_output_t &pool) {
  const auto invalid =
      pool.valid_mask.logical_not().unsqueeze(-1).expand_as(pool.values);
  const auto selected = pool.values.masked_select(invalid);
  return selected.numel() == 0 || selected.eq(0).all().template item<bool>();
}

template <typename KeyT>
[[nodiscard]] repstream::channel_representation_batch_t<KeyT>
make_representation_batch(const mtf::mtf_channel_node_input_t &input,
                          const mtf::mtf_serving_pool_output_t &pool,
                          const cuwacunu::wikimyei::expression::nodelift::srl::
                              stream::node_lifted_batch_t<KeyT> &lifted) {
  vicreg::graph_row_index_t row_index{
      .anchor_index = input.anchor_index,
      .node_index = input.node_index,
      .B_anchor = input.B_anchor,
      .N = input.N,
  };
  auto adapted = vicreg::make_channel_representation_batch(
      pool.values, pool.valid_mask, row_index, input.anchor_keys,
      input.node_ids);

  repstream::channel_representation_batch_t<KeyT> out{};
  out.node_encoding = std::move(adapted.node_encoding);
  out.node_encoding_mask = std::move(adapted.node_encoding_mask);
  out.anchor_keys = lifted.anchor_keys;
  out.node_ids = lifted.node_ids;
  out.edge_ids = lifted.edge_ids;
  out.graph_order_fingerprint = lifted.graph_order_fingerprint;
  out.cursor = lifted.cursor;
  out.nodelift_stream_report = lifted.stream_report;
  out.nodelift_runtime_lls = lifted.runtime_lls;
  out.future_node_features = lifted.future_node_features;
  out.future_node_mask = lifted.future_node_mask;
  return out;
}

template <typename KeyT>
[[nodiscard]] bool paired_adapter_contract_exact(
    const mdnstream::channel_mdn_input_batch_t<KeyT> &baseline,
    const mdnstream::channel_mdn_input_batch_t<KeyT> &candidate) {
  return baseline.context.sizes() == candidate.context.sizes() &&
         baseline.context.scalar_type() == candidate.context.scalar_type() &&
         baseline.context.device() == candidate.context.device() &&
         tensor_contract_and_bytes_equal(baseline.context_mask,
                                         candidate.context_mask) &&
         tensor_contract_and_bytes_equal(baseline.future, candidate.future) &&
         tensor_contract_and_bytes_equal(baseline.future_mask,
                                         candidate.future_mask) &&
         tensor_contract_and_bytes_equal(baseline.anchor_keys,
                                         candidate.anchor_keys) &&
         tensor_contract_and_bytes_equal(baseline.anchor_index,
                                         candidate.anchor_index) &&
         tensor_contract_and_bytes_equal(baseline.node_index,
                                         candidate.node_index) &&
         baseline.node_ids == candidate.node_ids &&
         baseline.edge_ids == candidate.edge_ids &&
         baseline.target_coords == candidate.target_coords &&
         baseline.graph_order_fingerprint ==
             candidate.graph_order_fingerprint &&
         baseline.cursor.begin_anchor_index ==
             candidate.cursor.begin_anchor_index &&
         baseline.cursor.end_anchor_index == candidate.cursor.end_anchor_index &&
         baseline.cursor.anchor_keys == candidate.cursor.anchor_keys &&
         baseline.cursor.anchor_indices == candidate.cursor.anchor_indices;
}

template <typename BundleT>
void configure_frozen_range(BundleT &bundle, int64_t begin, int64_t end) {
  bundle.wave_settings.source_range_policy =
      cuwacunu::hero::runtime::settings::wave_source_range_policy_t::anchor_index;
  bundle.wave_settings.source_order_policy =
      cuwacunu::hero::runtime::settings::wave_source_order_policy_t::sequential;
  bundle.wave_settings.source_order_policy_explicit = true;
  bundle.wave_settings.anchor_index_begin = static_cast<std::size_t>(begin);
  bundle.wave_settings.anchor_index_end = static_cast<std::size_t>(end);
  bundle.wave_settings.source_key_begin = std::nullopt;
  bundle.wave_settings.source_key_end = std::nullopt;
  cuwacunu::hero::runtime::settings::validate_wave_settings(
      bundle.wave_settings);
  protocol::validate_channel_graph_first_config_bundle(bundle);
}

void initialize_support_audit(SupportAudit &audit) {
  audit.source_token_count_min.fill(std::numeric_limits<int64_t>::max());
  audit.source_token_count_max.fill(std::numeric_limits<int64_t>::min());
  audit.supported_cell_count_min.fill(std::numeric_limits<int64_t>::max());
  audit.supported_cell_count_max.fill(std::numeric_limits<int64_t>::min());
  audit.repeated_support_count_min.fill(std::numeric_limits<int64_t>::max());
  audit.repeated_support_count_max.fill(std::numeric_limits<int64_t>::min());
  for (auto &channel : audit.cell_source_count_min) {
    channel.fill(std::numeric_limits<int64_t>::max());
  }
  for (auto &channel : audit.cell_source_count_max) {
    channel.fill(std::numeric_limits<int64_t>::min());
  }
}

void audit_sparse_support(
    SupportAudit &audit,
    const mtf::detail::structured_cdsb_sparse_v1_lifted_t &detail,
    const mtf::mtf_serving_pool_output_t &candidate, int64_t rows) {
  const bool shapes_exact =
      detail.grouped_token_mask.sizes() == torch::IntArrayRef({rows, 3, 24}) &&
      detail.cell_support_mask.sizes() == torch::IntArrayRef({rows, 3, 16}) &&
      detail.cell_source_token_count.sizes() ==
          torch::IntArrayRef({rows, 3, 16}) &&
      detail.cell_expected_token_count.sizes() == torch::IntArrayRef({16}) &&
      detail.domain_scale_support_mask.sizes() ==
          torch::IntArrayRef({rows, 3, 8}) &&
      detail.repeated_support_position_count.sizes() ==
          torch::IntArrayRef({rows, 3}) &&
      detail.valid_mask.sizes() == torch::IntArrayRef({rows, 3}) &&
      detail.complete_mask.sizes() == torch::IntArrayRef({rows, 3});
  if (!shapes_exact ||
      !tensor_contract_and_bytes_equal(detail.valid_mask,
                                       candidate.valid_mask)) {
    throw std::runtime_error("sparse lift diagnostic/public mask mismatch");
  }

  const auto grouped_mask =
      detail.grouped_token_mask.to(torch::kCPU).to(torch::kBool).contiguous();
  const auto cell_support =
      detail.cell_support_mask.to(torch::kCPU).to(torch::kBool).contiguous();
  const auto cell_counts = detail.cell_source_token_count
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();
  const auto expected_counts = detail.cell_expected_token_count
                                   .to(torch::kCPU)
                                   .to(torch::kInt64)
                                   .contiguous();
  const auto group_support = detail.domain_scale_support_mask
                                 .to(torch::kCPU)
                                 .to(torch::kBool)
                                 .contiguous();
  const auto repeated_counts = detail.repeated_support_position_count
                                   .to(torch::kCPU)
                                   .to(torch::kInt64)
                                   .contiguous();
  const auto complete =
      detail.complete_mask.to(torch::kCPU).to(torch::kBool).contiguous();

  for (int64_t cell = 0; cell < kCompactCellCount; ++cell) {
    audit.expected_cell_cardinality_exact =
        audit.expected_cell_cardinality_exact &&
        expected_counts[cell].template item<int64_t>() ==
            kExpectedCellTokenCount[static_cast<std::size_t>(cell)];
  }
  audit.cell_support_semantics_exact =
      audit.cell_support_semantics_exact &&
      tensor_contract_and_bytes_equal(cell_support, cell_counts.gt(0));
  audit.domain_scale_coverage_exact =
      audit.domain_scale_coverage_exact &&
      group_support.all().template item<bool>();

  for (int64_t row = 0; row < rows; ++row) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const auto channel_index = static_cast<std::size_t>(channel);
      const auto source_count = grouped_mask.index({row, channel}).sum()
                                    .template item<int64_t>();
      const auto cell_source_sum = cell_counts.index({row, channel}).sum()
                                       .template item<int64_t>();
      const auto supported_cells = cell_support.index({row, channel}).sum()
                                       .template item<int64_t>();
      const auto repeated_count =
          repeated_counts.index({row, channel}).template item<int64_t>();
      int64_t independently_repeated = 0;
      for (int64_t cell = 0; cell < kCompactCellCount; ++cell) {
        const auto cell_index = static_cast<std::size_t>(cell);
        const auto observed =
            cell_counts.index({row, channel, cell}).template item<int64_t>();
        const auto expected = kExpectedCellTokenCount[cell_index];
        if (observed < 0 || observed > expected) {
          throw std::runtime_error("cell source count escaped cardinality");
        }
        audit.cell_source_count_min[channel_index][cell_index] = std::min(
            audit.cell_source_count_min[channel_index][cell_index], observed);
        audit.cell_source_count_max[channel_index][cell_index] = std::max(
            audit.cell_source_count_max[channel_index][cell_index], observed);
        if (observed > 0) {
          independently_repeated += expected;
        }
      }
      audit.repeated_support_semantics_exact =
          audit.repeated_support_semantics_exact &&
          independently_repeated == repeated_count;
      if (source_count != kExpectedSourceTokenCount[channel_index] ||
          cell_source_sum != source_count ||
          supported_cells != kExpectedSupportedCellCount[channel_index] ||
          repeated_count !=
              kExpectedRepeatedSupportPositionCount[channel_index]) {
        throw std::runtime_error("actual sparse-surface support count changed");
      }
      ++audit.row_count[channel_index];
      audit.complete_row_count[channel_index] +=
          complete.index({row, channel}).template item<bool>() ? 1 : 0;
      audit.source_token_count_min[channel_index] =
          std::min(audit.source_token_count_min[channel_index], source_count);
      audit.source_token_count_max[channel_index] =
          std::max(audit.source_token_count_max[channel_index], source_count);
      audit.supported_cell_count_min[channel_index] = std::min(
          audit.supported_cell_count_min[channel_index], supported_cells);
      audit.supported_cell_count_max[channel_index] = std::max(
          audit.supported_cell_count_max[channel_index], supported_cells);
      audit.repeated_support_count_min[channel_index] = std::min(
          audit.repeated_support_count_min[channel_index], repeated_count);
      audit.repeated_support_count_max[channel_index] = std::max(
          audit.repeated_support_count_max[channel_index], repeated_count);
    }
  }
  if (!audit.expected_cell_cardinality_exact ||
      !audit.cell_support_semantics_exact ||
      !audit.repeated_support_semantics_exact ||
      !audit.domain_scale_coverage_exact) {
    throw std::runtime_error("sparse support diagnostic semantics failed");
  }
}

void write_receipt(const fs::path &path, const Options &options,
                   const Mechanics &mechanics,
                   const std::string &config_sha256,
                   const std::string &checkpoint_sha256,
                   const fs::path &baseline_path,
                   const std::string &baseline_sha256,
                   const fs::path &candidate_path,
                   const std::string &candidate_sha256) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to create mechanics receipt");
  }
  out << std::boolalpha;
  out << "schema_id=cuwacunu.srr4.sparse_readout_capture.mechanics.v1\n";
  out << "status=mechanics_pass\n";
  out << "sealed_protocol_sha256=" << kSealedProtocolSha256 << "\n";
  out << "range_id=" << options.range_id << "\n";
  out << "anchor_range=[" << options.begin << ',' << options.end << ")\n";
  out << "anchor_count=" << mechanics.anchors << "\n";
  out << "maximum_anchor_read=" << options.end - 1 << "\n";
  out << "final_holdout_range=[" << kFinalHoldoutBegin << ','
      << kFinalHoldoutEnd << ")\n";
  out << "final_holdout_access=false\n";
  out << "seed=" << kSeed << "\n";
  out << "source_order=contiguous_sequential_anchor_index\n";
  out << "graph_order_fingerprint=" << mechanics.graph_order_fingerprint
      << "\n";
  out << "active_production_policy=all_tokens\n";
  out << "baseline_policy=all_tokens\n";
  out << "candidate_policy=structured_cdsb_sparse_v1\n";
  out << "source_batches=" << mechanics.source_batches << "\n";
  out << "source_batch_ceiling=" << options.batch_ceiling << "\n";
  out << "encoder_calls=" << mechanics.encoder_calls << "\n";
  out << "encoder_calls_equal_source_batches="
      << (mechanics.encoder_calls == mechanics.source_batches) << "\n";
  out << "same_retained_encoded_object=true\n";
  out << "encoded_bytes_stable=" << mechanics.encoded_bytes_stable << "\n";
  out << "public_rows=" << mechanics.public_rows << "\n";
  out << "public_mask_cells=" << mechanics.public_mask_cells << "\n";
  out << "public_contract_exact=" << mechanics.public_contract_exact << "\n";
  out << "public_masks_exact=" << mechanics.public_masks_exact << "\n";
  out << "baseline_valid_cells=" << mechanics.baseline_valid_cells << "\n";
  out << "candidate_valid_cells=" << mechanics.candidate_valid_cells << "\n";
  out << "invalid_zero_exact=" << mechanics.invalid_zero_exact << "\n";
  out << "outputs_finite=" << mechanics.outputs_finite << "\n";
  out << "paired_adapter_contract_exact="
      << mechanics.paired_adapter_contract_exact << "\n";
  out << "paired_rows_keys_targets_order_exact="
      << mechanics.paired_adapter_contract_exact << "\n";
  out << "input_data_unchanged=" << mechanics.input_data_unchanged << "\n";
  out << "input_mask_unchanged=" << mechanics.input_mask_unchanged << "\n";
  out << "converted_data_bytes_stable="
      << mechanics.converted_data_bytes_stable << "\n";
  out << "converted_feature_mask_bytes_stable="
      << mechanics.converted_feature_mask_bytes_stable << "\n";
  out << "representation_parameters_unchanged="
      << mechanics.representation_parameters_unchanged << "\n";
  out << "representation_buffers_unchanged="
      << mechanics.representation_buffers_unchanged << "\n";
  out << "representation_eval_unchanged="
      << mechanics.representation_eval_unchanged << "\n";
  out << "cpu_rng_unchanged=" << mechanics.cpu_rng_unchanged << "\n";
  out << "cuda_rng_unchanged=" << mechanics.cuda_rng_unchanged << "\n";
  out << "support.expected_cell_cardinality_exact="
      << mechanics.support.expected_cell_cardinality_exact << "\n";
  out << "support.cell_support_semantics_exact="
      << mechanics.support.cell_support_semantics_exact << "\n";
  out << "support.repeated_support_semantics_exact="
      << mechanics.support.repeated_support_semantics_exact << "\n";
  out << "support.domain_scale_coverage_exact="
      << mechanics.support.domain_scale_coverage_exact << "\n";
  for (std::size_t channel = 0; channel < kChannelCount; ++channel) {
    const auto prefix = "channel_" + std::to_string(channel) + ".";
    out << prefix << "rows=" << mechanics.support.row_count[channel] << "\n";
    out << prefix << "complete_rows="
        << mechanics.support.complete_row_count[channel] << "\n";
    out << prefix << "expected_source_token_count="
        << kExpectedSourceTokenCount[channel] << "\n";
    out << prefix << "source_token_count_min="
        << mechanics.support.source_token_count_min[channel] << "\n";
    out << prefix << "source_token_count_max="
        << mechanics.support.source_token_count_max[channel] << "\n";
    out << prefix << "expected_supported_cell_count="
        << kExpectedSupportedCellCount[channel] << "\n";
    out << prefix << "supported_cell_count_min="
        << mechanics.support.supported_cell_count_min[channel] << "\n";
    out << prefix << "supported_cell_count_max="
        << mechanics.support.supported_cell_count_max[channel] << "\n";
    out << prefix << "expected_repeated_support_position_count="
        << kExpectedRepeatedSupportPositionCount[channel] << "\n";
    out << prefix << "repeated_support_position_count_min="
        << mechanics.support.repeated_support_count_min[channel] << "\n";
    out << prefix << "repeated_support_position_count_max="
        << mechanics.support.repeated_support_count_max[channel] << "\n";
    for (std::size_t cell = 0; cell < kCompactCellCount; ++cell) {
      out << prefix << "cell_" << cell << ".expected_token_count="
          << kExpectedCellTokenCount[cell] << "\n";
      out << prefix << "cell_" << cell << ".source_token_count_min="
          << mechanics.support.cell_source_count_min[channel][cell] << "\n";
      out << prefix << "cell_" << cell << ".source_token_count_max="
          << mechanics.support.cell_source_count_max[channel][cell] << "\n";
    }
  }
  out << "feature_count=" << kFeatureCount << "\n";
  out << "baseline_feature_rows=" << mechanics.baseline_feature_rows << "\n";
  out << "candidate_feature_rows=" << mechanics.candidate_feature_rows
      << "\n";
  out << "config_path=" << options.config_path.string() << "\n";
  out << "config_size=" << fs::file_size(options.config_path) << "\n";
  out << "config_sha256=" << config_sha256 << "\n";
  out << "representation_checkpoint_path="
      << options.representation_checkpoint_path.string() << "\n";
  out << "representation_checkpoint_size="
      << fs::file_size(options.representation_checkpoint_path) << "\n";
  out << "representation_checkpoint_sha256=" << checkpoint_sha256 << "\n";
  out << "baseline_feature_path=" << baseline_path.string() << "\n";
  out << "baseline_feature_size=" << fs::file_size(baseline_path) << "\n";
  out << "baseline_feature_sha256=" << baseline_sha256 << "\n";
  out << "baseline_frozen_hash_match="
      << (baseline_sha256 == options.expected_baseline_sha256) << "\n";
  out << "candidate_feature_path=" << candidate_path.string() << "\n";
  out << "candidate_feature_size=" << fs::file_size(candidate_path) << "\n";
  out << "candidate_feature_sha256=" << candidate_sha256 << "\n";
  out << "mdn_checkpoint_access=false\n";
  out << "mdn_constructions=0\n";
  out << "mdn_forwards=0\n";
  out << "prediction_artifacts_written=false\n";
  out << "endpoint_metrics_computed=false\n";
  out << "augmentations_enabled=false\n";
  out << "optimizer_steps=0\n";
  out << "backward_calls=0\n";
  out << "checkpoint_writes=0\n";
  out << "policy_activation_changed=false\n";
  out.flush();
  if (!out) {
    throw std::runtime_error("failed while writing mechanics receipt");
  }
}

void run(const Options &options) {
  if (options.begin >= kFinalHoldoutBegin || options.end > kFinalHoldoutBegin) {
    throw std::runtime_error("final holdout access is forbidden");
  }
  require_authority(options.config_path, kExpectedConfigSize,
                    kExpectedConfigSha256, "graph-first config");
  require_authority(options.representation_checkpoint_path,
                    kExpectedRepresentationCheckpointSize,
                    kExpectedRepresentationCheckpointSha256,
                    "representation checkpoint");
  const auto config_sha_before = sha256_file(options.config_path);
  const auto checkpoint_sha_before =
      sha256_file(options.representation_checkpoint_path);

  require_new_output_dir(options.output_dir);
  const fs::path baseline_path =
      options.output_dir / "all_tokens.representation_edge_features.probe";
  const fs::path candidate_path = options.output_dir /
                                  "structured_cdsb_sparse_v1."
                                  "representation_edge_features.probe";
  const fs::path receipt_path = options.output_dir / "mechanics.receipt";

  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path.string());
  if (!protocol::active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    throw std::runtime_error("active protocol does not use MTF representation");
  }
  if (bundle.mtf_jepa_mae_vicreg.config.serving_pool_policy !=
      mtf::mtf_serving_pool_policy_t::all_tokens) {
    throw std::runtime_error("active production DSL must remain all_tokens");
  }
  if (bundle.channel_mdn_training.seed != kSeed) {
    throw std::runtime_error("configured seed differs from sentinel 31");
  }
  configure_frozen_range(bundle, options.begin, options.end);

  protocol::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.compute_alignment_diagnostics = true;
  builder_options.runtime_report_mode =
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::normal;
  protocol::channel_graph_first_pipeline_builder_t<types::kline_t> builder(
      std::move(bundle), builder_options);
  const auto &config = builder.bundle().mtf_jepa_mae_vicreg.config;
  inference_detail::seed_torch_runtime(kSeed, builder.options().device);

  auto source = builder.make_graph_source();
  auto lifted_stream = builder.make_node_lifted_stream(
      std::move(source),
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::normal);

  auto representation_model = mtf::MtfJepaMaeVicreg(config);
  representation_model->to(config.device, config.dtype);
  inference_detail::load_mtf_jepa_mae_vicreg_checkpoint_file(
      options.representation_checkpoint_path, representation_model,
      builder.options().device, config.channel_count, config.history_length,
      config.input_width, config.d_model, config.latent_dim,
      config.projector_dim);
  inference_detail::freeze_vicreg_encoder(representation_model);
  if (representation_model->is_training() ||
      !all_parameters_frozen(representation_model)) {
    throw std::runtime_error("representation model is not frozen/eval");
  }

  Mechanics mechanics{};
  initialize_support_audit(mechanics.support);
  const auto module_before = snapshot_module(representation_model);
  const auto rng_before = generator_snapshot(builder.options().device);
  const bool mode_before = representation_model->is_training();

  std::size_t next_expected_anchor = static_cast<std::size_t>(options.begin);
  while (lifted_stream.has_next()) {
    auto lifted = lifted_stream.next();
    ++mechanics.source_batches;
    if (mechanics.source_batches > options.batch_ceiling) {
      throw std::runtime_error("source batch ceiling exceeded");
    }
    if (lifted.cursor.begin_anchor_index != next_expected_anchor ||
        lifted.cursor.anchor_indices.size() != lifted.cursor.anchor_count()) {
      throw std::runtime_error("source cursor is not contiguous and exact");
    }
    for (std::size_t index = 0; index < lifted.cursor.anchor_indices.size();
         ++index) {
      if (lifted.cursor.anchor_indices[index] != next_expected_anchor + index) {
        throw std::runtime_error("source anchor order changed");
      }
    }
    next_expected_anchor += lifted.cursor.anchor_count();
    if (lifted.cursor.end_anchor_index != next_expected_anchor ||
        next_expected_anchor > static_cast<std::size_t>(options.end)) {
      throw std::runtime_error("source cursor escaped frozen range");
    }
    if (mechanics.graph_order_fingerprint.empty()) {
      mechanics.graph_order_fingerprint = lifted.graph_order_fingerprint;
    } else if (mechanics.graph_order_fingerprint !=
               lifted.graph_order_fingerprint) {
      throw std::runtime_error("graph order fingerprint changed");
    }
    mechanics.anchors += static_cast<int64_t>(lifted.cursor.anchor_count());

    auto input = mtf::make_mtf_channel_node_input(lifted);
    const auto input_data_before = input.data.clone();
    const auto input_mask_before = input.feature_mask.clone();
    const auto tensor_options =
        torch::TensorOptions().dtype(config.dtype).device(config.device);
    const auto data = input.data.to(tensor_options);
    const auto feature_mask = input.feature_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config.device));
    const auto converted_data_before = data.clone();
    const auto converted_feature_mask_before = feature_mask.clone();

    torch::NoGradGuard no_grad;
    auto encoded = representation_model->encode(data, feature_mask);
    ++mechanics.encoder_calls;
    const bool converted_data_stable_after_encode =
        tensor_contract_and_bytes_equal(data, converted_data_before);
    const bool converted_mask_stable_after_encode =
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);
    const auto encoded_before = clone_encoded(encoded);
    const auto baseline = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, config);
    const bool encoded_stable_between =
        encoded_bytes_equal(encoded, encoded_before);
    const bool converted_data_stable_between =
        tensor_contract_and_bytes_equal(data, converted_data_before);
    const bool converted_mask_stable_between =
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);
    const auto candidate = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        config);
    const bool encoded_stable_after =
        encoded_bytes_equal(encoded, encoded_before);
    const bool converted_data_stable_after =
        tensor_contract_and_bytes_equal(data, converted_data_before);
    const bool converted_mask_stable_after =
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);
    const auto sparse_detail =
        mtf::detail::structured_cdsb_sparse_v1_lift(encoded, config);
    const bool encoded_stable_after_detail =
        encoded_bytes_equal(encoded, encoded_before);
    const bool converted_data_stable_after_detail =
        tensor_contract_and_bytes_equal(data, converted_data_before);
    const bool converted_mask_stable_after_detail =
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);

    const int64_t rows = input.data.size(0);
    const bool batch_contract =
        public_contract_exact(baseline, candidate, rows);
    const bool batch_masks =
        batch_contract && tensor_contract_and_bytes_equal(
                              baseline.valid_mask, candidate.valid_mask);
    const bool batch_invalid_zero =
        batch_contract && invalid_values_zero(baseline) &&
        invalid_values_zero(candidate);
    const bool batch_finite =
        batch_contract &&
        torch::isfinite(baseline.values).all().template item<bool>() &&
        torch::isfinite(candidate.values).all().template item<bool>();
    const bool batch_input_data_unchanged =
        tensor_contract_and_bytes_equal(input.data, input_data_before);
    const bool batch_input_mask_unchanged =
        tensor_contract_and_bytes_equal(input.feature_mask, input_mask_before);

    mechanics.encoded_bytes_stable =
        mechanics.encoded_bytes_stable && encoded_stable_between &&
        encoded_stable_after && encoded_stable_after_detail;
    mechanics.public_contract_exact =
        mechanics.public_contract_exact && batch_contract;
    mechanics.public_masks_exact = mechanics.public_masks_exact && batch_masks;
    mechanics.invalid_zero_exact =
        mechanics.invalid_zero_exact && batch_invalid_zero;
    mechanics.outputs_finite = mechanics.outputs_finite && batch_finite;
    mechanics.input_data_unchanged =
        mechanics.input_data_unchanged && batch_input_data_unchanged;
    mechanics.input_mask_unchanged =
        mechanics.input_mask_unchanged && batch_input_mask_unchanged;
    mechanics.converted_data_bytes_stable =
        mechanics.converted_data_bytes_stable &&
        converted_data_stable_after_encode && converted_data_stable_between &&
        converted_data_stable_after && converted_data_stable_after_detail;
    mechanics.converted_feature_mask_bytes_stable =
        mechanics.converted_feature_mask_bytes_stable &&
        converted_mask_stable_after_encode && converted_mask_stable_between &&
        converted_mask_stable_after && converted_mask_stable_after_detail;
    if (!mechanics.encoded_bytes_stable || !mechanics.public_contract_exact ||
        !mechanics.public_masks_exact || !mechanics.invalid_zero_exact ||
        !mechanics.outputs_finite || !mechanics.input_data_unchanged ||
        !mechanics.input_mask_unchanged ||
        !mechanics.converted_data_bytes_stable ||
        !mechanics.converted_feature_mask_bytes_stable) {
      throw std::runtime_error("paired sparse readout mechanics failed");
    }
    mechanics.public_rows += rows;
    mechanics.public_mask_cells += baseline.valid_mask.numel();
    mechanics.baseline_valid_cells +=
        baseline.valid_mask.sum().template item<int64_t>();
    mechanics.candidate_valid_cells +=
        candidate.valid_mask.sum().template item<int64_t>();
    audit_sparse_support(mechanics.support, sparse_detail, candidate, rows);

    auto baseline_representation =
        make_representation_batch(input, baseline, lifted);
    auto candidate_representation =
        make_representation_batch(input, candidate, lifted);
    auto baseline_batch = mdnstream::make_channel_mdn_input_batch(
        baseline_representation, builder.channel_mdn_adapter_options());
    auto candidate_batch = mdnstream::make_channel_mdn_input_batch(
        candidate_representation, builder.channel_mdn_adapter_options());
    mechanics.paired_adapter_contract_exact =
        mechanics.paired_adapter_contract_exact &&
        paired_adapter_contract_exact(baseline_batch, candidate_batch);
    if (!mechanics.paired_adapter_contract_exact) {
      throw std::runtime_error("paired adapter/order/target contract changed");
    }

    mechanics.baseline_feature_rows +=
        inference_detail::append_representation_edge_feature_probe_rows(
            baseline_path, baseline_representation, baseline_batch,
            kCloseFeatureIndex);
    mechanics.candidate_feature_rows +=
        inference_detail::append_representation_edge_feature_probe_rows(
            candidate_path, candidate_representation, candidate_batch,
            kCloseFeatureIndex);
  }

  const auto rng_after = generator_snapshot(builder.options().device);
  mechanics.cpu_rng_unchanged =
      tensor_equal_optional(rng_before.cpu_state, rng_after.cpu_state);
  mechanics.cuda_rng_unchanged =
      tensor_equal_optional(rng_before.cuda_state, rng_after.cuda_state);
  mechanics.representation_parameters_unchanged =
      module_state_equal(representation_model, module_before, true);
  mechanics.representation_buffers_unchanged =
      module_state_equal(representation_model, module_before, false);
  mechanics.representation_eval_unchanged =
      representation_model->is_training() == mode_before &&
      !representation_model->is_training();

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_public_rows = expected_anchors * kNodeCount;
  const int64_t expected_feature_rows = expected_anchors * 9;
  const int64_t expected_mask_cells = expected_public_rows * kChannelCount;
  bool support_rows_exact = true;
  for (std::size_t channel = 0; channel < kChannelCount; ++channel) {
    support_rows_exact = support_rows_exact &&
                         mechanics.support.row_count[channel] ==
                             expected_public_rows;
  }
  const bool hard_gate =
      mechanics.anchors == expected_anchors &&
      next_expected_anchor == static_cast<std::size_t>(options.end) &&
      mechanics.source_batches <= options.batch_ceiling &&
      mechanics.encoder_calls == mechanics.source_batches &&
      mechanics.public_rows == expected_public_rows &&
      mechanics.public_mask_cells == expected_mask_cells &&
      mechanics.baseline_valid_cells == expected_mask_cells &&
      mechanics.candidate_valid_cells == expected_mask_cells &&
      mechanics.baseline_feature_rows == expected_feature_rows &&
      mechanics.candidate_feature_rows == expected_feature_rows &&
      mechanics.encoded_bytes_stable && mechanics.public_contract_exact &&
      mechanics.public_masks_exact && mechanics.invalid_zero_exact &&
      mechanics.outputs_finite && mechanics.paired_adapter_contract_exact &&
      mechanics.input_data_unchanged && mechanics.input_mask_unchanged &&
      mechanics.converted_data_bytes_stable &&
      mechanics.converted_feature_mask_bytes_stable &&
      mechanics.representation_parameters_unchanged &&
      mechanics.representation_buffers_unchanged &&
      mechanics.representation_eval_unchanged && mechanics.cpu_rng_unchanged &&
      mechanics.cuda_rng_unchanged && support_rows_exact &&
      mechanics.support.expected_cell_cardinality_exact &&
      mechanics.support.cell_support_semantics_exact &&
      mechanics.support.repeated_support_semantics_exact &&
      mechanics.support.domain_scale_coverage_exact;
  if (!hard_gate) {
    throw std::runtime_error("SRR-4 hard mechanics gate failed");
  }

  const auto config_sha_after = sha256_file(options.config_path);
  const auto checkpoint_sha_after =
      sha256_file(options.representation_checkpoint_path);
  if (config_sha_before != config_sha_after ||
      checkpoint_sha_before != checkpoint_sha_after ||
      fs::file_size(options.config_path) != kExpectedConfigSize ||
      fs::file_size(options.representation_checkpoint_path) !=
          kExpectedRepresentationCheckpointSize) {
    throw std::runtime_error("frozen authority bytes changed during capture");
  }

  require_authority(baseline_path, options.expected_baseline_size,
                    options.expected_baseline_sha256,
                    "fresh all_tokens feature probe");
  const auto baseline_sha256 = sha256_file(baseline_path);
  const auto candidate_sha256 = sha256_file(candidate_path);
  if (baseline_sha256 != sha256_file(baseline_path) ||
      candidate_sha256 != sha256_file(candidate_path)) {
    throw std::runtime_error("persisted feature probe replay hash changed");
  }
  write_receipt(receipt_path, options, mechanics, config_sha_after,
                checkpoint_sha_after, baseline_path, baseline_sha256,
                candidate_path, candidate_sha256);
}

} // namespace

int main(int argc, char **argv) {
  try {
    run(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "cuwacunu_srr4_sparse_readout_capture: " << error.what()
              << "\n";
    return 1;
  }
}
