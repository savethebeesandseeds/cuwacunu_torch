// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "kikijyeba/settings/wave.h"

namespace cuwacunu::kikijyeba::runtime {

struct wave_plan_t {
  std::string wave_id{};
  std::string target_component_family_id{};
  std::string action{};
  std::string mode_text{};
  std::string source_cursor_kind{};
  std::string source_cursor_scope{};
  std::string source_range_policy{};
  std::string source_order_policy{};
  bool source_order_policy_explicit{false};
  std::string source_order_warning_level{"none"};
  std::string source_order_warnings{};
  std::optional<std::size_t> requested_anchor_index_begin{std::nullopt};
  std::optional<std::size_t> requested_anchor_index_end{std::nullopt};
  std::optional<std::int64_t> requested_source_key_begin{std::nullopt};
  std::optional<std::int64_t> requested_source_key_end{std::nullopt};
  std::size_t resolved_anchor_index_begin{0};
  std::size_t resolved_anchor_index_end{0};
  std::size_t accepted_anchor_count{0};
  std::size_t candidate_anchor_count{0};
  std::size_t skipped_outside_common_range{0};
  std::size_t skipped_missing_edge_coverage{0};
  std::size_t skipped_failed_fetch_probe{0};
  std::size_t duplicate_anchor_count{0};
  double accepted_anchor_fraction{std::numeric_limits<double>::quiet_NaN()};
  std::string anchor_domain_warning_level{"none"};
  std::string anchor_domain_warnings{};
  std::string source_cursor_token{};
  std::string first_anchor_key{};
  std::string last_anchor_key{};
  std::string common_left_key{};
  std::string common_right_key{};
  std::string reference_left_key{};
  std::string reference_right_key{};
  std::size_t source_input_length{0};
  std::size_t source_future_length{0};
  std::int64_t observed_source_row_begin{0};
  std::int64_t observed_source_row_end{0};
  std::int64_t target_source_row_begin{0};
  std::int64_t target_source_row_end{0};
  std::string source_footprint_precision{"graph_anchor_row_index_v1"};
  std::string observed_source_key_begin{};
  std::string observed_source_key_end{};
  std::string target_source_key_begin{};
  std::string target_source_key_end{};
  std::string source_key_footprint_precision{};

  [[nodiscard]] std::size_t resolved_anchor_count() const {
    return resolved_anchor_index_end >= resolved_anchor_index_begin
               ? resolved_anchor_index_end - resolved_anchor_index_begin
               : 0;
  }

  [[nodiscard]] std::string to_text() const {
    std::ostringstream out;
    out << "wave_id=" << wave_id << "\n";
    out << "target_component_family_id=" << target_component_family_id << "\n";
    out << "action=" << action << "\n";
    out << "mode_text=" << mode_text << "\n";
    out << "source_cursor_kind=" << source_cursor_kind << "\n";
    out << "source_cursor_scope=" << source_cursor_scope << "\n";
    out << "source_range_policy=" << source_range_policy << "\n";
    out << "source_order_policy=" << source_order_policy << "\n";
    out << "source_order_policy_explicit="
        << (source_order_policy_explicit ? "true" : "false") << "\n";
    out << "source_order_warning_level=" << source_order_warning_level << "\n";
    out << "source_order_warnings=" << source_order_warnings << "\n";
    out << "requested_anchor_index_begin=";
    if (requested_anchor_index_begin.has_value()) {
      out << *requested_anchor_index_begin;
    } else {
      out << -1;
    }
    out << "\nrequested_anchor_index_end=";
    if (requested_anchor_index_end.has_value()) {
      out << *requested_anchor_index_end;
    } else {
      out << -1;
    }
    out << "\nrequested_source_key_begin=";
    if (requested_source_key_begin.has_value()) {
      out << *requested_source_key_begin;
    }
    out << "\nrequested_source_key_end=";
    if (requested_source_key_end.has_value()) {
      out << *requested_source_key_end;
    }
    out << "\nresolved_anchor_index_begin=" << resolved_anchor_index_begin
        << "\n";
    out << "resolved_anchor_index_end=" << resolved_anchor_index_end << "\n";
    out << "resolved_anchor_count=" << resolved_anchor_count() << "\n";
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
    out << "source_cursor_token=" << source_cursor_token << "\n";
    out << "first_anchor_key=" << first_anchor_key << "\n";
    out << "last_anchor_key=" << last_anchor_key << "\n";
    out << "common_left_key=" << common_left_key << "\n";
    out << "common_right_key=" << common_right_key << "\n";
    out << "reference_left_key=" << reference_left_key << "\n";
    out << "reference_right_key=" << reference_right_key << "\n";
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
    return out.str();
  }
};

namespace wave_plan_detail {

template <typename KeyT>
[[nodiscard]] inline std::string key_to_string(const KeyT &value) {
  std::ostringstream out;
  out << value;
  return out.str();
}

template <typename KeyT>
[[nodiscard]] inline std::string
optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  return key_to_string(*value);
}

inline void append_warning_token(std::string &warnings,
                                 const std::string &token) {
  if (token.empty()) {
    return;
  }
  if (!warnings.empty()) {
    warnings.push_back('|');
  }
  warnings.append(token);
}

inline void populate_anchor_domain_warning(wave_plan_t &plan) {
  plan.accepted_anchor_fraction =
      plan.candidate_anchor_count == 0
          ? 0.0
          : static_cast<double>(plan.accepted_anchor_count) /
                static_cast<double>(plan.candidate_anchor_count);
  plan.anchor_domain_warning_level = "none";
  plan.anchor_domain_warnings.clear();
  if (plan.accepted_anchor_count == 0) {
    plan.anchor_domain_warning_level = "error";
    append_warning_token(plan.anchor_domain_warnings,
                         "accepted_anchor_count_zero");
    return;
  }
  if (plan.accepted_anchor_fraction < 0.80) {
    plan.anchor_domain_warning_level = "warning";
    append_warning_token(plan.anchor_domain_warnings,
                         "accepted_anchor_fraction_below_0.80");
  }
  if (plan.skipped_failed_fetch_probe > 0) {
    plan.anchor_domain_warning_level = "warning";
    append_warning_token(plan.anchor_domain_warnings,
                         "skipped_failed_fetch_probe_nonzero");
  }
  const double missing_edge_fraction =
      plan.candidate_anchor_count == 0
          ? 0.0
          : static_cast<double>(plan.skipped_missing_edge_coverage) /
                static_cast<double>(plan.candidate_anchor_count);
  if (missing_edge_fraction > 0.10) {
    plan.anchor_domain_warning_level = "warning";
    append_warning_token(plan.anchor_domain_warnings,
                         "skipped_missing_edge_coverage_fraction_above_0.10");
  }
}

inline void populate_source_order_warning(
    wave_plan_t &plan,
    const cuwacunu::kikijyeba::settings::wave_settings_t &settings) {
  plan.source_order_warning_level = "none";
  plan.source_order_warnings.clear();
  const std::string warning =
      cuwacunu::kikijyeba::settings::source_order_warning_token(settings);
  if (warning.empty()) {
    return;
  }
  plan.source_order_warning_level = "warning";
  append_warning_token(plan.source_order_warnings, warning);
}

template <typename KeyT>
[[nodiscard]] inline std::string key_offset_to_string(const KeyT &value,
                                                      const KeyT &step,
                                                      std::int64_t multiples) {
  std::ostringstream out;
  const auto value_i = static_cast<std::int64_t>(value);
  const auto step_i = static_cast<std::int64_t>(step);
  out << (value_i + step_i * multiples);
  return out.str();
}

template <typename CursorReportT>
inline void populate_resolved_anchor_keys(wave_plan_t &plan,
                                          const CursorReportT &cursor_report) {
  if (plan.resolved_anchor_index_begin >= plan.resolved_anchor_index_end) {
    plan.first_anchor_key.clear();
    plan.last_anchor_key.clear();
    return;
  }
  if (plan.resolved_anchor_index_end > cursor_report.anchor_keys.size()) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] resolved wave range exceeds cursor anchor "
        "keys");
  }
  plan.first_anchor_key = key_to_string(
      cursor_report.anchor_keys.at(plan.resolved_anchor_index_begin));
  plan.last_anchor_key = key_to_string(
      cursor_report.anchor_keys.at(plan.resolved_anchor_index_end - 1));
}

template <typename CursorReportT>
inline void populate_source_key_footprint(wave_plan_t &plan,
                                          const CursorReportT &cursor_report) {
  plan.observed_source_key_begin.clear();
  plan.observed_source_key_end.clear();
  plan.target_source_key_begin.clear();
  plan.target_source_key_end.clear();
  plan.source_key_footprint_precision.clear();
  if (plan.resolved_anchor_index_begin >= plan.resolved_anchor_index_end ||
      !cursor_report.reference_key_step.has_value()) {
    return;
  }
  if (plan.resolved_anchor_index_end > cursor_report.anchor_keys.size()) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] resolved wave range exceeds cursor anchor "
        "keys");
  }
  const auto first_key =
      cursor_report.anchor_keys.at(plan.resolved_anchor_index_begin);
  const auto last_key =
      cursor_report.anchor_keys.at(plan.resolved_anchor_index_end - 1);
  const auto step = *cursor_report.reference_key_step;
  const auto input_back =
      plan.source_input_length > 0
          ? static_cast<std::int64_t>(plan.source_input_length - 1)
          : 0;
  const auto future_length =
      static_cast<std::int64_t>(plan.source_future_length);

  plan.observed_source_key_begin =
      key_offset_to_string(first_key, step, -input_back);
  plan.observed_source_key_end = key_offset_to_string(last_key, step, 1);
  plan.target_source_key_begin = key_offset_to_string(first_key, step, 1);
  plan.target_source_key_end =
      key_offset_to_string(last_key, step, future_length + 1);
  plan.source_key_footprint_precision = "graph_anchor_key_window_v1";
}

} // namespace wave_plan_detail

inline void populate_source_footprint(wave_plan_t &plan) {
  const auto begin =
      static_cast<std::int64_t>(plan.resolved_anchor_index_begin);
  const auto end = static_cast<std::int64_t>(plan.resolved_anchor_index_end);
  const auto input_length = static_cast<std::int64_t>(plan.source_input_length);
  const auto future_length =
      static_cast<std::int64_t>(plan.source_future_length);
  const auto input_back = input_length > 0 ? input_length - 1 : 0;
  plan.observed_source_row_begin = begin - input_back;
  plan.observed_source_row_end = end;
  plan.target_source_row_begin = begin + 1;
  plan.target_source_row_end = end + future_length;
  plan.source_footprint_precision = "graph_anchor_row_index_v1";
}

template <typename GraphSourceT>
[[nodiscard]] inline wave_plan_t
make_wave_plan(const cuwacunu::kikijyeba::settings::wave_settings_t &settings,
               const GraphSourceT &source) {
  const auto cursor_report = source.cursor_report();
  wave_plan_t out{};
  out.wave_id = settings.wave_id;
  out.target_component_family_id =
      cuwacunu::kikijyeba::settings::wave_target_name(settings.target);
  out.action = cuwacunu::kikijyeba::settings::wave_action_name(settings.action);
  out.mode_text = settings.mode_text;
  out.source_cursor_kind = settings.source_cursor_kind;
  out.source_cursor_scope = settings.source_cursor_scope;
  out.source_range_policy =
      cuwacunu::kikijyeba::settings::source_range_policy_name(
          settings.source_range_policy);
  out.source_order_policy =
      cuwacunu::kikijyeba::settings::source_order_policy_name(
          settings.source_order_policy);
  out.source_order_policy_explicit = settings.source_order_policy_explicit;
  out.requested_anchor_index_begin = settings.anchor_index_begin;
  out.requested_anchor_index_end = settings.anchor_index_end;
  out.requested_source_key_begin = settings.source_key_begin;
  out.requested_source_key_end = settings.source_key_end;
  out.accepted_anchor_count = cursor_report.accepted_anchor_count;
  out.candidate_anchor_count = cursor_report.candidate_anchor_count;
  out.skipped_outside_common_range = cursor_report.skipped_outside_common_range;
  out.skipped_missing_edge_coverage =
      cursor_report.skipped_missing_edge_coverage;
  out.skipped_failed_fetch_probe = cursor_report.skipped_failed_fetch_probe;
  out.duplicate_anchor_count = cursor_report.duplicate_anchor_count;
  wave_plan_detail::populate_anchor_domain_warning(out);
  wave_plan_detail::populate_source_order_warning(out, settings);
  out.source_cursor_token = cursor_report.cursor_token();
  out.source_input_length = cursor_report.max_input_length;
  out.source_future_length = cursor_report.max_future_length;
  out.common_left_key =
      wave_plan_detail::optional_key_to_string(cursor_report.common_left_key);
  out.common_right_key =
      wave_plan_detail::optional_key_to_string(cursor_report.common_right_key);
  out.reference_left_key = wave_plan_detail::optional_key_to_string(
      cursor_report.reference_left_key);
  out.reference_right_key = wave_plan_detail::optional_key_to_string(
      cursor_report.reference_right_key);

  const auto resolved =
      cuwacunu::kikijyeba::settings::resolve_source_range_to_anchor_indices(
          settings, cursor_report);
  out.resolved_anchor_index_begin = resolved.anchor_index_begin;
  out.resolved_anchor_index_end = resolved.anchor_index_end;
  populate_source_footprint(out);
  wave_plan_detail::populate_resolved_anchor_keys(out, cursor_report);
  wave_plan_detail::populate_source_key_footprint(out, cursor_report);
  return out;
}

} // namespace cuwacunu::kikijyeba::runtime
