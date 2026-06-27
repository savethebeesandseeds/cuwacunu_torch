// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hero/runtime_hero/runtime/job_events_probe_sidecar.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::hero::runtime::probe_settings {

inline constexpr const char *k_runtime_probe_job_events_kind = "job_events";

struct runtime_probe_t {
  std::string probe_id{};
  std::string probe_kind{};
  bool enabled{false};
  job_events_probe::job_events_probe_stream_config_t job_events{};
};

struct runtime_probe_catalog_t {
  std::vector<runtime_probe_t> probes{};
};

namespace detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline bool
parse_optional_bool(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                    const std::string &key, bool default_value) {
  const auto raw = kv::optional(block, key, "");
  const auto value = kv::lowercase(kv::trim(raw));
  if (value.empty()) {
    return default_value;
  }
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error("[runtime_probes] " + key +
                           " must be true or false");
}

[[nodiscard]] inline std::string
optional_trimmed(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                 const std::string &key, const std::string &fallback) {
  return kv::trim(kv::optional(block, key, fallback));
}

inline void validate_job_local_stream_leaf(const std::string &stream_leaf) {
  const std::filesystem::path path(stream_leaf);
  if (path.empty() || path.is_absolute()) {
    throw std::runtime_error(
        "[runtime_probes] STREAM_LEAF must be a job-local relative path");
  }
  for (const auto &part : path) {
    if (part == "..") {
      throw std::runtime_error(
          "[runtime_probes] STREAM_LEAF must not contain parent traversal");
    }
  }
}

} // namespace detail

[[nodiscard]] inline runtime_probe_t decode_runtime_probe_from_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  if (block.name != "RUNTIME_PROBE") {
    throw std::runtime_error("[runtime_probes] expected RUNTIME_PROBE block");
  }
  runtime_probe_t out{};
  out.probe_id = kv::trim(kv::required(block, "PROBE_ID"));
  out.probe_kind = kv::trim(kv::required(block, "PROBE_KIND"));
  out.enabled = detail::parse_optional_bool(block, "ENABLED", false);
  if (out.probe_id.empty()) {
    throw std::runtime_error("[runtime_probes] PROBE_ID is required");
  }
  if (out.probe_kind.empty()) {
    throw std::runtime_error("[runtime_probes] PROBE_KIND is required");
  }
  if (out.probe_kind != k_runtime_probe_job_events_kind) {
    if (out.enabled) {
      throw std::runtime_error(
          "[runtime_probes] unsupported enabled PROBE_KIND: " + out.probe_kind);
    }
    return out;
  }

  out.job_events.record_schema = detail::optional_trimmed(
      block, "RECORD_SCHEMA",
      job_events_probe::k_job_events_probe_record_schema_v1);
  out.job_events.stream_leaf = detail::optional_trimmed(
      block, "STREAM_LEAF", job_events_probe::k_job_events_probe_stream_leaf);
  if (out.job_events.record_schema.empty()) {
    throw std::runtime_error("[runtime_probes] RECORD_SCHEMA is required");
  }
  detail::validate_job_local_stream_leaf(out.job_events.stream_leaf);
  out.job_events.emit_lifecycle =
      detail::parse_optional_bool(block, "EMIT_LIFECYCLE", true);
  out.job_events.emit_scalar_metrics =
      detail::parse_optional_bool(block, "EMIT_SCALAR_METRICS", true);
  out.job_events.emit_report_metrics =
      detail::parse_optional_bool(block, "EMIT_REPORT_METRICS", true);
  out.job_events.emit_artifacts =
      detail::parse_optional_bool(block, "EMIT_ARTIFACTS", true);
  out.job_events.emit_warnings =
      detail::parse_optional_bool(block, "EMIT_WARNINGS", true);
  return out;
}

[[nodiscard]] inline runtime_probe_catalog_t
decode_runtime_probe_catalog_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  runtime_probe_catalog_t out{};
  std::unordered_set<std::string> probe_ids;
  bool enabled_job_events_seen = false;
  for (const auto &block : kv::parse_blocks(dsl_text)) {
    if (block.name != "RUNTIME_PROBE") {
      throw std::runtime_error(
          "[runtime_probes] unexpected block in probe catalog: " + block.name);
    }
    auto probe = decode_runtime_probe_from_block(block);
    if (!probe_ids.insert(probe.probe_id).second) {
      throw std::runtime_error("[runtime_probes] duplicate PROBE_ID: " +
                               probe.probe_id);
    }
    if (probe.enabled && probe.probe_kind == k_runtime_probe_job_events_kind) {
      if (enabled_job_events_seen) {
        throw std::runtime_error(
            "[runtime_probes] multiple enabled job_events probes");
      }
      enabled_job_events_seen = true;
    }
    out.probes.push_back(std::move(probe));
  }
  return out;
}

[[nodiscard]] inline bool select_enabled_job_events_probe(
    const runtime_probe_catalog_t &catalog,
    job_events_probe::job_events_probe_stream_config_t *config) {
  for (const auto &probe : catalog.probes) {
    if (!probe.enabled || probe.probe_kind != k_runtime_probe_job_events_kind) {
      continue;
    }
    if (config != nullptr) {
      *config = probe.job_events;
    }
    return true;
  }
  return false;
}

[[nodiscard]] inline runtime_probe_catalog_t
load_runtime_probe_catalog_from_config(std::string config_path = {}) {
  namespace config_detail =
      cuwacunu::kikijyeba::protocol::graph_first_config_detail;
  if (detail::kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  const auto cfg = config_detail::parse_assignment_config(config_path);
  const auto probe_it = cfg.find("runtime_probes_dsl_path");
  if (probe_it == cfg.end() || detail::kv::trim(probe_it->second).empty()) {
    return {};
  }
  const auto probe_path = probe_it->second;
  const auto text = config_detail::read_text_file_or_throw(probe_path);
  return decode_runtime_probe_catalog_from_dsl(text);
}

} // namespace cuwacunu::hero::runtime::probe_settings
