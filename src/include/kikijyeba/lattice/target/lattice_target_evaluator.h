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
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "kikijyeba/lattice/split/split_policy.h"
#include "kikijyeba/lattice/target/lattice_target.h"
#include "kikijyeba/protocol/config_provenance.h"
#include "kikijyeba/runtime/job_layout.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::lattice::target {

inline constexpr const char *k_lattice_read_only_validation_model_state_reason =
    "validation/evaluation evidence must not write a new checkpoint or model "
    "state";
inline constexpr const char
    *k_lattice_exact_validation_model_state_input_reason =
        "validation/evaluation evidence must load exactly the evaluated "
        "checkpoint input edges";
inline constexpr const char
    *k_lattice_non_mutating_validation_model_state_reason =
        "validation/evaluation evidence must not mutate model state or run "
        "optimizer steps";

struct lattice_target_active_identity_t {
  std::string protocol_id{};
  std::string protocol_contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string vicreg_assembly_fingerprint{};
  std::string mtf_jepa_mae_vicreg_assembly_fingerprint{};
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
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
    return identity.vicreg_assembly_fingerprint;
  case lattice_target_kind_t::mtf_representation_ready:
    return identity.mtf_jepa_mae_vicreg_assembly_fingerprint;
  case lattice_target_kind_t::channel_mdn_ready:
    return identity.mdn_assembly_fingerprint;
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
report_leaf_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
  case lattice_target_kind_t::mtf_representation_ready:
    return "channel_representation.report";
  case lattice_target_kind_t::channel_mdn_ready:
    return "channel_inference.report";
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

[[nodiscard]] inline std::vector<std::string>
model_state_path_keys(const std::vector<fs::path> &paths, bool &has_empty,
                      bool &has_duplicate) {
  std::vector<std::string> out;
  has_empty = false;
  has_duplicate = false;
  out.reserve(paths.size());
  for (const auto &path : paths) {
    if (path.empty()) {
      has_empty = true;
      continue;
    }
    out.push_back(path.lexically_normal().string());
  }
  std::sort(out.begin(), out.end());
  has_duplicate = std::adjacent_find(out.begin(), out.end()) != out.end();
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

[[nodiscard]] inline std::vector<std::string>
expected_model_state_input_keys(const evaluated_checkpoint_binding_t &binding) {
  bool has_empty = false;
  bool has_duplicate = false;
  std::vector<fs::path> expected;
  if (!binding.representation_checkpoint_path.empty()) {
    expected.push_back(binding.representation_checkpoint_path);
  }
  if (!binding.mdn_checkpoint_path.empty()) {
    expected.push_back(binding.mdn_checkpoint_path);
  }
  return model_state_path_keys(expected, has_empty, has_duplicate);
}

[[nodiscard]] inline bool evaluation_fact_has_exact_model_state_inputs(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t &fact,
    const evaluated_checkpoint_binding_t &binding) {
  bool has_empty = false;
  bool has_duplicate = false;
  const auto actual =
      model_state_path_keys(fact.input_checkpoints, has_empty, has_duplicate);
  if (has_empty || has_duplicate) {
    return false;
  }
  return actual == expected_model_state_input_keys(binding);
}

[[nodiscard]] inline std::vector<job_candidate_t>
find_job_candidates(const fs::path &runtime_root,
                    const lattice_target_spec_t &spec) {
  std::vector<job_candidate_t> out;
  std::error_code ec;
  if (!fs::is_directory(runtime_root, ec)) {
    return out;
  }
  for (const auto &job_dir :
       cuwacunu::kikijyeba::runtime::job_layout::discover_runtime_job_dirs(
           runtime_root)) {
    const fs::path manifest_path = job_dir.manifest_path;
    const auto manifest = parse_assignment_file_if_exists(manifest_path);
    if (manifest.empty()) {
      continue;
    }
    if (map_get(manifest, "job_kind") != job_kind_for_target_kind(spec.kind)) {
      continue;
    }
    if (!spec.protocol_id.empty() &&
        map_get(manifest, "protocol_id") != spec.protocol_id) {
      continue;
    }
    if (map_get(manifest, "target_component_family_id") != spec.component) {
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
        fs::last_write_time(newest_marker_path(job_dir.dir), time_ec);
    if (time_ec) {
      marker_time = fs::file_time_type::min();
    }
    out.push_back(job_candidate_t{
        .dir = job_dir.dir,
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
  out.report_path = candidate.dir / report_leaf_for_target_kind(spec.kind);
  out.report_exists = fs::exists(out.report_path);
  const auto state = parse_assignment_file_if_exists(out.state_path);
  const auto report = parse_assignment_file_if_exists(out.report_path);
  const auto checkpoint_io = parse_assignment_file_if_exists(
      candidate.dir / "runtime.checkpoint_io.fact");
  const auto first_non_empty =
      [](std::initializer_list<std::string> values) -> std::string {
    for (const auto &value : values) {
      if (!value.empty()) {
        return value;
      }
    }
    return {};
  };
  const auto resolve_job_path = [&](const std::string &value) -> fs::path {
    if (value.empty()) {
      return {};
    }
    const fs::path raw(value);
    return raw.is_absolute() ? raw : candidate.dir / raw;
  };

  out.report_schema_id = map_get(report, "report_schema_id");
  out.report_schema_version =
      parse_i64_fallback(map_get(report, "report_schema_version"), 0);
  out.report_writer_id = map_get(report, "report_writer_id");
  out.report_writer_version = map_get(report, "report_writer_version");
  out.protocol_id = map_get(candidate.manifest, "protocol_id");
  out.protocol_contract_fingerprint =
      map_get(candidate.manifest, "protocol_contract_fingerprint");
  out.component_fingerprint = map_get(
      candidate.manifest, component_fingerprint_key_for_target_kind(spec.kind));
  out.graph_order_fingerprint =
      map_get(candidate.manifest, "graph_order_fingerprint");
  out.source_cursor_token = map_get(candidate.manifest, "source_cursor_token");
  out.config_bundle_id = map_get(candidate.manifest, "config_bundle_id");
  out.config_receipt_id = map_get(candidate.manifest, "config_receipt_id");
  out.component_spawn_registry_id =
      map_get(candidate.manifest, "component_spawn_registry_id");
  out.component_family_id = map_get(candidate.manifest, "component_family_id");
  out.component_spawn_fingerprint =
      map_get(candidate.manifest, "component_spawn_fingerprint");
  out.component_spawn_id = map_get(candidate.manifest, "component_spawn_id");
  out.component_spawn_label =
      map_get(candidate.manifest, "component_spawn_label");
  out.status = map_get(state, "status");
  out.matching_job_count = matching_job_count;
  out.matching_train_attempt_count = matching_train_attempt_count;
  out.optimizer_steps =
      first_i64(state, report, {"optimizer_steps"}, /*fallback=*/0);
  out.loss = first_finite_double(state, report, {"last_loss", "mean_loss"});
  out.valid_target_fraction = first_finite_double(
      report, state,
      {"mean_valid_node_target_fraction", "last_valid_node_target_fraction",
       "mean_valid_target_fraction", "last_valid_target_fraction"});
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
  const auto checkpoint_path = first_non_empty(
      {map_get(state, "checkpoint_path"), map_get(report, "checkpoint_path"),
       map_get(checkpoint_io, "checkpoint_path")});
  out.checkpoint_path = resolve_job_path(checkpoint_path);
  out.checkpoint_path_reported = resolve_job_path(first_non_empty(
      {map_get(checkpoint_io, "checkpoint_path_reported"),
       map_get(report, "checkpoint_path_reported"), checkpoint_path}));
  out.checkpoint_digest_reported =
      first_non_empty({map_get(checkpoint_io, "checkpoint_digest_reported"),
                       map_get(report, "checkpoint_digest_reported"),
                       map_get(checkpoint_io, "checkpoint_artifact_hash")});
  out.checkpoint_file_exists =
      !out.checkpoint_path.empty() && fs::exists(out.checkpoint_path);
  out.checkpoint_bytes =
      first_i64(checkpoint_io, report, {"checkpoint_bytes"}, 0);
  if (out.checkpoint_file_exists) {
    const auto digest =
        cuwacunu::kikijyeba::lattice::exposure::file_content_digest_checked(
            out.checkpoint_path);
    if (digest.ok) {
      out.checkpoint_digest_actual = digest.digest;
    }
  }
  const bool reported_verified = parse_bool_fallback(
      first_non_empty({map_get(checkpoint_io, "checkpoint_digest_verified"),
                       map_get(report, "checkpoint_digest_verified")}),
      false);
  out.checkpoint_digest_verified =
      reported_verified && out.checkpoint_file_exists &&
      !out.checkpoint_digest_reported.empty() &&
      out.checkpoint_digest_reported == out.checkpoint_digest_actual;
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
  if (!active_identity.protocol_id.empty() &&
      map_get(candidate.manifest, "protocol_id") !=
          active_identity.protocol_id) {
    return false;
  }
  if (!spec.protocol_id.empty() &&
      map_get(candidate.manifest, "protocol_id") != spec.protocol_id) {
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
  if (!active_identity.protocol_id.empty() &&
      fact.protocol_id != active_identity.protocol_id) {
    return false;
  }
  if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
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
  if (spec.require_component_match &&
      fact.target_component_family_id == spec.component &&
      fact.component_assembly_fingerprint != expected_component_fingerprint) {
    return false;
  }
  if (!expected_split_policy_fingerprint.empty()) {
    if (fact.split_policy_fingerprint.empty()) {
      if (certificate_names_concrete_split(fact.split_name)) {
        return false;
      }
    } else if (fact.split_policy_fingerprint !=
               expected_split_policy_fingerprint) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::optional<std::string>
strict_channel_semantic_contract_mismatch(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t &fact,
    lattice_target_kind_t kind) {
  const auto check_field =
      [](const char *field_name, const std::string &actual,
         const char *expected) -> std::optional<std::string> {
    if (actual == expected) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected " << expected << " but got "
        << (actual.empty() ? "<empty>" : actual);
    return out.str();
  };

  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
    if (auto mismatch = check_field(
            "representation_architecture", fact.representation_architecture,
            "channel_preserving_local_node_encoder.v1")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("representation_contract", fact.representation_contract,
                        "graph_order.channel_node_representation.v1")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("representation_value_shape",
                        fact.representation_value_shape, "[B,N,C,De]")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("channel_axis_policy", fact.channel_axis_policy,
                        "preserved_primary_output")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("temporal_reduction", fact.temporal_reduction,
                        "mask_aware_anchor_weighted")) {
      return mismatch;
    }
    return std::nullopt;

  case lattice_target_kind_t::mtf_representation_ready:
    if (auto mismatch = check_field("representation_architecture",
                                    fact.representation_architecture,
                                    "mtf_jepa_mae_vicreg.v1")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("representation_contract", fact.representation_contract,
                        "standalone.mtf_jepa_mae_vicreg.v1")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("representation_value_shape",
                        fact.representation_value_shape, "[B_flat,De]")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("channel_axis_policy", fact.channel_axis_policy,
                        "optional_channel_output")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("temporal_reduction", fact.temporal_reduction,
                        "mask_aware_token_pool")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("input_nodelift_shape", fact.input_nodelift_shape,
                        "[B,C,Hx,N,F]")) {
      return mismatch;
    }
    if (auto mismatch = check_field(
            "mtf_training_shape", fact.mtf_training_shape, "[B_flat,C,Hx,F]")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("flattening_contract", fact.flattening_contract,
                        "anchor_node_flatten.v1")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("recommended_graph_restore_shape",
                        fact.recommended_graph_restore_shape, "[B,N,C,De]")) {
      return mismatch;
    }
    if (!fact.reshape_lossless) {
      return "reshape_lossless expected true but got false";
    }
    return std::nullopt;

  case lattice_target_kind_t::channel_mdn_ready:
    if (fact.input_representation_assembly_id != "vicreg_v1" &&
        fact.input_representation_assembly_id != "mtf_jepa_mae_vicreg_v1") {
      std::ostringstream out;
      out << "input_representation_assembly_id expected vicreg_v1 or "
             "mtf_jepa_mae_vicreg_v1 but got "
          << (fact.input_representation_assembly_id.empty()
                  ? "<empty>"
                  : fact.input_representation_assembly_id);
      return out.str();
    }
    if (auto mismatch = check_field("context_mode", fact.context_mode,
                                    "channel_context_strict")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("context_contract", fact.context_contract,
                        "graph_order.channel_node_representation.v1")) {
      return mismatch;
    }
    if (auto mismatch = check_field("context_value_shape",
                                    fact.context_value_shape, "[B,N,C,De]")) {
      return mismatch;
    }
    if (auto mismatch =
            check_field("output_contract", fact.output_contract,
                        "graph_order.channel_node_future_distribution.v1")) {
      return mismatch;
    }
    return std::nullopt;
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::optional<std::string>
strict_channel_representation_health_mismatch(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
        &fact) {
  const auto check_finite = [](const char *field_name,
                               double value) -> std::optional<std::string> {
    if (std::isfinite(value)) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " must be finite";
    return out.str();
  };
  const auto check_fraction = [&](const char *field_name,
                                  double value) -> std::optional<std::string> {
    if (const auto mismatch = check_finite(field_name, value)) {
      return mismatch;
    }
    if (value > 0.0 && value <= 1.0) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected in (0,1] but got " << value;
    return out.str();
  };
  const auto check_nonnegative_finite =
      [&](const char *field_name, double value) -> std::optional<std::string> {
    if (const auto mismatch = check_finite(field_name, value)) {
      return mismatch;
    }
    if (value >= 0.0) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected nonnegative but got " << value;
    return out.str();
  };

  if (!fact.representation_health_available) {
    return "representation_health_available expected true but got false";
  }
  if (!fact.finite_parameter_check) {
    return "finite_parameter_check expected true but got false";
  }
  if (fact.total_valid_projection_rows <= 0) {
    std::ostringstream out;
    out << "total_valid_projection_rows expected positive but got "
        << fact.total_valid_projection_rows;
    return out.str();
  }
  if (const auto mismatch =
          check_fraction("mean_adapter_valid_channel_time_fraction",
                         fact.mean_adapter_valid_channel_time_fraction)) {
    return mismatch;
  }
  if (fact.augmented_valid_feature_count <= 0) {
    std::ostringstream out;
    out << "augmented_valid_feature_count expected positive but got "
        << fact.augmented_valid_feature_count;
    return out.str();
  }
  if (const auto mismatch =
          check_fraction("mean_augmented_valid_feature_fraction",
                         fact.mean_augmented_valid_feature_fraction)) {
    return mismatch;
  }
  if (const auto mismatch =
          check_fraction("mean_augmented_feature_retention_fraction",
                         fact.mean_augmented_feature_retention_fraction)) {
    return mismatch;
  }
  if (fact.representation_embedding_dim <= 0) {
    std::ostringstream out;
    out << "representation_embedding_dim expected positive but got "
        << fact.representation_embedding_dim;
    return out.str();
  }
  if (const auto mismatch = check_finite("representation_effective_rank",
                                         fact.representation_effective_rank)) {
    return mismatch;
  }
  if (fact.representation_effective_rank <= 0.0) {
    std::ostringstream out;
    out << "representation_effective_rank expected positive but got "
        << fact.representation_effective_rank;
    return out.str();
  }
  if (const auto mismatch =
          check_fraction("representation_effective_rank_fraction",
                         fact.representation_effective_rank_fraction)) {
    return mismatch;
  }
  if (const auto mismatch = check_nonnegative_finite(
          "representation_min_dimension_variance",
          fact.representation_min_dimension_variance)) {
    return mismatch;
  }
  if (const auto mismatch = check_nonnegative_finite(
          "representation_max_dimension_variance",
          fact.representation_max_dimension_variance)) {
    return mismatch;
  }
  if (fact.representation_max_dimension_variance <
      fact.representation_min_dimension_variance) {
    std::ostringstream out;
    out << "representation_max_dimension_variance expected >= "
           "representation_min_dimension_variance but got "
        << fact.representation_max_dimension_variance << " < "
        << fact.representation_min_dimension_variance;
    return out.str();
  }
  if (const auto mismatch =
          check_finite("representation_condition_number",
                       fact.representation_condition_number)) {
    return mismatch;
  }
  if (fact.representation_condition_number < 1.0) {
    std::ostringstream out;
    out << "representation_condition_number expected >= 1 but got "
        << fact.representation_condition_number;
    return out.str();
  }
  if (const auto mismatch =
          check_fraction("representation_isotropy_score",
                         fact.representation_isotropy_score)) {
    return mismatch;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string>
strict_mtf_representation_health_mismatch(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
        &fact) {
  const auto check_finite = [](const char *field_name,
                               double value) -> std::optional<std::string> {
    if (std::isfinite(value)) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " must be finite";
    return out.str();
  };
  const auto check_fraction = [&](const char *field_name,
                                  double value) -> std::optional<std::string> {
    if (const auto mismatch = check_finite(field_name, value)) {
      return mismatch;
    }
    if (value > 0.0 && value <= 1.0) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected in (0,1] but got " << value;
    return out.str();
  };

  if (!fact.representation_health_available) {
    return "representation_health_available expected true but got false";
  }
  if (!fact.finite_parameter_check) {
    return "finite_parameter_check expected true but got false";
  }
  if (!fact.gradients_finite) {
    return "gradients_finite expected true but got false";
  }
  if (fact.sample_valid_count <= 0) {
    std::ostringstream out;
    out << "sample_valid_count expected positive but got "
        << fact.sample_valid_count;
    return out.str();
  }
  if (const auto mismatch =
          check_fraction("sample_valid_fraction", fact.sample_valid_fraction)) {
    return mismatch;
  }
  if (fact.channel_valid_count <= 0) {
    std::ostringstream out;
    out << "channel_valid_count expected positive but got "
        << fact.channel_valid_count;
    return out.str();
  }
  if (const auto mismatch = check_fraction("channel_valid_fraction",
                                           fact.channel_valid_fraction)) {
    return mismatch;
  }
  if (fact.valid_latent_rows <= 0) {
    std::ostringstream out;
    out << "valid_latent_rows expected positive but got "
        << fact.valid_latent_rows;
    return out.str();
  }
  if (fact.total_valid_projection_rows <= 0) {
    std::ostringstream out;
    out << "total_valid_projection_rows expected positive but got "
        << fact.total_valid_projection_rows;
    return out.str();
  }
  if (fact.representation_embedding_dim <= 0) {
    std::ostringstream out;
    out << "representation_embedding_dim expected positive but got "
        << fact.representation_embedding_dim;
    return out.str();
  }
  if (const auto mismatch = check_finite("latent_std", fact.latent_std)) {
    return mismatch;
  }
  if (fact.latent_std <= 0.0) {
    std::ostringstream out;
    out << "latent_std expected positive but got " << fact.latent_std;
    return out.str();
  }
  if (const auto mismatch = check_finite("latent_norm", fact.latent_norm)) {
    return mismatch;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string>
strict_mtf_report_schema_mismatch(const lattice_target_evidence_t &evidence) {
  const auto check =
      [](const char *field_name, const std::string &actual,
         const std::string &expected) -> std::optional<std::string> {
    if (actual == expected) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected " << expected << " but got "
        << (actual.empty() ? "<empty>" : actual);
    return out.str();
  };
  if (const auto mismatch =
          check("report_schema_id", evidence.report_schema_id,
                "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1")) {
    return mismatch;
  }
  if (evidence.report_schema_version != 1) {
    std::ostringstream out;
    out << "report_schema_version expected 1 but got "
        << evidence.report_schema_version;
    return out.str();
  }
  if (const auto mismatch = check("report_writer_id", evidence.report_writer_id,
                                  "jkimyei.training.representation."
                                  "mtf_jepa_mae_vicreg_graph_first_launcher")) {
    return mismatch;
  }
  if (evidence.report_writer_version.empty()) {
    return "report_writer_version expected non-empty but got <empty>";
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string>
strict_channel_mdn_health_mismatch(
    const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
        &fact) {
  const auto check_positive_finite =
      [](const char *field_name, double value) -> std::optional<std::string> {
    if (!std::isfinite(value)) {
      std::ostringstream out;
      out << field_name << " must be finite";
      return out.str();
    }
    if (value > 0.0) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected positive but got " << value;
    return out.str();
  };
  const auto check_nonnegative_finite =
      [](const char *field_name, double value) -> std::optional<std::string> {
    if (!std::isfinite(value)) {
      std::ostringstream out;
      out << field_name << " must be finite";
      return out.str();
    }
    if (value >= 0.0) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << field_name << " expected nonnegative but got " << value;
    return out.str();
  };
  const auto check_finite_vector =
      [](const char *field_name,
         const std::vector<double> &values) -> std::optional<std::string> {
    if (values.empty()) {
      std::ostringstream out;
      out << field_name << " expected at least one value";
      return out.str();
    }
    for (const auto value : values) {
      if (!std::isfinite(value)) {
        std::ostringstream out;
        out << field_name << " must contain only finite values";
        return out.str();
      }
    }
    return std::nullopt;
  };

  if (!fact.inference_health_available) {
    return "inference_health_available expected true but got false";
  }
  if (!fact.finite_parameter_check) {
    return "finite_parameter_check expected true but got false";
  }
  if (fact.nonfinite_output_count != 0) {
    std::ostringstream out;
    out << "nonfinite_output_count expected 0 but got "
        << fact.nonfinite_output_count;
    return out.str();
  }
  if (fact.valid_target_count <= 0) {
    std::ostringstream out;
    out << "valid_target_count expected positive but got "
        << fact.valid_target_count;
    return out.str();
  }
  if (const auto mismatch =
          check_positive_finite("mean_sigma_mean", fact.mean_sigma_mean)) {
    return mismatch;
  }
  if (const auto mismatch =
          check_positive_finite("min_sigma_min", fact.min_sigma_min)) {
    return mismatch;
  }
  if (const auto mismatch =
          check_positive_finite("max_sigma_max", fact.max_sigma_max)) {
    return mismatch;
  }
  if (fact.max_sigma_max < fact.min_sigma_min) {
    std::ostringstream out;
    out << "max_sigma_max expected >= min_sigma_min but got "
        << fact.max_sigma_max << " < " << fact.min_sigma_min;
    return out.str();
  }
  if (const auto mismatch = check_nonnegative_finite(
          "mean_mixture_entropy", fact.mean_mixture_entropy)) {
    return mismatch;
  }
  if (const auto mismatch = check_finite_vector("mean_nll_per_channel",
                                                fact.mean_nll_per_channel)) {
    return mismatch;
  }
  if (const auto mismatch = check_finite_vector(
          "mean_nll_per_target_feature", fact.mean_nll_per_target_feature)) {
    return mismatch;
  }
  if (const auto mismatch =
          check_finite_vector("mean_mixture_usage", fact.mean_mixture_usage)) {
    return mismatch;
  }
  double mixture_usage_sum = 0.0;
  for (const auto value : fact.mean_mixture_usage) {
    if (value < 0.0) {
      std::ostringstream out;
      out << "mean_mixture_usage expected nonnegative values but got " << value;
      return out.str();
    }
    mixture_usage_sum += value;
  }
  constexpr double k_usage_sum_tolerance = 1e-4;
  if (std::abs(mixture_usage_sum - 1.0) > k_usage_sum_tolerance) {
    std::ostringstream out;
    out << "mean_mixture_usage expected to sum to 1 but got "
        << mixture_usage_sum;
    return out.str();
  }
  return std::nullopt;
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

[[nodiscard]] inline lattice_target_proof_certificate_t::proof_context_t
make_proof_context(const lattice_target_spec_t &spec,
                   const lattice_target_active_identity_t &active_identity,
                   const lattice_target_evidence_t &evidence,
                   const std::string &expected_component_fingerprint,
                   const std::string &split_policy_fingerprint = {}) {
  lattice_target_proof_certificate_t::proof_context_t out{};
  out.require_contract_match = spec.require_contract_match;
  out.require_component_match = spec.require_component_match;
  out.require_graph_anchor_identity =
      target_requires_graph_anchor_identity(spec);
  out.active_contract_fingerprint =
      active_identity.protocol_contract_fingerprint;
  out.evidence_contract_fingerprint = evidence.protocol_contract_fingerprint;
  out.contract_match =
      !out.require_contract_match ||
      out.active_contract_fingerprint == out.evidence_contract_fingerprint;
  out.expected_component_fingerprint = expected_component_fingerprint;
  out.evidence_component_fingerprint = evidence.component_fingerprint;
  out.component_match =
      !out.require_component_match ||
      out.expected_component_fingerprint == out.evidence_component_fingerprint;
  out.active_graph_order_fingerprint = active_identity.graph_order_fingerprint;
  out.evidence_graph_order_fingerprint = evidence.graph_order_fingerprint;
  out.graph_order_match = !out.require_graph_anchor_identity ||
                          out.active_graph_order_fingerprint ==
                              out.evidence_graph_order_fingerprint;
  out.active_source_cursor_token = active_identity.source_cursor_token;
  out.evidence_source_cursor_token = evidence.source_cursor_token;
  out.source_cursor_match =
      !out.require_graph_anchor_identity ||
      out.active_source_cursor_token == out.evidence_source_cursor_token;
  out.component_family_id =
      !spec.component.empty() ? spec.component : evidence.component_family_id;
  out.target_spec_fingerprint = spec.target_spec_fingerprint;
  out.split_policy_fingerprint = split_policy_fingerprint;
  out.config_bundle_id = evidence.config_bundle_id;
  out.config_receipt_id = evidence.config_receipt_id;
  out.component_spawn_registry_id = evidence.component_spawn_registry_id;
  out.component_spawn_id = evidence.component_spawn_id;
  out.component_spawn_label = evidence.component_spawn_label;
  out.evidence_component_spawn_fingerprint =
      evidence.component_spawn_fingerprint;
  out.config_receipt_available =
      !out.config_bundle_id.empty() && !out.config_receipt_id.empty();
  out.spawn_id_readability_hint_only = true;

  namespace provenance = cuwacunu::kikijyeba::protocol::config_provenance;
  provenance::component_spawn_tuple_t spawn{};
  spawn.component_family_id = out.component_family_id;
  spawn.protocol_contract_fingerprint = out.active_contract_fingerprint;
  spawn.graph_order_fingerprint = out.active_graph_order_fingerprint;
  spawn.component_assembly_fingerprint = expected_component_fingerprint;
  out.computed_component_spawn_fingerprint =
      provenance::component_spawn_fingerprint(spawn);

  if (out.config_bundle_id.empty()) {
    out.provenance_warnings.push_back("missing config_bundle_id");
  }
  if (out.config_receipt_id.empty()) {
    out.provenance_warnings.push_back("missing config_receipt_id");
  }
  if (out.component_family_id.empty()) {
    out.provenance_warnings.push_back("missing component_family_id");
  }
  if (out.component_spawn_registry_id.empty()) {
    out.provenance_warnings.push_back("missing component_spawn_registry_id");
  }
  if (out.evidence_component_spawn_fingerprint.empty()) {
    out.provenance_warnings.push_back(
        "missing evidence component_spawn_fingerprint");
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> forbidden_use_names(
    const std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
        &uses) {
  std::vector<std::string> out;
  out.reserve(uses.size());
  for (const auto use : uses) {
    out.push_back(
        cuwacunu::kikijyeba::lattice::exposure::exposure_use_name(use));
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> use_set_names(
    const cuwacunu::kikijyeba::lattice::exposure::exposure_use_set_t &use) {
  std::vector<std::string> out;
  if (use.observed_input) {
    out.emplace_back("observed_input");
  }
  if (use.target_supervision) {
    out.emplace_back("target_supervision");
  }
  if (use.evaluation_metric) {
    out.emplace_back("evaluation_metric");
  }
  if (use.selection_signal) {
    out.emplace_back("selection_signal");
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
    return finalize_evaluation(evaluate_spec(*spec, /*dependency_stack=*/{}));
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
  [[nodiscard]] bool reject_non_checkpoint_latest_satisfying_reference(
      const lattice_target_spec_t &owner, const std::string &field_name,
      const std::string &source, lattice_target_evaluation_t &result,
      const std::string &dependency_kind = {}) const {
    const auto source_target_id = latest_satisfying_reference_target_id(source);
    if (source_target_id.empty()) {
      return false;
    }
    const auto *source_target = find_target(source_target_id);
    if (source_target == nullptr ||
        is_latest_satisfying_checkpoint_selectable_target_class(
            source_target->target_class)) {
      return false;
    }
    if (!dependency_kind.empty()) {
      result.proof_certificate.dependencies.push_back(
          lattice_target_proof_certificate_t::dependency_proof_t{
              .kind = dependency_kind,
              .source_target_id = source_target_id,
              .status = "blocked"});
    }
    result.status = lattice_target_status_t::blocked;
    result.reasons.push_back(
        field_name + " latest_satisfying cannot reference TARGET_CLASS=" +
        source_target->target_class + " target " + source_target_id +
        "; latest_satisfying selects checkpoint-producing "
        "readiness/leakage_guard targets");
    result.plan_ready = false;
    result.suggested_wave = {};
    return true;
  }

  static void assign_warning_threshold_relation(
      lattice_target_evaluation_t::warning_result_t &warning, bool triggered) {
    warning.threshold_triggered = false;
    if (!std::isfinite(warning.threshold)) {
      warning.threshold_relation = "no_threshold";
      return;
    }
    if (warning.threshold_direction != "above" &&
        warning.threshold_direction != "below") {
      warning.threshold_relation = "unknown_direction";
      return;
    }
    if (!warning.measurement_available ||
        !std::isfinite(warning.measured_value)) {
      warning.threshold_relation = "unavailable";
      return;
    }
    warning.threshold_triggered = triggered;
    if (warning.threshold_direction == "above") {
      warning.threshold_relation =
          triggered ? "above_threshold" : "not_above_threshold";
    } else {
      warning.threshold_relation =
          triggered ? "below_threshold" : "not_below_threshold";
    }
  }

  [[nodiscard]] static std::string
  warning_family_for_kind(const std::string &kind) {
    if (kind == "source_analytics") {
      return "source_health";
    }
    if (kind == "target_transform") {
      return "target_transform_contract";
    }
    if (kind == "forecast_eval") {
      return "forecast_quality_visibility";
    }
    if (kind == "forecast_baseline") {
      return "forecast_baseline_comparison";
    }
    if (kind == "observer_belief") {
      return "observer_belief_consistency";
    }
    if (kind == "allocation_engine") {
      return "allocation_engine_diagnostics";
    }
    if (kind == "mdn_distribution_calibration") {
      return "forecast_calibration";
    }
    if (kind == "node_support_balance" || kind == "node_support_floor") {
      return "forecast_support";
    }
    if (kind == "representation_health") {
      return "representation_health";
    }
    if (kind == "runtime_health") {
      return "runtime_health";
    }
    if (kind == "exposure_load" || kind == "effort_density") {
      return "exposure_load_visibility";
    }
    if (kind == "selection_signal") {
      return "selection_signal_audit";
    }
    if (kind == "replay_environment") {
      return "replay_environment_diagnostics";
    }
    return "lattice_target_diagnostics";
  }

  [[nodiscard]] static std::string warning_family_for_warning(
      const lattice_target_evaluation_t::warning_result_t &warning) {
    const auto &kind = warning.kind;
    const auto &metric = warning.metric;
    const auto &metric_family = warning.diagnostic_metric_family;
    if (kind == "forecast_eval") {
      if (metric_family == "interval_calibration") {
        return "forecast_calibration";
      }
      if (metric_family == "support_surface") {
        return "forecast_support";
      }
      if (metric_family == "baseline_comparison") {
        return "forecast_baseline_comparison";
      }
      return "forecast_quality_visibility";
    }
    if (kind == "forecast_baseline") {
      if (metric_family == "baseline_support_surface" ||
          metric == "support_count" || metric == "valid_count" ||
          metric == "missing_count") {
        return "forecast_support";
      }
      return "forecast_baseline_comparison";
    }
    if (kind == "observer_belief") {
      if (metric_family == "observer_lineage_integrity") {
        return "lineage_integrity";
      }
      return "observer_belief_consistency";
    }
    if (kind == "allocation_engine") {
      if (metric_family == "allocation_lineage_integrity") {
        return "lineage_integrity";
      }
      return "allocation_engine_diagnostics";
    }
    return warning_family_for_kind(kind);
  }

  [[nodiscard]] static std::string
  warning_fact_family_for_kind(const std::string &kind) {
    if (kind == "source_analytics" || kind == "target_transform" ||
        kind == "forecast_eval" || kind == "forecast_baseline" ||
        kind == "observer_belief" || kind == "allocation_engine" ||
        kind == "selection_signal" || kind == "replay_environment") {
      return kind;
    }
    if (kind == "mdn_distribution_calibration") {
      return "forecast_eval";
    }
    if (kind == "node_support_balance" || kind == "node_support_floor") {
      return "node_exposure";
    }
    if (kind == "representation_health") {
      return "representation_support";
    }
    return {};
  }

  [[nodiscard]] static std::string
  warning_inspection_panel_for_family(const std::string &family) {
    if (family == "source_health") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=source_analytics";
    }
    if (family == "target_transform_contract") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=target_transform";
    }
    if (family == "forecast_quality_visibility" ||
        family == "forecast_calibration" ||
        family == "forecast_baseline_comparison" ||
        family == "forecast_support") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=forecast_eval";
    }
    if (family == "lineage_integrity") {
      return "hero.lattice.inspect subject=facts mode=lineage";
    }
    if (family == "selection_signal_audit") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=selection_signal";
    }
    if (family == "observer_belief_consistency") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=observer_belief";
    }
    if (family == "allocation_engine_diagnostics") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=allocation_engine";
    }
    if (family == "replay_environment_diagnostics") {
      return "hero.lattice.inspect subject=facts mode=summary "
             "family=replay_environment";
    }
    if (family == "runtime_health") {
      return "hero.runtime.health";
    }
    return "hero.lattice.inspect subject=target";
  }

  [[nodiscard]] static std::string warning_reason_code(
      const lattice_target_evaluation_t::warning_result_t &warning) {
    std::string basis = warning.warning_family.empty()
                            ? warning_family_for_warning(warning)
                            : warning.warning_family;
    if (basis.empty()) {
      basis = "lattice_target_diagnostics";
    }
    std::string metric = warning.metric.empty() ? warning.kind : warning.metric;
    std::replace(metric.begin(), metric.end(), '-', '_');
    std::replace(basis.begin(), basis.end(), '-', '_');
    if (warning.threshold_triggered) {
      return basis + "_" + metric + "_" + warning.threshold_relation;
    }
    if (!warning.measurement_available) {
      return basis + "_" + metric + "_measurement_unavailable";
    }
    return basis + "_" + metric + "_clear";
  }

  [[nodiscard]] static std::string warning_evidence_digest(
      const lattice_target_evaluation_t &result,
      const lattice_target_evaluation_t::warning_result_t &warning) {
    std::ostringstream out;
    out << "kikijyeba.lattice.warning_envelope.v1\n";
    out << "target_id=" << result.target_id << "\n";
    out << "warning_id=" << warning.warning_id << "\n";
    out << "kind=" << warning.kind << "\n";
    out << "warning_family=" << warning.warning_family << "\n";
    out << "metric=" << warning.metric << "\n";
    out << "status=" << warning.status << "\n";
    out << "severity=" << warning.severity << "\n";
    out << "threshold_relation=" << warning.threshold_relation << "\n";
    out << "threshold_triggered="
        << (warning.threshold_triggered ? "true" : "false") << "\n";
    out << "measurement_available="
        << (warning.measurement_available ? "true" : "false") << "\n";
    out << "measured_value=" << warning.measured_value << "\n";
    out << "threshold=" << warning.threshold << "\n";
    out << "split=" << warning.split << "\n";
    out << "anchor_begin=" << warning.warning_anchor_range.begin << "\n";
    out << "anchor_end=" << warning.warning_anchor_range.end << "\n";
    out << "evidence_basis=" << warning.evidence_basis << "\n";
    out << "fact_family=" << warning.fact_family << "\n";
    out << "fact_digest=" << warning.fact_digest << "\n";
    return cuwacunu::kikijyeba::lattice::exposure::exposure_digest_for_text(
        out.str());
  }

  static void normalize_warning_results(lattice_target_evaluation_t &result) {
    for (auto &warning : result.warning_results) {
      warning.target_id = result.target_id;
      if (warning.source.empty()) {
        warning.source = "lattice";
      }
      if (warning.component.empty()) {
        warning.component = "lattice_target";
      }
      warning.blocking = false;
      warning.readiness_effect = "non_blocking_warning_only";
      if (warning.warning_family.empty()) {
        warning.warning_family = warning_family_for_warning(warning);
      }
      if (warning.fact_family.empty()) {
        warning.fact_family = warning_fact_family_for_kind(warning.kind);
      }
      if (warning.target_ids_observed_against.empty() &&
          !result.target_id.empty()) {
        warning.target_ids_observed_against.push_back(result.target_id);
      }
      if (warning.machine_reason_code.empty()) {
        warning.machine_reason_code = warning_reason_code(warning);
      }
      if (warning.human_explanation.empty()) {
        warning.human_explanation = warning.message;
      }
      if (warning.suggested_inspection_panel.empty()) {
        warning.suggested_inspection_panel =
            warning_inspection_panel_for_family(warning.warning_family);
      }
      if (warning.evidence_digest.empty()) {
        warning.evidence_digest = warning_evidence_digest(result, warning);
      }
    }
  }

  [[nodiscard]] static lattice_target_evaluation_t::warning_summary_t
  make_warning_summary(
      const std::vector<lattice_target_evaluation_t::warning_result_t>
          &warnings) {
    lattice_target_evaluation_t::warning_summary_t out{};
    out.warning_result_count = static_cast<std::int64_t>(warnings.size());
    for (const auto &warning : warnings) {
      if (warning.threshold_triggered) {
        ++out.triggered_warning_count;
      }
      if (!warning.measurement_available) {
        ++out.unavailable_warning_count;
      }
      if (!warning.threshold_triggered && warning.measurement_available) {
        ++out.clear_measured_warning_count;
      }
      if (warning.blocking) {
        ++out.blocking_warning_count;
      } else {
        ++out.non_blocking_warning_count;
      }

      if (warning.threshold_relation == "above_threshold") {
        ++out.above_threshold_count;
      } else if (warning.threshold_relation == "not_above_threshold") {
        ++out.not_above_threshold_count;
      } else if (warning.threshold_relation == "below_threshold") {
        ++out.below_threshold_count;
      } else if (warning.threshold_relation == "not_below_threshold") {
        ++out.not_below_threshold_count;
      } else if (warning.threshold_relation == "unavailable") {
        ++out.unavailable_relation_count;
      } else if (warning.threshold_relation == "no_threshold") {
        ++out.no_threshold_relation_count;
      } else if (warning.threshold_relation == "unknown_direction") {
        ++out.unknown_direction_relation_count;
      }
    }
    out.warning_count = out.triggered_warning_count;
    out.all_warnings_non_blocking = out.blocking_warning_count == 0;
    return out;
  }

  void bind_proof_obligation(lattice_target_evaluation_t &result,
                             const lattice_target_spec_t &spec) const {
    result.proof_certificate.target_id = spec.target_id;
    result.proof_certificate.target_spec_fingerprint =
        spec.target_spec_fingerprint;
    result.proof_certificate.split_policy_fingerprint =
        result.split_policy_fingerprint;
  }

  void append_deficit(
      lattice_target_evaluation_t &result, std::string kind,
      std::string dimension, std::string status, std::string message,
      double required = std::numeric_limits<double>::quiet_NaN(),
      double actual = std::numeric_limits<double>::quiet_NaN(),
      std::string unit = {},
      cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t range = {},
      std::vector<cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t>
          missing_intervals = {},
      bool plan_relevant = true,
      std::vector<std::string> related_fact_integrity_issue_codes = {}) const {
    lattice_target_evaluation_t::proof_deficit_t deficit{};
    deficit.kind = std::move(kind);
    deficit.dimension = std::move(dimension);
    deficit.key = deficit.kind + ":" + deficit.dimension;
    deficit.status = std::move(status);
    const auto [priority, priority_class] =
        lattice_deficit_priority_for(deficit.kind, deficit.dimension);
    deficit.priority = priority;
    deficit.priority_class = priority_class;
    deficit.message = std::move(message);
    deficit.required = required;
    deficit.actual = actual;
    deficit.unit = std::move(unit);
    deficit.range = range;
    deficit.missing_intervals = std::move(missing_intervals);
    deficit.plan_relevant = plan_relevant;
    deficit.related_fact_integrity_issue_codes =
        std::move(related_fact_integrity_issue_codes);
    result.deficits.push_back(std::move(deficit));
  }

  [[nodiscard]] static bool contains_interval(
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t> &intervals,
      cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t interval) {
    return std::any_of(intervals.begin(), intervals.end(),
                       [&](const auto &existing) {
                         return existing.begin == interval.begin &&
                                existing.end == interval.end;
                       });
  }

  [[nodiscard]] static std::string
  join_keys(const std::vector<std::string> &keys) {
    std::ostringstream out;
    for (std::size_t i = 0; i < keys.size(); ++i) {
      if (i != 0) {
        out << ", ";
      }
      out << keys[i];
    }
    return out.str();
  }

  [[nodiscard]] static std::string
  artifact_issue_deficit_status(const std::string &issue) {
    if (issue.starts_with("missing_") ||
        issue.find("_not_found") != std::string::npos) {
      return "missing";
    }
    if (issue.find("mismatch") != std::string::npos) {
      return "mismatch";
    }
    if (issue.find("_authority") != std::string::npos ||
        issue.find("_must_remain_") != std::string::npos ||
        issue.find("_cannot_be_") != std::string::npos) {
      return "forbidden";
    }
    return "failed";
  }

  [[nodiscard]] static bool
  fact_integrity_issue_code_matches(const std::string &code,
                                    const std::string &issue) {
    if (code == issue) {
      return true;
    }
    const auto separator = code.rfind(':');
    if (separator == std::string::npos || separator + 1 >= code.size()) {
      return false;
    }
    return code.substr(separator + 1) == issue;
  }

  [[nodiscard]] static std::vector<std::string>
  matching_fact_integrity_issue_codes(const lattice_target_evaluation_t &result,
                                      const std::string &issue) {
    std::vector<std::string> out;
    if (!result.fact_integrity_summary_available || issue.empty()) {
      return out;
    }
    const auto append_if_match = [&](const std::string &code) {
      if (fact_integrity_issue_code_matches(code, issue) &&
          std::find(out.begin(), out.end(), code) == out.end()) {
        out.push_back(code);
      }
    };
    for (const auto &code : result.fact_integrity_summary.issue_codes) {
      append_if_match(code);
    }
    for (const auto &family : result.fact_integrity_summary.families) {
      for (const auto &code : family.issues) {
        append_if_match(code);
      }
    }
    return out;
  }

  [[nodiscard]] static std::vector<std::string> artifact_authority_flags(
      const lattice_target_proof_certificate_t::artifact_proof_t &artifact) {
    std::vector<std::string> flags;
    if (artifact.readiness_authority) {
      flags.emplace_back("readiness_authority");
    }
    if (artifact.quality_authority) {
      flags.emplace_back("quality_authority");
    }
    if (artifact.performance_authority) {
      flags.emplace_back("performance_authority");
    }
    if (artifact.checkpoint_selector) {
      flags.emplace_back("checkpoint_selector");
    }
    if (artifact.coverage_authority) {
      flags.emplace_back("coverage_authority");
    }
    if (artifact.leakage_authority) {
      flags.emplace_back("leakage_authority");
    }
    if (artifact.contract_identity_authority) {
      flags.emplace_back("contract_identity_authority");
    }
    if (artifact.allocation_authority) {
      flags.emplace_back("allocation_authority");
    }
    if (artifact.execution_authority) {
      flags.emplace_back("execution_authority");
    }
    if (artifact.market_readiness_authority) {
      flags.emplace_back("market_readiness_authority");
    }
    if (artifact.deployment_authority) {
      flags.emplace_back("deployment_authority");
    }
    if (artifact.policy_gate) {
      flags.emplace_back("policy_gate");
    }
    if (artifact.target_dependency_authority) {
      flags.emplace_back("target_dependency_authority");
    }
    if (artifact.runtime_wave_authority) {
      flags.emplace_back("runtime_wave_authority");
    }
    if (artifact.marshal_reachability) {
      flags.emplace_back("marshal_reachability");
    }
    if (artifact.checkpoint_source_authority) {
      flags.emplace_back("checkpoint_source_authority");
    }
    if (artifact.plan_checkpoint_input_authority) {
      flags.emplace_back("plan_checkpoint_input_authority");
    }
    if (artifact.model_state_mutation) {
      flags.emplace_back("model_state_mutation");
    }
    if (artifact.raw_potential_tradable_return) {
      flags.emplace_back("raw_potential_tradable_return");
    }
    if (artifact.replay_executor) {
      flags.emplace_back("replay_executor");
    }
    if (artifact.fact_identity_target_kind_authority) {
      flags.emplace_back("fact_identity_target_kind_authority");
    }
    if (artifact.fact_identity_runtime_wave_authority) {
      flags.emplace_back("fact_identity_runtime_wave_authority");
    }
    if (artifact.fact_identity_marshal_reachability) {
      flags.emplace_back("fact_identity_marshal_reachability");
    }
    if (artifact.fact_identity_policy_gate_authority) {
      flags.emplace_back("fact_identity_policy_gate_authority");
    }
    return flags;
  }

  static void append_fact_preview_hint(
      lattice_target_evaluation_t::plan_basis_t &basis,
      const lattice_target_proof_certificate_t::artifact_proof_t &artifact,
      const std::string &fallback_family) {
    const std::string family =
        artifact.fact_family.empty() ? fallback_family : artifact.fact_family;
    if (family.empty() || artifact.fact_digest.empty()) {
      return;
    }
    const auto duplicate =
        std::find_if(basis.fact_preview_hints.begin(),
                     basis.fact_preview_hints.end(), [&](const auto &hint) {
                       return hint.fact_family == family &&
                              hint.fact_digest == artifact.fact_digest;
                     });
    if (duplicate != basis.fact_preview_hints.end()) {
      return;
    }
    lattice_target_evaluation_t::plan_basis_t::fact_preview_hint_t hint{};
    hint.fact_family = family;
    hint.fact_digest = artifact.fact_digest;
    basis.fact_preview_hints.push_back(std::move(hint));
  }

  [[nodiscard]] lattice_target_evaluation_t::plan_basis_t
  make_plan_basis(const lattice_target_evaluation_t &result) const {
    lattice_target_evaluation_t::plan_basis_t out{};
    if (result.status == lattice_target_status_t::satisfied) {
      out.reason = "target already satisfied; no wave suggested";
      return out;
    }
    if (is_artifact_readiness_target_class(result.target_class)) {
      for (const auto &artifact : result.proof_certificate.artifacts) {
        if (!artifact.passed) {
          append_fact_preview_hint(out, artifact, result.subject_fact_family);
        }
      }
      for (const auto &deficit : result.deficits) {
        const std::string key = deficit.key.empty()
                                    ? deficit.kind + ":" + deficit.dimension
                                    : deficit.key;
        if (out.primary_deficit_key.empty() ||
            deficit.priority < out.primary_deficit_priority) {
          out.primary_deficit_key = key;
          out.primary_deficit_message = deficit.message;
          out.primary_deficit_priority = deficit.priority;
          out.primary_deficit_priority_class = deficit.priority_class;
        }
        if (std::find(out.deficit_keys.begin(), out.deficit_keys.end(), key) ==
            out.deficit_keys.end()) {
          out.deficit_keys.push_back(key);
        }
        if (!deficit.message.empty() &&
            std::find(out.deficit_messages.begin(), out.deficit_messages.end(),
                      deficit.message) == out.deficit_messages.end()) {
          out.deficit_messages.push_back(deficit.message);
        }
        if (!deficit.priority_class.empty() &&
            std::find(out.deficit_priority_classes.begin(),
                      out.deficit_priority_classes.end(),
                      deficit.priority_class) ==
                out.deficit_priority_classes.end()) {
          out.deficit_priority_classes.push_back(deficit.priority_class);
        }
      }
      out.reason =
          out.deficit_keys.empty()
              ? "artifact_readiness target is non-dispatchable; inspect "
                "evidence catalog"
              : "artifact_readiness target is non-dispatchable; inspect "
                "evidence catalog for proof deficits: " +
                    join_keys(out.deficit_keys);
      out.suggested_action = "inspect";
      return out;
    }

    for (const auto &deficit : result.deficits) {
      if (!deficit.plan_relevant) {
        continue;
      }
      const std::string key = deficit.key.empty()
                                  ? deficit.kind + ":" + deficit.dimension
                                  : deficit.key;
      if (out.primary_deficit_key.empty() ||
          deficit.priority < out.primary_deficit_priority) {
        out.primary_deficit_key = key;
        out.primary_deficit_message = deficit.message;
        out.primary_deficit_priority = deficit.priority;
        out.primary_deficit_priority_class = deficit.priority_class;
      }
      if (std::find(out.deficit_keys.begin(), out.deficit_keys.end(), key) ==
          out.deficit_keys.end()) {
        out.deficit_keys.push_back(key);
      }
      if (!deficit.message.empty() &&
          std::find(out.deficit_messages.begin(), out.deficit_messages.end(),
                    deficit.message) == out.deficit_messages.end()) {
        out.deficit_messages.push_back(deficit.message);
      }
      if (!deficit.priority_class.empty() &&
          std::find(out.deficit_priority_classes.begin(),
                    out.deficit_priority_classes.end(),
                    deficit.priority_class) ==
              out.deficit_priority_classes.end()) {
        out.deficit_priority_classes.push_back(deficit.priority_class);
      }
      if (deficit.kind == "coverage") {
        if (out.target_range.empty() && !deficit.range.empty()) {
          out.target_range = deficit.range;
        }
        for (const auto &missing : deficit.missing_intervals) {
          if (!contains_interval(out.missing_intervals, missing)) {
            out.missing_intervals.push_back(missing);
          }
        }
      }
    }

    if (!result.plan_ready) {
      out.reason = out.deficit_keys.empty()
                       ? "target is unsatisfied, but no wave is plan-ready"
                       : "no plan-ready wave for proof deficits: " +
                             join_keys(out.deficit_keys);
      return out;
    }
    if (result.suggested_wave.empty()) {
      out.reason = out.deficit_keys.empty()
                       ? "target is unsatisfied, but no wave is suggested"
                       : "no suggested wave for proof deficits: " +
                             join_keys(out.deficit_keys);
      return out;
    }

    out.available = true;
    out.suggested_action = result.suggested_wave.target;
    if (!result.suggested_wave.mode.empty()) {
      out.suggested_action += " " + result.suggested_wave.mode;
    }

    if (out.deficit_keys.empty()) {
      out.reason = "suggested wave follows target status " +
                   std::string(lattice_target_status_name(result.status));
    } else {
      out.reason = "suggested wave addresses proof deficits: " +
                   join_keys(out.deficit_keys);
    }
    return out;
  }

  [[nodiscard]] static lattice_target_evaluation_t::evidence_order_vector_t
  make_evidence_order_vector(const lattice_target_evaluation_t &result) {
    lattice_target_evaluation_t::evidence_order_vector_t out{};
    const auto &proof = result.proof_certificate;
    out.available = !proof.target_id.empty();
    out.satisfied = result.status == lattice_target_status_t::satisfied;
    out.proof_certificate_check_passed = result.proof_certificate_check.passed;

    const auto &proof_context = proof.proof_context;
    out.identity_contract_match =
        !proof_context.require_contract_match || proof_context.contract_match;
    out.identity_component_match =
        !proof_context.require_component_match || proof_context.component_match;
    out.identity_graph_order_match =
        !proof_context.require_graph_anchor_identity ||
        proof_context.graph_order_match;
    out.identity_source_cursor_match =
        !proof_context.require_graph_anchor_identity ||
        proof_context.source_cursor_match;

    bool saw_coverage = false;
    for (const auto &coverage : proof.coverage) {
      if (std::isfinite(coverage.actual_fraction)) {
        out.min_unique_coverage_fraction =
            saw_coverage ? std::min(out.min_unique_coverage_fraction,
                                    coverage.actual_fraction)
                         : coverage.actual_fraction;
        saw_coverage = true;
      }
      if (std::isfinite(coverage.load_summary.cursor_exposure_load)) {
        out.max_cursor_exposure_load =
            std::isfinite(out.max_cursor_exposure_load)
                ? std::max(out.max_cursor_exposure_load,
                           coverage.load_summary.cursor_exposure_load)
                : coverage.load_summary.cursor_exposure_load;
      }
    }

    out.closure_checked = proof.closure.checked;
    out.closure_complete = proof.closure.complete;
    out.leakage_checked = proof.leakage.checked;
    out.leakage_passed = proof.leakage.passed;
    out.selection_signal_leakage_found = std::any_of(
        proof.leakage.overlap_witnesses.begin(),
        proof.leakage.overlap_witnesses.end(), [](const auto &witness) {
          return witness.use == exposure::exposure_use_t::selection_signal;
        });
    out.unresolved_lineage_count =
        static_cast<std::int64_t>(
            proof.closure.unresolved_input_checkpoints.size()) +
        static_cast<std::int64_t>(proof.closure.identity_mismatches.size());

    for (const auto &summary : proof.node_support_summaries) {
      if (std::isfinite(summary.valid_target_wilson_lower_95)) {
        out.aggregate_valid_target_wilson_lower_95 =
            std::isfinite(out.aggregate_valid_target_wilson_lower_95)
                ? std::min(out.aggregate_valid_target_wilson_lower_95,
                           summary.valid_target_wilson_lower_95)
                : summary.valid_target_wilson_lower_95;
      }
      if (std::isfinite(summary.weakest_valid_target_wilson_lower_95)) {
        out.weakest_node_valid_target_wilson_lower_95 =
            std::isfinite(out.weakest_node_valid_target_wilson_lower_95)
                ? std::min(out.weakest_node_valid_target_wilson_lower_95,
                           summary.weakest_valid_target_wilson_lower_95)
                : summary.weakest_valid_target_wilson_lower_95;
      }
      if (std::isfinite(summary.valid_target_count_coefficient_of_variation)) {
        out.max_node_support_coefficient_of_variation =
            std::isfinite(out.max_node_support_coefficient_of_variation)
                ? std::max(out.max_node_support_coefficient_of_variation,
                           summary.valid_target_count_coefficient_of_variation)
                : summary.valid_target_count_coefficient_of_variation;
      }
      if (std::isfinite(summary.valid_target_count_gini)) {
        out.max_node_support_gini =
            std::isfinite(out.max_node_support_gini)
                ? std::max(out.max_node_support_gini,
                           summary.valid_target_count_gini)
                : summary.valid_target_count_gini;
      }
    }

    for (const auto &causal : proof.closure.causal_exposures) {
      const auto &audit = causal.source_key_window_audit;
      if (!audit.available) {
        continue;
      }
      ++out.source_key_audit_count;
      out.source_key_audit_issue_count +=
          static_cast<std::int64_t>(audit.issues.size());
      if (audit.affine_step_available && !audit.affine_consistent) {
        ++out.source_key_affine_inconsistent_count;
      }
    }

    out.blocking_deficit_count =
        static_cast<std::int64_t>(result.deficits.size());
    out.warning_result_count = result.warning_summary.warning_result_count;
    out.triggered_warning_count =
        result.warning_summary.triggered_warning_count;
    out.unavailable_warning_count =
        result.warning_summary.unavailable_warning_count;
    out.warning_count = result.warning_summary.warning_count;
    return out;
  }

  [[nodiscard]] lattice_target_evaluation_t
  finalize_evaluation(lattice_target_evaluation_t result) const {
    result.proof_certificate.certificate_digest =
        lattice_target_proof_certificate_digest(result.proof_certificate);
    result.proof_certificate_check =
        verify_lattice_target_proof_certificate(result.proof_certificate);
    normalize_warning_results(result);
    result.warning_summary = make_warning_summary(result.warning_results);
    if (result.status == lattice_target_status_t::satisfied &&
        !result.proof_certificate_check.passed) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back("proof certificate self-check failed");
      result.plan_ready = false;
      result.suggested_wave = {};
    }
    result = with_deficit_vector(std::move(result));
    result.plan_basis = make_plan_basis(result);
    result.evidence_order_vector = make_evidence_order_vector(result);
    return result;
  }

  void
  append_artifact_proof_deficits(lattice_target_evaluation_t &result) const {
    for (const auto &artifact : result.proof_certificate.artifacts) {
      if (artifact.passed) {
        continue;
      }
      const std::string family = artifact.fact_family.empty()
                                     ? result.subject_fact_family
                                     : artifact.fact_family;
      if (!artifact.identity_match) {
        append_deficit(result, "artifact", family + "_identity", "mismatch",
                       "artifact proof identity does not match active target "
                       "identity");
      }
      if (!artifact.artifact_evidence) {
        append_deficit(result, "artifact", family + "_artifact_evidence",
                       "missing",
                       "artifact proof does not carry artifact evidence");
      }
      if (!artifact.fact_identity_contract_bound) {
        append_deficit(result, "artifact", family + "_fact_identity_contract",
                       "missing",
                       "artifact proof is not bound to the catalog fact "
                       "identity contract");
      }
      if (!artifact.fact_identity_envelope_complete) {
        append_deficit(result, "artifact", family + "_fact_identity_envelope",
                       "missing",
                       "artifact proof does not carry a complete fact identity "
                       "envelope");
      }
      if (!artifact.deterministic_artifact) {
        append_deficit(result, "artifact", family + "_deterministic_artifact",
                       "missing",
                       "artifact proof does not prove a deterministic "
                       "artifact");
      }
      if (!artifact.visibility_only) {
        append_deficit(result, "artifact", family + "_visibility_only",
                       "missing",
                       "artifact proof is not marked visibility-only");
      }
      const auto authority_flags = artifact_authority_flags(artifact);
      if (!authority_flags.empty() || !artifact.authority_clean) {
        append_deficit(result, "artifact", family + "_authority", "forbidden",
                       authority_flags.empty()
                           ? "artifact proof contains authority drift"
                           : "artifact proof contains forbidden authority "
                             "flags: " +
                                 join_keys(authority_flags));
      }
      if (!artifact.lineage_bound) {
        append_deficit(result, "artifact", family + "_lineage", "missing",
                       "artifact proof lineage is not bound");
      }
      for (const auto &issue : artifact.issues) {
        if (issue.empty()) {
          continue;
        }
        auto related_issue_codes =
            matching_fact_integrity_issue_codes(result, issue);
        append_deficit(result, "artifact", family + "_issue_" + issue,
                       artifact_issue_deficit_status(issue),
                       "artifact proof issue: " + issue,
                       std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::quiet_NaN(), {}, {}, {},
                       true, std::move(related_issue_codes));
      }
    }
  }

  void
  append_fact_integrity_deficits(lattice_target_evaluation_t &result) const {
    if (!is_artifact_readiness_target_class(result.target_class) ||
        !result.fact_integrity_summary_available ||
        result.fact_integrity_summary.relation_integrity_clean) {
      return;
    }

    const auto &summary = result.fact_integrity_summary;
    const std::string family = result.subject_fact_family.empty()
                                   ? "fact_family"
                                   : result.subject_fact_family;
    if (summary.unresolved_relation_count > 0) {
      append_deficit(
          result, "fact_integrity", family + "_unresolved_relation", "missing",
          "fact catalog has unresolved relation digests for " + family, 0.0,
          static_cast<double>(summary.unresolved_relation_count),
          "relation_count", {}, {}, false, summary.issue_codes);
    }
    if (summary.identity_mismatch_count > 0) {
      append_deficit(result, "fact_integrity", family + "_identity_mismatch",
                     "mismatch",
                     "fact catalog has identity-mismatched relation digests "
                     "for " +
                         family,
                     0.0, static_cast<double>(summary.identity_mismatch_count),
                     "relation_count", {}, {}, false, summary.issue_codes);
    }
    if (summary.digest_mismatch_count > 0) {
      append_deficit(result, "fact_integrity", family + "_digest_mismatch",
                     "mismatch",
                     "fact catalog has digest-mismatched relation evidence "
                     "for " +
                         family,
                     0.0, static_cast<double>(summary.digest_mismatch_count),
                     "relation_count", {}, {}, false, summary.issue_codes);
    }
  }

  [[nodiscard]] lattice_target_evaluation_t
  with_deficit_vector(lattice_target_evaluation_t result) const {
    result.deficits.clear();
    if (result.status == lattice_target_status_t::satisfied) {
      return result;
    }

    const auto &proof_context = result.proof_certificate.proof_context;
    if (proof_context.require_contract_match && !proof_context.contract_match) {
      append_deficit(result, "proof_context", "contract_fingerprint",
                     "mismatch",
                     "evidence contract fingerprint does not match the active "
                     "contract fingerprint");
    }
    if (proof_context.require_component_match &&
        !proof_context.component_match) {
      append_deficit(result, "proof_context", "component_assembly_fingerprint",
                     "mismatch",
                     "evidence component fingerprint does not match the active "
                     "component assembly fingerprint");
    }
    if (proof_context.require_graph_anchor_identity &&
        !proof_context.graph_order_match) {
      append_deficit(result, "proof_context", "graph_order_fingerprint",
                     "mismatch",
                     "evidence graph order fingerprint does not match the "
                     "active graph order fingerprint");
    }
    if (proof_context.require_graph_anchor_identity &&
        !proof_context.source_cursor_match) {
      append_deficit(result, "proof_context", "source_cursor_token", "mismatch",
                     "evidence source cursor token does not match the active "
                     "source cursor token");
    }
    const bool has_failed_artifact_proof =
        is_artifact_readiness_target_class(result.target_class) &&
        std::any_of(result.proof_certificate.artifacts.begin(),
                    result.proof_certificate.artifacts.end(),
                    [](const auto &artifact) { return !artifact.passed; });
    if (has_failed_artifact_proof) {
      append_artifact_proof_deficits(result);
    }
    append_fact_integrity_deficits(result);
    if (!result.proof_certificate_check.passed && !has_failed_artifact_proof) {
      append_deficit(result, "certificate", "proof_certificate_check", "failed",
                     "proof certificate self-check failed");
    }

    for (const auto &dependency : result.proof_certificate.dependencies) {
      if (dependency.status != "satisfied") {
        append_deficit(result, "dependency", dependency.kind, dependency.status,
                       "dependency target is not satisfied: " +
                           dependency.source_target_id);
      }
      if (!dependency.expected_mdn_checkpoint.empty() &&
          !dependency.mdn_checkpoint_match) {
        append_deficit(result, "dependency", "mdn_checkpoint", "mismatch",
                       "loaded MDN checkpoint does not match dependency " +
                           dependency.source_target_id);
      }
      if (!dependency.expected_representation_checkpoint.empty() &&
          !dependency.representation_checkpoint_match) {
        append_deficit(
            result, "dependency", "representation_checkpoint", "mismatch",
            "loaded representation checkpoint does not match dependency " +
                dependency.source_target_id);
      }
    }

    for (const auto &coverage : result.proof_certificate.coverage) {
      if (!coverage.passed) {
        append_deficit(result, "coverage", coverage.use, "below_required",
                       coverage.use + " coverage is below the required "
                                      "cursor coverage fraction",
                       coverage.required_fraction, coverage.actual_fraction,
                       "coverage_fraction", coverage.target_range,
                       coverage.missing_intervals);
      }
    }
    if (result.status == lattice_target_status_t::metric_failed) {
      for (const auto &gate : result.representation_geometry_gate_results) {
        if (gate.passed) {
          continue;
        }
        append_deficit(result, "metric", "representation_geometry", gate.status,
                       gate.message, gate.threshold, gate.measured_value,
                       gate.unit);
      }
      for (const auto &reason : result.reasons) {
        if (reason == "loss is not finite") {
          append_deficit(result, "metric", "loss", "non_finite", reason,
                         std::numeric_limits<double>::quiet_NaN(),
                         result.evidence.loss, "loss");
        } else if (reason == "optimizer_steps below MIN_OPTIMIZER_STEPS") {
          append_deficit(result, "metric", "optimizer_steps", "below_required",
                         reason, std::numeric_limits<double>::quiet_NaN(),
                         static_cast<double>(result.evidence.optimizer_steps),
                         "count");
        } else if (reason ==
                   "valid target fraction below MIN_VALID_TARGET_FRACTION") {
          append_deficit(result, "metric", "valid_target_fraction",
                         "below_required", reason,
                         std::numeric_limits<double>::quiet_NaN(),
                         result.evidence.valid_target_fraction, "fraction");
        } else if (reason == "active node head count below "
                             "REQUIRE_ACTIVE_NODE_HEAD_COUNT") {
          append_deficit(
              result, "metric", "active_node_head_count", "below_required",
              reason, std::numeric_limits<double>::quiet_NaN(),
              static_cast<double>(result.evidence.active_node_head_count),
              "count");
        } else if (reason == "trained node head count below "
                             "REQUIRE_TRAINED_NODE_HEAD_COUNT") {
          append_deficit(
              result, "metric", "trained_node_head_count", "below_required",
              reason, std::numeric_limits<double>::quiet_NaN(),
              static_cast<double>(result.evidence.trained_node_head_count),
              "count");
        } else if (reason == "evaluated node head count below "
                             "REQUIRE_EVALUATED_NODE_HEAD_COUNT") {
          append_deficit(
              result, "metric", "evaluated_node_head_count", "below_required",
              reason, std::numeric_limits<double>::quiet_NaN(),
              static_cast<double>(result.evidence.evaluated_node_head_count),
              "count");
        }
      }
    }
    for (const auto &reason : result.reasons) {
      if (reason == "active contract fingerprint is required but was not "
                    "provided") {
        append_deficit(result, "proof_context", "contract_fingerprint",
                       "missing", reason);
      } else if (reason == "active component fingerprint is required but was "
                           "not provided") {
        append_deficit(result, "proof_context",
                       "component_assembly_fingerprint", "missing", reason);
      } else if (reason == "active graph order fingerprint is required for "
                           "graph-anchor lattice targets but was not "
                           "provided") {
        append_deficit(result, "proof_context", "graph_order_fingerprint",
                       "missing", reason);
      } else if (reason == "active source cursor token is required for "
                           "graph-anchor lattice targets but was not "
                           "provided") {
        append_deficit(result, "proof_context", "source_cursor_token",
                       "missing", reason);
      } else if (reason.starts_with("[lattice_target] target references "
                                    "lattice split names but no split policy "
                                    "was loaded") ||
                 reason.starts_with("[lattice_target] unknown TRAIN_SPLIT: ") ||
                 reason.starts_with(
                     "[lattice_target] unknown FORBID_SPLIT: ") ||
                 reason.starts_with("[lattice_target] PROTECT_SPLIT "
                                    "references split without "
                                    "PROTECT_FROM_USES: ") ||
                 reason.starts_with(
                     "[lattice_target] unknown LATTICE_WARN SPLIT: ") ||
                 reason.starts_with("[lattice_target] LATTICE_WARN must use "
                                    "either SPLIT or explicit "
                                    "ANCHOR_INDEX_BEGIN/END")) {
        append_deficit(result, "artifact", "split_policy", "missing", reason);
      } else if (reason == "cyclic lattice target dependency") {
        append_deficit(result, "dependency", "target_cycle", "blocked", reason);
      } else if (reason.starts_with(
                     "no matching job manifest/report found under")) {
        append_deficit(result, "artifact", "job_manifest_report", "missing",
                       reason);
      } else if (is_artifact_readiness_target_class(result.target_class) &&
                 reason.starts_with("no " + result.subject_fact_family +
                                    " artifact facts found under")) {
        append_deficit(result, "artifact", result.subject_fact_family + "_fact",
                       "missing", reason);
      } else if (is_artifact_readiness_target_class(result.target_class) &&
                 reason.starts_with("no " + result.subject_fact_family +
                                    " artifact facts matched the target "
                                    "identity")) {
        append_deficit(result, "artifact",
                       result.subject_fact_family + "_identity", "mismatch",
                       reason);
      } else if (is_artifact_readiness_target_class(result.target_class) &&
                 reason.starts_with("no clean " + result.subject_fact_family +
                                    " artifact proof matched ")) {
        append_deficit(result, "artifact",
                       result.subject_fact_family + "_proof", "failed", reason);
      } else if (is_artifact_readiness_target_class(result.target_class) &&
                 (reason.find("_must_remain_artifact_evidence_only") !=
                      std::string::npos ||
                  reason.find("_must_remain_audit_evidence_only") !=
                      std::string::npos ||
                  reason.find("_cannot_be_tradable_return") !=
                      std::string::npos)) {
        append_deficit(result, "artifact",
                       result.subject_fact_family + "_authority", "forbidden",
                       reason);
      } else if (reason == "target has exposure requirements but no exposure "
                           "ledger was provided") {
        append_deficit(result, "artifact", "exposure_ledger", "missing",
                       reason);
      } else if (reason.starts_with(
                     "matching job exists but component report is missing")) {
        append_deficit(result, "artifact", "component_report", "missing",
                       reason);
      } else if (reason.starts_with("MTF report schema mismatch: ")) {
        append_deficit(result, "artifact", "report_schema", "mismatch", reason);
      } else if (reason == "candidate job status is missing") {
        append_deficit(result, "artifact", "job_state", "missing", reason);
      } else if (reason.starts_with("candidate job status is not completed")) {
        append_deficit(result, "artifact", "job_status", "not_completed",
                       reason);
      } else if (reason == "compatible checkpoint artifact is missing" ||
                 reason == "MTF checkpoint digest verification failed" ||
                 reason.starts_with("CHECKPOINT_SOURCE "
                                    "latest_satisfying target has no "
                                    "checkpoint artifact") ||
                 reason.starts_with("EVALUATED_CHECKPOINT_SOURCE "
                                    "latest_satisfying target has no "
                                    "checkpoint artifact")) {
        append_deficit(result, "artifact", "checkpoint",
                       reason == "MTF checkpoint digest verification failed"
                           ? "digest_mismatch"
                           : "missing",
                       reason);
      } else if (reason.starts_with(
                     "exposure ledger has no checkpoint closure facts for ")) {
        append_deficit(result, "artifact", "checkpoint_closure_fact", "missing",
                       reason);
      } else if (reason.starts_with("exposure ledger has no evaluated "
                                    "checkpoint closure facts for ")) {
        append_deficit(result, "artifact", "evaluated_checkpoint_closure_fact",
                       "missing", reason);
      } else if (reason == "exposure ledger has no active graph/"
                           "source-compatible facts for this target/"
                           "checkpoint") {
        append_deficit(result, "artifact", "exposure_fact", "missing", reason);
      } else if (reason == "loaded representation checkpoint has no active "
                           "compatible representation producer exposure fact" ||
                 reason == "loaded representation checkpoint has no active "
                           "compatible VICReg producer exposure fact") {
        append_deficit(result, "artifact",
                       "representation_producer_exposure_fact", "missing",
                       reason);
      } else if (reason == "checkpoint closure has no "
                           "target-component-local exposure facts") {
        append_deficit(result, "artifact",
                       "target_component_family_exposure_fact", "missing",
                       reason);
      } else if (reason == "MDN readiness requires a loaded compatible "
                           "representation checkpoint") {
        append_deficit(result, "model_state", "representation_checkpoint",
                       "missing", reason);
      } else if (reason.starts_with(
                     "validation/evaluation evidence must load the MDN "
                     "checkpoint from EVALUATED_CHECKPOINT_SOURCE target")) {
        append_deficit(result, "model_state", "input_checkpoint", "missing",
                       reason);
      } else if (reason ==
                 "validation/evaluation evidence must load the "
                 "representation checkpoint used by the evaluated MDN "
                 "checkpoint") {
        append_deficit(result, "model_state", "representation_checkpoint",
                       "missing", reason);
        append_deficit(result, "model_state", "input_checkpoint", "missing",
                       reason);
      } else if (reason.starts_with(
                     "validation/evaluation MDN checkpoint does not match "
                     "EVALUATED_CHECKPOINT_SOURCE target") ||
                 reason.starts_with("validation/evaluation representation "
                                    "checkpoint does not match the evaluated "
                                    "MDN checkpoint lineage")) {
        append_deficit(result, "model_state", "input_checkpoint", "mismatch",
                       reason);
      } else if (reason.starts_with("loaded representation checkpoint closure "
                                    "has unresolved input checkpoint "
                                    "lineage")) {
        append_deficit(result, "closure", "representation_checkpoint_lineage",
                       "incomplete", reason);
      }
    }

    const auto &closure = result.proof_certificate.closure;
    if (closure.checked && !closure.complete) {
      append_deficit(result, "closure", "checkpoint_lineage", "incomplete",
                     "checkpoint closure has unresolved input checkpoint "
                     "lineage");
    }

    const auto &leakage = result.proof_certificate.leakage;
    if (leakage.checked && !leakage.passed) {
      append_deficit(result, "leakage", "protected_split", "overlap",
                     "checkpoint exposure intersects the protected split "
                     "footprint",
                     std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::quiet_NaN(),
                     "source_row_interval", leakage.protected_range);
    }

    for (const auto &reason : result.reasons) {
      if (reason == k_lattice_read_only_validation_model_state_reason) {
        append_deficit(result, "model_state", "output_checkpoint", "forbidden",
                       reason);
        break;
      }
      if (reason == k_lattice_exact_validation_model_state_input_reason) {
        append_deficit(result, "model_state", "input_checkpoint", "mismatch",
                       reason);
        break;
      }
      if (reason == k_lattice_non_mutating_validation_model_state_reason) {
        append_deficit(result, "model_state", "mutated_component", "forbidden",
                       reason);
        break;
      }
    }

    if (result.deficits.empty()) {
      if (result.reasons.empty()) {
        append_deficit(result, "status", "target", "unsatisfied",
                       "target is not satisfied");
      } else {
        for (const auto &reason : result.reasons) {
          append_deficit(result, "status", "target",
                         lattice_target_status_name(result.status), reason);
        }
      }
    }

    return result;
  }

  [[nodiscard]] static bool
  has_cyclic_dependency_deficit(const lattice_target_evaluation_t &evaluation) {
    return std::find(evaluation.reasons.begin(), evaluation.reasons.end(),
                     "cyclic lattice target dependency") !=
               evaluation.reasons.end() ||
           std::any_of(evaluation.deficits.begin(), evaluation.deficits.end(),
                       [](const auto &deficit) {
                         return deficit.key == "dependency:target_cycle";
                       });
  }

  template <typename Fact>
  [[nodiscard]] bool artifact_fact_matches_identity(
      const lattice_target_spec_t &spec, const Fact &fact,
      const std::string &expected_split_policy_fingerprint) const {
    namespace detail = lattice_target_eval_detail;
    if (spec.require_contract_match &&
        fact.contract_fingerprint !=
            options_.active_identity.protocol_contract_fingerprint) {
      return false;
    }
    if (!options_.active_identity.protocol_id.empty() &&
        fact.protocol_id != options_.active_identity.protocol_id) {
      return false;
    }
    if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
      return false;
    }
    if (detail::target_requires_graph_anchor_identity(spec)) {
      if (fact.graph_order_fingerprint !=
          options_.active_identity.graph_order_fingerprint) {
        return false;
      }
      if (fact.source_cursor_token !=
          options_.active_identity.source_cursor_token) {
        return false;
      }
    }
    if (!spec.component.empty() &&
        fact.target_component_family_id != spec.component) {
      return false;
    }
    if (!spec.train_split.empty() &&
        certificate_names_concrete_split(fact.split_name) &&
        fact.split_name != spec.train_split) {
      return false;
    }
    if (!expected_split_policy_fingerprint.empty()) {
      if (fact.split_policy_fingerprint.empty()) {
        if (certificate_names_concrete_split(fact.split_name)) {
          return false;
        }
      } else if (fact.split_policy_fingerprint !=
                 expected_split_policy_fingerprint) {
        return false;
      }
    }
    const auto target_range = detail::target_anchor_interval(spec);
    if (!target_range.empty()) {
      if (fact.anchor_range.empty()) {
        return false;
      }
      if (fact.anchor_range.begin > target_range.begin ||
          fact.anchor_range.end < target_range.end) {
        return false;
      }
    }
    return true;
  }

  template <typename Fact>
  [[nodiscard]] lattice_target_evidence_t
  artifact_evidence_from_fact(const Fact &fact, std::int64_t family_fact_count,
                              std::int64_t identity_fact_count) const {
    lattice_target_evidence_t out{};
    out.protocol_contract_fingerprint = fact.contract_fingerprint;
    out.protocol_id = fact.protocol_id;
    out.component_fingerprint = fact.component_assembly_fingerprint;
    out.graph_order_fingerprint = fact.graph_order_fingerprint;
    out.source_cursor_token = fact.source_cursor_token;
    out.component_family_id = fact.target_component_family_id;
    out.component_spawn_fingerprint = fact.component_assembly_fingerprint;
    out.status = fact.job_status.empty() ? "completed" : fact.job_status;
    out.matching_job_count = family_fact_count;
    out.matching_train_attempt_count = identity_fact_count;
    out.checkpoint_written = false;
    return out;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_readiness_authority(const Fact &fact) {
    if constexpr (requires { fact.readiness_authority; }) {
      return fact.readiness_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_quality_authority(const Fact &fact) {
    if constexpr (requires { fact.quality_authority; }) {
      return fact.quality_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_performance_authority(const Fact &fact) {
    if constexpr (requires { fact.performance_authority; }) {
      return fact.performance_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_checkpoint_selector(const Fact &fact) {
    if constexpr (requires { fact.checkpoint_selector; }) {
      return fact.checkpoint_selector;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_coverage_authority(const Fact &fact) {
    if constexpr (requires { fact.coverage_authority; }) {
      return fact.coverage_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_leakage_authority(const Fact &fact) {
    if constexpr (requires { fact.leakage_authority; }) {
      return fact.leakage_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_contract_identity_authority(const Fact &fact) {
    if constexpr (requires { fact.contract_identity_authority; }) {
      return fact.contract_identity_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_allocation_authority(const Fact &fact) {
    if constexpr (requires { fact.allocation_authority; }) {
      return fact.allocation_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_execution_authority(const Fact &fact) {
    if constexpr (requires { fact.execution_authority; }) {
      return fact.execution_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_market_readiness_authority(const Fact &fact) {
    if constexpr (requires { fact.market_readiness_authority; }) {
      return fact.market_readiness_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_deployment_authority(const Fact &fact) {
    if constexpr (requires { fact.deployment_authority; }) {
      return fact.deployment_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_target_dependency_authority(const Fact &fact) {
    if constexpr (requires { fact.target_dependency_authority; }) {
      return fact.target_dependency_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_runtime_wave_authority(const Fact &fact) {
    if constexpr (requires { fact.runtime_wave_authority; }) {
      return fact.runtime_wave_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_marshal_reachability(const Fact &fact) {
    if constexpr (requires { fact.marshal_reachability; }) {
      return fact.marshal_reachability;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_checkpoint_source_authority(const Fact &fact) {
    if constexpr (requires { fact.checkpoint_source_authority; }) {
      return fact.checkpoint_source_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool
  fact_plan_checkpoint_input_authority(const Fact &fact) {
    if constexpr (requires { fact.plan_checkpoint_input_authority; }) {
      return fact.plan_checkpoint_input_authority;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_model_state_mutation(const Fact &fact) {
    if constexpr (requires { fact.model_state_mutation; }) {
      return fact.model_state_mutation;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool
  fact_raw_potential_tradable_return(const Fact &fact) {
    if constexpr (requires { fact.raw_potential_tradable_return; }) {
      return fact.raw_potential_tradable_return;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static bool fact_replay_executor(const Fact &fact) {
    if constexpr (requires { fact.replay_executor; }) {
      return fact.replay_executor;
    }
    return false;
  }

  template <typename Fact>
  [[nodiscard]] static std::string fact_schema(const Fact &fact) {
    if constexpr (requires { fact.schema; }) {
      return fact.schema;
    }
    return {};
  }

  template <typename Fact>
  [[nodiscard]] static std::string fact_type(const Fact &fact) {
    if constexpr (requires { fact.fact_type; }) {
      return fact.fact_type;
    }
    return {};
  }

  template <typename Fact>
  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_common_artifact_proof(const lattice_target_spec_t &spec,
                             const Fact &fact, const std::string &fact_family,
                             const std::string &fact_digest,
                             bool identity_match, bool artifact_evidence,
                             bool deterministic_artifact, bool visibility_only,
                             bool lineage_bound,
                             std::vector<std::string> issues) const {
    lattice_target_proof_certificate_t::artifact_proof_t proof{};
    proof.proof_kind = spec.proof_kind;
    proof.fact_family = fact_family;
    proof.fact_schema = fact_schema(fact);
    proof.fact_type = fact_type(fact);
    proof.fact_identity_contract_schema =
        k_lattice_fact_identity_contract_schema_v1;
    proof.fact_identity_contract_id = k_lattice_fact_identity_contract_id_v1;
    if (const auto *proof_template =
            artifact_readiness_proof_template_for_subject_fact_family(
                fact_family)) {
      proof.proof_template_bound =
          proof.proof_kind == proof_template->proof_kind;
      proof.proof_template_claim = proof_template->proof_claim;
    }
    if (const auto family =
            cuwacunu::kikijyeba::lattice::exposure::parse_lattice_fact_family(
                fact_family)) {
      if (const auto descriptor = cuwacunu::kikijyeba::lattice::exposure::
              lattice_fact_family_descriptor(*family)) {
        proof.fact_identity_contract_bound =
            descriptor->fact_schema == proof.fact_schema;
      }
    }
    proof.fact_digest = fact_digest;
    proof.parent_exposure_fact_digest = fact.parent_exposure_fact_digest;
    proof.job_id = fact.job_id;
    proof.wave_id = fact.wave_id;
    proof.protocol_id = fact.protocol_id;
    proof.contract_fingerprint = fact.contract_fingerprint;
    proof.component = fact.target_component_family_id;
    proof.component_assembly_fingerprint = fact.component_assembly_fingerprint;
    proof.graph_order_fingerprint = fact.graph_order_fingerprint;
    proof.source_cursor_token = fact.source_cursor_token;
    proof.split_policy_fingerprint =
        fact.split_policy_fingerprint.empty()
            ? cuwacunu::kikijyeba::lattice::exposure::exposure_digest_for_text(
                  "implicit_split_policy|" + fact.split_name)
            : fact.split_policy_fingerprint;
    proof.split_name = fact.split_name;
    proof.anchor_range = fact.anchor_range;
    proof.completed_anchor_range = fact.completed_anchor_range;
    proof.row_index_interval_authority = true;
    proof.source_key_window_audit_only = true;
    proof.fact_identity_envelope_complete =
        !proof.fact_schema.empty() && !proof.fact_type.empty() &&
        !proof.fact_digest.empty() &&
        !proof.parent_exposure_fact_digest.empty() && !proof.job_id.empty() &&
        !proof.wave_id.empty() && !proof.protocol_id.empty() &&
        !proof.contract_fingerprint.empty() && !proof.component.empty() &&
        !proof.component_assembly_fingerprint.empty() &&
        !proof.graph_order_fingerprint.empty() &&
        !proof.source_cursor_token.empty() &&
        !proof.split_policy_fingerprint.empty() && !proof.split_name.empty() &&
        !proof.anchor_range.empty() && !proof.completed_anchor_range.empty();
    proof.identity_match = identity_match;
    proof.artifact_evidence = artifact_evidence;
    proof.deterministic_artifact = deterministic_artifact;
    proof.visibility_only = visibility_only;
    proof.readiness_authority = fact_readiness_authority(fact);
    proof.quality_authority = fact_quality_authority(fact);
    proof.performance_authority = fact_performance_authority(fact);
    proof.checkpoint_selector = fact_checkpoint_selector(fact);
    proof.coverage_authority = fact_coverage_authority(fact);
    proof.leakage_authority = fact_leakage_authority(fact);
    proof.contract_identity_authority = fact_contract_identity_authority(fact);
    proof.allocation_authority = fact_allocation_authority(fact);
    proof.execution_authority = fact_execution_authority(fact);
    proof.market_readiness_authority = fact_market_readiness_authority(fact);
    proof.deployment_authority = fact_deployment_authority(fact);
    proof.policy_gate = false;
    proof.target_dependency_authority = fact_target_dependency_authority(fact);
    proof.runtime_wave_authority = fact_runtime_wave_authority(fact);
    proof.marshal_reachability = fact_marshal_reachability(fact);
    proof.checkpoint_source_authority = fact_checkpoint_source_authority(fact);
    proof.plan_checkpoint_input_authority =
        fact_plan_checkpoint_input_authority(fact);
    proof.model_state_mutation = fact_model_state_mutation(fact);
    proof.raw_potential_tradable_return =
        fact_raw_potential_tradable_return(fact);
    proof.replay_executor = fact_replay_executor(fact);
    const bool authority_drift =
        proof.readiness_authority || proof.quality_authority ||
        proof.performance_authority || proof.checkpoint_selector ||
        proof.coverage_authority || proof.leakage_authority ||
        proof.contract_identity_authority || proof.allocation_authority ||
        proof.execution_authority || proof.market_readiness_authority ||
        proof.deployment_authority || proof.policy_gate ||
        proof.target_dependency_authority || proof.runtime_wave_authority ||
        proof.marshal_reachability || proof.checkpoint_source_authority ||
        proof.plan_checkpoint_input_authority || proof.model_state_mutation ||
        proof.raw_potential_tradable_return || proof.replay_executor ||
        proof.fact_identity_target_kind_authority ||
        proof.fact_identity_runtime_wave_authority ||
        proof.fact_identity_marshal_reachability ||
        proof.fact_identity_policy_gate_authority;
    proof.authority_clean = !authority_drift;
    proof.lineage_bound = lineage_bound;
    proof.issues = std::move(issues);
    proof.passed = proof.proof_template_bound &&
                   proof.fact_identity_contract_bound &&
                   proof.fact_identity_envelope_complete &&
                   proof.row_index_interval_authority &&
                   proof.source_key_window_audit_only && proof.identity_match &&
                   proof.artifact_evidence && proof.deterministic_artifact &&
                   proof.visibility_only && proof.authority_clean &&
                   proof.lineage_bound && proof.issues.empty();
    return proof;
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_target_transform_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_target_transform_fact_t &fact,
      bool identity_match) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::target_transform_fact_issues(fact);
    if (fact.support_surface_identity.empty()) {
      issues.emplace_back("missing_support_surface_identity");
    }
    if (fact.support_surface_digest.empty()) {
      issues.emplace_back("missing_support_surface_digest");
    }
    const bool lineage_bound =
        !fact.parent_exposure_fact_digest.empty() &&
        !fact.target_feature_ids.empty() && fact.horizon > 0 &&
        !fact.target_mode.empty() && !fact.normalization_contract.empty() &&
        !fact.inverse_transform_contract.empty() && !fact.units.empty() &&
        !fact.target_mask_policy_digest.empty() &&
        !fact.support_surface_identity.empty() &&
        !fact.support_surface_digest.empty();
    return make_common_artifact_proof(
        spec, fact, "target_transform",
        exposure::target_transform_fact_digest(fact), identity_match,
        fact.artifact_contract_prerequisite,
        fact.artifact_contract_prerequisite, fact.visibility_only,
        lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_forecast_baseline_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_forecast_baseline_fact_t &fact,
      bool identity_match,
      const std::set<std::string> &target_transform_digests) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::forecast_baseline_fact_issues(fact);
    if (!fact.target_transform_fact_digest.empty() &&
        target_transform_digests.find(fact.target_transform_fact_digest) ==
            target_transform_digests.end()) {
      issues.emplace_back("target_transform_fact_digest_not_found");
    }
    const bool lineage_bound =
        !fact.parent_exposure_fact_digest.empty() &&
        !fact.target_feature_ids.empty() && fact.horizon > 0 &&
        !fact.baseline_kind.empty() &&
        !fact.target_transform_fact_digest.empty() &&
        target_transform_digests.find(fact.target_transform_fact_digest) !=
            target_transform_digests.end() &&
        (fact.support_count > 0 || fact.valid_count > 0);
    return make_common_artifact_proof(
        spec, fact, "forecast_baseline",
        exposure::forecast_baseline_fact_digest(fact), identity_match,
        fact.evidence_prerequisite, fact.evidence_prerequisite,
        fact.visibility_only, lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_forecast_eval_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::lattice_forecast_eval_fact_t
          &fact,
      bool identity_match,
      const std::set<std::string> &target_transform_digests,
      const std::set<std::string> &forecast_baseline_digests,
      const std::set<std::string> &selection_signal_digests) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::forecast_eval_fact_issues(fact);
    if (!fact.target_transform_fact_digest.empty() &&
        target_transform_digests.find(fact.target_transform_fact_digest) ==
            target_transform_digests.end()) {
      issues.emplace_back("target_transform_fact_digest_not_found");
    }
    for (const auto &digest : fact.baseline_fact_digests) {
      if (!digest.empty() && forecast_baseline_digests.find(digest) ==
                                 forecast_baseline_digests.end()) {
        issues.emplace_back("baseline_fact_digest_not_found");
        break;
      }
    }
    if (fact.selection_signal_fact_digests.empty()) {
      issues.emplace_back("missing_selection_signal_audit");
    }
    for (const auto &digest : fact.selection_signal_fact_digests) {
      if (!digest.empty() && selection_signal_digests.find(digest) ==
                                 selection_signal_digests.end()) {
        issues.emplace_back("selection_signal_fact_digest_not_found");
        break;
      }
    }
    const bool all_baselines_bound = std::all_of(
        fact.baseline_fact_digests.begin(), fact.baseline_fact_digests.end(),
        [&](const std::string &digest) {
          return !digest.empty() && forecast_baseline_digests.find(digest) !=
                                        forecast_baseline_digests.end();
        });
    const bool all_selection_signals_bound = std::all_of(
        fact.selection_signal_fact_digests.begin(),
        fact.selection_signal_fact_digests.end(),
        [&](const std::string &digest) {
          return !digest.empty() && selection_signal_digests.find(digest) !=
                                        selection_signal_digests.end();
        });
    const bool lineage_bound =
        !fact.parent_exposure_fact_digest.empty() &&
        !fact.target_feature_ids.empty() && fact.horizon > 0 &&
        (fact.support_count > 0 || fact.valid_count > 0) &&
        !fact.forecast_artifact_digest.empty() &&
        !fact.evaluated_representation_checkpoint_digest.empty() &&
        !fact.evaluated_mdn_checkpoint_digest.empty() &&
        !fact.target_transform_fact_digest.empty() &&
        target_transform_digests.find(fact.target_transform_fact_digest) !=
            target_transform_digests.end() &&
        !fact.baseline_fact_digests.empty() && all_baselines_bound &&
        !fact.selection_signal_fact_digests.empty() &&
        all_selection_signals_bound && !fact.support_by_horizon.empty() &&
        !fact.valid_target_count_per_horizon.empty() &&
        !fact.model_state_mutation;
    return make_common_artifact_proof(
        spec, fact, "forecast_eval", exposure::forecast_eval_fact_digest(fact),
        identity_match, fact.artifact_evidence,
        fact.artifact_evidence && !fact.model_state_mutation,
        fact.visibility_only, lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_observer_belief_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_observer_belief_fact_t &fact,
      bool identity_match,
      const std::unordered_map<std::string, std::string>
          &forecast_artifact_by_eval_digest) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::observer_belief_fact_issues(fact);
    if (fact.forecast_artifact_lineage.empty()) {
      issues.emplace_back("missing_forecast_artifact_lineage");
    }
    bool forecast_lineage_found = false;
    bool forecast_artifact_matches_lineage = false;
    if (!fact.forecast_artifact_lineage.empty()) {
      const auto it =
          forecast_artifact_by_eval_digest.find(fact.forecast_artifact_lineage);
      forecast_lineage_found = it != forecast_artifact_by_eval_digest.end();
      if (!forecast_lineage_found) {
        issues.emplace_back("forecast_artifact_lineage_not_found");
      } else {
        forecast_artifact_matches_lineage =
            !fact.forecast_artifact_digest.empty() &&
            it->second == fact.forecast_artifact_digest;
        if (!forecast_artifact_matches_lineage) {
          issues.emplace_back("forecast_artifact_digest_mismatch");
        }
      }
    }
    const bool lineage_bound = !fact.parent_exposure_fact_digest.empty() &&
                               !fact.forecast_artifact_digest.empty() &&
                               !fact.forecast_artifact_lineage.empty() &&
                               forecast_lineage_found &&
                               forecast_artifact_matches_lineage &&
                               !fact.scenario_bank_digest.empty() &&
                               !fact.feature_semantics_fingerprint.empty() &&
                               !fact.dock_binding_fingerprint.empty();
    return make_common_artifact_proof(
        spec, fact, "observer_belief",
        exposure::observer_belief_fact_digest(fact), identity_match,
        fact.artifact_evidence, fact.artifact_evidence, fact.visibility_only,
        lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_allocation_engine_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_allocation_engine_fact_t &fact,
      bool identity_match, const std::set<std::string> &observer_belief_digests,
      const std::unordered_map<std::string, std::string>
          &forecast_artifact_by_observer_digest,
      const std::set<std::string> &forecast_artifact_digests) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::allocation_engine_fact_issues(fact);
    bool observer_digest_found = false;
    bool observer_forecast_artifact_matches = false;
    if (!fact.observer_belief_fact_digest.empty() &&
        observer_belief_digests.find(fact.observer_belief_fact_digest) ==
            observer_belief_digests.end()) {
      issues.emplace_back("observer_belief_fact_digest_not_found");
    }
    if (!fact.observer_belief_fact_digest.empty()) {
      const auto it = forecast_artifact_by_observer_digest.find(
          fact.observer_belief_fact_digest);
      observer_digest_found = it != forecast_artifact_by_observer_digest.end();
      observer_forecast_artifact_matches =
          observer_digest_found && !fact.forecast_artifact_digest.empty() &&
          it->second == fact.forecast_artifact_digest;
      if (observer_digest_found && !observer_forecast_artifact_matches) {
        issues.emplace_back("observer_forecast_artifact_digest_mismatch");
      }
    }
    const bool forecast_artifact_found =
        !fact.forecast_artifact_digest.empty() &&
        forecast_artifact_digests.find(fact.forecast_artifact_digest) !=
            forecast_artifact_digests.end();
    if (!fact.forecast_artifact_digest.empty() && !forecast_artifact_found) {
      issues.emplace_back("forecast_artifact_digest_not_found");
    }
    const bool reserve_node_from_base_policy =
        fact.reserve_node_source == "base_policy";
    const bool reserve_node_matches_base_policy =
        !fact.reserve_node_id.empty() &&
        !fact.base_policy_reserve_node_id.empty() &&
        fact.reserve_node_id == fact.base_policy_reserve_node_id;
    const bool reason_contracts_declared =
        !fact.fallback_reasons.empty() && !fact.derisk_reasons.empty();
    const bool lineage_bound =
        !fact.parent_exposure_fact_digest.empty() &&
        !fact.observer_belief_fact_digest.empty() &&
        observer_belief_digests.find(fact.observer_belief_fact_digest) !=
            observer_belief_digests.end() &&
        !fact.forecast_artifact_digest.empty() && observer_digest_found &&
        observer_forecast_artifact_matches && forecast_artifact_found &&
        !fact.base_policy_digest.empty() && reserve_node_from_base_policy &&
        reserve_node_matches_base_policy && fact.reserve_node_graph_bound &&
        reason_contracts_declared;
    return make_common_artifact_proof(
        spec, fact, "allocation_engine",
        exposure::allocation_engine_fact_digest(fact), identity_match,
        fact.deterministic_artifact, fact.deterministic_artifact,
        fact.visibility_only, lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_replay_environment_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_replay_environment_fact_t &fact,
      bool identity_match) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::replay_environment_fact_issues(fact);
    const bool trace_count_bound =
        fact.cajtucu_valid_trace_count + fact.cajtucu_invalid_trace_count ==
            fact.time_law_execution_step_count &&
        fact.cajtucu_valid_trace_count > 0 &&
        fact.cajtucu_invalid_trace_count == 0;
    const bool lineage_bound =
        !fact.parent_exposure_fact_digest.empty() &&
        !fact.batch_index_path.empty() && !fact.experiment_index_path.empty() &&
        !fact.experiment_report_path.empty() &&
        !fact.experiment_index_report_digest.empty() &&
        !fact.experiment_report_digest.empty() &&
        fact.experiment_index_report_digest == fact.experiment_report_digest &&
        !fact.experiment_id.empty() && !fact.runtime_run_id.empty() &&
        !fact.environment_run_id.empty() &&
        !fact.execution_profile_digest.empty() &&
        !fact.policy_set_digest.empty() && fact.policy_count > 0 &&
        fact.policy_summary_count > 0 &&
        fact.policy_summary_count == fact.policy_count &&
        fact.attempted_count > 0 && fact.failed_count == 0 &&
        fact.completed_count == fact.attempted_count &&
        fact.episode_count > 0 &&
        fact.episode_requested_range_bound_count >= fact.episode_count &&
        fact.episode_cursor_bound_count >= fact.episode_count &&
        fact.episode_anchor_interval_bound_count >= fact.episode_count &&
        fact.episode_anchor_keys_bound_count >= fact.episode_count &&
        fact.projection_validation_step_count > 0 && trace_count_bound &&
        fact.cajtucu_synthetic_market_step_count == 0 &&
        std::isfinite(fact.mean_total_transaction_cost_base) &&
        fact.mean_total_transaction_cost_base > 0.0 &&
        std::isfinite(fact.fill_ratio) && fact.fill_ratio >= 0.0 &&
        fact.fill_ratio <= 1.0;
    return make_common_artifact_proof(
        spec, fact, "replay_environment",
        exposure::replay_environment_fact_digest(fact), identity_match,
        fact.artifact_evidence, fact.artifact_evidence, fact.visibility_only,
        lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_proof_certificate_t::artifact_proof_t
  make_policy_training_artifact_proof(
      const lattice_target_spec_t &spec,
      const cuwacunu::kikijyeba::lattice::exposure::
          lattice_policy_training_fact_t &fact,
      bool identity_match, const std::set<std::string> &forecast_eval_digests,
      const std::set<std::string> &observer_belief_digests,
      const std::set<std::string> &allocation_engine_digests,
      const std::set<std::string> &replay_environment_digests) const {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    auto issues = exposure::policy_training_fact_issues(fact);
    const auto digest_missing = [&](const std::string &digest,
                                    const std::set<std::string> &known) {
      return digest.empty() || known.find(digest) == known.end();
    };
    if (digest_missing(fact.parent_forecast_eval_fact_digest,
                       forecast_eval_digests)) {
      issues.emplace_back("parent_forecast_eval_fact_digest_not_found");
    }
    if (digest_missing(fact.parent_observer_belief_fact_digest,
                       observer_belief_digests)) {
      issues.emplace_back("parent_observer_belief_fact_digest_not_found");
    }
    if (digest_missing(fact.parent_allocation_engine_fact_digest,
                       allocation_engine_digests)) {
      issues.emplace_back("parent_allocation_engine_fact_digest_not_found");
    }
    if (digest_missing(fact.parent_replay_environment_fact_digest,
                       replay_environment_digests)) {
      issues.emplace_back("parent_replay_environment_fact_digest_not_found");
    }
    const bool parent_lineage_bound =
        !fact.parent_forecast_eval_fact_digest.empty() &&
        forecast_eval_digests.find(fact.parent_forecast_eval_fact_digest) !=
            forecast_eval_digests.end() &&
        !fact.parent_observer_belief_fact_digest.empty() &&
        observer_belief_digests.find(fact.parent_observer_belief_fact_digest) !=
            observer_belief_digests.end() &&
        !fact.parent_allocation_engine_fact_digest.empty() &&
        allocation_engine_digests.find(
            fact.parent_allocation_engine_fact_digest) !=
            allocation_engine_digests.end() &&
        !fact.parent_replay_environment_fact_digest.empty() &&
        replay_environment_digests.find(
            fact.parent_replay_environment_fact_digest) !=
            replay_environment_digests.end();
    const bool anti_leakage_bound =
        fact.training_range_disjoint_validation &&
        fact.training_range_disjoint_test &&
        fact.validation_range_disjoint_test &&
        fact.normalization_fit_training_only &&
        fact.replay_buffer_training_only &&
        fact.reward_baseline_training_only &&
        fact.training_schedule_mode == "causal_walk_forward_training.v1" &&
        fact.causal_schedule_readiness_eligible &&
        fact.causal_schedule_no_future_snapshot_use &&
        fact.causal_schedule_no_future_snapshot_use_source ==
            "derived_from_artifact_fit_use_ledgers" &&
        !fact.offline_full_window_research &&
        fact.early_stopping_uses_validation_only &&
        fact.hyperparameter_selection_uses_validation_only &&
        fact.test_sealed_until_final_report &&
        fact.test_first_access_after_selection;
    const bool contract_bound =
        !fact.policy_id.empty() && !fact.policy_kind.empty() &&
        !fact.policy_architecture_digest.empty() &&
        !fact.training_config_digest.empty() &&
        !fact.training_range_digest.empty() &&
        !fact.validation_range_digest.empty() &&
        !fact.test_range_digest.empty() &&
        !fact.environment_contract_id.empty() &&
        !fact.observation_schema_digest.empty() &&
        !fact.action_schema_digest.empty() &&
        !fact.reward_contract_digest.empty() &&
        !fact.execution_profile_digest.empty() &&
        !fact.training_schedule_mode.empty() &&
        !fact.causal_schedule_schema_id.empty() &&
        !fact.causal_schedule_digest.empty() &&
        !fact.causal_schedule_cursor_key_kind.empty() &&
        !fact.causal_schedule_no_future_snapshot_use_source.empty() &&
        !fact.normalization_fit_range_digest.empty() &&
        !fact.replay_buffer_source_range_digest.empty() &&
        !fact.early_stopping_policy_digest.empty() &&
        !fact.hyperparameter_selection_policy_digest.empty() &&
        !fact.selector_split.empty() && !fact.selector_policy_digest.empty() &&
        fact.random_seed_bound && !fact.parent_checkpoint_digest.empty() &&
        !fact.checkpoint_digest.empty() && fact.runtime_job_kind_bound &&
        fact.policy_checkpoint_written;
    const bool lineage_bound = !fact.parent_exposure_fact_digest.empty() &&
                               contract_bound && parent_lineage_bound &&
                               anti_leakage_bound;
    return make_common_artifact_proof(
        spec, fact, "policy_training",
        exposure::policy_training_fact_digest(fact), identity_match,
        fact.artifact_evidence, fact.artifact_evidence, fact.visibility_only,
        lineage_bound, std::move(issues));
  }

  [[nodiscard]] lattice_target_evaluation_t evaluate_artifact_readiness_spec(
      const lattice_target_spec_t &spec, lattice_target_evaluation_t result,
      std::vector<lattice_target_proof_certificate_t::dependency_proof_t>
          dependency_proofs) const {
    namespace detail = lattice_target_eval_detail;
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;

    result.proof_certificate.dependencies = std::move(dependency_proofs);
    result.plan_ready = false;
    result.suggested_wave = {};

    try {
      validate_artifact_readiness_proof_template(
          spec.subject_fact_family, spec.proof_kind, spec.target_id);
    } catch (const std::exception &ex) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(ex.what());
      return result;
    }

    if (detail::target_requires_graph_anchor_identity(spec)) {
      if (options_.active_identity.graph_order_fingerprint.empty()) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "active graph order fingerprint is required for artifact "
            "readiness targets but was not provided");
        return result;
      }
      if (options_.active_identity.source_cursor_token.empty()) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "active source cursor token is required for artifact readiness "
            "targets but was not provided");
        return result;
      }
    }

    const auto expected_split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    std::optional<exposure::exposure_ledger_scan_result_t> scanned_ledger{};
    const exposure::lattice_exposure_ledger_t *ledger =
        options_.exposure_ledger;
    if (ledger == nullptr && options_.auto_build_exposure_ledger) {
      exposure::exposure_build_context_t context{};
      context.split_policy_fingerprint = expected_split_policy_fingerprint;
      exposure::exposure_scan_options_t scan_options{};
      if (spec.subject_fact_family == "replay_environment") {
        scan_options.derive_replay_environment_facts = true;
      }
      scanned_ledger = exposure::scan_exposure_ledger_from_runtime_root(
          options_.runtime_root, context, scan_options);
      ledger = &scanned_ledger->ledger;
    }
    if (ledger == nullptr) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "artifact readiness target requires an exposure ledger but none was "
          "provided");
      return result;
    }
    try {
      const auto subject_family =
          exposure::parse_lattice_fact_family(spec.subject_fact_family);
      if (subject_family.has_value()) {
        result.fact_integrity_summary =
            exposure::summarize_lattice_fact_integrity(
                *ledger,
                std::vector<exposure::lattice_fact_family_t>{*subject_family});
        result.fact_integrity_summary_available = true;
      }
    } catch (const std::exception &) {
      result.fact_integrity_summary_available = false;
    }

    std::int64_t family_fact_count = 0;
    std::int64_t identity_fact_count = 0;
    std::optional<lattice_target_proof_certificate_t::artifact_proof_t>
        selected_proof{};
    std::optional<lattice_target_proof_certificate_t::artifact_proof_t>
        first_identity_proof{};
    lattice_target_evidence_t selected_evidence{};
    lattice_target_evidence_t first_identity_evidence{};
    std::vector<std::string> first_identity_issues{};
    std::set<std::string> target_transform_digests{};
    for (const auto &fact : ledger->target_transform_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        target_transform_digests.insert(
            exposure::target_transform_fact_digest(fact));
      }
    }
    std::set<std::string> forecast_baseline_digests{};
    for (const auto &fact : ledger->forecast_baseline_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        forecast_baseline_digests.insert(
            exposure::forecast_baseline_fact_digest(fact));
      }
    }
    std::set<std::string> selection_signal_digests{};
    for (const auto &fact : ledger->selection_signal_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        selection_signal_digests.insert(
            exposure::selection_signal_fact_digest(fact));
      }
    }
    std::unordered_map<std::string, std::string>
        forecast_artifact_by_eval_digest{};
    std::set<std::string> forecast_eval_digests{};
    std::set<std::string> forecast_artifact_digests{};
    for (const auto &fact : ledger->forecast_eval_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        const auto digest = exposure::forecast_eval_fact_digest(fact);
        forecast_eval_digests.insert(digest);
        forecast_artifact_by_eval_digest[digest] =
            fact.forecast_artifact_digest;
        if (!fact.forecast_artifact_digest.empty()) {
          forecast_artifact_digests.insert(fact.forecast_artifact_digest);
        }
      }
    }
    std::set<std::string> observer_belief_digests{};
    std::unordered_map<std::string, std::string>
        forecast_artifact_by_observer_digest{};
    for (const auto &fact : ledger->observer_belief_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        const auto digest = exposure::observer_belief_fact_digest(fact);
        observer_belief_digests.insert(digest);
        forecast_artifact_by_observer_digest[digest] =
            fact.forecast_artifact_digest;
      }
    }
    std::set<std::string> allocation_engine_digests{};
    for (const auto &fact : ledger->allocation_engine_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        allocation_engine_digests.insert(
            exposure::allocation_engine_fact_digest(fact));
      }
    }
    std::set<std::string> replay_environment_digests{};
    for (const auto &fact : ledger->replay_environment_facts()) {
      if (artifact_fact_matches_identity(spec, fact,
                                         expected_split_policy_fingerprint)) {
        replay_environment_digests.insert(
            exposure::replay_environment_fact_digest(fact));
      }
    }
    const auto remember_identity_proof = [&](const auto &fact,
                                             const auto &proof) {
      ++identity_fact_count;
      if (!first_identity_proof.has_value()) {
        first_identity_proof = proof;
        first_identity_evidence = artifact_evidence_from_fact(
            fact, family_fact_count, identity_fact_count);
      }
      if (first_identity_issues.empty() && !proof.issues.empty()) {
        first_identity_issues = proof.issues;
      }
    };

    if (spec.subject_fact_family == "target_transform") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->target_transform_facts().size());
      for (const auto &fact : ledger->target_transform_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof =
            make_target_transform_artifact_proof(spec, fact, identity_match);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "forecast_baseline") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->forecast_baseline_facts().size());
      for (const auto &fact : ledger->forecast_baseline_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof = make_forecast_baseline_artifact_proof(
            spec, fact, identity_match, target_transform_digests);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "forecast_eval") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->forecast_eval_facts().size());
      for (const auto &fact : ledger->forecast_eval_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof = make_forecast_eval_artifact_proof(
            spec, fact, identity_match, target_transform_digests,
            forecast_baseline_digests, selection_signal_digests);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "observer_belief") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->observer_belief_facts().size());
      for (const auto &fact : ledger->observer_belief_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof = make_observer_belief_artifact_proof(
            spec, fact, identity_match, forecast_artifact_by_eval_digest);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "allocation_engine") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->allocation_engine_facts().size());
      for (const auto &fact : ledger->allocation_engine_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof = make_allocation_engine_artifact_proof(
            spec, fact, identity_match, observer_belief_digests,
            forecast_artifact_by_observer_digest, forecast_artifact_digests);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "replay_environment") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->replay_environment_facts().size());
      for (const auto &fact : ledger->replay_environment_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof =
            make_replay_environment_artifact_proof(spec, fact, identity_match);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else if (spec.subject_fact_family == "policy_training") {
      family_fact_count =
          static_cast<std::int64_t>(ledger->policy_training_facts().size());
      for (const auto &fact : ledger->policy_training_facts()) {
        const bool identity_match = artifact_fact_matches_identity(
            spec, fact, expected_split_policy_fingerprint);
        auto proof = make_policy_training_artifact_proof(
            spec, fact, identity_match, forecast_eval_digests,
            observer_belief_digests, allocation_engine_digests,
            replay_environment_digests);
        if (identity_match) {
          remember_identity_proof(fact, proof);
        }
        if (proof.passed) {
          selected_evidence = artifact_evidence_from_fact(
              fact, family_fact_count, identity_fact_count);
          selected_proof = std::move(proof);
          break;
        }
      }
    } else {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "unsupported artifact readiness SUBJECT_FACT_FAMILY: " +
          spec.subject_fact_family);
      return result;
    }

    result.evidence.matching_job_count = family_fact_count;
    result.evidence.matching_train_attempt_count = identity_fact_count;
    if (family_fact_count == 0) {
      result.status = lattice_target_status_t::missing_report;
      result.reasons.push_back("no " + spec.subject_fact_family +
                               " artifact facts found under " +
                               options_.runtime_root.string());
      return result;
    }
    if (identity_fact_count == 0) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back("no " + spec.subject_fact_family +
                               " artifact facts matched the target identity");
      return result;
    }
    if (!selected_proof.has_value()) {
      result.status = lattice_target_status_t::blocked;
      if (first_identity_proof.has_value()) {
        result.evidence = first_identity_evidence;
        result.proof_certificate.artifacts.push_back(*first_identity_proof);
        result.proof_certificate.proof_context = detail::make_proof_context(
            spec, options_.active_identity, result.evidence,
            result.evidence.component_fingerprint,
            result.split_policy_fingerprint);
      }
      std::ostringstream reason;
      reason << "no clean " << spec.subject_fact_family
             << " artifact proof matched " << spec.proof_kind;
      if (!first_identity_issues.empty()) {
        reason << "; first issues=";
        for (std::size_t i = 0; i < first_identity_issues.size(); ++i) {
          if (i != 0) {
            reason << ",";
          }
          reason << first_identity_issues[i];
        }
      }
      result.reasons.push_back(reason.str());
      return result;
    }

    result.evidence = std::move(selected_evidence);
    result.proof_certificate.artifacts.push_back(*selected_proof);
    result.proof_certificate.proof_context = detail::make_proof_context(
        spec, options_.active_identity, result.evidence,
        result.evidence.component_fingerprint, result.split_policy_fingerprint);
    if (!spec.warning_specs.empty()) {
      std::vector<exposure::lattice_exposure_fact_t> exposure_facts =
          ledger->facts();
      std::vector<exposure::lattice_exposure_fact_t>
          target_component_family_facts;
      for (const auto &fact : exposure_facts) {
        if (fact.target_component_family_id == spec.component) {
          target_component_family_facts.push_back(fact);
        }
      }
      std::vector<exposure::lattice_node_exposure_fact_t> node_facts;
      std::vector<exposure::lattice_source_analytics_fact_t>
          source_analytics_facts;
      for (const auto &fact : ledger->source_analytics_facts()) {
        if (artifact_fact_matches_identity(spec, fact,
                                           expected_split_policy_fingerprint)) {
          source_analytics_facts.push_back(fact);
        }
      }
      std::vector<exposure::lattice_forecast_baseline_fact_t>
          forecast_baseline_facts;
      for (const auto &fact : ledger->forecast_baseline_facts()) {
        if (artifact_fact_matches_identity(spec, fact,
                                           expected_split_policy_fingerprint)) {
          forecast_baseline_facts.push_back(fact);
        }
      }
      std::vector<exposure::lattice_forecast_eval_fact_t> forecast_eval_facts;
      for (const auto &fact : ledger->forecast_eval_facts()) {
        if (artifact_fact_matches_identity(spec, fact,
                                           expected_split_policy_fingerprint)) {
          forecast_eval_facts.push_back(fact);
        }
      }
      std::vector<exposure::lattice_observer_belief_fact_t>
          observer_belief_facts;
      for (const auto &fact : ledger->observer_belief_facts()) {
        if (artifact_fact_matches_identity(spec, fact,
                                           expected_split_policy_fingerprint)) {
          observer_belief_facts.push_back(fact);
        }
      }
      std::vector<exposure::lattice_allocation_engine_fact_t>
          allocation_engine_facts;
      for (const auto &fact : ledger->allocation_engine_facts()) {
        if (artifact_fact_matches_identity(spec, fact,
                                           expected_split_policy_fingerprint)) {
          allocation_engine_facts.push_back(fact);
        }
      }
      evaluate_warning_specs(
          spec, result, exposure_facts, target_component_family_facts,
          node_facts, source_analytics_facts, forecast_baseline_facts,
          forecast_eval_facts, observer_belief_facts, allocation_engine_facts,
          result.node_support_summaries);
    }
    result.status = lattice_target_status_t::satisfied;
    return result;
  }

  [[nodiscard]] lattice_target_evaluation_t
  evaluate_spec(const lattice_target_spec_t &input_spec,
                std::vector<std::string> dependency_stack) const {
    namespace detail = lattice_target_eval_detail;
    try {
      validate_lattice_target_spec_lowered(input_spec);
    } catch (const std::exception &ex) {
      lattice_target_evaluation_t result{};
      result.target_id = input_spec.target_id;
      result.kind = input_spec.kind;
      result.target_kind_applicable =
          lattice_target_kind_applicable(input_spec);
      result.target_class = input_spec.target_class;
      result.proof_kind = input_spec.proof_kind;
      result.subject_fact_family = input_spec.subject_fact_family;
      result.component = input_spec.component;
      result.split_policy_fingerprint =
          detail::active_split_policy_fingerprint(options_.split_policy);
      bind_proof_obligation(result, input_spec);
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(ex.what());
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }

    lattice_target_spec_t spec{};
    try {
      spec =
          detail::resolve_split_references(input_spec, options_.split_policy);
    } catch (const std::exception &ex) {
      lattice_target_evaluation_t result{};
      result.target_id = input_spec.target_id;
      result.kind = input_spec.kind;
      result.target_kind_applicable =
          lattice_target_kind_applicable(input_spec);
      result.target_class = input_spec.target_class;
      result.proof_kind = input_spec.proof_kind;
      result.subject_fact_family = input_spec.subject_fact_family;
      result.component = input_spec.component;
      result.split_policy_fingerprint =
          detail::active_split_policy_fingerprint(options_.split_policy);
      bind_proof_obligation(result, input_spec);
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(ex.what());
      result.plan_ready = false;
      result.suggested_wave = {};
      return result;
    }
    lattice_target_evaluation_t result{};
    result.target_id = spec.target_id;
    result.kind = spec.kind;
    result.target_kind_applicable = lattice_target_kind_applicable(spec);
    result.target_class = spec.target_class;
    result.proof_kind = spec.proof_kind;
    result.subject_fact_family = spec.subject_fact_family;
    result.component = spec.component;
    result.split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    bind_proof_obligation(result, spec);
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

    std::vector<lattice_target_proof_certificate_t::dependency_proof_t>
        dependency_proofs{};
    if (!spec.upstream_target_id.empty()) {
      const auto *upstream = find_target(spec.upstream_target_id);
      if (upstream == nullptr) {
        result.proof_certificate.dependencies.push_back(
            lattice_target_proof_certificate_t::dependency_proof_t{
                .kind = "upstream_target",
                .source_target_id = spec.upstream_target_id,
                .status = "missing"});
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back("missing upstream target: " +
                                 spec.upstream_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      auto upstream_eval =
          finalize_evaluation(evaluate_spec(*upstream, dependency_stack));
      lattice_target_proof_certificate_t::dependency_proof_t upstream_proof{};
      upstream_proof.kind = "upstream_target";
      upstream_proof.source_target_id = spec.upstream_target_id;
      upstream_proof.status = lattice_target_status_name(upstream_eval.status);
      if (upstream_eval.status != lattice_target_status_t::satisfied) {
        result.proof_certificate.dependencies.push_back(upstream_proof);
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "upstream target is not satisfied: " + spec.upstream_target_id +
            " status=" + lattice_target_status_name(upstream_eval.status));
        if (has_cyclic_dependency_deficit(upstream_eval)) {
          result.reasons.push_back("cyclic lattice target dependency");
        }
        result.plan_ready = upstream_eval.plan_ready;
        result.suggested_wave = upstream_eval.suggested_wave;
        return result;
      }
      dependency_proofs.push_back(std::move(upstream_proof));
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

    if (!spec.protocol_id.empty()) {
      if (options_.active_identity.protocol_id.empty()) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "active protocol id is required but was not provided");
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      if (options_.active_identity.protocol_id != spec.protocol_id) {
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back("target protocol id does not match active "
                                 "protocol id");
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
    }

    if (is_artifact_readiness_target_class(spec.target_class)) {
      return evaluate_artifact_readiness_spec(spec, std::move(result),
                                              std::move(dependency_proofs));
    }

    if (reject_non_checkpoint_latest_satisfying_reference(
            spec, "PLAN_INPUT_MDN_CHECKPOINT", spec.plan_input_mdn_checkpoint,
            result) ||
        reject_non_checkpoint_latest_satisfying_reference(
            spec, "PLAN_INPUT_REPRESENTATION_CHECKPOINT",
            spec.plan_input_representation_checkpoint, result)) {
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
        result.proof_certificate.dependencies.push_back(
            lattice_target_proof_certificate_t::dependency_proof_t{
                .kind = "checkpoint_source",
                .source_target_id = checkpoint_source_target_id,
                .status = "missing"});
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back("missing CHECKPOINT_SOURCE target: " +
                                 checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      if (reject_non_checkpoint_latest_satisfying_reference(
              spec, "CHECKPOINT_SOURCE", spec.checkpoint_source, result,
              "checkpoint_source")) {
        return result;
      }
      auto source_eval = finalize_evaluation(
          evaluate_spec(*checkpoint_source_target, dependency_stack));
      lattice_target_proof_certificate_t::dependency_proof_t source_proof{};
      source_proof.kind = "checkpoint_source";
      source_proof.source_target_id = checkpoint_source_target_id;
      source_proof.status = lattice_target_status_name(source_eval.status);
      source_proof.expected_mdn_checkpoint =
          source_eval.evidence.checkpoint_path;
      source_proof.actual_mdn_checkpoint = source_eval.evidence.checkpoint_path;
      source_proof.mdn_checkpoint_match =
          !source_eval.evidence.checkpoint_path.empty();
      source_proof.expected_representation_checkpoint =
          source_eval.evidence.representation_checkpoint_path;
      source_proof.actual_representation_checkpoint =
          source_eval.evidence.representation_checkpoint_path;
      source_proof.representation_checkpoint_match =
          !source_eval.evidence.representation_checkpoint_path.empty();
      if (source_eval.status != lattice_target_status_t::satisfied) {
        result.proof_certificate.dependencies.push_back(source_proof);
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "CHECKPOINT_SOURCE target is not satisfied: " +
            checkpoint_source_target_id +
            " status=" + lattice_target_status_name(source_eval.status));
        if (has_cyclic_dependency_deficit(source_eval)) {
          result.reasons.push_back("cyclic lattice target dependency");
        }
        result.plan_ready = source_eval.plan_ready;
        result.suggested_wave = source_eval.suggested_wave;
        return result;
      }
      if (source_eval.evidence.checkpoint_path.empty() ||
          !std::filesystem::exists(source_eval.evidence.checkpoint_path)) {
        source_proof.status = lattice_target_status_name(
            lattice_target_status_t::missing_checkpoint);
        result.proof_certificate.dependencies.push_back(source_proof);
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back(
            "CHECKPOINT_SOURCE latest_satisfying target has no checkpoint "
            "artifact: " +
            checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      result.proof_certificate.dependencies = dependency_proofs;
      result.proof_certificate.dependencies.push_back(std::move(source_proof));
      result.evidence = source_eval.evidence;
      result.proof_certificate.proof_context = detail::make_proof_context(
          spec, options_.active_identity, result.evidence,
          expected_component_fingerprint, result.split_policy_fingerprint);
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
        result.proof_certificate.dependencies.push_back(
            lattice_target_proof_certificate_t::dependency_proof_t{
                .kind = "evaluated_checkpoint_source",
                .source_target_id = evaluated_checkpoint_source_target_id,
                .status = "missing"});
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "missing EVALUATED_CHECKPOINT_SOURCE target: " +
            evaluated_checkpoint_source_target_id);
        result.plan_ready = false;
        result.suggested_wave = {};
        return result;
      }
      if (reject_non_checkpoint_latest_satisfying_reference(
              spec, "EVALUATED_CHECKPOINT_SOURCE",
              spec.evaluated_checkpoint_source, result,
              "evaluated_checkpoint_source")) {
        return result;
      }
      auto source_eval = finalize_evaluation(
          evaluate_spec(*checkpoint_source_target, dependency_stack));
      lattice_target_proof_certificate_t::dependency_proof_t source_proof{};
      source_proof.kind = "evaluated_checkpoint_source";
      source_proof.source_target_id = evaluated_checkpoint_source_target_id;
      source_proof.status = lattice_target_status_name(source_eval.status);
      source_proof.expected_mdn_checkpoint =
          source_eval.evidence.checkpoint_path;
      source_proof.expected_representation_checkpoint =
          source_eval.evidence.representation_checkpoint_path;
      if (source_eval.status != lattice_target_status_t::satisfied) {
        result.proof_certificate.dependencies.push_back(source_proof);
        result.status = lattice_target_status_t::blocked;
        result.reasons.push_back(
            "EVALUATED_CHECKPOINT_SOURCE target is not satisfied: " +
            evaluated_checkpoint_source_target_id +
            " status=" + lattice_target_status_name(source_eval.status));
        if (has_cyclic_dependency_deficit(source_eval)) {
          result.reasons.push_back("cyclic lattice target dependency");
        }
        result.plan_ready = source_eval.plan_ready;
        result.suggested_wave = source_eval.suggested_wave;
        return result;
      }
      if (source_eval.evidence.checkpoint_path.empty() ||
          !std::filesystem::exists(source_eval.evidence.checkpoint_path)) {
        source_proof.status = lattice_target_status_name(
            lattice_target_status_t::missing_checkpoint);
        result.proof_certificate.dependencies.push_back(source_proof);
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
      dependency_proofs.push_back(std::move(source_proof));
    }

    const auto candidates =
        detail::find_job_candidates(options_.runtime_root, spec);
    if (candidates.empty()) {
      result.proof_certificate.dependencies = dependency_proofs;
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
      auto candidate_eval =
          evaluate_candidate(spec, candidate, expected_component_fingerprint,
                             matching_job_count, matching_train_attempt_count,
                             evaluated_checkpoint_binding, dependency_proofs);
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
          &evaluated_checkpoint_binding = std::nullopt,
      std::vector<lattice_target_proof_certificate_t::dependency_proof_t>
          dependency_proofs = {}) const {
    namespace detail = lattice_target_eval_detail;
    lattice_target_evaluation_t result{};
    result.target_id = spec.target_id;
    result.kind = spec.kind;
    result.target_kind_applicable = lattice_target_kind_applicable(spec);
    result.component = spec.component;
    result.split_policy_fingerprint =
        detail::active_split_policy_fingerprint(options_.split_policy);
    bind_proof_obligation(result, spec);
    result.suggested_wave = detail::make_suggested_wave(spec);
    result.evidence = detail::make_evidence_from_candidate(
        candidate, spec, matching_job_count, matching_train_attempt_count);
    result.proof_certificate.dependencies = std::move(dependency_proofs);
    result.proof_certificate.proof_context = detail::make_proof_context(
        spec, options_.active_identity, result.evidence,
        expected_component_fingerprint, result.split_policy_fingerprint);

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

    if (spec.kind == lattice_target_kind_t::mtf_representation_ready) {
      if (const auto mismatch =
              detail::strict_mtf_report_schema_mismatch(result.evidence)) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back("MTF report schema mismatch: " + *mismatch);
        result.plan_ready = plan_budget_remaining;
        return result;
      }
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

    if (!options_.active_identity.protocol_id.empty() &&
        result.evidence.protocol_id != options_.active_identity.protocol_id) {
      result.status = lattice_target_status_t::stale_contract;
      result.reasons.push_back("candidate protocol id does not match active "
                               "protocol id");
      result.plan_ready = plan_budget_remaining;
      return result;
    }

    if (!spec.protocol_id.empty() &&
        result.evidence.protocol_id != spec.protocol_id) {
      result.status = lattice_target_status_t::stale_contract;
      result.reasons.push_back(
          "candidate protocol id does not match target protocol id");
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
      if (spec.kind == lattice_target_kind_t::mtf_representation_ready &&
          (!result.evidence.checkpoint_file_exists ||
           !result.evidence.checkpoint_digest_verified)) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back("MTF checkpoint digest verification failed");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (lattice_target_kind_is_mdn(spec.kind)) {
      if (!result.evidence.representation_checkpoint_loaded ||
          result.evidence.allow_untrained_representation ||
          result.evidence.representation_checkpoint_path.empty() ||
          !std::filesystem::exists(
              result.evidence.representation_checkpoint_path)) {
        result.status = lattice_target_status_t::missing_checkpoint;
        result.reasons.push_back(
            "MDN readiness requires a loaded compatible representation "
            "checkpoint");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (evaluated_checkpoint_binding.has_value()) {
      auto dependency = std::find_if(
          result.proof_certificate.dependencies.begin(),
          result.proof_certificate.dependencies.end(),
          [&](const lattice_target_proof_certificate_t::dependency_proof_t
                  &proof) {
            return proof.kind == "evaluated_checkpoint_source" &&
                   proof.source_target_id ==
                       evaluated_checkpoint_binding->source_target_id;
          });
      if (dependency != result.proof_certificate.dependencies.end()) {
        dependency->actual_mdn_checkpoint = result.evidence.mdn_checkpoint_path;
        dependency->mdn_checkpoint_match = detail::same_model_state_path(
            result.evidence.mdn_checkpoint_path,
            evaluated_checkpoint_binding->mdn_checkpoint_path);
        dependency->actual_representation_checkpoint =
            result.evidence.representation_checkpoint_path;
        dependency->representation_checkpoint_match =
            evaluated_checkpoint_binding->representation_checkpoint_path
                .empty() ||
            detail::same_model_state_path(
                result.evidence.representation_checkpoint_path,
                evaluated_checkpoint_binding->representation_checkpoint_path);
      }
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
      if (result.evidence.checkpoint_written ||
          !result.evidence.checkpoint_path.empty()) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            k_lattice_read_only_validation_model_state_reason);
        result.plan_ready = plan_budget_remaining;
        return result;
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
    if (lattice_target_kind_is_mdn(spec.kind)) {
      if (std::isfinite(spec.min_valid_target_fraction) &&
          result.evidence.valid_target_fraction <
              spec.min_valid_target_fraction) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back(
            "valid target fraction below MIN_VALID_TARGET_FRACTION");
        result.plan_ready = plan_budget_remaining;
        return result;
      }
    }

    if (spec.require_active_node_head_count > 0 ||
        spec.require_trained_node_head_count > 0 ||
        spec.require_evaluated_node_head_count > 0) {
      result.status = lattice_target_status_t::blocked;
      result.reasons.push_back(
          "node head count requirements were removed with node MDN readiness");
      result.plan_ready = false;
      return result;
    }

    if (!evaluate_exposure_requirements(spec, result, plan_budget_remaining,
                                        expected_component_fingerprint,
                                        evaluated_checkpoint_binding)) {
      return result;
    }

    result.status = lattice_target_status_t::satisfied;
    result.plan_ready = false;
    result.suggested_wave = {};
    return result;
  }

  [[nodiscard]] static std::optional<double> representation_health_metric(
      const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
          &fact,
      const std::string &metric) {
    if (!fact.representation_health_available) {
      return std::nullopt;
    }
    if (metric == "mean_invariance_loss") {
      return fact.mean_invariance_loss;
    }
    if (metric == "mean_variance_loss") {
      return fact.mean_variance_loss;
    }
    if (metric == "mean_covariance_loss") {
      return fact.mean_covariance_loss;
    }
    if (metric == "loss_jepa_mean") {
      return fact.loss_jepa_mean;
    }
    if (metric == "loss_mae_time_mean") {
      return fact.loss_mae_time_mean;
    }
    if (metric == "loss_mae_frequency_mean") {
      return fact.loss_mae_frequency_mean;
    }
    if (metric == "loss_tf_align_mean") {
      return fact.loss_tf_align_mean;
    }
    if (metric == "loss_vicreg_global_mean") {
      return fact.loss_vicreg_global_mean;
    }
    if (metric == "loss_vicreg_channel_mean") {
      return fact.loss_vicreg_channel_mean;
    }
    if (metric == "last_grad_norm") {
      return fact.last_grad_norm;
    }
    if (metric == "max_grad_norm") {
      return fact.max_grad_norm;
    }
    if (metric == "total_valid_projection_rows") {
      return static_cast<double>(fact.total_valid_projection_rows);
    }
    if (metric == "mean_adapter_valid_channel_time_fraction") {
      return fact.mean_adapter_valid_channel_time_fraction;
    }
    if (metric == "augmented_valid_feature_count") {
      return static_cast<double>(fact.augmented_valid_feature_count);
    }
    if (metric == "mean_augmented_valid_feature_fraction") {
      return fact.mean_augmented_valid_feature_fraction;
    }
    if (metric == "mean_augmented_feature_retention_fraction") {
      return fact.mean_augmented_feature_retention_fraction;
    }
    if (metric == "finite_parameter_check") {
      return fact.finite_parameter_check ? 1.0 : 0.0;
    }
    if (metric == "gradients_finite") {
      return fact.gradients_finite ? 1.0 : 0.0;
    }
    if (metric == "sample_valid_count") {
      return static_cast<double>(fact.sample_valid_count);
    }
    if (metric == "sample_valid_fraction") {
      return fact.sample_valid_fraction;
    }
    if (metric == "channel_valid_count") {
      return static_cast<double>(fact.channel_valid_count);
    }
    if (metric == "channel_valid_fraction") {
      return fact.channel_valid_fraction;
    }
    if (metric == "valid_latent_rows") {
      return static_cast<double>(fact.valid_latent_rows);
    }
    if (metric == "tf_pair_count") {
      return static_cast<double>(fact.tf_pair_count);
    }
    if (metric == "tf_pair_valid_count") {
      return static_cast<double>(fact.tf_pair_valid_count);
    }
    if (metric == "vicreg_global_valid_rows") {
      return static_cast<double>(fact.vicreg_global_valid_rows);
    }
    if (metric == "vicreg_channel_valid_rows") {
      return static_cast<double>(fact.vicreg_channel_valid_rows);
    }
    if (metric == "context_starved_sample_count") {
      return static_cast<double>(fact.context_starved_sample_count);
    }
    if (metric == "reduced_targets_for_context_count") {
      return static_cast<double>(fact.reduced_targets_for_context_count);
    }
    if (metric == "min_context_satisfied_count") {
      return static_cast<double>(fact.min_context_satisfied_count);
    }
    if (metric == "target_ema_distance") {
      return fact.target_ema_distance;
    }
    if (metric == "latent_std") {
      return fact.latent_std;
    }
    if (metric == "latent_norm") {
      return fact.latent_norm;
    }
    if (metric == "representation_embedding_dim") {
      return static_cast<double>(fact.representation_embedding_dim);
    }
    if (metric == "representation_effective_rank") {
      return fact.representation_effective_rank;
    }
    if (metric == "representation_effective_rank_fraction") {
      return fact.representation_effective_rank_fraction;
    }
    if (metric == "representation_min_dimension_variance") {
      return fact.representation_min_dimension_variance;
    }
    if (metric == "representation_max_dimension_variance") {
      return fact.representation_max_dimension_variance;
    }
    if (metric == "representation_condition_number") {
      return fact.representation_condition_number;
    }
    if (metric == "representation_isotropy_score") {
      return fact.representation_isotropy_score;
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  representation_health_unit(const std::string &metric) {
    if (metric == "total_valid_projection_rows" ||
        metric == "augmented_valid_feature_count" ||
        metric == "sample_valid_count" || metric == "channel_valid_count" ||
        metric == "valid_latent_rows" || metric == "tf_pair_count" ||
        metric == "tf_pair_valid_count" ||
        metric == "vicreg_global_valid_rows" ||
        metric == "vicreg_channel_valid_rows" ||
        metric == "context_starved_sample_count" ||
        metric == "reduced_targets_for_context_count" ||
        metric == "min_context_satisfied_count" ||
        metric == "representation_embedding_dim") {
      return "count";
    }
    if (metric == "representation_effective_rank") {
      return "rank";
    }
    if (metric == "mean_adapter_valid_channel_time_fraction" ||
        metric == "mean_augmented_valid_feature_fraction" ||
        metric == "mean_augmented_feature_retention_fraction" ||
        metric == "sample_valid_fraction" ||
        metric == "channel_valid_fraction" ||
        metric == "representation_effective_rank_fraction" ||
        metric == "representation_isotropy_score") {
      return "fraction";
    }
    if (metric == "finite_parameter_check" || metric == "gradients_finite") {
      return "boolean";
    }
    if (metric == "last_grad_norm" || metric == "max_grad_norm") {
      return "gradient_norm";
    }
    if (metric == "loss_jepa_mean" || metric == "loss_mae_time_mean" ||
        metric == "loss_mae_frequency_mean" || metric == "loss_tf_align_mean" ||
        metric == "loss_vicreg_global_mean" ||
        metric == "loss_vicreg_channel_mean") {
      return "loss";
    }
    if (metric == "target_ema_distance" || metric == "latent_norm") {
      return "distance";
    }
    if (metric == "latent_std" ||
        metric == "representation_min_dimension_variance" ||
        metric == "representation_max_dimension_variance") {
      return "variance";
    }
    if (metric == "representation_condition_number") {
      return "condition_number";
    }
    return "loss";
  }

  [[nodiscard]] static std::optional<double> runtime_health_metric(
      const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
          &fact,
      const std::string &metric) {
    const bool has_runtime_health_surface =
        fact.runtime_result_fact_available ||
        fact.runtime_checkpoint_io_fact_available ||
        fact.runtime_health_measurement_fact_available ||
        fact.inference_health_available || fact.representation_health_available;
    if (metric == "finite_parameter_check") {
      return has_runtime_health_surface
                 ? std::optional<double>(fact.finite_parameter_check ? 1.0
                                                                     : 0.0)
                 : std::nullopt;
    }
    if (metric == "nonfinite_output_count") {
      return has_runtime_health_surface
                 ? std::optional<double>(
                       static_cast<double>(fact.nonfinite_output_count))
                 : std::nullopt;
    }
    if (metric == "checkpoint_written") {
      return fact.checkpoint_written || !fact.output_checkpoint.empty() ? 1.0
                                                                        : 0.0;
    }
    if (metric == "model_state_mutated") {
      return fact.model_state_mutated || fact.use.mutated_component ? 1.0 : 0.0;
    }
    if (metric == "representation_checkpoint_loaded") {
      return fact.runtime_checkpoint_io_fact_available ||
                     !fact.input_checkpoints.empty()
                 ? std::optional<double>(
                       fact.representation_checkpoint_loaded ? 1.0 : 0.0)
                 : std::nullopt;
    }
    if (metric == "mdn_checkpoint_loaded") {
      return fact.runtime_checkpoint_io_fact_available ||
                     !fact.input_checkpoints.empty()
                 ? std::optional<double>(fact.mdn_checkpoint_loaded ? 1.0 : 0.0)
                 : std::nullopt;
    }
    if (metric == "grad_norm_max_pre_clip" || metric == "max_grad_norm") {
      return fact.max_grad_norm;
    }
    if (metric == "grad_norm_clip_ratio") {
      if (std::isfinite(fact.max_grad_norm) &&
          std::isfinite(fact.grad_clip_norm) && fact.grad_clip_norm > 0.0) {
        return fact.max_grad_norm / fact.grad_clip_norm;
      }
      return std::nullopt;
    }
    if (metric == "sigma_min") {
      return fact.min_sigma_min;
    }
    if (metric == "sigma_mean") {
      return fact.mean_sigma_mean;
    }
    if (metric == "sigma_max") {
      return fact.max_sigma_max;
    }
    if (metric == "sigma_min_valid") {
      return fact.sigma_min_valid;
    }
    if (metric == "sigma_mean_valid") {
      return fact.sigma_mean_valid;
    }
    if (metric == "sigma_max_valid") {
      return fact.sigma_max_valid;
    }
    if (metric == "mixture_entropy") {
      return fact.mean_mixture_entropy;
    }
    if (metric == "valid_target_fraction") {
      return fact.valid_target_fraction;
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  runtime_health_unit(const std::string &metric) {
    if (metric == "finite_parameter_check" || metric == "checkpoint_written" ||
        metric == "model_state_mutated" ||
        metric == "representation_checkpoint_loaded" ||
        metric == "mdn_checkpoint_loaded") {
      return "boolean";
    }
    if (metric == "nonfinite_output_count") {
      return "count";
    }
    if (metric == "valid_target_fraction") {
      return "fraction";
    }
    if (metric == "grad_norm_max_pre_clip" || metric == "max_grad_norm") {
      return "gradient_norm";
    }
    if (metric == "grad_norm_clip_ratio") {
      return "ratio";
    }
    if (metric == "sigma_min" || metric == "sigma_mean" ||
        metric == "sigma_max" || metric == "sigma_min_valid" ||
        metric == "sigma_mean_valid" || metric == "sigma_max_valid") {
      return "sigma";
    }
    if (metric == "mixture_entropy") {
      return "entropy";
    }
    return "metric";
  }

  [[nodiscard]] static bool
  runtime_health_metric_is_severe(const std::string &metric) {
    return metric == "finite_parameter_check" ||
           metric == "nonfinite_output_count" ||
           metric == "checkpoint_written" || metric == "model_state_mutated" ||
           metric == "representation_checkpoint_loaded" ||
           metric == "mdn_checkpoint_loaded";
  }

  [[nodiscard]] static std::optional<double>
  source_analytics_metric(const cuwacunu::kikijyeba::lattice::exposure::
                              lattice_source_analytics_fact_t &fact,
                          const std::string &metric) {
    if (metric == "entropy") {
      return fact.entropy;
    }
    if (metric == "entropy_rate") {
      return fact.entropy_rate;
    }
    if (metric == "information_density") {
      return fact.information_density;
    }
    if (metric == "compression_ratio") {
      return fact.compression_ratio;
    }
    if (metric == "power_spectrum_entropy") {
      return fact.power_spectrum_entropy;
    }
    if (metric == "sample_validity_fraction") {
      return fact.sample_validity_fraction;
    }
    if (metric == "missingness_fraction") {
      return fact.missingness_fraction;
    }
    if (metric == "duplicate_sample_count") {
      return static_cast<double>(fact.duplicate_sample_count);
    }
    if (metric == "source_receipt_fact_count") {
      return static_cast<double>(fact.source_receipt_fact_count);
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  source_analytics_unit(const std::string &metric) {
    if (metric == "sample_validity_fraction" ||
        metric == "missingness_fraction") {
      return "fraction";
    }
    if (metric == "duplicate_sample_count" ||
        metric == "source_receipt_fact_count") {
      return "count";
    }
    if (metric == "compression_ratio") {
      return "ratio";
    }
    if (metric == "entropy" || metric == "entropy_rate" ||
        metric == "power_spectrum_entropy") {
      return "entropy";
    }
    if (metric == "information_density") {
      return "density";
    }
    return "metric";
  }

  [[nodiscard]] static std::optional<double>
  max_finite_value(const std::vector<double> &values) {
    double out = std::numeric_limits<double>::quiet_NaN();
    for (const auto value : values) {
      if (!std::isfinite(value)) {
        continue;
      }
      out = std::isfinite(out) ? std::max(out, value) : value;
    }
    if (!std::isfinite(out)) {
      return std::nullopt;
    }
    return out;
  }

  [[nodiscard]] static std::optional<double>
  min_positive_count_value(const std::vector<std::int64_t> &values) {
    std::optional<std::int64_t> out;
    for (const auto value : values) {
      if (value <= 0) {
        continue;
      }
      out = out.has_value() ? std::min(*out, value) : value;
    }
    if (!out.has_value()) {
      return std::nullopt;
    }
    return static_cast<double>(*out);
  }

  [[nodiscard]] static std::optional<double> forecast_eval_metric(
      const cuwacunu::kikijyeba::lattice::exposure::lattice_forecast_eval_fact_t
          &fact,
      const std::string &metric) {
    if (metric == "mean_nll") {
      return fact.mean_nll;
    }
    if (metric == "ev_mae") {
      return fact.ev_mae;
    }
    if (metric == "ev_rmse") {
      return fact.ev_rmse;
    }
    if (metric == "signed_error") {
      return fact.signed_error;
    }
    if (metric == "directional_accuracy") {
      return fact.directional_accuracy;
    }
    if (metric == "calibration_coverage") {
      return fact.calibration_coverage;
    }
    if (metric == "support_count") {
      return static_cast<double>(fact.support_count);
    }
    if (metric == "valid_count") {
      return static_cast<double>(fact.valid_count);
    }
    if (metric == "missing_count") {
      return static_cast<double>(fact.missing_count);
    }
    if (metric == "weakest_support_rows") {
      return static_cast<double>(fact.weakest_support_rows);
    }
    if (metric == "weakest_horizon_support_rows") {
      return min_positive_count_value(fact.valid_target_count_per_horizon);
    }
    if (metric == "mean_nll_per_horizon_max") {
      return max_finite_value(fact.mean_nll_per_horizon);
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::optional<double> forecast_eval_baseline_metric(
      const cuwacunu::kikijyeba::lattice::exposure::lattice_forecast_eval_fact_t
          &fact,
      const std::vector<cuwacunu::kikijyeba::lattice::exposure::
                            lattice_forecast_baseline_fact_t> &baseline_facts,
      const std::string &metric) {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    for (const auto &digest : fact.baseline_fact_digests) {
      const exposure::lattice_forecast_baseline_fact_t *baseline = nullptr;
      const auto resolution =
          exposure::resolve_related_fact_digest_for_identity(
              fact, digest, baseline_facts,
              exposure::forecast_baseline_fact_digest, &baseline);
      if (resolution != exposure::related_fact_digest_resolution_t::resolved ||
          baseline == nullptr) {
        continue;
      }
      if (metric == "skill_vs_baseline" || metric == "skill_vs_naive" ||
          metric == "skill_vs_baseline_ev_rmse") {
        return exposure::forecast_skill_lower_is_better(
            fact.ev_rmse, baseline->baseline_ev_rmse);
      }
      if (metric == "skill_vs_baseline_ev_mae") {
        return exposure::forecast_skill_lower_is_better(
            fact.ev_mae, baseline->baseline_ev_mae);
      }
      if (metric == "skill_vs_baseline_mean_nll") {
        return exposure::forecast_skill_lower_is_better(
            fact.mean_nll, baseline->baseline_mean_nll);
      }
      if (metric == "directional_accuracy_delta_vs_baseline") {
        return exposure::forecast_delta_higher_is_better(
            fact.directional_accuracy, baseline->baseline_directional_accuracy);
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  forecast_eval_unit(const std::string &metric) {
    if (metric == "skill_vs_baseline" || metric == "skill_vs_naive" ||
        metric == "skill_vs_baseline_mean_nll" ||
        metric == "skill_vs_baseline_ev_mae" ||
        metric == "skill_vs_baseline_ev_rmse" ||
        metric == "directional_accuracy_delta_vs_baseline") {
      return "relative_skill";
    }
    if (metric == "directional_accuracy" || metric == "calibration_coverage") {
      return "fraction";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count" || metric == "weakest_support_rows" ||
        metric == "weakest_horizon_support_rows") {
      return "count";
    }
    if (metric == "mean_nll" || metric == "mean_nll_per_horizon_max") {
      return "nll";
    }
    if (metric == "ev_mae" || metric == "ev_rmse" || metric == "signed_error") {
      return "expected_value_error";
    }
    return "metric";
  }

  [[nodiscard]] static std::string
  forecast_eval_metric_family(const std::string &metric) {
    if (metric == "skill_vs_baseline" || metric == "skill_vs_naive" ||
        metric == "skill_vs_baseline_mean_nll" ||
        metric == "skill_vs_baseline_ev_mae" ||
        metric == "skill_vs_baseline_ev_rmse" ||
        metric == "directional_accuracy_delta_vs_baseline") {
      return "baseline_comparison";
    }
    if (metric == "mean_nll") {
      return "proper_scoring_loss";
    }
    if (metric == "mean_nll_per_horizon_max") {
      return "horizon_stratified_scoring_loss";
    }
    if (metric == "ev_mae" || metric == "ev_rmse" || metric == "signed_error") {
      return "expected_value_error";
    }
    if (metric == "directional_accuracy") {
      return "directional_accuracy";
    }
    if (metric == "calibration_coverage") {
      return "interval_calibration";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count" || metric == "weakest_support_rows" ||
        metric == "weakest_horizon_support_rows") {
      return "support_surface";
    }
    return "forecast_evaluation";
  }

  [[nodiscard]] static std::string
  forecast_eval_uncertainty_method(const std::string &metric) {
    if (metric == "skill_vs_baseline" || metric == "skill_vs_naive" ||
        metric == "skill_vs_baseline_mean_nll" ||
        metric == "skill_vs_baseline_ev_mae" ||
        metric == "skill_vs_baseline_ev_rmse" ||
        metric == "directional_accuracy_delta_vs_baseline") {
      return "none_point_estimate_only_baseline_bound";
    }
    if (metric == "directional_accuracy" || metric == "calibration_coverage") {
      return "wilson_or_bootstrap_future";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count" || metric == "weakest_support_rows" ||
        metric == "weakest_horizon_support_rows") {
      return "count_identity";
    }
    return "none_point_estimate_only";
  }

  [[nodiscard]] static std::optional<double>
  forecast_baseline_metric(const cuwacunu::kikijyeba::lattice::exposure::
                               lattice_forecast_baseline_fact_t &fact,
                           const std::string &metric) {
    if (metric == "baseline_mean_nll") {
      return fact.baseline_mean_nll;
    }
    if (metric == "baseline_ev_mae") {
      return fact.baseline_ev_mae;
    }
    if (metric == "baseline_ev_rmse") {
      return fact.baseline_ev_rmse;
    }
    if (metric == "baseline_signed_error") {
      return fact.baseline_signed_error;
    }
    if (metric == "baseline_directional_accuracy") {
      return fact.baseline_directional_accuracy;
    }
    if (metric == "support_count") {
      return static_cast<double>(fact.support_count);
    }
    if (metric == "valid_count") {
      return static_cast<double>(fact.valid_count);
    }
    if (metric == "missing_count") {
      return static_cast<double>(fact.missing_count);
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  forecast_baseline_unit(const std::string &metric) {
    if (metric == "baseline_directional_accuracy") {
      return "fraction";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count" || metric == "baseline_kind_count" ||
        metric == "computed_metric_fact_count" ||
        metric == "partial_metric_fact_count" ||
        metric == "deferred_metric_fact_count" ||
        metric == "computed_metric_value_count" ||
        metric == "metric_status_mismatch_count") {
      return "count";
    }
    if (metric == "baseline_mean_nll") {
      return "nll";
    }
    if (metric == "baseline_ev_mae" || metric == "baseline_ev_rmse" ||
        metric == "baseline_signed_error") {
      return "expected_value_error";
    }
    return "metric";
  }

  [[nodiscard]] static std::string
  forecast_baseline_metric_family(const std::string &metric) {
    if (metric == "baseline_mean_nll") {
      return "baseline_scoring_loss";
    }
    if (metric == "baseline_ev_mae" || metric == "baseline_ev_rmse" ||
        metric == "baseline_signed_error") {
      return "baseline_expected_value_error";
    }
    if (metric == "baseline_directional_accuracy") {
      return "baseline_directional_accuracy";
    }
    if (metric == "baseline_kind_count") {
      return "baseline_kind_coverage";
    }
    if (metric == "computed_metric_fact_count" ||
        metric == "partial_metric_fact_count" ||
        metric == "deferred_metric_fact_count" ||
        metric == "computed_metric_value_count" ||
        metric == "metric_status_mismatch_count") {
      return "baseline_metric_coverage";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count") {
      return "baseline_support_surface";
    }
    return "forecast_baseline";
  }

  [[nodiscard]] static std::string
  forecast_baseline_uncertainty_method(const std::string &metric) {
    if (metric == "baseline_directional_accuracy") {
      return "wilson_or_bootstrap_future";
    }
    if (metric == "support_count" || metric == "valid_count" ||
        metric == "missing_count" || metric == "baseline_kind_count" ||
        metric == "computed_metric_fact_count" ||
        metric == "partial_metric_fact_count" ||
        metric == "deferred_metric_fact_count" ||
        metric == "computed_metric_value_count" ||
        metric == "metric_status_mismatch_count") {
      return "count_identity";
    }
    return "none_point_estimate_only";
  }

  [[nodiscard]] static std::optional<double>
  observer_belief_metric(const cuwacunu::kikijyeba::lattice::exposure::
                             lattice_observer_belief_fact_t &fact,
                         const std::string &metric) {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    if (metric == "confidence") {
      return fact.confidence;
    }
    if (metric == "data_quality") {
      return fact.data_quality;
    }
    if (metric == "liquidity") {
      return fact.liquidity;
    }
    if (metric == "raw_potential_tradable_return") {
      return fact.raw_potential_tradable_return ? 1.0 : 0.0;
    }
    if (metric == "allocation_authority") {
      return fact.allocation_authority ? 1.0 : 0.0;
    }
    if (metric == "forecast_lineage_bound") {
      return (!fact.forecast_artifact_digest.empty() &&
              !fact.forecast_artifact_lineage.empty())
                 ? 1.0
                 : 0.0;
    }
    if (metric == "channel_consensus_bound") {
      return exposure::lattice_visibility_text_available(fact.channel_consensus)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "potential_surface_diagnostics_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.potential_surface_diagnostics)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "nodelift_return_projection_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.nodelift_return_projection)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "covariance_coupling_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.covariance_coupling)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "scenario_bank_bound") {
      return fact.scenario_bank_digest.empty() ? 0.0 : 1.0;
    }
    if (metric == "nodelift_residual_quality_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.nodelift_residual_quality)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "projection_validation_scores_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.projection_validation_scores)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "feature_semantics_bound") {
      return fact.feature_semantics_fingerprint.empty() ? 0.0 : 1.0;
    }
    if (metric == "dock_binding_bound") {
      return fact.dock_binding_fingerprint.empty() ? 0.0 : 1.0;
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  observer_belief_unit(const std::string &metric) {
    if (metric == "raw_potential_tradable_return" ||
        metric == "allocation_authority" ||
        metric == "forecast_lineage_bound" || metric == "scenario_bank_bound" ||
        metric == "channel_consensus_bound" ||
        metric == "potential_surface_diagnostics_bound" ||
        metric == "nodelift_return_projection_bound" ||
        metric == "covariance_coupling_bound" ||
        metric == "nodelift_residual_quality_bound" ||
        metric == "projection_validation_scores_bound" ||
        metric == "feature_semantics_bound" || metric == "dock_binding_bound") {
      return "boolean";
    }
    if (metric == "confidence" || metric == "data_quality" ||
        metric == "liquidity") {
      return "fraction";
    }
    return "metric";
  }

  [[nodiscard]] static std::string
  observer_belief_metric_family(const std::string &metric) {
    if (metric == "confidence" || metric == "data_quality" ||
        metric == "liquidity") {
      return "observer_quality_visibility";
    }
    if (metric == "raw_potential_tradable_return" ||
        metric == "allocation_authority") {
      return "observer_authority_boundary";
    }
    if (metric == "forecast_lineage_bound" ||
        metric == "channel_consensus_bound" ||
        metric == "potential_surface_diagnostics_bound" ||
        metric == "nodelift_return_projection_bound" ||
        metric == "covariance_coupling_bound" ||
        metric == "scenario_bank_bound" ||
        metric == "nodelift_residual_quality_bound" ||
        metric == "projection_validation_scores_bound" ||
        metric == "feature_semantics_bound" || metric == "dock_binding_bound") {
      return "observer_lineage_integrity";
    }
    return "observer_belief";
  }

  [[nodiscard]] static std::string
  observer_belief_uncertainty_method(const std::string &metric) {
    if (metric == "confidence" || metric == "data_quality" ||
        metric == "liquidity") {
      return "none_point_estimate_only";
    }
    return "boolean_identity";
  }

  [[nodiscard]] static std::optional<double>
  allocation_engine_metric(const cuwacunu::kikijyeba::lattice::exposure::
                               lattice_allocation_engine_fact_t &fact,
                           const std::string &metric) {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    if (metric == "reserve_weight") {
      return fact.reserve_weight;
    }
    if (metric == "turnover") {
      return fact.turnover;
    }
    if (metric == "cvar_loss") {
      return fact.cvar_loss;
    }
    if (metric == "transaction_cost_estimate") {
      return fact.transaction_cost_estimate;
    }
    if (metric == "deterministic_artifact") {
      return fact.deterministic_artifact ? 1.0 : 0.0;
    }
    if (metric == "visibility_only") {
      return fact.visibility_only ? 1.0 : 0.0;
    }
    if (metric == "allocation_authority") {
      return fact.allocation_authority ? 1.0 : 0.0;
    }
    if (metric == "execution_authority") {
      return fact.execution_authority ? 1.0 : 0.0;
    }
    if (metric == "market_readiness_authority") {
      return fact.market_readiness_authority ? 1.0 : 0.0;
    }
    if (metric == "deployment_authority") {
      return fact.deployment_authority ? 1.0 : 0.0;
    }
    if (metric == "reserve_node_bound") {
      return fact.reserve_node_id.empty() ? 0.0 : 1.0;
    }
    if (metric == "reserve_node_from_base_policy") {
      return fact.reserve_node_source == "base_policy" ? 1.0 : 0.0;
    }
    if (metric == "reserve_node_base_policy_match") {
      return !fact.reserve_node_id.empty() &&
                     !fact.base_policy_reserve_node_id.empty() &&
                     fact.reserve_node_id == fact.base_policy_reserve_node_id
                 ? 1.0
                 : 0.0;
    }
    if (metric == "reserve_node_graph_bound") {
      return fact.reserve_node_graph_bound ? 1.0 : 0.0;
    }
    if (metric == "cap_diagnostics_bound") {
      return exposure::lattice_visibility_text_available(fact.cap_diagnostics)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "scenario_growth_floor_status_bound") {
      return exposure::lattice_visibility_text_available(
                 fact.scenario_growth_floor_status)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "scenario_growth_floor_met") {
      return exposure::allocation_scenario_growth_floor_status_met(
                 fact.scenario_growth_floor_status)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "scenario_growth_floor_attention") {
      return exposure::allocation_scenario_growth_floor_status_attention(
                 fact.scenario_growth_floor_status)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "observer_belief_bound") {
      return fact.observer_belief_fact_digest.empty() ? 0.0 : 1.0;
    }
    if (metric == "forecast_artifact_bound") {
      return fact.forecast_artifact_digest.empty() ? 0.0 : 1.0;
    }
    if (metric == "base_policy_bound") {
      return fact.base_policy_digest.empty() ? 0.0 : 1.0;
    }
    if (metric == "fallback_declared") {
      return fact.fallback_reasons.empty() ? 0.0 : 1.0;
    }
    if (metric == "fallback_active") {
      return exposure::lattice_reason_text_is_active(fact.fallback_reasons)
                 ? 1.0
                 : 0.0;
    }
    if (metric == "derisk_declared") {
      return fact.derisk_reasons.empty() ? 0.0 : 1.0;
    }
    if (metric == "derisk_active") {
      return exposure::lattice_reason_text_is_active(fact.derisk_reasons) ? 1.0
                                                                          : 0.0;
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  allocation_engine_unit(const std::string &metric) {
    if (metric == "reserve_weight" || metric == "turnover" ||
        metric == "transaction_cost_estimate") {
      return "fraction";
    }
    if (metric == "deterministic_artifact" || metric == "visibility_only" ||
        metric == "allocation_authority" || metric == "execution_authority" ||
        metric == "market_readiness_authority" ||
        metric == "deployment_authority" || metric == "reserve_node_bound" ||
        metric == "reserve_node_from_base_policy" ||
        metric == "reserve_node_base_policy_match" ||
        metric == "reserve_node_graph_bound" ||
        metric == "cap_diagnostics_bound" ||
        metric == "scenario_growth_floor_status_bound" ||
        metric == "scenario_growth_floor_met" ||
        metric == "scenario_growth_floor_attention" ||
        metric == "observer_belief_bound" ||
        metric == "forecast_artifact_bound" || metric == "base_policy_bound" ||
        metric == "fallback_declared" || metric == "fallback_active" ||
        metric == "derisk_declared" || metric == "derisk_active") {
      return "boolean";
    }
    if (metric == "cvar_loss") {
      return "loss";
    }
    return "metric";
  }

  [[nodiscard]] static std::string
  allocation_engine_metric_family(const std::string &metric) {
    if (metric == "reserve_weight" || metric == "turnover" ||
        metric == "cvar_loss" || metric == "transaction_cost_estimate") {
      return "allocation_diagnostics";
    }
    if (metric == "allocation_authority" || metric == "execution_authority" ||
        metric == "market_readiness_authority" ||
        metric == "deployment_authority") {
      return "allocation_authority_boundary";
    }
    if (metric == "reserve_node_bound" || metric == "observer_belief_bound" ||
        metric == "forecast_artifact_bound" || metric == "base_policy_bound" ||
        metric == "reserve_node_from_base_policy" ||
        metric == "reserve_node_base_policy_match" ||
        metric == "reserve_node_graph_bound" ||
        metric == "cap_diagnostics_bound" ||
        metric == "scenario_growth_floor_status_bound" ||
        metric == "scenario_growth_floor_met" ||
        metric == "scenario_growth_floor_attention" ||
        metric == "fallback_declared" || metric == "fallback_active" ||
        metric == "derisk_declared" || metric == "derisk_active" ||
        metric == "deterministic_artifact" || metric == "visibility_only") {
      return "allocation_lineage_integrity";
    }
    return "allocation_engine";
  }

  [[nodiscard]] static std::string
  allocation_engine_uncertainty_method(const std::string &metric) {
    if (metric == "reserve_weight" || metric == "turnover" ||
        metric == "cvar_loss" || metric == "transaction_cost_estimate") {
      return "none_point_estimate_only";
    }
    return "boolean_identity";
  }

  [[nodiscard]] bool evaluate_representation_geometry_requirements(
      const lattice_target_spec_t &spec, lattice_target_evaluation_t &result,
      bool plan_budget_remaining,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
          &target_component_family_facts) const {
    namespace detail = lattice_target_eval_detail;
    if (spec.representation_geometry_requirements.empty()) {
      return true;
    }
    const auto target_range = detail::target_anchor_interval(spec);
    for (const auto &requirement : spec.representation_geometry_requirements) {
      lattice_target_evaluation_t::representation_geometry_gate_result_t gate{};
      gate.requirement_id = requirement.requirement_id;
      gate.metric = requirement.metric;
      gate.op = requirement.op;
      gate.threshold = requirement.value;
      gate.unit = representation_health_unit(requirement.metric);

      for (const auto &fact : target_component_family_facts) {
        if (requirement.require_mutated_component &&
            !fact.use.mutated_component) {
          continue;
        }
        if (!target_range.empty() &&
            cuwacunu::kikijyeba::lattice::exposure::interval_intersection(
                fact.completed_anchor_range, target_range)
                .empty()) {
          continue;
        }
        const auto value = lattice_representation_geometry_metric_value(
            fact, requirement.metric);
        if (!value.has_value() || !std::isfinite(*value)) {
          continue;
        }
        ++gate.fact_count;
        if (!std::isfinite(gate.measured_value)) {
          gate.measured_value = *value;
        } else if (requirement.op == "ge") {
          gate.measured_value = std::min(gate.measured_value, *value);
        } else {
          gate.measured_value = std::max(gate.measured_value, *value);
        }
      }

      gate.measurement_available = std::isfinite(gate.measured_value);
      gate.passed = gate.measurement_available &&
                    (requirement.op == "ge"
                         ? gate.measured_value + 1e-12 >= requirement.value
                         : gate.measured_value <= requirement.value + 1e-12);
      std::ostringstream message;
      message << "representation geometry " << requirement.metric;
      if (gate.measurement_available) {
        message << " is " << gate.measured_value
                << (requirement.op == "ge" ? " >= " : " <= ")
                << requirement.value;
      } else {
        message << " is unavailable for explicit gate "
                << (requirement.op == "ge" ? ">= " : "<= ")
                << requirement.value;
      }
      gate.status = gate.passed
                        ? "satisfied"
                        : (gate.measurement_available ? "failed" : "missing");
      gate.message = message.str();
      result.representation_geometry_gate_results.push_back(gate);

      if (!gate.passed) {
        result.status = lattice_target_status_t::metric_failed;
        result.reasons.push_back(gate.message);
        result.plan_ready = plan_budget_remaining;
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] static bool exposure_use_set_has(
      const cuwacunu::kikijyeba::lattice::exposure::exposure_use_set_t &use,
      cuwacunu::kikijyeba::lattice::exposure::exposure_use_t requested) {
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    switch (requested) {
    case exposure::exposure_use_t::observed_input:
      return use.observed_input;
    case exposure::exposure_use_t::target_supervision:
      return use.target_supervision;
    case exposure::exposure_use_t::evaluation_metric:
      return use.evaluation_metric;
    case exposure::exposure_use_t::selection_signal:
      return use.selection_signal;
    }
    return false;
  }

  [[nodiscard]] static std::optional<double> mdn_distribution_metric(
      const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
          &fact,
      const std::string &metric) {
    if (fact.target_component_family_id !=
            "wikimyei.inference.expected_value.mdn" &&
        fact.target_component_family_id !=
            "wikimyei.inference.expected_value.mdn") {
      return std::nullopt;
    }
    if (metric == "mean_nll") {
      return fact.mean_nll;
    }
    const auto max_finite =
        [](const std::vector<double> &values) -> std::optional<double> {
      double out = std::numeric_limits<double>::quiet_NaN();
      for (const auto value : values) {
        if (!std::isfinite(value)) {
          continue;
        }
        out = std::isfinite(out) ? std::max(out, value) : value;
      }
      if (!std::isfinite(out)) {
        return std::nullopt;
      }
      return out;
    };
    if (metric == "mean_nll_per_channel_max") {
      return max_finite(fact.mean_nll_per_channel);
    }
    if (metric == "mean_nll_per_target_feature_max") {
      return max_finite(fact.mean_nll_per_target_feature);
    }
    if (metric == "mean_nll_per_channel_target_feature_max") {
      return max_finite(fact.mean_nll_per_channel_target_feature);
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::string
  mdn_distribution_metric_family(const std::string &metric) {
    if (metric == "mean_nll") {
      return "proper_scoring_loss";
    }
    if (metric == "mean_nll_per_channel_max") {
      return "channel_stratified_scoring_loss";
    }
    if (metric == "mean_nll_per_target_feature_max") {
      return "target_feature_stratified_scoring_loss";
    }
    if (metric == "mean_nll_per_channel_target_feature_max") {
      return "channel_target_feature_stratified_scoring_loss";
    }
    if (metric == "per_node_mean_nll_max") {
      return "node_stratified_scoring_loss";
    }
    if (metric == "pit_ks_statistic") {
      return "probability_integral_transform";
    }
    if (metric == "predictive_interval_coverage_error") {
      return "interval_calibration";
    }
    if (metric == "tail_coverage_error") {
      return "tail_calibration";
    }
    if (metric == "calibration_slope_error") {
      return "calibration_regression";
    }
    return "proper_scoring_loss";
  }

  [[nodiscard]] static std::string
  mdn_distribution_uncertainty_method(const std::string &metric) {
    if (metric == "mean_nll" || metric == "mean_nll_per_channel_max" ||
        metric == "mean_nll_per_target_feature_max" ||
        metric == "mean_nll_per_channel_target_feature_max" ||
        metric == "per_node_mean_nll_max") {
      return "none_point_estimate_only";
    }
    if (metric == "pit_ks_statistic") {
      return "asymptotic_or_bootstrap_future";
    }
    if (metric == "predictive_interval_coverage_error") {
      return "wilson_or_bootstrap_future";
    }
    if (metric == "tail_coverage_error") {
      return "bootstrap_future";
    }
    if (metric == "calibration_slope_error") {
      return "standard_error_future";
    }
    return "future_uncertainty_required";
  }

  [[nodiscard]] static std::string
  mdn_distribution_metric_unit(const std::string &metric) {
    if (metric == "mean_nll" || metric == "mean_nll_per_channel_max" ||
        metric == "mean_nll_per_target_feature_max" ||
        metric == "mean_nll_per_channel_target_feature_max" ||
        metric == "per_node_mean_nll_max") {
      return "nll";
    }
    if (metric == "pit_ks_statistic" ||
        metric == "predictive_interval_coverage_error" ||
        metric == "tail_coverage_error") {
      return "fraction";
    }
    if (metric == "calibration_slope_error") {
      return "absolute_error";
    }
    return "loss";
  }

  [[nodiscard]] static const char *
  threshold_comparison_phrase(bool triggered, bool use_above_threshold) {
    if (triggered) {
      return use_above_threshold ? " above threshold " : " below threshold ";
    }
    return use_above_threshold ? ", not above threshold "
                               : ", not below threshold ";
  }

  [[nodiscard]] static bool warning_fact_overlaps_anchor_range(
      const lattice_target_spec_t &spec, const lattice_warning_spec_t &warning,
      const cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t
          &fact,
      cuwacunu::kikijyeba::lattice::exposure::exposure_use_t use) {
    namespace detail = lattice_target_eval_detail;
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
    const auto range = detail::warning_anchor_interval(spec, warning);
    if (range.empty()) {
      return true;
    }
    auto coverage = exposure::anchor_coverage_for_use(fact, use);
    if (coverage.empty()) {
      coverage = fact.completed_anchor_range.empty()
                     ? fact.anchor_range
                     : fact.completed_anchor_range;
    }
    return !coverage.empty() &&
           !exposure::interval_intersection(coverage, range).empty();
  }

  void evaluate_warning_specs(
      const lattice_target_spec_t &spec, lattice_target_evaluation_t &result,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
          &exposure_facts,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_exposure_fact_t>
          &target_component_family_facts,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_node_exposure_fact_t>
          &node_facts,
      const std::vector<cuwacunu::kikijyeba::lattice::exposure::
                            lattice_source_analytics_fact_t>
          &source_analytics_facts,
      const std::vector<cuwacunu::kikijyeba::lattice::exposure::
                            lattice_forecast_baseline_fact_t>
          &forecast_baseline_facts,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::lattice_forecast_eval_fact_t>
          &forecast_eval_facts,
      const std::vector<cuwacunu::kikijyeba::lattice::exposure::
                            lattice_observer_belief_fact_t>
          &observer_belief_facts,
      const std::vector<cuwacunu::kikijyeba::lattice::exposure::
                            lattice_allocation_engine_fact_t>
          &allocation_engine_facts,
      const std::vector<
          cuwacunu::kikijyeba::lattice::exposure::node_support_summary_t>
          &node_support_summaries) const {
    namespace detail = lattice_target_eval_detail;
    namespace exposure = cuwacunu::kikijyeba::lattice::exposure;

    for (const auto &warning : spec.warning_specs) {
      lattice_target_evaluation_t::warning_result_t out{};
      out.warning_id = warning.warning_id;
      out.kind = warning.kind;
      out.split = warning.split;
      out.warning_anchor_range = detail::warning_anchor_interval(spec, warning);
      out.use = exposure::exposure_use_name(warning.use);
      out.scope = warning.scope;
      out.effect =
          warning.require_mutated_component ? "mutated_component" : "any";
      out.metric = warning.metric;

      const auto &facts = warning.scope == "all_components"
                              ? exposure_facts
                              : target_component_family_facts;
      const auto target_component_family_id_filter =
          warning.scope == "target_component_family_id" ? spec.component
                                                        : std::string{};

      if (warning.kind == "exposure_load" || warning.kind == "effort_density") {
        out.evidence_basis = "exposure_load_summary";
        const auto range = detail::warning_anchor_interval(spec, warning);
        out.exposure_summary = exposure::exposure_load_for_use(
            facts, range, warning.use, warning.require_mutated_component,
            target_component_family_id_filter);
        out.exposure_summary_available = true;
        if (warning.kind == "exposure_load") {
          out.measured_value = out.exposure_summary.cursor_exposure_load;
          out.threshold = warning.cursor_epochs_above;
          out.threshold_direction = "above";
          out.unit = "cursor_epochs";
          out.measurement_available = std::isfinite(out.measured_value);
          const bool triggered = std::isfinite(out.threshold) &&
                                 out.measured_value > out.threshold;
          std::ostringstream message;
          message << out.use << " exposure load";
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " is " << out.measured_value << " cursor_epochs";
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered,
                                                   /*use_above_threshold=*/true)
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
          out.threshold_direction = "above";
          out.unit = warning.metric;
          out.measurement_available = std::isfinite(out.measured_value);
          const bool triggered = std::isfinite(out.threshold) &&
                                 out.measured_value > out.threshold;
          std::ostringstream message;
          message << warning.metric;
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered,
                                                   /*use_above_threshold=*/true)
                    << out.threshold;
          }
          out.message = message.str();
        }
        if (std::isfinite(out.threshold) &&
            out.measured_value > out.threshold) {
          assign_warning_threshold_relation(out, /*triggered=*/true);
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        } else {
          assign_warning_threshold_relation(out, /*triggered=*/false);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "node_support_balance" ||
          warning.kind == "node_support_floor") {
        out.evidence_basis = "filtered_node_support_summary";
        out.metric = warning.metric;
        const bool node_support_floor = warning.kind == "node_support_floor";
        const bool entropy_balance =
            warning.kind == "node_support_balance" &&
            warning.metric == "valid_target_count_normalized_entropy";
        out.threshold = node_support_floor || entropy_balance ? warning.below
                                                              : warning.above;
        out.threshold_direction =
            node_support_floor || entropy_balance ? "below" : "above";
        if (node_support_floor) {
          out.unit = warning.metric == "weakest_valid_target_count"
                         ? "count"
                         : "fraction";
        } else if (warning.metric == "valid_target_count_gini" ||
                   warning.metric == "valid_target_count_normalized_entropy") {
          out.unit = "fraction";
        } else {
          out.unit = "coefficient_of_variation";
        }
        const auto summary_it = std::find_if(
            node_support_summaries.begin(), node_support_summaries.end(),
            [&](const exposure::node_support_summary_t &summary) {
              return warning.split.empty() ||
                     summary.split_name == warning.split;
            });
        const auto *summary = summary_it == node_support_summaries.end()
                                  ? (node_support_summaries.size() == 1
                                         ? &node_support_summaries.front()
                                         : nullptr)
                                  : &*summary_it;
        exposure::node_support_summary_t warning_summary{};
        bool warning_summary_available = false;
        if (!node_facts.empty()) {
          warning_summary = exposure::summarize_node_support_for_range(
              node_facts, detail::warning_anchor_interval(spec, warning),
              spec.component);
          warning_summary_available = warning_summary.node_count > 0;
        } else if (summary != nullptr) {
          warning_summary = *summary;
          warning_summary_available = true;
        }
        if (!warning_summary_available) {
          std::ostringstream message;
          message << "node-support summary";
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " is unavailable for " << spec.target_id;
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
          out.message = message.str();
          assign_warning_threshold_relation(out, /*triggered=*/false);
          result.warning_results.push_back(std::move(out));
          continue;
        }
        auto filtered_summary = exposure::filter_node_support_summary(
            warning_summary, warning.use, warning.require_mutated_component);
        if (filtered_summary.node_count == 0) {
          std::ostringstream message;
          message << "node-support summary";
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          message << " has no " << out.use << " rows for " << out.effect
                  << " in " << spec.target_id;
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
          out.message = message.str();
          out.node_support_summary = std::move(filtered_summary);
          out.node_support_summary_available = true;
          assign_warning_threshold_relation(out, /*triggered=*/false);
          result.warning_results.push_back(std::move(out));
          continue;
        }
        const auto *filtered = &filtered_summary;
        out.node_support_summary = filtered_summary;
        out.node_support_summary_available = true;
        if (warning.kind == "node_support_balance") {
          if (warning.metric == "valid_target_count_gini") {
            out.measured_value = filtered->valid_target_count_gini;
          } else if (warning.metric ==
                     "valid_target_count_normalized_entropy") {
            out.measured_value =
                filtered->valid_target_count_normalized_entropy;
          } else {
            out.measured_value =
                filtered->valid_target_count_coefficient_of_variation;
          }
          out.measurement_available = std::isfinite(out.measured_value);
          const bool triggered =
              std::isfinite(out.threshold) &&
              std::isfinite(out.measured_value) &&
              (entropy_balance ? out.measured_value < out.threshold
                               : out.measured_value > out.threshold);
          std::ostringstream message;
          message << "node-support " << warning.metric;
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          if (std::isfinite(out.measured_value)) {
            message << " is " << out.measured_value;
            if (std::isfinite(out.threshold)) {
              message << threshold_comparison_phrase(
                             triggered,
                             /*use_above_threshold=*/!entropy_balance)
                      << out.threshold;
            }
          } else {
            message << " is unavailable";
            if (std::isfinite(out.threshold)) {
              message << " for threshold " << out.threshold;
            }
          }
          if (!filtered->weakest_valid_target_node_id.empty()) {
            message << "; weakest node "
                    << filtered->weakest_valid_target_node_id
                    << " has valid_target_count "
                    << filtered->weakest_valid_target_count;
          }
          out.message = message.str();
          assign_warning_threshold_relation(out, triggered);
          if (triggered) {
            out.status = "warning";
            out.severity = "warn";
            result.warnings.push_back(out.message);
          }
        } else {
          if (warning.metric == "weakest_valid_target_count") {
            out.measured_value =
                static_cast<double>(filtered->weakest_valid_target_count);
          } else if (warning.metric == "valid_target_wilson_lower_95") {
            out.measured_value = filtered->valid_target_wilson_lower_95;
          } else {
            out.measured_value = filtered->min_valid_target_fraction;
          }
          out.measurement_available = std::isfinite(out.measured_value);
          const bool triggered = std::isfinite(out.threshold) &&
                                 std::isfinite(out.measured_value) &&
                                 out.measured_value < out.threshold;
          std::ostringstream message;
          message << "node-support " << warning.metric;
          if (!out.split.empty()) {
            message << " over " << out.split;
          }
          if (std::isfinite(out.measured_value)) {
            message << " is " << out.measured_value;
            if (std::isfinite(out.threshold)) {
              message << threshold_comparison_phrase(
                             triggered, /*use_above_threshold=*/false)
                      << out.threshold;
            }
          } else {
            message << " is unavailable";
            if (std::isfinite(out.threshold)) {
              message << " for threshold " << out.threshold;
            }
          }
          if (!filtered->weakest_valid_target_node_id.empty()) {
            message << "; weakest node "
                    << filtered->weakest_valid_target_node_id;
          }
          out.message = message.str();
          assign_warning_threshold_relation(out, triggered);
          if (triggered) {
            out.status = "warning";
            out.severity = "warn";
            result.warnings.push_back(out.message);
          }
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "representation_health") {
        out.evidence_basis = "representation_health_facts";
        out.metric = warning.metric;
        out.unit = representation_health_unit(warning.metric);
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        for (const auto &fact : facts) {
          if (!warning_fact_overlaps_anchor_range(spec, warning, fact,
                                                  warning.use)) {
            continue;
          }
          if (warning.require_mutated_component &&
              !fact.use.mutated_component) {
            continue;
          }
          const auto value = representation_health_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          if (!std::isfinite(measured)) {
            measured = *value;
          } else {
            measured = use_above ? std::max(measured, *value)
                                 : std::min(measured, *value);
          }
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary_available = fact_count > 0;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "representation-health " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "runtime_health") {
        out.evidence_basis = "runtime_exposure_facts";
        out.metric = warning.metric;
        out.unit = runtime_health_unit(warning.metric);
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        std::int64_t sample_count = 0;
        bool saw_terminal_fact = false;
        double measured = std::numeric_limits<double>::quiet_NaN();
        for (const auto &fact : facts) {
          if (!warning_fact_overlaps_anchor_range(spec, warning, fact,
                                                  warning.use)) {
            continue;
          }
          if (!exposure_use_set_has(fact.use, warning.use)) {
            continue;
          }
          if (warning.require_mutated_component &&
              !fact.use.mutated_component) {
            continue;
          }
          const auto value = runtime_health_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          sample_count += fact.valid_target_count;
          saw_terminal_fact = saw_terminal_fact ||
                              fact.runtime_result_fact_available ||
                              fact.runtime_checkpoint_io_fact_available ||
                              fact.runtime_health_measurement_fact_available;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        if (saw_terminal_fact) {
          out.evidence_basis = "runtime_terminal_facts";
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary.valid_target_count_total = sample_count;
        out.exposure_summary_available = fact_count > 0;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "runtime-health " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        message << " for " << out.use;
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = runtime_health_metric_is_severe(warning.metric)
                             ? "severe"
                             : "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "source_analytics") {
        out.evidence_basis = "source_analytics_facts";
        out.metric = warning.metric;
        out.unit = source_analytics_unit(warning.metric);
        out.use = "source_observation";
        out.effect = "none";
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        std::int64_t source_receipt_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        const auto range = detail::warning_anchor_interval(spec, warning);
        for (const auto &fact : source_analytics_facts) {
          if (warning.scope == "target_component_family_id" &&
              fact.target_component_family_id != spec.component) {
            continue;
          }
          if (!warning.split.empty() && fact.split_name != warning.split) {
            continue;
          }
          if (!range.empty()) {
            const auto coverage = fact.completed_anchor_range.empty()
                                      ? fact.anchor_range
                                      : fact.completed_anchor_range;
            if (coverage.empty() ||
                exposure::interval_intersection(coverage, range).empty()) {
              continue;
            }
          }
          const auto value = source_analytics_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          source_receipt_count += fact.source_receipt_fact_count;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary_available = fact_count > 0;
        out.diagnostic_sample_count = source_receipt_count;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "source-analytics " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "forecast_baseline") {
        out.evidence_basis = "forecast_baseline_facts";
        out.metric = warning.metric;
        out.unit = forecast_baseline_unit(warning.metric);
        out.use = "evaluation_baseline";
        out.effect = "none";
        out.diagnostic_metric_family =
            forecast_baseline_metric_family(warning.metric);
        out.diagnostic_uncertainty_method =
            forecast_baseline_uncertainty_method(warning.metric);
        out.diagnostic_split_policy_fingerprint =
            result.split_policy_fingerprint;
        out.diagnostic_active_contract_fingerprint =
            options_.active_identity.protocol_contract_fingerprint;
        out.diagnostic_active_graph_order_fingerprint =
            options_.active_identity.graph_order_fingerprint;
        out.diagnostic_active_source_cursor_token =
            options_.active_identity.source_cursor_token;
        out.diagnostic_validation_split =
            warning.split.empty()
                ? (spec.forbid_split.empty() ? spec.train_split
                                             : spec.forbid_split)
                : warning.split;
        out.diagnostic_validation_split_bound =
            spec.target_class == "evaluation_readiness" ||
            spec.target_class == "artifact_readiness" ||
            out.diagnostic_validation_split.find("validation") !=
                std::string::npos;
        out.diagnostic_readiness_effect = "non_blocking_warning_only";
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        std::int64_t sample_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        std::set<std::string> baseline_kinds;
        const bool baseline_kind_count_metric =
            warning.metric == "baseline_kind_count";
        const bool baseline_metric_coverage_count =
            warning.metric == "computed_metric_fact_count" ||
            warning.metric == "partial_metric_fact_count" ||
            warning.metric == "deferred_metric_fact_count" ||
            warning.metric == "computed_metric_value_count" ||
            warning.metric == "metric_status_mismatch_count";
        std::int64_t baseline_metric_coverage_value = 0;
        const auto range = detail::warning_anchor_interval(spec, warning);
        for (const auto &fact : forecast_baseline_facts) {
          if (warning.scope == "target_component_family_id" &&
              fact.target_component_family_id != spec.component) {
            continue;
          }
          if (!warning.split.empty() && fact.split_name != warning.split) {
            continue;
          }
          if (!range.empty()) {
            const auto coverage = fact.completed_anchor_range.empty()
                                      ? fact.anchor_range
                                      : fact.completed_anchor_range;
            if (coverage.empty() ||
                exposure::interval_intersection(coverage, range).empty()) {
              continue;
            }
          }
          if (baseline_kind_count_metric) {
            ++fact_count;
            sample_count +=
                fact.valid_count > 0 ? fact.valid_count : fact.support_count;
            const auto baseline_kind =
                exposure::normalized_forecast_baseline_kind(fact.baseline_kind);
            if (!baseline_kind.empty()) {
              baseline_kinds.insert(baseline_kind);
            }
            continue;
          }
          if (baseline_metric_coverage_count) {
            ++fact_count;
            sample_count +=
                fact.valid_count > 0 ? fact.valid_count : fact.support_count;
            const auto finite_metric_count =
                exposure::forecast_baseline_finite_metric_count(fact);
            const auto metric_status =
                exposure::normalized_forecast_baseline_metric_status(
                    fact.metric_status);
            if (warning.metric == "computed_metric_fact_count" &&
                metric_status == "computed") {
              ++baseline_metric_coverage_value;
            } else if (warning.metric == "partial_metric_fact_count" &&
                       metric_status == "partial") {
              ++baseline_metric_coverage_value;
            } else if (warning.metric == "deferred_metric_fact_count" &&
                       metric_status == "deferred_v1") {
              ++baseline_metric_coverage_value;
            } else if (warning.metric == "computed_metric_value_count") {
              baseline_metric_coverage_value += finite_metric_count;
            } else if (warning.metric == "metric_status_mismatch_count") {
              if (metric_status.empty() ||
                  (metric_status == "computed" && finite_metric_count != 5) ||
                  (metric_status == "partial" && finite_metric_count <= 0) ||
                  (metric_status == "deferred_v1" && finite_metric_count > 0) ||
                  (metric_status != "computed" && metric_status != "partial" &&
                   metric_status != "deferred_v1")) {
                ++baseline_metric_coverage_value;
              }
            }
            continue;
          }
          const auto value = forecast_baseline_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          sample_count +=
              fact.valid_count > 0 ? fact.valid_count : fact.support_count;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        if (baseline_kind_count_metric && fact_count > 0) {
          measured = static_cast<double>(baseline_kinds.size());
        }
        if (baseline_metric_coverage_count && fact_count > 0) {
          measured = static_cast<double>(baseline_metric_coverage_value);
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary.valid_target_count_total = sample_count;
        out.exposure_summary_available = fact_count > 0;
        out.diagnostic_sample_count = sample_count;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "forecast-baseline " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "forecast_eval") {
        out.evidence_basis = "forecast_eval_facts";
        out.metric = warning.metric;
        out.unit = forecast_eval_unit(warning.metric);
        out.use = "evaluation_metric";
        out.effect = "none";
        out.diagnostic_metric_family =
            forecast_eval_metric_family(warning.metric);
        out.diagnostic_uncertainty_method =
            forecast_eval_uncertainty_method(warning.metric);
        out.diagnostic_split_policy_fingerprint =
            result.split_policy_fingerprint;
        out.diagnostic_active_contract_fingerprint =
            options_.active_identity.protocol_contract_fingerprint;
        out.diagnostic_active_graph_order_fingerprint =
            options_.active_identity.graph_order_fingerprint;
        out.diagnostic_active_source_cursor_token =
            options_.active_identity.source_cursor_token;
        out.diagnostic_validation_split =
            warning.split.empty()
                ? (spec.forbid_split.empty() ? spec.train_split
                                             : spec.forbid_split)
                : warning.split;
        out.diagnostic_validation_split_bound =
            spec.target_class == "evaluation_readiness" ||
            spec.target_class == "artifact_readiness" ||
            out.diagnostic_validation_split.find("validation") !=
                std::string::npos;
        out.diagnostic_readiness_effect = "non_blocking_warning_only";
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        std::int64_t sample_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        const auto range = detail::warning_anchor_interval(spec, warning);
        for (const auto &fact : forecast_eval_facts) {
          if (warning.scope == "target_component_family_id" &&
              fact.target_component_family_id != spec.component) {
            continue;
          }
          if (!warning.split.empty() && fact.split_name != warning.split) {
            continue;
          }
          if (!range.empty()) {
            const auto coverage = fact.completed_anchor_range.empty()
                                      ? fact.anchor_range
                                      : fact.completed_anchor_range;
            if (coverage.empty() ||
                exposure::interval_intersection(coverage, range).empty()) {
              continue;
            }
          }
          auto value = forecast_eval_metric(fact, warning.metric);
          if (!value.has_value()) {
            value = forecast_eval_baseline_metric(fact, forecast_baseline_facts,
                                                  warning.metric);
          }
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          sample_count +=
              fact.valid_count > 0 ? fact.valid_count : fact.support_count;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary.valid_target_count_total = sample_count;
        out.exposure_summary_available = fact_count > 0;
        out.diagnostic_sample_count = sample_count;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "forecast-eval " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "observer_belief") {
        out.evidence_basis = "observer_belief_facts";
        out.metric = warning.metric;
        out.unit = observer_belief_unit(warning.metric);
        out.use = "observer_belief";
        out.effect = "none";
        out.diagnostic_metric_family =
            observer_belief_metric_family(warning.metric);
        out.diagnostic_uncertainty_method =
            observer_belief_uncertainty_method(warning.metric);
        out.diagnostic_split_policy_fingerprint =
            result.split_policy_fingerprint;
        out.diagnostic_active_contract_fingerprint =
            options_.active_identity.protocol_contract_fingerprint;
        out.diagnostic_active_graph_order_fingerprint =
            options_.active_identity.graph_order_fingerprint;
        out.diagnostic_active_source_cursor_token =
            options_.active_identity.source_cursor_token;
        out.diagnostic_validation_split =
            warning.split.empty()
                ? (spec.forbid_split.empty() ? spec.train_split
                                             : spec.forbid_split)
                : warning.split;
        out.diagnostic_validation_split_bound =
            spec.target_class == "artifact_readiness" ||
            out.diagnostic_validation_split.find("validation") !=
                std::string::npos;
        out.diagnostic_readiness_effect = "non_blocking_warning_only";
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        const auto range = detail::warning_anchor_interval(spec, warning);
        for (const auto &fact : observer_belief_facts) {
          if (warning.scope == "target_component_family_id" &&
              fact.target_component_family_id != spec.component) {
            continue;
          }
          if (!warning.split.empty() && fact.split_name != warning.split) {
            continue;
          }
          if (!range.empty()) {
            const auto coverage = fact.completed_anchor_range.empty()
                                      ? fact.anchor_range
                                      : fact.completed_anchor_range;
            if (coverage.empty() ||
                exposure::interval_intersection(coverage, range).empty()) {
              continue;
            }
          }
          const auto value = observer_belief_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary_available = fact_count > 0;
        out.diagnostic_sample_count = fact_count;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "observer-belief " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = warning.metric == "raw_potential_tradable_return" ||
                                 warning.metric == "allocation_authority"
                             ? "severe"
                             : "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "allocation_engine") {
        out.evidence_basis = "allocation_engine_facts";
        out.metric = warning.metric;
        out.unit = allocation_engine_unit(warning.metric);
        out.use = "allocation_engine";
        out.effect = "none";
        out.diagnostic_metric_family =
            allocation_engine_metric_family(warning.metric);
        out.diagnostic_uncertainty_method =
            allocation_engine_uncertainty_method(warning.metric);
        out.diagnostic_split_policy_fingerprint =
            result.split_policy_fingerprint;
        out.diagnostic_active_contract_fingerprint =
            options_.active_identity.protocol_contract_fingerprint;
        out.diagnostic_active_graph_order_fingerprint =
            options_.active_identity.graph_order_fingerprint;
        out.diagnostic_active_source_cursor_token =
            options_.active_identity.source_cursor_token;
        out.diagnostic_validation_split =
            warning.split.empty()
                ? (spec.forbid_split.empty() ? spec.train_split
                                             : spec.forbid_split)
                : warning.split;
        out.diagnostic_validation_split_bound =
            spec.target_class == "artifact_readiness" ||
            out.diagnostic_validation_split.find("validation") !=
                std::string::npos;
        out.diagnostic_readiness_effect = "non_blocking_warning_only";
        const bool use_above = std::isfinite(warning.above);
        out.threshold = use_above ? warning.above : warning.below;
        out.threshold_direction = use_above ? "above" : "below";
        std::int64_t fact_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        const auto range = detail::warning_anchor_interval(spec, warning);
        for (const auto &fact : allocation_engine_facts) {
          if (warning.scope == "target_component_family_id" &&
              fact.target_component_family_id != spec.component) {
            continue;
          }
          if (!warning.split.empty() && fact.split_name != warning.split) {
            continue;
          }
          if (!range.empty()) {
            const auto coverage = fact.completed_anchor_range.empty()
                                      ? fact.anchor_range
                                      : fact.completed_anchor_range;
            if (coverage.empty() ||
                exposure::interval_intersection(coverage, range).empty()) {
              continue;
            }
          }
          const auto value = allocation_engine_metric(fact, warning.metric);
          if (!value.has_value() || !std::isfinite(*value)) {
            continue;
          }
          ++fact_count;
          measured = std::isfinite(measured)
                         ? (use_above ? std::max(measured, *value)
                                      : std::min(measured, *value))
                         : *value;
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary_available = fact_count > 0;
        out.diagnostic_sample_count = fact_count;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               (use_above ? out.measured_value > out.threshold
                                          : out.measured_value < out.threshold);
        std::ostringstream message;
        message << "allocation-engine " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered, use_above)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity =
              warning.metric == "allocation_authority" ||
                      warning.metric == "execution_authority" ||
                      warning.metric == "market_readiness_authority" ||
                      warning.metric == "deployment_authority"
                  ? "severe"
                  : "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "mdn_distribution_calibration") {
        out.evidence_basis = "mdn_distribution_facts";
        out.metric = warning.metric;
        out.unit = mdn_distribution_metric_unit(warning.metric);
        out.threshold = warning.above;
        out.threshold_direction = "above";
        out.diagnostic_metric_family =
            mdn_distribution_metric_family(warning.metric);
        out.diagnostic_uncertainty_method =
            mdn_distribution_uncertainty_method(warning.metric);
        out.diagnostic_evaluated_mdn_checkpoint =
            result.evidence.mdn_checkpoint_loaded
                ? result.evidence.mdn_checkpoint_path
                : result.evidence.checkpoint_path;
        out.diagnostic_representation_checkpoint =
            result.evidence.representation_checkpoint_path;
        out.diagnostic_mdn_checkpoint_loaded =
            result.evidence.mdn_checkpoint_loaded;
        out.diagnostic_representation_checkpoint_loaded =
            result.evidence.representation_checkpoint_loaded;
        out.diagnostic_split_policy_fingerprint =
            result.split_policy_fingerprint;
        out.diagnostic_active_contract_fingerprint =
            options_.active_identity.protocol_contract_fingerprint;
        out.diagnostic_active_graph_order_fingerprint =
            options_.active_identity.graph_order_fingerprint;
        out.diagnostic_active_source_cursor_token =
            options_.active_identity.source_cursor_token;
        out.diagnostic_validation_split =
            warning.split.empty()
                ? (spec.forbid_split.empty() ? spec.train_split
                                             : spec.forbid_split)
                : warning.split;
        out.diagnostic_validation_split_bound =
            spec.target_class == "evaluation_readiness" ||
            out.diagnostic_validation_split.find("validation") !=
                std::string::npos;
        out.diagnostic_readiness_effect = "non_blocking_warning_only";
        out.diagnostic_exact_checkpoint_binding_proven = std::any_of(
            result.proof_certificate.dependencies.begin(),
            result.proof_certificate.dependencies.end(),
            [](const lattice_target_proof_certificate_t::dependency_proof_t
                   &proof) {
              return proof.kind == "evaluated_checkpoint_source" &&
                     proof.mdn_checkpoint_match &&
                     proof.representation_checkpoint_match;
            });
        std::int64_t fact_count = 0;
        std::int64_t sample_count = 0;
        double measured = std::numeric_limits<double>::quiet_NaN();
        if (warning.metric == "per_node_mean_nll_max") {
          out.evidence_basis = "filtered_node_exposure_facts";
          const auto range = detail::warning_anchor_interval(spec, warning);
          for (const auto &fact : node_facts) {
            if (fact.target_component_family_id != spec.component) {
              continue;
            }
            if (!warning.split.empty() && fact.split_name != warning.split) {
              continue;
            }
            if (!range.empty()) {
              const auto coverage = fact.completed_anchor_range.empty()
                                        ? fact.anchor_range
                                        : fact.completed_anchor_range;
              if (coverage.empty() ||
                  exposure::interval_intersection(coverage, range).empty()) {
                continue;
              }
            }
            if (!exposure_use_set_has(fact.use, warning.use)) {
              continue;
            }
            if (warning.require_mutated_component &&
                !fact.use.mutated_component) {
              continue;
            }
            if (!std::isfinite(fact.mean_nll)) {
              continue;
            }
            ++fact_count;
            sample_count += fact.valid_target_count;
            measured = std::isfinite(measured)
                           ? std::max(measured, fact.mean_nll)
                           : fact.mean_nll;
          }
        } else {
          for (const auto &fact : facts) {
            if (!warning.split.empty() && fact.split_name != warning.split) {
              continue;
            }
            if (!warning_fact_overlaps_anchor_range(spec, warning, fact,
                                                    warning.use)) {
              continue;
            }
            if (!exposure_use_set_has(fact.use, warning.use)) {
              continue;
            }
            if (warning.require_mutated_component &&
                !fact.use.mutated_component) {
              continue;
            }
            const auto value = mdn_distribution_metric(fact, warning.metric);
            if (!value.has_value() || !std::isfinite(*value)) {
              continue;
            }
            ++fact_count;
            sample_count += fact.valid_target_count;
            measured =
                std::isfinite(measured) ? std::max(measured, *value) : *value;
          }
        }
        out.exposure_summary.fact_count = fact_count;
        out.exposure_summary.valid_target_count_total = sample_count;
        out.diagnostic_sample_count = sample_count;
        out.exposure_summary_available = fact_count > 0;
        out.measured_value = measured;
        out.measurement_available = std::isfinite(out.measured_value);
        const bool triggered = std::isfinite(out.threshold) &&
                               std::isfinite(out.measured_value) &&
                               out.measured_value > out.threshold;
        std::ostringstream message;
        message << "MDN distribution " << warning.metric;
        if (!out.split.empty()) {
          message << " over " << out.split;
        }
        message << " for " << out.use;
        if (std::isfinite(out.measured_value)) {
          message << " is " << out.measured_value;
          if (std::isfinite(out.threshold)) {
            message << threshold_comparison_phrase(triggered,
                                                   /*use_above_threshold=*/true)
                    << out.threshold;
          }
        } else {
          message << " is unavailable";
          if (std::isfinite(out.threshold)) {
            message << " for threshold " << out.threshold;
          }
        }
        out.message = message.str();
        assign_warning_threshold_relation(out, triggered);
        if (triggered) {
          out.status = "warning";
          out.severity = "warn";
          result.warnings.push_back(out.message);
        }
        result.warning_results.push_back(std::move(out));
        continue;
      }

      if (warning.kind == "anchor_domain_health") {
        out.evidence_basis = "anchor_domain_facts";
        double min_accepted_fraction = std::numeric_limits<double>::quiet_NaN();
        std::int64_t skipped_failed_fetch_probe_total = 0;
        std::int64_t fact_count = 0;
        for (const auto &fact : facts) {
          if (!warning_fact_overlaps_anchor_range(spec, warning, fact,
                                                  warning.use)) {
            continue;
          }
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
        out.exposure_summary_available = fact_count > 0;
        if (std::isfinite(warning.accepted_fraction_below)) {
          out.measured_value = min_accepted_fraction;
          out.threshold = warning.accepted_fraction_below;
          out.threshold_direction = "below";
          out.unit = "accepted_fraction";
          out.measurement_available = std::isfinite(out.measured_value);
          const bool triggered = std::isfinite(out.measured_value) &&
                                 out.measured_value < out.threshold;
          std::ostringstream message;
          message << "anchor-domain accepted fraction";
          if (std::isfinite(out.measured_value)) {
            message << " is " << out.measured_value
                    << threshold_comparison_phrase(
                           triggered, /*use_above_threshold=*/false)
                    << out.threshold;
          } else {
            message << " is unavailable";
            if (std::isfinite(out.threshold)) {
              message << " for threshold " << out.threshold;
            }
          }
          out.message = message.str();
          assign_warning_threshold_relation(out, triggered);
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
          out.threshold_direction = "above";
          out.unit = "count";
          out.measurement_available = fact_count > 0;
          const bool triggered =
              out.measurement_available && out.measured_value > out.threshold;
          std::ostringstream message;
          message << "anchor-domain failed fetch probes total";
          if (out.measurement_available) {
            message << " " << skipped_failed_fetch_probe_total
                    << threshold_comparison_phrase(triggered,
                                                   /*use_above_threshold=*/true)
                    << *warning.skipped_failed_fetch_probe_above;
          } else {
            message << " is unavailable";
            if (std::isfinite(out.threshold)) {
              message << " for threshold " << out.threshold;
            }
          }
          out.message = message.str();
          assign_warning_threshold_relation(out, triggered);
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
      const std::string &expected_component_fingerprint,
      const std::optional<
          lattice_target_eval_detail::evaluated_checkpoint_binding_t>
          &evaluated_checkpoint_binding = std::nullopt) const {
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
    std::vector<exposure::lattice_exposure_fact_t> closure_exposure_facts;
    const auto record_closure_proof =
        [&](const std::filesystem::path &checkpoint_path,
            const std::string &missing_reason,
            const std::string &unresolved_reason) -> bool {
      const auto closure = ledger->checkpoint_closure_result(checkpoint_path);
      result.proof_certificate.closure.checked = true;
      result.proof_certificate.closure.checkpoint_path = checkpoint_path;
      result.proof_certificate.closure.complete = closure.complete();
      result.proof_certificate.closure.fact_count =
          static_cast<std::int64_t>(closure.facts.size());
      result.proof_certificate.closure.resolution_authority =
          closure.resolution_authority;
      result.proof_certificate.closure.root_checkpoint_id =
          closure.root_checkpoint_id;
      result.proof_certificate.closure.root_checkpoint_file_digest =
          closure.root_checkpoint_file_digest;
      result.proof_certificate.closure.identity_mismatches =
          closure.identity_mismatches;
      result.proof_certificate.closure.unresolved_input_checkpoints =
          closure.unresolved_input_checkpoints;
      result.proof_certificate.closure.fact_digests.clear();
      result.proof_certificate.closure.causal_exposures.clear();
      result.proof_certificate.closure.checkpoint_identity_previews.clear();
      result.proof_certificate.closure.fact_digests.reserve(
          closure.facts.size());
      result.proof_certificate.closure.causal_exposures.reserve(
          closure.facts.size());
      std::set<std::string> checkpoint_preview_paths;
      for (const auto &fact : closure.facts) {
        const auto fact_digest = exposure::exposure_fact_digest(fact);
        result.proof_certificate.closure.fact_digests.push_back(fact_digest);
        lattice_target_proof_certificate_t::closure_proof_t::causal_exposure_t
            causal{};
        causal.fact_digest = fact_digest;
        causal.contract_fingerprint = fact.contract_fingerprint;
        causal.graph_order_fingerprint = fact.graph_order_fingerprint;
        causal.source_cursor_token = fact.source_cursor_token;
        causal.cursor_domain = fact.cursor_domain;
        causal.component_assembly_fingerprint =
            fact.component_assembly_fingerprint;
        causal.split_policy_fingerprint = fact.split_policy_fingerprint;
        causal.job_id = fact.job_id;
        causal.wave_id = fact.wave_id;
        causal.wave_action = fact.wave_action;
        causal.job_status = fact.job_status;
        causal.runtime_handoff_id = fact.runtime_handoff_id;
        causal.runtime_handoff_digest = fact.runtime_handoff_digest;
        causal.marshal_target_driver_run_id = fact.marshal_target_driver_run_id;
        causal.target_component_family_id = fact.target_component_family_id;
        causal.representation_architecture = fact.representation_architecture;
        causal.representation_contract = fact.representation_contract;
        causal.representation_value_shape = fact.representation_value_shape;
        causal.representation_sequence_value_shape =
            fact.representation_sequence_value_shape;
        causal.channel_axis_policy = fact.channel_axis_policy;
        causal.temporal_reduction = fact.temporal_reduction;
        causal.input_representation_assembly_id =
            fact.input_representation_assembly_id;
        causal.context_mode = fact.context_mode;
        causal.context_contract = fact.context_contract;
        causal.context_value_shape = fact.context_value_shape;
        causal.output_contract = fact.output_contract;
        causal.output_value_shape = fact.output_value_shape;
        causal.uses = detail::use_set_names(fact.use);
        causal.mutated_component = fact.use.mutated_component;
        causal.split_name = fact.split_name;
        causal.split_role = fact.split_role;
        causal.anchor_range = fact.anchor_range;
        causal.completed_anchor_range = fact.completed_anchor_range;
        causal.observed_footprint = fact.observed_footprint;
        causal.target_footprint = fact.target_footprint;
        causal.first_anchor_key = fact.first_anchor_key;
        causal.last_anchor_key = fact.last_anchor_key;
        causal.observed_source_key_begin = fact.observed_source_key_begin;
        causal.observed_source_key_end = fact.observed_source_key_end;
        causal.target_source_key_begin = fact.target_source_key_begin;
        causal.target_source_key_end = fact.target_source_key_end;
        causal.source_key_footprint_precision =
            fact.source_key_footprint_precision;
        causal.source_key_window_audit =
            exposure::audit_source_key_window(fact);
        causal.output_checkpoint = fact.output_checkpoint;
        causal.input_checkpoints = fact.input_checkpoints;
        result.proof_certificate.closure.causal_exposures.push_back(
            std::move(causal));
        const auto fact_checkpoint_path =
            fact.output_checkpoint.lexically_normal().string();
        if (fact_checkpoint_path.empty() ||
            !checkpoint_preview_paths.insert(fact_checkpoint_path).second) {
          continue;
        }
        const auto checkpoint_fact = std::find_if(
            ledger->checkpoint_facts().begin(),
            ledger->checkpoint_facts().end(),
            [&](const exposure::lattice_checkpoint_fact_t &candidate) {
              return candidate.direct_exposure_digest == fact_digest;
            });
        if (checkpoint_fact != ledger->checkpoint_facts().end() &&
            !checkpoint_fact->checkpoint_id.empty() &&
            !checkpoint_fact->checkpoint_file_digest.empty()) {
          lattice_target_proof_certificate_t::closure_proof_t::
              checkpoint_identity_preview_t preview{};
          preview.checkpoint_path = checkpoint_fact->checkpoint_path;
          preview.checkpoint_id = checkpoint_fact->checkpoint_id;
          preview.checkpoint_file_digest =
              checkpoint_fact->checkpoint_file_digest;
          preview.component = checkpoint_fact->component;
          preview.component_assembly_fingerprint =
              checkpoint_fact->component_assembly_fingerprint;
          preview.representation_architecture =
              checkpoint_fact->representation_architecture;
          preview.representation_contract =
              checkpoint_fact->representation_contract;
          preview.representation_value_shape =
              checkpoint_fact->representation_value_shape;
          preview.representation_sequence_value_shape =
              checkpoint_fact->representation_sequence_value_shape;
          preview.channel_axis_policy = checkpoint_fact->channel_axis_policy;
          preview.temporal_reduction = checkpoint_fact->temporal_reduction;
          preview.input_representation_assembly_id =
              checkpoint_fact->input_representation_assembly_id;
          preview.context_mode = checkpoint_fact->context_mode;
          preview.context_contract = checkpoint_fact->context_contract;
          preview.context_value_shape = checkpoint_fact->context_value_shape;
          preview.output_contract = checkpoint_fact->output_contract;
          preview.output_value_shape = checkpoint_fact->output_value_shape;
          preview.direct_exposure_digest =
              checkpoint_fact->direct_exposure_digest;
          preview.created_by_job_id = checkpoint_fact->created_by_job_id;
          preview.created_by_wave_id = checkpoint_fact->created_by_wave_id;
          result.proof_certificate.closure.checkpoint_identity_previews
              .push_back(std::move(preview));
        }
      }
      closure_exposure_facts = closure.facts;
      if (closure_exposure_facts.empty()) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(missing_reason + checkpoint_path.string());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      if (!closure.complete()) {
        result.status = lattice_target_status_t::exposure_failed;
        std::ostringstream reason;
        reason << unresolved_reason;
        for (const auto &path : closure.unresolved_input_checkpoints) {
          reason << " " << path.string();
        }
        for (const auto &mismatch : closure.identity_mismatches) {
          reason << " " << mismatch;
        }
        result.reasons.push_back(reason.str());
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      return true;
    };

    if (!result.evidence.checkpoint_path.empty()) {
      if (!record_closure_proof(
              result.evidence.checkpoint_path,
              "exposure ledger has no checkpoint closure facts for ",
              "checkpoint closure has unresolved input checkpoint lineage")) {
        return false;
      }
      exposure_facts = closure_exposure_facts;
    } else {
      for (const auto &fact : ledger->facts()) {
        if (fact.target_component_family_id != spec.component) {
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
      if (evaluated_checkpoint_binding.has_value() &&
          !evaluated_checkpoint_binding->mdn_checkpoint_path.empty()) {
        if (!record_closure_proof(
                evaluated_checkpoint_binding->mdn_checkpoint_path,
                "exposure ledger has no evaluated checkpoint closure facts "
                "for ",
                "evaluated checkpoint closure has unresolved input checkpoint "
                "lineage")) {
          return false;
        }
      }
    }
    exposure_facts = detail::filter_facts_for_active_identity(
        exposure_facts, options_.active_identity, spec,
        expected_component_fingerprint, expected_split_policy_fingerprint);
    if (!closure_exposure_facts.empty()) {
      closure_exposure_facts = detail::filter_facts_for_active_identity(
          closure_exposure_facts, options_.active_identity, spec,
          expected_component_fingerprint, expected_split_policy_fingerprint);
    }
    if (exposure_facts.empty()) {
      result.status = lattice_target_status_t::exposure_failed;
      result.reasons.push_back(
          "exposure ledger has no active graph/source-compatible facts for "
          "this target/checkpoint");
      result.plan_ready = plan_budget_remaining;
      return false;
    }

    if (lattice_target_kind_is_mdn(spec.kind) &&
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
      const bool expect_mtf_representation = spec.protocol_id == "cwu_02v";
      const std::string expected_representation_component =
          expect_mtf_representation
              ? std::string{"wikimyei.representation.encoding.mtf_jepa_mae_"
                            "vicreg"}
              : upstream_representation_component_for_target_kind(spec.kind);
      const std::string expected_representation_fingerprint =
          expect_mtf_representation
              ? options_.active_identity
                    .mtf_jepa_mae_vicreg_assembly_fingerprint
              : options_.active_identity.vicreg_assembly_fingerprint;
      const auto rep_ready_fact =
          std::find_if(rep_facts.begin(), rep_facts.end(),
                       [&](const exposure::lattice_exposure_fact_t &fact) {
                         return fact.target_component_family_id ==
                                    expected_representation_component &&
                                fact.component_assembly_fingerprint ==
                                    expected_representation_fingerprint;
                       });
      if (rep_ready_fact == rep_facts.end()) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            "loaded representation checkpoint has no active compatible "
            "representation producer exposure fact");
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      if (spec.kind == lattice_target_kind_t::channel_mdn_ready) {
        const auto representation_kind =
            expect_mtf_representation
                ? lattice_target_kind_t::mtf_representation_ready
                : lattice_target_kind_t::vicreg_ready;
        if (const auto mismatch =
                detail::strict_channel_semantic_contract_mismatch(
                    *rep_ready_fact, representation_kind)) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back(
              "loaded representation checkpoint semantic contract mismatch: " +
              *mismatch);
          result.plan_ready = plan_budget_remaining;
          return false;
        }
        const auto health_mismatch =
            expect_mtf_representation
                ? detail::strict_mtf_representation_health_mismatch(
                      *rep_ready_fact)
                : detail::strict_channel_representation_health_mismatch(
                      *rep_ready_fact);
        if (const auto mismatch = health_mismatch) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back(
              "loaded representation checkpoint health mismatch: " + *mismatch);
          result.plan_ready = plan_budget_remaining;
          return false;
        }
      }
    }

    std::vector<exposure::lattice_exposure_fact_t>
        target_component_family_facts;
    for (const auto &fact : exposure_facts) {
      if (fact.target_component_family_id == spec.component) {
        target_component_family_facts.push_back(fact);
      }
    }
    std::vector<exposure::lattice_source_analytics_fact_t>
        source_analytics_facts;
    for (const auto &fact : ledger->source_analytics_facts()) {
      if (spec.require_contract_match &&
          fact.contract_fingerprint !=
              options_.active_identity.protocol_contract_fingerprint) {
        continue;
      }
      if (!options_.active_identity.protocol_id.empty() &&
          fact.protocol_id != options_.active_identity.protocol_id) {
        continue;
      }
      if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
        continue;
      }
      if (detail::target_requires_graph_anchor_identity(spec)) {
        if (fact.graph_order_fingerprint !=
            options_.active_identity.graph_order_fingerprint) {
          continue;
        }
        if (fact.source_cursor_token !=
            options_.active_identity.source_cursor_token) {
          continue;
        }
      }
      if (spec.require_component_match &&
          fact.target_component_family_id == spec.component &&
          fact.component_assembly_fingerprint !=
              expected_component_fingerprint) {
        continue;
      }
      if (!expected_split_policy_fingerprint.empty()) {
        if (fact.split_policy_fingerprint.empty()) {
          if (certificate_names_concrete_split(fact.split_name)) {
            continue;
          }
        } else if (fact.split_policy_fingerprint !=
                   expected_split_policy_fingerprint) {
          continue;
        }
      }
      source_analytics_facts.push_back(fact);
    }
    std::vector<exposure::lattice_forecast_baseline_fact_t>
        forecast_baseline_facts;
    for (const auto &fact : ledger->forecast_baseline_facts()) {
      if (spec.require_contract_match &&
          fact.contract_fingerprint !=
              options_.active_identity.protocol_contract_fingerprint) {
        continue;
      }
      if (!options_.active_identity.protocol_id.empty() &&
          fact.protocol_id != options_.active_identity.protocol_id) {
        continue;
      }
      if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
        continue;
      }
      if (detail::target_requires_graph_anchor_identity(spec)) {
        if (fact.graph_order_fingerprint !=
            options_.active_identity.graph_order_fingerprint) {
          continue;
        }
        if (fact.source_cursor_token !=
            options_.active_identity.source_cursor_token) {
          continue;
        }
      }
      if (spec.require_component_match &&
          fact.target_component_family_id == spec.component &&
          fact.component_assembly_fingerprint !=
              expected_component_fingerprint) {
        continue;
      }
      if (!expected_split_policy_fingerprint.empty()) {
        if (fact.split_policy_fingerprint.empty()) {
          if (certificate_names_concrete_split(fact.split_name)) {
            continue;
          }
        } else if (fact.split_policy_fingerprint !=
                   expected_split_policy_fingerprint) {
          continue;
        }
      }
      forecast_baseline_facts.push_back(fact);
    }
    std::vector<exposure::lattice_forecast_eval_fact_t> forecast_eval_facts;
    for (const auto &fact : ledger->forecast_eval_facts()) {
      if (spec.require_contract_match &&
          fact.contract_fingerprint !=
              options_.active_identity.protocol_contract_fingerprint) {
        continue;
      }
      if (!options_.active_identity.protocol_id.empty() &&
          fact.protocol_id != options_.active_identity.protocol_id) {
        continue;
      }
      if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
        continue;
      }
      if (detail::target_requires_graph_anchor_identity(spec)) {
        if (fact.graph_order_fingerprint !=
            options_.active_identity.graph_order_fingerprint) {
          continue;
        }
        if (fact.source_cursor_token !=
            options_.active_identity.source_cursor_token) {
          continue;
        }
      }
      if (spec.require_component_match &&
          fact.target_component_family_id == spec.component &&
          fact.component_assembly_fingerprint !=
              expected_component_fingerprint) {
        continue;
      }
      if (!expected_split_policy_fingerprint.empty()) {
        if (fact.split_policy_fingerprint.empty()) {
          if (certificate_names_concrete_split(fact.split_name)) {
            continue;
          }
        } else if (fact.split_policy_fingerprint !=
                   expected_split_policy_fingerprint) {
          continue;
        }
      }
      forecast_eval_facts.push_back(fact);
    }
    std::vector<exposure::lattice_observer_belief_fact_t> observer_belief_facts;
    for (const auto &fact : ledger->observer_belief_facts()) {
      if (spec.require_contract_match &&
          fact.contract_fingerprint !=
              options_.active_identity.protocol_contract_fingerprint) {
        continue;
      }
      if (!options_.active_identity.protocol_id.empty() &&
          fact.protocol_id != options_.active_identity.protocol_id) {
        continue;
      }
      if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
        continue;
      }
      if (detail::target_requires_graph_anchor_identity(spec)) {
        if (fact.graph_order_fingerprint !=
            options_.active_identity.graph_order_fingerprint) {
          continue;
        }
        if (fact.source_cursor_token !=
            options_.active_identity.source_cursor_token) {
          continue;
        }
      }
      if (spec.require_component_match &&
          fact.target_component_family_id == spec.component &&
          fact.component_assembly_fingerprint !=
              expected_component_fingerprint) {
        continue;
      }
      if (!expected_split_policy_fingerprint.empty()) {
        if (fact.split_policy_fingerprint.empty()) {
          if (certificate_names_concrete_split(fact.split_name)) {
            continue;
          }
        } else if (fact.split_policy_fingerprint !=
                   expected_split_policy_fingerprint) {
          continue;
        }
      }
      observer_belief_facts.push_back(fact);
    }
    std::vector<exposure::lattice_allocation_engine_fact_t>
        allocation_engine_facts;
    for (const auto &fact : ledger->allocation_engine_facts()) {
      if (spec.require_contract_match &&
          fact.contract_fingerprint !=
              options_.active_identity.protocol_contract_fingerprint) {
        continue;
      }
      if (!options_.active_identity.protocol_id.empty() &&
          fact.protocol_id != options_.active_identity.protocol_id) {
        continue;
      }
      if (!spec.protocol_id.empty() && fact.protocol_id != spec.protocol_id) {
        continue;
      }
      if (detail::target_requires_graph_anchor_identity(spec)) {
        if (fact.graph_order_fingerprint !=
            options_.active_identity.graph_order_fingerprint) {
          continue;
        }
        if (fact.source_cursor_token !=
            options_.active_identity.source_cursor_token) {
          continue;
        }
      }
      if (spec.require_component_match &&
          fact.target_component_family_id == spec.component &&
          fact.component_assembly_fingerprint !=
              expected_component_fingerprint) {
        continue;
      }
      if (!expected_split_policy_fingerprint.empty()) {
        if (fact.split_policy_fingerprint.empty()) {
          if (certificate_names_concrete_split(fact.split_name)) {
            continue;
          }
        } else if (fact.split_policy_fingerprint !=
                   expected_split_policy_fingerprint) {
          continue;
        }
      }
      allocation_engine_facts.push_back(fact);
    }
    if (target_component_family_facts.empty()) {
      result.status = lattice_target_status_t::exposure_failed;
      result.reasons.push_back(
          "checkpoint closure has no target-component-local exposure facts");
      result.plan_ready = plan_budget_remaining;
      return false;
    }
    for (const auto &fact : target_component_family_facts) {
      if (const auto mismatch =
              detail::strict_channel_semantic_contract_mismatch(fact,
                                                                spec.kind)) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back("target semantic contract mismatch: " +
                                 *mismatch);
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      if (spec.kind == lattice_target_kind_t::vicreg_ready) {
        if (const auto mismatch =
                detail::strict_channel_representation_health_mismatch(fact)) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back("target representation health mismatch: " +
                                   *mismatch);
          result.plan_ready = plan_budget_remaining;
          return false;
        }
      }
      if (spec.kind == lattice_target_kind_t::mtf_representation_ready) {
        if (const auto mismatch =
                detail::strict_mtf_representation_health_mismatch(fact)) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back(
              "target MTF representation health mismatch: " + *mismatch);
          result.plan_ready = plan_budget_remaining;
          return false;
        }
      }
      if (spec.kind == lattice_target_kind_t::channel_mdn_ready) {
        if (const auto mismatch =
                detail::strict_channel_mdn_health_mismatch(fact)) {
          result.status = lattice_target_status_t::exposure_failed;
          result.reasons.push_back("target channel MDN health mismatch: " +
                                   *mismatch);
          result.plan_ready = plan_budget_remaining;
          return false;
        }
      }
    }
    if (!evaluate_representation_geometry_requirements(
            spec, result, plan_budget_remaining,
            target_component_family_facts)) {
      return false;
    }

    const auto target_range = detail::target_anchor_interval(spec);
    std::vector<exposure::lattice_node_exposure_fact_t> node_facts;
    evaluate_warning_specs(
        spec, result, exposure_facts, target_component_family_facts, node_facts,
        source_analytics_facts, forecast_baseline_facts, forecast_eval_facts,
        observer_belief_facts, allocation_engine_facts,
        result.node_support_summaries);
    if (spec.min_observed_input_coverage > 0.0) {
      auto summary = exposure::exposure_load_for_use(
          target_component_family_facts, target_range,
          exposure::exposure_use_t::observed_input,
          /*require_mutated_component=*/true, spec.component);
      result.exposure_summaries.push_back(summary);
      const auto coverage = exposure::coverage_for_use(
          target_component_family_facts, target_range,
          exposure::exposure_use_t::observed_input,
          /*require_mutated_component=*/true);
      lattice_target_proof_certificate_t::coverage_proof_t proof{};
      proof.use = "observed_input";
      proof.target_range = target_range;
      proof.contributing_intervals = coverage.contributing_intervals;
      proof.contributing_fact_digests = coverage.contributing_fact_digests;
      proof.contributing_facts = coverage.contributing_facts;
      proof.covered_intervals = coverage.covered_intervals;
      proof.missing_intervals = coverage.missing_intervals;
      proof.required_fraction = spec.min_observed_input_coverage;
      proof.actual_fraction = coverage.coverage_fraction;
      proof.covered_anchors = coverage.covered_anchors;
      proof.missing_anchors = coverage.missing_anchors;
      proof.require_mutated_component = true;
      proof.passed = coverage.coverage_fraction + 1e-12 >=
                     spec.min_observed_input_coverage;
      proof.load_summary = summary;
      result.proof_certificate.coverage.push_back(std::move(proof));
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
      auto summary = exposure::exposure_load_for_use(
          target_component_family_facts, target_range,
          exposure::exposure_use_t::target_supervision,
          /*require_mutated_component=*/true, spec.component);
      result.exposure_summaries.push_back(summary);
      const auto coverage = exposure::coverage_for_use(
          target_component_family_facts, target_range,
          exposure::exposure_use_t::target_supervision,
          /*require_mutated_component=*/true);
      lattice_target_proof_certificate_t::coverage_proof_t proof{};
      proof.use = "target_supervision";
      proof.target_range = target_range;
      proof.contributing_intervals = coverage.contributing_intervals;
      proof.contributing_fact_digests = coverage.contributing_fact_digests;
      proof.contributing_facts = coverage.contributing_facts;
      proof.covered_intervals = coverage.covered_intervals;
      proof.missing_intervals = coverage.missing_intervals;
      proof.required_fraction = spec.min_target_supervision_coverage;
      proof.actual_fraction = coverage.coverage_fraction;
      proof.covered_anchors = coverage.covered_anchors;
      proof.missing_anchors = coverage.missing_anchors;
      proof.require_mutated_component = true;
      proof.passed = coverage.coverage_fraction + 1e-12 >=
                     spec.min_target_supervision_coverage;
      proof.load_summary = summary;
      result.proof_certificate.coverage.push_back(std::move(proof));
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
      std::vector<exposure::lattice_exposure_fact_t> evaluation_metric_facts;
      evaluation_metric_facts.reserve(target_component_family_facts.size());
      bool saw_mutating_evaluation_metric_fact = false;
      bool saw_output_checkpoint_evaluation_metric_fact = false;
      bool saw_bad_input_checkpoint_evaluation_metric_fact = false;
      for (const auto &fact : target_component_family_facts) {
        const bool evaluation_metric_overlaps_target =
            fact.use.evaluation_metric &&
            !exposure::interval_intersection(
                 exposure::anchor_coverage_for_use(
                     fact, exposure::exposure_use_t::evaluation_metric),
                 target_range)
                 .empty();
        if (evaluation_metric_overlaps_target &&
            (fact.use.mutated_component || fact.optimizer_steps != 0)) {
          saw_mutating_evaluation_metric_fact = true;
        }
        if (evaluation_metric_overlaps_target &&
            !fact.output_checkpoint.empty()) {
          saw_output_checkpoint_evaluation_metric_fact = true;
        }
        if (fact.use.mutated_component || fact.optimizer_steps != 0 ||
            (fact.use.evaluation_metric && !fact.output_checkpoint.empty())) {
          continue;
        }
        if (evaluated_checkpoint_binding.has_value() &&
            evaluation_metric_overlaps_target &&
            !detail::evaluation_fact_has_exact_model_state_inputs(
                fact, *evaluated_checkpoint_binding)) {
          saw_bad_input_checkpoint_evaluation_metric_fact = true;
          continue;
        }
        evaluation_metric_facts.push_back(fact);
      }
      auto summary = exposure::exposure_load_for_use(
          evaluation_metric_facts, target_range,
          exposure::exposure_use_t::evaluation_metric,
          /*require_mutated_component=*/false, spec.component);
      result.exposure_summaries.push_back(summary);
      const auto coverage = exposure::coverage_for_use(
          evaluation_metric_facts, target_range,
          exposure::exposure_use_t::evaluation_metric,
          /*require_mutated_component=*/false);
      lattice_target_proof_certificate_t::coverage_proof_t proof{};
      proof.use = "evaluation_metric";
      proof.target_range = target_range;
      proof.contributing_intervals = coverage.contributing_intervals;
      proof.contributing_fact_digests = coverage.contributing_fact_digests;
      proof.contributing_facts = coverage.contributing_facts;
      proof.covered_intervals = coverage.covered_intervals;
      proof.missing_intervals = coverage.missing_intervals;
      proof.required_fraction = spec.min_evaluation_metric_coverage;
      proof.actual_fraction = coverage.coverage_fraction;
      proof.covered_anchors = coverage.covered_anchors;
      proof.missing_anchors = coverage.missing_anchors;
      proof.require_mutated_component = false;
      proof.passed = coverage.coverage_fraction + 1e-12 >=
                     spec.min_evaluation_metric_coverage;
      proof.load_summary = summary;
      result.proof_certificate.coverage.push_back(std::move(proof));
      if (saw_mutating_evaluation_metric_fact) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            k_lattice_non_mutating_validation_model_state_reason);
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      if (saw_output_checkpoint_evaluation_metric_fact) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            k_lattice_read_only_validation_model_state_reason);
        result.plan_ready = plan_budget_remaining;
        return false;
      }
      if (saw_bad_input_checkpoint_evaluation_metric_fact) {
        result.status = lattice_target_status_t::exposure_failed;
        result.reasons.push_back(
            k_lattice_exact_validation_model_state_input_reason);
        result.plan_ready = plan_budget_remaining;
        return false;
      }
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
    const auto base_forbidden_range = detail::forbidden_exposure_interval(spec);
    const auto forbidden_range =
        options_.split_policy.has_value()
            ? detail::expanded_forbidden_exposure_interval(
                  spec, *options_.split_policy, exposure_facts)
            : base_forbidden_range;
    if (!forbidden_range.empty()) {
      const auto &forbidden_facts = result.proof_certificate.closure.checked
                                        ? closure_exposure_facts
                                        : exposure_facts;
      const exposure::forbidden_exposure_query_t query{
          .forbidden_range = forbidden_range,
          .forbidden_uses = spec.forbid_exposure_uses,
          .require_mutated_component =
              spec.forbid_exposure_requires_mutated_component,
      };
      const auto overlap_witnesses =
          exposure::forbidden_exposure_overlaps(forbidden_facts, query);
      const bool overlap = !overlap_witnesses.empty();
      result.proof_certificate.leakage.checked = true;
      result.proof_certificate.leakage.split_name = spec.forbid_split;
      result.proof_certificate.leakage.base_range =
          base_forbidden_range.empty() ? forbidden_range : base_forbidden_range;
      result.proof_certificate.leakage.protected_range = forbidden_range;
      if (!result.proof_certificate.leakage.base_range.empty()) {
        result.proof_certificate.leakage.dilation_left = std::max<std::int64_t>(
            0, result.proof_certificate.leakage.base_range.begin -
                   forbidden_range.begin);
        result.proof_certificate.leakage.dilation_right =
            std::max<std::int64_t>(
                0, forbidden_range.end -
                       result.proof_certificate.leakage.base_range.end);
      }
      result.proof_certificate.leakage.forbidden_uses =
          detail::forbidden_use_names(spec.forbid_exposure_uses);
      result.proof_certificate.leakage.require_mutated_component =
          spec.forbid_exposure_requires_mutated_component;
      result.proof_certificate.leakage.overlap_found = overlap;
      result.proof_certificate.leakage.overlap_witnesses = overlap_witnesses;
      result.proof_certificate.leakage.passed = !overlap;
      if (overlap) {
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

[[nodiscard]] inline std::vector<lattice_policy_gate_spec_t>
load_lattice_policy_gates_from_file(const std::filesystem::path &path) {
  return decode_lattice_policy_gates_from_dsl(
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

[[nodiscard]] inline std::vector<lattice_policy_gate_spec_t>
load_lattice_policy_gates_from_config(
    const std::filesystem::path &config_path = "/cuwacunu/src/config/.config") {
  const auto paths = resolve_lattice_target_config_paths(config_path);
  (void)lattice_target_eval_detail::read_text_file_or_throw(
      paths.targets_dsl_bnf_path);
  return load_lattice_policy_gates_from_file(paths.targets_dsl_path);
}

} // namespace cuwacunu::kikijyeba::lattice::target
