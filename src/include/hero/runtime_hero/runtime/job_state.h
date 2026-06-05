// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "hero/runtime_hero/runtime/job_manifest.h"
#include "hero/runtime_hero/runtime/wave_plan.h"

namespace cuwacunu::hero::runtime {

struct job_state_t {
  std::string state_format{"kikijyeba.runtime.job_state.v1"};
  std::string job_id{};
  std::string job_stable_id{};
  std::string job_attempt_id{};
  std::size_t job_attempt_index{0};
  std::string job_attempt_policy{};
  std::string job_kind{};
  std::string status{"created"};
  std::string error_message{};
  std::string wave_id{};
  std::string target_component_family_id{};
  std::string wave_action{};
  std::string source_cursor_token{};
  std::string source_order_warning_level{"none"};
  std::string source_order_warnings{};
  std::string requested_source_key_begin{};
  std::string requested_source_key_end{};
  std::size_t resolved_anchor_index_begin{0};
  std::size_t resolved_anchor_index_end{0};
  std::size_t accepted_anchor_count{0};
  std::size_t candidate_anchor_count{0};
  double accepted_anchor_fraction{0.0};
  std::size_t skipped_outside_common_range{0};
  std::size_t skipped_missing_edge_coverage{0};
  std::size_t skipped_failed_fetch_probe{0};
  std::size_t duplicate_anchor_count{0};
  std::string anchor_domain_warning_level{"none"};
  std::string anchor_domain_warnings{};
  std::size_t source_input_length{0};
  std::size_t source_future_length{0};
  int64_t observed_source_row_begin{0};
  int64_t observed_source_row_end{0};
  int64_t target_source_row_begin{0};
  int64_t target_source_row_end{0};
  std::string source_footprint_precision{"graph_anchor_row_index_v1"};
  std::string observed_source_key_begin{};
  std::string observed_source_key_end{};
  std::string target_source_key_begin{};
  std::string target_source_key_end{};
  std::string source_key_footprint_precision{};
  std::string common_left_key{};
  std::string common_right_key{};
  std::string reference_left_key{};
  std::string reference_right_key{};
  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};
  int64_t wave_pulses_attempted{0};
  int64_t wave_pulses_completed{0};
  int64_t wave_pulses_skipped{0};
  int64_t wave_streamed_anchor_count{0};
  std::string wave_first_anchor_key{};
  std::string wave_last_anchor_key{};
  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  std::string checkpoint_path{};
  bool report_written{false};
  int64_t report_write_count{0};
  std::string report_path{};
  bool lattice_exposure_fact_written{false};
  std::string lattice_exposure_fact_path{};
  bool lattice_source_analytics_fact_written{false};
  std::string lattice_source_analytics_fact_path{};
  bool lattice_checkpoint_fact_written{false};
  std::string lattice_checkpoint_fact_path{};
  std::string lattice_fact_error{};
  bool runtime_result_fact_written{false};
  std::string runtime_result_fact_path{};
  bool runtime_checkpoint_io_fact_written{false};
  std::string runtime_checkpoint_io_fact_path{};
  bool runtime_health_measurement_fact_written{false};
  std::string runtime_health_measurement_fact_path{};
  std::string runtime_terminal_fact_error{};
  bool replay_artifacts_written{false};
  std::string replay_artifact_path_index_path{};
  std::string replay_batch_index_path{};
  std::string replay_graph_anchor_edge_batch_artifact_path{};
  std::string replay_observation_artifact_index_path{};
  std::string replay_artifact_error{};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream out;
    out << "state_format=" << state_format << "\n";
    out << "job_id=" << job_id << "\n";
    out << "job_stable_id=" << job_stable_id << "\n";
    out << "job_attempt_id=" << job_attempt_id << "\n";
    out << "job_attempt_index=" << job_attempt_index << "\n";
    out << "job_attempt_policy=" << job_attempt_policy << "\n";
    out << "job_kind=" << job_kind << "\n";
    out << "status=" << status << "\n";
    out << "error_message=" << error_message << "\n";
    out << "wave_id=" << wave_id << "\n";
    out << "target_component_family_id=" << target_component_family_id << "\n";
    out << "wave_action=" << wave_action << "\n";
    out << "source_cursor_token=" << source_cursor_token << "\n";
    out << "source_order_warning_level=" << source_order_warning_level << "\n";
    out << "source_order_warnings=" << source_order_warnings << "\n";
    out << "requested_source_key_begin=" << requested_source_key_begin << "\n";
    out << "requested_source_key_end=" << requested_source_key_end << "\n";
    out << "resolved_anchor_index_begin=" << resolved_anchor_index_begin
        << "\n";
    out << "resolved_anchor_index_end=" << resolved_anchor_index_end << "\n";
    out << "accepted_anchor_count=" << accepted_anchor_count << "\n";
    out << "candidate_anchor_count=" << candidate_anchor_count << "\n";
    out << "accepted_anchor_fraction=" << accepted_anchor_fraction << "\n";
    out << "skipped_outside_common_range=" << skipped_outside_common_range
        << "\n";
    out << "skipped_missing_edge_coverage=" << skipped_missing_edge_coverage
        << "\n";
    out << "skipped_failed_fetch_probe=" << skipped_failed_fetch_probe << "\n";
    out << "duplicate_anchor_count=" << duplicate_anchor_count << "\n";
    out << "anchor_domain_warning_level=" << anchor_domain_warning_level
        << "\n";
    out << "anchor_domain_warnings=" << anchor_domain_warnings << "\n";
    out << "source_input_length=" << source_input_length << "\n";
    out << "source_future_length=" << source_future_length << "\n";
    out << "observed_source_row_begin=" << observed_source_row_begin << "\n";
    out << "observed_source_row_end=" << observed_source_row_end << "\n";
    out << "target_source_row_begin=" << target_source_row_begin << "\n";
    out << "target_source_row_end=" << target_source_row_end << "\n";
    out << "source_footprint_precision=" << source_footprint_precision << "\n";
    out << "observed_source_key_begin=" << observed_source_key_begin << "\n";
    out << "observed_source_key_end=" << observed_source_key_end << "\n";
    out << "target_source_key_begin=" << target_source_key_begin << "\n";
    out << "target_source_key_end=" << target_source_key_end << "\n";
    out << "source_key_footprint_precision=" << source_key_footprint_precision
        << "\n";
    out << "common_left_key=" << common_left_key << "\n";
    out << "common_right_key=" << common_right_key << "\n";
    out << "reference_left_key=" << reference_left_key << "\n";
    out << "reference_right_key=" << reference_right_key << "\n";
    out << "steps_attempted=" << steps_attempted << "\n";
    out << "steps_completed=" << steps_completed << "\n";
    out << "skipped_batches=" << skipped_batches << "\n";
    out << "optimizer_steps=" << optimizer_steps << "\n";
    out << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    out << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    out << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    out << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    out << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    out << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    out << "last_loss=" << last_loss << "\n";
    out << "checkpoint_written=" << (checkpoint_written ? "true" : "false")
        << "\n";
    out << "checkpoint_write_count=" << checkpoint_write_count << "\n";
    out << "checkpoint_path=" << checkpoint_path << "\n";
    out << "report_written=" << (report_written ? "true" : "false") << "\n";
    out << "report_write_count=" << report_write_count << "\n";
    out << "report_path=" << report_path << "\n";
    out << "lattice_exposure_fact_written="
        << (lattice_exposure_fact_written ? "true" : "false") << "\n";
    out << "lattice_exposure_fact_path=" << lattice_exposure_fact_path << "\n";
    out << "lattice_source_analytics_fact_written="
        << (lattice_source_analytics_fact_written ? "true" : "false") << "\n";
    out << "lattice_source_analytics_fact_path="
        << lattice_source_analytics_fact_path << "\n";
    out << "lattice_checkpoint_fact_written="
        << (lattice_checkpoint_fact_written ? "true" : "false") << "\n";
    out << "lattice_checkpoint_fact_path=" << lattice_checkpoint_fact_path
        << "\n";
    out << "lattice_fact_error=" << lattice_fact_error << "\n";
    out << "runtime_result_fact_written="
        << (runtime_result_fact_written ? "true" : "false") << "\n";
    out << "runtime_result_fact_path=" << runtime_result_fact_path << "\n";
    out << "runtime_checkpoint_io_fact_written="
        << (runtime_checkpoint_io_fact_written ? "true" : "false") << "\n";
    out << "runtime_checkpoint_io_fact_path=" << runtime_checkpoint_io_fact_path
        << "\n";
    out << "runtime_health_measurement_fact_written="
        << (runtime_health_measurement_fact_written ? "true" : "false") << "\n";
    out << "runtime_health_measurement_fact_path="
        << runtime_health_measurement_fact_path << "\n";
    out << "runtime_terminal_fact_error=" << runtime_terminal_fact_error
        << "\n";
    out << "replay_artifacts_written="
        << (replay_artifacts_written ? "true" : "false") << "\n";
    out << "replay_artifact_path_index_path=" << replay_artifact_path_index_path
        << "\n";
    out << "replay_batch_index_path=" << replay_batch_index_path << "\n";
    out << "replay_graph_anchor_edge_batch_artifact_path="
        << replay_graph_anchor_edge_batch_artifact_path << "\n";
    out << "replay_observation_artifact_index_path="
        << replay_observation_artifact_index_path << "\n";
    out << "replay_artifact_error=" << replay_artifact_error << "\n";
    return out.str();
  }
};

inline job_state_t make_initial_job_state(const job_manifest_t &manifest,
                                          const wave_plan_t &wave_plan) {
  job_state_t out{};
  out.job_id = manifest.job_id;
  out.job_stable_id = manifest.job_stable_id;
  out.job_attempt_id = manifest.job_attempt_id;
  out.job_attempt_index = manifest.job_attempt_index;
  out.job_attempt_policy = manifest.job_attempt_policy;
  out.job_kind = manifest.job_kind;
  out.wave_id = wave_plan.wave_id;
  out.target_component_family_id = wave_plan.target_component_family_id;
  out.wave_action = wave_plan.action;
  out.source_cursor_token = wave_plan.source_cursor_token;
  out.source_order_warning_level = wave_plan.source_order_warning_level;
  out.source_order_warnings = wave_plan.source_order_warnings;
  if (wave_plan.requested_source_key_begin.has_value()) {
    out.requested_source_key_begin =
        std::to_string(*wave_plan.requested_source_key_begin);
  }
  if (wave_plan.requested_source_key_end.has_value()) {
    out.requested_source_key_end =
        std::to_string(*wave_plan.requested_source_key_end);
  }
  out.resolved_anchor_index_begin = wave_plan.resolved_anchor_index_begin;
  out.resolved_anchor_index_end = wave_plan.resolved_anchor_index_end;
  out.accepted_anchor_count = wave_plan.accepted_anchor_count;
  out.candidate_anchor_count = wave_plan.candidate_anchor_count;
  out.accepted_anchor_fraction = wave_plan.accepted_anchor_fraction;
  out.skipped_outside_common_range = wave_plan.skipped_outside_common_range;
  out.skipped_missing_edge_coverage = wave_plan.skipped_missing_edge_coverage;
  out.skipped_failed_fetch_probe = wave_plan.skipped_failed_fetch_probe;
  out.duplicate_anchor_count = wave_plan.duplicate_anchor_count;
  out.anchor_domain_warning_level = wave_plan.anchor_domain_warning_level;
  out.anchor_domain_warnings = wave_plan.anchor_domain_warnings;
  out.source_input_length = wave_plan.source_input_length;
  out.source_future_length = wave_plan.source_future_length;
  out.observed_source_row_begin = wave_plan.observed_source_row_begin;
  out.observed_source_row_end = wave_plan.observed_source_row_end;
  out.target_source_row_begin = wave_plan.target_source_row_begin;
  out.target_source_row_end = wave_plan.target_source_row_end;
  out.source_footprint_precision = wave_plan.source_footprint_precision;
  out.observed_source_key_begin = wave_plan.observed_source_key_begin;
  out.observed_source_key_end = wave_plan.observed_source_key_end;
  out.target_source_key_begin = wave_plan.target_source_key_begin;
  out.target_source_key_end = wave_plan.target_source_key_end;
  out.source_key_footprint_precision = wave_plan.source_key_footprint_precision;
  out.common_left_key = wave_plan.common_left_key;
  out.common_right_key = wave_plan.common_right_key;
  out.reference_left_key = wave_plan.reference_left_key;
  out.reference_right_key = wave_plan.reference_right_key;
  out.wave_first_anchor_key = wave_plan.first_anchor_key;
  out.wave_last_anchor_key = wave_plan.last_anchor_key;
  return out;
}

template <typename TrainingReportT>
[[nodiscard]] inline job_state_t
make_completed_job_state(const job_manifest_t &manifest,
                         const wave_plan_t &wave_plan,
                         const TrainingReportT &report, bool dry_run) {
  auto out = make_initial_job_state(manifest, wave_plan);
  out.status = dry_run ? "dry_run" : "completed";
  out.steps_attempted = report.steps_attempted;
  out.steps_completed = report.steps_completed;
  out.skipped_batches = report.skipped_batches;
  out.optimizer_steps = report.optimizer_steps;
  out.wave_pulses_attempted = report.wave_pulses_attempted;
  out.wave_pulses_completed = report.wave_pulses_completed;
  out.wave_pulses_skipped = report.wave_pulses_skipped;
  out.wave_streamed_anchor_count = report.wave_streamed_anchor_count;
  // Wave key bounds are Runtime plan identity, not component stream order.
  // Train-mode samplers may visit anchors randomly, so component report keys
  // must not override the canonical planned range recorded in the manifest.
  out.last_loss = report.last_loss;
  out.checkpoint_written = report.checkpoint_written;
  out.checkpoint_write_count = report.checkpoint_write_count;
  out.checkpoint_path = report.checkpoint_path;
  out.report_written = report.report_written;
  out.report_write_count = report.report_write_count;
  out.report_path = report.report_path;
  return out;
}

[[nodiscard]] inline job_state_t
make_failed_job_state(const job_manifest_t &manifest,
                      const wave_plan_t &wave_plan,
                      const std::string &error_message) {
  auto out = make_initial_job_state(manifest, wave_plan);
  out.status = "failed";
  out.error_message = error_message;
  return out;
}

inline void write_job_state_file(const std::filesystem::path &path,
                                 const job_state_t &state) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] failed to open job state path: " +
        path.string());
  }
  out << state.to_text();
}

} // namespace cuwacunu::hero::runtime
