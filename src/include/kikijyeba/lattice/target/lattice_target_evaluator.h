// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "kikijyeba/lattice/split/split_policy.h"
#include "kikijyeba/lattice/target/lattice_target.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::lattice::target {

struct lattice_target_active_identity_t {
  std::string protocol_contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string vicreg_assembly_fingerprint{};
  std::string mdn_assembly_fingerprint{};
};

struct lattice_target_evaluator_options_t {
  std::filesystem::path runtime_root{"/cuwacunu/.runtime/cuwacunu_exec"};
  lattice_target_active_identity_t active_identity{};
  const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_ledger_t
      *exposure_ledger{nullptr};
  std::optional<cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
      split_policy{std::nullopt};
  bool auto_build_exposure_ledger{true};
};

struct lattice_target_config_paths_t {
  std::filesystem::path splits_dsl_bnf_path{};
  std::filesystem::path splits_dsl_path{};
  std::filesystem::path targets_dsl_bnf_path{};
  std::filesystem::path targets_dsl_path{};
};

namespace lattice_target_eval_detail {

namespace fs = std::filesystem;
namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string read_text_file_or_throw(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[lattice_target] unable to open file: " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::optional<std::string>
read_text_file_if_exists(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    return std::nullopt;
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

[[nodiscard]] inline std::filesystem::path
resolve_against(const std::filesystem::path &base_file,
                const std::string &value) {
  const fs::path path(value);
  if (path.is_absolute()) {
    return path;
  }
  return base_file.parent_path() / path;
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_file_if_exists(const fs::path &path) {
  const auto text = read_text_file_if_exists(path);
  return text.has_value() ? parse_assignment_text(*text)
                          : std::unordered_map<std::string, std::string>{};
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

[[nodiscard]] inline double
first_finite_double(const std::unordered_map<std::string, std::string> &a,
                    const std::unordered_map<std::string, std::string> &b,
                    const std::vector<std::string> &keys) {
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
  return std::numeric_limits<double>::quiet_NaN();
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

[[nodiscard]] inline fs::path newest_marker_path(const fs::path &dir) {
  const auto state = dir / "job.state";
  if (fs::exists(state)) {
    return state;
  }
  return dir / "job.manifest";
}

[[nodiscard]] inline std::string
expected_component_fingerprint(const lattice_target_active_identity_t &identity,
                               lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::representation_ready:
    return identity.vicreg_assembly_fingerprint;
  case lattice_target_kind_t::node_mdn_ready:
    return identity.mdn_assembly_fingerprint;
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string active_split_policy_fingerprint(
    const std::optional<
        cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
        &split_policy) {
  if (!split_policy.has_value()) {
    return {};
  }
  return cuwacunu::kikijyeba::lattice::split::lattice_split_policy_fingerprint(
      *split_policy);
}

[[nodiscard]] inline lattice_suggested_wave_t
make_suggested_wave(const lattice_target_spec_t &spec) {
  lattice_suggested_wave_t out{};
  out.target = spec.component;
  out.mode = spec.plan_mode;
  out.source_range = spec.source_range;
  out.anchor_index_begin = spec.anchor_index_begin;
  out.anchor_index_end = spec.anchor_index_end;
  out.input_mdn_checkpoint = spec.plan_input_mdn_checkpoint;
  out.input_representation_checkpoint =
      spec.plan_input_representation_checkpoint;
  return out;
}

[[nodiscard]] inline lattice_target_spec_t resolve_split_references(
    lattice_target_spec_t spec,
    const std::optional<
        cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
        &split_policy) {
  const bool has_warning_split =
      std::any_of(spec.warning_specs.begin(), spec.warning_specs.end(),
                  [](const lattice_warning_spec_t &warning) {
                    return !warning.split.empty();
                  });
  if (spec.train_split.empty() && spec.forbid_split.empty() &&
      !has_warning_split) {
    return spec;
  }
  if (!split_policy.has_value()) {
    throw std::runtime_error(
        "[lattice_target] target references lattice split names but no split "
        "policy was loaded");
  }
  if (!spec.train_split.empty()) {
    const auto *split = split_policy->find_split(spec.train_split);
    if (split == nullptr) {
      throw std::runtime_error("[lattice_target] unknown TRAIN_SPLIT: " +
                               spec.train_split);
    }
    spec.source_range = "anchor_index";
    spec.anchor_index_begin =
        static_cast<std::size_t>(split->anchor_range.begin);
    spec.anchor_index_end = static_cast<std::size_t>(split->anchor_range.end);
  }
  if (!spec.forbid_split.empty()) {
    const auto *split = split_policy->find_split(spec.forbid_split);
    if (split == nullptr) {
      throw std::runtime_error("[lattice_target] unknown FORBID_SPLIT: " +
                               spec.forbid_split);
    }
    if (spec.forbid_exposure_uses_from_split_policy) {
      if (split->protect_from_uses.empty()) {
        throw std::runtime_error(
            "[lattice_target] PROTECT_SPLIT references split without "
            "PROTECT_FROM_USES: " +
            spec.forbid_split);
      }
      spec.forbid_exposure_uses = split->protect_from_uses;
      spec.forbid_exposure_requires_mutated_component =
          split->protect_requires_mutated_component;
      spec.forbid_exposure_uses_from_split_policy = false;
    }
    spec.forbid_exposure_anchor_index_begin =
        static_cast<std::size_t>(split->anchor_range.begin);
    spec.forbid_exposure_anchor_index_end =
        static_cast<std::size_t>(split->anchor_range.end);
  }
  for (auto &warning : spec.warning_specs) {
    if (warning.split.empty()) {
      continue;
    }
    const auto *split = split_policy->find_split(warning.split);
    if (split == nullptr) {
      throw std::runtime_error("[lattice_target] unknown LATTICE_WARN SPLIT: " +
                               warning.split);
    }
    if (warning.anchor_index_begin.has_value() ||
        warning.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN must use either SPLIT or explicit "
          "ANCHOR_INDEX_BEGIN/END, not both");
    }
    warning.anchor_index_begin =
        static_cast<std::size_t>(split->anchor_range.begin);
    warning.anchor_index_end =
        static_cast<std::size_t>(split->anchor_range.end);
  }
  return spec;
}

[[nodiscard]] inline bool
target_has_exposure_requirements(const lattice_target_spec_t &spec) {
  return spec.min_observed_input_coverage > 0.0 ||
         spec.min_target_supervision_coverage > 0.0 ||
         spec.min_evaluation_metric_coverage > 0.0 ||
         spec.forbid_exposure_anchor_index_begin.has_value() ||
         !spec.forbid_split.empty() || !spec.warning_specs.empty();
}

[[nodiscard]] inline std::string latest_satisfying_checkpoint_source_target_id(
    const lattice_target_spec_t &spec) {
  const std::string prefix = "latest_satisfying:";
  const auto source = kv::trim(spec.checkpoint_source);
  if (kv::lowercase(source).rfind(prefix, 0) != 0) {
    return {};
  }
  return kv::trim(source.substr(prefix.size()));
}

[[nodiscard]] inline std::string
latest_satisfying_target_id_from_reference(const std::string &source) {
  const std::string prefix = "latest_satisfying:";
  const auto trimmed = kv::trim(source);
  if (kv::lowercase(trimmed).rfind(prefix, 0) != 0) {
    return {};
  }
  return kv::trim(trimmed.substr(prefix.size()));
}

[[nodiscard]] inline bool
target_requires_graph_anchor_identity(const lattice_target_spec_t &spec) {
  return target_has_exposure_requirements(spec) ||
         kv::lowercase(kv::trim(std::string(spec.source_range))) ==
             "anchor_index";
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t
target_anchor_interval(const lattice_target_spec_t &spec) {
  if (!spec.anchor_index_begin.has_value() ||
      !spec.anchor_index_end.has_value()) {
    return {};
  }
  return cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t{
      .begin = static_cast<std::int64_t>(*spec.anchor_index_begin),
      .end = static_cast<std::int64_t>(*spec.anchor_index_end),
  };
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t
forbidden_exposure_interval(const lattice_target_spec_t &spec) {
  if (!spec.forbid_exposure_anchor_index_begin.has_value() ||
      !spec.forbid_exposure_anchor_index_end.has_value()) {
    return {};
  }
  return cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t{
      .begin =
          static_cast<std::int64_t>(*spec.forbid_exposure_anchor_index_begin),
      .end = static_cast<std::int64_t>(*spec.forbid_exposure_anchor_index_end),
  };
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t
warning_anchor_interval(const lattice_target_spec_t &spec,
                        const lattice_warning_spec_t &warning) {
  if (warning.anchor_index_begin.has_value() &&
      warning.anchor_index_end.has_value()) {
    return cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t{
        .begin = static_cast<std::int64_t>(*warning.anchor_index_begin),
        .end = static_cast<std::int64_t>(*warning.anchor_index_end),
    };
  }
  return target_anchor_interval(spec);
}

[[nodiscard]] inline bool split_purge_policy_enabled(std::string value) {
  value = kv::lowercase(kv::trim(std::move(value)));
  return value.empty() || value == "auto_from_hx" || value == "auto_from_hf" ||
         value == "auto";
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t
expanded_forbidden_exposure_interval(
    const lattice_target_spec_t &spec,
    const cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t
        &split_policy,
    const std::vector<
        cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
        &facts) {
  namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
  auto out = forbidden_exposure_interval(spec);
  if (out.empty() || spec.forbid_split.empty()) {
    return out;
  }
  std::int64_t left_context = 0;
  std::int64_t right_future = 0;
  for (const auto &fact : facts) {
    left_context =
        std::max(left_context, exposure::left_context_rows_for_fact(fact));
    right_future =
        std::max(right_future, exposure::right_future_rows_for_fact(fact));
  }
  if (split_purge_policy_enabled(split_policy.purge_left_context)) {
    out.begin -= left_context;
  }
  if (split_purge_policy_enabled(split_policy.purge_right_future)) {
    out.end += right_future;
  }
  return out;
}

struct job_candidate_t {
  fs::path dir{};
  fs::file_time_type time{};
  std::unordered_map<std::string, std::string> manifest{};
};

struct evaluated_checkpoint_binding_t {
  std::string source_target_id{};
  fs::path mdn_checkpoint_path{};
  fs::path representation_checkpoint_path{};
};

[[nodiscard]] inline bool same_model_state_path(const fs::path &a,
                                                const fs::path &b) {
  return a.lexically_normal().string() == b.lexically_normal().string();
}

[[nodiscard]] inline std::vector<job_candidate_t>
find_job_candidates(const fs::path &runtime_root,
                    const lattice_target_spec_t &spec) {
  std::vector<job_candidate_t> out;
  std::error_code ec;
  if (!fs::is_directory(runtime_root, ec)) {
    return out;
  }
  for (fs::directory_iterator it(runtime_root, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (!it->is_directory(ec)) {
      continue;
    }
    const fs::path manifest_path = it->path() / "job.manifest";
    const auto manifest = parse_assignment_file_if_exists(manifest_path);
    if (manifest.empty()) {
      continue;
    }
    if (map_get(manifest, "job_kind") != job_kind_for_target_kind(spec.kind)) {
      continue;
    }
    if (map_get(manifest, "target_component") != spec.component) {
      continue;
    }
    if (!spec.source_range.empty() &&
        map_get(manifest, "source_range_policy") != spec.source_range) {
      continue;
    }
    if (spec.anchor_index_begin.has_value() &&
        parse_i64_fallback(map_get(manifest, "resolved_anchor_index_begin"),
                           -1) !=
            static_cast<std::int64_t>(*spec.anchor_index_begin)) {
      continue;
    }
    if (spec.anchor_index_end.has_value() &&
        parse_i64_fallback(map_get(manifest, "resolved_anchor_index_end"),
                           -1) !=
            static_cast<std::int64_t>(*spec.anchor_index_end)) {
      continue;
    }
    std::error_code time_ec;
    auto marker_time =
        fs::last_write_time(newest_marker_path(it->path()), time_ec);
    if (time_ec) {
      marker_time = fs::file_time_type::min();
    }
    out.push_back(job_candidate_t{
        .dir = it->path(),
        .time = marker_time,
        .manifest = manifest,
    });
  }
  std::sort(out.begin(), out.end(),
            [](const job_candidate_t &a, const job_candidate_t &b) {
              return a.time > b.time;
            });
  return out;
}

[[nodiscard]] inline lattice_target_evidence_t
make_evidence_from_candidate(const job_candidate_t &candidate,
                             const lattice_target_spec_t &spec,
                             std::int64_t matching_job_count,
                             std::int64_t matching_train_attempt_count) {
  lattice_target_evidence_t out{};
  out.job_dir = candidate.dir;
  out.manifest_path = candidate.dir / "job.manifest";
  out.state_path = candidate.dir / "job.state";
  out.report_path =
      candidate.dir / (spec.kind == lattice_target_kind_t::node_mdn_ready
                           ? "inference.report"
                           : "representation.report");
  out.report_exists = fs::exists(out.report_path);
  const auto state = parse_assignment_file_if_exists(out.state_path);
  const auto report = parse_assignment_file_if_exists(out.report_path);

  out.protocol_contract_fingerprint =
      map_get(candidate.manifest, "protocol_contract_fingerprint");
  out.component_fingerprint = map_get(
      candidate.manifest, component_fingerprint_key_for_target_kind(spec.kind));
  out.graph_order_fingerprint =
      map_get(candidate.manifest, "graph_order_fingerprint");
  out.source_cursor_token = map_get(candidate.manifest, "source_cursor_token");
  out.status = map_get(state, "status");
  out.matching_job_count = matching_job_count;
  out.matching_train_attempt_count = matching_train_attempt_count;
  out.optimizer_steps =
      first_i64(state, report, {"optimizer_steps"}, /*fallback=*/0);
  out.loss = first_finite_double(state, report, {"last_loss", "mean_loss"});
  out.valid_target_fraction = first_finite_double(
      report, state,
      {"mean_valid_node_target_fraction", "last_valid_node_target_fraction"});
  out.active_node_head_count =
      first_i64(report, state,
                {"last_active_node_head_count", "total_active_node_head_count"},
                /*fallback=*/0);
  out.trained_node_head_count = first_i64(
      report, state,
      {"last_trained_node_head_count", "total_trained_node_head_count"},
      /*fallback=*/0);
  out.evaluated_node_head_count = first_i64(
      report, state,
      {"last_evaluated_node_head_count", "total_evaluated_node_head_count"},
      /*fallback=*/0);
  out.checkpoint_written =
      parse_bool_fallback(map_get(state, "checkpoint_written"), false) ||
      parse_bool_fallback(map_get(report, "checkpoint_written"), false) ||
      first_i64(state, report, {"checkpoint_write_count"}, 0) > 0;
  const auto checkpoint_path = !map_get(state, "checkpoint_path").empty()
                                   ? map_get(state, "checkpoint_path")
                                   : map_get(report, "checkpoint_path");
  if (!checkpoint_path.empty()) {
    const fs::path raw_checkpoint_path(checkpoint_path);
    out.checkpoint_path = raw_checkpoint_path.is_absolute()
                              ? raw_checkpoint_path
                              : candidate.dir / raw_checkpoint_path;
  }
  const auto representation_checkpoint_path =
      map_get(report, "representation_checkpoint_path");
  if (!representation_checkpoint_path.empty()) {
    const fs::path raw_representation_checkpoint_path(
        representation_checkpoint_path);
    out.representation_checkpoint_path =
        raw_representation_checkpoint_path.is_absolute()
            ? raw_representation_checkpoint_path
            : candidate.dir / raw_representation_checkpoint_path;
  }
  out.representation_checkpoint_loaded = parse_bool_fallback(
      map_get(report, "representation_checkpoint_loaded"), false);
  const auto mdn_checkpoint_path = map_get(report, "mdn_checkpoint_path");
  if (!mdn_checkpoint_path.empty()) {
    const fs::path raw_mdn_checkpoint_path(mdn_checkpoint_path);
    out.mdn_checkpoint_path = raw_mdn_checkpoint_path.is_absolute()
                                  ? raw_mdn_checkpoint_path
                                  : candidate.dir / raw_mdn_checkpoint_path;
  }
  out.mdn_checkpoint_loaded =
      parse_bool_fallback(map_get(report, "mdn_checkpoint_loaded"), false);
  out.allow_untrained_representation = parse_bool_fallback(
      map_get(report, "allow_untrained_representation"), false);
  return out;
}

[[nodiscard]] inline bool candidate_matches_active_identity_for_attempt_budget(
    const job_candidate_t &candidate, const lattice_target_spec_t &spec,
    const lattice_target_active_identity_t &active_identity,
    const std::string &expected_component_fingerprint) {
  const auto state =
      parse_assignment_file_if_exists(candidate.dir / "job.state");
  const auto status = map_get(state, "status");
  if (status != "completed" && status != "failed") {
    return false;
  }
  if (map_get(candidate.manifest, "wave_action",
              map_get(state, "wave_action")) != "train") {
    return false;
  }
  if (spec.require_contract_match &&
      map_get(candidate.manifest, "protocol_contract_fingerprint") !=
          active_identity.protocol_contract_fingerprint) {
    return false;
  }
  if (spec.require_component_match &&
      map_get(candidate.manifest,
              component_fingerprint_key_for_target_kind(spec.kind)) !=
          expected_component_fingerprint) {
    return false;
  }
  if (target_requires_graph_anchor_identity(spec)) {
    if (map_get(candidate.manifest, "graph_order_fingerprint") !=
        active_identity.graph_order_fingerprint) {
      return false;
    }
    if (map_get(candidate.manifest, "source_cursor_token") !=
        active_identity.source_cursor_token) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::int64_t count_matching_train_attempts(
    const std::vector<job_candidate_t> &candidates,
    const lattice_target_spec_t &spec,
    const lattice_target_active_identity_t &active_identity,
    const std::string &expected_component_fingerprint) {
  std::int64_t count = 0;
  for (const auto &candidate : candidates) {
    if (candidate_matches_active_identity_for_attempt_budget(
            candidate, spec, active_identity, expected_component_fingerprint)) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] inline bool fact_matches_active_identity(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t &fact,
    const lattice_target_active_identity_t &active_identity,
    const lattice_target_spec_t &spec,
    const std::string &expected_component_fingerprint,
    const std::string &expected_split_policy_fingerprint = {}) {
  if (fact.cursor_domain != "ujcamei.graph_anchor") {
    return false;
  }
  if (spec.require_contract_match &&
      fact.contract_fingerprint !=
          active_identity.protocol_contract_fingerprint) {
    return false;
  }
  if (target_requires_graph_anchor_identity(spec)) {
    if (fact.graph_order_fingerprint !=
        active_identity.graph_order_fingerprint) {
      return false;
    }
    if (fact.source_cursor_token != active_identity.source_cursor_token) {
      return false;
    }
  }
  if (spec.require_component_match && fact.target_component == spec.component &&
      fact.component_assembly_fingerprint != expected_component_fingerprint) {
    return false;
  }
  if (!expected_split_policy_fingerprint.empty() &&
      !fact.split_policy_fingerprint.empty() &&
      fact.split_policy_fingerprint != expected_split_policy_fingerprint) {
    return false;
  }
  return true;
}

[[nodiscard]] inline std::vector<
    cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
filter_facts_for_active_identity(
    const std::vector<
        cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t> &facts,
    const lattice_target_active_identity_t &active_identity,
    const lattice_target_spec_t &spec,
    const std::string &expected_component_fingerprint,
    const std::string &expected_split_policy_fingerprint = {}) {
  std::vector<cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
      out;
  for (const auto &fact : facts) {
    if (fact_matches_active_identity(fact, active_identity, spec,
                                     expected_component_fingerprint,
                                     expected_split_policy_fingerprint)) {
      out.push_back(fact);
    }
  }
  return out;
}

} // namespace lattice_target_eval_detail

class lattice_target_evaluator_t {
public:
  lattice_target_evaluator_t(std::vector<lattice_target_spec_t> targets,
                             lattice_target_evaluator_options_t options = {})
      : targets_(std::move(targets)), options_(std::move(options)) {}

  [[nodiscard]] const lattice_target_spec_t *
  find_target(const std::string &target_id) const {
    for (const auto &target : targets_) {
      if (target.target_id == target_id) {
        return &target;
      }
    }
    return nullptr;
  }

  [[nodiscard]] lattice_target_evaluation_t
  evaluate(const std::string &target_id) const {
    const auto *spec = find_target(target_id);
    if (spec == nullptr) {
      throw std::runtime_error("[lattice_target] unknown target id: " +
                               target_id);
    }
    return evaluate_spec(*spec, /*dependency_stack=*/{});
  }

  [[nodiscard]] std::vector<lattice_target_evaluation_t> evaluate_all() const {
    std::vector<lattice_target_evaluation_t> out;
    out.reserve(targets_.size());
    for (const auto &target : targets_) {
      out.push_back(evaluate(target.target_id));
    }
    return out;
  }

private:
  [[nodiscard]] lattice_target_evaluation_t
  evaluate_spec(const lattice_target_spec_t &input_spec,
                std::vector<std::string> dependency_stack) const {
    namespace detail = lattice_target_eval_detail;
    lattice_target_spec_t spec{};
    try {
      spec =
          detail::resolve_split_references(input_spec, options_.split_policy);
    } catch (const std::exception &ex) {
      lattice_target_evaluation_t result{};
      result.target_id = input_spec.target_id;
      result.kind = input_spec.kind;
      result.component = input_spec.component;
      result.split_policy_fingerprint =
          detail::active_split_policy_fingerprint(options_.split_policy);
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(ex.what());
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }
    lattice_target_evaluation_t result{};
    result.target_id = spec.target_id;
    result.kind = spec.kind;
    result.component = spec.component;
    result.split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    result.suggested_wave = detail::make_suggested_wave(spec);

    if (std::find(dependency_stack.begin(), dependency_stack.end(),
                  spec.target_id) != dependency_stack.end()) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back("cyclic lattice target dependency");
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }
    dependency_stack.push_back(spec.target_id);

    if (!spec.upstream_target_id.empty()) {
      const auto *upstream = find_target(spec.upstream_target_id);
      if (upstream == nullptr) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back("missing upstream target: " +
                                 spec.upstream_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      auto upstream_eval = evaluate_spec(*upstream, dependency_stack);
      if (upstream_eval.status != lattice_target_status_t::satisfied) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "upstream target is not satisfied: " + spec.upstream_target_id +
            " status=" + lattice_target_status_name(upstream_eval.status));
        result.plan_ready = upstream_eval.plan_ready;
        result.suggested_wave = upstream_eval.suggested_wave;
        return result;
      }
    }

    if (spec.require_contract_match &&
        options_.active_identity.protocol_contract_fingerprint.empty()) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "active contract fingerprint is required but was not provided");
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }

    const auto expected_component_fingerprint =
        detail::expected_component_fingerprint(options_.active_identity,
                                               spec.kind);
    if (spec.require_component_match &&
        expected_component_fingerprint.empty()) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "active component fingerprint is required but was not provided");
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }

    if (detail::target_requires_graph_anchor_identity(spec)) {
      if (options_.active_identity.graph_order_fingerprint.empty()) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "active graph order fingerprint is required for graph-anchor "
            "lattice targets but was not provided");
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      if (options_.active_identity.source_cursor_token.empty()) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "active source cursor token is required for graph-anchor lattice "
            "targets but was not provided");
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
    }

    const auto checkpoint_source_target_id =
        detail::latest_satisfying_checkpoint_source_target_id(spec);
    if (!checkpoint_source_target_id.empty()) {
      const auto *checkpoint_source_target =
          find_target(checkpoint_source_target_id);
      if (checkpoint_source_target == nullptr) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back("missing CHECKPOINT_SOURCE target: " +
                                 checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      auto source_eval =
          evaluate_spec(*checkpoint_source_target, dependency_stack);
      if (source_eval.status != lattice_target_status_t::satisfied) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "CHECKPOINT_SOURCE target is not satisfied: " +
            checkpoint_source_target_id +
            " status=" + lattice_target_status_name(source_eval.status));
        result.plan_ready = source_eval.plan_ready;
        result.suggested_wave = source_eval.suggested_wave;
        return result;
      }
      if (source_eval.evidence.checkpoint_path.empty() ||
          !std::filesystem::exists(source_eval.evidence.checkpoint_path)) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back(
            "CHECKPOINT_SOURCE latest_satisfying target has no checkpoint "
            "artifact: " +
            checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      result.evidence = source_eval.evidence;
      if (!evaluate_exposure_requirements(spec, result,
                                          /*plan_budget_remaining=*/false,
                                          expected_component_fingerprint)) {
        return result;
      }
      result.status = lattice_target_status_t::satisfied;
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }

    std::optional<detail::evaluated_checkpoint_binding_t>
        evaluated_checkpoint_binding{};
    const auto evaluated_checkpoint_source_target_id =
        detail::latest_satisfying_target_id_from_reference(
            spec.evaluated_checkpoint_source);
    if (!evaluated_checkpoint_source_target_id.empty()) {
      const auto *checkpoint_source_target =
          find_target(evaluated_checkpoint_source_target_id);
      if (checkpoint_source_target == nullptr) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "missing EVALUATED_CHECKPOINT_SOURCE target: " +
            evaluated_checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      auto source_eval =
          evaluate_spec(*checkpoint_source_target, dependency_stack);
      if (source_eval.status != lattice_target_status_t::satisfied) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "EVALUATED_CHECKPOINT_SOURCE target is not satisfied: " +
            evaluated_checkpoint_source_target_id +
            " status=" + lattice_target_status_name(source_eval.status));
        result.plan_ready = source_eval.plan_ready;
        result.suggested_wave = source_eval.suggested_wave;
        return result;
      }
      if (source_eval.evidence.checkpoint_path.empty() ||
          !std::filesystem::exists(source_eval.evidence.checkpoint_path)) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back(
            "EVALUATED_CHECKPOINT_SOURCE latest_satisfying target has no "
            "checkpoint artifact: " +
            evaluated_checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      evaluated_checkpoint_binding = detail::evaluated_checkpoint_binding_t{
          .source_target_id = evaluated_checkpoint_source_target_id,
          .mdn_checkpoint_path = source_eval.evidence.checkpoint_path,
          .representation_checkpoint_path =
              source_eval.evidence.representation_checkpoint_path,
      };
    }

    const auto candidates =
        detail::find_job_candidates(options_.runtime_root, spec);
    if (candidates.empty()) {
      result.status = lattice_target_status_t::missing_report;
      result.reasons.push_back("no matching job manifest/report found under " +
                               options_.runtime_root.string());
      result.plan_ready = spec.max_waves > 0;
      return result;
    }
    std::optional<lattice_target_evaluation_t> newest_failure{};
    const auto matching_job_count =
        static_cast<std::int64_t>(candidates.size());
    const auto matching_train_attempt_count =
        detail::count_matching_train_attempts(candidates, spec,
                                              options_.active_identity,
                                              expected_component_fingerprint);
    for (const auto &candidate : candidates) {
      auto candidate_eval = evaluate_candidate(
          spec, candidate, expected_component_fingerprint, matching_job_count,
          matching_train_attempt_count, evaluated_checkpoint_binding);
      if (candidate_eval.status == lattice_target_status_t::satisfied) {
        return candidate_eval;
      }
      if (!newest_failure.has_value()) {
        newest_failure = std::move(candidate_eval);
      }
    }
    return *newest_failure;
  }

  [[nodiscard]] lattice_target_evaluation_t evaluate_candidate(
      const lattice_target_spec_t &spec,
      const lattice_target_eval_detail::job_candidate_t &candidate,
      const std::string &expected_component_fingerprint,
      std::int64_t matching_job_count,
      std::int64_t matching_train_attempt_count,
      const std::optional<
          lattice_target_eval_detail::evaluated_checkpoint_binding_t>
          &evaluated_checkpoint_binding = std::nullopt) const {
    namespace detail = lattice_target_eval_detail;
    lattice_target_evaluation_t result{};
    result.target_id = spec.target_id;
    result.kind = spec.kind;
    result.component = spec.component;
    result.split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    result.suggested_wave = detail::make_suggested_wave(spec);
    result.evidence = detail::make_evidence_from_candidate(
        candidate, spec, matching_job_count, matching_train_attempt_count);

    const auto plan_budget_remaining =
        matching_train_attempt_count < spec.max_waves;

    if (!result.evidence.report_exists &&
        (spec.require_finite_loss || spec.min_optimizer_steps > 0 ||
         spec.min_valid_target_fraction > 0.0 ||
         spec.require_active_node_head_count > 0 ||
         spec.require_evaluated_node_head_count > 0)) {
      result.status = lattice_target_status_t::missing_report;
      result.reasons.push_back("matching job exists but component report is "
                               "missing: " +
                               result.evidence.report_path.string());
      result.plan_ready = plan_budget_remaining;
      return result;
    }

    if (result.evidence.status != "completed") {
      result.status = lattice_target_status_t::metric_failed;
      result.reasons.push_back(result.evidence.status.empty()
                                   ? "candidate job status is missing"
                                   : "candidate job status is not completed: " +
                                         result.evidence.status);
      result.plan_ready = plan_budget_remaining;
      return result;
    }

    if (spec.require_contract_match &&
        result.evidence.protocol_contract_fingerprint !=
            options_.active_identity.protocol_contract_fingerprint) {
      result.status = lattice_target_status_t::stale_contract;
      result.reasons.push_back("candidate contract fingerprint does not match "
                               "active contract fingerprint");
      result.plan_ready = plan_budget_remaining;
      return result;
    }

    if (spec.require_component_match && result.evidence.component_fingerprint !=
                                            expected_component_fingerprint) {
      result.status = lattice_target_status_t::stale_component;
      result.reasons.push_back("candidate component fingerprint does not match "
                               "active component fingerprint");
      result.plan_ready = plan_budget_remaining;
      return result;
    }

    if (detail::target_requires_graph_anchor_identity(spec)) {
      if (result.evidence.graph_order_fingerprint !=
          options_.active_identity.graph_order_fingerprint) {
        result.status = lattice_target_status_t::stale_contract;
        result.reasons.push_back(
            "candidate graph order fingerprint does not match active graph "
            "order fingerprint");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (result.evidence.source_cursor_token !=
          options_.active_identity.source_cursor_token) {
        result.status = lattice_target_status_t::stale_contract;
        result.reasons.push_back("candidate source cursor token does not match "
                                 "active source cursor token");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (spec.require_checkpoint_exists) {
      if (!result.evidence.checkpoint_written ||
          result.evidence.checkpoint_path.empty() ||
          !std::filesystem::exists(result.evidence.checkpoint_path)) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back("compatible checkpoint artifact is missing");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (spec.kind == lattice_target_kind_t::node_mdn_ready) {
      if (!result.evidence.representation_checkpoint_loaded ||
          result.evidence.allow_untrained_representation ||
          result.evidence.representation_checkpoint_path.empty() ||
          !std::filesystem::exists(
              result.evidence.representation_checkpoint_path)) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back("node MDN readiness requires a loaded "
                                 "compatible representation checkpoint");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (evaluated_checkpoint_binding.has_value()) {
      if (!result.evidence.mdn_checkpoint_loaded ||
          result.evidence.mdn_checkpoint_path.empty()) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back(
            "validation/evaluation evidence must load the MDN checkpoint from "
            "EVALUATED_CHECKPOINT_SOURCE target " +
            evaluated_checkpoint_binding->source_target_id);
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (!detail::same_model_state_path(
              result.evidence.mdn_checkpoint_path,
              evaluated_checkpoint_binding->mdn_checkpoint_path)) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            "validation/evaluation MDN checkpoint does not match "
            "EVALUATED_CHECKPOINT_SOURCE target " +
            evaluated_checkpoint_binding->source_target_id);
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (!evaluated_checkpoint_binding->representation_checkpoint_path
               .empty()) {
        if (!result.evidence.representation_checkpoint_loaded ||
            result.evidence.representation_checkpoint_path.empty()) {
          result.status = lattice_target_status_t::missing_checkpoint;
          result.reasons.push_back(
              "validation/evaluation evidence must load the representation "
              "checkpoint used by the evaluated MDN checkpoint");
          result.plan_ready = plan_budget_remaining;
          return result;
        }
        if (!detail::same_model_state_path(
                result.evidence.representation_checkpoint_path,
                evaluated_checkpoint_binding->representation_checkpoint_path)) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back(
              "validation/evaluation representation checkpoint does not match "
              "the evaluated MDN checkpoint lineage");
          result.plan_ready = plan_budget_remaining;
          return result;
        }
      }
    }

    if (spec.require_finite_loss && !std::isfinite(result.evidence.loss)) {
      result.status = lattice_target_status_t::metric_failed;
      result.reasons.push_back("loss is not finite");
      result.plan_ready = plan_budget_remaining;
      return result;
    }
    if (result.evidence.optimizer_steps < spec.min_optimizer_steps) {
      result.status = lattice_target_status_t::metric_failed;
      result.reasons.push_back("optimizer_steps below MIN_OPTIMIZER_STEPS");
      result.plan_ready = plan_budget_remaining;
      return result;
    }
    if (spec.kind == lattice_target_kind_t::node_mdn_ready) {
      if (std::isfinite(spec.min_valid_target_fraction) &&
          result.evidence.valid_target_fraction <
              spec.min_valid_target_fraction) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back(
            "valid target fraction below MIN_VALID_TARGET_FRACTION");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (result.evidence.active_node_head_count <
          spec.require_active_node_head_count) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back(
            "active node head count below REQUIRE_ACTIVE_NODE_HEAD_COUNT");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (result.evidence.trained_node_head_count <
          spec.require_trained_node_head_count) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back(
            "trained node head count below REQUIRE_TRAINED_NODE_HEAD_COUNT");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
      if (result.evidence.evaluated_node_head_count <
          spec.require_evaluated_node_head_count) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back("evaluated node head count below "
                                 "REQUIRE_EVALUATED_NODE_HEAD_COUNT");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (!evaluate_exposure_requirements(spec, result, plan_budget_remaining,
                                        expected_component_fingerprint)) {
      return result;
    }

    result.status = lattice_target_status_t::satisfied;
    result.plan_ready = false;
    result.suggested_wave = {};
    return result;
  }

  void evaluate_warning_specs(
      const lattice_target_spec_t &spec, lattice_target_evaluation_t &result,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
          &exposure_facts,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
          &target_component_facts) const {
    namespace detail = lattice_target_eval_detail;
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;

    for (const auto &warning : spec.warning_specs) {
      lattice_target_evaluation_t::warning_result_t out{};
      out.warning_id = warning.warning_id;
      out.kind = warning.kind;
      out.split = warning.split;
      out.use = exposure::exposure_use_name(warning.use);
      out.scope = warning.scope;
      out.effect =
          warning.require_mutated_component ? "mutated_component" : "any";
      out.metric = warning.metric;

      const auto &facts = warning.scope == "all_components"
                              ? exposure_facts
                              : target_component_facts;
      const auto target_component_filter =
          warning.scope == "target_component" ? spec.component : std::string{};

      if (warning.kind == "exposure_load" || warning.kind == "effort_density") {
        const auto range = detail::warning_anchor_interval(spec, warning);
        out.exposure_summary = exposure::exposure_load_for_use(
            facts, range, warning.use, warning.require_mutated_component,
            target_component_filter);
        if (warning.kind == "exposure_load") {
          out.measured_value = out.exposure_summary.cursor_exposure_load;
          out.threshold = warning.cursor_epochs_above;
          out.unit = "cursor_epochs";
          const bool triggered = std::isfinite(out.threshold) &&
                                 out.measured_value > out.threshold;
          std::ostringstream message;
          message << out.use << " exposure load";
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " is " << out.measured_value << " cursor_epochs";
          if (std::isfinite(out.threshold)) {
            message << (triggered ? " above threshold "
                                  : ", not above threshold ")
                    << out.threshold;
          }
          out.message = message.str();
        } else {
          out.metric = warning.metric;
          out.measured_value =
              warning.metric == "optimizer_steps_per_unique_anchor"
                  ? out.exposure_summary.optimizer_steps_per_unique_anchor
                  : out.exposure_summary.optimizer_steps_per_cursor_epoch;
          out.threshold = warning.above;
          out.unit = warning.metric;
          const bool triggered = std::isfinite(out.threshold) &&
                                 out.measured_value > out.threshold;
          std::ostringstream message;
          message << warning.metric;
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << (triggered ? " above threshold "
                                  : ", not above threshold ")
                    << out.threshold;
          }
          out.message = message.str();
        }
        if (std::isfinite(out.threshold) &&
            out.measured_value > out.threshold) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "anchor_domain_health") {
        double min_accepted_fraction = std::numeric_limits<double>::quiet_NaN();
        std::int64_t skipped_failed_fetch_probe_total = 0;
        std::int64_t fact_count = 0;
        for (const auto &fact : facts) {
          ++fact_count;
          if (std::isfinite(fact.accepted_anchor_fraction)) {
            if (!std::isfinite(min_accepted_fraction)) {
              min_accepted_fraction = fact.accepted_anchor_fraction;
            } else {
              min_accepted_fraction = std::min(min_accepted_fraction,
                                               fact.accepted_anchor_fraction);
            }
          }
          skipped_failed_fetch_probe_total += fact.skipped_failed_fetch_probe;
        }
        out.exposure_summary.fact_count = fact_count;
        if (std::isfinite(warning.accepted_fraction_below)) {
          out.measured_value = min_accepted_fraction;
          out.threshold = warning.accepted_fraction_below;
          out.unit = "accepted_fraction";
          const bool triggered = std::isfinite(out.measured_value) &&
                                 out.measured_value < out.threshold;
          std::ostringstream message;
          message << "anchor-domain accepted fraction is " << out.measured_value
                  << (triggered ? " below threshold "
                                : ", not below threshold ")
                  << out.threshold;
          out.message = message.str();
          if (triggered) {
            out.status = "warning";
            out.severity = "warn";
            result.warnings.push_back(out.message);
          }
        }
        if (warning.skipped_failed_fetch_probe_above.has_value()) {
          out.metric = "skipped_failed_fetch_probe";
          out.measured_value =
              static_cast<double>(skipped_failed_fetch_probe_total);
          out.threshold =
              static_cast<double>(*warning.skipped_failed_fetch_probe_above);
          out.unit = "count";
          const bool triggered = out.measured_value > out.threshold;
          std::ostringstream message;
          message << "anchor-domain failed fetch probes total "
                  << skipped_failed_fetch_probe_total
                  << (triggered ? " above threshold "
                                : ", not above threshold ")
                  << *warning.skipped_failed_fetch_probe_above;
          out.message = message.str();
          if (triggered) {
            out.status = "warning";
            out.severity = "warn";
            result.warnings.push_back(out.message);
          }
        }
        result.warning_results.push_back(std::move(out));
      }
    }
  }

  [[nodiscard]] bool evaluate_exposure_requirements(
      const lattice_target_spec_t &spec, lattice_target_evaluation_t &result,
      bool plan_budget_remaining,
      const std::string &expected_component_fingerprint) const {
    namespace detail = lattice_target_eval_detail;
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;

    if (!detail::target_has_exposure_requirements(spec)) {
      return true;
    }
    const auto expected_split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    std::optional<exposure::exposure_ledger_scan_result_t> scanned_ledger{};
    const exposure::lattice_exposure_ledger_t *ledger =
        options_.exposure_ledger;
    if (ledger == nullptr && options_.auto_build_exposure_ledger) {
      exposure::exposure_build_context_t context{};
      context.split_policy_fingerprint = expected_split_policy_fingerprint;
      scanned_ledger = exposure::scan_exposure_ledger_from_runtime_root(
          options_.runtime_root, context);
      ledger = &scanned_ledger->ledger;
    }
    if (ledger == nullptr) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "target has exposure requirements but no exposure ledger was "
          "provided");
      result.plan_ready = false;
      result.suggested_wave = {};
      return false;
    }

    std::vector<exposure::lattice_exposure_fact_t> exposure_facts;
    if (!result.evidence.checkpoint_path.empty()) {
      const auto closure =
          ledger->checkpoint_closure_result(result.evidence.checkpoint_path);
      if (!closure.complete()) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "checkpoint closure has unresolved input checkpoint lineage";
        for (const auto &path : closure.unresolved_input_checkpoints) {
          reason << " " << path.string();
        }
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      exposure_facts = closure.facts;
      if (exposure_facts.empty()) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            "exposure ledger has no checkpoint closure facts for " +
            result.evidence.checkpoint_path.string());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    } else {
      for (const auto &fact : ledger->facts()) {
        if (fact.target_component != spec.component) {
          continue;
        }
        if (spec.require_contract_match &&
            fact.contract_fingerprint !=
                options_.active_identity.protocol_contract_fingerprint) {
          continue;
        }
        if (spec.require_component_match &&
            fact.component_assembly_fingerprint !=
                result.evidence.component_fingerprint) {
          continue;
        }
        exposure_facts.push_back(fact);
      }
    }
    exposure_facts = detail::filter_facts_for_active_identity(
        exposure_facts, options_.active_identity, spec,
        expected_component_fingerprint, expected_split_policy_fingerprint);
    if (exposure_facts.empty()) {
      result.status = lattice_target_status_t::exposure_failed;
      result.reasons.push_back(
          "exposure ledger has no active graph/source-compatible facts for "
          "this target/checkpoint");
      result.plan_ready = plan_budget_remaining;
      return false;
    }

    if (spec.kind == lattice_target_kind_t::node_mdn_ready &&
        !result.evidence.representation_checkpoint_path.empty()) {
      const auto rep_closure = ledger->checkpoint_closure_result(
          result.evidence.representation_checkpoint_path);
      if (!rep_closure.complete()) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "loaded representation checkpoint closure has unresolved "
                  "input checkpoint lineage";
        for (const auto &path : rep_closure.unresolved_input_checkpoints) {
          reason << " " << path.string();
        }
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      auto rep_facts = detail::filter_facts_for_active_identity(
          rep_closure.facts, options_.active_identity, spec,
          expected_component_fingerprint, expected_split_policy_fingerprint);
      const auto rep_ready_fact = std::find_if(
          rep_facts.begin(), rep_facts.end(),
          [&](const exposure::lattice_exposure_fact_t &fact) {
            return fact.target_component ==
                       "wikimyei.representation.encoding.vicreg" &&
                   fact.component_assembly_fingerprint ==
                       options_.active_identity.vicreg_assembly_fingerprint;
          });
      if (rep_ready_fact == rep_facts.end()) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            "loaded representation checkpoint has no active compatible VICReg "
            "producer exposure fact");
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }

    std::vector<exposure::lattice_exposure_fact_t> target_component_facts;
    for (const auto &fact : exposure_facts) {
      if (fact.target_component == spec.component) {
        target_component_facts.push_back(fact);
      }
    }
    if (target_component_facts.empty()) {
      result.status = lattice_target_status_t::exposure_failed;
      result.reasons.push_back(
          "checkpoint closure has no target-component-local exposure facts");
      result.plan_ready = plan_budget_remaining;
      return false;
    }

    const auto target_range = detail::target_anchor_interval(spec);
    evaluate_warning_specs(spec, result, exposure_facts,
                           target_component_facts);
    if (spec.min_observed_input_coverage > 0.0) {
      result.exposure_summaries.push_back(exposure::exposure_load_for_use(
          target_component_facts, target_range,
          exposure::exposure_use_t::observed_input,
          /*require_mutated_component=*/true, spec.component));
      const auto coverage =
          exposure::coverage_for_use(target_component_facts, target_range,
                                     exposure::exposure_use_t::observed_input,
                                     /*require_mutated_component=*/true);
      if (coverage.coverage_fraction + 1e-12 <
          spec.min_observed_input_coverage) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "observed_input exposure coverage "
               << coverage.coverage_fraction
               << " below MIN_OBSERVED_INPUT_COVERAGE="
               << spec.min_observed_input_coverage;
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }
    if (spec.min_target_supervision_coverage > 0.0) {
      result.exposure_summaries.push_back(exposure::exposure_load_for_use(
          target_component_facts, target_range,
          exposure::exposure_use_t::target_supervision,
          /*require_mutated_component=*/true, spec.component));
      const auto coverage = exposure::coverage_for_use(
          target_component_facts, target_range,
          exposure::exposure_use_t::target_supervision,
          /*require_mutated_component=*/true);
      if (coverage.coverage_fraction + 1e-12 <
          spec.min_target_supervision_coverage) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "target_supervision exposure coverage "
               << coverage.coverage_fraction
               << " below MIN_TARGET_SUPERVISION_COVERAGE="
               << spec.min_target_supervision_coverage;
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }
    if (spec.min_evaluation_metric_coverage > 0.0) {
      result.exposure_summaries.push_back(exposure::exposure_load_for_use(
          target_component_facts, target_range,
          exposure::exposure_use_t::evaluation_metric,
          /*require_mutated_component=*/false, spec.component));
      const auto coverage = exposure::coverage_for_use(
          target_component_facts, target_range,
          exposure::exposure_use_t::evaluation_metric,
          /*require_mutated_component=*/false);
      if (coverage.coverage_fraction + 1e-12 <
          spec.min_evaluation_metric_coverage) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "evaluation_metric exposure coverage "
               << coverage.coverage_fraction
               << " below MIN_EVALUATION_METRIC_COVERAGE="
               << spec.min_evaluation_metric_coverage;
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }
    const auto forbidden_range =
        options_.split_policy.has_value()
            ? detail::expanded_forbidden_exposure_interval(
                  spec, *options_.split_policy, exposure_facts)
            : detail::forbidden_exposure_interval(spec);
    if (!forbidden_range.empty()) {
      const exposure::forbidden_exposure_query_t query{
          .forbidden_range = forbidden_range,
          .forbidden_uses = spec.forbid_exposure_uses,
          .require_mutated_component =
              spec.forbid_exposure_requires_mutated_component,
      };
      if (exposure::has_forbidden_exposure_overlap(exposure_facts, query)) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << "checkpoint exposure overlaps forbidden anchor range ["
               << forbidden_range.begin << ", " << forbidden_range.end << ")";
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }
    return true;
  }

  std::vector<lattice_target_spec_t> targets_{};
  lattice_target_evaluator_options_t options_{};
};

[[nodiscard]] inline std::vector<lattice_target_spec_t>
load_lattice_targets_from_file(const std::filesystem::path &path) {
  return decode_lattice_targets_from_dsl(
      lattice_target_eval_detail::read_text_file_or_throw(path));
}

[[nodiscard]] inline std::vector<lattice_target_compiled_t>
load_lattice_compiled_targets_from_file(const std::filesystem::path &path) {
  return decode_lattice_compiled_targets_from_dsl(
      lattice_target_eval_detail::read_text_file_or_throw(path));
}

[[nodiscard]] inline lattice_target_config_paths_t
resolve_lattice_target_config_paths(
    const std::filesystem::path &config_path = "/cuwacunu/src/config/.config") {
  const auto cfg = lattice_target_eval_detail::parse_assignment_text(
      lattice_target_eval_detail::read_text_file_or_throw(config_path));
  const auto splits_bnf = lattice_target_eval_detail::map_get(
      cfg, "kikijyeba_lattice_splits_dsl_bnf_path");
  const auto splits_dsl = lattice_target_eval_detail::map_get(
      cfg, "kikijyeba_lattice_splits_dsl_path");
  const auto bnf = lattice_target_eval_detail::map_get(
      cfg, "kikijyeba_lattice_targets_dsl_bnf_path");
  const auto dsl = lattice_target_eval_detail::map_get(
      cfg, "kikijyeba_lattice_targets_dsl_path");
  if (bnf.empty() || dsl.empty()) {
    throw std::runtime_error(
        "[lattice_target] missing kikijyeba_lattice_targets_dsl_* path in " +
        config_path.string());
  }
  return lattice_target_config_paths_t{
      .splits_dsl_bnf_path = splits_bnf.empty()
                                 ? std::filesystem::path{}
                                 : lattice_target_eval_detail::resolve_against(
                                       config_path, splits_bnf),
      .splits_dsl_path = splits_dsl.empty()
                             ? std::filesystem::path{}
                             : lattice_target_eval_detail::resolve_against(
                                   config_path, splits_dsl),
      .targets_dsl_bnf_path =
          lattice_target_eval_detail::resolve_against(config_path, bnf),
      .targets_dsl_path =
          lattice_target_eval_detail::resolve_against(config_path, dsl),
  };
}

[[nodiscard]] inline std::optional<
    cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
load_lattice_split_policy_from_config_if_available(
    const std::filesystem::path &config_path = "/cuwacunu/src/config/.config") {
  const auto paths = resolve_lattice_target_config_paths(config_path);
  if (paths.splits_dsl_path.empty()) {
    return std::nullopt;
  }
  if (!paths.splits_dsl_bnf_path.empty()) {
    (void)lattice_target_eval_detail::read_text_file_or_throw(
        paths.splits_dsl_bnf_path);
  }
  return cuwacunu::kikijyeba::lattice::split::
      load_lattice_split_policy_from_file(paths.splits_dsl_path);
}

[[nodiscard]] inline std::vector<lattice_target_spec_t>
load_lattice_targets_from_config(
    const std::filesystem::path &config_path = "/cuwacunu/src/config/.config") {
  const auto paths = resolve_lattice_target_config_paths(config_path);
  (void)lattice_target_eval_detail::read_text_file_or_throw(
      paths.targets_dsl_bnf_path);
  return load_lattice_targets_from_file(paths.targets_dsl_path);
}

[[nodiscard]] inline std::vector<lattice_target_compiled_t>
load_lattice_compiled_targets_from_config(
    const std::filesystem::path &config_path = "/cuwacunu/src/config/.config") {
  const auto paths = resolve_lattice_target_config_paths(config_path);
  (void)lattice_target_eval_detail::read_text_file_or_throw(
      paths.targets_dsl_bnf_path);
  return load_lattice_compiled_targets_from_file(paths.targets_dsl_path);
}

} // namespace cuwacunu::kikijyeba::lattice::target
