// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"
#include "wikimyei/representation/encoding/vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_projector.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_train_model.h"

namespace cuwacunu::jkimyei::training::representation {

struct channel_graph_first_representation_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  std::function<void(std::string_view)> learning_probe_report_sink{};
  cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::hero::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct channel_graph_first_representation_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string component_assembly_id{};
  std::string representation_architecture{
      "channel_preserving_local_node_encoder.v1"};
  std::string representation_contract{
      "graph_order.channel_node_representation.v1"};
  std::string primary_value_shape{"[B,N,C,De]"};
  std::string sequence_value_shape{"[M,C,Hx,De]"};
  std::string channel_axis_policy{"preserved_primary_output"};
  std::string temporal_reduction{"mask_aware_anchor_weighted"};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> required_feature_coords{};
  std::string cell_valid_policy{};
  int64_t channel_count{0};
  int64_t history_length{0};
  int64_t input_width{0};
  int64_t encoding_dim{0};
  int64_t feature_hidden_dim{0};
  int64_t temporal_depth{0};
  double recency_decay{std::numeric_limits<double>::quiet_NaN()};
  double min_valid_fraction{std::numeric_limits<double>::quiet_NaN()};
  bool use_missingness_indicators{true};
  int64_t projector_dim{0};
  int64_t projector_hidden_dim{0};
  int64_t projector_depth{0};
  double vicreg_invariance_weight{std::numeric_limits<double>::quiet_NaN()};
  double vicreg_variance_weight{std::numeric_limits<double>::quiet_NaN()};
  double vicreg_covariance_weight{std::numeric_limits<double>::quiet_NaN()};
  double vicreg_variance_floor{std::numeric_limits<double>::quiet_NaN()};
  double vicreg_eps{std::numeric_limits<double>::quiet_NaN()};
  double global_aux_weight{std::numeric_limits<double>::quiet_NaN()};
  int64_t min_valid_rows{0};
  bool skip_non_finite_loss{true};
  double jitter_std{std::numeric_limits<double>::quiet_NaN()};
  double feature_dropout_prob{std::numeric_limits<double>::quiet_NaN()};
  double history_dropout_prob{std::numeric_limits<double>::quiet_NaN()};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
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

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_invariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_invariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_variance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_variance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_covariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_covariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_global_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_global_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double max_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  int64_t last_valid_projection_rows{0};
  int64_t total_valid_projection_rows{0};

  int64_t adapter_original_rows{0};
  int64_t adapter_valid_feature_count{0};
  int64_t adapter_valid_cell_any_count{0};
  int64_t adapter_valid_cell_all_count{0};
  double mean_valid_feature_fraction{std::numeric_limits<double>::quiet_NaN()};
  std::vector<int64_t> node_support_denominator{};
  std::vector<int64_t> node_valid_lifted_row_count{};
  std::vector<int64_t> node_valid_lifted_feature_count{};
  std::vector<int64_t> node_valid_lifted_cell_any_count{};
  std::vector<int64_t> node_valid_lifted_cell_all_count{};
  std::vector<int64_t> node_valid_projection_row_count{};
  std::vector<double> node_valid_lifted_feature_fraction{};
  int64_t augmented_valid_feature_count{0};
  double last_view1_valid_feature_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double last_view2_valid_feature_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double last_augmented_valid_feature_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_augmented_valid_feature_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double last_augmented_feature_retention_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_augmented_feature_retention_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};
  int64_t representation_embedding_dim{0};
  double representation_effective_rank{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_effective_rank_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_min_dimension_variance{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_max_dimension_variance{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_condition_number{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_isotropy_score{
      std::numeric_limits<double>::quiet_NaN()};
  std::vector<double> representation_effective_rank_per_channel{};
  std::vector<double> representation_min_dimension_variance_per_channel{};
  std::vector<double> representation_condition_number_per_channel{};
  std::vector<double> representation_isotropy_score_per_channel{};

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
  std::string representation_training_runtime_lls{};

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

  static void append_i64_list(std::ostringstream &oss, const char *key,
                              const std::vector<int64_t> &values) {
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
    oss << "representation_architecture=" << representation_architecture
        << "\n";
    oss << "representation_contract=" << representation_contract << "\n";
    oss << "primary_value_shape=" << primary_value_shape << "\n";
    oss << "sequence_value_shape=" << sequence_value_shape << "\n";
    oss << "channel_axis_policy=" << channel_axis_policy << "\n";
    oss << "temporal_reduction=" << temporal_reduction << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "cell_valid_policy=" << cell_valid_policy << "\n";
    oss << "channel_count=" << channel_count << "\n";
    oss << "history_length=" << history_length << "\n";
    oss << "input_width=" << input_width << "\n";
    oss << "encoding_dim=" << encoding_dim << "\n";
    oss << "feature_hidden_dim=" << feature_hidden_dim << "\n";
    oss << "temporal_depth=" << temporal_depth << "\n";
    oss << "recency_decay=" << recency_decay << "\n";
    oss << "min_valid_fraction=" << min_valid_fraction << "\n";
    oss << "use_missingness_indicators="
        << (use_missingness_indicators ? "true" : "false") << "\n";
    oss << "projector_dim=" << projector_dim << "\n";
    oss << "projector_hidden_dim=" << projector_hidden_dim << "\n";
    oss << "projector_depth=" << projector_depth << "\n";
    oss << "vicreg_invariance_weight=" << vicreg_invariance_weight << "\n";
    oss << "vicreg_variance_weight=" << vicreg_variance_weight << "\n";
    oss << "vicreg_covariance_weight=" << vicreg_covariance_weight << "\n";
    oss << "vicreg_variance_floor=" << vicreg_variance_floor << "\n";
    oss << "vicreg_eps=" << vicreg_eps << "\n";
    oss << "global_aux_weight=" << global_aux_weight << "\n";
    oss << "min_valid_rows=" << min_valid_rows << "\n";
    oss << "skip_non_finite_loss=" << (skip_non_finite_loss ? "true" : "false")
        << "\n";
    oss << "jitter_std=" << jitter_std << "\n";
    oss << "feature_dropout_prob=" << feature_dropout_prob << "\n";
    oss << "history_dropout_prob=" << history_dropout_prob << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
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
    oss << "required_feature_coords=";
    for (std::size_t i = 0; i < required_feature_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << required_feature_coords[i];
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
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_invariance_loss=" << last_invariance_loss << "\n";
    oss << "mean_invariance_loss=" << mean_invariance_loss << "\n";
    oss << "last_variance_loss=" << last_variance_loss << "\n";
    oss << "mean_variance_loss=" << mean_variance_loss << "\n";
    oss << "last_covariance_loss=" << last_covariance_loss << "\n";
    oss << "mean_covariance_loss=" << mean_covariance_loss << "\n";
    oss << "last_global_loss=" << last_global_loss << "\n";
    oss << "mean_global_loss=" << mean_global_loss << "\n";
    oss << "last_grad_norm=" << last_grad_norm << "\n";
    oss << "max_grad_norm=" << max_grad_norm << "\n";
    oss << "last_valid_projection_rows=" << last_valid_projection_rows << "\n";
    oss << "total_valid_projection_rows=" << total_valid_projection_rows
        << "\n";
    oss << "adapter_original_rows=" << adapter_original_rows << "\n";
    oss << "adapter_valid_feature_count=" << adapter_valid_feature_count
        << "\n";
    oss << "adapter_valid_cell_any_count=" << adapter_valid_cell_any_count
        << "\n";
    oss << "adapter_valid_cell_all_count=" << adapter_valid_cell_all_count
        << "\n";
    oss << "mean_valid_feature_fraction=" << mean_valid_feature_fraction
        << "\n";
    append_i64_list(oss, "node_support_denominator", node_support_denominator);
    append_i64_list(oss, "node_valid_lifted_row_count",
                    node_valid_lifted_row_count);
    append_i64_list(oss, "node_valid_lifted_feature_count",
                    node_valid_lifted_feature_count);
    append_i64_list(oss, "node_valid_lifted_cell_any_count",
                    node_valid_lifted_cell_any_count);
    append_i64_list(oss, "node_valid_lifted_cell_all_count",
                    node_valid_lifted_cell_all_count);
    append_i64_list(oss, "node_valid_projection_row_count",
                    node_valid_projection_row_count);
    append_double_list(oss, "node_valid_lifted_feature_fraction",
                       node_valid_lifted_feature_fraction);
    oss << "augmented_valid_feature_count=" << augmented_valid_feature_count
        << "\n";
    oss << "last_view1_valid_feature_fraction="
        << last_view1_valid_feature_fraction << "\n";
    oss << "last_view2_valid_feature_fraction="
        << last_view2_valid_feature_fraction << "\n";
    oss << "last_augmented_valid_feature_fraction="
        << last_augmented_valid_feature_fraction << "\n";
    oss << "mean_augmented_valid_feature_fraction="
        << mean_augmented_valid_feature_fraction << "\n";
    oss << "last_augmented_feature_retention_fraction="
        << last_augmented_feature_retention_fraction << "\n";
    oss << "mean_augmented_feature_retention_fraction="
        << mean_augmented_feature_retention_fraction << "\n";
    oss << "finite_parameter_check=" << finite_parameter_check << "\n";
    oss << "representation_embedding_dim=" << representation_embedding_dim
        << "\n";
    oss << "representation_effective_rank=" << representation_effective_rank
        << "\n";
    oss << "representation_effective_rank_fraction="
        << representation_effective_rank_fraction << "\n";
    oss << "representation_min_dimension_variance="
        << representation_min_dimension_variance << "\n";
    oss << "representation_max_dimension_variance="
        << representation_max_dimension_variance << "\n";
    oss << "representation_condition_number=" << representation_condition_number
        << "\n";
    oss << "representation_isotropy_score=" << representation_isotropy_score
        << "\n";
    append_double_list(oss, "representation_effective_rank_per_channel",
                       representation_effective_rank_per_channel);
    append_double_list(oss, "representation_min_dimension_variance_per_channel",
                       representation_min_dimension_variance_per_channel);
    append_double_list(oss, "representation_condition_number_per_channel",
                       representation_condition_number_per_channel);
    append_double_list(oss, "representation_isotropy_score_per_channel",
                       representation_isotropy_score_per_channel);
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

namespace channel_graph_first_representation_launcher_detail {

inline double scalar_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .template item<double>();
}

inline void append_finite_double(
    cuwacunu::hero::lattice::runtime_report::runtime_lls_document_t &document,
    std::string key, double value, std::string domain = "(-inf,+inf)") {
  if (std::isfinite(value)) {
    document.entries.push_back(
        cuwacunu::hero::lattice::runtime_report::
            make_component_runtime_lls_double_entry(std::move(key), value,
                                                    std::move(domain)));
  }
}

struct channel_representation_node_support_t {
  std::vector<int64_t> support_denominator{};
  std::vector<int64_t> valid_lifted_row_count{};
  std::vector<int64_t> valid_lifted_feature_count{};
  std::vector<int64_t> valid_lifted_cell_any_count{};
  std::vector<int64_t> valid_lifted_cell_all_count{};
  std::vector<int64_t> valid_projection_row_count{};
  std::vector<double> valid_lifted_feature_fraction{};
};

inline std::string join_i64_list(const std::vector<int64_t> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

inline std::string join_string_list(const std::vector<std::string> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

inline std::string join_double_list(const std::vector<double> &values) {
  std::ostringstream out;
  out.setf(std::ios::fixed);
  out << std::setprecision(12);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

inline void append_string_list_entry(
    cuwacunu::hero::lattice::runtime_report::runtime_lls_document_t &document,
    const std::string &key, const std::string &value) {
  namespace lls = cuwacunu::hero::lattice::runtime_report;
  if (!value.empty()) {
    document.entries.push_back(
        lls::make_component_runtime_lls_string_entry(key, value));
  }
}

inline void add_assign(std::vector<int64_t> &dst,
                       const std::vector<int64_t> &src) {
  if (dst.size() < src.size()) {
    dst.resize(src.size(), 0);
  }
  for (std::size_t i = 0; i < src.size(); ++i) {
    dst[i] += src[i];
  }
}

inline void
recompute_node_support_fractions(channel_representation_node_support_t &support,
                                 int64_t channel_count, int64_t history_length,
                                 int64_t input_width) {
  const std::size_t n = std::max(support.support_denominator.size(),
                                 support.valid_lifted_feature_count.size());
  support.valid_lifted_feature_fraction.assign(
      n, std::numeric_limits<double>::quiet_NaN());
  const int64_t features_per_row = channel_count * history_length * input_width;
  for (std::size_t i = 0; i < n; ++i) {
    const int64_t denominator = i < support.support_denominator.size()
                                    ? support.support_denominator[i]
                                    : 0;
    const int64_t valid = i < support.valid_lifted_feature_count.size()
                              ? support.valid_lifted_feature_count[i]
                              : 0;
    const int64_t opportunity = denominator * features_per_row;
    if (opportunity > 0) {
      support.valid_lifted_feature_fraction[i] =
          static_cast<double>(valid) / static_cast<double>(opportunity);
    }
  }
}

inline channel_representation_node_support_t
summarize_node_support(const cuwacunu::wikimyei::representation::encoding::
                           vicreg::channel_node_encoder_input_t &input) {
  channel_representation_node_support_t out{};
  if (input.N <= 0 || !input.node_index.defined() ||
      !input.feature_mask.defined() || !input.cell_mask_any.defined() ||
      !input.cell_mask_all.defined()) {
    return out;
  }

  const auto node_index =
      input.node_index.to(torch::kCPU).to(torch::kInt64).contiguous();
  const auto feature_count = input.feature_mask.to(torch::kCPU)
                                 .to(torch::kInt64)
                                 .sum({1, 2, 3})
                                 .contiguous();
  const auto cell_any_count = input.cell_mask_any.to(torch::kCPU)
                                  .to(torch::kInt64)
                                  .sum({1, 2})
                                  .contiguous();
  const auto cell_all_count = input.cell_mask_all.to(torch::kCPU)
                                  .to(torch::kInt64)
                                  .sum({1, 2})
                                  .contiguous();
  const auto row_support = input.cell_mask_any.any(/*dim=*/1)
                               .any(/*dim=*/1)
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();

  out.support_denominator.assign(static_cast<std::size_t>(input.N), 0);
  out.valid_lifted_row_count.assign(static_cast<std::size_t>(input.N), 0);
  out.valid_lifted_feature_count.assign(static_cast<std::size_t>(input.N), 0);
  out.valid_lifted_cell_any_count.assign(static_cast<std::size_t>(input.N), 0);
  out.valid_lifted_cell_all_count.assign(static_cast<std::size_t>(input.N), 0);
  out.valid_projection_row_count.assign(static_cast<std::size_t>(input.N), 0);

  const auto node_acc = node_index.accessor<int64_t, 1>();
  const auto feature_acc = feature_count.accessor<int64_t, 1>();
  const auto cell_any_acc = cell_any_count.accessor<int64_t, 1>();
  const auto cell_all_acc = cell_all_count.accessor<int64_t, 1>();
  const auto row_support_acc = row_support.accessor<int64_t, 1>();
  for (int64_t row = 0; row < node_index.size(0); ++row) {
    const auto node = node_acc[row];
    if (node < 0 || node >= input.N) {
      continue;
    }
    const auto idx = static_cast<std::size_t>(node);
    ++out.support_denominator[idx];
    out.valid_lifted_feature_count[idx] += feature_acc[row];
    out.valid_lifted_cell_any_count[idx] += cell_any_acc[row];
    out.valid_lifted_cell_all_count[idx] += cell_all_acc[row];
    out.valid_lifted_row_count[idx] += row_support_acc[row] > 0 ? 1 : 0;
    out.valid_projection_row_count[idx] += row_support_acc[row] > 0 ? 1 : 0;
  }
  recompute_node_support_fractions(out, input.data.size(1), input.data.size(2),
                                   input.data.size(3));
  return out;
}

inline void accumulate_node_support(
    channel_graph_first_representation_training_report_t &report,
    const channel_representation_node_support_t &support) {
  add_assign(report.node_support_denominator, support.support_denominator);
  add_assign(report.node_valid_lifted_row_count,
             support.valid_lifted_row_count);
  add_assign(report.node_valid_lifted_feature_count,
             support.valid_lifted_feature_count);
  add_assign(report.node_valid_lifted_cell_any_count,
             support.valid_lifted_cell_any_count);
  add_assign(report.node_valid_lifted_cell_all_count,
             support.valid_lifted_cell_all_count);
  add_assign(report.node_valid_projection_row_count,
             support.valid_projection_row_count);
  channel_representation_node_support_t cumulative{};
  cumulative.support_denominator = report.node_support_denominator;
  cumulative.valid_lifted_feature_count =
      report.node_valid_lifted_feature_count;
  recompute_node_support_fractions(cumulative, report.channel_count,
                                   report.history_length, report.input_width);
  report.node_valid_lifted_feature_fraction =
      std::move(cumulative.valid_lifted_feature_fraction);
}

struct representation_geometry_summary_t {
  int64_t embedding_dim{0};
  double effective_rank{std::numeric_limits<double>::quiet_NaN()};
  double effective_rank_fraction{std::numeric_limits<double>::quiet_NaN()};
  double min_dimension_variance{std::numeric_limits<double>::quiet_NaN()};
  double max_dimension_variance{std::numeric_limits<double>::quiet_NaN()};
  double condition_number{std::numeric_limits<double>::quiet_NaN()};
  double isotropy_score{std::numeric_limits<double>::quiet_NaN()};

  [[nodiscard]] bool has_rows() const {
    return embedding_dim > 0 && std::isfinite(effective_rank);
  }
};

[[nodiscard]] inline representation_geometry_summary_t
summarize_channel_representation_geometry(const torch::Tensor &z,
                                          const torch::Tensor &mask) {
  representation_geometry_summary_t out{};
  if (!z.defined() || !mask.defined() || z.dim() != 3 || mask.dim() != 2) {
    return out;
  }
  if (z.size(0) != mask.size(0) || z.size(1) != mask.size(1)) {
    return out;
  }
  out.embedding_dim = z.size(2);
  if (out.embedding_dim <= 0) {
    return out;
  }
  auto flat_mask = mask.to(torch::kBool).reshape({-1});
  auto rows = torch::nonzero(flat_mask).reshape({-1});
  if (rows.numel() == 0) {
    return out;
  }
  auto valid_rows = z.reshape({-1, out.embedding_dim})
                        .index_select(/*dim=*/0, rows)
                        .detach()
                        .to(torch::kCPU)
                        .to(torch::kFloat64);
  if (valid_rows.size(0) <= 1) {
    return out;
  }
  auto variances = valid_rows.var(/*dim=*/0, /*unbiased=*/false).clamp_min(0.0);
  variances = torch::where(torch::isfinite(variances), variances,
                           torch::zeros_like(variances));
  out.min_dimension_variance = variances.min().template item<double>();
  out.max_dimension_variance = variances.max().template item<double>();

  constexpr double eps = 1e-12;
  const double total_variance = variances.sum().template item<double>();
  if (total_variance <= eps) {
    out.effective_rank = 0.0;
    out.effective_rank_fraction = 0.0;
    out.condition_number = std::numeric_limits<double>::infinity();
    out.isotropy_score = 0.0;
    return out;
  }
  auto probabilities = variances / total_variance;
  auto entropy = -(probabilities * torch::log(probabilities.clamp_min(eps)))
                      .sum()
                      .template item<double>();
  out.effective_rank = std::exp(entropy);
  out.effective_rank_fraction =
      out.effective_rank / static_cast<double>(out.embedding_dim);
  out.condition_number =
      (out.max_dimension_variance + eps) / (out.min_dimension_variance + eps);
  out.isotropy_score =
      out.min_dimension_variance / (out.max_dimension_variance + eps);
  return out;
}

inline void accumulate_finite_value(std::size_t index, double value,
                                    std::vector<double> &sum,
                                    std::vector<int64_t> &count) {
  if (!std::isfinite(value)) {
    return;
  }
  if (sum.size() <= index) {
    sum.resize(index + 1, 0.0);
    count.resize(index + 1, 0);
  }
  sum[index] += value;
  ++count[index];
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

[[nodiscard]] inline std::vector<representation_geometry_summary_t>
summarize_channel_representation_geometry_by_channel(
    const torch::Tensor &z, const torch::Tensor &mask) {
  std::vector<representation_geometry_summary_t> out;
  if (!z.defined() || !mask.defined() || z.dim() != 3 || mask.dim() != 2 ||
      z.size(0) != mask.size(0) || z.size(1) != mask.size(1)) {
    return out;
  }
  out.reserve(static_cast<std::size_t>(z.size(1)));
  for (int64_t channel = 0; channel < z.size(1); ++channel) {
    out.push_back(summarize_channel_representation_geometry(
        z.select(/*dim=*/1, channel).unsqueeze(/*dim=*/1),
        mask.select(/*dim=*/1, channel).unsqueeze(/*dim=*/1)));
  }
  return out;
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
        std::string("[channel_graph_first_representation_launcher] failed to "
                    "create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void write_report_file(
    const std::filesystem::path &path,
    const channel_graph_first_representation_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_representation_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[channel_graph_first_representation_launcher] failed to open report "
        "path");
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
        std::string("[channel_graph_first_representation_launcher] failed to "
                    "open ") +
        path_kind + " path");
  }
  out << payload;
}

inline void
write_runtime_lls_sidecars(const std::filesystem::path &report_path,
                           const std::string &nodelift_runtime_lls,
                           const std::string &training_runtime_lls) {
  write_text_file(std::filesystem::path(report_path.string() + ".nodelift.lls"),
                  nodelift_runtime_lls, "nodelift runtime lls");
  write_text_file(std::filesystem::path(report_path.string() +
                                        ".representation_training.lls"),
                  training_runtime_lls,
                  "channel representation training runtime lls");
}

template <typename KeyT>
inline std::string make_channel_representation_training_runtime_lls(
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        channel_node_encoder_input_t &input,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_train_step_result_t &step,
    const std::string &component_assembly_id, const std::string &assembly_token,
    const std::string &dock_binding_token,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode,
    int64_t optimizer_steps, int64_t wave_pulse_index) {
  namespace lls = cuwacunu::hero::lattice::runtime_report;
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family_id = "wikimyei.representation.encoding.vicreg",
              .component_assembly_id = component_assembly_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = lifted.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          runtime_report_mode, std::numeric_limits<double>::quiet_NaN(),
          static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(step.valid_projection_rows),
          step.skipped ? 1 : 0);
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.representation.vicreg.training.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, lifted.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "optimizer_steps", static_cast<std::uint64_t>(optimizer_steps)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "wave_pulse_index", static_cast<std::uint64_t>(wave_pulse_index)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "anchor_count", static_cast<std::uint64_t>(input.B_anchor)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(input.N)));
  append_string_list_entry(document, "node_ids",
                           join_string_list(input.node_ids));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_feature_count",
      static_cast<std::uint64_t>(input.diagnostics.valid_feature_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_cell_any_count",
      static_cast<std::uint64_t>(input.diagnostics.valid_cell_any_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_cell_all_count",
      static_cast<std::uint64_t>(input.diagnostics.valid_cell_all_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_projection_rows",
      static_cast<std::uint64_t>(step.valid_projection_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "view1_valid_feature_count",
      static_cast<std::uint64_t>(step.view1_valid_feature_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "view2_valid_feature_count",
      static_cast<std::uint64_t>(step.view2_valid_feature_count)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("trained", !step.skipped));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "optimizer_step_applied", step.optimizer_step_applied));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "loss_preference", "lower_is_better"));
  append_finite_double(document, "valid_feature_fraction",
                       input.diagnostics.valid_feature_fraction, "[0,1]");
  append_finite_double(document, "view1_valid_feature_fraction",
                       step.view1_valid_feature_fraction, "[0,1]");
  append_finite_double(document, "view2_valid_feature_fraction",
                       step.view2_valid_feature_fraction, "[0,1]");
  append_finite_double(document, "view1_feature_retention_fraction",
                       step.view1_feature_retention_fraction, "[0,1]");
  append_finite_double(document, "view2_feature_retention_fraction",
                       step.view2_feature_retention_fraction, "[0,1]");
  const auto node_support = summarize_node_support(input);
  append_string_list_entry(document, "node_support_denominator",
                           join_i64_list(node_support.support_denominator));
  append_string_list_entry(document, "node_valid_lifted_row_count",
                           join_i64_list(node_support.valid_lifted_row_count));
  append_string_list_entry(
      document, "node_valid_lifted_feature_count",
      join_i64_list(node_support.valid_lifted_feature_count));
  append_string_list_entry(
      document, "node_valid_lifted_cell_any_count",
      join_i64_list(node_support.valid_lifted_cell_any_count));
  append_string_list_entry(
      document, "node_valid_lifted_cell_all_count",
      join_i64_list(node_support.valid_lifted_cell_all_count));
  append_string_list_entry(
      document, "node_valid_projection_row_count",
      join_i64_list(node_support.valid_projection_row_count));
  append_string_list_entry(
      document, "node_valid_lifted_feature_fraction",
      join_double_list(node_support.valid_lifted_feature_fraction));
  const auto add_loss = [&](const char *key, const torch::Tensor &value) {
    const double scalar = scalar_or_nan(value);
    append_finite_double(document, key, scalar);
  };
  add_loss("loss", step.loss);
  add_loss("invariance_loss", step.invariance_loss);
  add_loss("variance_loss", step.variance_loss);
  add_loss("covariance_loss", step.covariance_loss);
  add_loss("global_loss", step.global_loss);
  append_finite_double(document, "grad_norm", step.grad_norm, "[0,+inf)");
  return lls::emit_component_runtime_lls_canonical(document);
}

inline void save_vicreg_checkpoint_file(
    const std::filesystem::path &path,
    const channel_graph_first_representation_training_report_t &report,
    cuwacunu::wikimyei::representation::encoding::vicreg::vicreg_train_model_t
        &model) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive encoder_archive;
  model.encoder()->save(encoder_archive);
  root.write("encoder", encoder_archive);
  torch::serialize::OutputArchive projector_archive;
  model.projector()->save(projector_archive);
  root.write("projector", projector_archive);
  torch::serialize::OutputArchive optimizer_archive;
  model.optimizer().save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  root.write("meta/channel_count", torch::tensor({report.channel_count}, i64));
  root.write("meta/history_length",
             torch::tensor({report.history_length}, i64));
  root.write("meta/input_width", torch::tensor({report.input_width}, i64));
  root.write("meta/encoding_dim", torch::tensor({report.encoding_dim}, i64));
  root.write("meta/feature_hidden_dim",
             torch::tensor({report.feature_hidden_dim}, i64));
  root.write("meta/temporal_depth",
             torch::tensor({report.temporal_depth}, i64));
  root.write("meta/recency_decay", torch::tensor({report.recency_decay}, f64));
  root.write("meta/min_valid_fraction",
             torch::tensor({report.min_valid_fraction}, f64));
  root.write("meta/use_missingness_indicators",
             torch::tensor({report.use_missingness_indicators ? 1 : 0}, i64));
  root.write("meta/projector_dim", torch::tensor({report.projector_dim}, i64));
  root.write("meta/projector_hidden_dim",
             torch::tensor({report.projector_hidden_dim}, i64));
  root.write("meta/projector_depth",
             torch::tensor({report.projector_depth}, i64));
  root.write("meta/vicreg_invariance_weight",
             torch::tensor({report.vicreg_invariance_weight}, f64));
  root.write("meta/vicreg_variance_weight",
             torch::tensor({report.vicreg_variance_weight}, f64));
  root.write("meta/vicreg_covariance_weight",
             torch::tensor({report.vicreg_covariance_weight}, f64));
  root.write("meta/vicreg_variance_floor",
             torch::tensor({report.vicreg_variance_floor}, f64));
  root.write("meta/vicreg_eps", torch::tensor({report.vicreg_eps}, f64));
  root.write("meta/global_aux_weight",
             torch::tensor({report.global_aux_weight}, f64));
  root.write("meta/min_valid_rows",
             torch::tensor({report.min_valid_rows}, i64));
  root.write("meta/skip_non_finite_loss",
             torch::tensor({report.skip_non_finite_loss ? 1 : 0}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write(
      "meta/component_assembly_id_bytes",
      int64_tensor_from_vector(string_to_bytes(report.component_assembly_id)));
  root.write("meta/representation_contract_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.representation_contract)));
  root.write(
      "meta/channel_axis_policy_bytes",
      int64_tensor_from_vector(string_to_bytes(report.channel_axis_policy)));
  root.write(
      "meta/cell_valid_policy_bytes",
      int64_tensor_from_vector(string_to_bytes(report.cell_valid_policy)));
  root.write("meta/required_feature_coords",
             int64_tensor_from_vector(report.required_feature_coords));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

} // namespace channel_graph_first_representation_launcher_detail

template <typename DatatypeT>
class channel_graph_first_representation_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::channel_graph_first_pipeline_builder_t<
          DatatypeT>;

  channel_graph_first_representation_launcher_t(
      builder_t builder,
      channel_graph_first_representation_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] channel_graph_first_representation_training_report_t
  dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    channel_graph_first_representation_training_report_t out{};
    out.training_id = builder_.bundle().vicreg_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.component_assembly_id = builder_.bundle().vicreg.component_assembly_id;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.required_feature_coords =
        builder_.bundle().vicreg.required_feature_coords;
    out.cell_valid_policy = plan.mask_profile;
    out.channel_count = builder_.bundle().vicreg.channel_count;
    out.history_length = builder_.bundle().vicreg.history_length;
    out.input_width = builder_.bundle().vicreg.input_width;
    out.encoding_dim = builder_.bundle().vicreg.encoding_dim;
    out.representation_embedding_dim = out.encoding_dim;
    out.feature_hidden_dim = builder_.bundle().vicreg.feature_hidden_dim;
    out.temporal_depth = builder_.bundle().vicreg.temporal_depth;
    out.recency_decay = builder_.bundle().vicreg.recency_decay;
    out.min_valid_fraction = builder_.bundle().vicreg.min_valid_fraction;
    out.use_missingness_indicators =
        builder_.bundle().vicreg.use_missingness_indicators;
    out.projector_dim = builder_.bundle().vicreg.vicreg_projector_dim;
    out.projector_hidden_dim =
        builder_.bundle().vicreg.vicreg_projector_hidden_dim;
    out.projector_depth = builder_.bundle().vicreg.vicreg_projector_depth;
    out.vicreg_invariance_weight =
        builder_.bundle().vicreg.vicreg_invariance_weight;
    out.vicreg_variance_weight =
        builder_.bundle().vicreg.vicreg_variance_weight;
    out.vicreg_covariance_weight =
        builder_.bundle().vicreg.vicreg_covariance_weight;
    out.vicreg_variance_floor = builder_.bundle().vicreg.vicreg_variance_floor;
    out.vicreg_eps = builder_.bundle().vicreg.vicreg_eps;
    out.global_aux_weight = builder_.bundle().vicreg.global_aux_weight;
    out.min_valid_rows = builder_.bundle().vicreg.min_valid_rows;
    out.skip_non_finite_loss = builder_.bundle().vicreg.skip_non_finite_loss;
    out.jitter_std = builder_.bundle().vicreg.jitter_std;
    out.feature_dropout_prob = builder_.bundle().vicreg.feature_dropout_prob;
    out.history_dropout_prob = builder_.bundle().vicreg.history_dropout_prob;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = builder_.bundle().vicreg_training.seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.checkpoint_every = builder_.bundle().vicreg_training.checkpoint_every;
    out.report_every = builder_.bundle().vicreg_training.report_every;
    out.validation_every = builder_.bundle().vicreg_training.validation_every;
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
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(builder_.bundle().wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] channel_graph_first_representation_training_report_t
  run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const bool train_target = effective_train_target();

    const auto checkpoint_every =
        builder_.bundle().vicreg_training.checkpoint_every;
    const auto report_every = builder_.bundle().vicreg_training.report_every;
    if ((options_.write_report || (train_target && checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error(
          "[channel_graph_first_representation_launcher] report_path is "
          "required for configured report or checkpoint output");
    }

    const auto seed_scope =
        channel_graph_first_representation_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(builder_.bundle().vicreg_training.seed),
            builder_.options().device);
    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            builder_.bundle().wave_settings, options_.runtime_report_mode);
    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    auto encoder = builder_.make_vicreg_encoder();
    auto projector =
        cuwacunu::wikimyei::representation::encoding::vicreg::VicregProjector(
            cuwacunu::wikimyei::representation::encoding::vicreg::
                channel_projector_options_from_spec(builder_.bundle().vicreg));
    auto train_options = cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_train_options_from_spec(builder_.bundle().vicreg);
    train_options.grad_clip_norm =
        builder_.bundle().vicreg_training.grad_clip_norm;
    auto model = cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_train_model_t(std::move(encoder), std::move(projector),
                             builder_.bundle().vicreg_training.learning_rate,
                             train_options);

    auto report = dry_run_report();
    report.seed_scope = seed_scope;
    report.runtime_report_mode =
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);
    double loss_sum = 0.0;
    double invariance_sum = 0.0;
    double variance_sum = 0.0;
    double covariance_sum = 0.0;
    double global_sum = 0.0;
    int64_t loss_count = 0;
    double valid_feature_fraction_sum = 0.0;
    int64_t diagnostics_count = 0;
    double augmented_valid_feature_fraction_sum = 0.0;
    double augmented_feature_retention_fraction_sum = 0.0;
    int64_t augmentation_diagnostics_count = 0;
    double max_grad_norm = -std::numeric_limits<double>::infinity();
    int64_t geometry_count = 0;
    double effective_rank_sum = 0.0;
    double effective_rank_fraction_sum = 0.0;
    double min_dimension_variance_sum = 0.0;
    double max_dimension_variance_sum = 0.0;
    double condition_number_sum = 0.0;
    double isotropy_score_sum = 0.0;
    std::vector<double> effective_rank_per_channel_sum;
    std::vector<int64_t> effective_rank_per_channel_count;
    std::vector<double> min_dimension_variance_per_channel_sum;
    std::vector<int64_t> min_dimension_variance_per_channel_count;
    std::vector<double> condition_number_per_channel_sum;
    std::vector<int64_t> condition_number_per_channel_count;
    std::vector<double> isotropy_score_per_channel_sum;
    std::vector<int64_t> isotropy_score_per_channel_count;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
        report.mean_invariance_loss =
            invariance_sum / static_cast<double>(loss_count);
        report.mean_variance_loss =
            variance_sum / static_cast<double>(loss_count);
        report.mean_covariance_loss =
            covariance_sum / static_cast<double>(loss_count);
        report.mean_global_loss = global_sum / static_cast<double>(loss_count);
      }
      if (diagnostics_count > 0) {
        report.mean_valid_feature_fraction =
            valid_feature_fraction_sum / static_cast<double>(diagnostics_count);
      }
      if (augmentation_diagnostics_count > 0) {
        report.mean_augmented_valid_feature_fraction =
            augmented_valid_feature_fraction_sum /
            static_cast<double>(augmentation_diagnostics_count);
        report.mean_augmented_feature_retention_fraction =
            augmented_feature_retention_fraction_sum /
            static_cast<double>(augmentation_diagnostics_count);
      }
      if (std::isfinite(max_grad_norm)) {
        report.max_grad_norm = max_grad_norm;
      }
      if (geometry_count > 0) {
        report.representation_effective_rank =
            effective_rank_sum / static_cast<double>(geometry_count);
        report.representation_effective_rank_fraction =
            effective_rank_fraction_sum / static_cast<double>(geometry_count);
        report.representation_min_dimension_variance =
            min_dimension_variance_sum / static_cast<double>(geometry_count);
        report.representation_max_dimension_variance =
            max_dimension_variance_sum / static_cast<double>(geometry_count);
        report.representation_condition_number =
            condition_number_sum / static_cast<double>(geometry_count);
        report.representation_isotropy_score =
            isotropy_score_sum / static_cast<double>(geometry_count);
        report.representation_effective_rank_per_channel =
            channel_graph_first_representation_launcher_detail::mean_vector(
                effective_rank_per_channel_sum,
                effective_rank_per_channel_count);
        report.representation_min_dimension_variance_per_channel =
            channel_graph_first_representation_launcher_detail::mean_vector(
                min_dimension_variance_per_channel_sum,
                min_dimension_variance_per_channel_count);
        report.representation_condition_number_per_channel =
            channel_graph_first_representation_launcher_detail::mean_vector(
                condition_number_per_channel_sum,
                condition_number_per_channel_count);
        report.representation_isotropy_score_per_channel =
            channel_graph_first_representation_launcher_detail::mean_vector(
                isotropy_score_per_channel_sum,
                isotropy_score_per_channel_count);
      }
    };

    auto record_representation_geometry =
        [&](const cuwacunu::wikimyei::representation::encoding::vicreg::
                channel_preserving_encoder_output_t &encoded) {
          const auto geometry =
              channel_graph_first_representation_launcher_detail::
                  summarize_channel_representation_geometry(
                      encoded.reduced, encoded.reduced_mask);
          if (!geometry.has_rows()) {
            return;
          }
          report.representation_embedding_dim = geometry.embedding_dim;
          effective_rank_sum += geometry.effective_rank;
          effective_rank_fraction_sum += geometry.effective_rank_fraction;
          min_dimension_variance_sum += geometry.min_dimension_variance;
          max_dimension_variance_sum += geometry.max_dimension_variance;
          if (std::isfinite(geometry.condition_number)) {
            condition_number_sum += geometry.condition_number;
          } else {
            condition_number_sum = std::numeric_limits<double>::infinity();
          }
          isotropy_score_sum += geometry.isotropy_score;
          ++geometry_count;
          const auto per_channel =
              channel_graph_first_representation_launcher_detail::
                  summarize_channel_representation_geometry_by_channel(
                      encoded.reduced, encoded.reduced_mask);
          for (std::size_t i = 0; i < per_channel.size(); ++i) {
            if (!per_channel[i].has_rows()) {
              continue;
            }
            channel_graph_first_representation_launcher_detail::
                accumulate_finite_value(i, per_channel[i].effective_rank,
                                        effective_rank_per_channel_sum,
                                        effective_rank_per_channel_count);
            channel_graph_first_representation_launcher_detail::
                accumulate_finite_value(
                    i, per_channel[i].min_dimension_variance,
                    min_dimension_variance_per_channel_sum,
                    min_dimension_variance_per_channel_count);
            channel_graph_first_representation_launcher_detail::
                accumulate_finite_value(i, per_channel[i].condition_number,
                                        condition_number_per_channel_sum,
                                        condition_number_per_channel_count);
            channel_graph_first_representation_launcher_detail::
                accumulate_finite_value(i, per_channel[i].isotropy_score,
                                        isotropy_score_per_channel_sum,
                                        isotropy_score_per_channel_count);
          }
        };

    auto write_checkpoint = [&]() {
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".vicreg.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_vicreg_v1";
      channel_graph_first_representation_launcher_detail::
          save_vicreg_checkpoint_file(checkpoint_path, report, model);
    };

    auto write_report = [&]() {
      refresh_running_report();
      report.last_report_attempted_step = report.steps_attempted;
      if (!options_.report_path.empty()) {
        report.report_path = options_.report_path.string();
      }
      if (options_.write_report) {
        report.report_written = true;
        ++report.report_write_count;
        channel_graph_first_representation_launcher_detail::write_report_file(
            options_.report_path, report);
      }
      if (options_.write_report && report.runtime_lls_emitted) {
        channel_graph_first_representation_launcher_detail::
            write_runtime_lls_sidecars(
                options_.report_path, report.nodelift_runtime_lls,
                report.representation_training_runtime_lls);
      }
      if (options_.learning_probe_report_sink) {
        options_.learning_probe_report_sink(report.to_text());
      }
    };

    const int64_t max_steps = builder_.bundle().vicreg_training.max_steps;
    while (max_steps == 0 || report.steps_attempted < max_steps) {
      if (!lifted_stream.has_next()) {
        if (!train_target || max_steps == 0) {
          break;
        }
        lifted_stream.reset();
        if (!lifted_stream.has_next()) {
          break;
        }
      }
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto lifted = lifted_stream.next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(lifted.cursor.anchor_count());

      auto input = cuwacunu::wikimyei::representation::encoding::vicreg::
          make_channel_node_encoder_input(lifted);
      const auto node_support =
          channel_graph_first_representation_launcher_detail::
              summarize_node_support(input);
      channel_graph_first_representation_launcher_detail::
          accumulate_node_support(report, node_support);
      cuwacunu::wikimyei::representation::encoding::vicreg::
          vicreg_train_step_result_t step{};
      if (train_target) {
        step = model.train_one_batch(input.data, input.feature_mask);
        auto encoded = model.encode(input.data, input.feature_mask,
                                    /*detach_to_cpu=*/false);
        record_representation_geometry(encoded);
      } else {
        auto encoded = model.encode(input.data, input.feature_mask,
                                    /*detach_to_cpu=*/false);
        record_representation_geometry(encoded);
        step.valid_projection_rows =
            encoded.reduced_mask.sum().template item<int64_t>();
        step.gradients_finite =
            torch::isfinite(encoded.reduced).all().template item<bool>();
        step.skipped =
            step.valid_projection_rows <= 0 || !step.gradients_finite;
      }

      report.adapter_original_rows += input.diagnostics.original_rows;
      report.adapter_valid_feature_count +=
          input.diagnostics.valid_feature_count;
      report.adapter_valid_cell_any_count +=
          input.diagnostics.valid_cell_any_count;
      report.adapter_valid_cell_all_count +=
          input.diagnostics.valid_cell_all_count;
      valid_feature_fraction_sum += input.diagnostics.valid_feature_fraction;
      ++diagnostics_count;
      if (train_target) {
        report.augmented_valid_feature_count +=
            step.view1_valid_feature_count + step.view2_valid_feature_count;
        report.last_view1_valid_feature_fraction =
            step.view1_valid_feature_fraction;
        report.last_view2_valid_feature_fraction =
            step.view2_valid_feature_fraction;
        double view_fraction_sum = 0.0;
        int64_t view_fraction_count = 0;
        if (std::isfinite(step.view1_valid_feature_fraction)) {
          view_fraction_sum += step.view1_valid_feature_fraction;
          ++view_fraction_count;
        }
        if (std::isfinite(step.view2_valid_feature_fraction)) {
          view_fraction_sum += step.view2_valid_feature_fraction;
          ++view_fraction_count;
        }
        double retention_sum = 0.0;
        int64_t retention_count = 0;
        if (std::isfinite(step.view1_feature_retention_fraction)) {
          retention_sum += step.view1_feature_retention_fraction;
          ++retention_count;
        }
        if (std::isfinite(step.view2_feature_retention_fraction)) {
          retention_sum += step.view2_feature_retention_fraction;
          ++retention_count;
        }
        if (view_fraction_count > 0 && retention_count > 0) {
          report.last_augmented_valid_feature_fraction =
              view_fraction_sum / static_cast<double>(view_fraction_count);
          report.last_augmented_feature_retention_fraction =
              retention_sum / static_cast<double>(retention_count);
          augmented_valid_feature_fraction_sum +=
              report.last_augmented_valid_feature_fraction;
          augmented_feature_retention_fraction_sum +=
              report.last_augmented_feature_retention_fraction;
          ++augmentation_diagnostics_count;
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
      } else if (step.skipped) {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }

      report.last_valid_projection_rows = step.valid_projection_rows;
      report.total_valid_projection_rows += step.valid_projection_rows;
      report.last_grad_norm = step.grad_norm;
      if (std::isfinite(step.grad_norm)) {
        max_grad_norm = std::max(max_grad_norm, step.grad_norm);
      }
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 && step.gradients_finite) ? 1.0
                                                                          : 0.0;
      if (cuwacunu::hero::lattice::runtime_report::runtime_report_enabled(
              runtime_report_mode)) {
        report.nodelift_runtime_lls = lifted.runtime_lls;
        report.representation_training_runtime_lls =
            channel_graph_first_representation_launcher_detail::
                make_channel_representation_training_runtime_lls(
                    lifted, input, step,
                    builder_.bundle().vicreg.component_assembly_id,
                    cuwacunu::wikimyei::assembly::make_assembly_token(
                        builder_.bundle().vicreg_assembly.family,
                        builder_.bundle().vicreg_assembly.component_assembly_id,
                        builder_.bundle().vicreg_assembly.version_token),
                    cuwacunu::kikijyeba::topology::dock_binding_token(
                        builder_.bundle().dock_binding),
                    cuwacunu::kikijyeba::protocol::
                        component_stream_wave_from_settings(
                            builder_.bundle().wave_settings),
                    runtime_report_mode, report.optimizer_steps,
                    report.wave_pulses_attempted);
        report.runtime_lls_emitted = true;
      }
      if (step.optimizer_step_applied) {
        report.last_loss =
            channel_graph_first_representation_launcher_detail::scalar_or_nan(
                step.loss);
        report.last_invariance_loss =
            channel_graph_first_representation_launcher_detail::scalar_or_nan(
                step.invariance_loss);
        report.last_variance_loss =
            channel_graph_first_representation_launcher_detail::scalar_or_nan(
                step.variance_loss);
        report.last_covariance_loss =
            channel_graph_first_representation_launcher_detail::scalar_or_nan(
                step.covariance_loss);
        report.last_global_loss =
            channel_graph_first_representation_launcher_detail::scalar_or_nan(
                step.global_loss);
        if (std::isfinite(report.last_loss)) {
          loss_sum += report.last_loss;
          invariance_sum += report.last_invariance_loss;
          variance_sum += report.last_variance_loss;
          covariance_sum += report.last_covariance_loss;
          global_sum += report.last_global_loss;
          ++loss_count;
        }
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
           cuwacunu::hero::runtime::settings::wave_action_t::train;
  }

  void validate_batch_size_contract() const {
    const auto expected =
        static_cast<std::size_t>(builder_.bundle().vicreg_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[channel_graph_first_representation_launcher] builder batch_size "
          "must match VICReg training BATCH_SIZE");
    }
  }

  builder_t builder_;
  channel_graph_first_representation_launcher_options_t options_{};
};

} // namespace cuwacunu::jkimyei::training::representation
