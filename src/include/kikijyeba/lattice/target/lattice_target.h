// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::lattice::target {

enum class lattice_target_kind_t {
  representation_ready,
  node_mdn_ready,
};

enum class lattice_target_status_t {
  satisfied,
  missing_report,
  stale_contract,
  stale_component,
  missing_checkpoint,
  metric_failed,
  exposure_failed,
  blocked,
};

struct lattice_warning_spec_t {
  std::string warning_id{};
  std::string kind{};
  std::string split{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  cuwacunu::kikijyeba::lattice::exposure::exposure_use_t use{
      cuwacunu::kikijyeba::lattice::exposure::exposure_use_t::observed_input};
  std::string scope{"target_component"};
  bool require_mutated_component{true};
  double cursor_epochs_above{std::numeric_limits<double>::quiet_NaN()};
  std::string metric{};
  double above{std::numeric_limits<double>::quiet_NaN()};
  double accepted_fraction_below{std::numeric_limits<double>::quiet_NaN()};
  std::optional<std::int64_t> skipped_failed_fetch_probe_above{std::nullopt};
};

struct lattice_target_spec_t {
  std::string target_id{};
  std::string use_profile{};
  std::string guard_id{};
  std::string target_class{"readiness"};
  lattice_target_kind_t kind{lattice_target_kind_t::node_mdn_ready};
  std::string component{};
  std::string checkpoint_source{"output_checkpoint"};
  std::string evaluated_checkpoint_source{};
  std::string source_range{"all"};
  std::string train_split{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::string upstream_target_id{};
  bool require_contract_match{true};
  bool require_component_match{true};
  bool require_checkpoint_exists{true};
  bool require_finite_loss{true};
  std::int64_t min_optimizer_steps{0};
  double min_valid_target_fraction{0.0};
  std::int64_t require_active_node_head_count{0};
  std::int64_t require_trained_node_head_count{0};
  std::int64_t require_evaluated_node_head_count{0};
  double min_observed_input_coverage{0.0};
  double min_target_supervision_coverage{0.0};
  double min_evaluation_metric_coverage{0.0};
  std::string forbid_split{};
  std::optional<std::size_t> forbid_exposure_anchor_index_begin{std::nullopt};
  std::optional<std::size_t> forbid_exposure_anchor_index_end{std::nullopt};
  std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
      forbid_exposure_uses{};
  bool forbid_exposure_uses_from_split_policy{false};
  bool forbid_exposure_requires_mutated_component{true};
  std::string plan_mode{"train | debug"};
  std::string plan_input_mdn_checkpoint{};
  std::string plan_input_representation_checkpoint{};
  std::int64_t max_waves{1};
  std::vector<lattice_warning_spec_t> warning_specs{};
  std::vector<std::string> language_warnings{};
};

struct lattice_guard_spec_t {
  std::string guard_id{};
  std::string split{};
  std::optional<std::size_t> forbid_exposure_anchor_index_begin{std::nullopt};
  std::optional<std::size_t> forbid_exposure_anchor_index_end{std::nullopt};
  std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t> uses{};
  bool requires_mutated_component{true};
};

struct lattice_clause_field_t {
  std::string key{};
  std::string value{};
};

struct lattice_dependency_clause_t {
  std::string clause_id{};
  std::string target_id{};
  std::string upstream_target_id{};
  std::string binding{"loaded_representation_checkpoint"};
  bool require_exact_loaded_checkpoint{false};
  std::vector<lattice_clause_field_t> fields{};
};

struct lattice_requirement_clause_t {
  std::string clause_id{};
  std::string target_id{};
  std::string kind{};
  std::vector<lattice_clause_field_t> fields{};
};

struct lattice_forbid_clause_t {
  std::string clause_id{};
  std::string target_id{};
  std::string kind{"exposure_overlap"};
  std::vector<lattice_clause_field_t> fields{};
};

struct lattice_plan_clause_t {
  std::string clause_id{};
  std::string target_id{};
  std::vector<lattice_clause_field_t> fields{};
};

struct lattice_warning_clause_t {
  std::string clause_id{};
  std::string target_id{};
  std::string kind{};
  std::vector<lattice_clause_field_t> fields{};
};

struct lattice_target_compiled_t {
  lattice_target_spec_t lowered_v0{};
  std::vector<lattice_dependency_clause_t> dependencies{};
  std::vector<lattice_requirement_clause_t> requirements{};
  std::vector<lattice_forbid_clause_t> forbids{};
  std::vector<lattice_plan_clause_t> plans{};
  std::vector<lattice_warning_clause_t> warning_clauses{};
  std::vector<std::string> applied_profiles{};
  std::vector<std::string> applied_guards{};
  std::vector<std::string> warnings{};
  std::string target_spec_fingerprint{};
};

struct lattice_suggested_wave_t {
  std::string target{};
  std::string mode{};
  std::string source_range{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::string input_mdn_checkpoint{};
  std::string input_representation_checkpoint{};

  [[nodiscard]] bool empty() const { return target.empty(); }

  [[nodiscard]] std::string to_text() const {
    if (empty()) {
      return {};
    }
    std::ostringstream out;
    out << "TARGET=" << target << "\n";
    out << "MODE=" << mode << "\n";
    out << "SOURCE_RANGE=" << source_range << "\n";
    if (anchor_index_begin.has_value()) {
      out << "ANCHOR_INDEX_BEGIN=" << *anchor_index_begin << "\n";
    }
    if (anchor_index_end.has_value()) {
      out << "ANCHOR_INDEX_END=" << *anchor_index_end << "\n";
    }
    if (!input_mdn_checkpoint.empty()) {
      out << "INPUT_MDN_CHECKPOINT=" << input_mdn_checkpoint << "\n";
    }
    if (!input_representation_checkpoint.empty()) {
      out << "INPUT_REPRESENTATION_CHECKPOINT="
          << input_representation_checkpoint << "\n";
    }
    return out.str();
  }
};

struct lattice_target_evidence_t {
  std::filesystem::path job_dir{};
  std::filesystem::path manifest_path{};
  std::filesystem::path state_path{};
  std::filesystem::path report_path{};
  std::filesystem::path checkpoint_path{};
  bool report_exists{false};
  std::string protocol_contract_fingerprint{};
  std::string component_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string status{};
  std::int64_t matching_job_count{0};
  std::int64_t matching_train_attempt_count{0};
  std::int64_t optimizer_steps{0};
  double loss{std::numeric_limits<double>::quiet_NaN()};
  double valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  std::int64_t active_node_head_count{0};
  std::int64_t trained_node_head_count{0};
  std::int64_t evaluated_node_head_count{0};
  bool checkpoint_written{false};
  std::filesystem::path representation_checkpoint_path{};
  bool representation_checkpoint_loaded{false};
  std::filesystem::path mdn_checkpoint_path{};
  bool mdn_checkpoint_loaded{false};
  bool allow_untrained_representation{false};
};

struct lattice_target_evaluation_t {
  std::string target_id{};
  lattice_target_kind_t kind{lattice_target_kind_t::node_mdn_ready};
  std::string component{};
  std::string split_policy_fingerprint{};
  lattice_target_status_t status{lattice_target_status_t::missing_report};
  std::vector<std::string> reasons{};
  bool plan_ready{false};
  lattice_suggested_wave_t suggested_wave{};
  lattice_target_evidence_t evidence{};
  std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_load_summary_t>
      exposure_summaries{};
  std::vector<std::string> warnings{};
  struct warning_result_t {
    std::string warning_id{};
    std::string kind{};
    std::string status{"clear"};
    std::string severity{"info"};
    double measured_value{std::numeric_limits<double>::quiet_NaN()};
    double threshold{std::numeric_limits<double>::quiet_NaN()};
    std::string unit{};
    std::string split{};
    std::string use{};
    std::string scope{};
    std::string effect{};
    std::string metric{};
    std::string message{};
    cuwacunu::kikijyeba::lattice::exposure::exposure_load_summary_t
        exposure_summary{};
  };
  std::vector<warning_result_t> warning_results{};
};

struct lattice_target_validation_report_t {
  std::vector<std::string> warnings{};

  [[nodiscard]] bool ok() const { return warnings.empty(); }
};

[[nodiscard]] inline const char *
lattice_target_kind_name(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::representation_ready:
    return "representation_ready";
  case lattice_target_kind_t::node_mdn_ready:
    return "node_mdn_ready";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline lattice_target_kind_t
parse_lattice_target_kind(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value == "representation_ready") {
    return lattice_target_kind_t::representation_ready;
  }
  if (value == "node_mdn_ready") {
    return lattice_target_kind_t::node_mdn_ready;
  }
  throw std::runtime_error("[lattice_target] invalid TARGET_KIND: " + value);
}

[[nodiscard]] inline const char *
lattice_target_status_name(lattice_target_status_t status) {
  switch (status) {
  case lattice_target_status_t::satisfied:
    return "satisfied";
  case lattice_target_status_t::missing_report:
    return "missing_report";
  case lattice_target_status_t::stale_contract:
    return "stale_contract";
  case lattice_target_status_t::stale_component:
    return "stale_component";
  case lattice_target_status_t::missing_checkpoint:
    return "missing_checkpoint";
  case lattice_target_status_t::metric_failed:
    return "metric_failed";
  case lattice_target_status_t::exposure_failed:
    return "exposure_failed";
  case lattice_target_status_t::blocked:
    return "blocked";
  }
  throw std::runtime_error("[lattice_target] unknown target status");
}

[[nodiscard]] inline std::string
default_component_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::representation_ready:
    return "wikimyei.representation.encoding.vicreg";
  case lattice_target_kind_t::node_mdn_ready:
    return "wikimyei.inference.expected_value.mdn";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
job_kind_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::representation_ready:
    return "representation_vicreg";
  case lattice_target_kind_t::node_mdn_ready:
    return "inference_mdn";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
component_fingerprint_key_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::representation_ready:
    return "vicreg_assembly_fingerprint";
  case lattice_target_kind_t::node_mdn_ready:
    return "mdn_assembly_fingerprint";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline bool
has_lattice_key(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                const std::string &key) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  return block.values.find(kv::uppercase(key)) != block.values.end();
}

[[nodiscard]] inline std::string
optional_alias(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
               const std::string &canonical_key, const std::string &alias_key,
               const std::string &fallback, std::vector<std::string> &warnings,
               const std::string &subject) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const bool has_canonical = has_lattice_key(block, canonical_key);
  const bool has_alias = has_lattice_key(block, alias_key);
  if (has_canonical && has_alias) {
    const auto canonical_value = kv::optional(block, canonical_key, "");
    const auto alias_value = kv::optional(block, alias_key, "");
    if (kv::trim(canonical_value) != kv::trim(alias_value)) {
      throw std::runtime_error("[lattice_target] " + subject + " mixes " +
                               canonical_key + " and " + alias_key +
                               " with different values");
    }
    warnings.push_back("[lattice_target] " + subject + " specifies both " +
                       canonical_key + " and alias " + alias_key + "; prefer " +
                       alias_key);
  }
  if (has_alias) {
    return kv::optional(block, alias_key, fallback);
  }
  return kv::optional(block, canonical_key, fallback);
}

[[nodiscard]] inline std::int64_t
optional_i64_alias(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                   const std::string &canonical_key,
                   const std::string &alias_key, const std::string &fallback,
                   std::vector<std::string> &warnings,
                   const std::string &subject) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  return kv::parse_i64(optional_alias(block, canonical_key, alias_key, fallback,
                                      warnings, subject));
}

[[nodiscard]] inline std::vector<lattice_clause_field_t>
lattice_clause_fields_from_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  std::vector<lattice_clause_field_t> fields;
  fields.reserve(block.values.size());
  for (const auto &[key, value] : block.values) {
    fields.push_back(lattice_clause_field_t{.key = key, .value = value});
  }
  std::sort(fields.begin(), fields.end(),
            [](const lattice_clause_field_t &a,
               const lattice_clause_field_t &b) { return a.key < b.key; });
  return fields;
}

[[nodiscard]] inline std::string
lattice_clause_field_value(const std::vector<lattice_clause_field_t> &fields,
                           const std::string &key,
                           const std::string &fallback = {}) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto target = kv::uppercase(key);
  for (const auto &field : fields) {
    if (field.key == target) {
      const auto trimmed = kv::trim(field.value);
      return trimmed.empty() ? fallback : trimmed;
    }
  }
  return fallback;
}

[[nodiscard]] inline std::string auto_clause_id(const std::string &prefix,
                                                const std::string &target_id,
                                                std::size_t index) {
  return prefix + ":" + target_id + ":" + std::to_string(index);
}

[[nodiscard]] inline lattice_dependency_clause_t make_lattice_dependency_clause(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    std::size_t index) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_dependency_clause_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.target_id = kv::required(block, "TARGET_ID");
  out.upstream_target_id = kv::required(block, "UPSTREAM_TARGET_ID");
  out.binding = kv::lowercase(
      kv::optional(block, "BINDING", "loaded_representation_checkpoint"));
  out.require_exact_loaded_checkpoint = kv::parse_bool(
      kv::optional(block, "REQUIRE_EXACT_LOADED_CHECKPOINT", "false"));
  out.clause_id = lattice_clause_field_value(
      out.fields, "DEPENDENCY_ID",
      auto_clause_id("depends", out.target_id, index));
  return out;
}

[[nodiscard]] inline lattice_requirement_clause_t
make_lattice_requirement_clause(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    std::size_t index) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_requirement_clause_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.target_id = kv::required(block, "TARGET_ID");
  out.kind = kv::lowercase(kv::required(block, "KIND"));
  out.clause_id =
      kv::optional(block, "REQUIREMENT_ID",
                   auto_clause_id("requires", out.target_id, index));
  return out;
}

[[nodiscard]] inline lattice_forbid_clause_t make_lattice_forbid_clause(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    std::size_t index) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_forbid_clause_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.target_id = kv::required(block, "TARGET_ID");
  out.kind = kv::lowercase(kv::optional(block, "KIND", "exposure_overlap"));
  out.clause_id = kv::optional(block, "FORBID_ID",
                               auto_clause_id("forbids", out.target_id, index));
  return out;
}

[[nodiscard]] inline lattice_forbid_clause_t
make_lattice_forbid_clause_from_guard(const std::string &target_id,
                                      const lattice_guard_spec_t &guard) {
  lattice_forbid_clause_t out{};
  out.clause_id = guard.guard_id;
  out.target_id = target_id;
  out.kind = "exposure_overlap";
  out.fields.push_back({"COORDINATE", "source_row_footprint"});
  out.fields.push_back({"EFFECT", guard.requires_mutated_component
                                      ? "mutated_component"
                                      : "any"});
  if (!guard.split.empty()) {
    out.fields.push_back({"SPLIT", guard.split});
  }
  if (guard.forbid_exposure_anchor_index_begin.has_value()) {
    out.fields.push_back(
        {"FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN",
         std::to_string(*guard.forbid_exposure_anchor_index_begin)});
  }
  if (guard.forbid_exposure_anchor_index_end.has_value()) {
    out.fields.push_back(
        {"FORBID_EXPOSURE_ANCHOR_INDEX_END",
         std::to_string(*guard.forbid_exposure_anchor_index_end)});
  }
  std::ostringstream uses;
  for (std::size_t i = 0; i < guard.uses.size(); ++i) {
    if (i != 0) {
      uses << "|";
    }
    uses << cuwacunu::kikijyeba::lattice::exposure::exposure_use_name(
        guard.uses[i]);
  }
  out.fields.push_back({"USES", uses.str()});
  out.fields.push_back({"EVIDENCE_SCOPE", "output_checkpoint_closure"});
  out.fields.push_back({"COMPONENT_SCOPE", "all_components"});
  out.fields.push_back({"SOURCE", "LATTICE_GUARD"});
  std::sort(out.fields.begin(), out.fields.end(),
            [](const lattice_clause_field_t &a,
               const lattice_clause_field_t &b) { return a.key < b.key; });
  return out;
}

[[nodiscard]] inline lattice_forbid_clause_t
make_lattice_forbid_clause_from_split_protection(
    const lattice_target_spec_t &spec) {
  lattice_forbid_clause_t out{};
  out.clause_id = "protect_split:" + spec.target_id + ":" + spec.forbid_split;
  out.target_id = spec.target_id;
  out.kind = "exposure_overlap";
  out.fields.push_back({"COMPONENT_SCOPE", "all_components"});
  out.fields.push_back({"COORDINATE", "source_row_footprint"});
  out.fields.push_back({"EVIDENCE_SCOPE", "output_checkpoint_closure"});
  out.fields.push_back(
      {"EFFECT", spec.forbid_exposure_requires_mutated_component
                     ? "mutated_component"
                     : "any"});
  out.fields.push_back({"SOURCE", "PROTECT_SPLIT"});
  out.fields.push_back({"SPLIT", spec.forbid_split});
  out.fields.push_back({"USES", "split_policy_default"});
  std::sort(out.fields.begin(), out.fields.end(),
            [](const lattice_clause_field_t &a,
               const lattice_clause_field_t &b) { return a.key < b.key; });
  return out;
}

[[nodiscard]] inline lattice_plan_clause_t make_lattice_plan_clause(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    std::size_t index) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_plan_clause_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.target_id = kv::required(block, "TARGET_ID");
  out.clause_id = kv::optional(block, "PLAN_ID",
                               auto_clause_id("plan", out.target_id, index));
  return out;
}

[[nodiscard]] inline lattice_warning_clause_t make_lattice_warning_clause(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    std::size_t index) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_warning_clause_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.target_id = kv::required(block, "TARGET_ID");
  out.kind = kv::lowercase(kv::required(block, "KIND"));
  out.clause_id = kv::optional(block, "WARNING_ID",
                               auto_clause_id("warn", out.target_id, index));
  return out;
}

[[nodiscard]] inline lattice_target_spec_t decode_lattice_target_spec_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_target_spec_t out{};
  out.target_id = kv::required(block, "TARGET_ID");
  out.use_profile = kv::optional(block, "USE_PROFILE", "");
  out.guard_id = kv::optional(block, "GUARD", "");
  out.target_class =
      kv::lowercase(kv::optional(block, "TARGET_CLASS", out.target_class));
  out.kind = parse_lattice_target_kind(kv::required(block, "TARGET_KIND"));
  out.component = optional_alias(block, "COMPONENT", "SUBJECT_COMPONENT",
                                 default_component_for_target_kind(out.kind),
                                 out.language_warnings, out.target_id);
  out.checkpoint_source =
      kv::optional(block, "CHECKPOINT_SOURCE", out.checkpoint_source);
  out.evaluated_checkpoint_source =
      kv::optional(block, "EVALUATED_CHECKPOINT_SOURCE", "");
  out.source_range = kv::optional(block, "SOURCE_RANGE", out.source_range);
  out.train_split = optional_alias(block, "TRAIN_SPLIT", "OVER_SPLIT", "",
                                   out.language_warnings, out.target_id);
  const auto anchor_begin_raw = kv::optional(block, "ANCHOR_INDEX_BEGIN", "");
  if (!kv::trim(anchor_begin_raw).empty()) {
    const auto value = kv::parse_i64(anchor_begin_raw);
    if (value < 0) {
      throw std::runtime_error(
          "[lattice_target] ANCHOR_INDEX_BEGIN must be non-negative");
    }
    out.anchor_index_begin = static_cast<std::size_t>(value);
  }
  const auto anchor_end_raw = kv::optional(block, "ANCHOR_INDEX_END", "");
  if (!kv::trim(anchor_end_raw).empty()) {
    const auto value = kv::parse_i64(anchor_end_raw);
    if (value < 0) {
      throw std::runtime_error(
          "[lattice_target] ANCHOR_INDEX_END must be non-negative");
    }
    out.anchor_index_end = static_cast<std::size_t>(value);
  }
  out.upstream_target_id = kv::optional(block, "UPSTREAM_TARGET_ID", "");
  out.require_contract_match =
      kv::parse_bool(kv::optional(block, "REQUIRE_CONTRACT_MATCH", "true"));
  out.require_component_match =
      kv::parse_bool(kv::optional(block, "REQUIRE_COMPONENT_MATCH", "true"));
  out.require_checkpoint_exists =
      kv::parse_bool(kv::optional(block, "REQUIRE_CHECKPOINT_EXISTS", "true"));
  out.require_finite_loss =
      kv::parse_bool(kv::optional(block, "REQUIRE_FINITE_LOSS", "true"));
  out.min_optimizer_steps =
      kv::parse_i64(kv::optional(block, "MIN_OPTIMIZER_STEPS", "0"));
  out.min_valid_target_fraction =
      kv::parse_double(kv::optional(block, "MIN_VALID_TARGET_FRACTION", "0"));
  out.require_active_node_head_count =
      kv::parse_i64(kv::optional(block, "REQUIRE_ACTIVE_NODE_HEAD_COUNT", "0"));
  out.require_trained_node_head_count = kv::parse_i64(
      kv::optional(block, "REQUIRE_TRAINED_NODE_HEAD_COUNT", "0"));
  out.require_evaluated_node_head_count = kv::parse_i64(
      kv::optional(block, "REQUIRE_EVALUATED_NODE_HEAD_COUNT", "0"));
  out.min_observed_input_coverage =
      kv::parse_double(kv::optional(block, "MIN_OBSERVED_INPUT_COVERAGE", "0"));
  out.min_target_supervision_coverage = kv::parse_double(
      kv::optional(block, "MIN_TARGET_SUPERVISION_COVERAGE", "0"));
  out.min_evaluation_metric_coverage = kv::parse_double(
      kv::optional(block, "MIN_EVALUATION_METRIC_COVERAGE", "0"));
  const auto protect_split =
      optional_alias(block, "PROTECT_SPLIT", "APPLY_SPLIT_PROTECTION", "",
                     out.language_warnings, out.target_id);
  out.forbid_split = kv::optional(block, "FORBID_SPLIT", "");
  if (!kv::trim(protect_split).empty()) {
    if (!out.forbid_split.empty() && out.forbid_split != protect_split) {
      throw std::runtime_error("[lattice_target] target " + out.target_id +
                               " mixes PROTECT_SPLIT and FORBID_SPLIT with "
                               "different values");
    }
    out.forbid_split = protect_split;
    out.forbid_exposure_uses_from_split_policy = true;
  }
  const auto forbid_begin_raw =
      kv::optional(block, "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN", "");
  if (!kv::trim(forbid_begin_raw).empty()) {
    const auto value = kv::parse_i64(forbid_begin_raw);
    if (value < 0) {
      throw std::runtime_error("[lattice_target] "
                               "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN must be "
                               "non-negative");
    }
    out.forbid_exposure_anchor_index_begin = static_cast<std::size_t>(value);
  }
  const auto forbid_end_raw =
      kv::optional(block, "FORBID_EXPOSURE_ANCHOR_INDEX_END", "");
  if (!kv::trim(forbid_end_raw).empty()) {
    const auto value = kv::parse_i64(forbid_end_raw);
    if (value < 0) {
      throw std::runtime_error("[lattice_target] "
                               "FORBID_EXPOSURE_ANCHOR_INDEX_END must be "
                               "non-negative");
    }
    out.forbid_exposure_anchor_index_end = static_cast<std::size_t>(value);
  }
  const auto forbid_uses_raw = kv::optional(block, "FORBID_EXPOSURE_USES", "");
  if (!kv::trim(forbid_uses_raw).empty()) {
    if (out.forbid_exposure_uses_from_split_policy) {
      throw std::runtime_error(
          "[lattice_target] target " + out.target_id +
          " mixes PROTECT_SPLIT with explicit FORBID_EXPOSURE_USES");
    }
    out.forbid_exposure_uses =
        cuwacunu::kikijyeba::lattice::exposure::parse_exposure_use_list(
            forbid_uses_raw);
  }
  out.forbid_exposure_requires_mutated_component = kv::parse_bool(kv::optional(
      block, "FORBID_EXPOSURE_REQUIRES_MUTATED_COMPONENT", "true"));
  out.plan_mode = optional_alias(block, "PLAN_MODE", "WAVE_MODE", out.plan_mode,
                                 out.language_warnings, out.target_id);
  out.plan_input_mdn_checkpoint =
      kv::optional(block, "PLAN_INPUT_MDN_CHECKPOINT", "");
  out.plan_input_representation_checkpoint =
      kv::optional(block, "PLAN_INPUT_REPRESENTATION_CHECKPOINT", "");
  out.max_waves = optional_i64_alias(block, "MAX_WAVES", "PLAN_MAX_ATTEMPTS",
                                     "1", out.language_warnings, out.target_id);
  if (out.target_id.empty()) {
    throw std::runtime_error("[lattice_target] TARGET_ID is required");
  }
  if (out.component != default_component_for_target_kind(out.kind)) {
    throw std::runtime_error(
        "[lattice_target] COMPONENT does not match TARGET_KIND for " +
        out.target_id);
  }
  const auto source_range =
      kv::lowercase(kv::trim(std::string(out.source_range)));
  const auto checkpoint_source =
      kv::lowercase(kv::trim(std::string(out.checkpoint_source)));
  const std::string latest_prefix = "latest_satisfying:";
  if (checkpoint_source != "output_checkpoint" &&
      checkpoint_source.rfind(latest_prefix, 0) != 0) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE must be output_checkpoint or "
        "latest_satisfying:<target_id> for " +
        out.target_id);
  }
  if (checkpoint_source.rfind(latest_prefix, 0) == 0 &&
      kv::trim(out.checkpoint_source.substr(latest_prefix.size())).empty()) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE latest_satisfying requires a "
        "target id for " +
        out.target_id);
  }
  const auto validate_latest_satisfying = [&](const std::string &value,
                                              const std::string &field_name) {
    const auto lowered = kv::lowercase(kv::trim(value));
    if (lowered.empty()) {
      return;
    }
    if (lowered.rfind(latest_prefix, 0) != 0) {
      throw std::runtime_error("[lattice_target] " + field_name +
                               " must be latest_satisfying:<target_id> "
                               "for " +
                               out.target_id);
    }
    if (kv::trim(value.substr(latest_prefix.size())).empty()) {
      throw std::runtime_error("[lattice_target] " + field_name +
                               " latest_satisfying requires a target id "
                               "for " +
                               out.target_id);
    }
  };
  validate_latest_satisfying(out.evaluated_checkpoint_source,
                             "EVALUATED_CHECKPOINT_SOURCE");
  validate_latest_satisfying(out.plan_input_mdn_checkpoint,
                             "PLAN_INPUT_MDN_CHECKPOINT");
  validate_latest_satisfying(out.plan_input_representation_checkpoint,
                             "PLAN_INPUT_REPRESENTATION_CHECKPOINT");
  if (source_range != "all" && source_range != "anchor_index") {
    throw std::runtime_error("[lattice_target] SOURCE_RANGE must be all or "
                             "anchor_index for " +
                             out.target_id);
  }
  if (source_range == "all" && out.train_split.empty() &&
      (out.anchor_index_begin.has_value() ||
       out.anchor_index_end.has_value())) {
    throw std::runtime_error("[lattice_target] ANCHOR_INDEX_BEGIN/END require "
                             "SOURCE_RANGE=anchor_index for " +
                             out.target_id);
  }
  if (source_range == "anchor_index" && out.train_split.empty()) {
    if (!out.anchor_index_begin.has_value() ||
        !out.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] SOURCE_RANGE=anchor_index requires "
          "ANCHOR_INDEX_BEGIN and ANCHOR_INDEX_END for " +
          out.target_id);
    }
    if (*out.anchor_index_end <= *out.anchor_index_begin) {
      throw std::runtime_error("[lattice_target] ANCHOR_INDEX_END must be "
                               "greater than ANCHOR_INDEX_BEGIN for " +
                               out.target_id);
    }
  }
  if (!out.train_split.empty()) {
    if (source_range != "anchor_index") {
      throw std::runtime_error("[lattice_target] TRAIN_SPLIT requires "
                               "SOURCE_RANGE=anchor_index for " +
                               out.target_id);
    }
    if (out.anchor_index_begin.has_value() ||
        out.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] TRAIN_SPLIT and ANCHOR_INDEX_BEGIN/END are "
          "mutually exclusive for " +
          out.target_id);
    }
  }
  if (out.min_optimizer_steps < 0 || out.min_valid_target_fraction < 0.0 ||
      out.min_valid_target_fraction > 1.0 ||
      out.min_observed_input_coverage < 0.0 ||
      out.min_observed_input_coverage > 1.0 ||
      out.min_target_supervision_coverage < 0.0 ||
      out.min_target_supervision_coverage > 1.0 ||
      out.min_evaluation_metric_coverage < 0.0 ||
      out.min_evaluation_metric_coverage > 1.0 || out.max_waves < 0 ||
      out.require_active_node_head_count < 0 ||
      out.require_trained_node_head_count < 0 ||
      out.require_evaluated_node_head_count < 0) {
    throw std::runtime_error("[lattice_target] numeric target thresholds are "
                             "outside their valid range");
  }
  const bool requires_exposure_coverage =
      out.min_observed_input_coverage > 0.0 ||
      out.min_target_supervision_coverage > 0.0 ||
      out.min_evaluation_metric_coverage > 0.0;
  if (requires_exposure_coverage && source_range != "anchor_index") {
    throw std::runtime_error(
        "[lattice_target] exposure coverage thresholds require "
        "SOURCE_RANGE=anchor_index for " +
        out.target_id);
  }
  if (!out.forbid_split.empty() &&
      (out.forbid_exposure_anchor_index_begin.has_value() ||
       out.forbid_exposure_anchor_index_end.has_value())) {
    throw std::runtime_error(
        "[lattice_target] FORBID_SPLIT and FORBID_EXPOSURE_ANCHOR_INDEX_* are "
        "mutually exclusive for " +
        out.target_id);
  }
  if (out.forbid_exposure_anchor_index_begin.has_value() !=
      out.forbid_exposure_anchor_index_end.has_value()) {
    throw std::runtime_error(
        "[lattice_target] FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN and "
        "FORBID_EXPOSURE_ANCHOR_INDEX_END must be provided together for " +
        out.target_id);
  }
  if (out.forbid_exposure_anchor_index_begin.has_value()) {
    if (*out.forbid_exposure_anchor_index_end <=
        *out.forbid_exposure_anchor_index_begin) {
      throw std::runtime_error("[lattice_target] "
                               "FORBID_EXPOSURE_ANCHOR_INDEX_END must be "
                               "greater than BEGIN for " +
                               out.target_id);
    }
    if (out.forbid_exposure_uses.empty()) {
      throw std::runtime_error(
          "[lattice_target] FORBID_EXPOSURE_USES is required when a forbidden "
          "exposure range is configured for " +
          out.target_id);
    }
  } else if (out.forbid_split.empty() && !out.forbid_exposure_uses.empty()) {
    throw std::runtime_error(
        "[lattice_target] FORBID_EXPOSURE_USES requires a forbidden exposure "
        "range for " +
        out.target_id);
  }
  if (!out.forbid_split.empty() && out.forbid_exposure_uses.empty() &&
      !out.forbid_exposure_uses_from_split_policy) {
    throw std::runtime_error(
        "[lattice_target] FORBID_EXPOSURE_USES is required when FORBID_SPLIT "
        "is configured directly for " +
        out.target_id);
  }
  return out;
}

[[nodiscard]] inline lattice_guard_spec_t decode_lattice_guard_spec_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_guard_spec_t out{};
  out.guard_id = kv::required(block, "GUARD_ID");
  const auto kind =
      kv::lowercase(kv::optional(block, "KIND", "exposure_overlap"));
  if (kind != "exposure_overlap") {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD KIND must be exposure_overlap for " +
        out.guard_id);
  }
  out.split = kv::optional(block, "SPLIT", "");
  const auto forbid_begin_raw =
      kv::optional(block, "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN", "");
  if (!kv::trim(forbid_begin_raw).empty()) {
    const auto value = kv::parse_i64(forbid_begin_raw);
    if (value < 0) {
      throw std::runtime_error("[lattice_target] guard " + out.guard_id +
                               " has negative forbidden begin");
    }
    out.forbid_exposure_anchor_index_begin = static_cast<std::size_t>(value);
  }
  const auto forbid_end_raw =
      kv::optional(block, "FORBID_EXPOSURE_ANCHOR_INDEX_END", "");
  if (!kv::trim(forbid_end_raw).empty()) {
    const auto value = kv::parse_i64(forbid_end_raw);
    if (value < 0) {
      throw std::runtime_error("[lattice_target] guard " + out.guard_id +
                               " has negative forbidden end");
    }
    out.forbid_exposure_anchor_index_end = static_cast<std::size_t>(value);
  }
  const auto uses_raw = kv::optional(block, "USES", "");
  if (!kv::trim(uses_raw).empty()) {
    out.uses = cuwacunu::kikijyeba::lattice::exposure::parse_exposure_use_list(
        uses_raw);
  }
  const auto scope = kv::lowercase(
      kv::optional(block, "EVIDENCE_SCOPE",
                   kv::optional(block, "SCOPE", "checkpoint_closure")));
  if (scope != "checkpoint_closure" && scope != "output_checkpoint_closure") {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD EVIDENCE_SCOPE must be "
        "checkpoint_closure or output_checkpoint_closure for " +
        out.guard_id);
  }
  const auto component_scope =
      kv::lowercase(kv::optional(block, "COMPONENT_SCOPE", "all_components"));
  if (component_scope != "all_components") {
    throw std::runtime_error("[lattice_target] LATTICE_GUARD COMPONENT_SCOPE "
                             "must be all_components for " +
                             out.guard_id);
  }
  const auto coordinate =
      kv::lowercase(kv::optional(block, "COORDINATE", "source_row_footprint"));
  if (coordinate != "source_row_footprint") {
    throw std::runtime_error("[lattice_target] LATTICE_GUARD COORDINATE must "
                             "be source_row_footprint for " +
                             out.guard_id);
  }
  const auto effect =
      kv::lowercase(kv::optional(block, "EFFECT", "mutated_component"));
  if (effect == "mutated_component") {
    out.requires_mutated_component = true;
  } else if (effect == "any") {
    out.requires_mutated_component = false;
  } else {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD EFFECT must be mutated_component or "
        "any for " +
        out.guard_id);
  }
  const bool has_split = !out.split.empty();
  const bool has_explicit_range =
      out.forbid_exposure_anchor_index_begin.has_value() ||
      out.forbid_exposure_anchor_index_end.has_value();
  if (has_split == has_explicit_range) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD must use either SPLIT or explicit "
        "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN/END for " +
        out.guard_id);
  }
  if (out.forbid_exposure_anchor_index_begin.has_value() !=
      out.forbid_exposure_anchor_index_end.has_value()) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD explicit forbidden range must provide "
        "both begin and end for " +
        out.guard_id);
  }
  if (out.forbid_exposure_anchor_index_begin.has_value() &&
      *out.forbid_exposure_anchor_index_end <=
          *out.forbid_exposure_anchor_index_begin) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_GUARD forbidden end must be greater than "
        "begin for " +
        out.guard_id);
  }
  if (out.uses.empty()) {
    throw std::runtime_error("[lattice_target] LATTICE_GUARD USES is required "
                             "for " +
                             out.guard_id);
  }
  return out;
}

inline void apply_lattice_guard(lattice_target_spec_t &spec,
                                const lattice_guard_spec_t &guard) {
  const bool has_explicit_forbid =
      !spec.forbid_split.empty() ||
      spec.forbid_exposure_anchor_index_begin.has_value() ||
      spec.forbid_exposure_anchor_index_end.has_value() ||
      !spec.forbid_exposure_uses.empty();
  if (has_explicit_forbid) {
    throw std::runtime_error(
        "[lattice_target] target " + spec.target_id +
        " mixes GUARD with explicit forbidden exposure fields");
  }
  spec.forbid_split = guard.split;
  spec.forbid_exposure_anchor_index_begin =
      guard.forbid_exposure_anchor_index_begin;
  spec.forbid_exposure_anchor_index_end =
      guard.forbid_exposure_anchor_index_end;
  spec.forbid_exposure_uses = guard.uses;
  spec.forbid_exposure_uses_from_split_policy = false;
  spec.forbid_exposure_requires_mutated_component =
      guard.requires_mutated_component;
}

inline void assign_or_confirm_string(std::string &field,
                                     const std::string &value,
                                     const std::string &field_name,
                                     const std::string &target_id) {
  if (value.empty()) {
    return;
  }
  if (!field.empty() && field != value) {
    throw std::runtime_error("[lattice_target] target " + target_id +
                             " has conflicting " + field_name + " values");
  }
  field = value;
}

inline void require_anchor_index_source_range(lattice_target_spec_t &spec,
                                              const std::string &why) {
  if (spec.source_range.empty() || spec.source_range == "all") {
    spec.source_range = "anchor_index";
    return;
  }
  if (spec.source_range != "anchor_index") {
    throw std::runtime_error("[lattice_target] target " + spec.target_id +
                             " cannot use " + why +
                             " unless SOURCE_RANGE=anchor_index");
  }
}

inline void require_lowered_bool_true(const std::string &raw,
                                      const std::string &field_name,
                                      const std::string &target_id) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  if (!kv::parse_bool(raw)) {
    throw std::runtime_error("[lattice_target] target " + target_id +
                             " clause can only lower " + field_name +
                             "=true in v0");
  }
}

inline void apply_lattice_depends_clause(
    lattice_target_spec_t &spec,
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto upstream_target_id = kv::required(block, "UPSTREAM_TARGET_ID");
  assign_or_confirm_string(spec.upstream_target_id, upstream_target_id,
                           "UPSTREAM_TARGET_ID", spec.target_id);
  const auto binding = kv::lowercase(
      kv::optional(block, "BINDING", "loaded_representation_checkpoint"));
  if (binding != "loaded_representation_checkpoint" && binding != "target") {
    throw std::runtime_error("[lattice_target] LATTICE_DEPENDS BINDING must be "
                             "loaded_representation_checkpoint or target for " +
                             spec.target_id);
  }
  const auto exact_loaded =
      kv::optional(block, "REQUIRE_EXACT_LOADED_CHECKPOINT", "");
  if (!kv::trim(exact_loaded).empty()) {
    require_lowered_bool_true(exact_loaded, "REQUIRE_EXACT_LOADED_CHECKPOINT",
                              spec.target_id);
  }
}

inline void apply_lattice_requires_clause(
    lattice_target_spec_t &spec,
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto kind = kv::lowercase(kv::required(block, "KIND"));
  if (kind == "artifact") {
    const auto artifact =
        kv::lowercase(kv::optional(block, "ARTIFACT", "output_checkpoint"));
    if (artifact != "output_checkpoint") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES artifact only supports "
          "output_checkpoint for " +
          spec.target_id);
    }
    require_lowered_bool_true(kv::optional(block, "EXISTS", "true"), "EXISTS",
                              spec.target_id);
    spec.require_checkpoint_exists = true;
    return;
  }
  if (kind == "metric") {
    const auto op = kv::lowercase(kv::optional(block, "OP", "finite"));
    if (op != "finite") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES metric only supports OP=finite "
          "for " +
          spec.target_id);
    }
    const auto metric = kv::lowercase(kv::optional(block, "METRIC", "loss"));
    if (metric != "loss" && metric != "mean_loss" && metric != "mean_nll") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES metric only supports loss, "
          "mean_loss, or mean_nll for " +
          spec.target_id);
    }
    spec.require_finite_loss = true;
    return;
  }
  if (kind == "effort") {
    const auto counter = kv::lowercase(
        kv::optional(block, "COUNTER", kv::optional(block, "METRIC", "")));
    const auto op = kv::lowercase(kv::optional(block, "OP", "ge"));
    if (counter != "optimizer_steps" || op != "ge") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES effort only supports "
          "optimizer_steps >= VALUE for " +
          spec.target_id);
    }
    const auto value = kv::parse_i64(
        kv::optional(block, "VALUE", kv::optional(block, "MIN_VALUE", "0")));
    if (value < 0) {
      throw std::runtime_error("[lattice_target] optimizer step requirement "
                               "must be non-negative for " +
                               spec.target_id);
    }
    spec.min_optimizer_steps = std::max(spec.min_optimizer_steps, value);
    return;
  }
  if (kind == "valid_target_fraction") {
    const auto op = kv::lowercase(kv::optional(block, "OP", "ge"));
    if (op != "ge") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES valid_target_fraction only "
          "supports OP=ge for " +
          spec.target_id);
    }
    const auto value = kv::parse_double(
        kv::optional(block, "VALUE", kv::optional(block, "MIN_VALUE", "0")));
    spec.min_valid_target_fraction =
        std::max(spec.min_valid_target_fraction, value);
    return;
  }
  if (kind == "node_head_count") {
    const auto head = kv::lowercase(kv::required(block, "HEAD"));
    const auto op = kv::lowercase(kv::optional(block, "OP", "ge"));
    if (op != "ge") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES node_head_count only supports "
          "OP=ge for " +
          spec.target_id);
    }
    const auto value = kv::parse_i64(kv::required(block, "VALUE"));
    if (value < 0) {
      throw std::runtime_error("[lattice_target] node head count requirement "
                               "must be non-negative for " +
                               spec.target_id);
    }
    if (head == "active") {
      spec.require_active_node_head_count =
          std::max(spec.require_active_node_head_count, value);
    } else if (head == "trained") {
      spec.require_trained_node_head_count =
          std::max(spec.require_trained_node_head_count, value);
    } else if (head == "evaluated") {
      spec.require_evaluated_node_head_count =
          std::max(spec.require_evaluated_node_head_count, value);
    } else {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES node_head_count HEAD must be "
          "active, trained, or evaluated for " +
          spec.target_id);
    }
    return;
  }
  if (kind == "exposure_coverage") {
    const auto use = cuwacunu::kikijyeba::lattice::exposure::parse_exposure_use(
        kv::required(block, "USE"));
    const auto split = kv::optional(block, "SPLIT", "");
    if (!split.empty()) {
      assign_or_confirm_string(spec.train_split, split, "OVER_SPLIT",
                               spec.target_id);
      require_anchor_index_source_range(spec, "exposure_coverage");
    }
    const auto evidence_scope = kv::lowercase(
        kv::optional(block, "EVIDENCE_SCOPE", "output_checkpoint_closure"));
    const auto scope = kv::lowercase(
        kv::optional(block, "COMPONENT_SCOPE",
                     kv::optional(block, "SCOPE", "target_component")));
    const auto effect =
        kv::lowercase(kv::optional(block, "EFFECT", "mutated_component"));
    const auto coordinate = kv::lowercase(
        kv::optional(block, "COORDINATE", "graph_anchor_coverage"));
    if ((evidence_scope != "output_checkpoint_closure" &&
         evidence_scope != "target_component") ||
        scope != "target_component" || coordinate != "graph_anchor_coverage") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES exposure_coverage v0 requires "
          "EVIDENCE_SCOPE=output_checkpoint_closure or target_component, "
          "COMPONENT_SCOPE=target_component, and "
          "COORDINATE=graph_anchor_coverage for " +
          spec.target_id);
    }
    const auto min_coverage_raw = kv::optional(
        block, "MIN_COVERAGE",
        kv::optional(block, "CURSOR_EPOCHS",
                     kv::optional(block, "MIN_CURSOR_EPOCHS", "")));
    if (kv::trim(min_coverage_raw).empty()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES exposure_coverage requires "
          "MIN_COVERAGE or CURSOR_EPOCHS for " +
          spec.target_id);
    }
    const auto min_coverage = kv::parse_double(min_coverage_raw);
    if (use == cuwacunu::kikijyeba::lattice::exposure::exposure_use_t::
                   observed_input) {
      if (effect != "mutated_component") {
        throw std::runtime_error(
            "[lattice_target] observed_input exposure_coverage v0 requires "
            "EFFECT=mutated_component for " +
            spec.target_id);
      }
      spec.min_observed_input_coverage =
          std::max(spec.min_observed_input_coverage, min_coverage);
    } else if (use == cuwacunu::kikijyeba::lattice::exposure::exposure_use_t::
                          target_supervision) {
      if (effect != "mutated_component") {
        throw std::runtime_error(
            "[lattice_target] target_supervision exposure_coverage v0 "
            "requires EFFECT=mutated_component for " +
            spec.target_id);
      }
      spec.min_target_supervision_coverage =
          std::max(spec.min_target_supervision_coverage, min_coverage);
    } else if (use == cuwacunu::kikijyeba::lattice::exposure::exposure_use_t::
                          evaluation_metric) {
      if (effect != "any" && effect != "none") {
        throw std::runtime_error(
            "[lattice_target] evaluation_metric exposure_coverage v0 "
            "requires EFFECT=any or EFFECT=none for " +
            spec.target_id);
      }
      spec.min_evaluation_metric_coverage =
          std::max(spec.min_evaluation_metric_coverage, min_coverage);
    } else {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES exposure_coverage only supports "
          "observed_input, target_supervision, or evaluation_metric for " +
          spec.target_id);
    }
    return;
  }
  throw std::runtime_error(
      "[lattice_target] unsupported LATTICE_REQUIRES KIND " + kind + " for " +
      spec.target_id);
}

inline void apply_lattice_forbids_clause(
    lattice_target_spec_t &spec,
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto kind =
      kv::lowercase(kv::optional(block, "KIND", "exposure_overlap"));
  if (kind != "exposure_overlap") {
    throw std::runtime_error(
        "[lattice_target] LATTICE_FORBIDS KIND must be exposure_overlap for " +
        spec.target_id);
  }
  cuwacunu::piaabo::parse::simple_kv::block_t guard_block{};
  guard_block.name = "LATTICE_GUARD";
  guard_block.values["GUARD_ID"] =
      kv::optional(block, "FORBID_ID", spec.target_id + "_forbid");
  guard_block.values["KIND"] = "exposure_overlap";
  if (has_lattice_key(block, "SPLIT")) {
    guard_block.values["SPLIT"] = kv::required(block, "SPLIT");
  }
  if (has_lattice_key(block, "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN")) {
    guard_block.values["FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN"] =
        kv::required(block, "FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN");
  }
  if (has_lattice_key(block, "FORBID_EXPOSURE_ANCHOR_INDEX_END")) {
    guard_block.values["FORBID_EXPOSURE_ANCHOR_INDEX_END"] =
        kv::required(block, "FORBID_EXPOSURE_ANCHOR_INDEX_END");
  }
  guard_block.values["USES"] = kv::required(block, "USES");
  guard_block.values["SCOPE"] =
      kv::optional(block, "EVIDENCE_SCOPE",
                   kv::optional(block, "SCOPE", "checkpoint_closure"));
  if (has_lattice_key(block, "COMPONENT_SCOPE")) {
    guard_block.values["COMPONENT_SCOPE"] =
        kv::required(block, "COMPONENT_SCOPE");
  }
  guard_block.values["EFFECT"] =
      kv::optional(block, "EFFECT", "mutated_component");
  guard_block.values["COORDINATE"] =
      kv::optional(block, "COORDINATE", "source_row_footprint");
  apply_lattice_guard(spec, decode_lattice_guard_spec_block(guard_block));
}

inline void apply_lattice_plan_clause(
    lattice_target_spec_t &spec,
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto wave_target = kv::optional(block, "WAVE_TARGET", spec.component);
  if (wave_target != spec.component) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_PLAN WAVE_TARGET must match "
        "SUBJECT_COMPONENT for " +
        spec.target_id);
  }
  spec.plan_mode = kv::optional(
      block, "WAVE_MODE", kv::optional(block, "PLAN_MODE", spec.plan_mode));
  if (has_lattice_key(block, "WAVE_RANGE")) {
    const auto wave_range = kv::required(block, "WAVE_RANGE");
    const std::string prefix = "split:";
    if (wave_range.rfind(prefix, 0) != 0) {
      throw std::runtime_error("[lattice_target] LATTICE_PLAN WAVE_RANGE v0 "
                               "must be split:<id> for " +
                               spec.target_id);
    }
    require_anchor_index_source_range(spec, "WAVE_RANGE=split:<id>");
    assign_or_confirm_string(spec.train_split, wave_range.substr(prefix.size()),
                             "OVER_SPLIT", spec.target_id);
  }
  if (has_lattice_key(block, "PLAN_MAX_ATTEMPTS") ||
      has_lattice_key(block, "MAX_WAVES")) {
    const auto value = kv::parse_i64(kv::optional(
        block, "PLAN_MAX_ATTEMPTS", kv::optional(block, "MAX_WAVES", "1")));
    if (value < 0) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_PLAN PLAN_MAX_ATTEMPTS must be "
          "non-negative for " +
          spec.target_id);
    }
    spec.max_waves = value;
  }
  if (has_lattice_key(block, "PLAN_INPUT_MDN_CHECKPOINT")) {
    assign_or_confirm_string(spec.plan_input_mdn_checkpoint,
                             kv::required(block, "PLAN_INPUT_MDN_CHECKPOINT"),
                             "PLAN_INPUT_MDN_CHECKPOINT", spec.target_id);
  }
  if (has_lattice_key(block, "PLAN_INPUT_REPRESENTATION_CHECKPOINT")) {
    assign_or_confirm_string(
        spec.plan_input_representation_checkpoint,
        kv::required(block, "PLAN_INPUT_REPRESENTATION_CHECKPOINT"),
        "PLAN_INPUT_REPRESENTATION_CHECKPOINT", spec.target_id);
  }
}

inline void apply_lattice_warn_clause(
    lattice_target_spec_t &spec,
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_warning_spec_t warning{};
  warning.warning_id = kv::optional(
      block, "WARNING_ID",
      auto_clause_id("warn", spec.target_id, spec.warning_specs.size()));
  warning.kind = kv::lowercase(kv::required(block, "KIND"));
  warning.split = kv::optional(block, "SPLIT", "");
  warning.scope = kv::lowercase(
      kv::optional(block, "COMPONENT_SCOPE",
                   kv::optional(block, "SCOPE", "target_component")));
  if (warning.scope != "target_component" &&
      warning.scope != "all_components") {
    throw std::runtime_error("[lattice_target] LATTICE_WARN SCOPE must be "
                             "target_component or all_components for " +
                             spec.target_id);
  }
  const auto effect =
      kv::lowercase(kv::optional(block, "EFFECT", "mutated_component"));
  if (effect == "mutated_component") {
    warning.require_mutated_component = true;
  } else if (effect == "any" || effect == "none") {
    warning.require_mutated_component = false;
  } else {
    throw std::runtime_error(
        "[lattice_target] LATTICE_WARN EFFECT must be mutated_component, any, "
        "or none for " +
        spec.target_id);
  }

  if (warning.kind == "exposure_load" || warning.kind == "effort_density") {
    warning.use = cuwacunu::kikijyeba::lattice::exposure::parse_exposure_use(
        kv::required(block, "USE"));
    if (has_lattice_key(block, "ANCHOR_INDEX_BEGIN")) {
      const auto value =
          kv::parse_i64(kv::required(block, "ANCHOR_INDEX_BEGIN"));
      if (value < 0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN ANCHOR_INDEX_BEGIN must be "
            "non-negative for " +
            spec.target_id);
      }
      warning.anchor_index_begin = static_cast<std::size_t>(value);
    }
    if (has_lattice_key(block, "ANCHOR_INDEX_END")) {
      const auto value = kv::parse_i64(kv::required(block, "ANCHOR_INDEX_END"));
      if (value < 0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN ANCHOR_INDEX_END must be "
            "non-negative for " +
            spec.target_id);
      }
      warning.anchor_index_end = static_cast<std::size_t>(value);
    }
    if (warning.anchor_index_begin.has_value() !=
        warning.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN explicit range requires both "
          "ANCHOR_INDEX_BEGIN and ANCHOR_INDEX_END for " +
          spec.target_id);
    }
    if (warning.anchor_index_begin.has_value() &&
        *warning.anchor_index_end <= *warning.anchor_index_begin) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN ANCHOR_INDEX_END must be greater "
          "than BEGIN for " +
          spec.target_id);
    }
  }

  if (warning.kind == "exposure_load") {
    warning.cursor_epochs_above =
        kv::parse_double(kv::required(block, "CURSOR_EPOCHS_ABOVE"));
    if (warning.cursor_epochs_above < 0.0) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN CURSOR_EPOCHS_ABOVE must be "
          "non-negative for " +
          spec.target_id);
    }
  } else if (warning.kind == "effort_density") {
    warning.metric = kv::lowercase(
        kv::optional(block, "METRIC", "optimizer_steps_per_cursor_epoch"));
    if (warning.metric != "optimizer_steps_per_cursor_epoch" &&
        warning.metric != "optimizer_steps_per_unique_anchor") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN effort_density METRIC must be "
          "optimizer_steps_per_cursor_epoch or "
          "optimizer_steps_per_unique_anchor for " +
          spec.target_id);
    }
    warning.above = kv::parse_double(kv::required(block, "ABOVE"));
    if (warning.above < 0.0) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN ABOVE must be non-negative for " +
          spec.target_id);
    }
  } else if (warning.kind == "anchor_domain_health") {
    const auto accepted_fraction_raw =
        kv::optional(block, "ACCEPTED_FRACTION_BELOW", "");
    if (!kv::trim(accepted_fraction_raw).empty()) {
      warning.accepted_fraction_below = kv::parse_double(accepted_fraction_raw);
      if (warning.accepted_fraction_below < 0.0 ||
          warning.accepted_fraction_below > 1.0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN ACCEPTED_FRACTION_BELOW must be "
            "between 0 and 1 for " +
            spec.target_id);
      }
    }
    const auto skipped_fetch_raw =
        kv::optional(block, "SKIPPED_FAILED_FETCH_PROBE_ABOVE", "");
    if (!kv::trim(skipped_fetch_raw).empty()) {
      const auto value = kv::parse_i64(skipped_fetch_raw);
      if (value < 0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN "
            "SKIPPED_FAILED_FETCH_PROBE_ABOVE must be non-negative for " +
            spec.target_id);
      }
      warning.skipped_failed_fetch_probe_above = value;
    }
    if (!std::isfinite(warning.accepted_fraction_below) &&
        !warning.skipped_failed_fetch_probe_above.has_value()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN anchor_domain_health requires "
          "ACCEPTED_FRACTION_BELOW or SKIPPED_FAILED_FETCH_PROBE_ABOVE for " +
          spec.target_id);
    }
  } else {
    throw std::runtime_error("[lattice_target] unsupported LATTICE_WARN KIND " +
                             warning.kind + " for " + spec.target_id);
  }

  spec.warning_specs.push_back(std::move(warning));
}

inline void
validate_lattice_target_spec_lowered(const lattice_target_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto source_range = kv::lowercase(kv::trim(spec.source_range));
  const auto checkpoint_source =
      kv::lowercase(kv::trim(std::string(spec.checkpoint_source)));
  const std::string latest_prefix = "latest_satisfying:";
  if (checkpoint_source != "output_checkpoint" &&
      checkpoint_source.rfind(latest_prefix, 0) != 0) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE must be output_checkpoint or "
        "latest_satisfying:<target_id> for " +
        spec.target_id);
  }
  if (checkpoint_source.rfind(latest_prefix, 0) == 0 &&
      kv::trim(spec.checkpoint_source.substr(latest_prefix.size())).empty()) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE latest_satisfying requires a "
        "target id for " +
        spec.target_id);
  }
  const auto validate_latest_satisfying = [&](const std::string &value,
                                              const std::string &field_name) {
    const auto lowered = kv::lowercase(kv::trim(value));
    if (lowered.empty()) {
      return;
    }
    if (lowered.rfind(latest_prefix, 0) != 0) {
      throw std::runtime_error("[lattice_target] " + field_name +
                               " must be latest_satisfying:<target_id> "
                               "for " +
                               spec.target_id);
    }
    if (kv::trim(value.substr(latest_prefix.size())).empty()) {
      throw std::runtime_error("[lattice_target] " + field_name +
                               " latest_satisfying requires a target id "
                               "for " +
                               spec.target_id);
    }
  };
  validate_latest_satisfying(spec.evaluated_checkpoint_source,
                             "EVALUATED_CHECKPOINT_SOURCE");
  validate_latest_satisfying(spec.plan_input_mdn_checkpoint,
                             "PLAN_INPUT_MDN_CHECKPOINT");
  validate_latest_satisfying(spec.plan_input_representation_checkpoint,
                             "PLAN_INPUT_REPRESENTATION_CHECKPOINT");
  if (source_range != "all" && source_range != "anchor_index") {
    throw std::runtime_error("[lattice_target] SOURCE_RANGE must be all or "
                             "anchor_index for " +
                             spec.target_id);
  }
  if (source_range == "all" &&
      (!spec.train_split.empty() || spec.anchor_index_begin.has_value() ||
       spec.anchor_index_end.has_value())) {
    throw std::runtime_error("[lattice_target] ranged target fields require "
                             "SOURCE_RANGE=anchor_index for " +
                             spec.target_id);
  }
  if (source_range == "anchor_index" && spec.train_split.empty()) {
    if (!spec.anchor_index_begin.has_value() ||
        !spec.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] SOURCE_RANGE=anchor_index requires either "
          "OVER_SPLIT or ANCHOR_INDEX_BEGIN/END for " +
          spec.target_id);
    }
    if (*spec.anchor_index_end <= *spec.anchor_index_begin) {
      throw std::runtime_error("[lattice_target] ANCHOR_INDEX_END must be "
                               "greater than ANCHOR_INDEX_BEGIN for " +
                               spec.target_id);
    }
  }
  if (!spec.train_split.empty() && (spec.anchor_index_begin.has_value() ||
                                    spec.anchor_index_end.has_value())) {
    throw std::runtime_error(
        "[lattice_target] OVER_SPLIT and ANCHOR_INDEX_BEGIN/END are mutually "
        "exclusive for " +
        spec.target_id);
  }
  if (spec.min_optimizer_steps < 0 || spec.min_valid_target_fraction < 0.0 ||
      spec.min_valid_target_fraction > 1.0 ||
      spec.min_observed_input_coverage < 0.0 ||
      spec.min_observed_input_coverage > 1.0 ||
      spec.min_target_supervision_coverage < 0.0 ||
      spec.min_target_supervision_coverage > 1.0 ||
      spec.min_evaluation_metric_coverage < 0.0 ||
      spec.min_evaluation_metric_coverage > 1.0 || spec.max_waves < 0 ||
      spec.require_active_node_head_count < 0 ||
      spec.require_trained_node_head_count < 0 ||
      spec.require_evaluated_node_head_count < 0) {
    throw std::runtime_error("[lattice_target] numeric target thresholds are "
                             "outside their valid range for " +
                             spec.target_id);
  }
  const bool requires_exposure_coverage =
      spec.min_observed_input_coverage > 0.0 ||
      spec.min_target_supervision_coverage > 0.0 ||
      spec.min_evaluation_metric_coverage > 0.0;
  if (requires_exposure_coverage && source_range != "anchor_index") {
    throw std::runtime_error(
        "[lattice_target] exposure coverage thresholds require "
        "SOURCE_RANGE=anchor_index for " +
        spec.target_id);
  }
  if (!spec.forbid_split.empty() &&
      (spec.forbid_exposure_anchor_index_begin.has_value() ||
       spec.forbid_exposure_anchor_index_end.has_value())) {
    throw std::runtime_error(
        "[lattice_target] FORBID_SPLIT and explicit forbidden exposure ranges "
        "are mutually exclusive for " +
        spec.target_id);
  }
  if (spec.forbid_exposure_anchor_index_begin.has_value() !=
      spec.forbid_exposure_anchor_index_end.has_value()) {
    throw std::runtime_error(
        "[lattice_target] explicit forbidden exposure ranges require both "
        "begin and end for " +
        spec.target_id);
  }
  if (spec.forbid_exposure_anchor_index_begin.has_value() &&
      *spec.forbid_exposure_anchor_index_end <=
          *spec.forbid_exposure_anchor_index_begin) {
    throw std::runtime_error(
        "[lattice_target] forbidden exposure end must be greater than begin "
        "for " +
        spec.target_id);
  }
  if ((!spec.forbid_split.empty() ||
       spec.forbid_exposure_anchor_index_begin.has_value()) &&
      spec.forbid_exposure_uses.empty() &&
      !spec.forbid_exposure_uses_from_split_policy) {
    throw std::runtime_error("[lattice_target] forbidden exposure ranges "
                             "require exposure uses for " +
                             spec.target_id);
  }
  if (spec.forbid_split.empty() &&
      !spec.forbid_exposure_anchor_index_begin.has_value() &&
      !spec.forbid_exposure_uses.empty()) {
    throw std::runtime_error(
        "[lattice_target] forbidden exposure uses require a forbidden range "
        "for " +
        spec.target_id);
  }
  for (const auto &warning : spec.warning_specs) {
    if ((warning.kind == "exposure_load" || warning.kind == "effort_density") &&
        warning.split.empty() && !warning.anchor_index_begin.has_value() &&
        source_range != "anchor_index") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN " + warning.kind +
          " requires SPLIT, explicit ANCHOR_INDEX_BEGIN/END, or an "
          "anchor_index target range for " +
          spec.target_id);
    }
  }
}

inline void append_string_vector_text(std::ostringstream &out,
                                      const std::string &name,
                                      std::vector<std::string> values) {
  std::sort(values.begin(), values.end());
  out << name << "=";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  out << "\n";
}

inline void
append_clause_fields_text(std::ostringstream &out,
                          const std::vector<lattice_clause_field_t> &fields) {
  for (const auto &field : fields) {
    out << "field." << field.key << "=" << field.value << "\n";
  }
}

[[nodiscard]] inline std::string
canonical_lattice_target_spec_text(const lattice_target_spec_t &spec) {
  std::ostringstream out;
  out << "target_id=" << spec.target_id << "\n";
  out << "target_class=" << spec.target_class << "\n";
  out << "kind=" << lattice_target_kind_name(spec.kind) << "\n";
  out << "component=" << spec.component << "\n";
  out << "checkpoint_source=" << spec.checkpoint_source << "\n";
  out << "evaluated_checkpoint_source=" << spec.evaluated_checkpoint_source
      << "\n";
  out << "source_range=" << spec.source_range << "\n";
  out << "train_split=" << spec.train_split << "\n";
  out << "anchor_index_begin=";
  if (spec.anchor_index_begin.has_value()) {
    out << *spec.anchor_index_begin;
  }
  out << "\nanchor_index_end=";
  if (spec.anchor_index_end.has_value()) {
    out << *spec.anchor_index_end;
  }
  out << "\nupstream_target_id=" << spec.upstream_target_id << "\n";
  out << "require_contract_match="
      << (spec.require_contract_match ? "true" : "false") << "\n";
  out << "require_component_match="
      << (spec.require_component_match ? "true" : "false") << "\n";
  out << "require_checkpoint_exists="
      << (spec.require_checkpoint_exists ? "true" : "false") << "\n";
  out << "require_finite_loss=" << (spec.require_finite_loss ? "true" : "false")
      << "\n";
  out << "min_optimizer_steps=" << spec.min_optimizer_steps << "\n";
  out << "min_valid_target_fraction=" << spec.min_valid_target_fraction << "\n";
  out << "require_active_node_head_count="
      << spec.require_active_node_head_count << "\n";
  out << "require_trained_node_head_count="
      << spec.require_trained_node_head_count << "\n";
  out << "require_evaluated_node_head_count="
      << spec.require_evaluated_node_head_count << "\n";
  out << "min_observed_input_coverage=" << spec.min_observed_input_coverage
      << "\n";
  out << "min_target_supervision_coverage="
      << spec.min_target_supervision_coverage << "\n";
  out << "min_evaluation_metric_coverage="
      << spec.min_evaluation_metric_coverage << "\n";
  out << "forbid_split=" << spec.forbid_split << "\n";
  out << "forbid_exposure_anchor_index_begin=";
  if (spec.forbid_exposure_anchor_index_begin.has_value()) {
    out << *spec.forbid_exposure_anchor_index_begin;
  }
  out << "\nforbid_exposure_anchor_index_end=";
  if (spec.forbid_exposure_anchor_index_end.has_value()) {
    out << *spec.forbid_exposure_anchor_index_end;
  }
  out << "\nforbid_exposure_uses=";
  for (std::size_t i = 0; i < spec.forbid_exposure_uses.size(); ++i) {
    if (i != 0) {
      out << "|";
    }
    out << cuwacunu::kikijyeba::lattice::exposure::exposure_use_name(
        spec.forbid_exposure_uses[i]);
  }
  out << "\nforbid_exposure_uses_from_split_policy="
      << (spec.forbid_exposure_uses_from_split_policy ? "true" : "false")
      << "\n";
  out << "forbid_exposure_requires_mutated_component="
      << (spec.forbid_exposure_requires_mutated_component ? "true" : "false")
      << "\n";
  out << "plan_mode=" << spec.plan_mode << "\n";
  out << "plan_input_mdn_checkpoint=" << spec.plan_input_mdn_checkpoint << "\n";
  out << "plan_input_representation_checkpoint="
      << spec.plan_input_representation_checkpoint << "\n";
  out << "max_waves=" << spec.max_waves << "\n";
  for (const auto &warning : spec.warning_specs) {
    out << "warning.id=" << warning.warning_id << "\n";
    out << "warning.kind=" << warning.kind << "\n";
    out << "warning.split=" << warning.split << "\n";
    out << "warning.anchor_index_begin=";
    if (warning.anchor_index_begin.has_value()) {
      out << *warning.anchor_index_begin;
    }
    out << "\nwarning.anchor_index_end=";
    if (warning.anchor_index_end.has_value()) {
      out << *warning.anchor_index_end;
    }
    out << "\nwarning.use="
        << cuwacunu::kikijyeba::lattice::exposure::exposure_use_name(
               warning.use)
        << "\n";
    out << "warning.scope=" << warning.scope << "\n";
    out << "warning.require_mutated_component="
        << (warning.require_mutated_component ? "true" : "false") << "\n";
    out << "warning.cursor_epochs_above=" << warning.cursor_epochs_above
        << "\n";
    out << "warning.metric=" << warning.metric << "\n";
    out << "warning.above=" << warning.above << "\n";
    out << "warning.accepted_fraction_below=" << warning.accepted_fraction_below
        << "\n";
    out << "warning.skipped_failed_fetch_probe_above=";
    if (warning.skipped_failed_fetch_probe_above.has_value()) {
      out << *warning.skipped_failed_fetch_probe_above;
    }
    out << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string canonical_lattice_compiled_target_text(
    const lattice_target_compiled_t &target) {
  std::ostringstream out;
  out << canonical_lattice_target_spec_text(target.lowered_v0);
  append_string_vector_text(out, "applied_profiles", target.applied_profiles);
  append_string_vector_text(out, "applied_guards", target.applied_guards);
  for (const auto &clause : target.dependencies) {
    out << "dependency=" << clause.clause_id << "\n";
    append_clause_fields_text(out, clause.fields);
  }
  for (const auto &clause : target.requirements) {
    out << "requirement=" << clause.clause_id << "\n";
    append_clause_fields_text(out, clause.fields);
  }
  for (const auto &clause : target.forbids) {
    out << "forbid=" << clause.clause_id << "\n";
    append_clause_fields_text(out, clause.fields);
  }
  for (const auto &clause : target.plans) {
    out << "plan=" << clause.clause_id << "\n";
    append_clause_fields_text(out, clause.fields);
  }
  for (const auto &clause : target.warning_clauses) {
    out << "warning=" << clause.clause_id << "\n";
    append_clause_fields_text(out, clause.fields);
  }
  return out.str();
}

[[nodiscard]] inline std::string
lattice_target_spec_fingerprint(const lattice_target_compiled_t &target) {
  return cuwacunu::kikijyeba::lattice::exposure::exposure_digest_for_text(
      std::string("kikijyeba.lattice.target.compiled.v1\n") +
      canonical_lattice_compiled_target_text(target));
}

[[nodiscard]] inline cuwacunu::piaabo::parse::simple_kv::block_t
merge_profile_into_target_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &target_block,
    const std::unordered_map<
        std::string, cuwacunu::piaabo::parse::simple_kv::block_t> &profiles) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto profile_id = kv::optional(target_block, "USE_PROFILE", "");
  if (profile_id.empty()) {
    return target_block;
  }
  const auto it = profiles.find(profile_id);
  if (it == profiles.end()) {
    throw std::runtime_error("[lattice_target] unknown USE_PROFILE " +
                             profile_id + " for target " +
                             kv::optional(target_block, "TARGET_ID", "<none>"));
  }
  auto merged = it->second;
  merged.name = "LATTICE_TARGET";
  merged.values.erase("PROFILE_ID");
  for (const auto &[key, value] : target_block.values) {
    merged.values[key] = value;
  }
  return merged;
}

[[nodiscard]] inline std::vector<lattice_target_compiled_t>
decode_lattice_compiled_targets_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  std::unordered_map<std::string, kv::block_t> profiles;
  std::unordered_map<std::string, lattice_guard_spec_t> guards;
  std::unordered_map<std::string, std::vector<kv::block_t>> depends_clauses;
  std::unordered_map<std::string, std::vector<kv::block_t>> requires_clauses;
  std::unordered_map<std::string, std::vector<kv::block_t>> forbids_clauses;
  std::unordered_map<std::string, std::vector<kv::block_t>> plan_clauses;
  std::unordered_map<std::string, std::vector<kv::block_t>> warning_clauses;
  std::vector<lattice_target_compiled_t> out;
  const auto blocks = kv::parse_blocks(dsl_text);
  const auto append_target_clause =
      [](std::unordered_map<std::string, std::vector<kv::block_t>> &clauses,
         const kv::block_t &block) {
        clauses[kv::required(block, "TARGET_ID")].push_back(block);
      };
  for (const auto &block : blocks) {
    if (block.name == "LATTICE_PROFILE") {
      const auto profile_id = kv::required(block, "PROFILE_ID");
      if (!profiles.emplace(profile_id, block).second) {
        throw std::runtime_error("[lattice_target] duplicate PROFILE_ID: " +
                                 profile_id);
      }
      continue;
    }
    if (block.name == "LATTICE_GUARD") {
      auto guard = decode_lattice_guard_spec_block(block);
      const auto guard_id = guard.guard_id;
      if (!guards.emplace(guard_id, std::move(guard)).second) {
        throw std::runtime_error("[lattice_target] duplicate GUARD_ID: " +
                                 guard_id);
      }
      continue;
    }
    if (block.name == "LATTICE_DEPENDS") {
      append_target_clause(depends_clauses, block);
      continue;
    }
    if (block.name == "LATTICE_REQUIRES") {
      append_target_clause(requires_clauses, block);
      continue;
    }
    if (block.name == "LATTICE_FORBIDS") {
      append_target_clause(forbids_clauses, block);
      continue;
    }
    if (block.name == "LATTICE_PLAN") {
      append_target_clause(plan_clauses, block);
      continue;
    }
    if (block.name == "LATTICE_WARN") {
      append_target_clause(warning_clauses, block);
      continue;
    }
    if (block.name != "LATTICE_TARGET") {
      throw std::runtime_error("[lattice_target] unexpected block: " +
                               block.name);
    }
  }
  for (const auto &block : blocks) {
    if (block.name != "LATTICE_TARGET") {
      continue;
    }
    lattice_target_compiled_t compiled{};
    const auto profile_id = kv::optional(block, "USE_PROFILE", "");
    if (!profile_id.empty()) {
      compiled.applied_profiles.push_back(profile_id);
    }
    const auto merged = merge_profile_into_target_block(block, profiles);
    auto spec = decode_lattice_target_spec_block(merged);
    if (!spec.guard_id.empty()) {
      const auto guard_it = guards.find(spec.guard_id);
      if (guard_it == guards.end()) {
        throw std::runtime_error("[lattice_target] unknown GUARD " +
                                 spec.guard_id + " for target " +
                                 spec.target_id);
      }
      compiled.applied_guards.push_back(spec.guard_id);
      compiled.forbids.push_back(make_lattice_forbid_clause_from_guard(
          spec.target_id, guard_it->second));
      apply_lattice_guard(spec, guard_it->second);
    }
    if (spec.forbid_exposure_uses_from_split_policy) {
      compiled.forbids.push_back(
          make_lattice_forbid_clause_from_split_protection(spec));
    }
    std::size_t clause_index = 0;
    for (const auto &clause : depends_clauses[spec.target_id]) {
      compiled.dependencies.push_back(
          make_lattice_dependency_clause(clause, clause_index++));
      apply_lattice_depends_clause(spec, clause);
    }
    clause_index = 0;
    for (const auto &clause : requires_clauses[spec.target_id]) {
      compiled.requirements.push_back(
          make_lattice_requirement_clause(clause, clause_index++));
      apply_lattice_requires_clause(spec, clause);
    }
    clause_index = 0;
    for (const auto &clause : forbids_clauses[spec.target_id]) {
      compiled.forbids.push_back(
          make_lattice_forbid_clause(clause, clause_index++));
      apply_lattice_forbids_clause(spec, clause);
    }
    clause_index = 0;
    for (const auto &clause : plan_clauses[spec.target_id]) {
      compiled.plans.push_back(
          make_lattice_plan_clause(clause, clause_index++));
      apply_lattice_plan_clause(spec, clause);
    }
    clause_index = 0;
    for (const auto &clause : warning_clauses[spec.target_id]) {
      compiled.warning_clauses.push_back(
          make_lattice_warning_clause(clause, clause_index++));
      apply_lattice_warn_clause(spec, clause);
    }
    validate_lattice_target_spec_lowered(spec);
    for (const auto &existing : out) {
      if (existing.lowered_v0.target_id == spec.target_id) {
        throw std::runtime_error("[lattice_target] duplicate TARGET_ID: " +
                                 spec.target_id);
      }
    }
    compiled.warnings = spec.language_warnings;
    compiled.lowered_v0 = std::move(spec);
    compiled.target_spec_fingerprint =
        lattice_target_spec_fingerprint(compiled);
    out.push_back(std::move(compiled));
  }
  std::set<std::string> target_ids;
  for (const auto &target : out) {
    target_ids.insert(target.lowered_v0.target_id);
  }
  const auto check_clause_targets = [&target_ids](
                                        const auto &clauses,
                                        const std::string &block_name) {
    for (const auto &[target_id, _] : clauses) {
      if (target_ids.find(target_id) == target_ids.end()) {
        throw std::runtime_error("[lattice_target] " + block_name +
                                 " references unknown TARGET_ID: " + target_id);
      }
    }
  };
  check_clause_targets(depends_clauses, "LATTICE_DEPENDS");
  check_clause_targets(requires_clauses, "LATTICE_REQUIRES");
  check_clause_targets(forbids_clauses, "LATTICE_FORBIDS");
  check_clause_targets(plan_clauses, "LATTICE_PLAN");
  check_clause_targets(warning_clauses, "LATTICE_WARN");
  if (out.empty()) {
    throw std::runtime_error("[lattice_target] no LATTICE_TARGET blocks found");
  }
  return out;
}

[[nodiscard]] inline std::vector<lattice_target_spec_t>
decode_lattice_targets_from_dsl(const std::string &dsl_text) {
  std::vector<lattice_target_spec_t> out;
  for (auto &target : decode_lattice_compiled_targets_from_dsl(dsl_text)) {
    out.push_back(std::move(target.lowered_v0));
  }
  return out;
}

[[nodiscard]] inline std::string
lattice_target_selector_key(const lattice_target_spec_t &spec) {
  std::ostringstream out;
  out << lattice_target_kind_name(spec.kind) << "|";
  out << spec.component << "|";
  out << spec.target_class << "|";
  out << spec.checkpoint_source << "|";
  out << spec.evaluated_checkpoint_source << "|";
  out << spec.source_range << "|";
  out << spec.train_split << "|";
  if (spec.anchor_index_begin.has_value()) {
    out << *spec.anchor_index_begin;
  }
  out << "|";
  if (spec.anchor_index_end.has_value()) {
    out << *spec.anchor_index_end;
  }
  return out.str();
}

[[nodiscard]] inline lattice_target_validation_report_t
validate_lattice_target_specs(const std::vector<lattice_target_spec_t> &specs) {
  lattice_target_validation_report_t report{};
  std::set<std::string> seen_selectors;
  for (const auto &spec : specs) {
    for (const auto &warning : spec.language_warnings) {
      report.warnings.push_back(warning);
    }
    const auto selector = lattice_target_selector_key(spec);
    if (!seen_selectors.insert(selector).second) {
      report.warnings.push_back(
          "[lattice_target] duplicate target selector for component=" +
          spec.component + " source_range=" + spec.source_range +
          "; multiple targets may be intentional later, but v0 cannot "
          "distinguish them without split/coverage semantics");
    }
  }
  return report;
}

} // namespace cuwacunu::kikijyeba::lattice::target
