// SPDX-License-Identifier: MIT
#pragma once

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/runtime_hero/runtime/job_manifest.h"
#include "hero/runtime_hero/runtime/job_state.h"

namespace cuwacunu::hero::runtime::job_events_probe {

inline constexpr const char *k_job_events_probe_record_schema_v1 =
    "kikijyeba.runtime.job_events.probe_record.v1";
inline constexpr const char *k_job_events_probe_stream_leaf =
    "runtime.job_events.probe";

struct job_events_probe_stream_config_t {
  std::string record_schema{k_job_events_probe_record_schema_v1};
  std::string stream_leaf{k_job_events_probe_stream_leaf};
  bool emit_lifecycle{true};
  bool emit_scalar_metrics{true};
  bool emit_report_metrics{true};
  bool emit_artifacts{true};
  bool emit_warnings{true};
};

struct job_events_probe_record_t {
  std::string event_kind{};
  std::string phase{};
  std::string status{};
  std::string severity{"info"};
  std::string message{};
  std::string metric_name{};
  std::string metric_value{};
  std::string metric_unit{};
  std::string metric_source{};
  std::string series_id{};
  std::string component_id{};
  std::string split_role{};
  std::string step{};
  std::string epoch{};
  std::string batch_index{};
  std::string anchor_begin{};
  std::string anchor_end{};
  std::string sample_index{};
  std::string artifact_kind{};
  std::string artifact_path{};
  std::string artifact_digest{};
  std::string artifact_schema{};
};

struct job_events_probe_write_summary_t {
  bool enabled{false};
  bool written{false};
  std::filesystem::path stream_path{};
  std::int64_t record_count{0};
  std::string error{};
  job_events_probe_stream_config_t config{};
};

[[nodiscard]] inline std::filesystem::path
probe_stream_path_for_job_dir(const std::filesystem::path &job_dir,
                              const job_events_probe_stream_config_t &config =
                                  job_events_probe_stream_config_t{}) {
  return job_dir / config.stream_leaf;
}

[[nodiscard]] inline std::string one_line(std::string value) {
  for (char &ch : value) {
    if (ch == '\n' || ch == '\r') {
      ch = ' ';
    }
  }
  return value;
}

[[nodiscard]] inline std::string double_text(double value) {
  std::ostringstream out;
  out << std::setprecision(17) << value;
  return out.str();
}

[[nodiscard]] inline std::string current_unix_ms_text() {
  const auto now = std::chrono::system_clock::now();
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  return std::to_string(millis.count());
}

inline void append_field(std::ostringstream &out, std::string_view key,
                         std::string value, bool keep_empty = false) {
  if (keep_empty || !value.empty()) {
    out << key << "=" << one_line(std::move(value)) << "\n";
  }
}

[[nodiscard]] inline std::string
serialize_record(const job_manifest_t &manifest,
                 const job_events_probe_record_t &event, std::int64_t sequence,
                 const std::string &event_time_unix_ms,
                 const job_events_probe_stream_config_t &config) {
  std::ostringstream out;
  append_field(out, "record_schema", config.record_schema, true);
  append_field(out, "probe_stream_leaf", config.stream_leaf, true);
  append_field(out, "sequence", std::to_string(sequence), true);
  append_field(out, "event_time_unix_ms", event_time_unix_ms, true);
  append_field(out, "authority", "visibility_only", true);
  append_field(out, "proof_authority", "false", true);
  append_field(out, "dispatch_authority", "false", true);
  append_field(out, "lattice_fact", "false", true);
  append_field(out, "event_kind", event.event_kind, true);
  append_field(out, "phase", event.phase);
  append_field(out, "status", event.status);
  append_field(out, "severity", event.severity, true);
  append_field(out, "message", event.message);
  append_field(out, "job_id", manifest.job_id, true);
  append_field(out, "job_stable_id", manifest.job_stable_id, true);
  append_field(out, "job_attempt_id", manifest.job_attempt_id, true);
  append_field(out, "job_attempt_index",
               std::to_string(manifest.job_attempt_index), true);
  append_field(out, "job_attempt_policy", manifest.job_attempt_policy, true);
  append_field(out, "job_kind", manifest.job_kind, true);
  append_field(out, "config_bundle_id", manifest.config_bundle_id);
  append_field(out, "config_receipt_id", manifest.config_receipt_id);
  append_field(out, "component_family_id", manifest.component_family_id);
  append_field(out, "component_spawn_id", manifest.component_spawn_id);
  append_field(out, "component_spawn_fingerprint",
               manifest.component_spawn_fingerprint);
  append_field(out, "component_operator_surface_digest",
               manifest.component_operator_surface_digest);
  append_field(out, "protocol_id", manifest.protocol_id);
  append_field(out, "target_component_family_id",
               manifest.target_component_family_id);
  append_field(out, "wave_id", manifest.wave_id);
  append_field(out, "wave_action", manifest.wave_action);
  append_field(out, "wave_mode", manifest.wave_mode);
  append_field(out, "source_cursor_token", manifest.source_cursor_token);
  append_field(out, "source_range_policy", manifest.source_range_policy);
  append_field(out, "source_order_policy", manifest.source_order_policy);
  append_field(out, "protocol_contract_fingerprint",
               manifest.protocol_contract_fingerprint);
  append_field(out, "graph_order_fingerprint",
               manifest.graph_order_fingerprint);
  append_field(out, "dock_binding_fingerprint",
               manifest.dock_binding_fingerprint);
  append_field(out, "runtime_handoff_id", manifest.runtime_handoff_id);
  append_field(out, "runtime_handoff_digest", manifest.runtime_handoff_digest);
  append_field(out, "marshal_target_driver_run_id",
               manifest.marshal_target_driver_run_id);
  append_field(out, "metric_name", event.metric_name);
  append_field(out, "metric_value", event.metric_value);
  append_field(out, "metric_unit", event.metric_unit);
  append_field(out, "metric_source", event.metric_source);
  append_field(out, "series_id", event.series_id);
  append_field(out, "component_id", event.component_id);
  append_field(out, "split_role", event.split_role);
  append_field(out, "step", event.step);
  append_field(out, "epoch", event.epoch);
  append_field(out, "batch_index", event.batch_index);
  append_field(out, "anchor_begin", event.anchor_begin);
  append_field(out, "anchor_end", event.anchor_end);
  append_field(out, "sample_index", event.sample_index);
  append_field(out, "artifact_kind", event.artifact_kind);
  append_field(out, "artifact_path", event.artifact_path);
  append_field(out, "artifact_digest", event.artifact_digest);
  append_field(out, "artifact_schema", event.artifact_schema);
  out << "\n";
  return out.str();
}

inline void append_record_file(const std::filesystem::path &path,
                               const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::app | std::ios::binary);
  if (!out) {
    throw std::runtime_error(
        "[runtime_job_events_probe] failed to open stream: " + path.string());
  }
  out << text;
  if (!out) {
    throw std::runtime_error(
        "[runtime_job_events_probe] failed to write stream: " + path.string());
  }
}

[[nodiscard]] inline job_events_probe_write_summary_t
make_write_summary(bool enabled, const std::filesystem::path &job_dir,
                   job_events_probe_stream_config_t config =
                       job_events_probe_stream_config_t{}) {
  job_events_probe_write_summary_t out{};
  out.enabled = enabled;
  out.config = std::move(config);
  if (enabled) {
    out.stream_path = probe_stream_path_for_job_dir(job_dir, out.config);
  }
  return out;
}

inline void append_record(const job_manifest_t &manifest,
                          job_events_probe_write_summary_t *summary,
                          job_events_probe_record_t event) {
  if (summary == nullptr || !summary->enabled || !summary->error.empty()) {
    return;
  }
  try {
    if (summary->stream_path.empty()) {
      throw std::runtime_error("probe stream path is empty");
    }
    const auto sequence = summary->record_count + 1;
    append_record_file(summary->stream_path,
                       serialize_record(manifest, event, sequence,
                                        current_unix_ms_text(),
                                        summary->config));
    summary->written = true;
    summary->record_count = sequence;
  } catch (const std::exception &ex) {
    summary->error = ex.what();
  }
}

inline void
apply_summary_to_state(const job_events_probe_write_summary_t &summary,
                       job_state_t *state) {
  if (state == nullptr) {
    return;
  }
  state->probe_sidecar_enabled = summary.enabled;
  state->probe_records_written = summary.written;
  state->probe_stream_path = summary.stream_path.empty()
                                 ? std::string{}
                                 : summary.stream_path.string();
  state->probe_record_count = summary.record_count;
  state->probe_record_error = summary.error;
}

inline void write_job_started_event(const job_manifest_t &manifest,
                                    job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_lifecycle) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = "job.lifecycle.started";
  event.phase = "started";
  event.status = "started";
  append_record(manifest, summary, std::move(event));
}

inline void write_job_progress_event(const job_manifest_t &manifest,
                                     job_events_probe_write_summary_t *summary,
                                     std::string phase, std::string status,
                                     std::string message = {}) {
  if (summary == nullptr || !summary->config.emit_lifecycle) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = "job.progress.phase";
  event.phase = std::move(phase);
  event.status = std::move(status);
  event.message = std::move(message);
  append_record(manifest, summary, std::move(event));
}

inline void append_scalar_metric(
    const job_manifest_t &manifest, job_events_probe_write_summary_t *summary,
    std::string name, std::string value, std::string unit = {},
    std::string source = "job_state", std::string step = {},
    std::string anchor_begin = {}, std::string anchor_end = {}) {
  if (value.empty()) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = "job.metric.scalar";
  event.phase = "metrics";
  event.metric_name = std::move(name);
  event.metric_value = std::move(value);
  event.metric_unit = std::move(unit);
  event.metric_source = std::move(source);
  event.series_id = event.metric_source + "." + event.metric_name;
  event.step = std::move(step);
  event.anchor_begin = std::move(anchor_begin);
  event.anchor_end = std::move(anchor_end);
  append_record(manifest, summary, std::move(event));
}

[[nodiscard]] inline bool is_finite_numeric_text(const std::string &value);

[[nodiscard]] inline bool has_prefix(std::string_view text,
                                     std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] inline bool has_suffix(std::string_view text,
                                     std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.substr(text.size() - suffix.size()) == suffix;
}

[[nodiscard]] inline bool contains_text(std::string_view text,
                                        std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

[[nodiscard]] inline std::string
map_value_or_empty(const std::unordered_map<std::string, std::string> &values,
                   std::string_view key) {
  const auto it = values.find(std::string(key));
  return it == values.end() ? std::string{} : it->second;
}

[[nodiscard]] inline std::string
first_nonempty(const std::unordered_map<std::string, std::string> &values,
               const std::vector<const char *> &keys,
               std::string fallback = {}) {
  for (const char *key : keys) {
    const auto value = map_value_or_empty(values, key);
    if (!value.empty()) {
      return value;
    }
  }
  return fallback;
}

[[nodiscard]] inline bool
is_nonnegative_integer_text(const std::string &value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return false;
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  const auto text = value.substr(begin, end - begin + 1);
  char *parse_end = nullptr;
  errno = 0;
  const long long parsed = std::strtoll(text.c_str(), &parse_end, 10);
  return parse_end != text.c_str() && parse_end != nullptr &&
         *parse_end == '\0' && errno != ERANGE && parsed >= 0;
}

[[nodiscard]] inline bool
parse_nonnegative_integer_text(const std::string &value,
                               std::int64_t *out = nullptr) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return false;
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  const auto text = value.substr(begin, end - begin + 1);
  char *parse_end = nullptr;
  errno = 0;
  const long long parsed = std::strtoll(text.c_str(), &parse_end, 10);
  if (parse_end == text.c_str() || parse_end == nullptr || *parse_end != '\0' ||
      errno == ERANGE || parsed < 0) {
    return false;
  }
  if (out != nullptr) {
    *out = static_cast<std::int64_t>(parsed);
  }
  return true;
}

[[nodiscard]] inline std::string first_nonnegative_integer(
    const std::unordered_map<std::string, std::string> &values,
    const std::vector<const char *> &keys, std::string fallback = {}) {
  for (const char *key : keys) {
    const auto value = map_value_or_empty(values, key);
    if (is_nonnegative_integer_text(value)) {
      return value;
    }
  }
  return fallback;
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_kv_text(std::string_view text) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream in{std::string(text)};
  std::string line;
  while (std::getline(in, line)) {
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto key =
        job_layout::trim_ascii(std::string_view(line).substr(0, eq));
    const auto value =
        job_layout::trim_ascii(std::string_view(line).substr(eq + 1U));
    if (!key.empty()) {
      out[key] = value;
    }
  }
  return out;
}

[[nodiscard]] inline bool is_progress_metric_name(std::string_view name) {
  return name == "steps_attempted" || name == "steps_completed" ||
         name == "optimizer_steps" || name == "wave_pulses_attempted" ||
         name == "wave_pulses_completed" || name == "wave_pulses_skipped";
}

[[nodiscard]] inline bool
is_representation_training_metric_name(std::string_view name) {
  return is_progress_metric_name(name) || name == "last_loss" ||
         name == "mean_loss" || contains_text(name, "invariance_loss") ||
         contains_text(name, "variance_loss") ||
         contains_text(name, "covariance_loss") ||
         contains_text(name, "global_loss") || has_prefix(name, "loss_jepa") ||
         has_prefix(name, "loss_mae") || has_prefix(name, "loss_tf_align") ||
         has_prefix(name, "loss_vicreg") || name == "last_grad_norm" ||
         name == "max_grad_norm" || name == "target_ema_distance" ||
         name == "latent_std" || name == "latent_norm" ||
         has_prefix(name, "representation_effective_rank") ||
         has_prefix(name, "representation_min_dimension_variance") ||
         has_prefix(name, "representation_max_dimension_variance") ||
         has_prefix(name, "representation_condition_number") ||
         has_prefix(name, "representation_isotropy_score") ||
         name == "valid_latent_rows" || name == "total_valid_latent_rows" ||
         name == "sample_valid_fraction" || name == "channel_valid_fraction" ||
         name == "mean_valid_feature_fraction" ||
         name == "mean_augmented_valid_feature_fraction" ||
         name == "last_augmented_valid_feature_fraction" ||
         name == "mean_augmented_feature_retention_fraction" ||
         name == "last_augmented_feature_retention_fraction" ||
         name == "nonfinite_output_count" || name == "finite_parameter_check" ||
         name == "gradients_finite";
}

[[nodiscard]] inline bool
is_representation_augmentation_metric_name(std::string_view name) {
  return name == "dropout" || has_prefix(name, "mask_ratio_") ||
         name == "min_context_ratio" || name == "gaussian_jitter_std" ||
         name == "feature_dropout_prob" || name == "history_dropout_prob" ||
         name == "time_crop_jitter_max" || name == "time_dilation_min" ||
         name == "time_dilation_max" || name == "time_warp_max" ||
         name == "amplitude_scale_min" || name == "amplitude_scale_max" ||
         name == "amplitude_shift_std" || name == "frequency_mask_ratio" ||
         name == "frequency_jitter_std" || name == "phase_jitter_max" ||
         name == "channel_dropout_prob" ||
         name == "cross_channel_dropout_prob" || name == "node_dropout_prob" ||
         name == "edge_dropout_prob" ||
         name == "magnitude_normalization_noise_std" ||
         name == "augmented_feature_retention_fraction";
}

[[nodiscard]] inline bool
is_forecast_training_metric_name(std::string_view name) {
  return is_progress_metric_name(name) || name == "last_loss" ||
         name == "mean_loss" || name == "last_valid_target_count" ||
         name == "total_valid_target_count" ||
         name == "last_valid_target_fraction" ||
         name == "mean_valid_target_fraction" || name == "last_sigma_mean" ||
         name == "mean_sigma_mean" || name == "min_sigma_min" ||
         name == "max_sigma_max" || name == "last_sigma_mean_valid" ||
         name == "mean_sigma_mean_valid" || name == "min_sigma_min_valid" ||
         name == "max_sigma_max_valid" || name == "mean_mixture_entropy" ||
         name == "last_grad_norm" || name == "max_grad_norm" ||
         name == "finite_parameter_check" ||
         name == "representation_parameter_device_check" ||
         name == "mdn_parameter_device_check" ||
         name == "nonfinite_output_count";
}

[[nodiscard]] inline bool
is_forecast_oracle_metric_name(std::string_view name) {
  return name == "forecast_ev_valid_count" || name == "ev_mae" ||
         name == "ev_rmse" || name == "signed_error" ||
         name == "directional_accuracy" ||
         has_prefix(name, "edge_return_projection_") ||
         has_prefix(name, "direct_edge_return_readout_");
}

[[nodiscard]] inline bool
is_policy_training_metric_name(std::string_view name) {
  return is_progress_metric_name(name) || has_prefix(name, "ppo_") ||
         (has_prefix(name, "sample_") &&
          (contains_text(name, "policy_loss") ||
           contains_text(name, "value_loss") ||
           contains_text(name, "entropy") || contains_text(name, "approx_kl") ||
           contains_text(name, "clip_fraction") ||
           contains_text(name, "explained_variance"))) ||
         (has_prefix(name, "episode_") &&
          (contains_text(name, "reward_") || contains_text(name, "entropy") ||
           contains_text(name, "turnover")));
}

[[nodiscard]] inline bool is_policy_oracle_metric_name(std::string_view name) {
  return contains_text(name, "oracle") || contains_text(name, "regret") ||
         contains_text(name, "best_asset") ||
         contains_text(name, "mean_projection_directional_accuracy") ||
         contains_text(name, "directional_agreement");
}

[[nodiscard]] inline std::string learning_metric_unit(std::string_view name) {
  if (has_suffix(name, "_count") || contains_text(name, "count") ||
      contains_text(name, "steps") || contains_text(name, "pulses")) {
    return "count";
  }
  if (contains_text(name, "loss") || contains_text(name, "nll")) {
    return "loss";
  }
  if (contains_text(name, "accuracy") || contains_text(name, "agreement") ||
      contains_text(name, "fraction") || contains_text(name, "ratio") ||
      contains_text(name, "prob") || contains_text(name, "dropout")) {
    return "fraction";
  }
  if (contains_text(name, "mae") || contains_text(name, "rmse") ||
      contains_text(name, "error") || contains_text(name, "sigma") ||
      contains_text(name, "entropy") || contains_text(name, "norm") ||
      contains_text(name, "std")) {
    return "scalar";
  }
  return {};
}

[[nodiscard]] inline std::string learning_component_id(
    const job_manifest_t &manifest,
    const std::unordered_map<std::string, std::string> &report) {
  if (!manifest.component_family_id.empty()) {
    return manifest.component_family_id;
  }
  if (!manifest.target_component_family_id.empty()) {
    return manifest.target_component_family_id;
  }
  return first_nonempty(report,
                        {"component_assembly_id",
                         "input_representation_assembly_id",
                         "policy_family_id"},
                        manifest.job_kind);
}

[[nodiscard]] inline std::string learning_split_role(
    const std::unordered_map<std::string, std::string> &report) {
  return first_nonempty(report, {"split_name", "selector_split",
                                 "protected_split_name", "target_action"});
}

[[nodiscard]] inline bool learning_report_range_is_unresolved_full_source(
    const job_state_t &state,
    const std::unordered_map<std::string, std::string> &report) {
  if (state.resolved_anchor_index_end <= state.resolved_anchor_index_begin) {
    return false;
  }
  if (is_nonnegative_integer_text(
          map_value_or_empty(report, "requested_anchor_index_begin")) ||
      is_nonnegative_integer_text(
          map_value_or_empty(report, "requested_anchor_index_end"))) {
    return false;
  }

  std::int64_t completed_begin = -1;
  std::int64_t completed_end = -1;
  if (!parse_nonnegative_integer_text(
          map_value_or_empty(report, "completed_anchor_start"),
          &completed_begin) ||
      !parse_nonnegative_integer_text(
          map_value_or_empty(report, "completed_anchor_end"), &completed_end)) {
    return false;
  }
  if (completed_begin ==
          static_cast<std::int64_t>(state.resolved_anchor_index_begin) &&
      completed_end ==
          static_cast<std::int64_t>(state.resolved_anchor_index_end)) {
    return false;
  }

  std::int64_t source_begin = -1;
  std::int64_t source_end = -1;
  if (parse_nonnegative_integer_text(
          map_value_or_empty(report, "source_range_start"), &source_begin) &&
      parse_nonnegative_integer_text(
          map_value_or_empty(report, "source_range_end"), &source_end) &&
      completed_begin == source_begin && completed_end == source_end) {
    return true;
  }

  std::int64_t source_anchor_count = -1;
  return completed_begin == 0 &&
         parse_nonnegative_integer_text(
             map_value_or_empty(report, "source_anchor_count"),
             &source_anchor_count) &&
         completed_end == source_anchor_count;
}

[[nodiscard]] inline std::string
learning_step(const job_state_t &state,
              const std::unordered_map<std::string, std::string> &report) {
  return first_nonempty(report, {"optimizer_steps", "steps_completed"},
                        std::to_string(state.steps_completed));
}

[[nodiscard]] inline std::string learning_anchor_begin(
    const job_state_t &state,
    const std::unordered_map<std::string, std::string> &report) {
  if (learning_report_range_is_unresolved_full_source(state, report)) {
    return std::to_string(state.resolved_anchor_index_begin);
  }
  return first_nonnegative_integer(
      report, {"completed_anchor_start", "requested_anchor_index_begin"},
      std::to_string(state.resolved_anchor_index_begin));
}

[[nodiscard]] inline std::string learning_anchor_end(
    const job_state_t &state,
    const std::unordered_map<std::string, std::string> &report) {
  if (learning_report_range_is_unresolved_full_source(state, report)) {
    return std::to_string(state.resolved_anchor_index_end);
  }
  return first_nonnegative_integer(
      report, {"completed_anchor_end", "requested_anchor_index_end"},
      std::to_string(state.resolved_anchor_index_end));
}

[[nodiscard]] inline std::string
learning_metric_event_kind(const job_manifest_t &manifest,
                           std::string_view name) {
  const std::string_view job_kind = manifest.job_kind;
  if (contains_text(job_kind, "representation")) {
    if (is_representation_augmentation_metric_name(name)) {
      return "representation.augmentation.metric";
    }
    if (is_representation_training_metric_name(name)) {
      return "representation.training.metric";
    }
    return {};
  }
  if (contains_text(job_kind, "channel_inference_mdn")) {
    if (is_forecast_oracle_metric_name(name)) {
      return "forecast.oracle.metric";
    }
    if (is_forecast_training_metric_name(name)) {
      return "forecast.training.metric";
    }
    return {};
  }
  if (contains_text(job_kind, "policy_training")) {
    if (is_policy_oracle_metric_name(name)) {
      return "policy.oracle.metric";
    }
    if (is_policy_training_metric_name(name)) {
      return "policy.training.metric";
    }
    return {};
  }
  return {};
}

inline void append_learning_diagnostic_metric_events(
    const job_manifest_t &manifest, const job_state_t &state,
    const std::unordered_map<std::string, std::string> &report,
    job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_report_metrics) {
    return;
  }
  const std::string component_id = learning_component_id(manifest, report);
  const std::string split_role = learning_split_role(report);
  const std::string step = learning_step(state, report);
  const std::string anchor_begin = learning_anchor_begin(state, report);
  const std::string anchor_end = learning_anchor_end(state, report);
  for (const auto &entry : report) {
    if (!is_finite_numeric_text(entry.second)) {
      continue;
    }
    const auto event_kind = learning_metric_event_kind(manifest, entry.first);
    if (event_kind.empty()) {
      continue;
    }
    job_events_probe_record_t event{};
    event.event_kind = event_kind;
    event.phase = "learning_diagnostics";
    event.metric_name = entry.first;
    event.metric_value = entry.second;
    event.metric_unit = learning_metric_unit(entry.first);
    event.metric_source = "learning_diagnostics";
    event.series_id = event_kind + "." + event.metric_name;
    event.component_id = component_id;
    event.split_role = split_role;
    event.step = step;
    event.anchor_begin = anchor_begin;
    event.anchor_end = anchor_end;
    append_record(manifest, summary, std::move(event));
  }
}

inline void write_learning_report_snapshot_events(
    const job_manifest_t &manifest, const job_state_t &fallback_state,
    job_events_probe_write_summary_t *summary, std::string_view report_text) {
  if (summary == nullptr || !summary->config.emit_report_metrics ||
      report_text.empty()) {
    return;
  }
  const auto report = parse_kv_text(report_text);
  append_learning_diagnostic_metric_events(manifest, fallback_state, report,
                                           summary);
}

inline void
write_learning_report_snapshot_events(const job_manifest_t &manifest,
                                      job_events_probe_write_summary_t *summary,
                                      std::string_view report_text) {
  job_state_t fallback_state{};
  write_learning_report_snapshot_events(manifest, fallback_state, summary,
                                        report_text);
}

inline void append_learning_state_metric(
    const job_manifest_t &manifest, job_events_probe_write_summary_t *summary,
    std::string name, std::string value, std::string step,
    std::string anchor_begin, std::string anchor_end) {
  if (summary == nullptr || value.empty()) {
    return;
  }
  const auto event_kind = learning_metric_event_kind(manifest, name);
  if (event_kind.empty()) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = event_kind;
  event.phase = "learning_diagnostics";
  event.metric_name = std::move(name);
  event.metric_value = std::move(value);
  event.metric_unit = learning_metric_unit(event.metric_name);
  event.metric_source = "learning_diagnostics";
  event.series_id = event.event_kind + "." + event.metric_name;
  event.component_id = manifest.component_family_id.empty()
                           ? manifest.target_component_family_id
                           : manifest.component_family_id;
  event.step = std::move(step);
  event.anchor_begin = std::move(anchor_begin);
  event.anchor_end = std::move(anchor_end);
  append_record(manifest, summary, std::move(event));
}

inline void
append_learning_state_metric_events(const job_manifest_t &manifest,
                                    const job_state_t &state,
                                    job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_scalar_metrics) {
    return;
  }
  const std::string step = std::to_string(state.steps_completed);
  const std::string anchor_begin =
      std::to_string(state.resolved_anchor_index_begin);
  const std::string anchor_end =
      std::to_string(state.resolved_anchor_index_end);
  append_learning_state_metric(manifest, summary, "steps_attempted",
                               std::to_string(state.steps_attempted), step,
                               anchor_begin, anchor_end);
  append_learning_state_metric(manifest, summary, "steps_completed",
                               std::to_string(state.steps_completed), step,
                               anchor_begin, anchor_end);
  append_learning_state_metric(manifest, summary, "optimizer_steps",
                               std::to_string(state.optimizer_steps), step,
                               anchor_begin, anchor_end);
  if (std::isfinite(state.last_loss)) {
    append_learning_state_metric(manifest, summary, "last_loss",
                                 double_text(state.last_loss), step,
                                 anchor_begin, anchor_end);
  }
}

inline void
append_state_metric_events(const job_manifest_t &manifest,
                           const job_state_t &state,
                           job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_scalar_metrics) {
    return;
  }
  const std::string step = std::to_string(state.steps_completed);
  const std::string anchor_begin =
      std::to_string(state.resolved_anchor_index_begin);
  const std::string anchor_end =
      std::to_string(state.resolved_anchor_index_end);
  append_scalar_metric(manifest, summary, "accepted_anchor_count",
                       std::to_string(state.accepted_anchor_count), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "candidate_anchor_count",
                       std::to_string(state.candidate_anchor_count), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "accepted_anchor_fraction",
                       double_text(state.accepted_anchor_fraction), "fraction",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "steps_attempted",
                       std::to_string(state.steps_attempted), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "steps_completed",
                       std::to_string(state.steps_completed), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "skipped_batches",
                       std::to_string(state.skipped_batches), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "optimizer_steps",
                       std::to_string(state.optimizer_steps), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "wave_pulses_attempted",
                       std::to_string(state.wave_pulses_attempted), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "wave_pulses_completed",
                       std::to_string(state.wave_pulses_completed), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "wave_pulses_skipped",
                       std::to_string(state.wave_pulses_skipped), "count",
                       "job_state", step, anchor_begin, anchor_end);
  append_scalar_metric(manifest, summary, "wave_streamed_anchor_count",
                       std::to_string(state.wave_streamed_anchor_count),
                       "count", "job_state", step, anchor_begin, anchor_end);
  if (std::isfinite(state.last_loss)) {
    append_scalar_metric(manifest, summary, "last_loss",
                         double_text(state.last_loss), "loss", "job_state",
                         step, anchor_begin, anchor_end);
  }
  append_learning_state_metric_events(manifest, state, summary);
}

[[nodiscard]] inline bool is_finite_numeric_text(const std::string &value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return false;
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  const auto text = value.substr(begin, end - begin + 1);
  char *parse_end = nullptr;
  errno = 0;
  const double parsed = std::strtod(text.c_str(), &parse_end);
  return parse_end != text.c_str() && parse_end != nullptr &&
         *parse_end == '\0' && errno != ERANGE && std::isfinite(parsed);
}

inline void
append_report_metric_events(const job_manifest_t &manifest,
                            const job_state_t &state,
                            job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_report_metrics) {
    return;
  }
  if (state.report_path.empty() ||
      !std::filesystem::exists(state.report_path)) {
    return;
  }
  const auto report = job_layout::parse_kv_file(state.report_path);
  const std::string step = std::to_string(state.steps_completed);
  const std::string anchor_begin =
      std::to_string(state.resolved_anchor_index_begin);
  const std::string anchor_end =
      std::to_string(state.resolved_anchor_index_end);
  for (const auto &entry : report) {
    if (is_finite_numeric_text(entry.second)) {
      append_scalar_metric(manifest, summary, entry.first, entry.second, {},
                           "delegated_report", step, anchor_begin, anchor_end);
    }
  }
  append_learning_diagnostic_metric_events(manifest, state, report, summary);
}

[[nodiscard]] inline std::string
first_assignment_value(const std::filesystem::path &path,
                       const std::vector<const char *> &keys) {
  const auto values = job_layout::parse_kv_file(path);
  for (const char *key : keys) {
    const auto it = values.find(key);
    if (it != values.end() && !it->second.empty()) {
      return it->second;
    }
  }
  return {};
}

inline void append_artifact_if_exists(const job_manifest_t &manifest,
                                      job_events_probe_write_summary_t *summary,
                                      std::string artifact_kind,
                                      const std::filesystem::path &path,
                                      std::string artifact_schema = {}) {
  if (path.empty() || !std::filesystem::exists(path)) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = "job.artifact.published";
  event.phase = "artifact";
  event.artifact_kind = std::move(artifact_kind);
  event.artifact_path = path.string();
  event.artifact_schema = std::move(artifact_schema);
  event.artifact_digest =
      first_assignment_value(path, {"fact_digest", "report_digest",
                                    "checkpoint_digest", "artifact_digest"});
  append_record(manifest, summary, std::move(event));
}

inline void append_known_artifact_events(
    const std::filesystem::path &job_dir, const job_manifest_t &manifest,
    const job_state_t &state, job_events_probe_write_summary_t *summary) {
  if (summary == nullptr || !summary->config.emit_artifacts) {
    return;
  }
  append_artifact_if_exists(manifest, summary, "delegated_report",
                            state.report_path);
  append_artifact_if_exists(manifest, summary, "checkpoint",
                            state.checkpoint_path);
  append_artifact_if_exists(manifest, summary, "runtime_result_fact",
                            state.runtime_result_fact_path,
                            "kikijyeba.runtime.result.fact.v1");
  append_artifact_if_exists(manifest, summary, "runtime_checkpoint_io_fact",
                            state.runtime_checkpoint_io_fact_path,
                            "kikijyeba.runtime.checkpoint_io.fact.v1");
  append_artifact_if_exists(manifest, summary,
                            "runtime_health_measurement_fact",
                            state.runtime_health_measurement_fact_path,
                            "kikijyeba.runtime.health_measurement.fact.v1");
  append_artifact_if_exists(manifest, summary, "lattice_exposure_fact",
                            state.lattice_exposure_fact_path,
                            "kikijyeba.lattice.exposure.v1");
  append_artifact_if_exists(manifest, summary, "lattice_source_analytics_fact",
                            state.lattice_source_analytics_fact_path,
                            "kikijyeba.lattice.source_analytics.v1");
  append_artifact_if_exists(manifest, summary, "lattice_checkpoint_fact",
                            state.lattice_checkpoint_fact_path,
                            "kikijyeba.lattice.checkpoint.v1");
  append_artifact_if_exists(manifest, summary,
                            "lattice_component_training_update_fact",
                            job_dir / "lattice.component_training_update.fact");
  append_artifact_if_exists(manifest, summary, "lattice_target_transform_fact",
                            job_dir / "lattice.target_transform.fact",
                            "kikijyeba.lattice.target_transform.v1");
  for (const char *leaf :
       {"lattice.forecast_baseline.fact",
        "lattice.forecast_baseline.previous_value.fact",
        "lattice.forecast_baseline.zero_return.fact",
        "lattice.forecast_baseline.moving_average.fact",
        "lattice.forecast_baseline.last_valid_channel.fact"}) {
    append_artifact_if_exists(manifest, summary,
                              "lattice_forecast_baseline_fact", job_dir / leaf,
                              "kikijyeba.lattice.forecast_baseline.v1");
  }
  append_artifact_if_exists(manifest, summary, "lattice_forecast_eval_fact",
                            job_dir / "lattice.forecast_eval.fact",
                            "kikijyeba.lattice.forecast_eval.v1");
  append_artifact_if_exists(manifest, summary, "lattice_observer_belief_fact",
                            job_dir / "lattice.observer_belief.fact",
                            "kikijyeba.lattice.observer_belief.v1");
  append_artifact_if_exists(manifest, summary, "lattice_allocation_engine_fact",
                            job_dir / "lattice.allocation_engine.fact",
                            "kikijyeba.lattice.allocation_engine.v1");
  append_artifact_if_exists(manifest, summary,
                            "lattice_replay_environment_fact",
                            job_dir / "lattice.replay_environment.fact",
                            "kikijyeba.lattice.replay_environment.v1");
  append_artifact_if_exists(manifest, summary, "runtime_replay_artifact_index",
                            state.replay_artifact_path_index_path);
  append_artifact_if_exists(manifest, summary, "runtime_replay_batch_index",
                            state.replay_batch_index_path);
  append_artifact_if_exists(manifest, summary,
                            "runtime_replay_graph_anchor_edge_batch",
                            state.replay_graph_anchor_edge_batch_artifact_path);
  append_artifact_if_exists(manifest, summary,
                            "runtime_replay_observation_artifact_index",
                            state.replay_observation_artifact_index_path);
  append_artifact_if_exists(
      manifest, summary, "representation_edge_feature_probe",
      state.representation_edge_feature_probe_path,
      "kikijyeba.synthetic.representation_edge_feature_probe.v1");
  append_artifact_if_exists(
      manifest, summary, "mdn_edge_context_feature_probe",
      state.mdn_edge_context_feature_probe_path,
      "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1");
}

inline void append_warning_if_present(const job_manifest_t &manifest,
                                      job_events_probe_write_summary_t *summary,
                                      std::string phase, std::string message) {
  if (summary == nullptr || !summary->config.emit_warnings) {
    return;
  }
  if (message.empty()) {
    return;
  }
  job_events_probe_record_t event{};
  event.event_kind = "job.probe.warning";
  event.phase = std::move(phase);
  event.severity = "warning";
  event.message = std::move(message);
  append_record(manifest, summary, std::move(event));
}

inline void write_job_final_events(const std::filesystem::path &job_dir,
                                   const job_manifest_t &manifest,
                                   const job_state_t &state,
                                   job_events_probe_write_summary_t *summary) {
  if (summary != nullptr && summary->config.emit_lifecycle) {
    job_events_probe_record_t lifecycle{};
    lifecycle.event_kind = "job.lifecycle." + state.status;
    lifecycle.phase = "finished";
    lifecycle.status = state.status;
    lifecycle.message = state.error_message;
    lifecycle.severity = state.status == "failed" ? "error" : "info";
    append_record(manifest, summary, std::move(lifecycle));
  }

  append_state_metric_events(manifest, state, summary);
  append_report_metric_events(manifest, state, summary);
  append_known_artifact_events(job_dir, manifest, state, summary);
  append_warning_if_present(manifest, summary, "runtime_terminal_fact",
                            state.runtime_terminal_fact_error);
  append_warning_if_present(manifest, summary, "lattice_fact",
                            state.lattice_fact_error);
  append_warning_if_present(manifest, summary, "replay_artifact",
                            state.replay_artifact_error);
}

} // namespace cuwacunu::hero::runtime::job_events_probe
