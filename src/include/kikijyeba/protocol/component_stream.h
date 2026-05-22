// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/settings/wave.h"
#include "ujcamei/source/retrieval/dataloader/source_cursor.h"

namespace cuwacunu::kikijyeba::protocol {

namespace component_stream_detail {

[[nodiscard]] inline std::string trim_stream_token(std::string text) {
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.front())) != 0) {
    text.erase(text.begin());
  }
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.back())) != 0) {
    text.pop_back();
  }
  return text;
}

} // namespace component_stream_detail

struct component_stream_wave_t {
  std::string wave_id{"unbound_wave"};
  std::string mode_text{"run"};
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"wave_batch"};
  std::string source_range_policy{"all"};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
};

struct component_stream_identity_t {
  std::string component_family{};
  std::string component_id{};
  std::string assembly_token{};
  std::string dock_binding_token{};
  std::string graph_order_fingerprint{};
};

struct component_stream_cursor_t {
  std::string wave_id{};
  std::string wave_mode{};
  std::string source_cursor_kind{};
  std::string source_cursor_scope{};
  std::string source_range_policy{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::string batch_cursor_token{};
};

struct component_stream_report_t {
  component_stream_identity_t identity{};
  component_stream_cursor_t cursor{};
  cuwacunu::kikijyeba::lattice::runtime_report::
      runtime_report_mode_t runtime_report_mode{
          cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::normal};
  double elapsed_ms{std::numeric_limits<double>::quiet_NaN()};
  std::uint64_t input_count{0};
  std::uint64_t output_count{0};
  std::uint64_t warning_count{0};
  std::string runtime_lls{};

  [[nodiscard]] bool runtime_lls_emitted() const {
    return !runtime_lls.empty();
  }
};

struct component_stream_plan_step_t {
  std::string name{};
  std::string component_family{};
  std::string component_id{};
  std::string assembly_token{};
  std::string dock_binding_token{};
  std::string input_batch{};
  std::string output_batch{};
};

struct component_stream_plan_t {
  std::vector<component_stream_plan_step_t> steps{};

  [[nodiscard]] std::string summary() const {
    std::ostringstream out;
    for (std::size_t i = 0; i < steps.size(); ++i) {
      if (i != 0) {
        out << " -> ";
      }
      const auto &step = steps[i];
      out << step.name << "(" << step.component_family << ":"
          << step.component_id << ")";
    }
    return out.str();
  }
};

inline void validate_component_stream_identity(
    const component_stream_identity_t &identity) {
  if (component_stream_detail::trim_stream_token(identity.component_family)
          .empty() ||
      component_stream_detail::trim_stream_token(identity.component_id)
          .empty() ||
      component_stream_detail::trim_stream_token(identity.assembly_token)
          .empty() ||
      component_stream_detail::trim_stream_token(
          identity.graph_order_fingerprint)
          .empty()) {
    throw std::runtime_error(
        "[component_stream] component family/id, assembly token, and graph "
        "fingerprint are required");
  }
}

inline void
validate_component_stream_cursor(const component_stream_cursor_t &cursor) {
  if (component_stream_detail::trim_stream_token(cursor.wave_id).empty() ||
      component_stream_detail::trim_stream_token(cursor.wave_mode).empty() ||
      component_stream_detail::trim_stream_token(cursor.source_cursor_kind)
          .empty() ||
      component_stream_detail::trim_stream_token(cursor.source_cursor_scope)
          .empty() ||
      component_stream_detail::trim_stream_token(cursor.source_range_policy)
          .empty() ||
      component_stream_detail::trim_stream_token(cursor.batch_cursor_token)
          .empty()) {
    throw std::runtime_error(
        "[component_stream] wave id/mode, source cursor kind/scope, source "
        "range policy, and batch cursor token are required");
  }
}

[[nodiscard]] inline component_stream_wave_t
component_stream_wave_from_settings(
    const cuwacunu::kikijyeba::settings::wave_settings_t &settings) {
  return component_stream_wave_t{
      .wave_id = settings.wave_id,
      .mode_text = settings.mode_text,
      .source_cursor_kind = settings.source_cursor_kind,
      .source_cursor_scope = settings.source_cursor_scope,
      .source_range_policy =
          cuwacunu::kikijyeba::settings::source_range_policy_name(
              settings.source_range_policy),
      .anchor_index_begin = settings.anchor_index_begin,
      .anchor_index_end = settings.anchor_index_end,
  };
}

template <typename KeyT>
[[nodiscard]] inline component_stream_cursor_t make_component_stream_cursor(
    const component_stream_wave_t &wave,
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_cursor_t<KeyT> &cursor) {
  return component_stream_cursor_t{
      .wave_id = wave.wave_id,
      .wave_mode = wave.mode_text,
      .source_cursor_kind = wave.source_cursor_kind,
      .source_cursor_scope = wave.source_cursor_scope,
      .source_range_policy = wave.source_range_policy,
      .anchor_index_begin = wave.anchor_index_begin,
      .anchor_index_end = wave.anchor_index_end,
      .batch_cursor_token = cuwacunu::kikijyeba::lattice::
          runtime_report::require_graph_anchor_cursor_token(cursor),
  };
}

[[nodiscard]] inline component_stream_report_t make_component_stream_report(
    component_stream_identity_t identity, component_stream_cursor_t cursor,
    cuwacunu::kikijyeba::lattice::runtime_report::
        runtime_report_mode_t runtime_report_mode,
    double elapsed_ms = std::numeric_limits<double>::quiet_NaN(),
    std::uint64_t input_count = 0, std::uint64_t output_count = 0,
    std::uint64_t warning_count = 0) {
  validate_component_stream_identity(identity);
  validate_component_stream_cursor(cursor);
  return component_stream_report_t{
      .identity = std::move(identity),
      .cursor = std::move(cursor),
      .runtime_report_mode = runtime_report_mode,
      .elapsed_ms = elapsed_ms,
      .input_count = input_count,
      .output_count = output_count,
      .warning_count = warning_count,
  };
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::
    runtime_report::runtime_lls_document_t
    make_component_stream_runtime_document(
        const std::string &schema, const component_stream_report_t &report) {
  namespace lls =
      cuwacunu::kikijyeba::lattice::runtime_report;
  validate_component_stream_identity(report.identity);
  validate_component_stream_cursor(report.cursor);

  auto document =
      lls::make_component_runtime_document(lls::component_runtime_identity_t{
          .schema = schema,
          .component_family = report.identity.component_family,
          .component_id = report.identity.component_id,
          .batch_cursor_token = report.cursor.batch_cursor_token,
          .graph_order_fingerprint = report.identity.graph_order_fingerprint,
      });
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "assembly_token", report.identity.assembly_token));
  if (!component_stream_detail::trim_stream_token(
           report.identity.dock_binding_token)
           .empty()) {
    document.entries.push_back(lls::make_component_runtime_lls_string_entry(
        "dock_binding_token", report.identity.dock_binding_token));
  }
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "wave_id", report.cursor.wave_id));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "wave_mode", report.cursor.wave_mode));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "source_cursor_kind", report.cursor.source_cursor_kind));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "source_cursor_scope", report.cursor.source_cursor_scope));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "source_range_policy", report.cursor.source_range_policy));
  if (report.cursor.anchor_index_begin.has_value()) {
    document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
        "requested_anchor_index_begin",
        static_cast<std::uint64_t>(*report.cursor.anchor_index_begin)));
  }
  if (report.cursor.anchor_index_end.has_value()) {
    document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
        "requested_anchor_index_end",
        static_cast<std::uint64_t>(*report.cursor.anchor_index_end)));
  }
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "runtime_report_mode",
      cuwacunu::kikijyeba::settings::runtime_report_mode_name(
          report.runtime_report_mode)));
  if (std::isfinite(report.elapsed_ms)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "elapsed_ms", report.elapsed_ms, "(0,+inf)"));
  }
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "stream_input_count", report.input_count));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "stream_output_count", report.output_count));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "stream_warning_count", report.warning_count));
  return document;
}

} // namespace cuwacunu::kikijyeba::protocol
