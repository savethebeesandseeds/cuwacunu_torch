// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::lattice::exposure {

enum class exposure_split_role_t {
  unknown,
  train,
  validation,
  test,
};

enum class exposure_use_t {
  observed_input,
  target_supervision,
  evaluation_metric,
  selection_signal,
};

struct anchor_interval_t {
  std::int64_t begin{0};
  std::int64_t end{0};

  [[nodiscard]] bool empty() const { return end <= begin; }

  [[nodiscard]] std::int64_t length() const {
    return empty() ? 0 : end - begin;
  }
};

struct exposure_use_set_t {
  bool observed_input{false};
  bool target_supervision{false};
  bool evaluation_metric{false};
  bool selection_signal{false};
  bool mutated_component{false};
};

struct lattice_exposure_fact_t {
  std::string schema{"kikijyeba.lattice.exposure.v1"};
  std::string fact_type{"exposure"};

  std::string contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string component_assembly_fingerprint{};
  std::string target_component{};
  std::string job_id{};
  std::string wave_id{};
  std::string job_status{};
  std::string wave_action{};

  std::string cursor_domain{"ujcamei.graph_anchor"};
  std::string source_cursor_token{};
  anchor_interval_t anchor_range{};
  anchor_interval_t observed_footprint{};
  anchor_interval_t target_footprint{};
  std::string first_anchor_key{};
  std::string last_anchor_key{};
  std::string footprint_precision{"anchor_range_v0"};
  std::string observed_source_key_begin{};
  std::string observed_source_key_end{};
  std::string target_source_key_begin{};
  std::string target_source_key_end{};
  std::string source_key_footprint_precision{};
  std::string source_file_receipts{};
  std::int64_t source_input_length{0};
  std::int64_t source_future_length{0};
  std::int64_t candidate_anchor_count{0};
  std::int64_t accepted_anchor_count{0};
  double accepted_anchor_fraction{std::numeric_limits<double>::quiet_NaN()};
  std::int64_t skipped_outside_common_range{0};
  std::int64_t skipped_missing_edge_coverage{0};
  std::int64_t skipped_failed_fetch_probe{0};
  std::int64_t duplicate_anchor_count{0};
  std::string anchor_domain_warning_level{"none"};
  std::string anchor_domain_warnings{};
  std::string common_left_key{};
  std::string common_right_key{};
  std::string reference_left_key{};
  std::string reference_right_key{};

  std::string split_name{"unknown"};
  exposure_split_role_t split_role{exposure_split_role_t::unknown};
  std::string split_policy_fingerprint{};
  std::string coverage_precision{"requested_range_v0"};
  anchor_interval_t completed_anchor_range{};
  exposure_use_set_t use{};

  std::int64_t anchors_seen{0};
  std::int64_t batches_seen{0};
  std::int64_t optimizer_steps{0};
  std::int64_t wave_pulses_completed{0};
  std::int64_t valid_target_count{0};
  double valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};

  std::vector<std::filesystem::path> input_checkpoints{};
  std::filesystem::path output_checkpoint{};
  std::string output_checkpoint_exposure_digest{};

  bool finite_loss{false};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_nll{std::numeric_limits<double>::quiet_NaN()};
  std::string diagnostics_digest{};
};

struct exposure_build_context_t {
  std::string split_name{"unknown"};
  exposure_split_role_t split_role{exposure_split_role_t::unknown};
  std::string cursor_domain{"ujcamei.graph_anchor"};
  std::string split_policy_fingerprint{};
  bool selection_signal{false};
};

struct forbidden_exposure_query_t {
  anchor_interval_t forbidden_range{};
  std::vector<exposure_use_t> forbidden_uses{};
  bool require_mutated_component{true};
};

struct exposure_coverage_t {
  anchor_interval_t target_range{};
  std::int64_t covered_anchors{0};
  double coverage_fraction{0.0};
};

struct exposure_load_summary_t {
  anchor_interval_t target_range{};
  exposure_use_t use{exposure_use_t::observed_input};
  std::string component_scope{"all_components"};
  std::string target_component{};
  bool require_mutated_component{true};

  std::int64_t unique_covered_anchors{0};
  double unique_coverage_fraction{0.0};

  std::int64_t loaded_anchor_events{0};
  double cursor_exposure_load{0.0};

  std::int64_t fact_count{0};
  std::int64_t optimizer_steps_total{0};
  double optimizer_steps_per_unique_anchor{0.0};
  double optimizer_steps_per_cursor_epoch{0.0};

  std::int64_t valid_target_count_total{0};
  double valid_target_cursor_epochs{0.0};
};

struct lattice_node_exposure_fact_t {
  std::string schema{"kikijyeba.lattice.node_exposure.v1"};
  std::string fact_type{"node_exposure"};
  std::string parent_exposure_fact_digest{};

  std::string contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string split_policy_fingerprint{};
  std::string component_assembly_fingerprint{};
  std::string target_component{};
  std::string job_id{};
  std::string wave_id{};
  std::string job_status{};
  std::string wave_action{};

  std::string node_id{};
  std::int64_t node_index{0};
  std::string split_name{"unknown"};
  exposure_split_role_t split_role{exposure_split_role_t::unknown};
  anchor_interval_t anchor_range{};
  anchor_interval_t completed_anchor_range{};
  exposure_use_set_t use{};

  std::int64_t routed_row_count{0};
  std::int64_t active_row_count{0};
  std::int64_t trained_row_count{0};
  std::int64_t evaluated_row_count{0};
  std::int64_t valid_target_count{0};
  double valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  double mean_nll{std::numeric_limits<double>::quiet_NaN()};

  std::filesystem::path output_checkpoint{};
};

struct checkpoint_closure_result_t {
  std::vector<lattice_exposure_fact_t> facts{};
  std::vector<std::filesystem::path> unresolved_input_checkpoints{};

  [[nodiscard]] bool complete() const {
    return unresolved_input_checkpoints.empty();
  }
};

struct lattice_checkpoint_fact_t {
  std::string schema{"kikijyeba.lattice.checkpoint.v1"};
  std::string fact_type{"checkpoint_identity"};
  std::string checkpoint_id{};
  std::filesystem::path checkpoint_path{};
  std::string checkpoint_file_digest{};
  std::string component{};
  std::string contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string component_assembly_fingerprint{};
  std::string created_by_job_id{};
  std::string created_by_wave_id{};
  std::string created_by_exposure_fact_id{};
  std::vector<std::filesystem::path> input_checkpoints{};
  std::string direct_exposure_digest{};
  std::string closure_digest{};
};

namespace exposure_detail {

namespace fs = std::filesystem;
namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string read_text_file_or_empty(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_text(const std::string &text) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(text);
  std::string line;
  while (std::getline(lines, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = kv::trim(std::move(line));
    if (line.empty() || (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    auto key = kv::trim(line.substr(0, eq));
    auto value = kv::trim(line.substr(eq + 1));
    if (!key.empty()) {
      out[std::move(key)] = std::move(value);
    }
  }
  return out;
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_file(const fs::path &path) {
  return parse_assignment_text(read_text_file_or_empty(path));
}

[[nodiscard]] inline std::string
map_get(const std::unordered_map<std::string, std::string> &map,
        const std::string &key, const std::string &fallback = {}) {
  const auto it = map.find(key);
  return it == map.end() ? fallback : it->second;
}

[[nodiscard]] inline bool parse_bool_fallback(std::string value,
                                              bool fallback = false) {
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value.empty()) {
    return fallback;
  }
  if (value == "true" || value == "yes" || value == "1") {
    return true;
  }
  if (value == "false" || value == "no" || value == "0") {
    return false;
  }
  return fallback;
}

[[nodiscard]] inline std::int64_t parse_i64_fallback(std::string value,
                                                     std::int64_t fallback) {
  value = kv::trim(std::move(value));
  if (value.empty()) {
    return fallback;
  }
  try {
    return kv::parse_i64(value);
  } catch (...) {
    return fallback;
  }
}

[[nodiscard]] inline double parse_double_fallback(std::string value,
                                                  double fallback) {
  value = kv::trim(std::move(value));
  if (value.empty()) {
    return fallback;
  }
  try {
    return kv::parse_double(value);
  } catch (...) {
    return fallback;
  }
}

[[nodiscard]] inline std::int64_t
first_i64(const std::unordered_map<std::string, std::string> &a,
          const std::unordered_map<std::string, std::string> &b,
          const std::vector<std::string> &keys, std::int64_t fallback = 0) {
  for (const auto &key : keys) {
    if (!map_get(a, key).empty()) {
      return parse_i64_fallback(map_get(a, key), fallback);
    }
    if (!map_get(b, key).empty()) {
      return parse_i64_fallback(map_get(b, key), fallback);
    }
  }
  return fallback;
}

[[nodiscard]] inline double
first_double(const std::unordered_map<std::string, std::string> &a,
             const std::unordered_map<std::string, std::string> &b,
             const std::vector<std::string> &keys,
             double fallback = std::numeric_limits<double>::quiet_NaN()) {
  for (const auto &key : keys) {
    const auto av = parse_double_fallback(
        map_get(a, key), std::numeric_limits<double>::quiet_NaN());
    if (std::isfinite(av)) {
      return av;
    }
    const auto bv = parse_double_fallback(
        map_get(b, key), std::numeric_limits<double>::quiet_NaN());
    if (std::isfinite(bv)) {
      return bv;
    }
  }
  return fallback;
}

[[nodiscard]] inline std::string
first_string(const std::unordered_map<std::string, std::string> &a,
             const std::unordered_map<std::string, std::string> &b,
             const std::unordered_map<std::string, std::string> &c,
             const std::vector<std::string> &keys) {
  for (const auto &key : keys) {
    const auto av = map_get(a, key);
    if (!av.empty()) {
      return av;
    }
    const auto bv = map_get(b, key);
    if (!bv.empty()) {
      return bv;
    }
    const auto cv = map_get(c, key);
    if (!cv.empty()) {
      return cv;
    }
  }
  return {};
}

[[nodiscard]] inline std::int64_t
first_i64(const std::unordered_map<std::string, std::string> &a,
          const std::unordered_map<std::string, std::string> &b,
          const std::unordered_map<std::string, std::string> &c,
          const std::vector<std::string> &keys, std::int64_t fallback = 0) {
  for (const auto &key : keys) {
    if (!map_get(a, key).empty()) {
      return parse_i64_fallback(map_get(a, key), fallback);
    }
    if (!map_get(b, key).empty()) {
      return parse_i64_fallback(map_get(b, key), fallback);
    }
    if (!map_get(c, key).empty()) {
      return parse_i64_fallback(map_get(c, key), fallback);
    }
  }
  return fallback;
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

inline void normalize_anchor_domain_health(lattice_exposure_fact_t &fact) {
  if (fact.candidate_anchor_count == 0 && fact.accepted_anchor_count > 0) {
    // Compatibility for older manifests that emitted accepted anchors but not
    // the candidate denominator.
    fact.candidate_anchor_count = fact.accepted_anchor_count;
  } else if (fact.accepted_anchor_count == 0 &&
             fact.candidate_anchor_count == 0 &&
             fact.anchor_range.length() > 0) {
    // Compatibility for older sidecars that predate explicit domain health.
    fact.accepted_anchor_count = fact.anchor_range.length();
    fact.candidate_anchor_count = fact.anchor_range.length();
  }
  if (!std::isfinite(fact.accepted_anchor_fraction)) {
    fact.accepted_anchor_fraction =
        fact.candidate_anchor_count == 0
            ? 0.0
            : static_cast<double>(fact.accepted_anchor_count) /
                  static_cast<double>(fact.candidate_anchor_count);
  }
  if (!fact.anchor_domain_warnings.empty() &&
      fact.anchor_domain_warning_level.empty()) {
    fact.anchor_domain_warning_level = "warning";
  }
  if (fact.anchor_domain_warning_level.empty()) {
    fact.anchor_domain_warning_level = "none";
  }
  if (fact.accepted_anchor_count == 0) {
    fact.anchor_domain_warning_level = "error";
    if (fact.anchor_domain_warnings.find("accepted_anchor_count_zero") ==
        std::string::npos) {
      append_anchor_domain_warning_token(fact.anchor_domain_warnings,
                                         "accepted_anchor_count_zero");
    }
    return;
  }
  if (fact.accepted_anchor_fraction < 0.80) {
    fact.anchor_domain_warning_level = "warning";
    if (fact.anchor_domain_warnings.find(
            "accepted_anchor_fraction_below_0.80") == std::string::npos) {
      append_anchor_domain_warning_token(fact.anchor_domain_warnings,
                                         "accepted_anchor_fraction_below_0.80");
    }
  }
  if (fact.skipped_failed_fetch_probe > 0) {
    fact.anchor_domain_warning_level = "warning";
    if (fact.anchor_domain_warnings.find(
            "skipped_failed_fetch_probe_nonzero") == std::string::npos) {
      append_anchor_domain_warning_token(fact.anchor_domain_warnings,
                                         "skipped_failed_fetch_probe_nonzero");
    }
  }
  const double missing_edge_fraction =
      fact.candidate_anchor_count == 0
          ? 0.0
          : static_cast<double>(fact.skipped_missing_edge_coverage) /
                static_cast<double>(fact.candidate_anchor_count);
  if (missing_edge_fraction > 0.10) {
    fact.anchor_domain_warning_level = "warning";
    if (fact.anchor_domain_warnings.find(
            "skipped_missing_edge_coverage_fraction_above_0.10") ==
        std::string::npos) {
      append_anchor_domain_warning_token(
          fact.anchor_domain_warnings,
          "skipped_missing_edge_coverage_fraction_above_0.10");
    }
  }
}

[[nodiscard]] inline fs::path resolve_job_path(const fs::path &job_dir,
                                               const std::string &raw) {
  if (kv::trim(raw).empty()) {
    return {};
  }
  const fs::path path(raw);
  return path.is_absolute() ? path.lexically_normal()
                            : (job_dir / path).lexically_normal();
}

[[nodiscard]] inline std::string checkpoint_key(const fs::path &path) {
  return path.empty() ? std::string{} : path.lexically_normal().string();
}

[[nodiscard]] inline std::vector<fs::path>
parse_checkpoint_path_list(const std::string &csv, const fs::path &job_dir) {
  std::vector<fs::path> out;
  std::istringstream in(csv);
  std::string item;
  while (std::getline(in, item, ',')) {
    item = kv::trim(std::move(item));
    if (!item.empty()) {
      out.push_back(resolve_job_path(job_dir, item));
    }
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string>
parse_string_list(const std::string &csv) {
  return kv::parse_list(csv);
}

[[nodiscard]] inline std::vector<std::int64_t>
parse_i64_list_fallback(const std::string &csv) {
  std::vector<std::int64_t> out;
  for (const auto &item : kv::parse_list(csv)) {
    out.push_back(parse_i64_fallback(item, 0));
  }
  return out;
}

[[nodiscard]] inline std::vector<double>
parse_double_list_fallback(const std::string &csv) {
  std::vector<double> out;
  for (const auto &item : kv::parse_list(csv)) {
    out.push_back(
        parse_double_fallback(item, std::numeric_limits<double>::quiet_NaN()));
  }
  return out;
}

[[nodiscard]] inline std::string component_fingerprint_from_manifest(
    const std::unordered_map<std::string, std::string> &manifest) {
  const auto target = map_get(manifest, "target_component");
  if (target == "wikimyei.representation.encoding.vicreg") {
    return map_get(manifest, "vicreg_assembly_fingerprint");
  }
  if (target == "wikimyei.inference.expected_value.mdn") {
    return map_get(manifest, "mdn_assembly_fingerprint");
  }
  return {};
}

[[nodiscard]] inline bool contains_token(const std::string &csv,
                                         const std::string &token) {
  std::istringstream in(csv);
  std::string item;
  while (std::getline(in, item, ',')) {
    if (kv::trim(item) == token) {
      return true;
    }
  }
  return false;
}

} // namespace exposure_detail

[[nodiscard]] inline std::filesystem::path
exposure_fact_path_for_job_dir(const std::filesystem::path &job_dir) {
  return job_dir / "lattice.exposure.fact";
}

[[nodiscard]] inline std::filesystem::path
checkpoint_fact_path_for_job_dir(const std::filesystem::path &job_dir) {
  return job_dir / "lattice.checkpoint.fact";
}

[[nodiscard]] inline const char *
exposure_split_role_name(exposure_split_role_t role) {
  switch (role) {
  case exposure_split_role_t::unknown:
    return "unknown";
  case exposure_split_role_t::train:
    return "train";
  case exposure_split_role_t::validation:
    return "validation";
  case exposure_split_role_t::test:
    return "test";
  }
  throw std::runtime_error("[lattice_exposure] unknown split role");
}

[[nodiscard]] inline exposure_split_role_t
parse_exposure_split_role(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value.empty() || value == "unknown") {
    return exposure_split_role_t::unknown;
  }
  if (value == "train") {
    return exposure_split_role_t::train;
  }
  if (value == "validation") {
    return exposure_split_role_t::validation;
  }
  if (value == "test") {
    return exposure_split_role_t::test;
  }
  throw std::runtime_error("[lattice_exposure] invalid split role: " + value);
}

[[nodiscard]] inline const char *exposure_use_name(exposure_use_t use) {
  switch (use) {
  case exposure_use_t::observed_input:
    return "observed_input";
  case exposure_use_t::target_supervision:
    return "target_supervision";
  case exposure_use_t::evaluation_metric:
    return "evaluation_metric";
  case exposure_use_t::selection_signal:
    return "selection_signal";
  }
  throw std::runtime_error("[lattice_exposure] unknown exposure use");
}

[[nodiscard]] inline exposure_use_t parse_exposure_use(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value == "observed_input") {
    return exposure_use_t::observed_input;
  }
  if (value == "target_supervision") {
    return exposure_use_t::target_supervision;
  }
  if (value == "evaluation_metric") {
    return exposure_use_t::evaluation_metric;
  }
  if (value == "selection_signal") {
    return exposure_use_t::selection_signal;
  }
  throw std::runtime_error("[lattice_exposure] invalid exposure use: " + value);
}

[[nodiscard]] inline std::vector<exposure_use_t>
parse_exposure_use_list(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  std::vector<exposure_use_t> out;
  std::string token;
  for (const char ch : value) {
    if (ch == '|' || ch == ',' || ch == '+') {
      token = kv::trim(std::move(token));
      if (!token.empty()) {
        out.push_back(parse_exposure_use(token));
      }
      token.clear();
      continue;
    }
    token.push_back(ch);
  }
  token = kv::trim(std::move(token));
  if (!token.empty()) {
    out.push_back(parse_exposure_use(token));
  }
  return out;
}

[[nodiscard]] inline bool intervals_overlap(anchor_interval_t a,
                                            anchor_interval_t b) {
  return !a.empty() && !b.empty() && a.begin < b.end && b.begin < a.end;
}

[[nodiscard]] inline anchor_interval_t
interval_intersection(anchor_interval_t a, anchor_interval_t b) {
  anchor_interval_t out{.begin = std::max(a.begin, b.begin),
                        .end = std::min(a.end, b.end)};
  if (out.empty()) {
    return {};
  }
  return out;
}

[[nodiscard]] inline bool fact_has_use(const lattice_exposure_fact_t &fact,
                                       exposure_use_t use) {
  switch (use) {
  case exposure_use_t::observed_input:
    return fact.use.observed_input;
  case exposure_use_t::target_supervision:
    return fact.use.target_supervision;
  case exposure_use_t::evaluation_metric:
    return fact.use.evaluation_metric;
  case exposure_use_t::selection_signal:
    return fact.use.selection_signal;
  }
  throw std::runtime_error("[lattice_exposure] unknown exposure use");
}

[[nodiscard]] inline anchor_interval_t
anchor_coverage_for_use(const lattice_exposure_fact_t &fact,
                        exposure_use_t use) {
  switch (use) {
  case exposure_use_t::observed_input:
  case exposure_use_t::target_supervision:
  case exposure_use_t::evaluation_metric:
  case exposure_use_t::selection_signal:
    if (fact.coverage_precision == "contiguous_completed_range_v1") {
      return fact.completed_anchor_range;
    }
    return {};
  }
  throw std::runtime_error("[lattice_exposure] unknown exposure use");
}

[[nodiscard]] inline anchor_interval_t
source_footprint_for_use(const lattice_exposure_fact_t &fact,
                         exposure_use_t use) {
  switch (use) {
  case exposure_use_t::observed_input:
    return fact.observed_footprint;
  case exposure_use_t::target_supervision:
    return fact.target_footprint;
  case exposure_use_t::evaluation_metric:
  case exposure_use_t::selection_signal:
    return fact.anchor_range;
  }
  throw std::runtime_error("[lattice_exposure] unknown exposure use");
}

[[nodiscard]] inline std::int64_t
left_context_rows_for_fact(const lattice_exposure_fact_t &fact) {
  if (fact.source_input_length > 0) {
    return std::max<std::int64_t>(0, fact.source_input_length - 1);
  }
  if (!fact.anchor_range.empty() && !fact.observed_footprint.empty()) {
    return std::max<std::int64_t>(0, fact.anchor_range.begin -
                                         fact.observed_footprint.begin);
  }
  return 0;
}

[[nodiscard]] inline std::int64_t
right_future_rows_for_fact(const lattice_exposure_fact_t &fact) {
  if (fact.source_future_length > 0) {
    return fact.source_future_length;
  }
  if (!fact.anchor_range.empty() && !fact.target_footprint.empty()) {
    return std::max<std::int64_t>(0, fact.target_footprint.end -
                                         fact.anchor_range.end);
  }
  return 0;
}

[[nodiscard]] inline lattice_exposure_fact_t
make_exposure_fact_from_job_dir(const std::filesystem::path &job_dir,
                                exposure_build_context_t context = {}) {
  namespace detail = exposure_detail;
  const auto manifest = detail::parse_assignment_file(job_dir / "job.manifest");
  if (manifest.empty()) {
    throw std::runtime_error(
        "[lattice_exposure] job.manifest is required under: " +
        job_dir.string());
  }
  const auto state = detail::parse_assignment_file(job_dir / "job.state");
  const auto job_kind = detail::map_get(manifest, "job_kind");
  const auto report_path =
      job_dir / (job_kind == "representation_vicreg" ? "representation.report"
                                                     : "inference.report");
  const auto report = detail::parse_assignment_file(report_path);
  const auto target_component = detail::map_get(manifest, "target_component");
  const auto wave_action = detail::map_get(
      manifest, "wave_action", detail::map_get(state, "wave_action"));

  lattice_exposure_fact_t out{};
  out.contract_fingerprint =
      detail::map_get(manifest, "protocol_contract_fingerprint");
  out.graph_order_fingerprint =
      detail::map_get(manifest, "graph_order_fingerprint");
  out.component_assembly_fingerprint =
      detail::component_fingerprint_from_manifest(manifest);
  out.target_component = target_component;
  out.job_id = detail::map_get(manifest, "job_id");
  out.wave_id =
      detail::map_get(manifest, "wave_id", detail::map_get(state, "wave_id"));
  out.job_status = detail::map_get(state, "status");
  out.wave_action = wave_action;
  out.cursor_domain = std::move(context.cursor_domain);
  out.source_cursor_token =
      detail::map_get(manifest, "source_cursor_token",
                      detail::map_get(state, "source_cursor_token"));
  out.anchor_range = anchor_interval_t{
      .begin = detail::parse_i64_fallback(
          detail::map_get(manifest, "resolved_anchor_index_begin"), 0),
      .end = detail::parse_i64_fallback(
          detail::map_get(manifest, "resolved_anchor_index_end"), 0),
  };
  out.observed_footprint = out.anchor_range;
  out.target_footprint = out.anchor_range;
  const auto footprint_precision = detail::first_string(
      manifest, state, report, {"source_footprint_precision"});
  if (!footprint_precision.empty()) {
    out.footprint_precision = footprint_precision;
  }
  const auto observed_begin =
      detail::first_i64(manifest, state, report, {"observed_source_row_begin"},
                        std::numeric_limits<std::int64_t>::min());
  const auto observed_end =
      detail::first_i64(manifest, state, report, {"observed_source_row_end"},
                        std::numeric_limits<std::int64_t>::min());
  if (observed_begin != std::numeric_limits<std::int64_t>::min() &&
      observed_end != std::numeric_limits<std::int64_t>::min()) {
    out.observed_footprint =
        anchor_interval_t{.begin = observed_begin, .end = observed_end};
  }
  const auto target_begin =
      detail::first_i64(manifest, state, report, {"target_source_row_begin"},
                        std::numeric_limits<std::int64_t>::min());
  const auto target_end =
      detail::first_i64(manifest, state, report, {"target_source_row_end"},
                        std::numeric_limits<std::int64_t>::min());
  if (target_begin != std::numeric_limits<std::int64_t>::min() &&
      target_end != std::numeric_limits<std::int64_t>::min()) {
    out.target_footprint =
        anchor_interval_t{.begin = target_begin, .end = target_end};
  }
  out.first_anchor_key = detail::first_string(
      state, manifest, report,
      {"wave_first_anchor_key", "first_anchor_key", "source_first_anchor_key"});
  out.last_anchor_key = detail::first_string(
      state, manifest, report,
      {"wave_last_anchor_key", "last_anchor_key", "source_last_anchor_key"});
  out.observed_source_key_begin = detail::first_string(
      manifest, state, report, {"observed_source_key_begin"});
  out.observed_source_key_end = detail::first_string(
      manifest, state, report, {"observed_source_key_end"});
  out.target_source_key_begin = detail::first_string(
      manifest, state, report, {"target_source_key_begin"});
  out.target_source_key_end =
      detail::first_string(manifest, state, report, {"target_source_key_end"});
  out.source_key_footprint_precision = detail::first_string(
      manifest, state, report, {"source_key_footprint_precision"});
  out.source_file_receipts =
      detail::first_string(manifest, state, report, {"source_file_receipts"});
  out.source_input_length = detail::first_i64(
      manifest, state, report,
      {"source_input_length", "max_input_length", "input_length"}, 0);
  out.source_future_length = detail::first_i64(
      manifest, state, report,
      {"source_future_length", "max_future_length", "future_length"}, 0);
  out.candidate_anchor_count = detail::first_i64(
      manifest, state, report,
      {"candidate_anchor_count", "source_candidate_anchor_count"}, 0);
  out.accepted_anchor_count =
      detail::first_i64(manifest, state, report,
                        {"accepted_anchor_count", "source_anchor_count"}, 0);
  out.accepted_anchor_fraction = detail::first_double(
      manifest, state,
      {"accepted_anchor_fraction", "source_accepted_anchor_fraction"});
  out.skipped_outside_common_range = detail::first_i64(
      manifest, state, report,
      {"skipped_outside_common_range", "source_skipped_outside_common_range"},
      0);
  out.skipped_missing_edge_coverage = detail::first_i64(
      manifest, state, report,
      {"skipped_missing_edge_coverage", "source_skipped_missing_edge_coverage"},
      0);
  out.skipped_failed_fetch_probe = detail::first_i64(
      manifest, state, report,
      {"skipped_failed_fetch_probe", "source_skipped_failed_fetch_probe"}, 0);
  out.duplicate_anchor_count = detail::first_i64(
      manifest, state, report,
      {"duplicate_anchor_count", "source_duplicate_anchor_count"}, 0);
  out.anchor_domain_warning_level = detail::first_string(
      manifest, state, report,
      {"anchor_domain_warning_level", "source_anchor_domain_warning_level"});
  out.anchor_domain_warnings = detail::first_string(
      manifest, state, report,
      {"anchor_domain_warnings", "source_anchor_domain_warnings"});
  out.common_left_key = detail::first_string(
      manifest, state, report, {"common_left_key", "source_common_left_key"});
  out.common_right_key = detail::first_string(
      manifest, state, report, {"common_right_key", "source_common_right_key"});
  out.reference_left_key =
      detail::first_string(manifest, state, report,
                           {"reference_left_key", "source_reference_left_key"});
  out.reference_right_key = detail::first_string(
      manifest, state, report,
      {"reference_right_key", "source_reference_right_key"});
  detail::normalize_anchor_domain_health(out);
  out.split_name = std::move(context.split_name);
  out.split_role = context.split_role;
  out.split_policy_fingerprint = std::move(context.split_policy_fingerprint);
  out.use.observed_input = true;
  out.use.target_supervision = job_kind == "inference_mdn";
  out.use.evaluation_metric = !report.empty();
  out.use.selection_signal = context.selection_signal;

  out.anchors_seen = detail::first_i64(
      state, report,
      {"wave_streamed_anchor_count", "source_anchor_count",
       "accepted_anchor_count"},
      detail::parse_i64_fallback(
          detail::map_get(manifest, "accepted_anchor_count"), 0));
  const auto skipped_batches =
      detail::first_i64(state, report, {"skipped_batches"}, 0);
  const auto wave_pulses_skipped =
      detail::first_i64(state, report, {"wave_pulses_skipped"}, 0);
  const auto requested_anchors = out.anchor_range.length();
  if (out.job_status == "completed" && requested_anchors > 0 &&
      out.anchors_seen >= requested_anchors && skipped_batches == 0 &&
      wave_pulses_skipped == 0) {
    out.coverage_precision = "contiguous_completed_range_v1";
    out.completed_anchor_range = out.anchor_range;
  } else {
    out.coverage_precision = "requested_range_untrusted_v0";
    out.completed_anchor_range = {};
  }
  out.wave_pulses_completed =
      detail::first_i64(state, report, {"wave_pulses_completed"}, 0);
  out.batches_seen =
      std::max(detail::first_i64(state, report, {"steps_attempted"}, 0),
               out.wave_pulses_completed);
  out.optimizer_steps =
      detail::first_i64(state, report, {"optimizer_steps"}, 0);
  out.use.mutated_component =
      out.job_status == "completed" && wave_action == "train" &&
      out.optimizer_steps > 0 &&
      detail::contains_token(detail::map_get(manifest, "mutated_components"),
                             target_component);
  out.valid_target_count = detail::first_i64(
      report, state, {"total_valid_target_count", "last_valid_target_count"},
      0);
  out.valid_target_fraction = detail::first_double(
      report, state,
      {"mean_valid_node_target_fraction", "last_valid_node_target_fraction"});
  out.mean_loss =
      detail::first_double(report, state, {"mean_loss", "last_loss"});
  out.mean_nll = job_kind == "inference_mdn"
                     ? out.mean_loss
                     : std::numeric_limits<double>::quiet_NaN();
  out.finite_loss = std::isfinite(out.mean_loss);

  const auto output_checkpoint = detail::map_get(
      state, "checkpoint_path", detail::map_get(report, "checkpoint_path"));
  out.output_checkpoint = detail::resolve_job_path(job_dir, output_checkpoint);
  const auto representation_checkpoint =
      detail::map_get(report, "representation_checkpoint_path");
  if (!representation_checkpoint.empty()) {
    out.input_checkpoints.push_back(
        detail::resolve_job_path(job_dir, representation_checkpoint));
  }
  const auto mdn_checkpoint = detail::map_get(report, "mdn_checkpoint_path");
  if (!mdn_checkpoint.empty() &&
      detail::parse_bool_fallback(
          detail::map_get(report, "mdn_checkpoint_loaded"), false)) {
    out.input_checkpoints.push_back(
        detail::resolve_job_path(job_dir, mdn_checkpoint));
  }
  return out;
}

[[nodiscard]] inline std::string
canonical_exposure_fact_text(const lattice_exposure_fact_t &fact) {
  std::ostringstream out;
  out << "schema=" << fact.schema << "\n";
  out << "fact_type=" << fact.fact_type << "\n";
  out << "contract_fingerprint=" << fact.contract_fingerprint << "\n";
  out << "graph_order_fingerprint=" << fact.graph_order_fingerprint << "\n";
  out << "component_assembly_fingerprint="
      << fact.component_assembly_fingerprint << "\n";
  out << "target_component=" << fact.target_component << "\n";
  out << "job_id=" << fact.job_id << "\n";
  out << "wave_id=" << fact.wave_id << "\n";
  out << "job_status=" << fact.job_status << "\n";
  out << "wave_action=" << fact.wave_action << "\n";
  out << "cursor_domain=" << fact.cursor_domain << "\n";
  out << "source_cursor_token=" << fact.source_cursor_token << "\n";
  out << "anchor_begin=" << fact.anchor_range.begin << "\n";
  out << "anchor_end=" << fact.anchor_range.end << "\n";
  out << "observed_begin=" << fact.observed_footprint.begin << "\n";
  out << "observed_end=" << fact.observed_footprint.end << "\n";
  out << "target_begin=" << fact.target_footprint.begin << "\n";
  out << "target_end=" << fact.target_footprint.end << "\n";
  out << "footprint_precision=" << fact.footprint_precision << "\n";
  out << "first_anchor_key=" << fact.first_anchor_key << "\n";
  out << "last_anchor_key=" << fact.last_anchor_key << "\n";
  out << "observed_source_key_begin=" << fact.observed_source_key_begin << "\n";
  out << "observed_source_key_end=" << fact.observed_source_key_end << "\n";
  out << "target_source_key_begin=" << fact.target_source_key_begin << "\n";
  out << "target_source_key_end=" << fact.target_source_key_end << "\n";
  out << "source_key_footprint_precision="
      << fact.source_key_footprint_precision << "\n";
  out << "source_file_receipts=" << fact.source_file_receipts << "\n";
  out << "source_input_length=" << fact.source_input_length << "\n";
  out << "source_future_length=" << fact.source_future_length << "\n";
  out << "candidate_anchor_count=" << fact.candidate_anchor_count << "\n";
  out << "accepted_anchor_count=" << fact.accepted_anchor_count << "\n";
  out << "accepted_anchor_fraction=" << fact.accepted_anchor_fraction << "\n";
  out << "skipped_outside_common_range=" << fact.skipped_outside_common_range
      << "\n";
  out << "skipped_missing_edge_coverage=" << fact.skipped_missing_edge_coverage
      << "\n";
  out << "skipped_failed_fetch_probe=" << fact.skipped_failed_fetch_probe
      << "\n";
  out << "duplicate_anchor_count=" << fact.duplicate_anchor_count << "\n";
  out << "anchor_domain_warning_level=" << fact.anchor_domain_warning_level
      << "\n";
  out << "anchor_domain_warnings=" << fact.anchor_domain_warnings << "\n";
  out << "common_left_key=" << fact.common_left_key << "\n";
  out << "common_right_key=" << fact.common_right_key << "\n";
  out << "reference_left_key=" << fact.reference_left_key << "\n";
  out << "reference_right_key=" << fact.reference_right_key << "\n";
  out << "split_name=" << fact.split_name << "\n";
  out << "split_role=" << exposure_split_role_name(fact.split_role) << "\n";
  out << "split_policy_fingerprint=" << fact.split_policy_fingerprint << "\n";
  out << "coverage_precision=" << fact.coverage_precision << "\n";
  out << "completed_anchor_begin=" << fact.completed_anchor_range.begin << "\n";
  out << "completed_anchor_end=" << fact.completed_anchor_range.end << "\n";
  out << "use_observed_input=" << (fact.use.observed_input ? "true" : "false")
      << "\n";
  out << "use_target_supervision="
      << (fact.use.target_supervision ? "true" : "false") << "\n";
  out << "use_evaluation_metric="
      << (fact.use.evaluation_metric ? "true" : "false") << "\n";
  out << "use_selection_signal="
      << (fact.use.selection_signal ? "true" : "false") << "\n";
  out << "mutated_component=" << (fact.use.mutated_component ? "true" : "false")
      << "\n";
  out << "anchors_seen=" << fact.anchors_seen << "\n";
  out << "batches_seen=" << fact.batches_seen << "\n";
  out << "optimizer_steps=" << fact.optimizer_steps << "\n";
  out << "wave_pulses_completed=" << fact.wave_pulses_completed << "\n";
  out << "valid_target_count=" << fact.valid_target_count << "\n";
  out << "valid_target_fraction=" << fact.valid_target_fraction << "\n";
  out << "output_checkpoint="
      << fact.output_checkpoint.lexically_normal().string() << "\n";
  std::vector<std::string> input_keys;
  input_keys.reserve(fact.input_checkpoints.size());
  for (const auto &checkpoint : fact.input_checkpoints) {
    input_keys.push_back(checkpoint.lexically_normal().string());
  }
  std::sort(input_keys.begin(), input_keys.end());
  out << "input_checkpoints=";
  for (std::size_t i = 0; i < input_keys.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << input_keys.at(i);
  }
  out << "\nfinite_loss=" << (fact.finite_loss ? "true" : "false") << "\n";
  out << "mean_loss=" << fact.mean_loss << "\n";
  out << "mean_nll=" << fact.mean_nll << "\n";
  out << "diagnostics_digest=" << fact.diagnostics_digest << "\n";
  return out.str();
}

[[nodiscard]] inline std::string
exposure_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string
exposure_fact_digest(const lattice_exposure_fact_t &fact) {
  return exposure_digest_for_text(canonical_exposure_fact_text(fact));
}

[[nodiscard]] inline lattice_exposure_fact_t
make_exposure_fact_from_sidecar_text(const std::string &text,
                                     const std::filesystem::path &job_dir) {
  namespace detail = exposure_detail;
  const auto map = detail::parse_assignment_text(text);
  lattice_exposure_fact_t out{};
  out.schema = detail::map_get(map, "schema", out.schema);
  out.fact_type = detail::map_get(map, "fact_type", out.fact_type);
  out.contract_fingerprint = detail::map_get(map, "contract_fingerprint");
  out.graph_order_fingerprint = detail::map_get(map, "graph_order_fingerprint");
  out.component_assembly_fingerprint =
      detail::map_get(map, "component_assembly_fingerprint");
  out.target_component = detail::map_get(map, "target_component");
  out.job_id = detail::map_get(map, "job_id");
  out.wave_id = detail::map_get(map, "wave_id");
  out.job_status = detail::map_get(map, "job_status");
  out.wave_action = detail::map_get(map, "wave_action");
  out.cursor_domain = detail::map_get(map, "cursor_domain", out.cursor_domain);
  out.source_cursor_token = detail::map_get(map, "source_cursor_token");
  out.anchor_range = anchor_interval_t{
      .begin =
          detail::parse_i64_fallback(detail::map_get(map, "anchor_begin"), 0),
      .end = detail::parse_i64_fallback(detail::map_get(map, "anchor_end"), 0),
  };
  out.observed_footprint = anchor_interval_t{
      .begin =
          detail::parse_i64_fallback(detail::map_get(map, "observed_begin"), 0),
      .end =
          detail::parse_i64_fallback(detail::map_get(map, "observed_end"), 0),
  };
  out.target_footprint = anchor_interval_t{
      .begin =
          detail::parse_i64_fallback(detail::map_get(map, "target_begin"), 0),
      .end = detail::parse_i64_fallback(detail::map_get(map, "target_end"), 0),
  };
  out.footprint_precision =
      detail::map_get(map, "footprint_precision", out.footprint_precision);
  out.first_anchor_key = detail::map_get(map, "first_anchor_key");
  out.last_anchor_key = detail::map_get(map, "last_anchor_key");
  out.observed_source_key_begin =
      detail::map_get(map, "observed_source_key_begin");
  out.observed_source_key_end = detail::map_get(map, "observed_source_key_end");
  out.target_source_key_begin = detail::map_get(map, "target_source_key_begin");
  out.target_source_key_end = detail::map_get(map, "target_source_key_end");
  out.source_key_footprint_precision =
      detail::map_get(map, "source_key_footprint_precision");
  out.source_file_receipts = detail::map_get(map, "source_file_receipts");
  out.source_input_length = detail::parse_i64_fallback(
      detail::map_get(map, "source_input_length"), 0);
  out.source_future_length = detail::parse_i64_fallback(
      detail::map_get(map, "source_future_length"), 0);
  out.candidate_anchor_count = detail::parse_i64_fallback(
      detail::map_get(map, "candidate_anchor_count"), 0);
  out.accepted_anchor_count = detail::parse_i64_fallback(
      detail::map_get(map, "accepted_anchor_count"), 0);
  out.accepted_anchor_fraction = detail::parse_double_fallback(
      detail::map_get(map, "accepted_anchor_fraction"),
      std::numeric_limits<double>::quiet_NaN());
  out.skipped_outside_common_range = detail::parse_i64_fallback(
      detail::map_get(map, "skipped_outside_common_range"), 0);
  out.skipped_missing_edge_coverage = detail::parse_i64_fallback(
      detail::map_get(map, "skipped_missing_edge_coverage"), 0);
  out.skipped_failed_fetch_probe = detail::parse_i64_fallback(
      detail::map_get(map, "skipped_failed_fetch_probe"), 0);
  out.duplicate_anchor_count = detail::parse_i64_fallback(
      detail::map_get(map, "duplicate_anchor_count"), 0);
  out.anchor_domain_warning_level =
      detail::map_get(map, "anchor_domain_warning_level", "none");
  out.anchor_domain_warnings = detail::map_get(map, "anchor_domain_warnings");
  out.common_left_key = detail::map_get(map, "common_left_key");
  out.common_right_key = detail::map_get(map, "common_right_key");
  out.reference_left_key = detail::map_get(map, "reference_left_key");
  out.reference_right_key = detail::map_get(map, "reference_right_key");
  detail::normalize_anchor_domain_health(out);
  out.split_name = detail::map_get(map, "split_name", out.split_name);
  out.split_role =
      parse_exposure_split_role(detail::map_get(map, "split_role", "unknown"));
  out.split_policy_fingerprint =
      detail::map_get(map, "split_policy_fingerprint");
  out.coverage_precision =
      detail::map_get(map, "coverage_precision", out.coverage_precision);
  out.completed_anchor_range = anchor_interval_t{
      .begin = detail::parse_i64_fallback(
          detail::map_get(map, "completed_anchor_begin"), 0),
      .end = detail::parse_i64_fallback(
          detail::map_get(map, "completed_anchor_end"), 0),
  };
  out.use.observed_input =
      detail::parse_bool_fallback(detail::map_get(map, "use_observed_input"));
  out.use.target_supervision = detail::parse_bool_fallback(
      detail::map_get(map, "use_target_supervision"));
  out.use.evaluation_metric = detail::parse_bool_fallback(
      detail::map_get(map, "use_evaluation_metric"));
  out.use.selection_signal =
      detail::parse_bool_fallback(detail::map_get(map, "use_selection_signal"));
  out.use.mutated_component =
      detail::parse_bool_fallback(detail::map_get(map, "mutated_component"));
  out.anchors_seen =
      detail::parse_i64_fallback(detail::map_get(map, "anchors_seen"), 0);
  out.batches_seen =
      detail::parse_i64_fallback(detail::map_get(map, "batches_seen"), 0);
  out.optimizer_steps =
      detail::parse_i64_fallback(detail::map_get(map, "optimizer_steps"), 0);
  out.wave_pulses_completed = detail::parse_i64_fallback(
      detail::map_get(map, "wave_pulses_completed"), 0);
  out.valid_target_count =
      detail::parse_i64_fallback(detail::map_get(map, "valid_target_count"), 0);
  out.valid_target_fraction = detail::parse_double_fallback(
      detail::map_get(map, "valid_target_fraction"),
      std::numeric_limits<double>::quiet_NaN());
  out.output_checkpoint = detail::resolve_job_path(
      job_dir, detail::map_get(map, "output_checkpoint"));
  out.input_checkpoints = detail::parse_checkpoint_path_list(
      detail::map_get(map, "input_checkpoints"), job_dir);
  out.finite_loss =
      detail::parse_bool_fallback(detail::map_get(map, "finite_loss"));
  out.mean_loss =
      detail::parse_double_fallback(detail::map_get(map, "mean_loss"),
                                    std::numeric_limits<double>::quiet_NaN());
  out.mean_nll =
      detail::parse_double_fallback(detail::map_get(map, "mean_nll"),
                                    std::numeric_limits<double>::quiet_NaN());
  out.diagnostics_digest = detail::map_get(map, "diagnostics_digest");
  return out;
}

[[nodiscard]] inline lattice_exposure_fact_t
make_exposure_fact_from_sidecar_file(const std::filesystem::path &path,
                                     const std::filesystem::path &job_dir) {
  const auto text = exposure_detail::read_text_file_or_empty(path);
  if (text.empty()) {
    throw std::runtime_error(
        "[lattice_exposure] exposure sidecar is empty or unreadable: " +
        path.string());
  }
  return make_exposure_fact_from_sidecar_text(text, job_dir);
}

[[nodiscard]] inline std::vector<lattice_node_exposure_fact_t>
make_node_exposure_facts_from_job_dir(const std::filesystem::path &job_dir,
                                      const lattice_exposure_fact_t &parent) {
  namespace detail = exposure_detail;
  if (parent.target_component != "wikimyei.inference.expected_value.mdn") {
    return {};
  }

  const auto report =
      detail::parse_assignment_file(job_dir / "inference.report");
  const auto manifest = detail::parse_assignment_file(job_dir / "job.manifest");
  if (report.empty() && manifest.empty()) {
    return {};
  }

  auto node_ids = detail::parse_string_list(detail::map_get(
      report, "node_ids", detail::map_get(manifest, "node_ids")));
  const auto routed_row_counts = detail::parse_i64_list_fallback(
      detail::map_get(report, "routed_row_count_by_node"));
  const auto active_row_counts = detail::parse_i64_list_fallback(
      detail::map_get(report, "active_row_count_by_node"));
  const auto trained_row_counts = detail::parse_i64_list_fallback(
      detail::map_get(report, "trained_row_count_by_node"));
  const auto evaluated_row_counts = detail::parse_i64_list_fallback(
      detail::map_get(report, "evaluated_row_count_by_node"));
  const auto valid_target_counts = detail::parse_i64_list_fallback(
      detail::map_get(report, "valid_target_count_by_node"));
  const auto mean_nlls = detail::parse_double_list_fallback(
      detail::map_get(report, "mean_nll_by_node"));
  const auto target_coords = detail::parse_i64_list_fallback(detail::map_get(
      report, "target_coords", detail::map_get(manifest, "target_coords")));
  const double target_coordinate_count =
      static_cast<double>(std::max<std::size_t>(target_coords.size(), 1));
  const double future_length = static_cast<double>(
      std::max<std::int64_t>(parent.source_future_length, 1));

  const std::size_t node_count = std::max(
      {node_ids.size(), routed_row_counts.size(), active_row_counts.size(),
       trained_row_counts.size(), evaluated_row_counts.size(),
       valid_target_counts.size(), mean_nlls.size()});
  if (node_count == 0) {
    return {};
  }
  while (node_ids.size() < node_count) {
    node_ids.push_back("node_" + std::to_string(node_ids.size()));
  }

  std::vector<lattice_node_exposure_fact_t> out;
  out.reserve(node_count);
  const auto parent_digest = exposure_fact_digest(parent);
  for (std::size_t i = 0; i < node_count; ++i) {
    lattice_node_exposure_fact_t fact{};
    fact.parent_exposure_fact_digest = parent_digest;
    fact.contract_fingerprint = parent.contract_fingerprint;
    fact.graph_order_fingerprint = parent.graph_order_fingerprint;
    fact.source_cursor_token = parent.source_cursor_token;
    fact.split_policy_fingerprint = parent.split_policy_fingerprint;
    fact.component_assembly_fingerprint = parent.component_assembly_fingerprint;
    fact.target_component = parent.target_component;
    fact.job_id = parent.job_id;
    fact.wave_id = parent.wave_id;
    fact.job_status = parent.job_status;
    fact.wave_action = parent.wave_action;
    fact.node_id = node_ids.at(i);
    fact.node_index = static_cast<std::int64_t>(i);
    fact.split_name = parent.split_name;
    fact.split_role = parent.split_role;
    fact.anchor_range = parent.anchor_range;
    fact.completed_anchor_range = parent.completed_anchor_range;
    fact.use = parent.use;
    if (i < routed_row_counts.size()) {
      fact.routed_row_count = routed_row_counts.at(i);
    }
    if (i < active_row_counts.size()) {
      fact.active_row_count = active_row_counts.at(i);
    }
    if (i < trained_row_counts.size()) {
      fact.trained_row_count = trained_row_counts.at(i);
    }
    if (i < evaluated_row_counts.size()) {
      fact.evaluated_row_count = evaluated_row_counts.at(i);
    }
    if (i < valid_target_counts.size()) {
      fact.valid_target_count = valid_target_counts.at(i);
    }
    std::int64_t support_rows = fact.active_row_count;
    if (support_rows <= 0 && active_row_counts.empty()) {
      support_rows = parent.completed_anchor_range.length();
    }
    const double possible_targets = static_cast<double>(support_rows) *
                                    target_coordinate_count * future_length;
    if (possible_targets > 0.0) {
      fact.valid_target_fraction =
          static_cast<double>(fact.valid_target_count) / possible_targets;
    }
    if (i < mean_nlls.size()) {
      fact.mean_nll = mean_nlls.at(i);
    }
    fact.output_checkpoint = parent.output_checkpoint;
    out.push_back(std::move(fact));
  }
  return out;
}

[[nodiscard]] inline std::string
file_content_digest(const std::filesystem::path &path) {
  const auto text = exposure_detail::read_text_file_or_empty(path);
  if (text.empty()) {
    return {};
  }
  return exposure_digest_for_text(std::string("file:") + text);
}

[[nodiscard]] inline lattice_checkpoint_fact_t
make_checkpoint_fact_from_exposure_fact(const lattice_exposure_fact_t &fact) {
  lattice_checkpoint_fact_t out{};
  out.checkpoint_path = fact.output_checkpoint.lexically_normal();
  out.checkpoint_file_digest = file_content_digest(out.checkpoint_path);
  out.component = fact.target_component;
  out.contract_fingerprint = fact.contract_fingerprint;
  out.graph_order_fingerprint = fact.graph_order_fingerprint;
  out.source_cursor_token = fact.source_cursor_token;
  out.component_assembly_fingerprint = fact.component_assembly_fingerprint;
  out.created_by_job_id = fact.job_id;
  out.created_by_wave_id = fact.wave_id;
  out.created_by_exposure_fact_id = exposure_fact_digest(fact);
  out.input_checkpoints = fact.input_checkpoints;
  out.direct_exposure_digest = exposure_fact_digest(fact);
  out.checkpoint_id = exposure_digest_for_text(
      std::string("checkpoint|") + out.checkpoint_path.string() + "|" +
      out.checkpoint_file_digest + "|" + out.direct_exposure_digest);
  return out;
}

[[nodiscard]] inline std::string
canonical_checkpoint_fact_text(const lattice_checkpoint_fact_t &fact) {
  std::ostringstream out;
  out << "schema=" << fact.schema << "\n";
  out << "fact_type=" << fact.fact_type << "\n";
  out << "checkpoint_id=" << fact.checkpoint_id << "\n";
  out << "checkpoint_path=" << fact.checkpoint_path.lexically_normal().string()
      << "\n";
  out << "checkpoint_file_digest=" << fact.checkpoint_file_digest << "\n";
  out << "component=" << fact.component << "\n";
  out << "contract_fingerprint=" << fact.contract_fingerprint << "\n";
  out << "graph_order_fingerprint=" << fact.graph_order_fingerprint << "\n";
  out << "source_cursor_token=" << fact.source_cursor_token << "\n";
  out << "component_assembly_fingerprint="
      << fact.component_assembly_fingerprint << "\n";
  out << "created_by_job_id=" << fact.created_by_job_id << "\n";
  out << "created_by_wave_id=" << fact.created_by_wave_id << "\n";
  out << "created_by_exposure_fact_id=" << fact.created_by_exposure_fact_id
      << "\n";
  std::vector<std::string> input_keys;
  input_keys.reserve(fact.input_checkpoints.size());
  for (const auto &checkpoint : fact.input_checkpoints) {
    input_keys.push_back(checkpoint.lexically_normal().string());
  }
  std::sort(input_keys.begin(), input_keys.end());
  out << "input_checkpoints=";
  for (std::size_t i = 0; i < input_keys.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << input_keys.at(i);
  }
  out << "\n";
  out << "direct_exposure_digest=" << fact.direct_exposure_digest << "\n";
  out << "closure_digest=" << fact.closure_digest << "\n";
  return out.str();
}

[[nodiscard]] inline lattice_checkpoint_fact_t
make_checkpoint_fact_from_sidecar_text(const std::string &text,
                                       const std::filesystem::path &job_dir) {
  namespace detail = exposure_detail;
  const auto map = detail::parse_assignment_text(text);
  lattice_checkpoint_fact_t out{};
  out.schema = detail::map_get(map, "schema", out.schema);
  out.fact_type = detail::map_get(map, "fact_type", out.fact_type);
  out.checkpoint_id = detail::map_get(map, "checkpoint_id");
  out.checkpoint_path = detail::resolve_job_path(
      job_dir, detail::map_get(map, "checkpoint_path"));
  out.checkpoint_file_digest = detail::map_get(map, "checkpoint_file_digest");
  out.component = detail::map_get(map, "component");
  out.contract_fingerprint = detail::map_get(map, "contract_fingerprint");
  out.graph_order_fingerprint = detail::map_get(map, "graph_order_fingerprint");
  out.source_cursor_token = detail::map_get(map, "source_cursor_token");
  out.component_assembly_fingerprint =
      detail::map_get(map, "component_assembly_fingerprint");
  out.created_by_job_id = detail::map_get(map, "created_by_job_id");
  out.created_by_wave_id = detail::map_get(map, "created_by_wave_id");
  out.created_by_exposure_fact_id =
      detail::map_get(map, "created_by_exposure_fact_id");
  out.input_checkpoints = detail::parse_checkpoint_path_list(
      detail::map_get(map, "input_checkpoints"), job_dir);
  out.direct_exposure_digest = detail::map_get(map, "direct_exposure_digest");
  out.closure_digest = detail::map_get(map, "closure_digest");
  return out;
}

[[nodiscard]] inline lattice_checkpoint_fact_t
make_checkpoint_fact_from_sidecar_file(const std::filesystem::path &path,
                                       const std::filesystem::path &job_dir) {
  const auto text = exposure_detail::read_text_file_or_empty(path);
  if (text.empty()) {
    throw std::runtime_error(
        "[lattice_exposure] checkpoint sidecar is empty or unreadable: " +
        path.string());
  }
  return make_checkpoint_fact_from_sidecar_text(text, job_dir);
}

inline void write_text_file_atomically(const std::filesystem::path &path,
                                       const std::string &text) {
  const auto tmp = path.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc | std::ios::binary);
    if (!out) {
      throw std::runtime_error(
          "[lattice_exposure] failed to open sidecar tmp: " + tmp);
    }
    out << text;
    if (!out) {
      throw std::runtime_error(
          "[lattice_exposure] failed to write sidecar tmp: " + tmp);
    }
  }
  std::filesystem::rename(tmp, path);
}

inline void write_exposure_fact_sidecar(const std::filesystem::path &path,
                                        const lattice_exposure_fact_t &fact) {
  const auto canonical = canonical_exposure_fact_text(fact);
  std::ostringstream text;
  text << canonical << "fact_digest=" << exposure_digest_for_text(canonical)
       << "\n";
  write_text_file_atomically(path, text.str());
}

inline void write_checkpoint_fact_sidecar(const std::filesystem::path &path,
                                          lattice_checkpoint_fact_t fact) {
  const auto canonical_without_digest = canonical_checkpoint_fact_text(fact);
  const auto digest = exposure_digest_for_text(canonical_without_digest);
  std::ostringstream text;
  text << canonical_without_digest << "fact_digest=" << digest << "\n";
  write_text_file_atomically(path, text.str());
}

class lattice_exposure_ledger_t {
public:
  void add(lattice_exposure_fact_t fact) { facts_.push_back(std::move(fact)); }

  void add_node(lattice_node_exposure_fact_t fact) {
    node_facts_.push_back(std::move(fact));
  }

  void add_checkpoint(lattice_checkpoint_fact_t fact) {
    checkpoint_facts_.push_back(std::move(fact));
  }

  [[nodiscard]] const std::vector<lattice_exposure_fact_t> &facts() const {
    return facts_;
  }

  [[nodiscard]] const std::vector<lattice_checkpoint_fact_t> &
  checkpoint_facts() const {
    return checkpoint_facts_;
  }

  [[nodiscard]] const std::vector<lattice_node_exposure_fact_t> &
  node_facts() const {
    return node_facts_;
  }

  [[nodiscard]] std::vector<lattice_exposure_fact_t>
  checkpoint_closure(const std::filesystem::path &checkpoint) const {
    return checkpoint_closure_result(checkpoint).facts;
  }

  [[nodiscard]] checkpoint_closure_result_t
  checkpoint_closure_result(const std::filesystem::path &checkpoint) const {
    checkpoint_closure_result_t out{};
    std::unordered_set<std::string> seen_checkpoints;
    std::unordered_set<std::string> seen_facts;
    checkpoint_closure_impl(exposure_detail::checkpoint_key(checkpoint), out,
                            seen_checkpoints, seen_facts, /*is_root=*/true);
    return out;
  }

  [[nodiscard]] std::string
  checkpoint_closure_digest(const std::filesystem::path &checkpoint) const {
    auto closure = checkpoint_closure(checkpoint);
    std::vector<std::string> canonical;
    canonical.reserve(closure.size());
    for (const auto &fact : closure) {
      canonical.push_back(canonical_exposure_fact_text(fact));
    }
    std::sort(canonical.begin(), canonical.end());
    std::ostringstream out;
    for (const auto &text : canonical) {
      out << text << "\n";
    }
    return exposure_digest_for_text(out.str());
  }

private:
  void checkpoint_closure_impl(
      const std::string &checkpoint_key, checkpoint_closure_result_t &out,
      std::unordered_set<std::string> &seen_checkpoints,
      std::unordered_set<std::string> &seen_facts, bool is_root) const {
    if (checkpoint_key.empty() ||
        !seen_checkpoints.insert(checkpoint_key).second) {
      return;
    }
    bool found_producer = false;
    for (const auto &fact : facts_) {
      if (exposure_detail::checkpoint_key(fact.output_checkpoint) !=
          checkpoint_key) {
        continue;
      }
      found_producer = true;
      const auto fact_key = fact.job_id + "|" + exposure_fact_digest(fact);
      if (seen_facts.insert(fact_key).second) {
        out.facts.push_back(fact);
      }
      for (const auto &input_checkpoint : fact.input_checkpoints) {
        checkpoint_closure_impl(
            exposure_detail::checkpoint_key(input_checkpoint), out,
            seen_checkpoints, seen_facts, /*is_root=*/false);
      }
    }
    if (!found_producer && !is_root) {
      out.unresolved_input_checkpoints.emplace_back(checkpoint_key);
    }
  }

  std::vector<lattice_exposure_fact_t> facts_{};
  std::vector<lattice_node_exposure_fact_t> node_facts_{};
  std::vector<lattice_checkpoint_fact_t> checkpoint_facts_{};
};

struct exposure_ledger_scan_result_t {
  lattice_exposure_ledger_t ledger{};
  std::vector<std::string> warnings{};
};

inline void
append_anchor_domain_scan_warning(const lattice_exposure_fact_t &fact,
                                  const std::filesystem::path &job_dir,
                                  std::vector<std::string> &warnings) {
  if (fact.anchor_domain_warning_level.empty() ||
      fact.anchor_domain_warning_level == "none") {
    return;
  }
  std::ostringstream oss;
  oss << "[lattice_exposure] anchor domain " << fact.anchor_domain_warning_level
      << " job_dir=" << job_dir.string()
      << " accepted=" << fact.accepted_anchor_count
      << " candidate=" << fact.candidate_anchor_count
      << " fraction=" << fact.accepted_anchor_fraction
      << " skipped_outside_common_range=" << fact.skipped_outside_common_range
      << " skipped_missing_edge_coverage=" << fact.skipped_missing_edge_coverage
      << " skipped_failed_fetch_probe=" << fact.skipped_failed_fetch_probe;
  if (!fact.anchor_domain_warnings.empty()) {
    oss << " warnings=" << fact.anchor_domain_warnings;
  }
  warnings.push_back(oss.str());
}

[[nodiscard]] inline exposure_ledger_scan_result_t
scan_exposure_ledger_from_runtime_root(
    const std::filesystem::path &runtime_root,
    exposure_build_context_t context = {}) {
  namespace fs = std::filesystem;
  exposure_ledger_scan_result_t out{};
  std::error_code ec;
  if (!fs::is_directory(runtime_root, ec)) {
    out.warnings.push_back("[lattice_exposure] runtime root is not a "
                           "directory: " +
                           runtime_root.string());
    return out;
  }
  const auto process_job_dir = [&](const fs::path &job_dir) {
    try {
      const auto sidecar = exposure_fact_path_for_job_dir(job_dir);
      if (fs::exists(sidecar)) {
        auto sidecar_fact =
            make_exposure_fact_from_sidecar_file(sidecar, job_dir);
        if (sidecar_fact.split_policy_fingerprint.empty() &&
            !context.split_policy_fingerprint.empty()) {
          sidecar_fact.split_policy_fingerprint =
              context.split_policy_fingerprint;
        }
        try {
          const auto derived_fact =
              make_exposure_fact_from_job_dir(job_dir, context);
          const auto sidecar_digest = exposure_fact_digest(sidecar_fact);
          const auto derived_digest = exposure_fact_digest(derived_fact);
          if (sidecar_digest != derived_digest) {
            out.warnings.push_back(
                "[lattice_exposure] sidecar digest differs from derived "
                "runtime artifact fact for job_dir=" +
                job_dir.string());
          }
        } catch (const std::exception &ex) {
          out.warnings.push_back(
              "[lattice_exposure] sidecar used but derived fallback failed for "
              "job_dir=" +
              job_dir.string() + " reason=" + ex.what());
        }
        append_anchor_domain_scan_warning(sidecar_fact, job_dir, out.warnings);
        for (auto node_fact :
             make_node_exposure_facts_from_job_dir(job_dir, sidecar_fact)) {
          out.ledger.add_node(std::move(node_fact));
        }
        out.ledger.add(std::move(sidecar_fact));
      } else {
        auto fact = make_exposure_fact_from_job_dir(job_dir, context);
        append_anchor_domain_scan_warning(fact, job_dir, out.warnings);
        for (auto node_fact :
             make_node_exposure_facts_from_job_dir(job_dir, fact)) {
          out.ledger.add_node(std::move(node_fact));
        }
        out.ledger.add(std::move(fact));
      }
      const auto checkpoint_sidecar = checkpoint_fact_path_for_job_dir(job_dir);
      if (fs::exists(checkpoint_sidecar)) {
        try {
          out.ledger.add_checkpoint(make_checkpoint_fact_from_sidecar_file(
              checkpoint_sidecar, job_dir));
        } catch (const std::exception &ex) {
          out.warnings.push_back(
              "[lattice_exposure] skipped checkpoint sidecar job_dir=" +
              job_dir.string() + " reason=" + ex.what());
        }
      }
    } catch (const std::exception &ex) {
      out.warnings.push_back("[lattice_exposure] skipped job_dir=" +
                             job_dir.string() + " reason=" + ex.what());
    }
  };
  if (fs::exists(runtime_root / "job.manifest")) {
    process_job_dir(runtime_root);
    return out;
  }
  for (fs::directory_iterator it(runtime_root, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (!it->is_directory(ec)) {
      continue;
    }
    if (!fs::exists(it->path() / "job.manifest")) {
      continue;
    }
    process_job_dir(it->path());
  }
  if (ec) {
    out.warnings.push_back("[lattice_exposure] runtime root scan stopped: " +
                           ec.message());
  }
  return out;
}

[[nodiscard]] inline bool has_forbidden_exposure_overlap(
    const std::vector<lattice_exposure_fact_t> &facts,
    const forbidden_exposure_query_t &query) {
  for (const auto &fact : facts) {
    if (query.require_mutated_component && !fact.use.mutated_component) {
      continue;
    }
    for (const auto use : query.forbidden_uses) {
      if (fact_has_use(fact, use) &&
          intervals_overlap(source_footprint_for_use(fact, use),
                            query.forbidden_range)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] inline exposure_coverage_t
coverage_for_use(const std::vector<lattice_exposure_fact_t> &facts,
                 anchor_interval_t target_range, exposure_use_t use,
                 bool require_mutated_component = true) {
  std::vector<anchor_interval_t> intervals;
  for (const auto &fact : facts) {
    if (require_mutated_component && !fact.use.mutated_component) {
      continue;
    }
    if (!fact_has_use(fact, use)) {
      continue;
    }
    auto interval =
        interval_intersection(anchor_coverage_for_use(fact, use), target_range);
    if (!interval.empty()) {
      intervals.push_back(interval);
    }
  }
  std::sort(intervals.begin(), intervals.end(),
            [](anchor_interval_t a, anchor_interval_t b) {
              return a.begin == b.begin ? a.end < b.end : a.begin < b.begin;
            });
  std::vector<anchor_interval_t> merged;
  for (const auto interval : intervals) {
    if (merged.empty() || merged.back().end < interval.begin) {
      merged.push_back(interval);
    } else {
      merged.back().end = std::max(merged.back().end, interval.end);
    }
  }
  std::int64_t covered = 0;
  for (const auto interval : merged) {
    covered += interval.length();
  }
  return exposure_coverage_t{
      .target_range = target_range,
      .covered_anchors = covered,
      .coverage_fraction = target_range.empty()
                               ? 0.0
                               : static_cast<double>(covered) /
                                     static_cast<double>(target_range.length()),
  };
}

[[nodiscard]] inline exposure_load_summary_t
exposure_load_for_use(const std::vector<lattice_exposure_fact_t> &facts,
                      anchor_interval_t target_range, exposure_use_t use,
                      bool require_mutated_component = true,
                      std::string target_component_filter = {}) {
  exposure_load_summary_t out{};
  out.target_range = target_range;
  out.use = use;
  out.require_mutated_component = require_mutated_component;
  out.target_component = std::move(target_component_filter);
  out.component_scope =
      out.target_component.empty() ? "all_components" : "target_component";

  std::vector<anchor_interval_t> intervals;
  for (const auto &fact : facts) {
    if (!out.target_component.empty() &&
        fact.target_component != out.target_component) {
      continue;
    }
    if (require_mutated_component && !fact.use.mutated_component) {
      continue;
    }
    if (!fact_has_use(fact, use)) {
      continue;
    }
    const auto interval =
        interval_intersection(anchor_coverage_for_use(fact, use), target_range);
    if (interval.empty()) {
      continue;
    }

    intervals.push_back(interval);
    out.loaded_anchor_events += interval.length();
    ++out.fact_count;
    out.optimizer_steps_total += fact.optimizer_steps;
    if (fact.valid_target_count > 0) {
      out.valid_target_count_total += fact.valid_target_count;
    }
    if (!target_range.empty() && std::isfinite(fact.valid_target_fraction)) {
      out.valid_target_cursor_epochs +=
          fact.valid_target_fraction *
          (static_cast<double>(interval.length()) /
           static_cast<double>(target_range.length()));
    }
  }

  std::sort(intervals.begin(), intervals.end(),
            [](anchor_interval_t a, anchor_interval_t b) {
              return a.begin == b.begin ? a.end < b.end : a.begin < b.begin;
            });
  std::vector<anchor_interval_t> merged;
  for (const auto interval : intervals) {
    if (merged.empty() || merged.back().end < interval.begin) {
      merged.push_back(interval);
    } else {
      merged.back().end = std::max(merged.back().end, interval.end);
    }
  }
  for (const auto interval : merged) {
    out.unique_covered_anchors += interval.length();
  }

  if (!target_range.empty()) {
    const auto target_length = static_cast<double>(target_range.length());
    out.unique_coverage_fraction =
        static_cast<double>(out.unique_covered_anchors) / target_length;
    out.cursor_exposure_load =
        static_cast<double>(out.loaded_anchor_events) / target_length;
  }
  if (out.unique_covered_anchors > 0) {
    out.optimizer_steps_per_unique_anchor =
        static_cast<double>(out.optimizer_steps_total) /
        static_cast<double>(out.unique_covered_anchors);
  }
  if (out.cursor_exposure_load > 0.0) {
    out.optimizer_steps_per_cursor_epoch =
        static_cast<double>(out.optimizer_steps_total) /
        out.cursor_exposure_load;
  }
  return out;
}

} // namespace cuwacunu::kikijyeba::lattice::exposure
