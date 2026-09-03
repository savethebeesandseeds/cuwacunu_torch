// SPDX-License-Identifier: MIT

#include <ATen/CPUGeneratorImpl.h>
#include <ATen/cuda/CUDAGeneratorImpl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
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
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace inference = cuwacunu::jkimyei::training::inference;
namespace inference_detail = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

namespace {

constexpr int64_t kSeed = 31;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kLatentDim = 32;
constexpr int64_t kFeatureCount = 3 * kLatentDim;
constexpr int64_t kCloseFeatureIndex = 3;
constexpr int64_t kStageABegin = 760;
constexpr int64_t kStageAEnd = 1088;
constexpr int64_t kFinalHoldoutBegin = 1088;
constexpr int64_t kFinalHoldoutEnd = 1170;

constexpr const char *kExpectedConfigSha256 =
    "23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0";
constexpr const char *kExpectedRepresentationCheckpointSha256 =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr const char *kExpectedMdnCheckpointSha256 =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr const char *kSealedProtocolSha256 =
    "6deee9c2420e205828322cee34b8d5d43a83c98918670ede682d8e36e17de6da";
constexpr uintmax_t kSealedProtocolSize = 13639;
constexpr uintmax_t kExpectedConfigSize = 4298;
constexpr uintmax_t kExpectedRepresentationCheckpointSize = 853867;
constexpr uintmax_t kExpectedMdnCheckpointSize = 3227665;
constexpr const char *kExpectedBaselineFeatureSha256 =
    "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7";
constexpr uintmax_t kExpectedBaselineFeatureSize = 6133066;
constexpr const char *kExpectedCandidateFeatureSha256 =
    "dfac215b73b08525dcba90d8891c8dede328ed99ec0117e2e2efaea6a5afbd73";
constexpr uintmax_t kExpectedCandidateFeatureSize = 6112783;

struct Options {
  fs::path config_path{};
  fs::path representation_checkpoint_path{};
  fs::path mdn_checkpoint_path{};
  fs::path output_dir{};
  int64_t begin{-1};
  int64_t end{-1};
};

struct GeneratorStateSnapshot {
  torch::Tensor cpu_state{};
  torch::Tensor cuda_state{};
};

struct ModuleStateSnapshot {
  std::vector<torch::Tensor> parameters{};
  std::vector<torch::Tensor> buffers{};
};

struct Mechanics {
  int64_t source_batches{0};
  int64_t encoder_calls{0};
  int64_t mdn_forwards{0};
  int64_t mdn_model_constructions{0};
  int64_t mdn_identity_authentication_attempts{0};
  int64_t mdn_successful_weight_loads{0};
  int64_t anchors{0};
  int64_t maximum_source_batch_anchors{0};
  int64_t public_rows{0};
  int64_t public_mask_cells{0};
  int64_t baseline_feature_rows{0};
  int64_t candidate_feature_rows{0};
  int64_t baseline_prediction_rows{0};
  int64_t candidate_prediction_rows{0};
  int64_t baseline_valid_prediction_rows{0};
  int64_t candidate_valid_prediction_rows{0};
  int64_t baseline_valid_cells{0};
  int64_t candidate_valid_cells{0};
  bool encoded_bytes_stable{true};
  bool same_encoded_object{true};
  bool public_contract_exact{true};
  bool public_masks_exact{true};
  bool invalid_zero_exact{true};
  bool selector_outputs_finite{true};
  bool mdn_outputs_finite{true};
  bool paired_adapter_contract_exact{true};
  bool prediction_valid_bits_exact{true};
  bool input_data_unchanged{true};
  bool input_mask_unchanged{true};
  bool converted_data_bytes_stable{true};
  bool converted_feature_mask_bytes_stable{true};
  bool representation_parameters_unchanged{true};
  bool representation_buffers_unchanged{true};
  bool mdn_parameters_unchanged{true};
  bool mdn_buffers_unchanged{true};
  bool cpu_rng_unchanged{true};
  bool cuda_rng_unchanged{true};
  bool representation_eval_unchanged{true};
  bool mdn_eval_unchanged{true};
  bool sigma_finite{true};
  bool all_tokens_identity_load_pass{false};
  bool same_loaded_mdn_both_arms{true};
  bool sparse_identity_rejected{false};
  bool sparse_identity_attempt_model_bytes_unchanged{false};
  bool checkpoint_graph_identity_exact{true};
  bool checkpoint_node_order_exact{true};
  std::string sparse_identity_error{};
  std::string graph_order_fingerprint{};
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

[[nodiscard]] Options parse_options(int argc, char **argv) {
  Options out{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--config") {
      out.config_path = require_value(argc, argv, &index, argument);
    } else if (argument == "--input-representation-checkpoint") {
      out.representation_checkpoint_path =
          require_value(argc, argv, &index, argument);
    } else if (argument == "--input-mdn-checkpoint") {
      out.mdn_checkpoint_path = require_value(argc, argv, &index, argument);
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
      out.mdn_checkpoint_path.empty() || out.output_dir.empty() ||
      out.begin < 0 || out.end <= out.begin) {
    throw std::runtime_error(
        "required exactly: --config ABS --input-representation-checkpoint "
        "ABS --input-mdn-checkpoint ABS --output-dir ABS "
        "--anchor-index-begin 760 --anchor-index-end 1088");
  }
  if (out.begin != kStageABegin || out.end != kStageAEnd) {
    throw std::runtime_error(
        "only frozen SRR-3R Stage A range [760,1088) is allowed; "
        "development and final-holdout access are forbidden");
  }
  if (!out.config_path.is_absolute() ||
      !out.representation_checkpoint_path.is_absolute() ||
      !out.output_dir.is_absolute() ||
      !out.mdn_checkpoint_path.is_absolute()) {
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
    throw std::runtime_error("failed to read file for SHA-256: " +
                             path.string());
  }
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] std::string sha256_file(const fs::path &path) {
  return cuwacunu::piaabo::digest::sha256_hex(read_file_bytes(path));
}

void require_authority(const fs::path &path, uintmax_t expected_size,
                       const std::string &expected, const char *kind) {
  if (!fs::is_regular_file(path) || fs::file_size(path) != expected_size) {
    throw std::runtime_error(std::string(kind) + " size mismatch");
  }
  const auto actual = sha256_file(path);
  if (actual != expected) {
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
[[nodiscard]] ModuleStateSnapshot
snapshot_module(const ModuleHolderT &module) {
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
  const auto &expected =
      parameters ? snapshot.parameters : snapshot.buffers;
  if (current.size() != expected.size()) {
    return false;
  }
  for (std::size_t index = 0; index < current.size(); ++index) {
    const auto value =
        current[index].detach().to(torch::kCPU).contiguous();
    if (!tensor_contract_and_bytes_equal(value, expected[index])) {
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
  const auto invalid = pool.valid_mask.logical_not().unsqueeze(-1).expand_as(
      pool.values);
  const auto selected =
      pool.values.masked_select(invalid).detach().to(torch::kCPU).contiguous();
  const auto byte_count = static_cast<std::size_t>(selected.numel()) *
                          static_cast<std::size_t>(selected.element_size());
  if (byte_count == 0) {
    return true;
  }
  const auto *bytes = static_cast<const uint8_t *>(selected.data_ptr());
  return std::all_of(bytes, bytes + byte_count,
                     [](uint8_t value) { return value == 0; });
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

[[nodiscard]] bool mdn_output_finite(const mdn::MdnOut &out) {
  return out.log_pi.defined() && out.mu.defined() && out.sigma.defined() &&
         out.direct_edge_return.defined() &&
         torch::isfinite(out.log_pi).all().template item<bool>() &&
         torch::isfinite(out.mu).all().template item<bool>() &&
         torch::isfinite(out.sigma).all().template item<bool>() &&
         torch::isfinite(out.direct_edge_return).all().template item<bool>();
}

template <typename KeyT>
[[nodiscard]] int64_t append_prediction_rows(
    const fs::path &path, const mdn::MdnOut &output,
    const mdnstream::channel_mdn_input_batch_t<KeyT> &batch,
    int64_t *valid_rows, std::vector<uint8_t> *valid_bits) {
  const auto close_it = std::find(batch.target_coords.begin(),
                                  batch.target_coords.end(),
                                  kCloseFeatureIndex);
  if (close_it == batch.target_coords.end()) {
    throw std::runtime_error("close target feature is absent from MDN batch");
  }
  const int64_t close_slot =
      static_cast<int64_t>(std::distance(batch.target_coords.begin(), close_it));
  const auto prediction = output.direct_edge_return.detach()
                              .to(torch::kCPU)
                              .to(torch::kFloat64)
                              .contiguous();
  const auto sigma = output.sigma.detach()
                         .to(torch::kCPU)
                         .to(torch::kFloat64)
                         .contiguous();
  const auto future = batch.future.detach()
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  const auto combined_mask =
      mdn::combine_channel_context_and_future_mask(batch.context_mask,
                                                   batch.future_mask)
          .detach()
          .to(torch::kCPU)
          .to(torch::kBool)
          .contiguous();
  const auto anchor_keys = batch.anchor_keys.detach()
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();
  const int64_t B = future.size(0);
  const int64_t N = future.size(1);
  const int64_t C = future.size(2);
  if (N != kNodeCount || C != kChannelCount ||
      batch.node_ids.size() != static_cast<std::size_t>(N) ||
      batch.edge_ids.size() != static_cast<std::size_t>(N - 1) ||
      prediction.sizes() != torch::IntArrayRef({B, N - 1, C}) ||
      sigma.size(0) != B || sigma.size(1) != N || sigma.size(2) != C ||
      anchor_keys.size(0) != B) {
    throw std::runtime_error("direct-edge prediction contract mismatch");
  }

  const bool write_header = !fs::exists(path) || fs::file_size(path) == 0;
  std::ofstream out(path, std::ios::app);
  if (!out) {
    throw std::runtime_error("failed to open prediction probe: " +
                             path.string());
  }
  if (write_header) {
    out << "record_schema,anchor_key,anchor_index,anchor_local_index,"
           "edge_index,edge_id,base_node_id,quote_node_id,channel_index,"
           "target_edge_close_return,predicted_edge_close_return,valid,"
           "sigma_finite\n";
  }
  out << std::setprecision(17);
  int64_t rows = 0;
  constexpr int64_t quote = 0;
  for (int64_t b = 0; b < B; ++b) {
    const auto anchor_key =
        anchor_keys.select(0, b).template item<int64_t>();
    const auto anchor_index =
        static_cast<int64_t>(batch.cursor.begin_anchor_index) + b;
    for (int64_t base = 1; base < N; ++base) {
      const int64_t edge = base - 1;
      for (int64_t channel = 0; channel < C; ++channel) {
        const double realized =
            future.index({b, base, channel, close_slot}).template item<double>() -
            future.index({b, quote, channel, close_slot})
                .template item<double>();
        const double raw_predicted =
            prediction.index({b, edge, channel}).template item<double>();
        const bool sigma_ok =
            torch::isfinite(sigma.index({b, base, channel}))
                .all()
                .template item<bool>() &&
            torch::isfinite(sigma.index({b, quote, channel}))
                .all()
                .template item<bool>();
        const bool valid =
            combined_mask.index({b, base, channel, close_slot})
                .template item<bool>() &&
            combined_mask.index({b, quote, channel, close_slot})
                .template item<bool>() &&
            std::isfinite(realized) && std::isfinite(raw_predicted) && sigma_ok;
        const double persisted_prediction = valid ? raw_predicted : 0.0;
        out << "kikijyeba.synthetic.srr3_direct_edge_prediction_probe.v1,"
            << anchor_key << ',' << anchor_index << ',' << b << ',' << edge
            << ',' << batch.edge_ids[static_cast<std::size_t>(edge)] << ','
            << batch.node_ids[static_cast<std::size_t>(base)] << ','
            << batch.node_ids[static_cast<std::size_t>(quote)] << ',' << channel
            << ',' << realized << ',' << persisted_prediction << ','
            << (valid ? "true" : "false") << ','
            << (sigma_ok ? "true" : "false") << '\n';
        ++rows;
        if (valid) {
          ++(*valid_rows);
        }
        valid_bits->push_back(valid ? uint8_t{1} : uint8_t{0});
      }
    }
  }
  if (!out) {
    throw std::runtime_error("failed while writing prediction probe");
  }
  return rows;
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

[[nodiscard]] inference_detail::channel_mdn_checkpoint_identity_t
make_baseline_checkpoint_identity(const Options &options) {
  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path.string());
  configure_frozen_range(bundle, options.begin, options.end);
  protocol::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.compute_alignment_diagnostics = true;
  builder_options.runtime_report_mode =
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::normal;
  protocol::channel_graph_first_pipeline_builder_t<types::kline_t> builder(
      std::move(bundle), builder_options);
  inference::channel_graph_first_inference_launcher_options_t launcher_options{};
  launcher_options.dry_run = true;
  inference::channel_graph_first_inference_launcher_t<types::kline_t> launcher(
      std::move(builder), launcher_options);
  const auto report = launcher.dry_run_report();
  if (report.input_representation_serving_pool_policy != "all_tokens") {
    throw std::runtime_error(
        "active production DSL is not the frozen all_tokens policy");
  }
  return inference_detail::checkpoint_identity_from_report(report);
}

[[nodiscard]] mdn::DirectEdgeReturnHeadOptions
direct_edge_options_from_bundle(
    const protocol::channel_graph_first_config_bundle_t &bundle) {
  mdn::DirectEdgeReturnHeadOptions out{};
  const auto &training = bundle.channel_mdn_training;
  out.identity_mode = training.mdn_direct_edge_return_readout_identity_mode;
  out.base_edge_count =
      training.mdn_direct_edge_return_readout_base_edge_count;
  out.identity_embedding_dim =
      training.mdn_direct_edge_return_readout_identity_embedding_dim;
  out.adapter_hidden_dim =
      training.mdn_direct_edge_return_readout_adapter_hidden_dim;
  return out;
}

[[nodiscard]] std::string one_line(std::string value) {
  std::replace(value.begin(), value.end(), '\n', ' ');
  std::replace(value.begin(), value.end(), '\r', ' ');
  return value;
}

void run(const Options &options) {
  require_authority(options.config_path, kExpectedConfigSize,
                    kExpectedConfigSha256, "base config");
  require_authority(options.representation_checkpoint_path,
                    kExpectedRepresentationCheckpointSize,
                    kExpectedRepresentationCheckpointSha256,
                    "representation checkpoint");
  require_authority(options.mdn_checkpoint_path, kExpectedMdnCheckpointSize,
                    kExpectedMdnCheckpointSha256, "MDN checkpoint");
  const auto config_sha_before = sha256_file(options.config_path);
  const auto representation_checkpoint_sha_before =
      sha256_file(options.representation_checkpoint_path);
  const auto mdn_checkpoint_sha_before = sha256_file(options.mdn_checkpoint_path);

  require_new_output_dir(options.output_dir);
  const fs::path baseline_features =
      options.output_dir / "all_tokens.features.csv";
  const fs::path candidate_features =
      options.output_dir / "structured_cdsb_sparse_v1.features.csv";
  const fs::path baseline_predictions =
      options.output_dir / "all_tokens.predictions.csv";
  const fs::path candidate_predictions =
      options.output_dir / "structured_cdsb_sparse_v1.predictions.csv";
  const fs::path receipt_path = options.output_dir / "mechanics.receipt";

  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path.string());
  if (!protocol::active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    throw std::runtime_error("active protocol does not use MTF representation");
  }
  if (bundle.mtf_jepa_mae_vicreg.config.serving_pool_policy !=
      mtf::mtf_serving_pool_policy_t::all_tokens) {
    throw std::runtime_error(
        "active production DSL must remain exactly all_tokens");
  }
  if (bundle.channel_mdn_training.seed != kSeed) {
    throw std::runtime_error("active MDN seed differs from SRR-3 sentinel 31");
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
    throw std::runtime_error("representation model is not frozen in eval mode");
  }

  Mechanics mechanics{};
  auto mdn_model = builder.make_channel_context_mdn(
      kLatentDim, kChannelCount, /*horizon_count=*/1,
      direct_edge_options_from_bundle(builder.bundle()));
  ++mechanics.mdn_model_constructions;
  const auto baseline_identity = make_baseline_checkpoint_identity(options);
  if (baseline_identity.input_representation_serving_pool_policy !=
      "all_tokens") {
    throw std::runtime_error("historical checkpoint identity is not all_tokens");
  }

  const auto identity_attempt_state = snapshot_module(mdn_model);
  auto sparse_identity = baseline_identity;
  sparse_identity.input_representation_serving_pool_policy =
      "structured_cdsb_sparse_v1";
  ++mechanics.mdn_identity_authentication_attempts;
  try {
    inference_detail::load_channel_mdn_checkpoint_file(
        options.mdn_checkpoint_path, mdn_model, nullptr, &sparse_identity);
    ++mechanics.mdn_successful_weight_loads;
  } catch (const std::exception &error) {
    mechanics.sparse_identity_error = one_line(error.what());
    mechanics.sparse_identity_rejected =
        mechanics.sparse_identity_error.find(
            "input representation serving pool policy does not match current "
            "config") != std::string::npos;
  }
  mechanics.sparse_identity_attempt_model_bytes_unchanged =
      module_state_equal(mdn_model, identity_attempt_state, true) &&
      module_state_equal(mdn_model, identity_attempt_state, false);
  if (!mechanics.sparse_identity_rejected ||
      !mechanics.sparse_identity_attempt_model_bytes_unchanged ||
      mechanics.mdn_successful_weight_loads != 0) {
    throw std::runtime_error(
        "sparse expected checkpoint identity was not safely rejected before "
        "weights loaded");
  }

  ++mechanics.mdn_identity_authentication_attempts;
  inference_detail::load_channel_mdn_checkpoint_file(
      options.mdn_checkpoint_path, mdn_model, nullptr, &baseline_identity);
  ++mechanics.mdn_successful_weight_loads;
  mechanics.all_tokens_identity_load_pass = true;
  inference_detail::freeze_vicreg_encoder(mdn_model);
  if (mdn_model->is_training() || !all_parameters_frozen(mdn_model)) {
    throw std::runtime_error("MDN model is not frozen in eval mode");
  }

  const auto representation_state_before = snapshot_module(representation_model);
  const auto mdn_state_before = snapshot_module(mdn_model);
  const auto rng_before = generator_snapshot(builder.options().device);
  const bool representation_mode_before = representation_model->is_training();
  const bool mdn_mode_before = mdn_model->is_training();

  std::size_t next_expected_anchor = static_cast<std::size_t>(options.begin);
  while (lifted_stream.has_next()) {
    auto lifted = lifted_stream.next();
    ++mechanics.source_batches;
    if (mechanics.source_batches > 6) {
      throw std::runtime_error("Stage A source batch ceiling exceeded");
    }
    const auto batch_anchors =
        static_cast<int64_t>(lifted.cursor.anchor_count());
    mechanics.maximum_source_batch_anchors =
        std::max(mechanics.maximum_source_batch_anchors, batch_anchors);
    if (batch_anchors > 64) {
      throw std::runtime_error("Stage A source batch size exceeded 64 anchors");
    }
    if (lifted.cursor.begin_anchor_index != next_expected_anchor ||
        lifted.cursor.anchor_indices.size() != lifted.cursor.anchor_count()) {
      throw std::runtime_error("source cursor is not contiguous and exact");
    }
    for (std::size_t index = 0; index < lifted.cursor.anchor_indices.size();
         ++index) {
      if (lifted.cursor.anchor_indices[index] != next_expected_anchor + index) {
        throw std::runtime_error("source anchor ordering changed");
      }
    }
    next_expected_anchor += lifted.cursor.anchor_count();
    if (lifted.cursor.end_anchor_index != next_expected_anchor ||
        next_expected_anchor > static_cast<std::size_t>(options.end)) {
      throw std::runtime_error("source cursor escaped the frozen range");
    }
    if (mechanics.graph_order_fingerprint.empty()) {
      mechanics.graph_order_fingerprint = lifted.graph_order_fingerprint;
    } else if (mechanics.graph_order_fingerprint !=
               lifted.graph_order_fingerprint) {
      throw std::runtime_error("graph order fingerprint changed");
    }
    mechanics.checkpoint_graph_identity_exact =
        mechanics.checkpoint_graph_identity_exact &&
        lifted.graph_order_fingerprint ==
            baseline_identity.graph_order_fingerprint;
    mechanics.checkpoint_node_order_exact =
        mechanics.checkpoint_node_order_exact &&
        lifted.node_ids == baseline_identity.node_ids;
    if (!mechanics.checkpoint_graph_identity_exact ||
        !mechanics.checkpoint_node_order_exact) {
      throw std::runtime_error(
          "runtime graph/node order differs from historical MDN identity");
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
    const bool encoded_stable_after = encoded_bytes_equal(encoded, encoded_before);
    const bool converted_data_stable_after =
        tensor_contract_and_bytes_equal(data, converted_data_before);
    const bool converted_mask_stable_after =
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);

    const int64_t rows = input.data.size(0);
    const bool batch_public_contract =
        public_contract_exact(baseline, candidate, rows);
    const bool batch_public_masks_exact =
        batch_public_contract && tensor_contract_and_bytes_equal(
                                    baseline.valid_mask, candidate.valid_mask);
    const bool batch_invalid_zero_exact =
        batch_public_contract && invalid_values_zero(baseline) &&
        invalid_values_zero(candidate);
    const bool batch_outputs_finite =
        batch_public_contract &&
        torch::isfinite(baseline.values).all().template item<bool>() &&
        torch::isfinite(candidate.values).all().template item<bool>();
    const bool batch_input_data_unchanged =
        tensor_contract_and_bytes_equal(input.data, input_data_before);
    const bool batch_input_mask_unchanged =
        tensor_contract_and_bytes_equal(input.feature_mask, input_mask_before);

    mechanics.encoded_bytes_stable =
        mechanics.encoded_bytes_stable && encoded_stable_between &&
        encoded_stable_after;
    mechanics.public_contract_exact =
        mechanics.public_contract_exact && batch_public_contract;
    mechanics.public_masks_exact =
        mechanics.public_masks_exact && batch_public_masks_exact;
    mechanics.invalid_zero_exact =
        mechanics.invalid_zero_exact && batch_invalid_zero_exact;
    mechanics.selector_outputs_finite =
        mechanics.selector_outputs_finite && batch_outputs_finite;
    mechanics.input_data_unchanged =
        mechanics.input_data_unchanged && batch_input_data_unchanged;
    mechanics.input_mask_unchanged =
        mechanics.input_mask_unchanged && batch_input_mask_unchanged;
    mechanics.converted_data_bytes_stable =
        mechanics.converted_data_bytes_stable &&
        converted_data_stable_after_encode && converted_data_stable_between &&
        converted_data_stable_after;
    mechanics.converted_feature_mask_bytes_stable =
        mechanics.converted_feature_mask_bytes_stable &&
        converted_mask_stable_after_encode && converted_mask_stable_between &&
        converted_mask_stable_after;
    if (!mechanics.encoded_bytes_stable ||
        !mechanics.public_contract_exact || !mechanics.public_masks_exact ||
        !mechanics.invalid_zero_exact || !mechanics.selector_outputs_finite ||
        !mechanics.input_data_unchanged || !mechanics.input_mask_unchanged ||
        !mechanics.converted_data_bytes_stable ||
        !mechanics.converted_feature_mask_bytes_stable) {
      throw std::runtime_error("paired sparse readout raw-byte mechanics failed");
    }
    mechanics.public_rows += rows;
    mechanics.public_mask_cells += baseline.valid_mask.numel();
    mechanics.baseline_valid_cells +=
        baseline.valid_mask.sum().template item<int64_t>();
    mechanics.candidate_valid_cells +=
        candidate.valid_mask.sum().template item<int64_t>();

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
      throw std::runtime_error("paired graph/MDN adapter contract changed");
    }

    mechanics.baseline_feature_rows +=
        inference_detail::append_representation_edge_feature_probe_rows(
            baseline_features, baseline_representation, baseline_batch,
            kCloseFeatureIndex);
    mechanics.candidate_feature_rows +=
        inference_detail::append_representation_edge_feature_probe_rows(
            candidate_features, candidate_representation, candidate_batch,
            kCloseFeatureIndex);

    auto baseline_input = mdnstream::to_channel_mdn_input(baseline_batch);
    auto candidate_input = mdnstream::to_channel_mdn_input(candidate_batch);
    inference_detail::move_channel_mdn_input_to_device(
        baseline_input, builder.options().dtype, builder.options().device);
    inference_detail::move_channel_mdn_input_to_device(
        candidate_input, builder.options().dtype, builder.options().device);
    const auto baseline_output = mdn_model->forward(
        baseline_input.context, baseline_input.context_mask);
    ++mechanics.mdn_forwards;
    const auto candidate_output = mdn_model->forward(
        candidate_input.context, candidate_input.context_mask);
    ++mechanics.mdn_forwards;
    const bool baseline_finite = mdn_output_finite(baseline_output);
    const bool candidate_finite = mdn_output_finite(candidate_output);
    mechanics.mdn_outputs_finite = mechanics.mdn_outputs_finite &&
                                  baseline_finite && candidate_finite;
    mechanics.sigma_finite =
        mechanics.sigma_finite &&
        torch::isfinite(baseline_output.sigma).all().template item<bool>() &&
        torch::isfinite(candidate_output.sigma).all().template item<bool>();
    if (!mechanics.mdn_outputs_finite || !mechanics.sigma_finite) {
      throw std::runtime_error("non-finite frozen MDN output");
    }
    std::vector<uint8_t> baseline_valid_bits{};
    std::vector<uint8_t> candidate_valid_bits{};
    mechanics.baseline_prediction_rows += append_prediction_rows(
        baseline_predictions, baseline_output, baseline_batch,
        &mechanics.baseline_valid_prediction_rows, &baseline_valid_bits);
    mechanics.candidate_prediction_rows += append_prediction_rows(
        candidate_predictions, candidate_output, candidate_batch,
        &mechanics.candidate_valid_prediction_rows, &candidate_valid_bits);
    mechanics.prediction_valid_bits_exact =
        mechanics.prediction_valid_bits_exact &&
        baseline_valid_bits == candidate_valid_bits;
    if (!mechanics.prediction_valid_bits_exact) {
      throw std::runtime_error("paired prediction validity bits changed by arm");
    }

    mechanics.encoded_bytes_stable =
        mechanics.encoded_bytes_stable &&
        encoded_bytes_equal(encoded, encoded_before);
    mechanics.input_data_unchanged =
        mechanics.input_data_unchanged &&
        tensor_contract_and_bytes_equal(input.data, input_data_before);
    mechanics.input_mask_unchanged =
        mechanics.input_mask_unchanged &&
        tensor_contract_and_bytes_equal(input.feature_mask, input_mask_before);
    mechanics.converted_data_bytes_stable =
        mechanics.converted_data_bytes_stable &&
        tensor_contract_and_bytes_equal(data, converted_data_before);
    mechanics.converted_feature_mask_bytes_stable =
        mechanics.converted_feature_mask_bytes_stable &&
        tensor_contract_and_bytes_equal(feature_mask,
                                        converted_feature_mask_before);
  }

  const auto rng_after = generator_snapshot(builder.options().device);
  mechanics.cpu_rng_unchanged =
      tensor_equal_optional(rng_before.cpu_state, rng_after.cpu_state);
  mechanics.cuda_rng_unchanged =
      tensor_equal_optional(rng_before.cuda_state, rng_after.cuda_state);
  mechanics.representation_parameters_unchanged = module_state_equal(
      representation_model, representation_state_before, true);
  mechanics.representation_buffers_unchanged = module_state_equal(
      representation_model, representation_state_before, false);
  mechanics.representation_eval_unchanged =
      representation_model->is_training() == representation_mode_before &&
      !representation_model->is_training();
  mechanics.mdn_parameters_unchanged =
      module_state_equal(mdn_model, mdn_state_before, true);
  mechanics.mdn_buffers_unchanged =
      module_state_equal(mdn_model, mdn_state_before, false);
  mechanics.mdn_eval_unchanged =
      mdn_model->is_training() == mdn_mode_before && !mdn_model->is_training();

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_public_rows = expected_anchors * kNodeCount;
  const int64_t expected_mask_cells = expected_public_rows * kChannelCount;
  const int64_t expected_rows = expected_anchors * 9;
  const bool hard_gate =
      mechanics.anchors == expected_anchors &&
      next_expected_anchor == static_cast<std::size_t>(options.end) &&
      mechanics.source_batches <= 6 &&
      mechanics.source_batches > 0 &&
      mechanics.maximum_source_batch_anchors <= 64 &&
      mechanics.encoder_calls == mechanics.source_batches &&
      mechanics.public_rows == expected_public_rows &&
      mechanics.public_mask_cells == expected_mask_cells &&
      mechanics.baseline_valid_cells == expected_mask_cells &&
      mechanics.candidate_valid_cells == expected_mask_cells &&
      mechanics.baseline_feature_rows == expected_rows &&
      mechanics.candidate_feature_rows == expected_rows &&
      mechanics.baseline_prediction_rows == expected_rows &&
      mechanics.candidate_prediction_rows == expected_rows &&
      mechanics.baseline_valid_prediction_rows ==
          mechanics.candidate_valid_prediction_rows &&
      mechanics.mdn_forwards == 2 * mechanics.source_batches &&
      mechanics.mdn_model_constructions == 1 &&
      mechanics.mdn_identity_authentication_attempts == 2 &&
      mechanics.mdn_successful_weight_loads == 1 &&
      mechanics.all_tokens_identity_load_pass &&
      mechanics.same_loaded_mdn_both_arms &&
      mechanics.sparse_identity_rejected &&
      mechanics.sparse_identity_attempt_model_bytes_unchanged &&
      mechanics.same_encoded_object && mechanics.encoded_bytes_stable &&
      mechanics.public_contract_exact && mechanics.public_masks_exact &&
      mechanics.invalid_zero_exact && mechanics.selector_outputs_finite &&
      mechanics.mdn_outputs_finite &&
      mechanics.sigma_finite && mechanics.paired_adapter_contract_exact &&
      mechanics.prediction_valid_bits_exact &&
      mechanics.input_data_unchanged && mechanics.input_mask_unchanged &&
      mechanics.converted_data_bytes_stable &&
      mechanics.converted_feature_mask_bytes_stable &&
      mechanics.checkpoint_graph_identity_exact &&
      mechanics.checkpoint_node_order_exact &&
      mechanics.representation_parameters_unchanged &&
      mechanics.representation_buffers_unchanged &&
      mechanics.mdn_parameters_unchanged && mechanics.mdn_buffers_unchanged &&
      mechanics.cpu_rng_unchanged && mechanics.cuda_rng_unchanged &&
      mechanics.representation_eval_unchanged && mechanics.mdn_eval_unchanged;
  if (!hard_gate) {
    throw std::runtime_error("SRR-3R Stage A pre-metric mechanics gate failed");
  }

  const auto config_sha_after = sha256_file(options.config_path);
  const auto representation_checkpoint_sha_after =
      sha256_file(options.representation_checkpoint_path);
  const auto mdn_checkpoint_sha_after = sha256_file(options.mdn_checkpoint_path);
  const bool config_bytes_unchanged = config_sha_before == config_sha_after &&
                                      fs::file_size(options.config_path) ==
                                          kExpectedConfigSize;
  const bool representation_checkpoint_bytes_unchanged =
      representation_checkpoint_sha_before ==
          representation_checkpoint_sha_after &&
      fs::file_size(options.representation_checkpoint_path) ==
          kExpectedRepresentationCheckpointSize;
  const bool mdn_checkpoint_bytes_unchanged =
      mdn_checkpoint_sha_before == mdn_checkpoint_sha_after &&
      fs::file_size(options.mdn_checkpoint_path) == kExpectedMdnCheckpointSize;
  if (!config_bytes_unchanged ||
      !representation_checkpoint_bytes_unchanged ||
      !mdn_checkpoint_bytes_unchanged ||
      config_sha_before != kExpectedConfigSha256 ||
      representation_checkpoint_sha_before !=
          kExpectedRepresentationCheckpointSha256 ||
      mdn_checkpoint_sha_before != kExpectedMdnCheckpointSha256) {
    throw std::runtime_error("authority input bytes changed during capture");
  }

  require_authority(baseline_features, kExpectedBaselineFeatureSize,
                    kExpectedBaselineFeatureSha256,
                    "fresh all_tokens feature artifact");
  require_authority(candidate_features, kExpectedCandidateFeatureSize,
                    kExpectedCandidateFeatureSha256,
                    "fresh structured_cdsb_sparse_v1 feature artifact");
  const auto baseline_feature_sha = sha256_file(baseline_features);
  const auto candidate_feature_sha = sha256_file(candidate_features);
  const auto baseline_prediction_sha = sha256_file(baseline_predictions);
  const auto candidate_prediction_sha = sha256_file(candidate_predictions);
  if (baseline_feature_sha != sha256_file(baseline_features) ||
      candidate_feature_sha != sha256_file(candidate_features) ||
      baseline_prediction_sha != sha256_file(baseline_predictions) ||
      candidate_prediction_sha != sha256_file(candidate_predictions)) {
    throw std::runtime_error("persisted replay artifact hash changed");
  }

  std::ofstream receipt(receipt_path, std::ios::trunc);
  if (!receipt) {
    throw std::runtime_error("failed to create mechanics receipt");
  }
  receipt << std::boolalpha;
  receipt << "schema_id=cuwacunu.srr3r.sparse_activation_capture.mechanics.v1\n";
  receipt << "status=mechanics_pass\n";
  receipt << "stage=stage_a\n";
  receipt << "sealed_protocol_sha256=" << kSealedProtocolSha256 << "\n";
  receipt << "sealed_protocol_size=" << kSealedProtocolSize << "\n";
  receipt << "range_id=historical_confirmation_760_1088\n";
  receipt << "anchor_range=[" << options.begin << ',' << options.end << ")\n";
  receipt << "anchor_count=" << mechanics.anchors << "\n";
  receipt << "maximum_anchor_read=" << options.end - 1 << "\n";
  receipt << "final_holdout_range=[" << kFinalHoldoutBegin << ','
          << kFinalHoldoutEnd << ")\n";
  receipt << "final_holdout_access=false\n";
  receipt << "seed=" << kSeed << "\n";
  receipt << "source_order=contiguous_sequential_anchor_index\n";
  receipt << "active_production_policy=all_tokens\n";
  receipt << "baseline_policy=all_tokens\n";
  receipt << "candidate_policy=structured_cdsb_sparse_v1\n";
  receipt << "loaded_mdn_checkpoint_policy=all_tokens\n";
  receipt << "sparse_identity_expected_policy=structured_cdsb_sparse_v1\n";
  receipt << "candidate_computation=legacy_all_tokens_head_on_sparse_semantics\n";
  receipt << "historical_checkpoint_identity_preserved=true\n";
  receipt << "candidate_checkpoint_identity=historical_all_tokens\n";
  receipt << "sparse_identity_authentication_expected_rejection=true\n";
  receipt << "candidate_checkpoint_identity_relabelled=false\n";
  receipt << "candidate_checkpoint_identity_migrated=false\n";
  receipt << "candidate_checkpoint_copied=false\n";
  receipt << "candidate_checkpoint_rewritten=false\n";
  receipt << "candidate_checkpoint_identity_bypassed=false\n";
  receipt << "same_loaded_mdn_both_arms="
          << mechanics.same_loaded_mdn_both_arms << "\n";
  receipt << "source_batches=" << mechanics.source_batches << "\n";
  receipt << "source_batch_ceiling=6\n";
  receipt << "source_batch_size_ceiling=64\n";
  receipt << "maximum_source_batch_anchors="
          << mechanics.maximum_source_batch_anchors << "\n";
  receipt << "encoder_calls=" << mechanics.encoder_calls << "\n";
  receipt << "encoder_calls_equal_source_batches=true\n";
  receipt << "mdn_model_constructions=" << mechanics.mdn_model_constructions
          << "\n";
  receipt << "mdn_forwards=" << mechanics.mdn_forwards << "\n";
  receipt << "mdn_identity_authentication_attempts="
          << mechanics.mdn_identity_authentication_attempts << "\n";
  receipt << "mdn_successful_weight_loads="
          << mechanics.mdn_successful_weight_loads << "\n";
  receipt << "same_encoded_object=" << mechanics.same_encoded_object << "\n";
  receipt << "same_retained_encoded_object=" << mechanics.same_encoded_object
          << "\n";
  receipt << "encoded_bytes_stable=" << mechanics.encoded_bytes_stable << "\n";
  receipt << "public_contract_exact=" << mechanics.public_contract_exact
          << "\n";
  receipt << "selector_contract_exact=" << mechanics.public_contract_exact
          << "\n";
  receipt << "public_masks_exact=" << mechanics.public_masks_exact << "\n";
  receipt << "context_masks_exact=" << mechanics.public_masks_exact << "\n";
  receipt << "invalid_zero_exact=" << mechanics.invalid_zero_exact << "\n";
  receipt << "paired_adapter_contract_exact="
          << mechanics.paired_adapter_contract_exact << "\n";
  receipt << "paired_rows_keys_targets_order_exact="
          << mechanics.paired_adapter_contract_exact << "\n";
  receipt << "prediction_valid_bits_exact="
          << mechanics.prediction_valid_bits_exact << "\n";
  receipt << "input_data_unchanged=" << mechanics.input_data_unchanged
          << "\n";
  receipt << "input_mask_unchanged=" << mechanics.input_mask_unchanged
          << "\n";
  receipt << "converted_data_bytes_stable="
          << mechanics.converted_data_bytes_stable << "\n";
  receipt << "converted_feature_mask_bytes_stable="
          << mechanics.converted_feature_mask_bytes_stable << "\n";
  receipt << "selector_outputs_finite="
          << mechanics.selector_outputs_finite << "\n";
  receipt << "mdn_outputs_finite=" << mechanics.mdn_outputs_finite << "\n";
  receipt << "sigma_finite=" << mechanics.sigma_finite << "\n";
  receipt << "representation_parameters_unchanged="
          << mechanics.representation_parameters_unchanged << "\n";
  receipt << "representation_buffers_unchanged="
          << mechanics.representation_buffers_unchanged << "\n";
  receipt << "mdn_parameters_unchanged=" << mechanics.mdn_parameters_unchanged
          << "\n";
  receipt << "mdn_buffers_unchanged=" << mechanics.mdn_buffers_unchanged
          << "\n";
  receipt << "cpu_rng_unchanged=" << mechanics.cpu_rng_unchanged << "\n";
  receipt << "cuda_rng_unchanged=" << mechanics.cuda_rng_unchanged << "\n";
  receipt << "representation_eval_unchanged="
          << mechanics.representation_eval_unchanged << "\n";
  receipt << "mdn_eval_unchanged=" << mechanics.mdn_eval_unchanged << "\n";
  receipt << "all_tokens_identity_load_pass="
          << mechanics.all_tokens_identity_load_pass << "\n";
  receipt << "sparse_identity_rejected="
          << mechanics.sparse_identity_rejected << "\n";
  receipt << "sparse_identity_attempt_model_bytes_unchanged="
          << mechanics.sparse_identity_attempt_model_bytes_unchanged << "\n";
  receipt << "sparse_identity_error=" << mechanics.sparse_identity_error
          << "\n";
  receipt << "checkpoint_graph_identity_exact="
          << mechanics.checkpoint_graph_identity_exact << "\n";
  receipt << "checkpoint_node_order_exact="
          << mechanics.checkpoint_node_order_exact << "\n";
  receipt << "graph_order_fingerprint="
          << mechanics.graph_order_fingerprint << "\n";
  receipt << "feature_count=" << kFeatureCount << "\n";
  receipt << "public_rows=" << mechanics.public_rows << "\n";
  receipt << "public_mask_cells=" << mechanics.public_mask_cells << "\n";
  receipt << "context_mask_cells=" << mechanics.public_mask_cells << "\n";
  receipt << "baseline_valid_cells=" << mechanics.baseline_valid_cells
          << "\n";
  receipt << "baseline_context_valid_cells="
          << mechanics.baseline_valid_cells << "\n";
  receipt << "candidate_valid_cells=" << mechanics.candidate_valid_cells
          << "\n";
  receipt << "candidate_context_valid_cells="
          << mechanics.candidate_valid_cells << "\n";
  receipt << "baseline_feature_rows=" << mechanics.baseline_feature_rows
          << "\n";
  receipt << "candidate_feature_rows=" << mechanics.candidate_feature_rows
          << "\n";
  receipt << "baseline_prediction_rows="
          << mechanics.baseline_prediction_rows << "\n";
  receipt << "candidate_prediction_rows="
          << mechanics.candidate_prediction_rows << "\n";
  receipt << "baseline_prediction_valid_rows="
          << mechanics.baseline_valid_prediction_rows << "\n";
  receipt << "candidate_prediction_valid_rows="
          << mechanics.candidate_valid_prediction_rows << "\n";
  receipt << "prediction_valid_coverage_exact="
          << (mechanics.baseline_valid_prediction_rows ==
              mechanics.candidate_valid_prediction_rows)
          << "\n";
  receipt << "observed_paired_prediction_valid_rows="
          << mechanics.baseline_valid_prediction_rows << "\n";
  receipt << "config_path=" << options.config_path.string() << "\n";
  receipt << "config_size=" << fs::file_size(options.config_path) << "\n";
  receipt << "config_sha256=" << config_sha_after << "\n";
  receipt << "config_bytes_unchanged=" << config_bytes_unchanged << "\n";
  receipt << "representation_checkpoint_path="
          << options.representation_checkpoint_path.string() << "\n";
  receipt << "representation_checkpoint_size="
          << fs::file_size(options.representation_checkpoint_path) << "\n";
  receipt << "representation_checkpoint_sha256="
          << representation_checkpoint_sha_after << "\n";
  receipt << "representation_checkpoint_bytes_unchanged="
          << representation_checkpoint_bytes_unchanged << "\n";
  receipt << "mdn_checkpoint_path=" << options.mdn_checkpoint_path.string()
          << "\n";
  receipt << "mdn_checkpoint_size="
          << fs::file_size(options.mdn_checkpoint_path) << "\n";
  receipt << "mdn_checkpoint_sha256=" << mdn_checkpoint_sha_after << "\n";
  receipt << "mdn_checkpoint_bytes_unchanged="
          << mdn_checkpoint_bytes_unchanged << "\n";
  receipt << "baseline_feature_path=" << baseline_features.string() << "\n";
  receipt << "baseline_feature_size=" << fs::file_size(baseline_features)
          << "\n";
  receipt << "baseline_feature_sha256=" << baseline_feature_sha << "\n";
  receipt << "baseline_feature_frozen_hash_match=true\n";
  receipt << "baseline_frozen_hash_match=true\n";
  receipt << "candidate_feature_path=" << candidate_features.string() << "\n";
  receipt << "candidate_feature_size=" << fs::file_size(candidate_features)
          << "\n";
  receipt << "candidate_feature_sha256=" << candidate_feature_sha << "\n";
  receipt << "candidate_feature_frozen_hash_match=true\n";
  receipt << "candidate_frozen_hash_match=true\n";
  receipt << "baseline_prediction_path="
          << baseline_predictions.string() << "\n";
  receipt << "baseline_prediction_size="
          << fs::file_size(baseline_predictions) << "\n";
  receipt << "baseline_prediction_sha256=" << baseline_prediction_sha << "\n";
  receipt << "candidate_prediction_path="
          << candidate_predictions.string() << "\n";
  receipt << "candidate_prediction_size="
          << fs::file_size(candidate_predictions) << "\n";
  receipt << "candidate_prediction_sha256=" << candidate_prediction_sha
          << "\n";
  receipt << "augmentations_enabled=false\n";
  receipt << "optimizer_steps=0\n";
  receipt << "backward_calls=0\n";
  receipt << "checkpoint_writes=0\n";
  receipt << "policy_activation_changed=false\n";
  receipt << "endpoint_metrics_computed=false\n";
  receipt.flush();
  if (!receipt) {
    throw std::runtime_error("failed while writing mechanics receipt");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    run(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "cuwacunu_srr3r_sparse_activation_capture: " << error.what()
              << "\n";
    return 1;
  }
}
