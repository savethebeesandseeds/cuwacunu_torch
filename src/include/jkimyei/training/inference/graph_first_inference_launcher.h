// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "jkimyei/training/inference/mdn_trainer.h"
#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::jkimyei::training::inference {

struct graph_first_inference_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::kikijyeba::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct graph_first_inference_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  int64_t context_dim{0};
  std::string head_policy{};
  std::string target_domain{};
  std::string activity_target_semantics{};
  int64_t mixture_count{0};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
  std::string representation_checkpoint_path{};
  bool representation_checkpoint_loaded{false};
  std::string mdn_checkpoint_path{};
  bool mdn_checkpoint_loaded{false};
  bool allow_untrained_representation{false};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  std::string analytics_status{};
  std::string wave_id{};
  std::string wave_mode{};
  std::string target_action{};
  std::string wave_source_cursor_kind{};
  std::string wave_source_cursor_scope{};
  std::string wave_source_range_policy{};
  int64_t requested_anchor_index_begin{-1};
  int64_t requested_anchor_index_end{-1};
  std::string runtime_report_mode{};
  std::string stream_plan{};
  std::string source_cursor_token{};
  int64_t source_anchor_count{0};
  int64_t source_candidate_anchor_count{0};
  int64_t source_skipped_anchor_count{0};
  int64_t source_skipped_outside_common_range{0};
  int64_t source_skipped_missing_edge_coverage{0};
  int64_t source_skipped_failed_fetch_probe{0};
  int64_t source_duplicate_anchor_count{0};
  double source_accepted_anchor_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  std::string source_anchor_domain_warning_level{"none"};
  std::string source_anchor_domain_warnings{};
  std::string source_common_left_key{};
  std::string source_common_right_key{};
  std::string source_reference_left_key{};
  std::string source_reference_right_key{};
  std::string source_first_anchor_key{};
  std::string source_last_anchor_key{};
  int64_t wave_streamed_anchor_count{0};
  std::string wave_first_anchor_key{};
  std::string wave_last_anchor_key{};
  int64_t wave_pulses_attempted{0};
  int64_t wave_pulses_completed{0};
  int64_t wave_pulses_skipped{0};

  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};
  bool all_target_masks_forced_empty{false};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  int64_t last_valid_target_count{0};
  int64_t total_valid_target_count{0};
  int64_t last_valid_row_count{0};
  int64_t total_valid_row_count{0};
  int64_t last_skipped_node_head_count{0};
  int64_t total_skipped_node_head_count{0};
  int64_t last_active_node_head_count{0};
  int64_t total_active_node_head_count{0};
  int64_t last_trained_node_head_count{0};
  int64_t total_trained_node_head_count{0};
  int64_t last_evaluated_node_head_count{0};
  int64_t total_evaluated_node_head_count{0};
  double last_valid_node_target_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_valid_node_target_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  bool low_target_coverage_warning{false};
  std::vector<int64_t> routed_row_count_by_node{};
  std::vector<int64_t> active_row_count_by_node{};
  std::vector<int64_t> trained_row_count_by_node{};
  std::vector<int64_t> evaluated_row_count_by_node{};
  std::vector<int64_t> valid_target_count_by_node{};
  std::vector<double> mean_nll_by_node{};

  double mean_node_mask_coverage{std::numeric_limits<double>::quiet_NaN()};
  double mean_future_mask_coverage{std::numeric_limits<double>::quiet_NaN()};
  double mean_price_residual_energy{std::numeric_limits<double>::quiet_NaN()};
  double mean_node_context_norm_mean{std::numeric_limits<double>::quiet_NaN()};
  double max_node_context_norm_max{std::numeric_limits<double>::quiet_NaN()};
  double mean_mixture_entropy_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_mixture_entropy_min{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};

  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  int64_t last_checkpoint_optimizer_step{0};
  std::string checkpoint_path{};
  std::string checkpoint_format{};
  bool report_written{false};
  int64_t report_write_count{0};
  int64_t last_report_attempted_step{0};
  std::string report_path{};
  bool runtime_lls_emitted{false};
  std::string nodelift_runtime_lls{};
  std::string representation_runtime_lls{};
  std::string mdn_runtime_lls{};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream oss;
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "context_dim=" << context_dim << "\n";
    oss << "head_policy=" << head_policy << "\n";
    oss << "target_domain=" << target_domain << "\n";
    oss << "activity_target_semantics=" << activity_target_semantics << "\n";
    oss << "mixture_count=" << mixture_count << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
    oss << "representation_checkpoint_path=" << representation_checkpoint_path
        << "\n";
    oss << "representation_checkpoint_loaded="
        << (representation_checkpoint_loaded ? "true" : "false") << "\n";
    oss << "mdn_checkpoint_path=" << mdn_checkpoint_path << "\n";
    oss << "mdn_checkpoint_loaded="
        << (mdn_checkpoint_loaded ? "true" : "false") << "\n";
    oss << "allow_untrained_representation="
        << (allow_untrained_representation ? "true" : "false") << "\n";
    oss << "checkpoint_every=" << checkpoint_every << "\n";
    oss << "report_every=" << report_every << "\n";
    oss << "validation_every=" << validation_every << "\n";
    oss << "analytics_status=" << analytics_status << "\n";
    oss << "wave_id=" << wave_id << "\n";
    oss << "wave_mode=" << wave_mode << "\n";
    oss << "target_action=" << target_action << "\n";
    oss << "wave_source_cursor_kind=" << wave_source_cursor_kind << "\n";
    oss << "wave_source_cursor_scope=" << wave_source_cursor_scope << "\n";
    oss << "wave_source_range_policy=" << wave_source_range_policy << "\n";
    oss << "requested_anchor_index_begin=" << requested_anchor_index_begin
        << "\n";
    oss << "requested_anchor_index_end=" << requested_anchor_index_end << "\n";
    oss << "runtime_report_mode=" << runtime_report_mode << "\n";
    oss << "stream_plan=" << stream_plan << "\n";
    oss << "source_cursor_token=" << source_cursor_token << "\n";
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "source_candidate_anchor_count=" << source_candidate_anchor_count
        << "\n";
    oss << "source_skipped_anchor_count=" << source_skipped_anchor_count
        << "\n";
    oss << "source_skipped_outside_common_range="
        << source_skipped_outside_common_range << "\n";
    oss << "source_skipped_missing_edge_coverage="
        << source_skipped_missing_edge_coverage << "\n";
    oss << "source_skipped_failed_fetch_probe="
        << source_skipped_failed_fetch_probe << "\n";
    oss << "source_duplicate_anchor_count=" << source_duplicate_anchor_count
        << "\n";
    oss << "source_accepted_anchor_fraction=" << source_accepted_anchor_fraction
        << "\n";
    oss << "source_anchor_domain_warning_level="
        << source_anchor_domain_warning_level << "\n";
    oss << "source_anchor_domain_warnings=" << source_anchor_domain_warnings
        << "\n";
    oss << "source_common_left_key=" << source_common_left_key << "\n";
    oss << "source_common_right_key=" << source_common_right_key << "\n";
    oss << "source_reference_left_key=" << source_reference_left_key << "\n";
    oss << "source_reference_right_key=" << source_reference_right_key << "\n";
    oss << "source_first_anchor_key=" << source_first_anchor_key << "\n";
    oss << "source_last_anchor_key=" << source_last_anchor_key << "\n";
    oss << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    oss << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    oss << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    oss << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    oss << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    oss << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    oss << "target_coords=";
    for (std::size_t i = 0; i < target_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << target_coords[i];
    }
    oss << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << edge_ids[i];
    }
    oss << "\nnode_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << node_ids[i];
    }
    oss << "\nsteps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "all_target_masks_forced_empty="
        << (all_target_masks_forced_empty ? "true" : "false") << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_valid_target_count=" << last_valid_target_count << "\n";
    oss << "total_valid_target_count=" << total_valid_target_count << "\n";
    oss << "last_valid_row_count=" << last_valid_row_count << "\n";
    oss << "total_valid_row_count=" << total_valid_row_count << "\n";
    oss << "last_skipped_node_head_count=" << last_skipped_node_head_count
        << "\n";
    oss << "total_skipped_node_head_count=" << total_skipped_node_head_count
        << "\n";
    oss << "last_active_node_head_count=" << last_active_node_head_count
        << "\n";
    oss << "total_active_node_head_count=" << total_active_node_head_count
        << "\n";
    oss << "last_trained_node_head_count=" << last_trained_node_head_count
        << "\n";
    oss << "total_trained_node_head_count=" << total_trained_node_head_count
        << "\n";
    oss << "last_evaluated_node_head_count=" << last_evaluated_node_head_count
        << "\n";
    oss << "total_evaluated_node_head_count=" << total_evaluated_node_head_count
        << "\n";
    oss << "last_valid_node_target_fraction=" << last_valid_node_target_fraction
        << "\n";
    oss << "mean_valid_node_target_fraction=" << mean_valid_node_target_fraction
        << "\n";
    oss << "low_target_coverage_warning="
        << (low_target_coverage_warning ? "true" : "false") << "\n";
    oss << "routed_row_count_by_node=";
    for (std::size_t i = 0; i < routed_row_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << routed_row_count_by_node[i];
    }
    oss << "\nactive_row_count_by_node=";
    for (std::size_t i = 0; i < active_row_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << active_row_count_by_node[i];
    }
    oss << "\ntrained_row_count_by_node=";
    for (std::size_t i = 0; i < trained_row_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << trained_row_count_by_node[i];
    }
    oss << "\nevaluated_row_count_by_node=";
    for (std::size_t i = 0; i < evaluated_row_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << evaluated_row_count_by_node[i];
    }
    oss << "\n";
    oss << "valid_target_count_by_node=";
    for (std::size_t i = 0; i < valid_target_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << valid_target_count_by_node[i];
    }
    oss << "\nmean_nll_by_node=";
    for (std::size_t i = 0; i < mean_nll_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << mean_nll_by_node[i];
    }
    oss << "\n";
    oss << "mean_node_mask_coverage=" << mean_node_mask_coverage << "\n";
    oss << "mean_future_mask_coverage=" << mean_future_mask_coverage << "\n";
    oss << "mean_price_residual_energy=" << mean_price_residual_energy << "\n";
    oss << "mean_node_context_norm_mean=" << mean_node_context_norm_mean
        << "\n";
    oss << "max_node_context_norm_max=" << max_node_context_norm_max << "\n";
    oss << "mean_mixture_entropy_mean=" << mean_mixture_entropy_mean << "\n";
    oss << "min_mixture_entropy_min=" << min_mixture_entropy_min << "\n";
    oss << "mean_sigma_mean=" << mean_sigma_mean << "\n";
    oss << "min_sigma_min=" << min_sigma_min << "\n";
    oss << "max_sigma_max=" << max_sigma_max << "\n";
    oss << "finite_parameter_check=" << finite_parameter_check << "\n";
    oss << "checkpoint_written=" << (checkpoint_written ? "true" : "false")
        << "\n";
    oss << "checkpoint_write_count=" << checkpoint_write_count << "\n";
    oss << "last_checkpoint_optimizer_step=" << last_checkpoint_optimizer_step
        << "\n";
    oss << "checkpoint_path=" << checkpoint_path << "\n";
    oss << "checkpoint_format=" << checkpoint_format << "\n";
    oss << "report_written=" << (report_written ? "true" : "false") << "\n";
    oss << "report_write_count=" << report_write_count << "\n";
    oss << "last_report_attempted_step=" << last_report_attempted_step << "\n";
    oss << "report_path=" << report_path << "\n";
    oss << "runtime_lls_emitted=" << (runtime_lls_emitted ? "true" : "false")
        << "\n";
    return oss.str();
  }
};

namespace graph_first_inference_launcher_detail {

struct mdn_checkpoint_identity_t {
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<int64_t> target_coords{};
  int64_t context_dim{0};
  int64_t mixture_count{0};
};

inline double tensor_mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .mean()
      .item<double>();
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

inline std::string seed_torch_runtime(uint64_t seed,
                                      const torch::Device &device) {
  torch::manual_seed(seed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(seed);
    return "torch_manual_seed_cuda_manual_seed_all";
  }
  return "torch_manual_seed_cpu";
}

template <typename KeyT>
inline std::string optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  std::ostringstream oss;
  oss << *value;
  return oss.str();
}

inline void append_anchor_domain_warning_token(std::string &warnings,
                                               const std::string &token) {
  if (token.empty()) {
    return;
  }
  if (!warnings.empty()) {
    warnings.push_back('|');
  }
  warnings.append(token);
}

inline void populate_anchor_domain_warning(
    int64_t accepted_anchor_count, int64_t candidate_anchor_count,
    int64_t skipped_missing_edge_coverage, int64_t skipped_failed_fetch_probe,
    double accepted_anchor_fraction, std::string &warning_level,
    std::string &warnings) {
  warning_level = "none";
  warnings.clear();
  if (accepted_anchor_count == 0) {
    warning_level = "error";
    append_anchor_domain_warning_token(warnings, "accepted_anchor_count_zero");
    return;
  }
  if (std::isfinite(accepted_anchor_fraction) &&
      accepted_anchor_fraction < 0.80) {
    warning_level = "warning";
    append_anchor_domain_warning_token(warnings,
                                       "accepted_anchor_fraction_below_0.80");
  }
  if (skipped_failed_fetch_probe > 0) {
    warning_level = "warning";
    append_anchor_domain_warning_token(warnings,
                                       "skipped_failed_fetch_probe_nonzero");
  }
  const double missing_edge_fraction =
      candidate_anchor_count == 0
          ? 0.0
          : static_cast<double>(skipped_missing_edge_coverage) /
                static_cast<double>(candidate_anchor_count);
  if (missing_edge_fraction > 0.10) {
    warning_level = "warning";
    append_anchor_domain_warning_token(
        warnings, "skipped_missing_edge_coverage_fraction_above_0.10");
  }
}

template <typename KeyT>
inline void
move_mdn_input_batch_to_device(cuwacunu::wikimyei::inference::expected_value::
                                   mdn::stream::mdn_input_batch_t<KeyT> &batch,
                               torch::Dtype dtype,
                               const torch::Device &device) {
  const auto value_options = torch::TensorOptions().dtype(dtype).device(device);
  const auto bool_options =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  const auto index_options =
      torch::TensorOptions().dtype(torch::kInt64).device(device);

  if (batch.context.defined()) {
    batch.context = batch.context.to(value_options);
  }
  if (batch.context_mask.defined()) {
    batch.context_mask = batch.context_mask.to(bool_options);
  }
  if (batch.future.defined()) {
    batch.future = batch.future.to(value_options);
  }
  if (batch.future_mask.defined()) {
    batch.future_mask = batch.future_mask.to(bool_options);
  }
  if (batch.node_index.defined()) {
    batch.node_index = batch.node_index.to(index_options);
  }
  if (batch.anchor_index.defined()) {
    batch.anchor_index = batch.anchor_index.to(index_options);
  }
  if (batch.anchor_keys.defined()) {
    batch.anchor_keys = batch.anchor_keys.to(index_options);
  }
}

inline void ensure_parent_directory(const std::filesystem::path &path,
                                    const char *path_kind) {
  const auto parent = path.parent_path();
  if (parent.empty()) {
    return;
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    throw std::runtime_error(
        std::string("[graph_first_inference_launcher] failed to create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void
write_report_file(const std::filesystem::path &path,
                  const graph_first_inference_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[graph_first_inference_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[graph_first_inference_launcher] failed to open report path");
  }
  out << report.to_text();
}

inline void write_text_file(const std::filesystem::path &path,
                            const std::string &payload, const char *path_kind) {
  if (path.empty() || payload.empty()) {
    return;
  }
  ensure_parent_directory(path, path_kind);
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        std::string("[graph_first_inference_launcher] failed to open ") +
        path_kind + " path");
  }
  out << payload;
}

[[nodiscard]] inline std::filesystem::path
runtime_lls_wave_batch_path(const std::filesystem::path &report_path,
                            const std::string &component_suffix) {
  return std::filesystem::path(report_path.string() + "." + component_suffix +
                               ".lls");
}

inline void write_component_runtime_lls_sidecars(
    const std::filesystem::path &report_path,
    const std::string &nodelift_runtime_lls,
    const std::string &representation_runtime_lls,
    const std::string &mdn_runtime_lls) {
  write_text_file(runtime_lls_wave_batch_path(report_path, "nodelift"),
                  nodelift_runtime_lls, "nodelift runtime lls");
  write_text_file(runtime_lls_wave_batch_path(report_path, "representation"),
                  representation_runtime_lls, "representation runtime lls");
  write_text_file(runtime_lls_wave_batch_path(report_path, "mdn"),
                  mdn_runtime_lls, "mdn runtime lls");
}

inline torch::Tensor
int64_tensor_from_vector(const std::vector<int64_t> &values) {
  const auto opts = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, opts);
  }
  return torch::tensor(values, opts);
}

inline std::pair<std::vector<int64_t>, std::vector<int64_t>>
string_list_to_lengths_and_bytes(const std::vector<std::string> &values) {
  std::vector<int64_t> lengths;
  std::vector<int64_t> bytes;
  lengths.reserve(values.size());
  for (const auto &value : values) {
    lengths.push_back(static_cast<int64_t>(value.size()));
    for (const unsigned char ch : value) {
      bytes.push_back(static_cast<int64_t>(ch));
    }
  }
  return {std::move(lengths), std::move(bytes)};
}

inline std::vector<int64_t> string_to_bytes(const std::string &value) {
  std::vector<int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<int64_t>(ch));
  }
  return bytes;
}

inline std::vector<int64_t>
tensor_to_int64_vector(const torch::Tensor &tensor) {
  TORCH_CHECK(tensor.defined() && tensor.dim() == 1,
              "[graph_first_inference_launcher] expected rank-1 metadata "
              "tensor");
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::vector<int64_t> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<int64_t, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

template <typename EncoderT>
inline void load_vicreg_encoder_checkpoint_file(
    const std::filesystem::path &path, EncoderT &encoder,
    const torch::Device &device,
    const std::string &expected_graph_order_fingerprint,
    const std::vector<std::string> &expected_node_ids,
    int64_t expected_encoding_dim) {
  TORCH_CHECK(!path.empty(),
              "[graph_first_inference_launcher] representation checkpoint "
              "path is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[graph_first_inference_launcher] representation checkpoint "
              "does not exist: ",
              path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);

  torch::Tensor saved_encoding_dim{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/encoding_dim", saved_encoding_dim);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  TORCH_CHECK(saved_encoding_dim.defined() && saved_encoding_dim.numel() == 1 &&
                  saved_encoding_dim.item<int64_t>() == expected_encoding_dim,
              "[graph_first_inference_launcher] representation checkpoint "
              "encoding_dim does not match current VICReg config");
  TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                  string_to_bytes(expected_graph_order_fingerprint),
              "[graph_first_inference_launcher] representation checkpoint "
              "graph fingerprint does not match current graph");

  auto [expected_node_lengths, expected_node_bytes] =
      string_list_to_lengths_and_bytes(expected_node_ids);
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                  expected_node_lengths,
              "[graph_first_inference_launcher] representation checkpoint "
              "node_id lengths do not match current node order");
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                  expected_node_bytes,
              "[graph_first_inference_launcher] representation checkpoint "
              "node_ids do not match current node order");

  torch::serialize::InputArchive encoder_archive;
  root.read("encoder", encoder_archive);
  encoder->load(encoder_archive);
}

template <typename EncoderT>
inline void freeze_vicreg_encoder(EncoderT &encoder) {
  encoder->eval();
  for (auto &parameter : encoder->parameters()) {
    parameter.set_requires_grad(false);
  }
}

inline mdn_checkpoint_identity_t checkpoint_identity_from_report(
    const graph_first_inference_training_report_t &report) {
  mdn_checkpoint_identity_t out{};
  out.graph_order_fingerprint = report.graph_order_fingerprint;
  out.node_ids = report.node_ids;
  out.target_coords = report.target_coords;
  out.context_dim = report.context_dim;
  out.mixture_count = report.mixture_count;
  return out;
}

template <typename KeyT>
inline std::string make_mdn_runtime_lls(
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        mdn_input_batch_t<KeyT> &batch,
    const mdn_train_report_t &step_report, const std::string &component_id,
    const std::string &assembly_token, const std::string &dock_binding_token,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode,
    int64_t optimizer_steps, int64_t wave_pulse_index) {
  namespace lls = cuwacunu::kikijyeba::lattice::runtime_report;
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.inference.expected_value.mdn",
              .component_id = component_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = batch.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, batch.cursor),
          runtime_report_mode, std::numeric_limits<double>::quiet_NaN(),
          static_cast<std::uint64_t>(batch.context.numel()),
          static_cast<std::uint64_t>(step_report.valid_target_count),
          static_cast<std::uint64_t>(step_report.skipped_node_head_count));
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.inference.expected_value.mdn.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, batch.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "optimizer_steps", static_cast<std::uint64_t>(optimizer_steps)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "wave_pulse_index", static_cast<std::uint64_t>(wave_pulse_index)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "row_count", static_cast<std::uint64_t>(step_report.row_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(step_report.node_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_row_count",
      static_cast<std::uint64_t>(step_report.valid_row_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_target_count",
      static_cast<std::uint64_t>(step_report.valid_target_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "skipped_node_head_count",
      static_cast<std::uint64_t>(step_report.skipped_node_head_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "active_node_head_count",
      static_cast<std::uint64_t>(step_report.active_node_head_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "trained_node_head_count",
      static_cast<std::uint64_t>(step_report.trained_node_head_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "evaluated_node_head_count",
      static_cast<std::uint64_t>(step_report.evaluated_node_head_count)));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "trained", step_report.trained));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "skipped_empty", step_report.skipped_empty));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "optimizer_step_applied", step_report.optimizer_step_applied));
  if (std::isfinite(step_report.loss)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "loss", step_report.loss, "(-inf,+inf)"));
    document.entries.push_back(lls::make_component_runtime_lls_string_entry(
        "loss_preference", "lower_is_better"));
  }
  if (std::isfinite(step_report.valid_node_target_fraction)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "valid_node_target_fraction", step_report.valid_node_target_fraction,
        "[0,1]"));
  }
  if (std::isfinite(step_report.node_context_norm_mean)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "node_context_norm_mean", step_report.node_context_norm_mean,
        "[0,+inf)"));
  }
  if (std::isfinite(step_report.mixture_entropy_mean)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "mixture_entropy_mean", step_report.mixture_entropy_mean, "[0,+inf)"));
  }
  if (std::isfinite(step_report.sigma_mean)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "sigma_mean", step_report.sigma_mean, "[0,+inf)"));
  }
  return lls::emit_component_runtime_lls_canonical(document);
}

inline void save_mdn_checkpoint_file(
    const std::filesystem::path &path,
    const graph_first_inference_training_report_t &report,
    const std::vector<
        cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel> &heads,
    torch::optim::Optimizer &optimizer) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  TORCH_CHECK(!heads.empty(),
              "[graph_first_inference_launcher] cannot checkpoint empty MDN "
              "head set");

  torch::serialize::OutputArchive root;
  for (std::size_t i = 0; i < heads.size(); ++i) {
    torch::serialize::OutputArchive head_archive;
    heads[i]->save(head_archive);
    root.write("node_head_" + std::to_string(i), head_archive);
  }

  torch::serialize::OutputArchive optimizer_archive;
  optimizer.save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/node_count",
             torch::tensor({static_cast<int64_t>(heads.size())}, i64));
  root.write("meta/context_dim", torch::tensor({report.context_dim}, i64));
  root.write("meta/mixture_count", torch::tensor({report.mixture_count}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write("meta/target_coords",
             int64_tensor_from_vector(report.target_coords));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

inline void load_mdn_checkpoint_file(
    const std::filesystem::path &path,
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        &heads,
    torch::optim::Optimizer *optimizer = nullptr,
    const mdn_checkpoint_identity_t *expected_identity = nullptr) {
  TORCH_CHECK(!path.empty(),
              "[graph_first_inference_launcher] checkpoint path is empty");
  TORCH_CHECK(!heads.empty(),
              "[graph_first_inference_launcher] cannot load into empty MDN "
              "head set");

  torch::serialize::InputArchive root;
  root.load_from(path.string(), torch::Device(torch::kCPU));

  torch::Tensor saved_node_count{};
  root.read("meta/node_count", saved_node_count);
  TORCH_CHECK(saved_node_count.defined() && saved_node_count.numel() == 1,
              "[graph_first_inference_launcher] checkpoint missing node_count "
              "metadata");
  TORCH_CHECK(saved_node_count.item<int64_t>() ==
                  static_cast<int64_t>(heads.size()),
              "[graph_first_inference_launcher] checkpoint node_count does "
              "not match current node head count");

  torch::Tensor saved_context_dim{};
  torch::Tensor saved_mixture_count{};
  torch::Tensor saved_target_coords{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/context_dim", saved_context_dim);
  root.read("meta/mixture_count", saved_mixture_count);
  root.read("meta/target_coords", saved_target_coords);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  if (expected_identity != nullptr) {
    TORCH_CHECK(saved_context_dim.defined() && saved_context_dim.numel() == 1 &&
                    saved_context_dim.item<int64_t>() ==
                        expected_identity->context_dim,
                "[graph_first_inference_launcher] checkpoint context_dim does "
                "not match current MDN context");
    TORCH_CHECK(saved_mixture_count.defined() &&
                    saved_mixture_count.numel() == 1 &&
                    saved_mixture_count.item<int64_t>() ==
                        expected_identity->mixture_count,
                "[graph_first_inference_launcher] checkpoint mixture_count "
                "does not match current MDN config");

    const auto expected_target_coords = expected_identity->target_coords;
    TORCH_CHECK(tensor_to_int64_vector(saved_target_coords) ==
                    expected_target_coords,
                "[graph_first_inference_launcher] checkpoint target_coords do "
                "not match current MDN target policy");

    TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                    string_to_bytes(expected_identity->graph_order_fingerprint),
                "[graph_first_inference_launcher] checkpoint graph fingerprint "
                "does not match current graph");

    auto [expected_node_lengths, expected_node_bytes] =
        string_list_to_lengths_and_bytes(expected_identity->node_ids);
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                    expected_node_lengths,
                "[graph_first_inference_launcher] checkpoint node_id lengths "
                "do not match current node order");
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                    expected_node_bytes,
                "[graph_first_inference_launcher] checkpoint node_ids do not "
                "match current node order");
  }

  for (std::size_t i = 0; i < heads.size(); ++i) {
    torch::serialize::InputArchive head_archive;
    root.read("node_head_" + std::to_string(i), head_archive);
    heads[i]->load(head_archive);
  }

  if (optimizer != nullptr) {
    torch::serialize::InputArchive optimizer_archive;
    root.read("optimizer", optimizer_archive);
    optimizer->load(optimizer_archive);
  }
}

} // namespace graph_first_inference_launcher_detail

template <typename DatatypeT> class graph_first_inference_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_t<DatatypeT>;

  graph_first_inference_launcher_t(
      builder_t builder, graph_first_inference_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] graph_first_inference_training_report_t dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    graph_first_inference_training_report_t out{};
    out.training_id = builder_.bundle().mdn_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.target_coords = plan.target_coords;
    out.context_dim = plan.context_dim;
    out.head_policy = plan.head_policy;
    out.target_domain = plan.target_domain;
    out.activity_target_semantics = plan.activity_target_semantics;
    out.mixture_count = plan.mixture_count;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = plan.mdn_seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.representation_checkpoint_path =
        builder_.bundle().mdn_training.input_representation_checkpoint_path;
    out.representation_checkpoint_loaded = false;
    out.mdn_checkpoint_path =
        builder_.bundle().mdn_training.input_mdn_checkpoint_path;
    out.mdn_checkpoint_loaded = false;
    out.allow_untrained_representation =
        builder_.bundle().mdn_training.allow_untrained_representation;
    out.checkpoint_every = plan.mdn_checkpoint_every;
    out.report_every = plan.mdn_report_every;
    out.validation_every = plan.mdn_validation_every;
    out.analytics_status = plan.analytics_status;
    out.wave_id = plan.wave_id;
    out.wave_mode = plan.wave_mode;
    out.target_action = effective_train_target() ? "train" : "run";
    out.wave_source_cursor_kind = plan.wave_source_cursor_kind;
    out.wave_source_cursor_scope = plan.wave_source_cursor_scope;
    out.wave_source_range_policy = plan.wave_source_range_policy;
    out.requested_anchor_index_begin = plan.requested_anchor_index_begin;
    out.requested_anchor_index_end = plan.requested_anchor_index_end;
    out.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(builder_.bundle().wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] graph_first_inference_training_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const bool train_target = effective_train_target();

    const auto checkpoint_every =
        builder_.bundle().mdn_training.checkpoint_every;
    const auto report_every = builder_.bundle().mdn_training.report_every;
    if ((options_.write_report || (train_target && checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error(
          "[graph_first_inference_launcher] report_path is required for "
          "configured report or checkpoint output");
    }

    const auto seed_scope =
        graph_first_inference_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(builder_.bundle().mdn_training.seed),
            builder_.options().device);

    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            builder_.bundle().wave_settings, options_.runtime_report_mode);

    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto srl_graph = builder_.srl_graph();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    auto encoder = builder_.make_vicreg_encoder();
    const auto &mdn_training_spec = builder_.bundle().mdn_training;
    bool representation_checkpoint_loaded = false;
    if (!mdn_training_spec.input_representation_checkpoint_path.empty()) {
      graph_first_inference_launcher_detail::
          load_vicreg_encoder_checkpoint_file(
              mdn_training_spec.input_representation_checkpoint_path, encoder,
              builder_.options().device,
              builder_.bundle()
                  .source_plan.market_graph.computed_graph_order_fingerprint(),
              builder_.bundle().source_plan.market_graph.node_ids,
              builder_.bundle().vicreg.encoding_dim);
      representation_checkpoint_loaded = true;
    } else if (!mdn_training_spec.allow_untrained_representation) {
      throw std::runtime_error(
          "[graph_first_inference_launcher] MDN ExpectedValue training "
          "requires a VICReg representation checkpoint unless "
          "ALLOW_UNTRAINED_REPRESENTATION is true for a smoke run");
    }
    graph_first_inference_launcher_detail::freeze_vicreg_encoder(encoder);
    auto representation_stream = builder_.make_node_representation_stream(
        std::move(lifted_stream), encoder, runtime_report_mode);

    graph_first_inference_training_report_t report = dry_run_report();
    report.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.seed_scope = seed_scope;
    report.representation_checkpoint_path =
        mdn_training_spec.input_representation_checkpoint_path;
    report.representation_checkpoint_loaded = representation_checkpoint_loaded;
    report.mdn_checkpoint_path = mdn_training_spec.input_mdn_checkpoint_path;
    report.mdn_checkpoint_loaded = false;
    report.allow_untrained_representation =
        mdn_training_spec.allow_untrained_representation;
    report.all_target_masks_forced_empty = force_empty_targets_for_test_;
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);
    report.source_candidate_anchor_count =
        static_cast<int64_t>(source_cursor_report.candidate_anchor_count);
    report.source_skipped_anchor_count =
        static_cast<int64_t>(source_cursor_report.skipped_anchor_count());
    report.source_skipped_outside_common_range =
        static_cast<int64_t>(source_cursor_report.skipped_outside_common_range);
    report.source_skipped_missing_edge_coverage = static_cast<int64_t>(
        source_cursor_report.skipped_missing_edge_coverage);
    report.source_skipped_failed_fetch_probe =
        static_cast<int64_t>(source_cursor_report.skipped_failed_fetch_probe);
    report.source_duplicate_anchor_count =
        static_cast<int64_t>(source_cursor_report.duplicate_anchor_count);
    report.source_accepted_anchor_fraction =
        source_cursor_report.candidate_anchor_count == 0
            ? std::numeric_limits<double>::quiet_NaN()
            : static_cast<double>(source_cursor_report.accepted_anchor_count) /
                  static_cast<double>(
                      source_cursor_report.candidate_anchor_count);
    graph_first_inference_launcher_detail::populate_anchor_domain_warning(
        report.source_anchor_count, report.source_candidate_anchor_count,
        report.source_skipped_missing_edge_coverage,
        report.source_skipped_failed_fetch_probe,
        report.source_accepted_anchor_fraction,
        report.source_anchor_domain_warning_level,
        report.source_anchor_domain_warnings);
    report.source_common_left_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.common_left_key);
    report.source_common_right_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.common_right_key);
    report.source_reference_left_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.reference_left_key);
    report.source_reference_right_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.reference_right_key);
    report.source_first_anchor_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.first_anchor_key());
    report.source_last_anchor_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.last_anchor_key());
    const auto training_options =
        cuwacunu::jkimyei::training::mdn_training_options_from_specs(
            builder_.bundle().mdn_training, builder_.bundle().mdn);

    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        heads;
    std::unique_ptr<torch::optim::Adam> optimizer;

    double loss_sum = 0.0;
    int64_t loss_count = 0;
    double node_mask_sum = 0.0;
    double future_mask_sum = 0.0;
    double residual_energy_sum = 0.0;
    int64_t diagnostics_count = 0;
    double node_context_norm_mean_sum = 0.0;
    int64_t node_context_norm_mean_count = 0;
    double node_context_norm_max = -std::numeric_limits<double>::infinity();
    double mixture_entropy_mean_sum = 0.0;
    int64_t mixture_entropy_mean_count = 0;
    double mixture_entropy_min = std::numeric_limits<double>::infinity();
    double sigma_mean_sum = 0.0;
    int64_t sigma_mean_count = 0;
    double sigma_min = std::numeric_limits<double>::infinity();
    double sigma_max = -std::numeric_limits<double>::infinity();
    double valid_node_target_fraction_sum = 0.0;
    int64_t valid_node_target_fraction_count = 0;
    std::vector<double> nll_by_node_sum;
    std::vector<int64_t> nll_by_node_count;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
      }
      if (diagnostics_count > 0) {
        report.mean_node_mask_coverage =
            node_mask_sum / static_cast<double>(diagnostics_count);
        report.mean_future_mask_coverage =
            future_mask_sum / static_cast<double>(diagnostics_count);
        report.mean_price_residual_energy =
            residual_energy_sum / static_cast<double>(diagnostics_count);
      }
      if (valid_node_target_fraction_count > 0) {
        report.mean_valid_node_target_fraction =
            valid_node_target_fraction_sum /
            static_cast<double>(valid_node_target_fraction_count);
      }
      if (node_context_norm_mean_count > 0) {
        report.mean_node_context_norm_mean =
            node_context_norm_mean_sum /
            static_cast<double>(node_context_norm_mean_count);
      }
      if (std::isfinite(node_context_norm_max)) {
        report.max_node_context_norm_max = node_context_norm_max;
      }
      if (mixture_entropy_mean_count > 0) {
        report.mean_mixture_entropy_mean =
            mixture_entropy_mean_sum /
            static_cast<double>(mixture_entropy_mean_count);
      }
      if (std::isfinite(mixture_entropy_min)) {
        report.min_mixture_entropy_min = mixture_entropy_min;
      }
      if (sigma_mean_count > 0) {
        report.mean_sigma_mean =
            sigma_mean_sum / static_cast<double>(sigma_mean_count);
      }
      if (std::isfinite(sigma_min)) {
        report.min_sigma_min = sigma_min;
      }
      if (std::isfinite(sigma_max)) {
        report.max_sigma_max = sigma_max;
      }
      if (!nll_by_node_sum.empty()) {
        report.mean_nll_by_node.assign(
            nll_by_node_sum.size(), std::numeric_limits<double>::quiet_NaN());
        for (std::size_t i = 0; i < nll_by_node_sum.size(); ++i) {
          if (nll_by_node_count[i] > 0) {
            report.mean_nll_by_node[i] =
                nll_by_node_sum[i] / static_cast<double>(nll_by_node_count[i]);
          }
        }
      }
    };

    auto write_checkpoint = [&]() {
      if (heads.empty() || !optimizer) {
        return;
      }
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".mdn.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_mdn_v1";
      graph_first_inference_launcher_detail::save_mdn_checkpoint_file(
          checkpoint_path, report, heads, *optimizer);
    };

    auto write_report = [&]() {
      if (!options_.write_report) {
        return;
      }
      refresh_running_report();
      report.report_written = true;
      ++report.report_write_count;
      report.last_report_attempted_step = report.steps_attempted;
      report.report_path = options_.report_path.string();
      graph_first_inference_launcher_detail::write_report_file(
          options_.report_path, report);
      if (report.runtime_lls_emitted) {
        graph_first_inference_launcher_detail::
            write_component_runtime_lls_sidecars(
                options_.report_path, report.nodelift_runtime_lls,
                report.representation_runtime_lls, report.mdn_runtime_lls);
      }
    };

    const int64_t max_steps = builder_.bundle().mdn_training.max_steps;
    while (representation_stream.has_next() &&
           (max_steps == 0 || report.steps_attempted < max_steps)) {
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto node_batch = representation_stream.next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(node_batch.cursor.anchor_count());
      if (report.wave_first_anchor_key.empty()) {
        report.wave_first_anchor_key =
            graph_first_inference_launcher_detail::optional_key_to_string(
                node_batch.cursor.first_anchor_key());
      }
      report.wave_last_anchor_key =
          graph_first_inference_launcher_detail::optional_key_to_string(
              node_batch.cursor.last_anchor_key());
      auto mdn_batch =
          cuwacunu::wikimyei::inference::expected_value::mdn::stream::
              make_mdn_input_batch(node_batch, builder_.mdn_adapter_options());
      graph_first_inference_launcher_detail::move_mdn_input_batch_to_device(
          mdn_batch, builder_.options().dtype, builder_.options().device);
      if (force_empty_targets_for_test_) {
        mdn_batch.future_mask.fill_(false);
      }

      if (heads.empty()) {
        heads = builder_.make_mdn_heads(
            /*context_dim=*/mdn_batch.context.size(1),
            /*channel_count=*/mdn_batch.future.size(1),
            /*horizon_count=*/mdn_batch.future.size(2));
        if (train_target) {
          auto params = builder_t::collect_mdn_head_parameters(heads);
          optimizer = std::make_unique<torch::optim::Adam>(
              params, torch::optim::AdamOptions(
                          builder_.bundle().mdn_training.learning_rate));
        }
        if (!mdn_training_spec.input_mdn_checkpoint_path.empty()) {
          const auto expected_identity = graph_first_inference_launcher_detail::
              checkpoint_identity_from_report(report);
          graph_first_inference_launcher_detail::load_mdn_checkpoint_file(
              mdn_training_spec.input_mdn_checkpoint_path, heads,
              train_target ? optimizer.get() : nullptr, &expected_identity);
          report.mdn_checkpoint_loaded = true;
        }
      }

      auto step_report =
          train_target
              ? train_mdn_batch(mdn_batch, heads, *optimizer, training_options)
              : evaluate_mdn_batch(mdn_batch, heads, training_options);
      const bool pulse_completed =
          train_target ? step_report.trained : !step_report.skipped_empty;
      if (pulse_completed) {
        ++report.steps_completed;
        ++report.wave_pulses_completed;
        if (step_report.optimizer_step_applied) {
          ++report.optimizer_steps;
        }
        report.last_loss = step_report.loss;
        if (std::isfinite(step_report.loss)) {
          loss_sum += step_report.loss;
          ++loss_count;
        }
      } else if (step_report.skipped_empty) {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }
      report.last_valid_target_count = step_report.valid_target_count;
      report.total_valid_target_count += step_report.valid_target_count;
      report.last_valid_row_count = step_report.valid_row_count;
      report.total_valid_row_count += step_report.valid_row_count;
      report.last_skipped_node_head_count = step_report.skipped_node_head_count;
      report.total_skipped_node_head_count +=
          step_report.skipped_node_head_count;
      report.last_active_node_head_count = step_report.active_node_head_count;
      report.total_active_node_head_count += step_report.active_node_head_count;
      report.last_trained_node_head_count = step_report.trained_node_head_count;
      report.total_trained_node_head_count +=
          step_report.trained_node_head_count;
      report.last_evaluated_node_head_count =
          step_report.evaluated_node_head_count;
      report.total_evaluated_node_head_count +=
          step_report.evaluated_node_head_count;
      report.last_valid_node_target_fraction =
          step_report.valid_node_target_fraction;
      if (std::isfinite(step_report.valid_node_target_fraction)) {
        valid_node_target_fraction_sum +=
            step_report.valid_node_target_fraction;
        ++valid_node_target_fraction_count;
      }
      report.low_target_coverage_warning =
          report.low_target_coverage_warning ||
          step_report.low_target_coverage_warning;
      auto ensure_i64_node_counts = [&](std::vector<int64_t> &values) {
        if (values.empty()) {
          values.assign(report.node_ids.size(), 0);
        }
      };
      auto accumulate_i64_node_counts = [&](std::vector<int64_t> &dst,
                                            const std::vector<int64_t> &src) {
        ensure_i64_node_counts(dst);
        for (std::size_t i = 0; i < src.size(); ++i) {
          if (i < dst.size()) {
            dst[i] += src[i];
          }
        }
      };
      accumulate_i64_node_counts(report.routed_row_count_by_node,
                                 step_report.routed_row_count_by_node);
      accumulate_i64_node_counts(report.active_row_count_by_node,
                                 step_report.active_row_count_by_node);
      accumulate_i64_node_counts(report.trained_row_count_by_node,
                                 step_report.trained_row_count_by_node);
      if (step_report.evaluated_node_head_count > 0) {
        accumulate_i64_node_counts(report.evaluated_row_count_by_node,
                                   step_report.active_row_count_by_node);
      } else {
        ensure_i64_node_counts(report.evaluated_row_count_by_node);
      }
      if (report.valid_target_count_by_node.empty()) {
        report.valid_target_count_by_node.assign(report.node_ids.size(), 0);
      }
      for (std::size_t i = 0; i < step_report.valid_target_count_by_node.size();
           ++i) {
        if (i < report.valid_target_count_by_node.size()) {
          report.valid_target_count_by_node[i] +=
              step_report.valid_target_count_by_node[i];
        }
      }
      if (nll_by_node_sum.empty()) {
        nll_by_node_sum.assign(report.node_ids.size(), 0.0);
        nll_by_node_count.assign(report.node_ids.size(), 0);
      }
      for (std::size_t i = 0; i < step_report.nll_by_node.size(); ++i) {
        if (i < nll_by_node_sum.size() &&
            std::isfinite(step_report.nll_by_node[i])) {
          nll_by_node_sum[i] += step_report.nll_by_node[i];
          ++nll_by_node_count[i];
        }
      }
      if (std::isfinite(step_report.node_context_norm_mean)) {
        node_context_norm_mean_sum += step_report.node_context_norm_mean;
        ++node_context_norm_mean_count;
      }
      if (std::isfinite(step_report.node_context_norm_max)) {
        node_context_norm_max =
            std::max(node_context_norm_max, step_report.node_context_norm_max);
      }
      if (std::isfinite(step_report.mixture_entropy_mean)) {
        mixture_entropy_mean_sum += step_report.mixture_entropy_mean;
        ++mixture_entropy_mean_count;
      }
      if (std::isfinite(step_report.mixture_entropy_min)) {
        mixture_entropy_min =
            std::min(mixture_entropy_min, step_report.mixture_entropy_min);
      }
      if (std::isfinite(step_report.sigma_mean)) {
        sigma_mean_sum += step_report.sigma_mean;
        ++sigma_mean_count;
      }
      if (std::isfinite(step_report.sigma_min)) {
        sigma_min = std::min(sigma_min, step_report.sigma_min);
      }
      if (std::isfinite(step_report.sigma_max)) {
        sigma_max = std::max(sigma_max, step_report.sigma_max);
      }
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 &&
           step_report.finite_parameter_check != 0.0)
              ? 1.0
              : 0.0;
      if (cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_enabled(
              runtime_report_mode)) {
        report.nodelift_runtime_lls = mdn_batch.nodelift_runtime_lls;
        report.representation_runtime_lls =
            mdn_batch.representation_runtime_lls;
        report.mdn_runtime_lls =
            graph_first_inference_launcher_detail::make_mdn_runtime_lls(
                mdn_batch, step_report, builder_.bundle().mdn.component_id,
                cuwacunu::wikimyei::assembly::make_assembly_token(
                    builder_.bundle().mdn_assembly.family,
                    builder_.bundle().mdn_assembly.component_id,
                    builder_.bundle().mdn_assembly.version_token),
                cuwacunu::kikijyeba::topology::dock_binding_token(
                    builder_.bundle().dock_binding),
                cuwacunu::kikijyeba::protocol::
                    component_stream_wave_from_settings(
                        builder_.bundle().wave_settings),
                runtime_report_mode, report.optimizer_steps,
                report.wave_pulses_attempted);
        report.runtime_lls_emitted = true;
      }

      const double node_mask_fraction =
          graph_first_inference_launcher_detail::bool_fraction_or_nan(
              node_batch.node_encoding_mask);
      const double future_mask_fraction =
          graph_first_inference_launcher_detail::bool_fraction_or_nan(
              mdn_batch.future_mask);
      const double residual_energy =
          graph_first_inference_launcher_detail::tensor_mean_or_nan(
              node_batch.price_residual.square().masked_fill(
                  node_batch.price_residual_mask.to(torch::kBool).logical_not(),
                  0.0));
      if (std::isfinite(node_mask_fraction)) {
        node_mask_sum += node_mask_fraction;
      }
      if (std::isfinite(future_mask_fraction)) {
        future_mask_sum += future_mask_fraction;
      }
      if (std::isfinite(residual_energy)) {
        residual_energy_sum += residual_energy;
      }
      ++diagnostics_count;

      if (train_target && step_report.optimizer_step_applied &&
          checkpoint_every > 0 &&
          report.optimizer_steps % checkpoint_every == 0) {
        write_checkpoint();
      }
      if (report_every > 0 && report.steps_attempted % report_every == 0) {
        write_report();
      }
    }

    refresh_running_report();
    if (!options_.write_report ||
        report.last_report_attempted_step != report.steps_attempted) {
      write_report();
    }

    return report;
  }

  void set_force_empty_targets_for_test(bool value) {
    force_empty_targets_for_test_ = value;
  }

private:
  [[nodiscard]] bool effective_train_target() const {
    if (!options_.train_target_from_wave) {
      return options_.train_target;
    }
    return builder_.bundle().wave_settings.action ==
           cuwacunu::kikijyeba::settings::wave_action_t::train;
  }

  void validate_batch_size_contract() const {
    const auto expected =
        static_cast<std::size_t>(builder_.bundle().mdn_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[graph_first_inference_launcher] builder batch_size must match "
          "MDN training BATCH_SIZE");
    }
  }

  builder_t builder_;
  graph_first_inference_launcher_options_t options_{};
  bool force_empty_targets_for_test_{false};
};

} // namespace cuwacunu::jkimyei::training::inference
