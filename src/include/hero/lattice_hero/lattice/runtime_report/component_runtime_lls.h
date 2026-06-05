// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "hero/lattice_hero/lattice/runtime_report/runtime_lls.h"
#include "ujcamei/source/retrieval/dataloader/source_cursor.h"

namespace cuwacunu::hero::lattice::runtime_report {

enum class runtime_report_mode_t {
  normal,
  debug,
};

[[nodiscard]] inline bool runtime_report_enabled(runtime_report_mode_t mode) {
  return mode == runtime_report_mode_t::debug;
}

[[nodiscard]] inline runtime_lls_entry_t
make_component_runtime_lls_entry(std::string key, std::string value,
                                 std::string declared_type,
                                 std::string declared_domain = {}) {
  return {
      .key = std::move(key),
      .declared_domain = std::move(declared_domain),
      .declared_type = std::move(declared_type),
      .value = std::move(value),
  };
}

[[nodiscard]] inline runtime_lls_entry_t
make_component_runtime_lls_string_entry(std::string key, std::string value) {
  return make_component_runtime_lls_entry(std::move(key), std::move(value),
                                          "str");
}

[[nodiscard]] inline runtime_lls_entry_t
make_component_runtime_lls_bool_entry(std::string key, bool value) {
  return make_component_runtime_lls_entry(std::move(key),
                                          value ? "true" : "false", "bool");
}

[[nodiscard]] inline runtime_lls_entry_t make_component_runtime_lls_int_entry(
    std::string key, std::int64_t value,
    std::string declared_domain = "(-inf,+inf)") {
  return make_component_runtime_lls_entry(std::move(key), std::to_string(value),
                                          "int", std::move(declared_domain));
}

[[nodiscard]] inline runtime_lls_entry_t make_component_runtime_lls_uint_entry(
    std::string key, std::uint64_t value,
    std::string declared_domain = "[0,+inf)") {
  return make_component_runtime_lls_entry(std::move(key), std::to_string(value),
                                          "uint", std::move(declared_domain));
}

[[nodiscard]] inline runtime_lls_entry_t
make_component_runtime_lls_double_entry(
    std::string key, double value,
    std::string declared_domain = "(-inf,+inf)") {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(12) << value;
  return make_component_runtime_lls_entry(std::move(key), oss.str(), "double",
                                          std::move(declared_domain));
}

[[nodiscard]] inline std::string
emit_component_runtime_lls_canonical(const runtime_lls_document_t &document) {
  std::ostringstream out;
  std::unordered_set<std::string> seen_keys;
  for (const auto &entry : document.entries) {
    if (entry.key.empty() || entry.declared_type.empty()) {
      throw std::runtime_error(
          "[component_runtime_lls] entries require key and declared_type");
    }
    if (!seen_keys.insert(entry.key).second) {
      throw std::runtime_error("[component_runtime_lls] duplicate entry key: " +
                               entry.key);
    }
    out << entry.key;
    if (!entry.declared_domain.empty()) {
      out << entry.declared_domain;
    }
    out << ":" << entry.declared_type << " = " << entry.value << "\n";
  }
  return out.str();
}

struct component_runtime_identity_t {
  std::string schema{};
  std::string component_family_id{};
  std::string component_assembly_id{};
  std::string batch_cursor_token{};
  std::string graph_order_fingerprint{};
};

[[nodiscard]] inline runtime_lls_document_t
make_component_runtime_document(component_runtime_identity_t identity) {
  if (identity.batch_cursor_token.empty()) {
    throw std::runtime_error(
        "[component_runtime_lls] batch_cursor_token is required");
  }
  if (identity.schema.empty() || identity.component_family_id.empty() ||
      identity.component_assembly_id.empty() ||
      identity.graph_order_fingerprint.empty()) {
    throw std::runtime_error("[component_runtime_lls] schema, "
                             "component_family_id, component_assembly_id, and "
                             "graph_order_fingerprint are required");
  }
  runtime_lls_document_t document{};
  document.entries.push_back(make_component_runtime_lls_string_entry(
      "schema", std::move(identity.schema)));
  document.entries.push_back(make_component_runtime_lls_string_entry(
      "component_family_id", std::move(identity.component_family_id)));
  document.entries.push_back(make_component_runtime_lls_string_entry(
      "component_assembly_id", std::move(identity.component_assembly_id)));
  document.entries.push_back(make_component_runtime_lls_string_entry(
      "batch_cursor_token", std::move(identity.batch_cursor_token)));
  document.entries.push_back(make_component_runtime_lls_string_entry(
      "graph_order_fingerprint", std::move(identity.graph_order_fingerprint)));
  return document;
}

template <typename KeyT>
[[nodiscard]] inline std::string require_graph_anchor_cursor_token(
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_cursor_t<KeyT> &cursor) {
  if (cursor.graph_order_fingerprint.empty() || cursor.empty()) {
    throw std::runtime_error(
        "[component_runtime_lls] graph anchor batch cursor is required");
  }
  return cursor.cursor_token();
}

inline void
append_runtime_cursor_common_entries(runtime_lls_document_t &document,
                                     const std::string &prefix,
                                     const std::string &cursor_token) {
  document.entries.push_back(make_component_runtime_lls_string_entry(
      prefix + "_cursor_token", cursor_token));
}

template <typename KeyT>
inline void append_graph_anchor_cursor_entries(
    runtime_lls_document_t &document,
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_cursor_t<KeyT> &cursor,
    const std::string &prefix = "batch", bool include_cursor_token = true) {
  const auto cursor_token = require_graph_anchor_cursor_token(cursor);
  if (include_cursor_token) {
    append_runtime_cursor_common_entries(document, prefix, cursor_token);
  }
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_begin_anchor_index",
      static_cast<std::uint64_t>(cursor.begin_anchor_index)));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_end_anchor_index",
      static_cast<std::uint64_t>(cursor.end_anchor_index)));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_requested_batch_size",
      static_cast<std::uint64_t>(cursor.requested_batch_size)));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_anchor_count",
      static_cast<std::uint64_t>(cursor.anchor_count())));
  if (const auto first = cursor.first_anchor_key(); first.has_value()) {
    document.entries.push_back(make_component_runtime_lls_int_entry(
        prefix + "_first_anchor_key", static_cast<std::int64_t>(*first)));
  }
  if (const auto last = cursor.last_anchor_key(); last.has_value()) {
    document.entries.push_back(make_component_runtime_lls_int_entry(
        prefix + "_last_anchor_key", static_cast<std::int64_t>(*last)));
  }
  if (!cursor.anchor_indices.empty()) {
    document.entries.push_back(make_component_runtime_lls_uint_entry(
        prefix + "_anchor_index_count",
        static_cast<std::uint64_t>(cursor.anchor_indices.size())));
    document.entries.push_back(make_component_runtime_lls_uint_entry(
        prefix + "_first_anchor_index",
        static_cast<std::uint64_t>(cursor.anchor_indices.front())));
    document.entries.push_back(make_component_runtime_lls_uint_entry(
        prefix + "_last_anchor_index",
        static_cast<std::uint64_t>(cursor.anchor_indices.back())));
    document.entries.push_back(make_component_runtime_lls_string_entry(
        prefix + "_anchor_index_fingerprint",
        cuwacunu::ujcamei::source::retrieval::dataloader::source_cursor_detail::
            fingerprint_anchor_indices(cursor.anchor_indices)));
  }
}

template <typename KeyT>
inline void append_graph_anchor_cursor_report_entries(
    runtime_lls_document_t &document,
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_cursor_report_t<KeyT> &report,
    const std::string &prefix = "source") {
  append_runtime_cursor_common_entries(document, prefix, report.cursor_token());
  document.entries.push_back(make_component_runtime_lls_string_entry(
      prefix + "_cursor_version", report.version));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_edge_count",
      static_cast<std::uint64_t>(report.edge_ids.size())));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_candidate_anchor_count",
      static_cast<std::uint64_t>(report.candidate_anchor_count)));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_accepted_anchor_count",
      static_cast<std::uint64_t>(report.accepted_anchor_count)));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_skipped_anchor_count",
      static_cast<std::uint64_t>(report.skipped_anchor_count())));
  document.entries.push_back(make_component_runtime_lls_uint_entry(
      prefix + "_duplicate_anchor_count",
      static_cast<std::uint64_t>(report.duplicate_anchor_count)));
  if (const auto first = report.first_anchor_key(); first.has_value()) {
    document.entries.push_back(make_component_runtime_lls_int_entry(
        prefix + "_first_anchor_key", static_cast<std::int64_t>(*first)));
  }
  if (const auto last = report.last_anchor_key(); last.has_value()) {
    document.entries.push_back(make_component_runtime_lls_int_entry(
        prefix + "_last_anchor_key", static_cast<std::int64_t>(*last)));
  }
}

} // namespace cuwacunu::hero::lattice::runtime_report
