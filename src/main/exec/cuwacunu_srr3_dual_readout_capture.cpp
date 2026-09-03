// SPDX-License-Identifier: MIT

#include <ATen/CPUGeneratorImpl.h>
#include <ATen/cuda/CUDAGeneratorImpl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

struct Options {
  fs::path config_path{};
  fs::path representation_checkpoint_path{};
  fs::path mdn_checkpoint_path{};
  fs::path output_dir{};
  int64_t begin{-1};
  int64_t end{-1};
  std::string range_id{};
  bool stage_a{false};
  bool mask_census_only{false};
};

struct GeneratorStateSnapshot {
  torch::Tensor cpu_state{};
  torch::Tensor cuda_state{};
};

struct ModuleStateSnapshot {
  std::vector<torch::Tensor> parameters{};
  std::vector<torch::Tensor> buffers{};
  uint64_t parameter_hash{0};
  uint64_t buffer_hash{0};
};

struct Mechanics {
  int64_t source_batches{0};
  int64_t encoder_calls{0};
  int64_t anchors{0};
  int64_t baseline_feature_rows{0};
  int64_t candidate_feature_rows{0};
  int64_t baseline_prediction_rows{0};
  int64_t candidate_prediction_rows{0};
  int64_t valid_prediction_rows{0};
  int64_t tokenizer_calls{0};
  int64_t mask_cells{0};
  int64_t baseline_valid_cells{0};
  int64_t candidate_valid_cells{0};
  int64_t both_valid_cells{0};
  int64_t baseline_only_cells{0};
  int64_t candidate_only_cells{0};
  int64_t both_invalid_cells{0};
  std::array<int64_t, kChannelCount> channel_mask_cells{};
  std::array<int64_t, kChannelCount> channel_baseline_valid_cells{};
  std::array<int64_t, kChannelCount> channel_candidate_valid_cells{};
  std::array<int64_t, kChannelCount> channel_both_valid_cells{};
  std::array<int64_t, kChannelCount> channel_baseline_only_cells{};
  std::array<int64_t, kChannelCount> channel_candidate_only_cells{};
  std::array<int64_t, kChannelCount> channel_both_invalid_cells{};
  bool same_encoded_object{true};
  bool pool_contract_exact{true};
  bool pool_masks_exact{true};
  bool invalid_zero_exact{true};
  bool paired_adapter_contract_exact{true};
  bool representation_parameters_unchanged{true};
  bool representation_buffers_unchanged{true};
  bool mdn_parameters_unchanged{true};
  bool mdn_buffers_unchanged{true};
  bool cpu_rng_unchanged{true};
  bool cuda_rng_unchanged{true};
  bool representation_eval_unchanged{true};
  bool mdn_eval_unchanged{true};
  bool all_outputs_finite{true};
  bool sigma_finite{true};
  bool all_tokens_identity_load_pass{false};
  bool structured_identity_rejected{false};
  bool structured_identity_attempt_model_unchanged{false};
  std::string structured_identity_error{};
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

void classify_range(Options &options) {
  if (options.begin == kStageABegin && options.end == kStageAEnd) {
    options.range_id = "historical_confirmation_760_1088";
    options.stage_a = true;
  } else if (options.begin == 0 && options.end == 554) {
    options.range_id = "development_selection_fit_0_554";
  } else if (options.begin == 584 && options.end == 730) {
    options.range_id = "development_validation_584_730";
  } else if (options.begin == 0 && options.end == 730) {
    options.range_id = "development_refit_0_730";
  } else {
    throw std::runtime_error(
        "only frozen SRR-3 ranges [0,554), [584,730), [0,730), or "
        "[760,1088) are allowed; final holdout [1088,1170) is forbidden");
  }
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
    } else if (argument == "--mask-census-only") {
      out.mask_census_only = true;
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  if (out.config_path.empty() || out.representation_checkpoint_path.empty() ||
      out.output_dir.empty() || out.begin < 0 || out.end <= out.begin) {
    throw std::runtime_error(
        "required: --config ABS --input-representation-checkpoint ABS "
        "--output-dir ABS --anchor-index-begin N --anchor-index-end N; "
        "Stage A also requires --input-mdn-checkpoint ABS");
  }
  classify_range(out);
  if (out.mask_census_only && !out.stage_a) {
    throw std::runtime_error(
        "--mask-census-only is restricted to Stage A [760,1088)");
  }
  if (out.mask_census_only && !out.mdn_checkpoint_path.empty()) {
    throw std::runtime_error(
        "--mask-census-only forbids MDN checkpoint access");
  }
  if (out.stage_a && !out.mask_census_only &&
      out.mdn_checkpoint_path.empty()) {
    throw std::runtime_error(
        "Stage A [760,1088) requires --input-mdn-checkpoint");
  }
  if (!out.config_path.is_absolute() ||
      !out.representation_checkpoint_path.is_absolute() ||
      !out.output_dir.is_absolute() ||
      (!out.mdn_checkpoint_path.empty() &&
       !out.mdn_checkpoint_path.is_absolute())) {
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

void require_sha256(const fs::path &path, const char *expected,
                    const char *kind) {
  const auto actual = sha256_file(path);
  if (actual != expected) {
    throw std::runtime_error(std::string(kind) + " SHA-256 mismatch: " +
                             actual);
  }
}

void mix_hash(uint64_t &hash, uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] uint64_t hash_tensor(const torch::Tensor &input) {
  if (!input.defined()) {
    return 0x9e3779b97f4a7c15ULL;
  }
  const auto contiguous = input.detach().to(torch::kCPU).contiguous();
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash(hash, static_cast<uint64_t>(contiguous.scalar_type()));
  mix_hash(hash, static_cast<uint64_t>(contiguous.dim()));
  for (const auto size : contiguous.sizes()) {
    mix_hash(hash, static_cast<uint64_t>(size));
  }
  const auto byte_count = static_cast<std::size_t>(contiguous.numel()) *
                          static_cast<std::size_t>(contiguous.element_size());
  const auto *bytes = static_cast<const uint8_t *>(contiguous.data_ptr());
  for (std::size_t index = 0; index < byte_count; ++index) {
    mix_hash(hash, static_cast<uint64_t>(bytes[index]));
  }
  return hash;
}

[[nodiscard]] uint64_t
hash_encoded(const mtf::mtf_jepa_mae_vicreg_encode_output_t &encoded) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto &tensor :
       {encoded.embeddings, encoded.pooled_embedding,
        encoded.pooled_by_channel, encoded.pooled_time,
        encoded.pooled_frequency, encoded.token_mask,
        encoded.sample_valid_mask, encoded.channel_valid_mask,
        encoded.metadata.start_index, encoded.metadata.width,
        encoded.metadata.scale_id, encoded.metadata.channel_id,
        encoded.metadata.domain_id}) {
    mix_hash(hash, hash_tensor(tensor));
  }
  return hash;
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
  if (left.defined() != right.defined()) {
    return false;
  }
  return !left.defined() || torch::equal(left, right);
}

template <typename ModuleHolderT>
[[nodiscard]] ModuleStateSnapshot
snapshot_module(const ModuleHolderT &module) {
  ModuleStateSnapshot out{};
  for (const auto &parameter : module->parameters()) {
    out.parameters.push_back(
        parameter.detach().to(torch::kCPU).contiguous().clone());
    mix_hash(out.parameter_hash, hash_tensor(out.parameters.back()));
  }
  for (const auto &buffer : module->buffers()) {
    out.buffers.push_back(buffer.detach().to(torch::kCPU).contiguous().clone());
    mix_hash(out.buffer_hash, hash_tensor(out.buffers.back()));
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
    if (value.scalar_type() != expected[index].scalar_type() ||
        value.sizes() != expected[index].sizes() ||
        !torch::equal(value, expected[index])) {
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

[[nodiscard]] bool pool_contract_exact(
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
  const auto selected = pool.values.masked_select(invalid);
  return selected.numel() == 0 || selected.eq(0).all().template item<bool>();
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_encode_output_t
make_mask_census_carrier(
    const mtf::mtf_token_batch_t &tokenized,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  const auto batch_size = tokenized.token_mask.size(0);
  const auto token_count = tokenized.token_mask.size(1);
  const auto options = tokenized.tokens.options();
  mtf::mtf_jepa_mae_vicreg_encode_output_t out{};
  out.embeddings =
      torch::zeros({batch_size, token_count, config.latent_dim}, options);
  out.pooled_embedding =
      torch::zeros({batch_size, config.latent_dim}, options);
  out.pooled_by_channel = torch::zeros(
      {batch_size, config.channel_count, config.latent_dim}, options);
  out.pooled_time = torch::zeros({batch_size, config.latent_dim}, options);
  out.pooled_frequency =
      torch::zeros({batch_size, config.latent_dim}, options);
  out.token_mask = tokenized.token_mask;
  out.sample_valid_mask = tokenized.token_mask.any(/*dim=*/1);
  out.channel_valid_mask = mtf::detail::channel_valid_mask(
      tokenized.metadata, tokenized.token_mask, config);
  out.metadata = tokenized.metadata;
  return out;
}

void record_mask_census(
    Mechanics &mechanics, const mtf::mtf_serving_pool_output_t &baseline,
    const mtf::mtf_serving_pool_output_t &candidate) {
  const auto baseline_mask = baseline.valid_mask.to(torch::kBool);
  const auto candidate_mask = candidate.valid_mask.to(torch::kBool);
  const auto both_valid = baseline_mask.logical_and(candidate_mask);
  const auto baseline_only =
      baseline_mask.logical_and(candidate_mask.logical_not());
  const auto candidate_only =
      candidate_mask.logical_and(baseline_mask.logical_not());
  const auto both_invalid =
      baseline_mask.logical_not().logical_and(candidate_mask.logical_not());
  const auto count = [](const torch::Tensor &mask) {
    return mask.sum().template item<int64_t>();
  };

  mechanics.mask_cells += baseline_mask.numel();
  mechanics.baseline_valid_cells += count(baseline_mask);
  mechanics.candidate_valid_cells += count(candidate_mask);
  mechanics.both_valid_cells += count(both_valid);
  mechanics.baseline_only_cells += count(baseline_only);
  mechanics.candidate_only_cells += count(candidate_only);
  mechanics.both_invalid_cells += count(both_invalid);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    mechanics.channel_mask_cells[index] += baseline_mask.size(0);
    mechanics.channel_baseline_valid_cells[index] +=
        count(baseline_mask.select(/*dim=*/1, channel));
    mechanics.channel_candidate_valid_cells[index] +=
        count(candidate_mask.select(/*dim=*/1, channel));
    mechanics.channel_both_valid_cells[index] +=
        count(both_valid.select(/*dim=*/1, channel));
    mechanics.channel_baseline_only_cells[index] +=
        count(baseline_only.select(/*dim=*/1, channel));
    mechanics.channel_candidate_only_cells[index] +=
        count(candidate_only.select(/*dim=*/1, channel));
    mechanics.channel_both_invalid_cells[index] +=
        count(both_invalid.select(/*dim=*/1, channel));
  }
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
         torch::equal(baseline.context_mask, candidate.context_mask) &&
         torch::equal(baseline.future, candidate.future) &&
         torch::equal(baseline.future_mask, candidate.future_mask) &&
         torch::equal(baseline.anchor_keys, candidate.anchor_keys) &&
         torch::equal(baseline.anchor_index, candidate.anchor_index) &&
         torch::equal(baseline.node_index, candidate.node_index) &&
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
    int64_t *valid_rows) {
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
  if (!(options.end <= kFinalHoldoutBegin ||
        options.begin >= kFinalHoldoutEnd)) {
    throw std::runtime_error("final holdout access is forbidden");
  }
  require_sha256(options.config_path, kExpectedConfigSha256, "base config");
  require_sha256(options.representation_checkpoint_path,
                 kExpectedRepresentationCheckpointSha256,
                 "representation checkpoint");
  if (options.stage_a && !options.mask_census_only) {
    require_sha256(options.mdn_checkpoint_path, kExpectedMdnCheckpointSha256,
                   "MDN checkpoint");
  }
  const auto config_sha_before = sha256_file(options.config_path);
  const auto representation_checkpoint_sha_before =
      sha256_file(options.representation_checkpoint_path);
  const auto mdn_checkpoint_sha_before =
      options.stage_a && !options.mask_census_only
          ? sha256_file(options.mdn_checkpoint_path)
          : "not_accessed";

  require_new_output_dir(options.output_dir);
  const fs::path baseline_features =
      options.output_dir / "all_tokens.features.csv";
  const fs::path candidate_features =
      options.output_dir / "structured_cdsb_v1.features.csv";
  const fs::path baseline_predictions =
      options.output_dir / "all_tokens.predictions.csv";
  const fs::path candidate_predictions =
      options.output_dir / "structured_cdsb_v1.predictions.csv";
  const fs::path receipt_path =
      options.output_dir /
      (options.mask_census_only ? "mask_contract_census.receipt"
                                : "mechanics.receipt");

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
  std::optional<mdn::ChannelContextMdn> mdn_model{};
  if (options.stage_a && !options.mask_census_only) {
    auto model = builder.make_channel_context_mdn(
        kLatentDim, kChannelCount, /*horizon_count=*/1,
        direct_edge_options_from_bundle(builder.bundle()));
    auto baseline_identity = make_baseline_checkpoint_identity(options);
    if (baseline_identity.input_representation_serving_pool_policy !=
        "all_tokens") {
      throw std::runtime_error("baseline checkpoint identity is not all_tokens");
    }
    inference_detail::load_channel_mdn_checkpoint_file(
        options.mdn_checkpoint_path, model, nullptr, &baseline_identity);
    mechanics.all_tokens_identity_load_pass = true;
    inference_detail::freeze_vicreg_encoder(model);
    if (model->is_training() || !all_parameters_frozen(model)) {
      throw std::runtime_error("MDN model is not frozen in eval mode");
    }

    const auto identity_attempt_state = snapshot_module(model);
    auto structured_identity = baseline_identity;
    structured_identity.input_representation_serving_pool_policy =
        "structured_cdsb_v1";
    try {
      inference_detail::load_channel_mdn_checkpoint_file(
          options.mdn_checkpoint_path, model, nullptr, &structured_identity);
    } catch (const std::exception &error) {
      mechanics.structured_identity_error = one_line(error.what());
      mechanics.structured_identity_rejected =
          mechanics.structured_identity_error.find("serving pool policy") !=
          std::string::npos;
    }
    mechanics.structured_identity_attempt_model_unchanged =
        module_state_equal(model, identity_attempt_state, true) &&
        module_state_equal(model, identity_attempt_state, false);
    if (!mechanics.structured_identity_rejected ||
        !mechanics.structured_identity_attempt_model_unchanged) {
      throw std::runtime_error(
          "structured checkpoint identity was not safely rejected");
    }
    mdn_model = std::move(model);
  }

  const auto representation_state_before = snapshot_module(representation_model);
  std::optional<ModuleStateSnapshot> mdn_state_before{};
  if (mdn_model.has_value()) {
    mdn_state_before = snapshot_module(*mdn_model);
  }
  const auto rng_before = generator_snapshot(builder.options().device);
  const bool representation_mode_before = representation_model->is_training();
  const bool mdn_mode_before =
      mdn_model.has_value() ? (*mdn_model)->is_training() : false;

  std::size_t next_expected_anchor = static_cast<std::size_t>(options.begin);
  while (lifted_stream.has_next()) {
    auto lifted = lifted_stream.next();
    ++mechanics.source_batches;
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
    mechanics.anchors += static_cast<int64_t>(lifted.cursor.anchor_count());

    auto input = mtf::make_mtf_channel_node_input(lifted);
    const auto input_data_hash_before = hash_tensor(input.data);
    const auto input_mask_hash_before = hash_tensor(input.feature_mask);
    const auto tensor_options =
        torch::TensorOptions().dtype(config.dtype).device(config.device);
    const auto data = input.data.to(tensor_options);
    const auto feature_mask = input.feature_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config.device));

    torch::NoGradGuard no_grad;
    mtf::mtf_jepa_mae_vicreg_encode_output_t encoded{};
    if (options.mask_census_only) {
      const auto tokenized =
          representation_model->tokenize(data, feature_mask);
      ++mechanics.tokenizer_calls;
      encoded = make_mask_census_carrier(tokenized, config);
    } else {
      encoded = representation_model->encode(data, feature_mask);
      ++mechanics.encoder_calls;
    }
    const auto encoded_hash_before = hash_encoded(encoded);
    const auto baseline = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, config);
    const auto encoded_hash_between = hash_encoded(encoded);
    const auto candidate = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_v1, config);
    const auto encoded_hash_after = hash_encoded(encoded);

    const bool batch_same_encoded_object =
        encoded_hash_before == encoded_hash_between &&
        encoded_hash_between == encoded_hash_after;
    const bool batch_pool_contract_exact =
        pool_contract_exact(baseline, candidate, input.data.size(0));
    const bool batch_pool_masks_exact =
        torch::equal(baseline.valid_mask, candidate.valid_mask);
    const bool batch_invalid_zero_exact =
        invalid_values_zero(baseline) && invalid_values_zero(candidate);
    const bool batch_outputs_finite =
        torch::isfinite(baseline.values).all().template item<bool>() &&
        torch::isfinite(candidate.values).all().template item<bool>();
    const bool batch_input_data_unchanged =
        hash_tensor(input.data) == input_data_hash_before;
    const bool batch_input_mask_unchanged =
        hash_tensor(input.feature_mask) == input_mask_hash_before;

    mechanics.same_encoded_object =
        mechanics.same_encoded_object && batch_same_encoded_object;
    mechanics.pool_contract_exact =
        mechanics.pool_contract_exact && batch_pool_contract_exact;
    mechanics.pool_masks_exact =
        mechanics.pool_masks_exact && batch_pool_masks_exact;
    mechanics.invalid_zero_exact =
        mechanics.invalid_zero_exact && batch_invalid_zero_exact;
    mechanics.all_outputs_finite =
        mechanics.all_outputs_finite && batch_outputs_finite;
    if (options.mask_census_only) {
      record_mask_census(mechanics, baseline, candidate);
    }
    if (!mechanics.same_encoded_object || !mechanics.pool_contract_exact ||
        (!options.mask_census_only && !mechanics.pool_masks_exact) ||
        !mechanics.invalid_zero_exact ||
        !mechanics.all_outputs_finite || !batch_input_data_unchanged ||
        !batch_input_mask_unchanged) {
      std::ostringstream detail;
      detail << std::boolalpha
             << "dual-readout production seam mechanics failed: "
             << "same_encoded_object=" << batch_same_encoded_object << ' '
             << "pool_contract_exact=" << batch_pool_contract_exact << ' '
             << "pool_masks_exact=" << batch_pool_masks_exact << ' '
             << "invalid_zero_exact=" << batch_invalid_zero_exact << ' '
             << "outputs_finite=" << batch_outputs_finite << ' '
             << "input_data_unchanged=" << batch_input_data_unchanged << ' '
             << "input_mask_unchanged=" << batch_input_mask_unchanged;
      throw std::runtime_error(detail.str());
    }

    if (options.mask_census_only) {
      continue;
    }

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

    if (mdn_model.has_value()) {
      auto baseline_input = mdnstream::to_channel_mdn_input(baseline_batch);
      auto candidate_input = mdnstream::to_channel_mdn_input(candidate_batch);
      inference_detail::move_channel_mdn_input_to_device(
          baseline_input, builder.options().dtype, builder.options().device);
      inference_detail::move_channel_mdn_input_to_device(
          candidate_input, builder.options().dtype, builder.options().device);
      const auto baseline_output = (*mdn_model)->forward(
          baseline_input.context, baseline_input.context_mask);
      const auto candidate_output = (*mdn_model)->forward(
          candidate_input.context, candidate_input.context_mask);
      const bool baseline_finite = mdn_output_finite(baseline_output);
      const bool candidate_finite = mdn_output_finite(candidate_output);
      mechanics.all_outputs_finite = mechanics.all_outputs_finite &&
                                     baseline_finite && candidate_finite;
      mechanics.sigma_finite =
          mechanics.sigma_finite &&
          torch::isfinite(baseline_output.sigma).all().template item<bool>() &&
          torch::isfinite(candidate_output.sigma).all().template item<bool>();
      if (!mechanics.all_outputs_finite || !mechanics.sigma_finite) {
        throw std::runtime_error("non-finite frozen MDN output");
      }
      int64_t baseline_valid_rows = 0;
      int64_t candidate_valid_rows = 0;
      mechanics.baseline_prediction_rows += append_prediction_rows(
          baseline_predictions, baseline_output, baseline_batch,
          &baseline_valid_rows);
      mechanics.candidate_prediction_rows += append_prediction_rows(
          candidate_predictions, candidate_output, candidate_batch,
          &candidate_valid_rows);
      if (baseline_valid_rows != candidate_valid_rows) {
        throw std::runtime_error("paired prediction validity changed by arm");
      }
      mechanics.valid_prediction_rows += baseline_valid_rows;
    }
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
  if (mdn_model.has_value()) {
    mechanics.mdn_parameters_unchanged =
        module_state_equal(*mdn_model, *mdn_state_before, true);
    mechanics.mdn_buffers_unchanged =
        module_state_equal(*mdn_model, *mdn_state_before, false);
    mechanics.mdn_eval_unchanged =
        (*mdn_model)->is_training() == mdn_mode_before &&
        !(*mdn_model)->is_training();
  }

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_rows = expected_anchors * 9;
  const bool common_state_gate =
      mechanics.anchors == expected_anchors &&
      next_expected_anchor == static_cast<std::size_t>(options.end) &&
      mechanics.representation_parameters_unchanged &&
      mechanics.representation_buffers_unchanged &&
      mechanics.cpu_rng_unchanged && mechanics.cuda_rng_unchanged &&
      mechanics.representation_eval_unchanged;
  if (options.mask_census_only) {
    const int64_t expected_mask_cells =
        expected_anchors * kNodeCount * kChannelCount;
    bool channel_partitions_exact = true;
    for (std::size_t channel = 0;
         channel < static_cast<std::size_t>(kChannelCount); ++channel) {
      channel_partitions_exact =
          channel_partitions_exact &&
          mechanics.channel_mask_cells[channel] ==
              expected_anchors * kNodeCount &&
          mechanics.channel_both_valid_cells[channel] +
                  mechanics.channel_baseline_only_cells[channel] +
                  mechanics.channel_candidate_only_cells[channel] +
                  mechanics.channel_both_invalid_cells[channel] ==
              mechanics.channel_mask_cells[channel];
    }
    if (!common_state_gate || mechanics.source_batches > 6 ||
        mechanics.tokenizer_calls != mechanics.source_batches ||
        mechanics.encoder_calls != 0 ||
        mechanics.mask_cells != expected_mask_cells ||
        mechanics.both_valid_cells + mechanics.baseline_only_cells +
                mechanics.candidate_only_cells +
                mechanics.both_invalid_cells !=
            mechanics.mask_cells ||
        mechanics.baseline_valid_cells !=
            mechanics.both_valid_cells + mechanics.baseline_only_cells ||
        mechanics.candidate_valid_cells !=
            mechanics.both_valid_cells + mechanics.candidate_only_cells ||
        mechanics.pool_masks_exact ||
        mechanics.baseline_only_cells + mechanics.candidate_only_cells == 0 ||
        !mechanics.same_encoded_object || !mechanics.pool_contract_exact ||
        !mechanics.invalid_zero_exact || !mechanics.all_outputs_finite ||
        !channel_partitions_exact || mechanics.baseline_feature_rows != 0 ||
        mechanics.candidate_feature_rows != 0 ||
        mechanics.baseline_prediction_rows != 0 ||
        mechanics.candidate_prediction_rows != 0) {
      throw std::runtime_error("SRR-3 mask census hard gate failed");
    }
  } else if (!common_state_gate ||
             mechanics.encoder_calls != mechanics.source_batches ||
             mechanics.baseline_feature_rows != expected_rows ||
             mechanics.candidate_feature_rows != expected_rows ||
             (options.stage_a && mechanics.encoder_calls > 6) ||
             (options.stage_a &&
              (mechanics.baseline_prediction_rows != expected_rows ||
               mechanics.candidate_prediction_rows != expected_rows)) ||
             (options.stage_a &&
              (!mechanics.mdn_parameters_unchanged ||
               !mechanics.mdn_buffers_unchanged ||
               !mechanics.mdn_eval_unchanged))) {
    throw std::runtime_error("SRR-3 hard mechanics gate failed");
  }

  const auto config_sha_after = sha256_file(options.config_path);
  const auto representation_checkpoint_sha_after =
      sha256_file(options.representation_checkpoint_path);
  const auto mdn_checkpoint_sha_after =
      options.stage_a && !options.mask_census_only
          ? sha256_file(options.mdn_checkpoint_path)
          : "not_accessed";
  if (config_sha_before != config_sha_after ||
      representation_checkpoint_sha_before !=
          representation_checkpoint_sha_after ||
      mdn_checkpoint_sha_before != mdn_checkpoint_sha_after) {
    throw std::runtime_error("authority input bytes changed during capture");
  }

  if (options.mask_census_only) {
    const auto fraction = [](int64_t numerator, int64_t denominator) {
      return denominator > 0
                 ? static_cast<double>(numerator) /
                       static_cast<double>(denominator)
                 : 0.0;
    };
    std::ofstream receipt(receipt_path, std::ios::trunc);
    if (!receipt) {
      throw std::runtime_error("failed to create mask census receipt");
    }
    receipt << std::boolalpha << std::setprecision(17);
    receipt << "schema_id=cuwacunu.srr3.mask_contract_census.v1\n";
    receipt << "status=mask_contract_incompatible\n";
    receipt << "stage_a_gate_status=mechanics_failed_pre_metric\n";
    receipt << "stage_b_authorized=false\n";
    receipt << "final_decision=downstream_bottleneck_remains_unresolved\n";
    receipt << "range_id=" << options.range_id << "\n";
    receipt << "anchor_range=[" << options.begin << ',' << options.end
            << ")\n";
    receipt << "anchor_count=" << mechanics.anchors << "\n";
    receipt << "maximum_anchor_read=" << options.end - 1 << "\n";
    receipt << "final_holdout_access=false\n";
    receipt << "seed=" << kSeed << "\n";
    receipt << "source_order=contiguous_sequential_anchor_index\n";
    receipt << "source_batches=" << mechanics.source_batches << "\n";
    receipt << "tokenizer_calls=" << mechanics.tokenizer_calls << "\n";
    receipt << "encoder_calls=0\n";
    receipt << "mdn_checkpoint_access=false\n";
    receipt << "mdn_forwards=0\n";
    receipt << "baseline_policy=all_tokens\n";
    receipt << "candidate_policy=structured_cdsb_v1\n";
    receipt << "same_carrier_object=" << mechanics.same_encoded_object
            << "\n";
    receipt << "pool_contract_exact=" << mechanics.pool_contract_exact
            << "\n";
    receipt << "pool_masks_exact=" << mechanics.pool_masks_exact << "\n";
    receipt << "invalid_zero_exact=" << mechanics.invalid_zero_exact << "\n";
    receipt << "all_outputs_finite=" << mechanics.all_outputs_finite << "\n";
    receipt << "mask_cells=" << mechanics.mask_cells << "\n";
    receipt << "baseline_valid_cells=" << mechanics.baseline_valid_cells
            << "\n";
    receipt << "candidate_valid_cells=" << mechanics.candidate_valid_cells
            << "\n";
    receipt << "both_valid_cells=" << mechanics.both_valid_cells << "\n";
    receipt << "baseline_only_cells=" << mechanics.baseline_only_cells
            << "\n";
    receipt << "candidate_only_cells=" << mechanics.candidate_only_cells
            << "\n";
    receipt << "both_invalid_cells=" << mechanics.both_invalid_cells << "\n";
    receipt << "mismatched_cells="
            << mechanics.baseline_only_cells + mechanics.candidate_only_cells
            << "\n";
    receipt << "baseline_valid_fraction="
            << fraction(mechanics.baseline_valid_cells, mechanics.mask_cells)
            << "\n";
    receipt << "candidate_valid_fraction="
            << fraction(mechanics.candidate_valid_cells, mechanics.mask_cells)
            << "\n";
    receipt << "mismatch_fraction="
            << fraction(mechanics.baseline_only_cells +
                            mechanics.candidate_only_cells,
                        mechanics.mask_cells)
            << "\n";
    for (std::size_t channel = 0;
         channel < static_cast<std::size_t>(kChannelCount); ++channel) {
      const auto prefix = "channel_" + std::to_string(channel) + ".";
      receipt << prefix << "mask_cells="
              << mechanics.channel_mask_cells[channel] << "\n";
      receipt << prefix << "baseline_valid_cells="
              << mechanics.channel_baseline_valid_cells[channel] << "\n";
      receipt << prefix << "candidate_valid_cells="
              << mechanics.channel_candidate_valid_cells[channel] << "\n";
      receipt << prefix << "both_valid_cells="
              << mechanics.channel_both_valid_cells[channel] << "\n";
      receipt << prefix << "baseline_only_cells="
              << mechanics.channel_baseline_only_cells[channel] << "\n";
      receipt << prefix << "candidate_only_cells="
              << mechanics.channel_candidate_only_cells[channel] << "\n";
      receipt << prefix << "both_invalid_cells="
              << mechanics.channel_both_invalid_cells[channel] << "\n";
    }
    receipt << "representation_parameters_unchanged="
            << mechanics.representation_parameters_unchanged << "\n";
    receipt << "representation_buffers_unchanged="
            << mechanics.representation_buffers_unchanged << "\n";
    receipt << "representation_eval_unchanged="
            << mechanics.representation_eval_unchanged << "\n";
    receipt << "cpu_rng_unchanged=" << mechanics.cpu_rng_unchanged << "\n";
    receipt << "cuda_rng_unchanged=" << mechanics.cuda_rng_unchanged << "\n";
    receipt << "config_sha256=" << config_sha_after << "\n";
    receipt << "representation_checkpoint_sha256="
            << representation_checkpoint_sha_after << "\n";
    receipt << "augmentations_enabled=false\n";
    receipt << "feature_artifacts_written=false\n";
    receipt << "prediction_artifacts_written=false\n";
    receipt << "endpoint_metrics_computed=false\n";
    receipt << "optimizer_steps=0\n";
    receipt << "backward_calls=0\n";
    receipt << "checkpoint_writes=0\n";
    receipt.flush();
    if (!receipt) {
      throw std::runtime_error("failed while writing mask census receipt");
    }
    return;
  }

  const auto baseline_feature_sha = sha256_file(baseline_features);
  const auto candidate_feature_sha = sha256_file(candidate_features);
  const auto baseline_prediction_sha =
      options.stage_a ? sha256_file(baseline_predictions) : "not_emitted";
  const auto candidate_prediction_sha =
      options.stage_a ? sha256_file(candidate_predictions) : "not_emitted";
  if (baseline_feature_sha != sha256_file(baseline_features) ||
      candidate_feature_sha != sha256_file(candidate_features) ||
      (options.stage_a &&
       (baseline_prediction_sha != sha256_file(baseline_predictions) ||
        candidate_prediction_sha != sha256_file(candidate_predictions)))) {
    throw std::runtime_error("persisted replay artifact hash changed");
  }

  std::ofstream receipt(receipt_path, std::ios::trunc);
  if (!receipt) {
    throw std::runtime_error("failed to create mechanics receipt");
  }
  receipt << std::boolalpha;
  receipt << "schema_id=cuwacunu.srr3.dual_readout_capture.mechanics.v1\n";
  receipt << "status=mechanics_pass\n";
  receipt << "stage=" << (options.stage_a ? "stage_a" : "stage_b_feature_capture")
          << "\n";
  receipt << "range_id=" << options.range_id << "\n";
  receipt << "anchor_range=[" << options.begin << ',' << options.end << ")\n";
  receipt << "anchor_count=" << mechanics.anchors << "\n";
  receipt << "maximum_anchor_read=" << options.end - 1 << "\n";
  receipt << "final_holdout_access=false\n";
  receipt << "seed=" << kSeed << "\n";
  receipt << "source_order=contiguous_sequential_anchor_index\n";
  receipt << "active_production_policy=all_tokens\n";
  receipt << "baseline_policy=all_tokens\n";
  receipt << "candidate_policy=structured_cdsb_v1\n";
  receipt << "source_batches=" << mechanics.source_batches << "\n";
  receipt << "encoder_calls=" << mechanics.encoder_calls << "\n";
  receipt << "encoder_calls_equal_source_batches=true\n";
  receipt << "same_encoded_object=" << mechanics.same_encoded_object << "\n";
  receipt << "pool_contract_exact=" << mechanics.pool_contract_exact << "\n";
  receipt << "pool_masks_exact=" << mechanics.pool_masks_exact << "\n";
  receipt << "invalid_zero_exact=" << mechanics.invalid_zero_exact << "\n";
  receipt << "paired_adapter_contract_exact="
          << mechanics.paired_adapter_contract_exact << "\n";
  receipt << "all_outputs_finite=" << mechanics.all_outputs_finite << "\n";
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
  receipt << "structured_identity_rejected="
          << mechanics.structured_identity_rejected << "\n";
  receipt << "structured_identity_attempt_model_unchanged="
          << mechanics.structured_identity_attempt_model_unchanged << "\n";
  receipt << "structured_identity_error="
          << mechanics.structured_identity_error << "\n";
  receipt << "graph_order_fingerprint="
          << mechanics.graph_order_fingerprint << "\n";
  receipt << "feature_count=" << kFeatureCount << "\n";
  receipt << "baseline_feature_rows=" << mechanics.baseline_feature_rows
          << "\n";
  receipt << "candidate_feature_rows=" << mechanics.candidate_feature_rows
          << "\n";
  receipt << "baseline_prediction_rows="
          << mechanics.baseline_prediction_rows << "\n";
  receipt << "candidate_prediction_rows="
          << mechanics.candidate_prediction_rows << "\n";
  receipt << "valid_prediction_rows=" << mechanics.valid_prediction_rows
          << "\n";
  receipt << "config_sha256=" << config_sha_after << "\n";
  receipt << "representation_checkpoint_sha256="
          << representation_checkpoint_sha_after << "\n";
  receipt << "mdn_checkpoint_sha256=" << mdn_checkpoint_sha_after << "\n";
  receipt << "baseline_feature_path=" << baseline_features.string() << "\n";
  receipt << "baseline_feature_sha256=" << baseline_feature_sha << "\n";
  receipt << "candidate_feature_path=" << candidate_features.string() << "\n";
  receipt << "candidate_feature_sha256=" << candidate_feature_sha << "\n";
  receipt << "baseline_prediction_path="
          << (options.stage_a ? baseline_predictions.string() : "not_emitted")
          << "\n";
  receipt << "baseline_prediction_sha256=" << baseline_prediction_sha << "\n";
  receipt << "candidate_prediction_path="
          << (options.stage_a ? candidate_predictions.string() : "not_emitted")
          << "\n";
  receipt << "candidate_prediction_sha256=" << candidate_prediction_sha
          << "\n";
  receipt << "augmentations_enabled=false\n";
  receipt << "optimizer_steps=0\n";
  receipt << "backward_calls=0\n";
  receipt << "checkpoint_writes=0\n";
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
    std::cerr << "cuwacunu_srr3_dual_readout_capture: " << error.what()
              << "\n";
    return 1;
  }
}
