// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/settings/wave.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::jkimyei::training::inference {

struct channel_graph_first_inference_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  bool force_empty_targets_for_test{false};
  cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::kikijyeba::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct channel_graph_first_inference_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string component_assembly_id{};
  std::string input_representation_assembly_id{};
  std::string context_contract{"graph_order.channel_node_representation.v1"};
  std::string context_value_shape{"[B_node,C,De]"};
  std::string output_contract{
      "graph_order.channel_node_future_distribution.v1"};
  std::string output_value_shape{
      "log_pi:[B_node,C,Hf,K];mu_sigma:[B_node,C,Hf,K,Df]"};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  int64_t channel_count{0};
  int64_t context_dim{0};
  int64_t future_horizon{0};
  std::string context_mode{"channel_context_strict"};
  std::string target_domain{"channel_node_future"};
  std::string target_mask_policy{"all_target_features_valid"};
  std::string activity_target{"node_feature_support_mean"};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double eps{std::numeric_limits<double>::quiet_NaN()};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
  bool allow_untrained_representation{false};
  std::string representation_checkpoint_path{};
  bool representation_checkpoint_loaded{false};
  std::string mdn_checkpoint_path{};
  bool mdn_checkpoint_loaded{false};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  std::string wave_id{};
  std::string wave_mode{};
  std::string target_action{};
  std::string wave_source_range_policy{};
  std::string wave_source_order_policy{};
  int64_t requested_anchor_index_begin{-1};
  int64_t requested_anchor_index_end{-1};
  std::string requested_source_key_begin{};
  std::string requested_source_key_end{};
  std::string runtime_report_mode{};
  std::string stream_plan{};
  std::string source_cursor_token{};
  int64_t source_anchor_count{0};
  int64_t wave_streamed_anchor_count{0};
  std::string wave_first_anchor_key{};
  std::string wave_last_anchor_key{};

  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};
  int64_t wave_pulses_attempted{0};
  int64_t wave_pulses_completed{0};
  int64_t wave_pulses_skipped{0};
  bool all_target_masks_forced_empty{false};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  int64_t last_valid_target_count{0};
  int64_t total_valid_target_count{0};
  double last_valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  double mean_valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  double last_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double last_sigma_mean_valid{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean_valid{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min_valid{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max_valid{std::numeric_limits<double>::quiet_NaN()};
  double mean_mixture_entropy{std::numeric_limits<double>::quiet_NaN()};
  std::vector<double> mean_nll_per_channel{};
  std::vector<double> mean_nll_per_horizon{};
  std::vector<double> mean_mixture_usage{};
  int64_t nonfinite_output_count{0};
  double last_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double max_grad_norm{std::numeric_limits<double>::quiet_NaN()};
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

  static void append_double_list(std::ostringstream &oss, const char *key,
                                 const std::vector<double> &values) {
    oss << key << "=";
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << values[i];
    }
    oss << "\n";
  }

  [[nodiscard]] std::string to_text() const {
    std::ostringstream oss;
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "component_assembly_id=" << component_assembly_id << "\n";
    oss << "input_representation_assembly_id="
        << input_representation_assembly_id << "\n";
    oss << "context_contract=" << context_contract << "\n";
    oss << "context_value_shape=" << context_value_shape << "\n";
    oss << "output_contract=" << output_contract << "\n";
    oss << "output_value_shape=" << output_value_shape << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "channel_count=" << channel_count << "\n";
    oss << "context_dim=" << context_dim << "\n";
    oss << "future_horizon=" << future_horizon << "\n";
    oss << "context_mode=" << context_mode << "\n";
    oss << "target_domain=" << target_domain << "\n";
    oss << "target_mask_policy=" << target_mask_policy << "\n";
    oss << "activity_target=" << activity_target << "\n";
    oss << "mixture_count=" << mixture_count << "\n";
    oss << "hidden_width=" << hidden_width << "\n";
    oss << "residual_depth=" << residual_depth << "\n";
    oss << "sigma_min=" << sigma_min << "\n";
    oss << "sigma_max=" << sigma_max << "\n";
    oss << "eps=" << eps << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
    oss << "allow_untrained_representation="
        << (allow_untrained_representation ? "true" : "false") << "\n";
    oss << "representation_checkpoint_path=" << representation_checkpoint_path
        << "\n";
    oss << "representation_checkpoint_loaded="
        << (representation_checkpoint_loaded ? "true" : "false") << "\n";
    oss << "mdn_checkpoint_path=" << mdn_checkpoint_path << "\n";
    oss << "mdn_checkpoint_loaded="
        << (mdn_checkpoint_loaded ? "true" : "false") << "\n";
    oss << "checkpoint_every=" << checkpoint_every << "\n";
    oss << "report_every=" << report_every << "\n";
    oss << "validation_every=" << validation_every << "\n";
    oss << "wave_id=" << wave_id << "\n";
    oss << "wave_mode=" << wave_mode << "\n";
    oss << "target_action=" << target_action << "\n";
    oss << "wave_source_range_policy=" << wave_source_range_policy << "\n";
    oss << "wave_source_order_policy=" << wave_source_order_policy << "\n";
    oss << "requested_anchor_index_begin=" << requested_anchor_index_begin
        << "\n";
    oss << "requested_anchor_index_end=" << requested_anchor_index_end << "\n";
    oss << "requested_source_key_begin=" << requested_source_key_begin << "\n";
    oss << "requested_source_key_end=" << requested_source_key_end << "\n";
    oss << "runtime_report_mode=" << runtime_report_mode << "\n";
    oss << "stream_plan=" << stream_plan << "\n";
    oss << "source_cursor_token=" << source_cursor_token << "\n";
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    oss << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    oss << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    oss << "target_coords=";
    for (std::size_t i = 0; i < target_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << target_coords[i];
    }
    oss << "\nnode_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << node_ids[i];
    }
    oss << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << edge_ids[i];
    }
    oss << "\nsteps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    oss << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    oss << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    oss << "all_target_masks_forced_empty="
        << (all_target_masks_forced_empty ? "true" : "false") << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_valid_target_count=" << last_valid_target_count << "\n";
    oss << "total_valid_target_count=" << total_valid_target_count << "\n";
    oss << "last_valid_target_fraction=" << last_valid_target_fraction << "\n";
    oss << "mean_valid_target_fraction=" << mean_valid_target_fraction << "\n";
    oss << "last_sigma_mean=" << last_sigma_mean << "\n";
    oss << "mean_sigma_mean=" << mean_sigma_mean << "\n";
    oss << "min_sigma_min=" << min_sigma_min << "\n";
    oss << "max_sigma_max=" << max_sigma_max << "\n";
    oss << "last_sigma_mean_valid=" << last_sigma_mean_valid << "\n";
    oss << "mean_sigma_mean_valid=" << mean_sigma_mean_valid << "\n";
    oss << "min_sigma_min_valid=" << min_sigma_min_valid << "\n";
    oss << "max_sigma_max_valid=" << max_sigma_max_valid << "\n";
    oss << "mean_mixture_entropy=" << mean_mixture_entropy << "\n";
    append_double_list(oss, "mean_nll_per_channel", mean_nll_per_channel);
    append_double_list(oss, "mean_nll_per_horizon", mean_nll_per_horizon);
    append_double_list(oss, "mean_mixture_usage", mean_mixture_usage);
    oss << "nonfinite_output_count=" << nonfinite_output_count << "\n";
    oss << "last_grad_norm=" << last_grad_norm << "\n";
    oss << "max_grad_norm=" << max_grad_norm << "\n";
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

namespace channel_graph_first_inference_launcher_detail {

template <typename KeyT>
inline std::string optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  std::ostringstream oss;
  oss << *value;
  return oss.str();
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

[[nodiscard]] inline torch::Tensor
int64_tensor_from_vector(const std::vector<int64_t> &values) {
  const auto opts = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, opts);
  }
  return torch::tensor(values, opts);
}

[[nodiscard]] inline std::vector<int64_t>
string_to_bytes(const std::string &value) {
  std::vector<int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<int64_t>(ch));
  }
  return bytes;
}

[[nodiscard]] inline std::pair<std::vector<int64_t>, std::vector<int64_t>>
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

[[nodiscard]] inline std::vector<int64_t>
tensor_to_int64_vector(const torch::Tensor &tensor) {
  TORCH_CHECK(tensor.defined() && tensor.dim() == 1,
              "[channel_graph_first_inference_launcher] expected rank-1 "
              "metadata tensor");
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::vector<int64_t> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<int64_t, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

[[nodiscard]] inline bool metadata_string_matches(const torch::Tensor &tensor,
                                                  const std::string &expected) {
  return tensor_to_int64_vector(tensor) == string_to_bytes(expected);
}

[[nodiscard]] inline bool metadata_i64_matches(const torch::Tensor &tensor,
                                               int64_t expected) {
  return tensor.defined() && tensor.numel() == 1 &&
         tensor.to(torch::kCPU).to(torch::kInt64).template item<int64_t>() ==
             expected;
}

[[nodiscard]] inline bool metadata_bool_matches(const torch::Tensor &tensor,
                                                bool expected) {
  return metadata_i64_matches(tensor, expected ? 1 : 0);
}

[[nodiscard]] inline bool metadata_double_matches(const torch::Tensor &tensor,
                                                  double expected,
                                                  double tolerance = 1e-12) {
  if (!tensor.defined() || tensor.numel() != 1 || !std::isfinite(expected)) {
    return false;
  }
  const double actual =
      tensor.to(torch::kCPU).to(torch::kFloat64).template item<double>();
  return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}

[[nodiscard]] inline std::vector<double>
tensor_to_double_vector(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return {};
  }
  auto cpu = tensor.detach().to(torch::kCPU).to(torch::kFloat64).contiguous();
  cpu = cpu.reshape({cpu.numel()});
  std::vector<double> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<double, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

inline void accumulate_finite_vector(const torch::Tensor &tensor,
                                     std::vector<double> &sum,
                                     std::vector<int64_t> &count) {
  const auto values = tensor_to_double_vector(tensor);
  if (values.empty()) {
    return;
  }
  if (sum.size() < values.size()) {
    sum.resize(values.size(), 0.0);
    count.resize(values.size(), 0);
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (std::isfinite(values[i])) {
      sum[i] += values[i];
      ++count[i];
    }
  }
}

[[nodiscard]] inline std::vector<double>
mean_vector(const std::vector<double> &sum, const std::vector<int64_t> &count) {
  std::vector<double> out;
  out.reserve(sum.size());
  for (std::size_t i = 0; i < sum.size(); ++i) {
    out.push_back(i < count.size() && count[i] > 0
                      ? sum[i] / static_cast<double>(count[i])
                      : std::numeric_limits<double>::quiet_NaN());
  }
  return out;
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
        std::string("[channel_graph_first_inference_launcher] failed to "
                    "create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void write_report_file(
    const std::filesystem::path &path,
    const channel_graph_first_inference_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] failed to open report path");
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
        std::string(
            "[channel_graph_first_inference_launcher] failed to open ") +
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
                  representation_runtime_lls,
                  "channel representation runtime lls");
  write_text_file(runtime_lls_wave_batch_path(report_path, "mdn"),
                  mdn_runtime_lls, "channel MDN runtime lls");
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

inline void append_finite_double(
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_lls_document_t
        &document,
    std::string key, double value, std::string domain = "(-inf,+inf)") {
  if (std::isfinite(value)) {
    document.entries.push_back(
        cuwacunu::kikijyeba::lattice::runtime_report::
            make_component_runtime_lls_double_entry(std::move(key), value,
                                                    std::move(domain)));
  }
}

template <typename KeyT>
inline std::string make_channel_mdn_runtime_lls(
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        channel_mdn_input_batch_t<KeyT> &batch,
    const cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_step_result_t &step,
    const std::string &component_assembly_id, const std::string &assembly_token,
    const std::string &dock_binding_token,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode,
    int64_t optimizer_steps, int64_t wave_pulse_index) {
  namespace lls = cuwacunu::kikijyeba::lattice::runtime_report;
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family_id = "wikimyei.inference.expected_value.mdn",
              .component_assembly_id = component_assembly_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = batch.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, batch.cursor),
          runtime_report_mode, std::numeric_limits<double>::quiet_NaN(),
          static_cast<std::uint64_t>(batch.context.numel()),
          static_cast<std::uint64_t>(step.valid_target_count),
          step.skipped ? 1 : 0);
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
      "context_row_count", static_cast<std::uint64_t>(batch.context.size(0))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "channel_count", static_cast<std::uint64_t>(batch.context.size(1))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "context_dim", static_cast<std::uint64_t>(batch.context.size(2))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "future_horizon", static_cast<std::uint64_t>(batch.future.size(2))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "target_coord_count",
      static_cast<std::uint64_t>(batch.target_coords.size())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_target_count",
      static_cast<std::uint64_t>(step.valid_target_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "nonfinite_output_count",
      static_cast<std::uint64_t>(step.nonfinite_output_count)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("trained", !step.skipped));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("skipped", step.skipped));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "optimizer_step_applied", step.optimizer_step_applied));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "loss_preference", "lower_is_better"));
  if (step.loss.defined() && step.loss.numel() > 0) {
    append_finite_double(
        document, "loss",
        step.loss.detach().to(torch::kCPU).to(torch::kFloat64).item<double>());
  }
  append_finite_double(document, "valid_target_fraction",
                       step.valid_target_fraction, "[0,1]");
  append_finite_double(document, "context_mask_fraction",
                       bool_fraction_or_nan(batch.context_mask), "[0,1]");
  append_finite_double(document, "future_mask_fraction",
                       bool_fraction_or_nan(batch.future_mask), "[0,1]");
  append_finite_double(document, "sigma_mean", step.sigma_mean, "[0,+inf)");
  append_finite_double(document, "sigma_min", step.sigma_min, "[0,+inf)");
  append_finite_double(document, "sigma_max", step.sigma_max, "[0,+inf)");
  append_finite_double(document, "sigma_mean_valid", step.sigma_mean_valid,
                       "[0,+inf)");
  append_finite_double(document, "sigma_min_valid", step.sigma_min_valid,
                       "[0,+inf)");
  append_finite_double(document, "sigma_max_valid", step.sigma_max_valid,
                       "[0,+inf)");
  append_finite_double(document, "mixture_entropy", step.mixture_entropy,
                       "[0,+inf)");
  append_finite_double(document, "grad_norm", step.grad_norm, "[0,+inf)");
  return lls::emit_component_runtime_lls_canonical(document);
}

inline void move_channel_mdn_input_to_device(
    cuwacunu::wikimyei::inference::expected_value::mdn::channel_mdn_input_t
        &input,
    torch::Dtype dtype, const torch::Device &device) {
  input.context =
      input.context.to(torch::TensorOptions().dtype(dtype).device(device));
  input.context_mask = input.context_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(device));
  input.future =
      input.future.to(torch::TensorOptions().dtype(dtype).device(device));
  input.future_mask = input.future_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(device));
}

template <typename EncoderT>
inline void load_vicreg_encoder_checkpoint_file(
    const std::filesystem::path &path, EncoderT &encoder,
    const torch::Device &device,
    const std::string &expected_component_assembly_id,
    const std::string &expected_representation_contract,
    const std::string &expected_channel_axis_policy,
    const std::string &expected_graph_order_fingerprint,
    const std::vector<std::string> &expected_node_ids,
    const std::vector<int64_t> &expected_required_feature_coords,
    const std::string &expected_cell_valid_policy,
    int64_t expected_channel_count, int64_t expected_history_length,
    int64_t expected_input_width, int64_t expected_encoding_dim,
    int64_t expected_feature_hidden_dim, int64_t expected_temporal_depth,
    double expected_recency_decay, double expected_min_valid_fraction,
    bool expected_use_missingness_indicators,
    double expected_vicreg_invariance_weight,
    double expected_vicreg_variance_weight,
    double expected_vicreg_covariance_weight,
    double expected_vicreg_variance_floor, double expected_vicreg_eps,
    int64_t expected_min_valid_rows, bool expected_skip_non_finite_loss) {
  TORCH_CHECK(!path.empty(),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint path is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint does not exist: ",
              path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);

  torch::Tensor saved_channel_count{};
  torch::Tensor saved_history_length{};
  torch::Tensor saved_input_width{};
  torch::Tensor saved_encoding_dim{};
  torch::Tensor saved_feature_hidden_dim{};
  torch::Tensor saved_temporal_depth{};
  torch::Tensor saved_recency_decay{};
  torch::Tensor saved_min_valid_fraction{};
  torch::Tensor saved_use_missingness_indicators{};
  torch::Tensor saved_vicreg_invariance_weight{};
  torch::Tensor saved_vicreg_variance_weight{};
  torch::Tensor saved_vicreg_covariance_weight{};
  torch::Tensor saved_vicreg_variance_floor{};
  torch::Tensor saved_vicreg_eps{};
  torch::Tensor saved_min_valid_rows{};
  torch::Tensor saved_skip_non_finite_loss{};
  torch::Tensor saved_component_assembly_id{};
  torch::Tensor saved_representation_contract{};
  torch::Tensor saved_channel_axis_policy{};
  torch::Tensor saved_cell_valid_policy{};
  torch::Tensor saved_required_feature_coords{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/channel_count", saved_channel_count);
  root.read("meta/history_length", saved_history_length);
  root.read("meta/input_width", saved_input_width);
  root.read("meta/encoding_dim", saved_encoding_dim);
  root.read("meta/feature_hidden_dim", saved_feature_hidden_dim);
  root.read("meta/temporal_depth", saved_temporal_depth);
  root.read("meta/recency_decay", saved_recency_decay);
  root.read("meta/min_valid_fraction", saved_min_valid_fraction);
  root.read("meta/use_missingness_indicators",
            saved_use_missingness_indicators);
  root.read("meta/vicreg_invariance_weight", saved_vicreg_invariance_weight);
  root.read("meta/vicreg_variance_weight", saved_vicreg_variance_weight);
  root.read("meta/vicreg_covariance_weight", saved_vicreg_covariance_weight);
  root.read("meta/vicreg_variance_floor", saved_vicreg_variance_floor);
  root.read("meta/vicreg_eps", saved_vicreg_eps);
  root.read("meta/min_valid_rows", saved_min_valid_rows);
  root.read("meta/skip_non_finite_loss", saved_skip_non_finite_loss);
  root.read("meta/component_assembly_id_bytes", saved_component_assembly_id);
  root.read("meta/representation_contract_bytes",
            saved_representation_contract);
  root.read("meta/channel_axis_policy_bytes", saved_channel_axis_policy);
  root.read("meta/cell_valid_policy_bytes", saved_cell_valid_policy);
  root.read("meta/required_feature_coords", saved_required_feature_coords);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  TORCH_CHECK(saved_channel_count.defined() &&
                  saved_channel_count.numel() == 1 &&
                  saved_channel_count.item<int64_t>() == expected_channel_count,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint channel_count does not match current config");
  TORCH_CHECK(
      saved_history_length.defined() && saved_history_length.numel() == 1 &&
          saved_history_length.item<int64_t>() == expected_history_length,
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint history_length does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_input_width, expected_input_width),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint input_width does not match current config");
  TORCH_CHECK(saved_encoding_dim.defined() && saved_encoding_dim.numel() == 1 &&
                  saved_encoding_dim.item<int64_t>() == expected_encoding_dim,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint encoding_dim does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_feature_hidden_dim,
                                   expected_feature_hidden_dim),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint feature_hidden_dim does not match current config");
  TORCH_CHECK(
      metadata_i64_matches(saved_temporal_depth, expected_temporal_depth),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint temporal_depth does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_recency_decay, expected_recency_decay),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint recency_decay does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_min_valid_fraction,
                                      expected_min_valid_fraction),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint min_valid_fraction does not match current config");
  TORCH_CHECK(metadata_bool_matches(saved_use_missingness_indicators,
                                    expected_use_missingness_indicators),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint use_missingness_indicators does not match current "
              "config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_invariance_weight,
                              expected_vicreg_invariance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg invariance weight does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_variance_weight,
                              expected_vicreg_variance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg variance weight does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_covariance_weight,
                              expected_vicreg_covariance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg covariance weight does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_vicreg_variance_floor,
                                      expected_vicreg_variance_floor),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint VICReg variance floor does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_vicreg_eps, expected_vicreg_eps),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint VICReg eps does not match current config");
  TORCH_CHECK(
      metadata_i64_matches(saved_min_valid_rows, expected_min_valid_rows),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint min_valid_rows does not match current config");
  TORCH_CHECK(metadata_bool_matches(saved_skip_non_finite_loss,
                                    expected_skip_non_finite_loss),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint skip_non_finite_loss does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_component_assembly_id,
                                      expected_component_assembly_id),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint component_assembly_id does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_representation_contract,
                                      expected_representation_contract),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint contract does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_channel_axis_policy,
                                      expected_channel_axis_policy),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint channel_axis_policy does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_cell_valid_policy,
                                      expected_cell_valid_policy),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint cell_valid_policy does not match current config");
  TORCH_CHECK(tensor_to_int64_vector(saved_required_feature_coords) ==
                  expected_required_feature_coords,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint required_feature_coords do not match current "
              "config");
  TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                  string_to_bytes(expected_graph_order_fingerprint),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint graph fingerprint does not match current graph");

  auto [expected_node_lengths, expected_node_bytes] =
      string_list_to_lengths_and_bytes(expected_node_ids);
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                  expected_node_lengths,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint node_id lengths do not match current node order");
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                  expected_node_bytes,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint node_ids do not match current node order");

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

inline void save_channel_mdn_checkpoint_file(
    const std::filesystem::path &path,
    const channel_graph_first_inference_training_report_t &report,
    cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_model_t &model) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive model_archive;
  model.model()->save(model_archive);
  root.write("model", model_archive);
  torch::serialize::OutputArchive optimizer_archive;
  model.optimizer().save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/channel_count", torch::tensor({report.channel_count}, i64));
  root.write("meta/context_dim", torch::tensor({report.context_dim}, i64));
  root.write("meta/future_horizon",
             torch::tensor({report.future_horizon}, i64));
  root.write("meta/mixture_count", torch::tensor({report.mixture_count}, i64));
  root.write("meta/hidden_width", torch::tensor({report.hidden_width}, i64));
  root.write("meta/residual_depth",
             torch::tensor({report.residual_depth}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write(
      "meta/component_assembly_id_bytes",
      int64_tensor_from_vector(string_to_bytes(report.component_assembly_id)));
  root.write("meta/input_representation_assembly_id_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.input_representation_assembly_id)));
  root.write(
      "meta/context_contract_bytes",
      int64_tensor_from_vector(string_to_bytes(report.context_contract)));
  root.write("meta/output_contract_bytes",
             int64_tensor_from_vector(string_to_bytes(report.output_contract)));
  root.write("meta/context_mode_bytes",
             int64_tensor_from_vector(string_to_bytes(report.context_mode)));
  root.write("meta/target_domain_bytes",
             int64_tensor_from_vector(string_to_bytes(report.target_domain)));
  root.write(
      "meta/target_mask_policy_bytes",
      int64_tensor_from_vector(string_to_bytes(report.target_mask_policy)));
  root.write("meta/activity_target_bytes",
             int64_tensor_from_vector(string_to_bytes(report.activity_target)));
  root.write("meta/target_coords",
             int64_tensor_from_vector(report.target_coords));
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  root.write("meta/sigma_min", torch::tensor({report.sigma_min}, f64));
  root.write("meta/sigma_max", torch::tensor({report.sigma_max}, f64));
  root.write("meta/eps", torch::tensor({report.eps}, f64));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

struct channel_mdn_checkpoint_identity_t {
  std::string component_assembly_id{};
  std::string input_representation_assembly_id{};
  std::string context_contract{};
  std::string output_contract{};
  std::string context_mode{};
  std::string target_domain{};
  std::string target_mask_policy{};
  std::string activity_target{};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<int64_t> target_coords{};
  int64_t channel_count{0};
  int64_t context_dim{0};
  int64_t future_horizon{0};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double eps{std::numeric_limits<double>::quiet_NaN()};
};

[[nodiscard]] inline channel_mdn_checkpoint_identity_t
checkpoint_identity_from_report(
    const channel_graph_first_inference_training_report_t &report) {
  channel_mdn_checkpoint_identity_t out{};
  out.component_assembly_id = report.component_assembly_id;
  out.input_representation_assembly_id =
      report.input_representation_assembly_id;
  out.context_contract = report.context_contract;
  out.output_contract = report.output_contract;
  out.context_mode = report.context_mode;
  out.target_domain = report.target_domain;
  out.target_mask_policy = report.target_mask_policy;
  out.activity_target = report.activity_target;
  out.graph_order_fingerprint = report.graph_order_fingerprint;
  out.node_ids = report.node_ids;
  out.target_coords = report.target_coords;
  out.channel_count = report.channel_count;
  out.context_dim = report.context_dim;
  out.future_horizon = report.future_horizon;
  out.mixture_count = report.mixture_count;
  out.hidden_width = report.hidden_width;
  out.residual_depth = report.residual_depth;
  out.sigma_min = report.sigma_min;
  out.sigma_max = report.sigma_max;
  out.eps = report.eps;
  return out;
}

inline void load_channel_mdn_checkpoint_file(
    const std::filesystem::path &path,
    cuwacunu::wikimyei::inference::expected_value::mdn::ChannelContextMdn
        &model,
    torch::optim::Optimizer *optimizer = nullptr,
    const channel_mdn_checkpoint_identity_t *expected_identity = nullptr) {
  TORCH_CHECK(!path.empty(),
              "[channel_graph_first_inference_launcher] MDN checkpoint path "
              "is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[channel_graph_first_inference_launcher] MDN checkpoint does "
              "not exist: ",
              path.string());

  torch::serialize::InputArchive root;
  root.load_from(path.string(), model->device);

  torch::Tensor saved_channel_count{};
  torch::Tensor saved_context_dim{};
  torch::Tensor saved_future_horizon{};
  torch::Tensor saved_mixture_count{};
  torch::Tensor saved_hidden_width{};
  torch::Tensor saved_residual_depth{};
  torch::Tensor saved_component_assembly_id{};
  torch::Tensor saved_input_representation_assembly_id{};
  torch::Tensor saved_context_contract{};
  torch::Tensor saved_output_contract{};
  torch::Tensor saved_context_mode{};
  torch::Tensor saved_target_domain{};
  torch::Tensor saved_target_mask_policy{};
  torch::Tensor saved_activity_target{};
  torch::Tensor saved_target_coords{};
  torch::Tensor saved_sigma_min{};
  torch::Tensor saved_sigma_max{};
  torch::Tensor saved_eps{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/channel_count", saved_channel_count);
  root.read("meta/context_dim", saved_context_dim);
  root.read("meta/future_horizon", saved_future_horizon);
  root.read("meta/mixture_count", saved_mixture_count);
  root.read("meta/hidden_width", saved_hidden_width);
  root.read("meta/residual_depth", saved_residual_depth);
  root.read("meta/component_assembly_id_bytes", saved_component_assembly_id);
  root.read("meta/input_representation_assembly_id_bytes",
            saved_input_representation_assembly_id);
  root.read("meta/context_contract_bytes", saved_context_contract);
  root.read("meta/output_contract_bytes", saved_output_contract);
  root.read("meta/context_mode_bytes", saved_context_mode);
  root.read("meta/target_domain_bytes", saved_target_domain);
  root.read("meta/target_mask_policy_bytes", saved_target_mask_policy);
  root.read("meta/activity_target_bytes", saved_activity_target);
  root.read("meta/target_coords", saved_target_coords);
  root.read("meta/sigma_min", saved_sigma_min);
  root.read("meta/sigma_max", saved_sigma_max);
  root.read("meta/eps", saved_eps);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  if (expected_identity != nullptr) {
    TORCH_CHECK(saved_channel_count.defined() &&
                    saved_channel_count.numel() == 1 &&
                    saved_channel_count.item<int64_t>() ==
                        expected_identity->channel_count,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "channel_count does not match current config");
    TORCH_CHECK(saved_context_dim.defined() && saved_context_dim.numel() == 1 &&
                    saved_context_dim.item<int64_t>() ==
                        expected_identity->context_dim,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_dim does not match current representation");
    TORCH_CHECK(saved_future_horizon.defined() &&
                    saved_future_horizon.numel() == 1 &&
                    saved_future_horizon.item<int64_t>() ==
                        expected_identity->future_horizon,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "future_horizon does not match current config");
    TORCH_CHECK(saved_mixture_count.defined() &&
                    saved_mixture_count.numel() == 1 &&
                    saved_mixture_count.item<int64_t>() ==
                        expected_identity->mixture_count,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "mixture_count does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_hidden_width,
                                     expected_identity->hidden_width),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "hidden_width does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_residual_depth,
                                     expected_identity->residual_depth),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "residual_depth does not match current config");
    TORCH_CHECK(
        metadata_string_matches(saved_component_assembly_id,
                                expected_identity->component_assembly_id),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "component_assembly_id does not match current config");
    TORCH_CHECK(
        metadata_string_matches(
            saved_input_representation_assembly_id,
            expected_identity->input_representation_assembly_id),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "input_representation_assembly_id does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_context_contract,
                                        expected_identity->context_contract),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_contract does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_output_contract,
                                        expected_identity->output_contract),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "output_contract does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_context_mode,
                                        expected_identity->context_mode),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_mode does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_target_domain,
                                        expected_identity->target_domain),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_domain does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_target_mask_policy,
                                        expected_identity->target_mask_policy),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_mask_policy does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_activity_target,
                                        expected_identity->activity_target),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "activity_target does not match current config");
    TORCH_CHECK(tensor_to_int64_vector(saved_target_coords) ==
                    expected_identity->target_coords,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_coords do not match current target policy");
    TORCH_CHECK(
        metadata_double_matches(saved_sigma_min, expected_identity->sigma_min),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "sigma_min does not match current config");
    TORCH_CHECK(
        metadata_double_matches(saved_sigma_max, expected_identity->sigma_max),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "sigma_max does not match current config");
    TORCH_CHECK(metadata_double_matches(saved_eps, expected_identity->eps),
                "[channel_graph_first_inference_launcher] MDN checkpoint eps "
                "does not match current config");
    TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                    string_to_bytes(expected_identity->graph_order_fingerprint),
                "[channel_graph_first_inference_launcher] MDN checkpoint graph "
                "fingerprint does not match current graph");

    auto [expected_node_lengths, expected_node_bytes] =
        string_list_to_lengths_and_bytes(expected_identity->node_ids);
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                    expected_node_lengths,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "node_id lengths do not match current node order");
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                    expected_node_bytes,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "node_ids do not match current node order");
  }

  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);

  if (optimizer != nullptr) {
    torch::serialize::InputArchive optimizer_archive;
    root.read("optimizer", optimizer_archive);
    optimizer->load(optimizer_archive);
  }
}

} // namespace channel_graph_first_inference_launcher_detail

template <typename DatatypeT> class channel_graph_first_inference_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::channel_graph_first_pipeline_builder_t<
          DatatypeT>;

  channel_graph_first_inference_launcher_t(
      builder_t builder,
      channel_graph_first_inference_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] channel_graph_first_inference_training_report_t
  dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    channel_graph_first_inference_training_report_t out{};
    out.training_id = builder_.bundle().channel_mdn_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.component_assembly_id =
        builder_.bundle().channel_mdn.component_assembly_id;
    out.input_representation_assembly_id =
        builder_.bundle().channel_mdn.input_representation_assembly_id;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.target_coords = builder_.bundle().channel_mdn.target_coords;
    out.channel_count = builder_.bundle().channel_mdn.channel_count;
    out.context_dim = plan.context_dim;
    out.future_horizon = builder_.bundle().channel_mdn.future_horizon;
    out.mixture_count = builder_.bundle().channel_mdn.mixture_count;
    out.hidden_width = builder_.bundle().channel_mdn.hidden_width;
    out.residual_depth = builder_.bundle().channel_mdn.residual_depth;
    out.sigma_min = builder_.bundle().channel_mdn.sigma_min;
    out.sigma_max = builder_.bundle().channel_mdn.sigma_max;
    out.eps = builder_.bundle().channel_mdn.eps;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = builder_.bundle().channel_mdn_training.seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.allow_untrained_representation =
        builder_.bundle().channel_mdn_training.allow_untrained_representation;
    out.representation_checkpoint_path =
        builder_.bundle()
            .channel_mdn_training.input_representation_checkpoint_path;
    out.representation_checkpoint_loaded = false;
    out.mdn_checkpoint_path =
        builder_.bundle().channel_mdn_training.input_mdn_checkpoint_path;
    out.mdn_checkpoint_loaded = false;
    out.checkpoint_every =
        builder_.bundle().channel_mdn_training.checkpoint_every;
    out.report_every = builder_.bundle().channel_mdn_training.report_every;
    out.validation_every =
        builder_.bundle().channel_mdn_training.validation_every;
    out.wave_id = plan.wave_id;
    out.wave_mode = plan.wave_mode;
    out.target_action = effective_train_target() ? "train" : "run";
    out.wave_source_range_policy = plan.wave_source_range_policy;
    out.wave_source_order_policy = plan.wave_source_order_policy;
    out.requested_anchor_index_begin = plan.requested_anchor_index_begin;
    out.requested_anchor_index_end = plan.requested_anchor_index_end;
    out.requested_source_key_begin = plan.requested_source_key_begin;
    out.requested_source_key_end = plan.requested_source_key_end;
    out.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(builder_.bundle().wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] channel_graph_first_inference_training_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const bool train_target = effective_train_target();

    const auto &training_spec = builder_.bundle().channel_mdn_training;
    const auto checkpoint_every = training_spec.checkpoint_every;
    const auto report_every = training_spec.report_every;
    if ((options_.write_report || (train_target && checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error("[channel_graph_first_inference_launcher] "
                               "report_path is required for "
                               "configured report or checkpoint output");
    }

    const auto seed_scope =
        channel_graph_first_inference_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(training_spec.seed),
            builder_.options().device);
    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            builder_.bundle().wave_settings, options_.runtime_report_mode);
    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    auto encoder = builder_.make_vicreg_encoder();
    bool representation_checkpoint_loaded = false;
    if (!training_spec.input_representation_checkpoint_path.empty()) {
      channel_graph_first_inference_launcher_detail::
          load_vicreg_encoder_checkpoint_file(
              training_spec.input_representation_checkpoint_path, encoder,
              builder_.options().device,
              builder_.bundle().vicreg.component_assembly_id,
              "graph_order.channel_node_representation.v1",
              "preserved_primary_output",
              builder_.bundle()
                  .source_plan.market_graph.computed_graph_order_fingerprint(),
              builder_.bundle().source_plan.market_graph.node_ids,
              builder_.bundle().vicreg.required_feature_coords,
              cuwacunu::kikijyeba::protocol::
                  graph_first_pipeline_builder_detail::cell_valid_policy_name(
                      builder_.bundle().vicreg.cell_valid_policy),
              builder_.bundle().vicreg.channel_count,
              builder_.bundle().vicreg.history_length,
              builder_.bundle().vicreg.input_width,
              builder_.bundle().vicreg.encoding_dim,
              builder_.bundle().vicreg.feature_hidden_dim,
              builder_.bundle().vicreg.temporal_depth,
              builder_.bundle().vicreg.recency_decay,
              builder_.bundle().vicreg.min_valid_fraction,
              builder_.bundle().vicreg.use_missingness_indicators,
              builder_.bundle().vicreg.vicreg_invariance_weight,
              builder_.bundle().vicreg.vicreg_variance_weight,
              builder_.bundle().vicreg.vicreg_covariance_weight,
              builder_.bundle().vicreg.vicreg_variance_floor,
              builder_.bundle().vicreg.vicreg_eps,
              builder_.bundle().vicreg.min_valid_rows,
              builder_.bundle().vicreg.skip_non_finite_loss);
      representation_checkpoint_loaded = true;
    } else if (!training_spec.allow_untrained_representation) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] Channel MDN training "
          "requires a channel representation checkpoint unless "
          "ALLOW_UNTRAINED_REPRESENTATION is true for a smoke run");
    }
    if (!train_target && training_spec.input_mdn_checkpoint_path.empty()) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] Channel MDN run/eval "
          "requires INPUT_MDN_CHECKPOINT; refusing to evaluate an untrained "
          "channel-context MDN");
    }
    channel_graph_first_inference_launcher_detail::freeze_vicreg_encoder(
        encoder);
    auto representation_stream = builder_.make_channel_representation_stream(
        std::move(lifted_stream), encoder, runtime_report_mode);

    auto report = dry_run_report();
    report.seed_scope = seed_scope;
    report.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.representation_checkpoint_path =
        training_spec.input_representation_checkpoint_path;
    report.representation_checkpoint_loaded = representation_checkpoint_loaded;
    report.mdn_checkpoint_path = training_spec.input_mdn_checkpoint_path;
    report.mdn_checkpoint_loaded = false;
    report.all_target_masks_forced_empty =
        options_.force_empty_targets_for_test;
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);

    cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_model_t *model_ptr = nullptr;
    std::unique_ptr<cuwacunu::wikimyei::inference::expected_value::mdn::
                        channel_context_mdn_train_model_t>
        model_holder;

    double loss_sum = 0.0;
    int64_t loss_count = 0;
    double valid_fraction_sum = 0.0;
    int64_t valid_fraction_count = 0;
    double sigma_mean_sum = 0.0;
    int64_t sigma_mean_count = 0;
    double sigma_min = std::numeric_limits<double>::infinity();
    double sigma_max = -std::numeric_limits<double>::infinity();
    double sigma_mean_valid_sum = 0.0;
    int64_t sigma_mean_valid_count = 0;
    double sigma_min_valid = std::numeric_limits<double>::infinity();
    double sigma_max_valid = -std::numeric_limits<double>::infinity();
    double mixture_entropy_sum = 0.0;
    int64_t mixture_entropy_count = 0;
    double max_grad_norm = -std::numeric_limits<double>::infinity();
    std::vector<double> nll_per_channel_sum;
    std::vector<int64_t> nll_per_channel_count;
    std::vector<double> nll_per_horizon_sum;
    std::vector<int64_t> nll_per_horizon_count;
    std::vector<double> mixture_usage_sum;
    std::vector<int64_t> mixture_usage_count;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
      }
      if (valid_fraction_count > 0) {
        report.mean_valid_target_fraction =
            valid_fraction_sum / static_cast<double>(valid_fraction_count);
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
      if (sigma_mean_valid_count > 0) {
        report.mean_sigma_mean_valid =
            sigma_mean_valid_sum / static_cast<double>(sigma_mean_valid_count);
      }
      if (std::isfinite(sigma_min_valid)) {
        report.min_sigma_min_valid = sigma_min_valid;
      }
      if (std::isfinite(sigma_max_valid)) {
        report.max_sigma_max_valid = sigma_max_valid;
      }
      if (mixture_entropy_count > 0) {
        report.mean_mixture_entropy =
            mixture_entropy_sum / static_cast<double>(mixture_entropy_count);
      }
      if (std::isfinite(max_grad_norm)) {
        report.max_grad_norm = max_grad_norm;
      }
      report.mean_nll_per_channel =
          channel_graph_first_inference_launcher_detail::mean_vector(
              nll_per_channel_sum, nll_per_channel_count);
      report.mean_nll_per_horizon =
          channel_graph_first_inference_launcher_detail::mean_vector(
              nll_per_horizon_sum, nll_per_horizon_count);
      report.mean_mixture_usage =
          channel_graph_first_inference_launcher_detail::mean_vector(
              mixture_usage_sum, mixture_usage_count);
    };

    auto write_checkpoint = [&]() {
      if (model_ptr == nullptr) {
        return;
      }
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".channel_mdn.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_channel_mdn_v1";
      channel_graph_first_inference_launcher_detail::
          save_channel_mdn_checkpoint_file(checkpoint_path, report, *model_ptr);
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
      channel_graph_first_inference_launcher_detail::write_report_file(
          options_.report_path, report);
      if (report.runtime_lls_emitted) {
        channel_graph_first_inference_launcher_detail::
            write_component_runtime_lls_sidecars(
                options_.report_path, report.nodelift_runtime_lls,
                report.representation_runtime_lls, report.mdn_runtime_lls);
      }
    };

    const int64_t max_steps = training_spec.max_steps;
    while (max_steps == 0 || report.steps_attempted < max_steps) {
      if (!representation_stream.has_next()) {
        if (!train_target || max_steps == 0) {
          break;
        }
        representation_stream.reset();
        if (!representation_stream.has_next()) {
          break;
        }
      }
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto channel_batch = representation_stream.next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(channel_batch.cursor.anchor_count());
      if (report.wave_first_anchor_key.empty()) {
        report.wave_first_anchor_key =
            channel_graph_first_inference_launcher_detail::
                optional_key_to_string(channel_batch.cursor.first_anchor_key());
      }
      report.wave_last_anchor_key =
          channel_graph_first_inference_launcher_detail::optional_key_to_string(
              channel_batch.cursor.last_anchor_key());

      auto batch = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          make_channel_mdn_input_batch(channel_batch,
                                       builder_.channel_mdn_adapter_options());
      auto input = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          to_channel_mdn_input(batch);
      if (options_.force_empty_targets_for_test) {
        input.future_mask.fill_(false);
      }
      channel_graph_first_inference_launcher_detail::
          move_channel_mdn_input_to_device(input, builder_.options().dtype,
                                           builder_.options().device);

      if (model_ptr == nullptr) {
        auto mdn = builder_.make_channel_context_mdn(
            /*context_dim=*/input.context.size(2),
            /*channel_count=*/input.context.size(1),
            /*horizon_count=*/input.future.size(2));
        auto train_options = cuwacunu::wikimyei::inference::expected_value::
            mdn::channel_context_mdn_train_options_from_spec(
                builder_.bundle().channel_mdn);
        train_options.grad_clip_norm = training_spec.grad_clip_norm;
        model_holder =
            std::make_unique<cuwacunu::wikimyei::inference::expected_value::
                                 mdn::channel_context_mdn_train_model_t>(
                std::move(mdn), training_spec.learning_rate, train_options);
        model_ptr = model_holder.get();
        if (!training_spec.input_mdn_checkpoint_path.empty()) {
          const auto expected_identity =
              channel_graph_first_inference_launcher_detail::
                  checkpoint_identity_from_report(report);
          auto *optimizer = train_target ? &model_ptr->optimizer() : nullptr;
          channel_graph_first_inference_launcher_detail::
              load_channel_mdn_checkpoint_file(
                  training_spec.input_mdn_checkpoint_path, model_ptr->model(),
                  optimizer, &expected_identity);
          report.mdn_checkpoint_loaded = true;
        }
      }

      cuwacunu::wikimyei::inference::expected_value::mdn::
          channel_context_mdn_train_step_result_t step{};
      if (train_target) {
        step = model_ptr->train_one_batch(input);
      } else {
        const auto combined_mask = cuwacunu::wikimyei::inference::
            expected_value::mdn::combine_channel_context_and_future_mask(
                input.context_mask, input.future_mask);
        step.valid_target_count = combined_mask.sum().template item<int64_t>();
        step.valid_target_fraction =
            combined_mask.numel() == 0
                ? 0.0
                : static_cast<double>(step.valid_target_count) /
                      static_cast<double>(combined_mask.numel());
        if (step.valid_target_count == 0) {
          step.skipped = true;
          step.loss = torch::zeros({}, input.context.options());
        } else {
          auto out = model_ptr->forward(input.context, input.context_mask);
          step.nonfinite_output_count =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  channel_context_mdn_train_detail::nonfinite_count(out);
          auto nll_map =
              cuwacunu::wikimyei::inference::expected_value::mdn::mdn_nll_map(
                  out, input.future, combined_mask,
                  cuwacunu::wikimyei::inference::expected_value::mdn::
                      channel_mdn_nll_options_from_spec(
                          builder_.bundle().channel_mdn));
          step.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::
              mdn::mdn_masked_mean_per_channel(nll_map, combined_mask);
          step.nll_per_horizon = cuwacunu::wikimyei::inference::expected_value::
              mdn::mdn_masked_mean_per_horizon(nll_map, combined_mask);
          step.loss = cuwacunu::wikimyei::inference::expected_value::mdn::
              compute_channel_context_mdn_nll(
                  out, input,
                  cuwacunu::wikimyei::inference::expected_value::mdn::
                      channel_mdn_nll_options_from_spec(
                          builder_.bundle().channel_mdn));
          step.sigma_mean =
              out.sigma.mean().to(torch::kCPU).template item<double>();
          step.sigma_min =
              out.sigma.min().to(torch::kCPU).template item<double>();
          step.sigma_max =
              out.sigma.max().to(torch::kCPU).template item<double>();
          const auto sigma_valid =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  channel_context_mdn_train_detail::masked_sigma_summary(
                      out, combined_mask);
          step.sigma_mean_valid = sigma_valid.mean;
          step.sigma_min_valid = sigma_valid.min;
          step.sigma_max_valid = sigma_valid.max;
          step.mixture_entropy = cuwacunu::wikimyei::inference::expected_value::
              mdn::channel_context_mdn_train_detail::masked_mixture_entropy(
                  out, combined_mask);
          step.mixture_usage = cuwacunu::wikimyei::inference::expected_value::
              mdn::channel_context_mdn_train_detail::mixture_usage(
                  out, combined_mask);
          step.gradients_finite =
              step.nonfinite_output_count == 0 &&
              torch::isfinite(step.loss).all().template item<bool>();
        }
      }

      const bool completed =
          train_target ? step.optimizer_step_applied : !step.skipped;
      if (completed) {
        ++report.steps_completed;
        ++report.wave_pulses_completed;
        if (step.optimizer_step_applied) {
          ++report.optimizer_steps;
        }
        report.last_loss = step.loss.defined() && step.loss.numel() > 0
                               ? step.loss.detach()
                                     .to(torch::kCPU)
                                     .to(torch::kFloat64)
                                     .template item<double>()
                               : std::numeric_limits<double>::quiet_NaN();
        if (std::isfinite(report.last_loss)) {
          loss_sum += report.last_loss;
          ++loss_count;
        }
      } else if (step.skipped) {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }

      report.last_valid_target_count = step.valid_target_count;
      report.total_valid_target_count += step.valid_target_count;
      report.last_valid_target_fraction = step.valid_target_fraction;
      if (std::isfinite(step.valid_target_fraction)) {
        valid_fraction_sum += step.valid_target_fraction;
        ++valid_fraction_count;
      }
      report.last_sigma_mean = step.sigma_mean;
      if (std::isfinite(step.sigma_mean)) {
        sigma_mean_sum += step.sigma_mean;
        ++sigma_mean_count;
      }
      if (std::isfinite(step.sigma_min)) {
        sigma_min = std::min(sigma_min, step.sigma_min);
      }
      if (std::isfinite(step.sigma_max)) {
        sigma_max = std::max(sigma_max, step.sigma_max);
      }
      report.last_sigma_mean_valid = step.sigma_mean_valid;
      if (std::isfinite(step.sigma_mean_valid)) {
        sigma_mean_valid_sum += step.sigma_mean_valid;
        ++sigma_mean_valid_count;
      }
      if (std::isfinite(step.sigma_min_valid)) {
        sigma_min_valid = std::min(sigma_min_valid, step.sigma_min_valid);
      }
      if (std::isfinite(step.sigma_max_valid)) {
        sigma_max_valid = std::max(sigma_max_valid, step.sigma_max_valid);
      }
      if (std::isfinite(step.mixture_entropy)) {
        mixture_entropy_sum += step.mixture_entropy;
        ++mixture_entropy_count;
      }
      report.nonfinite_output_count += step.nonfinite_output_count;
      report.last_grad_norm = step.grad_norm;
      if (std::isfinite(step.grad_norm)) {
        max_grad_norm = std::max(max_grad_norm, step.grad_norm);
      }
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.nll_per_channel, nll_per_channel_sum, nll_per_channel_count);
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.nll_per_horizon, nll_per_horizon_sum, nll_per_horizon_count);
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.mixture_usage, mixture_usage_sum, mixture_usage_count);
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 && step.gradients_finite) ? 1.0
                                                                          : 0.0;
      if (cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_enabled(
              runtime_report_mode)) {
        report.nodelift_runtime_lls = batch.nodelift_runtime_lls;
        report.representation_runtime_lls = batch.representation_runtime_lls;
        report.mdn_runtime_lls = channel_graph_first_inference_launcher_detail::
            make_channel_mdn_runtime_lls(
                batch, step,
                builder_.bundle().channel_mdn.component_assembly_id,
                cuwacunu::wikimyei::assembly::make_assembly_token(
                    builder_.bundle().channel_mdn_assembly.family,
                    builder_.bundle()
                        .channel_mdn_assembly.component_assembly_id,
                    builder_.bundle().channel_mdn_assembly.version_token),
                cuwacunu::kikijyeba::topology::dock_binding_token(
                    builder_.bundle().dock_binding),
                cuwacunu::kikijyeba::protocol::
                    component_stream_wave_from_settings(
                        builder_.bundle().wave_settings),
                runtime_report_mode, report.optimizer_steps,
                report.wave_pulses_attempted);
        report.runtime_lls_emitted = true;
      }

      if (train_target && step.optimizer_step_applied && checkpoint_every > 0 &&
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

private:
  [[nodiscard]] bool effective_train_target() const {
    if (!options_.train_target_from_wave) {
      return options_.train_target;
    }
    return builder_.bundle().wave_settings.action ==
           cuwacunu::kikijyeba::settings::wave_action_t::train;
  }

  void validate_batch_size_contract() const {
    const auto expected = static_cast<std::size_t>(
        builder_.bundle().channel_mdn_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] builder batch_size must "
          "match Channel MDN training BATCH_SIZE");
    }
  }

  builder_t builder_;
  channel_graph_first_inference_launcher_options_t options_{};
};

} // namespace cuwacunu::jkimyei::training::inference
