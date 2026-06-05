// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::hero::lattice::target {

inline constexpr const char *k_lattice_fact_identity_contract_schema_v1 =
    cuwacunu::hero::lattice::exposure::
        k_lattice_fact_identity_contract_schema_v1;
inline constexpr const char *k_lattice_fact_identity_contract_id_v1 = cuwacunu::
    hero::lattice::exposure::k_lattice_fact_identity_contract_id_v1;

enum class lattice_target_kind_t {
  not_applicable,
  vicreg_ready,
  mtf_representation_ready,
  channel_mdn_ready,
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
  cuwacunu::hero::lattice::exposure::exposure_use_t use{
      cuwacunu::hero::lattice::exposure::exposure_use_t::observed_input};
  std::string scope{"target_component_family_id"};
  bool require_mutated_component{true};
  double cursor_epochs_above{std::numeric_limits<double>::quiet_NaN()};
  std::string metric{};
  double above{std::numeric_limits<double>::quiet_NaN()};
  double below{std::numeric_limits<double>::quiet_NaN()};
  double accepted_fraction_below{std::numeric_limits<double>::quiet_NaN()};
  std::optional<std::int64_t> skipped_failed_fetch_probe_above{std::nullopt};
};

struct lattice_representation_geometry_requirement_spec_t {
  std::string requirement_id{};
  std::string metric{};
  std::string op{"ge"};
  double value{std::numeric_limits<double>::quiet_NaN()};
  bool require_mutated_component{true};
};

struct lattice_target_spec_t {
  std::string target_id{};
  std::string target_spec_fingerprint{};
  std::string use_profile{};
  std::string guard_id{};
  std::string target_class{"readiness"};
  lattice_target_kind_t kind{lattice_target_kind_t::not_applicable};
  bool target_kind_applicable{true};
  std::string proof_kind{};
  std::string subject_fact_family{};
  std::string protocol_id{};
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
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t>
      forbid_exposure_uses{};
  bool forbid_exposure_uses_from_split_policy{false};
  bool forbid_exposure_requires_mutated_component{true};
  std::string plan_mode{"train | debug"};
  std::string plan_input_mdn_checkpoint{};
  std::string plan_input_representation_checkpoint{};
  std::int64_t max_waves{1};
  std::vector<lattice_warning_spec_t> warning_specs{};
  std::vector<lattice_representation_geometry_requirement_spec_t>
      representation_geometry_requirements{};
  std::vector<std::string> language_warnings{};
};

struct lattice_guard_spec_t {
  std::string guard_id{};
  std::string split{};
  std::optional<std::size_t> forbid_exposure_anchor_index_begin{std::nullopt};
  std::optional<std::size_t> forbid_exposure_anchor_index_end{std::nullopt};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t> uses{};
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

struct lattice_policy_gate_spec_t {
  std::string policy_id{};
  std::string policy_kind{};
  std::string target_id{};
  std::string metric{};
  std::string metric_definition{};
  std::string baseline{};
  std::string baseline_definition{};
  double threshold{std::numeric_limits<double>::quiet_NaN()};
  std::string uncertainty_policy{};
  std::string uncertainty_model{};
  double support_minimum{std::numeric_limits<double>::quiet_NaN()};
  std::string selector_split{};
  std::string anti_leakage_policy{};
  std::string tie_policy{};
  std::string negative_tests{};
  std::string calibration_requirements{};
  std::string holdout_declaration{};
  std::string threshold_selection_audit{};
  bool enabled{false};
  std::string policy_fingerprint{};
  std::string status{"disabled_reserved"};
  std::string disabled_reason{
      "policy gates are reserved until metric and baseline definitions, "
      "thresholds, uncertainty policy/model, support minimums, selector split, "
      "leakage policy, negative tests, calibration requirements, holdout "
      "declaration, and threshold-selection audit are explicit"};
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
  std::string report_schema_id{};
  std::int64_t report_schema_version{0};
  std::string report_writer_id{};
  std::string report_writer_version{};
  std::string protocol_contract_fingerprint{};
  std::string protocol_id{};
  std::string component_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string config_bundle_id{};
  std::string config_receipt_id{};
  std::string component_spawn_registry_id{};
  std::string component_family_id{};
  std::string component_spawn_fingerprint{};
  std::string component_spawn_id{};
  std::string component_spawn_label{};
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
  std::filesystem::path checkpoint_path_reported{};
  std::string checkpoint_digest_reported{};
  std::string checkpoint_digest_actual{};
  bool checkpoint_file_exists{false};
  bool checkpoint_digest_verified{false};
  std::int64_t checkpoint_bytes{0};
  std::filesystem::path representation_checkpoint_path{};
  bool representation_checkpoint_loaded{false};
  std::filesystem::path mdn_checkpoint_path{};
  bool mdn_checkpoint_loaded{false};
  bool allow_untrained_representation{false};
};

inline constexpr const char *k_lattice_leakage_rule =
    "protected_split_dilation_intersection";

struct lattice_leakage_rule_entry_t {
  std::string rule{};
  std::string protected_set_algebra{};
  std::string dilation_kernel{};
  std::string predicate{};
  std::string witness_basis{};
  bool requires_closure_complete{true};
  bool protected_range_contains_base{true};
};

[[nodiscard]] inline std::vector<lattice_leakage_rule_entry_t>
lattice_leakage_rule_vocabulary() {
  return {{k_lattice_leakage_rule, "temporal_morphological_dilation",
           "[-dilation_left,+dilation_right]",
           "closure_forbidden_use_intersection_empty",
           "labeled_causal_exposure_reachability", true, true}};
}

struct lattice_leakage_rule_summary_t {
  std::string schema{"kikijyeba.lattice.leakage_rule.summary.v1"};
  std::int64_t rule_count{0};
  std::int64_t temporal_dilation_rule_count{0};
  std::int64_t closure_complete_required_count{0};
  std::int64_t protected_contains_base_count{0};
  std::int64_t empty_field_count{0};
  bool single_v1_rule{false};
  bool protected_split_dilation_intersection_rule_present{false};
  bool predicate_is_forbidden_use_empty_intersection{false};
  bool witness_basis_is_labeled_causal_reachability{false};
  bool closure_completeness_required{false};
  bool protected_range_contains_base{false};
  bool all_rules_have_policy_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> rule_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_leakage_rule_summary_t
lattice_leakage_rule_summary() {
  lattice_leakage_rule_summary_t out{};
  const auto vocabulary = lattice_leakage_rule_vocabulary();
  out.rule_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_rule = [&](const std::string &rule) {
    return std::find_if(vocabulary.begin(), vocabulary.end(),
                        [&](const auto &entry) { return entry.rule == rule; });
  };

  for (const auto &entry : vocabulary) {
    out.rule_names.push_back(entry.rule);
    if (entry.protected_set_algebra == "temporal_morphological_dilation") {
      ++out.temporal_dilation_rule_count;
    }
    if (entry.requires_closure_complete) {
      ++out.closure_complete_required_count;
    }
    if (entry.protected_range_contains_base) {
      ++out.protected_contains_base_count;
    }
    if (entry.rule.empty() || entry.protected_set_algebra.empty() ||
        entry.dilation_kernel.empty() || entry.predicate.empty() ||
        entry.witness_basis.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto rule = find_rule(k_lattice_leakage_rule);
  out.single_v1_rule = out.rule_count == 1;
  out.protected_split_dilation_intersection_rule_present =
      rule != vocabulary.end() &&
      rule->protected_set_algebra == "temporal_morphological_dilation" &&
      rule->dilation_kernel == "[-dilation_left,+dilation_right]";
  out.predicate_is_forbidden_use_empty_intersection =
      rule != vocabulary.end() &&
      rule->predicate == "closure_forbidden_use_intersection_empty";
  out.witness_basis_is_labeled_causal_reachability =
      rule != vocabulary.end() &&
      rule->witness_basis == "labeled_causal_exposure_reachability";
  out.closure_completeness_required =
      out.closure_complete_required_count == out.rule_count &&
      out.rule_count > 0;
  out.protected_range_contains_base =
      out.protected_contains_base_count == out.rule_count && out.rule_count > 0;
  out.all_rules_have_policy_text = out.empty_field_count == 0;

  if (!out.single_v1_rule) {
    out.summary_issues.push_back(
        "leakage rule vocabulary must contain exactly one V1 rule");
  }
  if (!out.protected_split_dilation_intersection_rule_present) {
    out.summary_issues.push_back(
        "V1 leakage rule must be protected_split_dilation_intersection with "
        "temporal dilation");
  }
  if (!out.predicate_is_forbidden_use_empty_intersection) {
    out.summary_issues.push_back(
        "V1 leakage predicate must be empty forbidden-use intersection");
  }
  if (!out.witness_basis_is_labeled_causal_reachability) {
    out.summary_issues.push_back(
        "V1 leakage witnesses must be labeled causal exposure reachability");
  }
  if (!out.closure_completeness_required) {
    out.summary_issues.push_back(
        "V1 leakage proof must require complete checkpoint closure");
  }
  if (!out.protected_range_contains_base) {
    out.summary_issues.push_back(
        "protected split range must contain the undilated base split");
  }
  if (!out.all_rules_have_policy_text) {
    out.summary_issues.push_back(
        "leakage rule rows must declare rule, algebra, kernel, predicate, and "
        "witness basis");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_causal_edge_entry_t {
  std::string edge{};
  std::string source{};
  std::string target{};
  std::string proof_field{};
  bool closure_reachability_edge{false};
  bool leakage_relevant_when_forbidden{false};
};

[[nodiscard]] inline std::vector<lattice_causal_edge_entry_t>
lattice_causal_edge_vocabulary() {
  return {{"observed_input", "source_row_footprint", "runtime_job",
           "closure.causal_exposures[].observed_footprint", false, true},
          {"target_supervision", "source_row_footprint", "runtime_job",
           "closure.causal_exposures[].target_footprint", false, true},
          {"evaluation_metric", "source_row_footprint", "runtime_job",
           "closure.causal_exposures[].anchor_range", false, true},
          {"selection_signal", "source_row_footprint", "runtime_job",
           "closure.causal_exposures[].anchor_range", false, true},
          {"loaded_checkpoint", "checkpoint", "runtime_job",
           "closure.causal_exposures[].input_checkpoints", true, false},
          {"created_checkpoint", "runtime_job", "checkpoint",
           "closure.causal_exposures[].output_checkpoint", true, false},
          {"mutated_component", "runtime_job", "component_state",
           "closure.causal_exposures[].mutated_component", false, true}};
}

struct lattice_causal_edge_summary_t {
  std::string schema{"kikijyeba.lattice.causal_edge.summary.v1"};
  std::int64_t edge_count{0};
  std::int64_t source_row_edge_count{0};
  std::int64_t checkpoint_reachability_edge_count{0};
  std::int64_t leakage_relevant_edge_count{0};
  std::int64_t mutation_edge_count{0};
  std::int64_t checkpoint_edge_leakage_overlap_count{0};
  std::int64_t empty_field_count{0};
  bool source_row_use_edges_present{false};
  bool checkpoint_load_create_edges_present{false};
  bool mutation_edge_present{false};
  bool checkpoint_reachability_distinct_from_leakage_edges{false};
  bool selection_signal_is_leakage_relevant{false};
  bool evaluation_metric_is_leakage_relevant_when_forbidden{false};
  bool all_edges_have_proof_fields{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> edge_names{};
  std::vector<std::string> missing_required_edges{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_causal_edge_summary_t
lattice_causal_edge_summary() {
  lattice_causal_edge_summary_t out{};
  const auto vocabulary = lattice_causal_edge_vocabulary();
  out.edge_count = static_cast<std::int64_t>(vocabulary.size());

  std::set<std::string> edge_names;
  for (const auto &entry : vocabulary) {
    edge_names.insert(entry.edge);
    out.edge_names.push_back(entry.edge);
    if (entry.source == "source_row_footprint") {
      ++out.source_row_edge_count;
    }
    if (entry.closure_reachability_edge) {
      ++out.checkpoint_reachability_edge_count;
    }
    if (entry.leakage_relevant_when_forbidden) {
      ++out.leakage_relevant_edge_count;
    }
    if (entry.edge == "mutated_component") {
      ++out.mutation_edge_count;
    }
    if (entry.closure_reachability_edge &&
        entry.leakage_relevant_when_forbidden) {
      ++out.checkpoint_edge_leakage_overlap_count;
    }
    if (entry.edge.empty() || entry.source.empty() || entry.target.empty() ||
        entry.proof_field.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto has_edge = [&](const std::string &edge) {
    return edge_names.find(edge) != edge_names.end();
  };
  const std::vector<std::string> required_edges{
      "observed_input",   "target_supervision", "evaluation_metric",
      "selection_signal", "loaded_checkpoint",  "created_checkpoint",
      "mutated_component"};
  for (const auto &edge : required_edges) {
    if (!has_edge(edge)) {
      out.missing_required_edges.push_back(edge);
    }
  }
  const auto find_edge = [&](const std::string &edge) {
    return std::find_if(vocabulary.begin(), vocabulary.end(),
                        [&](const auto &entry) { return entry.edge == edge; });
  };
  const auto selection_signal = find_edge("selection_signal");
  const auto evaluation_metric = find_edge("evaluation_metric");

  out.source_row_use_edges_present =
      has_edge("observed_input") && has_edge("target_supervision") &&
      has_edge("evaluation_metric") && has_edge("selection_signal") &&
      out.source_row_edge_count == 4;
  out.checkpoint_load_create_edges_present =
      has_edge("loaded_checkpoint") && has_edge("created_checkpoint") &&
      out.checkpoint_reachability_edge_count == 2;
  out.mutation_edge_present =
      has_edge("mutated_component") && out.mutation_edge_count == 1;
  out.checkpoint_reachability_distinct_from_leakage_edges =
      out.checkpoint_edge_leakage_overlap_count == 0;
  out.selection_signal_is_leakage_relevant =
      selection_signal != vocabulary.end() &&
      selection_signal->leakage_relevant_when_forbidden;
  out.evaluation_metric_is_leakage_relevant_when_forbidden =
      evaluation_metric != vocabulary.end() &&
      evaluation_metric->leakage_relevant_when_forbidden;
  out.all_edges_have_proof_fields = out.empty_field_count == 0;

  if (out.edge_count != 7) {
    out.summary_issues.push_back(
        "causal edge vocabulary must contain 7 V1 edge labels");
  }
  if (!out.missing_required_edges.empty()) {
    out.summary_issues.push_back(
        "causal edge vocabulary is missing required V1 edge labels");
  }
  if (!out.source_row_use_edges_present) {
    out.summary_issues.push_back(
        "causal edge vocabulary must include four source-row use edges");
  }
  if (!out.checkpoint_load_create_edges_present) {
    out.summary_issues.push_back(
        "causal edge vocabulary must include loaded and created checkpoint "
        "reachability edges");
  }
  if (!out.mutation_edge_present) {
    out.summary_issues.push_back(
        "causal edge vocabulary must include the mutated component edge");
  }
  if (!out.checkpoint_reachability_distinct_from_leakage_edges) {
    out.summary_issues.push_back("checkpoint reachability edges must not be "
                                 "direct forbidden-use leakage "
                                 "edges");
  }
  if (!out.selection_signal_is_leakage_relevant) {
    out.summary_issues.push_back(
        "selection_signal source-row edges must be leakage relevant when "
        "forbidden");
  }
  if (!out.evaluation_metric_is_leakage_relevant_when_forbidden) {
    out.summary_issues.push_back("evaluation_metric source-row edges must "
                                 "remain leakage relevant when a "
                                 "rule forbids them");
  }
  if (!out.all_edges_have_proof_fields) {
    out.summary_issues.push_back(
        "causal edge rows must declare proof certificate fields");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_selection_signal_policy_entry_t {
  std::string policy{};
  std::string relation{};
  std::string proof_field{};
  std::string source_footprint_basis{};
  std::string status_effect{};
  std::string v1_scope{};
  bool forbidden_use{false};
  bool requires_closure_complete{true};
  bool first_class_event_stream{false};
  bool runtime_executor{false};
};

[[nodiscard]] inline std::vector<lattice_selection_signal_policy_entry_t>
lattice_selection_signal_policy_vocabulary() {
  return {
      {"selection_signal_forbidden_use",
       "forbidden_overlap(T,W) :- selection_signal_use(W), "
       "intersects(source_footprint(W), protected_split(T))",
       "proof_certificate.leakage.forbidden_uses[]",
       "closure.causal_exposures[].anchor_range", "exposure_failed",
       "selection_signal is a forbidden causal use for protected holdouts",
       true, true, false, false},
      {"selection_signal_causal_edge",
       "selection_signal_edge(F) :- causal_exposure(F), "
       "contains_use(F,selection_signal)",
       "closure.causal_exposures[].uses",
       "closure.causal_exposures[].anchor_range",
       "leakage_relevant_when_forbidden",
       "the selected anchors are the source footprint until structured "
       "selection events exist",
       true, true, false, false},
      {"selection_signal_event_stream_status",
       "first_class_selection_event(F) :- lattice_selection_signal_fact(F)",
       "scan_exposure.selection_signal_facts[]",
       "lattice.selection_signal.selection_footprint",
       "first_class_event_stream_v2",
       "V2 exposes read-only structured model-selection provenance events; "
       "they do not alter contract identity or coverage authority",
       false, true, true, false},
      {"selection_signal_path_policy",
       "no path: protected_holdout evaluation_metric -> selection_signal -> "
       "checkpoint_choice -> reported_model",
       "scan_exposure.selection_signal_facts[]",
       "selection event parent exposure plus selected checkpoint path",
       "known_selector_path_fail_closed_when_forbidden",
       "V2 blocks known selection_signal exposure paths through the parent "
       "exposure closure; arbitrary cross-event selector graphs remain future "
       "work",
       false, true, false, false}};
}

struct lattice_selection_signal_policy_summary_t {
  std::string schema{"kikijyeba.lattice.selection_signal_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t forbidden_use_policy_count{0};
  std::int64_t requires_closure_complete_count{0};
  std::int64_t first_class_event_stream_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t exposure_failed_policy_count{0};
  std::int64_t future_policy_count{0};
  std::int64_t empty_field_count{0};
  bool forbidden_use_policy_present{false};
  bool causal_edge_policy_present{false};
  bool event_stream_deferred{false};
  bool event_stream_available{false};
  bool path_policy_future_fail_closed{false};
  bool known_selector_path_fail_closed{false};
  bool anchor_range_is_v1_source_footprint{false};
  bool all_policies_require_closure_complete{false};
  bool no_first_class_event_stream_in_v1{false};
  bool first_class_event_stream_is_read_only{false};
  bool no_runtime_executor{false};
  bool all_policies_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> policy_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_selection_signal_policy_summary_t
lattice_selection_signal_policy_summary() {
  lattice_selection_signal_policy_summary_t out{};
  const auto vocabulary = lattice_selection_signal_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };

  for (const auto &entry : vocabulary) {
    out.policy_names.push_back(entry.policy);
    if (entry.forbidden_use) {
      ++out.forbidden_use_policy_count;
    }
    if (entry.requires_closure_complete) {
      ++out.requires_closure_complete_count;
    }
    if (entry.first_class_event_stream) {
      ++out.first_class_event_stream_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.status_effect == "exposure_failed") {
      ++out.exposure_failed_policy_count;
    }
    if (entry.proof_field.find("future") != std::string::npos ||
        entry.status_effect.find("future") != std::string::npos ||
        entry.v1_scope.find("future") != std::string::npos) {
      ++out.future_policy_count;
    }
    if (entry.policy.empty() || entry.relation.empty() ||
        entry.proof_field.empty() || entry.source_footprint_basis.empty() ||
        entry.status_effect.empty() || entry.v1_scope.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto forbidden_policy = find_policy("selection_signal_forbidden_use");
  const auto causal_edge_policy = find_policy("selection_signal_causal_edge");
  const auto event_stream_policy =
      find_policy("selection_signal_event_stream_status");
  const auto path_policy = find_policy("selection_signal_path_policy");

  out.forbidden_use_policy_present =
      forbidden_policy != vocabulary.end() && forbidden_policy->forbidden_use &&
      forbidden_policy->status_effect == "exposure_failed";
  out.causal_edge_policy_present =
      causal_edge_policy != vocabulary.end() &&
      causal_edge_policy->forbidden_use &&
      causal_edge_policy->status_effect == "leakage_relevant_when_forbidden";
  out.event_stream_deferred =
      event_stream_policy != vocabulary.end() &&
      !event_stream_policy->first_class_event_stream &&
      event_stream_policy->proof_field == "none_in_v1" &&
      event_stream_policy->status_effect == "non_claiming_future_work";
  out.event_stream_available =
      event_stream_policy != vocabulary.end() &&
      event_stream_policy->first_class_event_stream &&
      event_stream_policy->proof_field ==
          "scan_exposure.selection_signal_facts[]" &&
      event_stream_policy->status_effect == "first_class_event_stream_v2";
  out.path_policy_future_fail_closed =
      path_policy != vocabulary.end() &&
      !path_policy->first_class_event_stream &&
      path_policy->status_effect == "future_fail_closed_selection_leakage";
  out.known_selector_path_fail_closed =
      path_policy != vocabulary.end() &&
      path_policy->proof_field == "scan_exposure.selection_signal_facts[]" &&
      path_policy->status_effect ==
          "known_selector_path_fail_closed_when_forbidden";
  out.anchor_range_is_v1_source_footprint =
      forbidden_policy != vocabulary.end() &&
      causal_edge_policy != vocabulary.end() &&
      forbidden_policy->source_footprint_basis ==
          "closure.causal_exposures[].anchor_range" &&
      causal_edge_policy->source_footprint_basis ==
          "closure.causal_exposures[].anchor_range";
  out.all_policies_require_closure_complete =
      out.requires_closure_complete_count == out.policy_count &&
      out.policy_count > 0;
  out.no_first_class_event_stream_in_v1 =
      out.first_class_event_stream_count == 0;
  out.first_class_event_stream_is_read_only =
      event_stream_policy != vocabulary.end() &&
      event_stream_policy->first_class_event_stream &&
      !event_stream_policy->runtime_executor &&
      event_stream_policy->v1_scope.find("read-only") != std::string::npos;
  out.no_runtime_executor = out.runtime_executor_count == 0;
  out.all_policies_have_claim_text = out.empty_field_count == 0;

  if (out.policy_count != 4) {
    out.summary_issues.push_back(
        "selection-signal policy vocabulary must contain 4 policies");
  }
  if (out.forbidden_use_policy_count != 2) {
    out.summary_issues.push_back(
        "selection-signal policy must expose exactly 2 known forbidden-use V1 "
        "rows");
  }
  if (out.exposure_failed_policy_count != 1) {
    out.summary_issues.push_back("selection-signal policy must have exactly "
                                 "one exposure_failed V1 gate");
  }
  if (!out.forbidden_use_policy_present) {
    out.summary_issues.push_back(
        "selection_signal_forbidden_use policy must be present and failing");
  }
  if (!out.causal_edge_policy_present) {
    out.summary_issues.push_back("selection_signal_causal_edge policy must "
                                 "mark known selection edges as "
                                 "leakage relevant when forbidden");
  }
  if (!out.event_stream_available) {
    out.summary_issues.push_back(
        "first-class selection event streams must be available as V2 scan "
        "evidence");
  }
  if (!out.known_selector_path_fail_closed) {
    out.summary_issues.push_back(
        "known selection paths must fail closed when forbidden");
  }
  if (!out.anchor_range_is_v1_source_footprint) {
    out.summary_issues.push_back("known selection-signal V1 source footprint "
                                 "must be the causal exposure "
                                 "anchor range");
  }
  if (!out.all_policies_require_closure_complete) {
    out.summary_issues.push_back(
        "selection-signal policy checks must require complete checkpoint "
        "closure");
  }
  if (!out.first_class_event_stream_is_read_only) {
    out.summary_issues.push_back(
        "first-class selection event streams must remain read-only lattice "
        "evidence");
  }
  if (!out.no_runtime_executor) {
    out.summary_issues.push_back(
        "selection-signal policy must not execute runtime work");
  }
  if (!out.all_policies_have_claim_text) {
    out.summary_issues.push_back(
        "selection-signal policy rows must declare relation, proof, source, "
        "status, and scope text");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_derived_query_rule_entry_t {
  std::string relation{};
  std::string rule{};
  std::vector<std::string> input_relations{};
  std::string proof_field{};
  std::string fail_closed_policy{};
  bool read_only{true};
  bool db_cacheable{true};
  bool runtime_executor{false};
  std::string projection_scope{"target"};
  std::string projection_quantifier{"single"};
  std::string empty_projection_policy{"not_applicable"};
  std::string result_projection_scope{"target"};
  std::string result_projection_quantifier{"single"};
  std::string result_empty_projection_policy{"not_applicable"};
};

[[nodiscard]] inline std::vector<lattice_derived_query_rule_entry_t>
lattice_derived_query_rule_vocabulary() {
  return {
      {"identity_match",
       "identity_match(T) :- active_identity(I), evidence_identity(T,I), "
       "required_identity_fields_match(T)",
       {"active_identity", "target_evidence"},
       "proof_certificate.proof_context",
       "missing or mismatched required identity blocks satisfaction",
       true,
       true,
       false,
       "target",
       "single",
       "not_applicable",
       "target",
       "single",
       "not_applicable"},
      {"dependency_satisfied",
       "dependency_satisfied(T,D) :- dependency_proof(T,D), "
       "dependency_status(D,satisfied), exact_checkpoint_bindings_hold(D)",
       {"proof_certificate.dependencies"},
       "proof_certificate.dependencies[]",
       "missing, blocked, mismatched, or vacuous bindings block satisfaction",
       true,
       true,
       false,
       "per_dependency_row",
       "per_row",
       "not_applicable",
       "all_dependency_rows_compact",
       "all",
       "vacuously_true"},
      {"all_dependencies_satisfied",
       "all_dependencies_satisfied(T) :- forall D dependency_satisfied(T,D)",
       {"proof_certificate.dependencies", "dependency_satisfied"},
       "proof_certificate.dependencies[]",
       "any dependency proof that is blocked, mismatched, vacuous, or "
       "non-satisfied keeps aggregate dependency satisfaction false",
       true,
       true,
       false,
       "all_dependency_rows_compact",
       "all",
       "vacuously_true",
       "all_dependency_rows_compact",
       "all",
       "vacuously_true"},
      {"coverage_satisfied",
       "coverage_satisfied(T,U) :- coverage_proof(T,U), "
       "idempotent_union_fraction(U) >= required_fraction(U)",
       {"proof_certificate.coverage", "exposure_summaries"},
       "proof_certificate.coverage[]",
       "untrusted or insufficient coverage becomes a coverage deficit",
       true,
       true,
       false,
       "per_coverage_row",
       "per_row",
       "not_applicable",
       "all_coverage_rows_compact",
       "all",
       "vacuously_true"},
      {"all_coverage_satisfied",
       "all_coverage_satisfied(T) :- forall U coverage_satisfied(T,U)",
       {"proof_certificate.coverage", "coverage_satisfied"},
       "proof_certificate.coverage[]",
       "any coverage proof below its idempotent-union threshold keeps "
       "aggregate coverage satisfaction false",
       true,
       true,
       false,
       "all_coverage_rows_compact",
       "all",
       "vacuously_true",
       "all_coverage_rows_compact",
       "all",
       "vacuously_true"},
      {"closure_complete",
       "closure_complete(T,C) :- closure_checked(T,C), "
       "unresolved_input_checkpoints(C)=empty, causal_output_root_reachable(C)",
       {"proof_certificate.closure", "checkpoint_facts"},
       "proof_certificate.closure",
       "unchecked or unresolved lineage is non-claiming or blocking",
       true,
       true,
       false,
       "root_checkpoint_closure",
       "single",
       "not_applicable",
       "root_checkpoint_closure",
       "single",
       "not_applicable"},
      {"checkpoint_ancestor",
       "checkpoint_ancestor(C,A) :- checkpoint_closure(C), "
       "ancestor_exposure(C,F), created_checkpoint(F,A)",
       {"proof_certificate.closure.causal_exposures", "lattice.exposure.fact",
        "checkpoint_facts"},
       "checkpoint_closure.facts[]",
       "unchecked, unresolved, or missing closure witnesses fail closed and "
       "cannot prove ancestry",
       true,
       true,
       false,
       "per_checkpoint_ancestor",
       "per_row",
       "not_applicable",
       "any_checkpoint_ancestor",
       "any",
       "false_when_empty"},
      {"unresolved_lineage",
       "unresolved_lineage(C,W) :- checkpoint_closure(C), "
       "unresolved_input_checkpoint(C,W) or checkpoint_identity_mismatch(C,W)",
       {"proof_certificate.closure", "checkpoint_closure"},
       "checkpoint_closure.unresolved_input_checkpoints[]",
       "any unresolved input checkpoint or checkpoint identity mismatch keeps "
       "lineage non-clean until runtime evidence closes the path",
       true,
       true,
       false,
       "per_unresolved_lineage_witness",
       "per_row",
       "not_applicable",
       "any_unresolved_lineage_witness",
       "any",
       "false_when_empty"},
      {"closure_clean_if_checked",
       "closure_clean_if_checked(T) :- not closure_checked(T); "
       "closure_complete(T)",
       {"proof_certificate.closure", "deficits", "status"},
       "proof_certificate.closure",
       "unchecked closure is acceptable only when no proof obligation requires "
       "it; required unchecked closure is represented by deficits/status",
       true,
       true,
       false,
       "target",
       "single",
       "not_applicable",
       "target",
       "single",
       "not_applicable"},
      {"forbidden_overlap",
       "forbidden_overlap(T,W) :- ancestor_exposure(T,F), forbidden_use(T,F), "
       "intersects(source_footprint(F), protected_split(T))",
       {"proof_certificate.closure.causal_exposures",
        "proof_certificate.leakage"},
       "proof_certificate.leakage.overlap_witnesses[]",
       "a non-empty forbidden intersection makes leakage fail",
       true,
       true,
       false,
       "per_overlap_witness",
       "per_row",
       "not_applicable",
       "any_overlap_witness",
       "any",
       "false_when_empty"},
      {"no_forbidden_overlap",
       "no_forbidden_overlap(T) :- not leakage_checked(T); "
       "not forbidden_overlap(T)",
       {"proof_certificate.leakage", "deficits", "status"},
       "proof_certificate.leakage",
       "unchecked leakage is acceptable only when no protected-split proof "
       "obligation requires it; checked overlap fails closed",
       true,
       true,
       false,
       "target",
       "none",
       "true_when_empty",
       "target",
       "none",
       "true_when_empty"},
      {"warning_triggered",
       "warning_triggered(T,W) :- warning_result(T,W), "
       "measurement_available(W), threshold_triggered(W)",
       {"warning_results"},
       "warning_results[]",
       "unavailable warning measurements remain visible but non-blocking",
       true,
       true,
       false,
       "per_warning_result",
       "per_row",
       "not_applicable",
       "any_warning_result",
       "any",
       "false_when_empty"},
      {"stale_cache",
       "stale_cache(R,I) :- runtime_index_cache_validation(R,I), "
       "cache_valid(I)=false",
       {"runtime_index_cache_validation", "runtime_index_cache"},
       "runtime_index_cache_validation.issues[]",
       "missing, stale, malformed, or authority-mismatched cache rows fail "
       "closed and never upgrade target satisfaction",
       true,
       false,
       false,
       "per_cache_validation_issue",
       "per_row",
       "not_applicable",
       "any_cache_validation_issue",
       "any",
       "false_when_empty"},
      {"plan_deficit",
       "plan_deficit(T,D) :- proof_deficit(T,D), plan_relevant(D), "
       "minimal_priority(D)",
       {"deficits", "plan_basis"},
       "plan_basis",
       "plans are advice over deficits and never execute runtime work",
       true,
       false,
       false,
       "per_plan_relevant_deficit",
       "per_row",
       "not_applicable",
       "any_plan_relevant_deficit",
       "any",
       "false_when_empty"},
      {"proof_certificate_check_passed",
       "proof_certificate_check_passed(T) :- proof_certificate_check(T), "
       "certificate_self_check_passed(T)",
       {"proof_certificate_check", "proof_certificate"},
       "proof_certificate_check",
       "failed certificate self-check keeps satisfaction non-claiming",
       true,
       true,
       false,
       "target",
       "single",
       "not_applicable",
       "target",
       "single",
       "not_applicable"},
      {"no_blocking_deficit",
       "no_blocking_deficit(T) :- proof_deficit_count(T,0)",
       {"deficits", "evidence_order_vector.blocking_deficit_count"},
       "deficits[]",
       "any proof deficit keeps satisfaction non-claiming until resolved",
       true,
       true,
       false,
       "target",
       "none",
       "true_when_empty",
       "target",
       "none",
       "true_when_empty"},
      {"status_satisfied",
       "status_satisfied(T) :- target_status(T,satisfied)",
       {"status"},
       "status",
       "non-satisfied target status prevents readiness even when individual "
       "proof relations are inspectable",
       true,
       true,
       false,
       "target",
       "single",
       "not_applicable",
       "target",
       "single",
       "not_applicable"},
      {"target_satisfied",
       "target_satisfied(T) :- status_satisfied(T), identity_match(T), "
       "all_dependencies_satisfied(T), "
       "all_coverage_satisfied(T), closure_clean_if_checked(T), "
       "no_forbidden_overlap(T), proof_certificate_check_passed(T), "
       "no_blocking_deficit(T)",
       {"status_satisfied", "identity_match", "all_dependencies_satisfied",
        "all_coverage_satisfied", "closure_clean_if_checked",
        "no_forbidden_overlap", "proof_certificate_check_passed",
        "no_blocking_deficit"},
       "status",
       "failure to prove any premise keeps the target unsatisfied",
       true,
       true,
       false,
       "target",
       "all",
       "fixed_nonempty",
       "target",
       "all",
       "fixed_nonempty"}};
}

struct lattice_derived_query_projection_semantics_entry_t {
  std::string field{};
  std::string value{};
  std::string meaning{};
  bool result_alias{false};
  bool compact_projection{false};
};

[[nodiscard]] inline std::vector<
    lattice_derived_query_projection_semantics_entry_t>
lattice_derived_query_projection_semantics_vocabulary() {
  return {
      {"projection_quantifier", "single",
       "one target-level boolean derived from one proof/status field", false,
       false},
      {"projection_quantifier", "per_row",
       "rule relation is parameterized and may be compacted in live "
       "derived_query_results",
       false, false},
      {"projection_quantifier", "all",
       "compact result is true only when every summarized row is true; "
       "zero-row behavior is specified by empty_projection_policy",
       false, true},
      {"projection_quantifier", "any",
       "compact result is true when at least one summarized row is true; "
       "zero-row behavior is specified by empty_projection_policy",
       false, true},
      {"projection_quantifier", "none",
       "compact result is true when no summarized rows or witnesses are "
       "present",
       false, true},
      {"empty_projection_policy", "not_applicable",
       "the relation is not an aggregate zero-row projection", false, false},
      {"empty_projection_policy", "vacuously_true",
       "an all-row compact projection over zero rows is true by universal "
       "quantification; row counts still expose that no rows contributed",
       false, true},
      {"empty_projection_policy", "false_when_empty",
       "an any-witness compact projection over zero rows is false", false,
       true},
      {"empty_projection_policy", "true_when_empty",
       "a none-exists compact projection over zero rows is true", false, true},
      {"empty_projection_policy", "fixed_nonempty",
       "the result has a fixed premise set and should not be interpreted as "
       "an empty relation",
       false, true},
      {"projection_scope", "target",
       "target-level projection over a single evaluation result", false, false},
      {"projection_scope", "per_dependency_row",
       "rule relation is parameterized by dependency proof rows", false, false},
      {"projection_scope", "all_dependency_rows_compact",
       "compact result summarizes all dependency proof rows", false, true},
      {"projection_scope", "per_coverage_row",
       "rule relation is parameterized by coverage proof rows", false, false},
      {"projection_scope", "all_coverage_rows_compact",
       "compact result summarizes all coverage proof rows", false, true},
      {"projection_scope", "root_checkpoint_closure",
       "projection over the selected root checkpoint closure proof", false,
       false},
      {"projection_scope", "per_checkpoint_ancestor",
       "rule relation is parameterized by checkpoint closure ancestor facts",
       false, false},
      {"projection_scope", "any_checkpoint_ancestor",
       "compact result summarizes whether any requested checkpoint ancestor "
       "witness exists",
       false, true},
      {"projection_scope", "per_unresolved_lineage_witness",
       "rule relation is parameterized by unresolved checkpoint lineage "
       "witnesses",
       false, false},
      {"projection_scope", "any_unresolved_lineage_witness",
       "compact result summarizes whether any unresolved lineage witness "
       "exists",
       false, true},
      {"projection_scope", "per_overlap_witness",
       "rule relation is parameterized by leakage overlap witnesses", false,
       false},
      {"projection_scope", "any_overlap_witness",
       "compact result summarizes whether any leakage overlap witness exists",
       false, true},
      {"projection_scope", "per_warning_result",
       "rule relation is parameterized by warning result rows", false, false},
      {"projection_scope", "any_warning_result",
       "compact result summarizes whether any warning result is triggered",
       false, true},
      {"projection_scope", "per_cache_validation_issue",
       "rule relation is parameterized by runtime-index cache validation "
       "issues",
       false, false},
      {"projection_scope", "any_cache_validation_issue",
       "compact result summarizes whether any cache validation issue exists",
       false, true},
      {"projection_scope", "per_plan_relevant_deficit",
       "rule relation is parameterized by plan-relevant deficit rows", false,
       false},
      {"projection_scope", "any_plan_relevant_deficit",
       "compact result summarizes whether any plan-relevant deficit exists",
       false, true},
      {"projection_scope", "projection_scope",
       "compatibility alias for result_projection_scope on live result "
       "rows",
       true, true},
      {"projection_quantifier", "projection_quantifier",
       "compatibility alias for result_projection_quantifier on live result "
       "rows",
       true, true},
      {"empty_projection_policy", "empty_projection_policy",
       "compatibility alias for result_empty_projection_policy on live "
       "result rows",
       true, true}};
}

struct lattice_db_cache_policy_entry_t {
  std::string policy{};
  std::vector<std::string> cached_relations{};
  std::string source_of_truth{};
  std::string allowed_use{};
  std::string invalidation_basis{};
  std::string fail_closed_policy{};
  bool db_writes_evidence{false};
  bool rebuildable_from_runtime_files{true};
  bool runtime_executor{false};
  bool contract_identity_authority{false};
};

[[nodiscard]] inline std::vector<lattice_db_cache_policy_entry_t>
lattice_db_cache_policy_vocabulary() {
  return {
      {"runtime_files_source_of_truth",
       {},
       "job.manifest, job.state, component reports, lattice.exposure.fact, "
       "lattice.checkpoint.fact, and checkpoint files",
       "authoritative evidence input for Lattice Hero reads and future cache "
       "rebuilds",
       "runtime artifact paths, fact digests, checkpoint paths, active "
       "contract/component/graph/source identity",
       "missing or unreadable runtime files block or fail closed instead of "
       "trusting cache rows",
       false,
       true,
       false,
       false},
      {"derived_relation_read_cache",
       {"identity_match", "dependency_satisfied", "all_dependencies_satisfied",
        "coverage_satisfied", "all_coverage_satisfied", "closure_complete",
        "checkpoint_ancestor", "unresolved_lineage", "closure_clean_if_checked",
        "forbidden_overlap", "no_forbidden_overlap", "warning_triggered",
        "proof_certificate_check_passed", "no_blocking_deficit",
        "status_satisfied", "target_satisfied"},
       "derived_query_rule_vocabulary plus proof_certificate fields",
       "speed read-only target queries and evidence inspection",
       "target_spec_fingerprint, split_policy_fingerprint, active identity, "
       "fact digests, checkpoint closure digest, and warning specs",
       "stale, missing, or identity-mismatched cache rows must be recomputed "
       "from runtime files before claiming readiness",
       false,
       true,
       false,
       false},
      {"plan_projection_not_evidence",
       {"plan_deficit"},
       "deficits, plan clauses, and latest evaluated evidence",
       "advice only; never satisfies a target and never executes a wave",
       "target_spec_fingerprint, deficits, plan inputs, and current evaluation "
       "status",
       "unavailable plan cache means recompute plan advice; never use cached "
       "plans as readiness evidence",
       false,
       true,
       false,
       false},
      {"checkpoint_digest_cache_preview",
       {"closure_complete", "dependency_satisfied"},
       "path-based checkpoint closure facts plus preview checkpoint_id and "
       "checkpoint_file_digest fields",
       "audit/index preview until v2 checkpoint identity promotion",
       "checkpoint path, producer exposure digest, checkpoint_id, "
       "checkpoint_file_digest, and component assembly fingerprint",
       "digest/id mismatch or unavailable checkpoint files prevent cache-key "
       "trust but do not alter protocol contract identity",
       false,
       true,
       false,
       false},
      {"executor_boundary",
       {},
       "Runtime Hero and runtime job artifacts",
       "Lattice Hero and future DB remain read-only evidence/query surfaces",
       "runtime tool invocation, job manifest, job state, and fact sidecars",
       "DB/cache rows must never execute waves, mutate evidence, or change "
       "MAX_WAVES accounting",
       false,
       true,
       false,
       false}};
}

struct lattice_evidence_retention_policy_entry_t {
  std::string artifact_family{};
  std::string retention_role{};
  std::string proof_replay_obligation{};
  std::vector<std::string> required_bindings{};
  std::string prune_policy{};
  bool source_of_truth{false};
  bool compact_receipt_authority{false};
  bool cache_row{false};
  bool human_receipt{false};
  bool runtime_executor{false};
};

[[nodiscard]] inline std::vector<lattice_evidence_retention_policy_entry_t>
lattice_evidence_retention_policy_vocabulary() {
  const std::vector<std::string> active_bindings{
      "protocol_contract_fingerprint", "target_spec_fingerprint",
      "split_policy_fingerprint", "graph_order_fingerprint",
      "source_cursor_token"};
  const std::vector<std::string> checkpoint_bindings{
      "checkpoint_path", "checkpoint_id", "checkpoint_file_digest",
      "component_assembly_fingerprint"};
  return {
      {"runtime_reports", "durable_source_material",
       "recompute target status, optimizer effort, loaded model-state inputs, "
       "loss/support metrics, and warning measurements",
       active_bindings,
       "retain or archive with manifest; pruning before replay is forbidden",
       true, false, false, false, false},
      {"lattice_sidecars", "durable_source_material",
       "recompute exposure facts, checkpoint facts, source receipts, selection "
       "signals, and representation support",
       active_bindings,
       "retain or archive with manifest; malformed/missing sidecars fail "
       "closed",
       true, false, false, false, false},
      {"checkpoint_files", "model_state_source_material",
       "verify checkpoint existence, byte digest in strict audit, and loaded "
       "model-state lineage",
       checkpoint_bindings,
       "retain while any proof refers to path/id/digest; unresolved lineage "
       "blocks pruning",
       true, false, false, false, false},
      {"source_receipt_facts",
       "audit_metadata",
       "replay source-key and source-file traceability without granting "
       "coverage or leakage authority",
       {"graph_order_fingerprint", "source_cursor_token",
        "parent_exposure_fact_digest"},
       "may be compacted only as audit metadata; never replaces row-index "
       "proof facts",
       false,
       false,
       false,
       false,
       false},
      {"selection_signal_facts", "durable_leakage_material",
       "recompute known selector paths and forbidden selection-signal overlap",
       active_bindings,
       "retain with parent exposure/checkpoint links; pruning selector edges "
       "requires leakage replay",
       true, false, false, false, false},
      {"proof_certificates",
       "derived_audit_receipt",
       "explain a prior evaluation but not replace runtime proof authority",
       {"target_spec_fingerprint", "split_policy_fingerprint",
        "certificate_digest"},
       "may be retained as receipt; cannot satisfy target without replay or "
       "future promoted proof format",
       false,
       false,
       false,
       false,
       false},
      {"runtime_index_cache_rows",
       "rebuildable_read_model",
       "speed audit queries but never upgrade target satisfaction",
       {"runtime_metadata_digest", "watched_file_metadata_digest",
        "row_set_digest"},
       "stale cache after archive movement is discarded or rebuilt from "
       "runtime files",
       false,
       false,
       true,
       false,
       false},
      {"human_receipts",
       "human_audit_receipt",
       "summarize a run for review without becoming source-of-truth evidence",
       {"runtime_root", "active_identity", "target_statuses"},
       "retain as narrative receipt; never replaces reports, sidecars, "
       "checkpoints, or closure facts",
       false,
       false,
       false,
       true,
       false},
      {"archive_manifest",
       "archive_binding",
       "bind archived evidence to identity, split policy, source cursor, graph "
       "order, and checkpoint identities for replay",
       {"protocol_contract_fingerprint", "target_spec_fingerprint",
        "split_policy_fingerprint", "graph_order_fingerprint",
        "source_cursor_token", "checkpoint_path", "checkpoint_id",
        "checkpoint_file_digest"},
       "archive publication must be atomic and replayable; missing required "
       "bindings warn or block compaction",
       false,
       false,
       false,
       false,
       false}};
}

struct lattice_evidence_retention_audit_scenario_entry_t {
  std::string scenario{};
  std::string expected_result{};
  std::string evidence_required{};
  bool preserves_replay_authority{false};
  bool refuses_or_warns{false};
  bool cache_non_authority{false};
  bool compact_receipt_non_authority{false};
};

[[nodiscard]] inline std::vector<
    lattice_evidence_retention_audit_scenario_entry_t>
lattice_evidence_retention_audit_scenario_vocabulary() {
  return {
      {"complete_archive_replay",
       "target status, checkpoint closure, leakage witnesses, and warning "
       "summaries recompute from archived runtime material",
       "reports, sidecars, checkpoint facts/files, split policy, active "
       "identity, and archive manifest",
       true, false, false, false},
      {"missing_lineage_after_pruning",
       "pruning refuses or reports unresolved upstream lineage before archive "
       "is accepted",
       "checkpoint closure facts and upstream checkpoint producer evidence",
       false, true, false, false},
      {"stale_cache_after_archive_movement",
       "cache rows are rejected or rebuilt; target satisfaction stays live "
       "runtime evidence based",
       "runtime metadata digest, watched-file digest, row-set digest, relation "
       "counts",
       false, true, true, false},
      {"compact_receipt_non_authority",
       "PASS.md or compact receipts remain review metadata and cannot satisfy "
       "coverage, leakage, or closure alone",
       "runtime reports, exposure facts, checkpoint facts, and sidecars", false,
       true, false, true}};
}

struct lattice_evidence_retention_policy_summary_t {
  std::string schema{"kikijyeba.lattice.evidence_retention_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t source_of_truth_count{0};
  std::int64_t compact_receipt_authority_count{0};
  std::int64_t cache_row_count{0};
  std::int64_t human_receipt_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t scenario_count{0};
  std::int64_t replay_preserving_scenario_count{0};
  std::int64_t refuse_or_warn_scenario_count{0};
  std::int64_t empty_binding_count{0};
  bool reports_preserved_for_replay{false};
  bool sidecars_preserved_for_replay{false};
  bool checkpoints_preserved_for_lineage{false};
  bool source_receipts_audit_only{false};
  bool selection_signals_preserved_for_leakage{false};
  bool proof_certificates_non_authority{false};
  bool cache_rows_rebuildable_non_authority{false};
  bool human_receipts_non_authority{false};
  bool archive_manifest_binds_identity{false};
  bool compact_receipts_non_authority{false};
  bool pruning_checks_unresolved_lineage{false};
  bool stale_cache_after_archive_non_authority{false};
  bool complete_archive_replay_scenario_present{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> artifact_families{};
  std::vector<std::string> scenario_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_evidence_retention_policy_summary_t
lattice_evidence_retention_policy_summary() {
  lattice_evidence_retention_policy_summary_t out{};
  const auto policies = lattice_evidence_retention_policy_vocabulary();
  const auto scenarios = lattice_evidence_retention_audit_scenario_vocabulary();
  out.policy_count = static_cast<std::int64_t>(policies.size());
  out.scenario_count = static_cast<std::int64_t>(scenarios.size());

  const auto find_policy = [&](const std::string &family) {
    return std::find_if(
        policies.begin(), policies.end(),
        [&](const auto &entry) { return entry.artifact_family == family; });
  };
  const auto find_scenario = [&](const std::string &scenario) {
    return std::find_if(
        scenarios.begin(), scenarios.end(),
        [&](const auto &entry) { return entry.scenario == scenario; });
  };

  for (const auto &entry : policies) {
    out.artifact_families.push_back(entry.artifact_family);
    if (entry.source_of_truth) {
      ++out.source_of_truth_count;
    }
    if (entry.compact_receipt_authority) {
      ++out.compact_receipt_authority_count;
    }
    if (entry.cache_row) {
      ++out.cache_row_count;
    }
    if (entry.human_receipt) {
      ++out.human_receipt_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.required_bindings.empty()) {
      ++out.empty_binding_count;
    }
  }
  for (const auto &entry : scenarios) {
    out.scenario_names.push_back(entry.scenario);
    if (entry.preserves_replay_authority) {
      ++out.replay_preserving_scenario_count;
    }
    if (entry.refuses_or_warns) {
      ++out.refuse_or_warn_scenario_count;
    }
  }

  const auto reports = find_policy("runtime_reports");
  const auto sidecars = find_policy("lattice_sidecars");
  const auto checkpoints = find_policy("checkpoint_files");
  const auto receipts = find_policy("source_receipt_facts");
  const auto selection = find_policy("selection_signal_facts");
  const auto certificates = find_policy("proof_certificates");
  const auto cache = find_policy("runtime_index_cache_rows");
  const auto human = find_policy("human_receipts");
  const auto archive = find_policy("archive_manifest");
  const auto complete = find_scenario("complete_archive_replay");
  const auto missing_lineage = find_scenario("missing_lineage_after_pruning");
  const auto stale_cache = find_scenario("stale_cache_after_archive_movement");
  const auto compact_non_authority =
      find_scenario("compact_receipt_non_authority");

  out.reports_preserved_for_replay =
      reports != policies.end() && reports->source_of_truth &&
      reports->proof_replay_obligation.find("target status") !=
          std::string::npos;
  out.sidecars_preserved_for_replay =
      sidecars != policies.end() && sidecars->source_of_truth &&
      sidecars->proof_replay_obligation.find("exposure facts") !=
          std::string::npos;
  out.checkpoints_preserved_for_lineage =
      checkpoints != policies.end() && checkpoints->source_of_truth &&
      checkpoints->prune_policy.find("unresolved lineage") != std::string::npos;
  out.source_receipts_audit_only =
      receipts != policies.end() && !receipts->source_of_truth &&
      !receipts->compact_receipt_authority &&
      receipts->proof_replay_obligation.find("without granting coverage") !=
          std::string::npos;
  out.selection_signals_preserved_for_leakage =
      selection != policies.end() && selection->source_of_truth &&
      selection->proof_replay_obligation.find("selector paths") !=
          std::string::npos;
  out.proof_certificates_non_authority =
      certificates != policies.end() && !certificates->source_of_truth &&
      certificates->prune_policy.find("cannot satisfy target") !=
          std::string::npos;
  out.cache_rows_rebuildable_non_authority =
      cache != policies.end() && cache->cache_row && !cache->source_of_truth &&
      cache->prune_policy.find("rebuilt from runtime files") !=
          std::string::npos;
  out.human_receipts_non_authority =
      human != policies.end() && human->human_receipt &&
      !human->source_of_truth &&
      human->prune_policy.find("never replaces") != std::string::npos;
  out.archive_manifest_binds_identity =
      archive != policies.end() &&
      std::find(archive->required_bindings.begin(),
                archive->required_bindings.end(),
                "protocol_contract_fingerprint") !=
          archive->required_bindings.end() &&
      std::find(archive->required_bindings.begin(),
                archive->required_bindings.end(),
                "checkpoint_file_digest") != archive->required_bindings.end();
  out.compact_receipts_non_authority =
      out.compact_receipt_authority_count == 0 &&
      compact_non_authority != scenarios.end() &&
      compact_non_authority->compact_receipt_non_authority;
  out.pruning_checks_unresolved_lineage =
      missing_lineage != scenarios.end() && missing_lineage->refuses_or_warns;
  out.stale_cache_after_archive_non_authority =
      stale_cache != scenarios.end() && stale_cache->cache_non_authority;
  out.complete_archive_replay_scenario_present =
      complete != scenarios.end() && complete->preserves_replay_authority;

  if (out.policy_count != 9) {
    out.summary_issues.push_back(
        "evidence retention policy vocabulary must contain 9 artifact "
        "families");
  }
  if (out.scenario_count != 4) {
    out.summary_issues.push_back(
        "evidence retention audit vocabulary must contain 4 scenarios");
  }
  if (!out.reports_preserved_for_replay || !out.sidecars_preserved_for_replay ||
      !out.checkpoints_preserved_for_lineage) {
    out.summary_issues.push_back(
        "reports, sidecars, and checkpoints must remain replayable proof "
        "material");
  }
  if (!out.source_receipts_audit_only || !out.compact_receipts_non_authority) {
    out.summary_issues.push_back(
        "compact/source receipts must stay audit metadata, not proof "
        "authority");
  }
  if (!out.selection_signals_preserved_for_leakage) {
    out.summary_issues.push_back(
        "selection-signal facts must be retained for leakage replay");
  }
  if (!out.proof_certificates_non_authority ||
      !out.human_receipts_non_authority) {
    out.summary_issues.push_back(
        "proof certificates and human receipts must not replace runtime "
        "evidence");
  }
  if (!out.cache_rows_rebuildable_non_authority ||
      !out.stale_cache_after_archive_non_authority) {
    out.summary_issues.push_back(
        "cache rows must be rebuildable and non-authoritative after archive "
        "movement");
  }
  if (!out.archive_manifest_binds_identity) {
    out.summary_issues.push_back(
        "archive manifests must bind identity, split policy, source cursor, "
        "graph order, and checkpoint identity");
  }
  if (!out.pruning_checks_unresolved_lineage ||
      !out.complete_archive_replay_scenario_present) {
    out.summary_issues.push_back(
        "retention audit scenarios must cover replay and unresolved lineage");
  }
  if (out.runtime_executor_count != 0 || out.empty_binding_count != 0) {
    out.summary_issues.push_back(
        "retention policy must be read-only and every row must declare "
        "bindings");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_benchmark_regression_budget_entry_t {
  std::string benchmark_id{};
  std::string timing_layer{};
  std::string proof_mode{};
  std::string measured_surface{};
  std::string required_measurement{};
  std::string regression_guard{};
  std::string expected_fast_path{};
  bool full_live_scan_allowed{false};
  bool metadata_digest_allowed{false};
  bool cache_target_authority{false};
  bool process_startup_included{false};
  bool session_reuse_required{false};
  bool regression_smoke_required{true};
  bool baseline_receipt_required{true};
};

[[nodiscard]] inline std::vector<lattice_benchmark_regression_budget_entry_t>
lattice_benchmark_regression_budget_vocabulary() {
  return {
      {"library_header_only_index_query", "library_function", "header_only",
       "read_runtime_index_cache + validate_runtime_index_cache + "
       "query_runtime_index_cache",
       "in-process cache read, header-only validation, row-set integrity, "
       "relation counts, and row query timing without process startup",
       "metadata_checked=false, watched_metadata_checked=false, "
       "compare_live_scan=false, selected_answer_parity.checked=false",
       "no recursive runtime metadata digest and no live scan", false, false,
       false, false, false, true, true},
      {"library_watched_manifest_index_query", "library_function",
       "watched_file_manifest",
       "read_runtime_index_cache + watched_runtime_metadata_digest + "
       "query_runtime_index_cache",
       "in-process watched-file freshness timing without direct CLI startup",
       "metadata_checked=false, watched_metadata_checked=true, "
       "full_runtime_metadata_digest not computed",
       "watch only files that can change runtime-index rows", false, false,
       false, false, false, true, true},
      {"library_full_metadata_index_status", "library_function",
       "full_runtime_metadata_digest", "validate_runtime_index_cache",
       "in-process recursive runtime metadata digest timing, separated from "
       "cache row-query timing",
       "metadata_checked=true and timing row labelled "
       "full_runtime_metadata_digest",
       "proof cost is named instead of hidden inside fast audit queries", false,
       true, false, false, false, true, true},
      {"library_live_scan_index_build", "library_function", "live_scan",
       "build_runtime_index_cache",
       "in-process live scan and cache-row rebuild timing without process "
       "startup",
       "timing row uses proof_mode=live_scan and may rebuild evidence from "
       "runtime files",
       "live scan cost is separated from cache query cost", true, true, false,
       false, false, true, true},
      {"long_lived_mcp_header_only_index_query", "long_lived_mcp",
       "header_only", "hero.lattice.inspect subject=index mode=query",
       "one Hero MCP process, repeated header-only audit queries, and session "
       "cache behavior visible",
       "compare_live_scan=false, allow_unproven_cache=true, "
       "validation_strength=header_only, result_source=cache",
       "no live scan inside the fast audit query after cache is present", false,
       false, false, false, true, true, true},
      {"long_lived_mcp_evaluate_targets_batch", "long_lived_mcp", "live_scan",
       "hero.lattice.evaluate operation=targets",
       "one Hero MCP process, multiple targets evaluated through one loaded "
       "target evaluator and session-scanned runtime ledger",
       "result_source=target_evaluation_batch and cache_used_for_target_"
       "satisfaction=false",
       "batch scan reuse is measured separately from cache audit queries", true,
       true, false, false, true, true, true},
      {"direct_cli_header_only_index_query", "direct_cli", "header_only",
       "hero_lattice.mcp --tool hero.lattice.inspect subject=index mode=query",
       "full direct process call timing for explicit header-only unproven "
       "audit-cache query",
       "compare_live_scan=false, allow_unproven_cache=true, "
       "validation_strength=header_only, cache_used_for_target_"
       "satisfaction=false",
       "direct CLI startup is charged to this row, not to library timings",
       false, false, false, true, false, true, true},
      {"direct_cli_live_parity_index_query", "direct_cli", "live_parity",
       "hero_lattice.mcp --tool hero.lattice.inspect subject=index mode=query",
       "full direct process call timing for cache validation plus live-scan "
       "parity proof",
       "compare_live_scan=true and selected_answer_parity.checked=true",
       "proof/parity cost is allowed and named separately from fast audit mode",
       true, true, false, true, false, true, true},
      {"direct_cli_evaluate_targets_batch", "direct_cli", "live_scan",
       "hero_lattice.mcp --tool hero.lattice.evaluate operation=targets",
       "full direct process call timing for batch target evaluation over live "
       "runtime evidence",
       "result_source=target_evaluation_batch and target satisfaction never "
       "uses cache rows",
       "direct CLI batch timing includes process startup and live evidence "
       "scan",
       true, true, false, true, false, true, true}};
}

struct lattice_benchmark_regression_budget_summary_t {
  std::string schema{
      "kikijyeba.lattice.benchmark_regression_budget.summary.v1"};
  std::int64_t budget_count{0};
  std::int64_t library_layer_count{0};
  std::int64_t long_lived_mcp_layer_count{0};
  std::int64_t direct_cli_layer_count{0};
  std::int64_t header_only_mode_count{0};
  std::int64_t watched_manifest_mode_count{0};
  std::int64_t full_metadata_mode_count{0};
  std::int64_t live_scan_mode_count{0};
  std::int64_t live_parity_mode_count{0};
  std::int64_t full_live_scan_allowed_count{0};
  std::int64_t metadata_digest_allowed_count{0};
  std::int64_t cache_target_authority_count{0};
  std::int64_t process_startup_included_count{0};
  std::int64_t session_reuse_required_count{0};
  std::int64_t regression_smoke_required_count{0};
  std::int64_t baseline_receipt_required_count{0};
  std::int64_t empty_field_count{0};
  bool all_required_layers_present{false};
  bool all_required_proof_modes_present{false};
  bool header_only_fast_paths_forbid_live_scan{false};
  bool proof_modes_separate_from_fast_path{false};
  bool long_lived_mcp_session_reuse_present{false};
  bool direct_cli_startup_separated{false};
  bool regression_smoke_declared{false};
  bool baseline_receipt_required_for_all_rows{false};
  bool no_cache_target_satisfaction_authority{false};
  bool all_rows_have_measurement_and_guard{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> benchmark_ids{};
  std::vector<std::string> proof_modes{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_benchmark_regression_budget_summary_t
lattice_benchmark_regression_budget_summary() {
  lattice_benchmark_regression_budget_summary_t out{};
  const auto rows = lattice_benchmark_regression_budget_vocabulary();
  out.budget_count = static_cast<std::int64_t>(rows.size());

  const auto has_row = [&](const std::string &benchmark_id) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto &entry) {
      return entry.benchmark_id == benchmark_id;
    });
  };
  const auto proof_mode_present = [&](const std::string &mode) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto &entry) {
      return entry.proof_mode == mode;
    });
  };

  for (const auto &entry : rows) {
    out.benchmark_ids.push_back(entry.benchmark_id);
    if (std::find(out.proof_modes.begin(), out.proof_modes.end(),
                  entry.proof_mode) == out.proof_modes.end()) {
      out.proof_modes.push_back(entry.proof_mode);
    }
    if (entry.timing_layer == "library_function") {
      ++out.library_layer_count;
    }
    if (entry.timing_layer == "long_lived_mcp") {
      ++out.long_lived_mcp_layer_count;
    }
    if (entry.timing_layer == "direct_cli") {
      ++out.direct_cli_layer_count;
    }
    if (entry.proof_mode == "header_only") {
      ++out.header_only_mode_count;
    }
    if (entry.proof_mode == "watched_file_manifest") {
      ++out.watched_manifest_mode_count;
    }
    if (entry.proof_mode == "full_runtime_metadata_digest") {
      ++out.full_metadata_mode_count;
    }
    if (entry.proof_mode == "live_scan") {
      ++out.live_scan_mode_count;
    }
    if (entry.proof_mode == "live_parity") {
      ++out.live_parity_mode_count;
    }
    if (entry.full_live_scan_allowed) {
      ++out.full_live_scan_allowed_count;
    }
    if (entry.metadata_digest_allowed) {
      ++out.metadata_digest_allowed_count;
    }
    if (entry.cache_target_authority) {
      ++out.cache_target_authority_count;
    }
    if (entry.process_startup_included) {
      ++out.process_startup_included_count;
    }
    if (entry.session_reuse_required) {
      ++out.session_reuse_required_count;
    }
    if (entry.regression_smoke_required) {
      ++out.regression_smoke_required_count;
    }
    if (entry.baseline_receipt_required) {
      ++out.baseline_receipt_required_count;
    }
    if (entry.benchmark_id.empty() || entry.timing_layer.empty() ||
        entry.proof_mode.empty() || entry.measured_surface.empty() ||
        entry.required_measurement.empty() || entry.regression_guard.empty() ||
        entry.expected_fast_path.empty()) {
      ++out.empty_field_count;
    }
  }

  out.all_required_layers_present = out.library_layer_count > 0 &&
                                    out.long_lived_mcp_layer_count > 0 &&
                                    out.direct_cli_layer_count > 0;
  out.all_required_proof_modes_present =
      proof_mode_present("header_only") &&
      proof_mode_present("watched_file_manifest") &&
      proof_mode_present("full_runtime_metadata_digest") &&
      proof_mode_present("live_scan") && proof_mode_present("live_parity");
  out.header_only_fast_paths_forbid_live_scan =
      std::all_of(rows.begin(), rows.end(), [](const auto &entry) {
        return entry.proof_mode != "header_only" ||
               (!entry.full_live_scan_allowed &&
                !entry.metadata_digest_allowed &&
                entry.regression_guard.find("compare_live_scan=false") !=
                    std::string::npos);
      });
  out.proof_modes_separate_from_fast_path =
      has_row("library_header_only_index_query") &&
      has_row("library_full_metadata_index_status") &&
      has_row("library_live_scan_index_build") &&
      has_row("direct_cli_live_parity_index_query");
  out.long_lived_mcp_session_reuse_present =
      has_row("long_lived_mcp_header_only_index_query") &&
      has_row("long_lived_mcp_evaluate_targets_batch") &&
      out.session_reuse_required_count >= 2;
  out.direct_cli_startup_separated =
      has_row("direct_cli_header_only_index_query") &&
      has_row("direct_cli_live_parity_index_query") &&
      has_row("direct_cli_evaluate_targets_batch") &&
      out.process_startup_included_count == 3;
  out.regression_smoke_declared =
      out.regression_smoke_required_count == out.budget_count &&
      out.header_only_fast_paths_forbid_live_scan;
  out.baseline_receipt_required_for_all_rows =
      out.baseline_receipt_required_count == out.budget_count;
  out.no_cache_target_satisfaction_authority =
      out.cache_target_authority_count == 0;
  out.all_rows_have_measurement_and_guard = out.empty_field_count == 0;

  if (out.budget_count != 9) {
    out.summary_issues.push_back(
        "benchmark regression budget must contain 9 finite timing rows");
  }
  if (!out.all_required_layers_present) {
    out.summary_issues.push_back(
        "benchmark budget must separate library, long-lived MCP, and direct "
        "CLI timing layers");
  }
  if (!out.all_required_proof_modes_present) {
    out.summary_issues.push_back(
        "benchmark budget must cover header_only, watched_file_manifest, "
        "full_runtime_metadata_digest, live_scan, and live_parity proof "
        "modes");
  }
  if (!out.header_only_fast_paths_forbid_live_scan) {
    out.summary_issues.push_back(
        "header-only fast audit rows must forbid full live scans and metadata "
        "digests");
  }
  if (!out.proof_modes_separate_from_fast_path) {
    out.summary_issues.push_back(
        "metadata, live-scan, and parity proof costs must be separated from "
        "header-only fast audit paths");
  }
  if (!out.long_lived_mcp_session_reuse_present) {
    out.summary_issues.push_back(
        "long-lived MCP benchmark rows must require session reuse");
  }
  if (!out.direct_cli_startup_separated) {
    out.summary_issues.push_back(
        "direct CLI benchmark rows must explicitly include process startup");
  }
  if (!out.regression_smoke_declared ||
      !out.baseline_receipt_required_for_all_rows) {
    out.summary_issues.push_back(
        "every benchmark row must require a regression smoke and committed "
        "baseline receipt");
  }
  if (!out.no_cache_target_satisfaction_authority) {
    out.summary_issues.push_back(
        "benchmark cache rows must never become target-satisfaction "
        "authority");
  }
  if (!out.all_rows_have_measurement_and_guard) {
    out.summary_issues.push_back(
        "benchmark rows must declare measured surface, measurement, guard, "
        "and expected path");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_evidence_abstraction_entry_t {
  std::string abstraction{};
  std::vector<std::string> concrete_inputs{};
  std::vector<std::string> abstract_outputs{};
  std::string soundness_condition{};
  std::string conservative_policy{};
  std::string join_semantics{};
  bool read_only{true};
  bool db_cacheable{true};
};

[[nodiscard]] inline std::vector<lattice_evidence_abstraction_entry_t>
lattice_evidence_abstraction_vocabulary() {
  return {
      {"identity_abstraction",
       {"active_identity", "runtime_manifest", "runtime_report",
        "lattice.exposure.fact"},
       {"proof_certificate.proof_context", "evidence_order_vector.identity_*"},
       "required active identity fields must exactly match the evidence "
       "identity before satisfaction can be claimed",
       "missing or mismatched identity fields block rather than degrade to a "
       "warning",
       "exact-match meet over contract, component, graph order, and source "
       "cursor identity",
       true,
       true},
      {"coverage_load_abstraction",
       {"contiguous_completed_range_v1 exposure facts"},
       {"proof_certificate.coverage", "exposure_summaries"},
       "unique coverage is the idempotent union of completed cursor intervals; "
       "load is the additive sum of repeated completed intervals",
       "requested or incomplete ranges remain untrusted for readiness coverage",
       "coverage joins by interval union; load joins by interval-measure sum",
       true,
       true},
      {"checkpoint_closure_abstraction",
       {"lattice.checkpoint.fact", "lattice.exposure.fact"},
       {"proof_certificate.closure"},
       "complete closure contains the root checkpoint producer and all "
       "resolved "
       "ancestor input checkpoints",
       "unresolved checkpoint inputs make lineage incomplete and non-claiming",
       "closure joins by labeled ancestor reachability with duplicate digest "
       "rejection",
       true,
       true},
      {"leakage_abstraction",
       {"proof_certificate.closure.causal_exposures", "split_policy"},
       {"proof_certificate.leakage"},
       "every forbidden source-footprint intersection with the dilated "
       "protected split must appear as an overlap witness",
       "unchecked or incomplete closure cannot prove no leakage",
       "forbidden witnesses join by fact digest, use, source footprint, and "
       "intersection",
       true,
       true},
      {"node_support_abstraction",
       {"lattice.node_exposure.fact", "proof_certificate.closure"},
       {"node_support_summaries", "proof_certificate.node_support_summaries"},
       "node summaries must aggregate inherited MDN node exposure facts "
       "without "
       "row-count or valid-target arithmetic drift",
       "missing node-support facts weaken visibility/warnings but do not "
       "create "
       "synthetic support",
       "support joins by node identity and exposure use, with "
       "target-opportunity "
       "denominators kept separate from row counts",
       true,
       true},
      {"source_receipt_audit_abstraction",
       {"lattice.exposure.fact.source_file_receipts", "runtime_manifest",
        "runtime_report"},
       {"source_receipt_policy_vocabulary", "source_receipt_policy_summary"},
       "compact source receipts trace runtime facts to source files but never "
       "imply coverage, leakage cleanliness, or contract identity",
       "missing receipts weaken audit traceability only and remain "
       "non-claiming "
       "for readiness",
       "receipts join as deduplicated audit strings by fact/job digest within "
       "matching identity",
       true,
       true},
      {"warning_abstraction",
       {"warning_specs", "exposure_summaries", "node_support_summaries",
        "anchor_domain_health"},
       {"warning_results", "warning_summary"},
       "warning trigger state is derived from structured measurements and "
       "thresholds, not from message text",
       "unavailable measurements are counted explicitly and remain "
       "non-blocking",
       "warnings join as structured rows plus aggregate per-relation counts",
       true,
       true},
      {"plan_abstraction",
       {"deficits", "plan_clauses"},
       {"plan_basis", "suggested_wave"},
       "plans are deterministic projections of proof deficits and plan inputs",
       "planning cannot execute waves and cannot satisfy a target by itself",
       "plan joins by minimal deficit priority and unique plan-relevant "
       "deficit "
       "keys",
       true,
       false},
      {"evidence_order_abstraction",
       {"proof_certificate", "proof_certificate_check", "deficits",
        "warning_summary"},
       {"evidence_order_vector"},
       "the evidence order vector preserves separate Pareto dimensions instead "
       "of collapsing them into one score",
       "unavailable vectors or incomparable dimensions must be reported as "
       "such",
       "joins use Pareto dominance over comparison dimensions and leave "
       "visibility-only dimensions out of dominance",
       true,
       true}};
}

struct lattice_product_evidence_state_entry_t {
  std::string factor{};
  std::string mathematical_object{};
  std::string partial_order{};
  std::string join_semantics{};
  std::vector<std::string> proof_fields{};
  std::string clean_growth_rule{};
  std::string target_effect{};
  bool join_requires_identity_match{true};
  bool status_monotone_dimension{false};
  bool db_cacheable{true};
};

struct lattice_join_law_entry_t {
  std::string factor{};
  std::string join_operator{};
  std::vector<std::string> algebraic_laws{};
  std::string identity_scope{};
  std::string duplicate_policy{};
  std::string unsafe_join_policy{};
  std::string cache_requirement{};
  bool associative{true};
  bool commutative{true};
  bool idempotent{false};
  bool additive{false};
  bool requires_identity_match{true};
  bool cache_safe{true};
};

[[nodiscard]] inline std::vector<lattice_product_evidence_state_entry_t>
lattice_product_evidence_state_vocabulary() {
  return {
      {"proof_context",
       "discrete_exact_match_identity",
       "equal_identity_or_incomparable",
       "no join across mismatched contract, component assembly, graph order, "
       "or source cursor identity",
       {"proof_certificate.proof_context", "evidence_order_vector.identity_*"},
       "clean growth preserves the active contract/component/graph/source "
       "proof_context",
       "missing identity blocks; stale or mismatched identity prevents "
       "satisfaction",
       true,
       true,
       true},
      {"coverage_lattice",
       "idempotent_interval_union_semilattice",
       "covered_interval_superset_within_same_split_use_and_identity",
       "canonical interval union over trusted completed cursor intervals",
       {"proof_certificate.coverage[]", "exposure_summaries"},
       "clean growth adds completed trusted intervals for the same split, use, "
       "component, and identity",
       "coverage below the required fraction produces an exposure deficit",
       true,
       true,
       true},
      {"exposure_load_monoid",
       "additive_interval_measure_monoid",
       "greater_or_equal_clean_load_with_warning_dimensions_separate",
       "sum interval measures and optimizer effort over matching exposure "
       "facts",
       {"exposure_summaries", "warning_results"},
       "clean load growth adds completed exposure without adding leakage, "
       "deficits, or triggered warnings",
       "load is visibility/warning evidence and does not satisfy readiness by "
       "itself",
       true,
       false,
       true},
      {"closure_graph",
       "labeled_causal_checkpoint_dag",
       "resolved_ancestor_reachability_superset",
       "union labeled checkpoint/exposure ancestors with duplicate digest "
       "rejection",
       {"proof_certificate.closure",
        "proof_certificate.closure.causal_exposures[]"},
       "clean growth resolves more ancestors without introducing unresolved "
       "inputs or identity conflicts",
       "incomplete closure blocks or fails leakage-dependent claims",
       true,
       true,
       true},
      {"leakage_predicate",
       "dilated_protected_set_intersection_predicate",
       "fewer_forbidden_overlap_witnesses_is_stronger",
       "union forbidden witnesses after protected-split dilation and "
       "forbidden-use filtering",
       {"proof_certificate.leakage",
        "proof_certificate.leakage.overlap_witnesses[]"},
       "clean growth preserves empty forbidden intersections over complete "
       "closure",
       "any forbidden witness makes the protected-split proof fail",
       true,
       true,
       true},
      {"metric_warning_vector",
       "structured_threshold_relation_vector",
       "clear_measured_stronger_than_unavailable_or_triggered",
       "join warning rows and aggregate per-relation counts from structured "
       "threshold bits",
       {"warning_results", "warning_summary"},
       "clean growth preserves measured clear warnings or resolves unavailable "
       "measurements without triggering thresholds",
       "warnings are non-blocking in V1 but weaken evidence order",
       true,
       false,
       true},
      {"node_support_matrix",
       "node_by_use_support_matrix",
       "per_node_support_and_balance_vector_order",
       "aggregate support by node identity and exposure use while preserving "
       "valid-target denominators",
       {"node_support_summaries", "proof_certificate.node_support_summaries[]"},
       "clean growth adds honest inherited MDN node facts without synthesizing "
       "missing support",
       "node support is V1 visibility/warning evidence unless a target adds a "
       "hard requirement",
       true,
       false,
       true},
      {"source_receipt_audit",
       "audit_receipt_set",
       "more_traceable_receipts_is_more_visible_not_stronger_for_readiness",
       "deduplicate compact source receipt strings by fact/job digest within "
       "matching identity",
       {"source_receipt_policy_vocabulary", "source_receipt_policy_summary",
        "lattice.exposure.fact.source_file_receipts"},
       "clean growth may add source traceability without changing proof "
       "identity, coverage, leakage, or status",
       "receipt visibility supports audit only and never satisfies readiness",
       true,
       false,
       true},
      {"deficit_vector",
       "proof_obligation_deficit_vector",
       "fewer_and_lower_priority_deficits_is_stronger",
       "project proof deficits through deterministic priority classes",
       {"deficits", "plan_basis"},
       "clean growth resolves missing proof obligations without adding "
       "contradicted obligations",
       "blocking deficits prevent satisfaction and drive plan advice only",
       true,
       true,
       false}};
}

[[nodiscard]] inline std::vector<lattice_join_law_entry_t>
lattice_join_law_vocabulary() {
  return {
      {"proof_context",
       "exact_match_meet_or_incomparable",
       {"associative", "commutative", "idempotent"},
       "protocol_contract_fingerprint, component assembly fingerprint, graph "
       "order fingerprint, and source cursor token",
       "duplicate matching identities collapse to one identity element",
       "mismatched identities are incomparable and must not be joined for a "
       "readiness claim",
       "cache rows must be keyed by active identity and recomputed on identity "
       "mismatch",
       true,
       true,
       true,
       false,
       true,
       true},
      {"coverage_lattice",
       "canonical_interval_union",
       {"associative", "commutative", "idempotent"},
       "same active identity, split, component scope, exposure use, and cursor "
       "domain",
       "overlapping or repeated intervals merge once by row-index union",
       "requested, incomplete, stale-identity, or wrong-use intervals are "
       "non-claiming",
       "cache must retain enough interval detail to recompute union and "
       "missing complement",
       true,
       true,
       true,
       false,
       true,
       true},
      {"exposure_load_monoid",
       "distinct_fact_interval_measure_sum",
       {"associative", "commutative", "additive", "not_idempotent"},
       "same active identity, split, component scope, exposure use, and cursor "
       "domain",
       "duplicate fact digests do not add load; distinct repeated exposures do",
       "load joins that hide duplicate facts or identity drift are audit "
       "failures and cannot support warning summaries",
       "cache must preserve fact identity or a rebuildable digest set before "
       "summing repeated load",
       true,
       true,
       false,
       true,
       true,
       true},
      {"closure_graph",
       "labeled_ancestor_reachability_union",
       {"associative", "commutative", "idempotent"},
       "same active identity and checkpoint closure root",
       "duplicate exposure/checkpoint fact digests collapse to one ancestor",
       "unresolved inputs, causal cycles, stale identity, or wrong root make "
       "closure non-claiming",
       "cache must preserve ancestor digests, input/output checkpoint edges, "
       "and unresolved inputs",
       true,
       true,
       true,
       false,
       true,
       true},
      {"leakage_witness_set",
       "forbidden_intersection_witness_union",
       {"associative", "commutative", "idempotent"},
       "same protected split, dilation rule, forbidden-use set, active "
       "identity, and closure root",
       "duplicate witnesses collapse by fact digest, use, footprint, and "
       "intersection",
       "any joined forbidden witness makes the protected-split proof fail; "
       "unchecked closure cannot prove the empty witness set",
       "cache must preserve enough witness detail to recompute intersections "
       "from causal exposures",
       true,
       true,
       true,
       false,
       true,
       true},
      {"warning_result_vector",
       "structured_warning_row_join",
       {"associative", "commutative", "idempotent"},
       "same target proof identity, warning id, evidence basis, active "
       "identity, split, and metric",
       "duplicate warning rows collapse only when their structured threshold "
       "relation and measurement match",
       "unavailable measurements remain visible; warnings never satisfy or "
       "fail a target by themselves in V1",
       "cache must preserve structured trigger bits and recompute aggregate "
       "warning_summary counts",
       true,
       true,
       true,
       false,
       true,
       true},
      {"node_support_matrix",
       "node_by_use_support_join",
       {"associative", "commutative", "additive_over_distinct_rows"},
       "same active identity, target component, split, node id, exposure use, "
       "and target-opportunity denominator policy",
       "duplicate node-support rows collapse by parent exposure digest and "
       "node id; distinct support rows add",
       "cross-component joins and synthetic backfill from MDN rows to VICReg "
       "readiness are forbidden",
       "cache must preserve node identity, use, denominator, and parent "
       "exposure digest",
       true,
       true,
       false,
       true,
       true,
       true},
      {"deficit_vector",
       "stable_deficit_key_priority_join",
       {"associative", "commutative", "idempotent"},
       "same target proof identity and active identity",
       "duplicate deficit keys collapse after preserving the highest-priority "
       "message for the key",
       "deficits are advisory/planning evidence only; joining them cannot "
       "satisfy a target",
       "cache must preserve stable deficit keys, priority classes, units, and "
       "missing intervals",
       true,
       true,
       true,
       false,
       true,
       false}};
}

struct lattice_mdn_distribution_calibration_entry_t {
  std::string metric{};
  std::string statistic_family{};
  std::vector<std::string> required_runtime_fields{};
  std::string unit{};
  std::string threshold_direction{};
  std::string warning_kind{"mdn_distribution_calibration"};
  std::string readiness_effect{"non_blocking_warning_only"};
  std::string uncertainty_method{};
  bool available_in_v1{false};
  bool performance_gate{false};
};

struct lattice_mdn_distribution_calibration_diagnostic_entry_t {
  std::string metric{};
  std::string diagnostic_family{};
  std::string evidence_basis{};
  std::vector<std::string> runtime_fields{};
  std::string sample_count_field{};
  std::string point_estimate_field{};
  std::string uncertainty_method{};
  std::vector<std::string> uncertainty_bound_fields{};
  std::string checkpoint_binding{};
  std::string split_binding{};
  std::string active_identity_binding{};
  std::string warning_metric{};
  std::string availability{};
  bool validation_run_binding_required{true};
  bool exact_mdn_checkpoint_required{true};
  bool representation_checkpoint_required{true};
  bool split_policy_identity_required{true};
  bool active_identity_required{true};
  bool warning_only{true};
  bool performance_gate{false};
};

struct lattice_representation_geometry_entry_t {
  std::string metric{};
  std::string statistic_family{};
  std::vector<std::string> required_runtime_fields{};
  std::string unit{};
  std::string threshold_direction{};
  std::string warning_kind{"representation_health"};
  std::string readiness_effect{"non_blocking_warning_only"};
  std::string v1_authority{"representation_health_facts"};
  bool available_in_v1{true};
  bool geometry_metric{false};
  bool performance_gate{false};
  bool future_hard_gate_candidate{false};
};

struct lattice_performance_uncertainty_policy_entry_t {
  std::string policy{};
  std::string statistic_family{};
  std::vector<std::string> applies_to{};
  std::string estimator{};
  std::string confidence_policy{};
  std::vector<std::string> required_runtime_fields{};
  std::string gate_rule{};
  std::string v1_scope{};
  std::string future_requirement{};
  bool available_in_v1{false};
  bool performance_gate_authority{false};
  bool point_estimate_gate_allowed{false};
  bool confidence_bound_required{true};
  bool selection_leakage_policy_required{true};
};

struct lattice_validation_performance_scope_policy_entry_t {
  std::string policy{};
  std::string evidence_scope{};
  std::string authority{};
  std::string allowed_claim{};
  std::string forbidden_claim{};
  std::string future_evidence_required{};
  bool evaluation_readiness_authority{false};
  bool performance_gate_authority{false};
  bool uncertainty_required_for_future_gate{true};
  bool available_in_v1{true};
  bool runtime_executor{false};
};

struct lattice_validation_performance_evidence_policy_entry_t {
  std::string policy{};
  std::string metric{};
  std::string metric_family{};
  std::string fact_schema{};
  std::string split_field{};
  std::vector<std::string> checkpoint_identity_fields{};
  std::string evaluated_checkpoint_binding_field{};
  std::string sample_count_field{};
  std::string point_estimate_field{};
  std::string uncertainty_method{};
  std::vector<std::string> uncertainty_bound_fields{};
  std::string target_syntax{};
  std::string selection_leakage_policy{};
  std::vector<std::string> fail_closed_cases{};
  std::string readiness_effect{};
  bool available_in_v3c{false};
  bool performance_gate_authority{false};
  bool warning_visibility{true};
  bool future_gate_candidate{false};
  bool requires_active_identity{true};
  bool requires_exact_checkpoint_binding{true};
  bool requires_selection_signal_clean{true};
  bool requires_uncertainty_bounds{true};
};

[[nodiscard]] inline std::vector<
    lattice_validation_performance_scope_policy_entry_t>
lattice_validation_performance_scope_policy_vocabulary() {
  return {
      {"validation_eval_readiness_scope", "channel_mdn_validation_eval_ready",
       "clean_evaluation_readiness",
       "run-mode evaluation evidence over validation_holdout with "
       "evaluation_metric coverage, zero optimizer mutation, exact MDN "
       "checkpoint binding, exact input checkpoint edges, matching "
       "representation lineage, and no output checkpoint",
       "model quality, deployability, or performance superiority",
       "future channel_mdn_validation_performance_ready target with explicit "
       "metrics, uncertainty, and thresholds",
       true, false, true, true, false},
      {"evaluation_metric_coverage_not_quality",
       "LATTICE_REQUIRES USE=evaluation_metric", "evaluation_coverage_proof",
       "the evaluated checkpoint was run over enough trusted validation "
       "anchors",
       "that loss, NLL, calibration, or economic utility is acceptable",
       "performance target clauses over metric estimates and confidence "
       "intervals",
       true, false, true, true, false},
      {"mdn_calibration_warning_not_gate",
       "LATTICE_WARN KIND=mdn_distribution_calibration",
       "non_blocking_warning_visibility",
       "surface available mean_nll warnings and future calibration metrics "
       "without changing readiness status",
       "hard validation performance acceptance or checkpoint ranking",
       "future performance target with statistical uncertainty and explicit "
       "gate semantics",
       false, false, true, true, false},
      {"future_validation_performance_target",
       "future channel_mdn_validation_performance_ready",
       "future_performance_authority",
       "future hard or warning performance checks after metrics, sample size, "
       "uncertainty, and selection-signal policy are explicit",
       "implicit promotion of V1 evaluation readiness into performance "
       "approval",
       "proper scoring/calibration metrics with "
       "Wilson/bootstrap/standard-error "
       "uncertainty and selection-leakage protection",
       false, true, true, false, false}};
}

[[nodiscard]] inline std::vector<
    lattice_validation_performance_evidence_policy_entry_t>
lattice_validation_performance_evidence_policy_vocabulary() {
  const std::vector<std::string> active_checkpoint_identity{
      "protocol_contract_fingerprint",
      "graph_order_fingerprint",
      "source_cursor_token",
      "split_policy_fingerprint",
      "checkpoint_path",
      "checkpoint_id",
      "checkpoint_file_digest"};
  const std::vector<std::string> fail_closed{
      "missing_uncertainty", "wrong_checkpoint_binding", "stale_identity",
      "selector_leakage"};
  return {
      {"valid_target_fraction_wilson_visibility",
       "valid_target_fraction",
       "binomial_support_fraction",
       "kikijyeba.lattice.exposure_fact.v1",
       "split_id|source_range|anchor_index_range",
       active_checkpoint_identity,
       "loaded_mdn_checkpoint_path",
       "valid_target_opportunity_count",
       "valid_target_fraction",
       "wilson_score_interval_95",
       {"valid_target_wilson_lower_95", "valid_target_wilson_upper_95"},
       "LATTICE_TARGET TARGET_CLASS=evaluation_readiness "
       "MIN_VALID_TARGET_FRACTION",
       "selection_signal_policy_summary.known_selector_path_fail_closed",
       fail_closed,
       "readiness_support_visibility",
       true,
       false,
       true,
       false,
       true,
       true,
       true,
       true},
      {"mean_nll_warning_visibility",
       "mean_nll",
       "proper_scoring_loss",
       "kikijyeba.lattice.exposure_fact.v1",
       "split_id|source_range|anchor_index_range",
       active_checkpoint_identity,
       "loaded_mdn_checkpoint_path",
       "valid_target_count",
       "mean_nll",
       "none_point_estimate_only",
       {},
       "LATTICE_WARN KIND=mdn_distribution_calibration METRIC=mean_nll",
       "selection_signal_policy_summary.known_selector_path_fail_closed",
       fail_closed,
       "non_blocking_warning_visibility",
       true,
       false,
       true,
       false,
       true,
       true,
       true,
       false},
      {"future_mean_nll_performance_gate",
       "mean_nll",
       "proper_scoring_loss",
       "future kikijyeba.lattice.validation_performance_fact.v1",
       "validation_split_id",
       {"protocol_contract_fingerprint", "graph_order_fingerprint",
        "source_cursor_token", "split_policy_fingerprint",
        "evaluated_checkpoint_id", "evaluated_checkpoint_file_digest"},
       "evaluated_checkpoint_id|evaluated_checkpoint_file_digest",
       "metric_sample_count",
       "metric_mean",
       "bootstrap_or_standard_error_95",
       {"metric_lower_95", "metric_upper_95"},
       "future TARGET_CLASS=validation_performance METRIC=mean_nll",
       "selection_signal_policy_summary.known_selector_path_fail_closed",
       fail_closed,
       "deferred_hard_gate_candidate",
       false,
       false,
       false,
       true,
       true,
       true,
       true,
       true},
      {"future_calibration_fraction_performance_gate",
       "predictive_interval_coverage_error|tail_coverage_error|"
       "calibration_slope_error",
       "distribution_calibration_fraction",
       "future kikijyeba.lattice.validation_performance_fact.v1",
       "validation_split_id",
       {"protocol_contract_fingerprint", "graph_order_fingerprint",
        "source_cursor_token", "split_policy_fingerprint",
        "evaluated_checkpoint_id", "evaluated_checkpoint_file_digest"},
       "evaluated_checkpoint_id|evaluated_checkpoint_file_digest",
       "calibration_trial_count",
       "metric_error",
       "wilson_or_bootstrap_or_standard_error_95",
       {"metric_error_lower_95", "metric_error_upper_95"},
       "future TARGET_CLASS=validation_performance "
       "METRIC=calibration_error",
       "selection_signal_policy_summary.known_selector_path_fail_closed",
       fail_closed,
       "deferred_hard_gate_candidate",
       false,
       false,
       false,
       true,
       true,
       true,
       true,
       true},
      {"performance_acceptance_syntax_guard",
       "performance_acceptance_target",
       "target_syntax_policy",
       "kikijyeba.lattice.target.dsl",
       "OVER_SPLIT|PROTECT_SPLIT",
       {"EVALUATED_CHECKPOINT_SOURCE", "target_spec_fingerprint",
        "split_policy_fingerprint"},
       "EVALUATED_CHECKPOINT_SOURCE",
       "metric_sample_count",
       "conservative_metric_bound",
       "declared_metric_uncertainty",
       {"metric_lower_95", "metric_upper_95"},
       "future TARGET_CLASS=validation_performance is explicit and currently "
       "fail-closed",
       "selection_signal forbidden-use guard over protected holdout",
       fail_closed,
       "syntax_guard_no_implicit_promotion",
       false,
       false,
       false,
       true,
       true,
       true,
       true,
       true}};
}

struct lattice_validation_performance_scope_policy_summary_t {
  std::string schema{
      "kikijyeba.lattice.validation_performance_scope_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t available_in_v1_count{0};
  std::int64_t deferred_future_count{0};
  std::int64_t evaluation_readiness_authority_count{0};
  std::int64_t performance_gate_authority_count{0};
  std::int64_t uncertainty_required_for_future_gate_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t empty_field_count{0};
  bool validation_eval_readiness_scope_available{false};
  bool evaluation_metric_coverage_not_quality_present{false};
  bool mdn_calibration_warning_not_gate_present{false};
  bool future_validation_performance_target_deferred{false};
  bool v1_evaluation_readiness_not_performance_gate{false};
  bool future_gate_requires_uncertainty{false};
  bool future_gate_mentions_selection_leakage{false};
  bool all_policies_require_uncertainty_for_future_gate{false};
  bool no_runtime_executor{false};
  bool all_policies_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> policy_names{};
  std::vector<std::string> summary_issues{};
};

struct lattice_validation_performance_evidence_policy_summary_t {
  std::string schema{
      "kikijyeba.lattice.validation_performance_evidence_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t available_in_v3c_count{0};
  std::int64_t deferred_future_count{0};
  std::int64_t performance_gate_authority_count{0};
  std::int64_t warning_visibility_count{0};
  std::int64_t future_gate_candidate_count{0};
  std::int64_t active_identity_required_count{0};
  std::int64_t exact_checkpoint_binding_required_count{0};
  std::int64_t selection_signal_clean_required_count{0};
  std::int64_t uncertainty_bounds_required_count{0};
  std::int64_t point_estimate_without_uncertainty_count{0};
  std::int64_t point_estimate_without_uncertainty_gate_count{0};
  std::int64_t missing_fail_closed_case_count{0};
  std::int64_t empty_field_count{0};
  bool finite_supported_metrics_documented{false};
  bool fact_fields_name_split_checkpoint_sample_uncertainty{false};
  bool available_point_estimates_are_bounded_or_warning_only{false};
  bool no_point_estimate_gate_without_uncertainty{false};
  bool exact_checkpoint_binding_required{false};
  bool active_identity_required{false};
  bool selection_signal_clean_required{false};
  bool default_warning_visibility{false};
  bool no_v3c_performance_gate_authority{false};
  bool readiness_syntax_distinct_from_performance_acceptance{false};
  bool failure_cases_fail_closed_named{false};
  bool mean_nll_warning_only_until_uncertainty{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> policy_names{};
  std::vector<std::string> supported_metrics{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_validation_performance_scope_policy_summary_t
lattice_validation_performance_scope_policy_summary() {
  lattice_validation_performance_scope_policy_summary_t out{};
  const auto vocabulary =
      lattice_validation_performance_scope_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };

  for (const auto &entry : vocabulary) {
    out.policy_names.push_back(entry.policy);
    if (entry.available_in_v1) {
      ++out.available_in_v1_count;
    } else {
      ++out.deferred_future_count;
    }
    if (entry.evaluation_readiness_authority) {
      ++out.evaluation_readiness_authority_count;
    }
    if (entry.performance_gate_authority) {
      ++out.performance_gate_authority_count;
    }
    if (entry.uncertainty_required_for_future_gate) {
      ++out.uncertainty_required_for_future_gate_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.policy.empty() || entry.evidence_scope.empty() ||
        entry.authority.empty() || entry.allowed_claim.empty() ||
        entry.forbidden_claim.empty() ||
        entry.future_evidence_required.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto readiness = find_policy("validation_eval_readiness_scope");
  const auto coverage = find_policy("evaluation_metric_coverage_not_quality");
  const auto calibration = find_policy("mdn_calibration_warning_not_gate");
  const auto future = find_policy("future_validation_performance_target");

  out.validation_eval_readiness_scope_available =
      readiness != vocabulary.end() && readiness->available_in_v1 &&
      readiness->evaluation_readiness_authority &&
      !readiness->performance_gate_authority &&
      readiness->forbidden_claim.find("model quality") != std::string::npos;
  out.evaluation_metric_coverage_not_quality_present =
      coverage != vocabulary.end() && coverage->available_in_v1 &&
      coverage->evaluation_readiness_authority &&
      !coverage->performance_gate_authority &&
      coverage->forbidden_claim.find("acceptable") != std::string::npos;
  out.mdn_calibration_warning_not_gate_present =
      calibration != vocabulary.end() && calibration->available_in_v1 &&
      calibration->authority == "non_blocking_warning_visibility" &&
      !calibration->evaluation_readiness_authority &&
      !calibration->performance_gate_authority;
  out.future_validation_performance_target_deferred =
      future != vocabulary.end() && !future->available_in_v1 &&
      !future->evaluation_readiness_authority &&
      future->performance_gate_authority;
  out.v1_evaluation_readiness_not_performance_gate =
      out.available_in_v1_count == 3 &&
      out.performance_gate_authority_count == 1 &&
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        return !entry.available_in_v1 || !entry.performance_gate_authority;
      });
  out.future_gate_requires_uncertainty =
      future != vocabulary.end() &&
      future->uncertainty_required_for_future_gate &&
      future->future_evidence_required.find("uncertainty") != std::string::npos;
  out.future_gate_mentions_selection_leakage =
      future != vocabulary.end() &&
      future->future_evidence_required.find("selection-leakage") !=
          std::string::npos;
  out.all_policies_require_uncertainty_for_future_gate =
      out.uncertainty_required_for_future_gate_count == out.policy_count &&
      out.policy_count > 0;
  out.no_runtime_executor = out.runtime_executor_count == 0;
  out.all_policies_have_claim_text = out.empty_field_count == 0;

  if (out.policy_count != 4) {
    out.summary_issues.push_back(
        "validation performance scope policy vocabulary must contain 4 "
        "policies");
  }
  if (out.available_in_v1_count != 3 || out.deferred_future_count != 1) {
    out.summary_issues.push_back(
        "validation performance scope must expose 3 V1 rows and 1 future row");
  }
  if (!out.validation_eval_readiness_scope_available) {
    out.summary_issues.push_back(
        "validation eval readiness must be V1 readiness authority, not model "
        "quality");
  }
  if (!out.evaluation_metric_coverage_not_quality_present) {
    out.summary_issues.push_back(
        "evaluation metric coverage must not imply quality acceptance");
  }
  if (!out.mdn_calibration_warning_not_gate_present) {
    out.summary_issues.push_back(
        "MDN calibration warnings must remain non-blocking visibility");
  }
  if (!out.future_validation_performance_target_deferred) {
    out.summary_issues.push_back(
        "validation performance target authority must remain future work");
  }
  if (!out.v1_evaluation_readiness_not_performance_gate) {
    out.summary_issues.push_back(
        "available V1 validation-evaluation policies must not be performance "
        "gates");
  }
  if (!out.future_gate_requires_uncertainty) {
    out.summary_issues.push_back(
        "future validation performance gate must require uncertainty");
  }
  if (!out.future_gate_mentions_selection_leakage) {
    out.summary_issues.push_back(
        "future validation performance gate must mention selection-leakage "
        "protection");
  }
  if (!out.all_policies_require_uncertainty_for_future_gate) {
    out.summary_issues.push_back(
        "all validation performance scope rows must require uncertainty for "
        "future gates");
  }
  if (!out.no_runtime_executor) {
    out.summary_issues.push_back(
        "validation performance scope policy must remain read-only");
  }
  if (!out.all_policies_have_claim_text) {
    out.summary_issues.push_back(
        "validation performance scope policies must declare evidence, "
        "authority, claims, and future evidence");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline lattice_validation_performance_evidence_policy_summary_t
lattice_validation_performance_evidence_policy_summary() {
  lattice_validation_performance_evidence_policy_summary_t out{};
  const auto vocabulary =
      lattice_validation_performance_evidence_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const std::set<std::string> required_fail_closed{
      "missing_uncertainty", "wrong_checkpoint_binding", "stale_identity",
      "selector_leakage"};
  std::set<std::string> supported_metrics{};
  bool all_fact_rows_name_required_fields = true;

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };

  for (const auto &entry : vocabulary) {
    out.policy_names.push_back(entry.policy);
    if (entry.available_in_v3c) {
      ++out.available_in_v3c_count;
      supported_metrics.insert(entry.metric);
    } else {
      ++out.deferred_future_count;
    }
    if (entry.performance_gate_authority) {
      ++out.performance_gate_authority_count;
    }
    if (entry.warning_visibility) {
      ++out.warning_visibility_count;
    }
    if (entry.future_gate_candidate) {
      ++out.future_gate_candidate_count;
    }
    if (entry.requires_active_identity) {
      ++out.active_identity_required_count;
    }
    if (entry.requires_exact_checkpoint_binding) {
      ++out.exact_checkpoint_binding_required_count;
    }
    if (entry.requires_selection_signal_clean) {
      ++out.selection_signal_clean_required_count;
    }
    if (entry.requires_uncertainty_bounds) {
      ++out.uncertainty_bounds_required_count;
    }
    if (entry.uncertainty_bound_fields.empty()) {
      ++out.point_estimate_without_uncertainty_count;
      if (entry.performance_gate_authority) {
        ++out.point_estimate_without_uncertainty_gate_count;
      }
    }
    if (entry.policy.empty() || entry.metric.empty() ||
        entry.metric_family.empty() || entry.fact_schema.empty() ||
        entry.split_field.empty() || entry.checkpoint_identity_fields.empty() ||
        entry.evaluated_checkpoint_binding_field.empty() ||
        entry.sample_count_field.empty() ||
        entry.point_estimate_field.empty() ||
        entry.uncertainty_method.empty() || entry.target_syntax.empty() ||
        entry.selection_leakage_policy.empty() ||
        entry.fail_closed_cases.empty() || entry.readiness_effect.empty()) {
      ++out.empty_field_count;
      all_fact_rows_name_required_fields = false;
    }
    for (const auto &required_case : required_fail_closed) {
      if (std::find(entry.fail_closed_cases.begin(),
                    entry.fail_closed_cases.end(),
                    required_case) == entry.fail_closed_cases.end()) {
        ++out.missing_fail_closed_case_count;
      }
    }
  }

  out.supported_metrics.assign(supported_metrics.begin(),
                               supported_metrics.end());
  const auto support = find_policy("valid_target_fraction_wilson_visibility");
  const auto mean_nll = find_policy("mean_nll_warning_visibility");
  const auto syntax_guard = find_policy("performance_acceptance_syntax_guard");

  out.finite_supported_metrics_documented =
      out.policy_count == 5 && out.available_in_v3c_count == 2 &&
      supported_metrics.count("valid_target_fraction") != 0 &&
      supported_metrics.count("mean_nll") != 0;
  out.fact_fields_name_split_checkpoint_sample_uncertainty =
      all_fact_rows_name_required_fields &&
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        return !entry.split_field.empty() &&
               !entry.checkpoint_identity_fields.empty() &&
               !entry.evaluated_checkpoint_binding_field.empty() &&
               !entry.sample_count_field.empty() &&
               !entry.uncertainty_method.empty();
      });
  out.available_point_estimates_are_bounded_or_warning_only =
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        if (!entry.available_in_v3c) {
          return true;
        }
        return !entry.uncertainty_bound_fields.empty() ||
               (entry.warning_visibility && !entry.performance_gate_authority);
      });
  out.no_point_estimate_gate_without_uncertainty =
      out.point_estimate_without_uncertainty_gate_count == 0;
  out.exact_checkpoint_binding_required =
      out.exact_checkpoint_binding_required_count == out.policy_count &&
      out.policy_count > 0;
  out.active_identity_required =
      out.active_identity_required_count == out.policy_count &&
      out.policy_count > 0;
  out.selection_signal_clean_required =
      out.selection_signal_clean_required_count == out.policy_count &&
      out.policy_count > 0;
  out.default_warning_visibility =
      out.available_in_v3c_count == out.warning_visibility_count &&
      out.available_in_v3c_count == 2;
  out.no_v3c_performance_gate_authority =
      out.performance_gate_authority_count == 0;
  out.readiness_syntax_distinct_from_performance_acceptance =
      support != vocabulary.end() &&
      support->target_syntax.find("evaluation_readiness") !=
          std::string::npos &&
      mean_nll != vocabulary.end() &&
      mean_nll->target_syntax.find("LATTICE_WARN") != std::string::npos &&
      syntax_guard != vocabulary.end() &&
      syntax_guard->target_syntax.find("validation_performance") !=
          std::string::npos &&
      !syntax_guard->available_in_v3c;
  out.failure_cases_fail_closed_named =
      out.missing_fail_closed_case_count == 0 && out.policy_count > 0;
  out.mean_nll_warning_only_until_uncertainty =
      mean_nll != vocabulary.end() && mean_nll->available_in_v3c &&
      mean_nll->warning_visibility && !mean_nll->performance_gate_authority &&
      mean_nll->uncertainty_method == "none_point_estimate_only" &&
      mean_nll->uncertainty_bound_fields.empty();

  if (!out.finite_supported_metrics_documented) {
    out.summary_issues.push_back(
        "V3-C must document exactly the finite available metrics "
        "valid_target_fraction and mean_nll");
  }
  if (!out.fact_fields_name_split_checkpoint_sample_uncertainty) {
    out.summary_issues.push_back(
        "validation performance evidence policies must name split, checkpoint "
        "identity, evaluated binding, sample count, and uncertainty method");
  }
  if (!out.available_point_estimates_are_bounded_or_warning_only) {
    out.summary_issues.push_back(
        "available point estimates must either carry uncertainty bounds or "
        "remain warning visibility only");
  }
  if (!out.no_point_estimate_gate_without_uncertainty) {
    out.summary_issues.push_back(
        "point estimates without uncertainty must not become performance "
        "gates");
  }
  if (!out.exact_checkpoint_binding_required) {
    out.summary_issues.push_back(
        "validation performance evidence must require exact checkpoint "
        "binding");
  }
  if (!out.active_identity_required) {
    out.summary_issues.push_back(
        "validation performance evidence must require active identity");
  }
  if (!out.selection_signal_clean_required) {
    out.summary_issues.push_back(
        "validation performance evidence must require selection-signal "
        "cleanliness");
  }
  if (!out.default_warning_visibility) {
    out.summary_issues.push_back(
        "V3-C available performance evidence must default to visibility or "
        "warnings");
  }
  if (!out.no_v3c_performance_gate_authority) {
    out.summary_issues.push_back(
        "V3-C must not grant performance gate authority by default");
  }
  if (!out.readiness_syntax_distinct_from_performance_acceptance) {
    out.summary_issues.push_back(
        "readiness, warning, and future validation-performance syntax must be "
        "distinct");
  }
  if (!out.failure_cases_fail_closed_named) {
    out.summary_issues.push_back(
        "missing uncertainty, wrong checkpoint binding, stale identity, and "
        "selector leakage must be named fail-closed cases");
  }
  if (!out.mean_nll_warning_only_until_uncertainty) {
    out.summary_issues.push_back(
        "mean_nll must remain warning-only until uncertainty is emitted");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_node_support_scope_policy_entry_t {
  std::string policy{};
  std::string evidence_scope{};
  std::string authority{};
  std::string allowed_claim{};
  std::string forbidden_claim{};
  std::string future_evidence_required{};
  bool mdn_node_support_authority{false};
  bool vicreg_per_node_readiness_authority{false};
  bool synthetic_backfill_allowed{false};
  bool available_in_v1{true};
  bool runtime_executor{false};
};

[[nodiscard]] inline std::vector<lattice_node_support_scope_policy_entry_t>
lattice_node_support_scope_policy_vocabulary() {
  return {
      {"mdn_node_support_scope",
       "lattice.node_exposure.fact and node_support_summaries",
       "mdn_head_support_visibility",
       "routed, active, trained, evaluated, valid-target, NLL, balance, and "
       "Wilson support visibility for MDN node heads",
       "per-node VICReg readiness or shared-encoder representation coverage",
       "none for V1 MDN support visibility", true, false, false, true, false},
      {"vicreg_shared_encoder_boundary",
       "channel_representation_vicreg exposure facts, checkpoint facts, and "
       "representation_health_facts; historical representation_vicreg facts "
       "remain replay-readable",
       "shared_encoder_readiness",
       "shared VICReg train-core readiness, checkpoint lineage, leakage "
       "cleanliness, and representation-health warnings",
       "per-node representation readiness derived from MDN node facts",
       "honest NodeLift or representation-emitted node-support summaries",
       false, false, false, true, false},
      {"future_representation_node_support_fact",
       "future lattice.representation_node_support.fact",
       "future_node_support_for_shared_representation",
       "future node-local support visibility for shared representations after "
       "NodeLift/representation reports expose it",
       "synthetic backfill from downstream MDN support rows",
       "component-scoped NodeLift/representation support facts with source "
       "identity, split, use, and node denominators",
       false, false, false, false, false},
      {"node_support_matrix_join_boundary",
       "product_evidence_state.node_support_matrix",
       "identity_scoped_mdn_support_matrix",
       "join inherited MDN node-support rows by node identity, exposure use, "
       "target component, split, and active graph/source identity",
       "cross-component joins that convert MDN head support into VICReg "
       "per-node readiness",
       "future component-scoped representation support matrix for shared "
       "encoders",
       true, false, false, true, false}};
}

struct lattice_node_support_scope_policy_summary_t {
  std::string schema{"kikijyeba.lattice.node_support_scope_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t available_in_v1_count{0};
  std::int64_t deferred_future_count{0};
  std::int64_t mdn_node_support_authority_count{0};
  std::int64_t vicreg_per_node_readiness_authority_count{0};
  std::int64_t synthetic_backfill_allowed_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t empty_field_count{0};
  bool mdn_node_support_scope_available{false};
  bool vicreg_shared_encoder_boundary_available{false};
  bool future_representation_node_support_deferred{false};
  bool node_support_matrix_join_boundary_available{false};
  bool no_vicreg_per_node_readiness_authority{false};
  bool no_synthetic_backfill{false};
  bool no_runtime_executor{false};
  bool future_representation_requires_nodelift{false};
  bool mdn_scope_forbids_vicreg_per_node_readiness{false};
  bool vicreg_boundary_forbids_mdn_backfill{false};
  bool matrix_join_boundary_forbids_cross_component{false};
  bool all_policies_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> policy_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_node_support_scope_policy_summary_t
lattice_node_support_scope_policy_summary() {
  lattice_node_support_scope_policy_summary_t out{};
  const auto vocabulary = lattice_node_support_scope_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };

  for (const auto &entry : vocabulary) {
    out.policy_names.push_back(entry.policy);
    if (entry.available_in_v1) {
      ++out.available_in_v1_count;
    } else {
      ++out.deferred_future_count;
    }
    if (entry.mdn_node_support_authority) {
      ++out.mdn_node_support_authority_count;
    }
    if (entry.vicreg_per_node_readiness_authority) {
      ++out.vicreg_per_node_readiness_authority_count;
    }
    if (entry.synthetic_backfill_allowed) {
      ++out.synthetic_backfill_allowed_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.policy.empty() || entry.evidence_scope.empty() ||
        entry.authority.empty() || entry.allowed_claim.empty() ||
        entry.forbidden_claim.empty() ||
        entry.future_evidence_required.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto mdn_scope = find_policy("mdn_node_support_scope");
  const auto vicreg_boundary = find_policy("vicreg_shared_encoder_boundary");
  const auto future_representation =
      find_policy("future_representation_node_support_fact");
  const auto matrix_join = find_policy("node_support_matrix_join_boundary");

  out.mdn_node_support_scope_available =
      mdn_scope != vocabulary.end() && mdn_scope->available_in_v1 &&
      mdn_scope->mdn_node_support_authority &&
      !mdn_scope->vicreg_per_node_readiness_authority;
  out.vicreg_shared_encoder_boundary_available =
      vicreg_boundary != vocabulary.end() && vicreg_boundary->available_in_v1 &&
      !vicreg_boundary->mdn_node_support_authority &&
      !vicreg_boundary->vicreg_per_node_readiness_authority &&
      vicreg_boundary->allowed_claim.find("shared VICReg") != std::string::npos;
  out.future_representation_node_support_deferred =
      future_representation != vocabulary.end() &&
      !future_representation->available_in_v1 &&
      !future_representation->mdn_node_support_authority &&
      !future_representation->vicreg_per_node_readiness_authority;
  out.node_support_matrix_join_boundary_available =
      matrix_join != vocabulary.end() && matrix_join->available_in_v1 &&
      matrix_join->mdn_node_support_authority &&
      matrix_join->evidence_scope.find("node_support_matrix") !=
          std::string::npos;
  out.no_vicreg_per_node_readiness_authority =
      out.vicreg_per_node_readiness_authority_count == 0;
  out.no_synthetic_backfill = out.synthetic_backfill_allowed_count == 0;
  out.no_runtime_executor = out.runtime_executor_count == 0;
  out.future_representation_requires_nodelift =
      future_representation != vocabulary.end() &&
      future_representation->future_evidence_required.find("NodeLift") !=
          std::string::npos;
  out.mdn_scope_forbids_vicreg_per_node_readiness =
      mdn_scope != vocabulary.end() &&
      mdn_scope->forbidden_claim.find("VICReg") != std::string::npos;
  out.vicreg_boundary_forbids_mdn_backfill =
      vicreg_boundary != vocabulary.end() &&
      vicreg_boundary->forbidden_claim.find("MDN node facts") !=
          std::string::npos &&
      vicreg_boundary->future_evidence_required.find("NodeLift") !=
          std::string::npos;
  out.matrix_join_boundary_forbids_cross_component =
      matrix_join != vocabulary.end() &&
      matrix_join->forbidden_claim.find("cross-component") != std::string::npos;
  out.all_policies_have_claim_text = out.empty_field_count == 0;

  if (out.policy_count != 4) {
    out.summary_issues.push_back(
        "node-support scope policy vocabulary must contain 4 policies");
  }
  if (out.available_in_v1_count != 3 || out.deferred_future_count != 1) {
    out.summary_issues.push_back(
        "node-support scope policy must expose 3 V1 rows and 1 future row");
  }
  if (!out.mdn_node_support_scope_available) {
    out.summary_issues.push_back(
        "MDN node-support scope must be available as V1 MDN-head visibility");
  }
  if (!out.vicreg_shared_encoder_boundary_available) {
    out.summary_issues.push_back(
        "VICReg shared-encoder boundary must be available without per-node "
        "readiness authority");
  }
  if (!out.future_representation_node_support_deferred) {
    out.summary_issues.push_back(
        "future representation node-support facts must remain deferred");
  }
  if (!out.node_support_matrix_join_boundary_available) {
    out.summary_issues.push_back(
        "node-support matrix join boundary must be available for MDN support");
  }
  if (!out.no_vicreg_per_node_readiness_authority) {
    out.summary_issues.push_back(
        "V1 node-support scope must not grant VICReg per-node readiness");
  }
  if (!out.no_synthetic_backfill) {
    out.summary_issues.push_back(
        "V1 node-support scope must not allow synthetic backfill");
  }
  if (!out.no_runtime_executor) {
    out.summary_issues.push_back(
        "node-support scope policy must remain read-only, not runtime "
        "execution");
  }
  if (!out.future_representation_requires_nodelift) {
    out.summary_issues.push_back(
        "future representation node support must require NodeLift or "
        "representation-emitted facts");
  }
  if (!out.mdn_scope_forbids_vicreg_per_node_readiness) {
    out.summary_issues.push_back(
        "MDN node-support scope must forbid VICReg per-node readiness claims");
  }
  if (!out.vicreg_boundary_forbids_mdn_backfill) {
    out.summary_issues.push_back(
        "VICReg boundary must reject downstream MDN-node backfill");
  }
  if (!out.matrix_join_boundary_forbids_cross_component) {
    out.summary_issues.push_back(
        "node-support matrix joins must forbid cross-component readiness "
        "conversion");
  }
  if (!out.all_policies_have_claim_text) {
    out.summary_issues.push_back(
        "node-support scope policies must declare evidence, authority, claims, "
        "and future evidence");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::vector<lattice_representation_geometry_entry_t>
lattice_representation_geometry_vocabulary() {
  return {{"mean_invariance_loss",
           "vicreg_loss_component",
           {"lattice.exposure.fact.mean_invariance_loss"},
           "loss",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           true},
          {"mean_variance_loss",
           "vicreg_loss_component",
           {"lattice.exposure.fact.mean_variance_loss"},
           "loss",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           true},
          {"mean_covariance_loss",
           "vicreg_loss_component",
           {"lattice.exposure.fact.mean_covariance_loss"},
           "loss",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           true},
          {"last_grad_norm",
           "optimization_health",
           {"lattice.exposure.fact.last_grad_norm"},
           "gradient_norm",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"max_grad_norm",
           "optimization_health",
           {"lattice.exposure.fact.max_grad_norm"},
           "gradient_norm",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"total_valid_projection_rows",
           "projection_support",
           {"lattice.exposure.fact.total_valid_projection_rows"},
           "count",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"mean_adapter_valid_channel_time_fraction",
           "adapter_support",
           {"lattice.exposure.fact.mean_adapter_valid_channel_time_fraction"},
           "fraction",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"augmented_valid_feature_count",
           "augmented_view_support",
           {"lattice.exposure.fact.augmented_valid_feature_count"},
           "count",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"mean_augmented_valid_feature_fraction",
           "augmented_view_support",
           {"lattice.exposure.fact.mean_augmented_valid_feature_fraction"},
           "fraction",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"mean_augmented_feature_retention_fraction",
           "augmented_view_support",
           {"lattice.exposure.fact.mean_augmented_feature_retention_fraction"},
           "fraction",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           false},
          {"finite_parameter_check",
           "parameter_sanity",
           {"lattice.exposure.fact.finite_parameter_check"},
           "boolean",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           false,
           false,
           true},
          {"representation_embedding_dim",
           "geometry_dimension",
           {"lattice.exposure.fact.representation_embedding_dim"},
           "count",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_effective_rank",
           "spectrum_effective_rank",
           {"lattice.exposure.fact.representation_effective_rank"},
           "rank",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_effective_rank_fraction",
           "spectrum_effective_rank",
           {"lattice.exposure.fact.representation_effective_rank_fraction"},
           "fraction",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_min_dimension_variance",
           "variance_floor",
           {"lattice.exposure.fact.representation_min_dimension_variance"},
           "variance",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_max_dimension_variance",
           "variance_ceiling",
           {"lattice.exposure.fact.representation_max_dimension_variance"},
           "variance",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_condition_number",
           "spectrum_conditioning",
           {"lattice.exposure.fact.representation_condition_number"},
           "condition_number",
           "above",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true},
          {"representation_isotropy_score",
           "spectrum_isotropy",
           {"lattice.exposure.fact.representation_isotropy_score"},
           "fraction",
           "below",
           "representation_health",
           "non_blocking_warning_only",
           "representation_health_facts",
           true,
           true,
           false,
           true}};
}

struct lattice_representation_geometry_summary_t {
  std::string schema{"kikijyeba.lattice.representation_geometry.summary.v1"};
  std::int64_t metric_count{0};
  std::int64_t available_in_v1_count{0};
  std::int64_t geometry_metric_count{0};
  std::int64_t performance_gate_count{0};
  std::int64_t future_hard_gate_candidate_count{0};
  std::int64_t non_blocking_warning_count{0};
  std::int64_t representation_health_authority_count{0};
  std::int64_t empty_field_count{0};
  bool loss_component_metrics_present{false};
  bool optimization_health_metrics_present{false};
  bool projection_support_metric_present{false};
  bool adapter_support_metric_present{false};
  bool augmented_view_support_metrics_present{false};
  bool finite_parameter_check_present{false};
  bool embedding_spectrum_geometry_present{false};
  bool threshold_directions_present{false};
  bool all_metrics_available_in_v1{false};
  bool all_metrics_non_blocking_warnings{false};
  bool all_metrics_use_representation_health_authority{false};
  bool no_performance_gates{false};
  bool future_gate_candidates_are_not_active_gates{false};
  bool all_metrics_have_runtime_fields{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> metric_names{};
  std::vector<std::string> summary_issues{};
};

struct lattice_representation_geometry_gate_review_metric_t {
  std::string metric{};
  std::string unit{};
  bool future_hard_gate_candidate{false};
  bool geometry_metric{false};
  std::int64_t observed_count{0};
  double min_value{std::numeric_limits<double>::quiet_NaN()};
  double max_value{std::numeric_limits<double>::quiet_NaN()};
  double mean_value{std::numeric_limits<double>::quiet_NaN()};
  bool observed_distribution_available{false};
  bool proposed_threshold{false};
  bool promoted_hard_gate{false};
};

struct lattice_representation_geometry_gate_review_summary_t {
  std::string schema{
      "kikijyeba.lattice.representation_geometry_gate_review.summary.v1"};
  std::int64_t metric_count{0};
  std::int64_t geometry_metric_count{0};
  std::int64_t future_hard_gate_candidate_count{0};
  std::int64_t observed_run_count{0};
  std::int64_t observed_geometry_fact_count{0};
  std::int64_t observed_candidate_metric_count{0};
  std::int64_t observed_metric_sample_count{0};
  std::int64_t missing_geometry_fact_count{0};
  std::int64_t proposed_threshold_count{0};
  std::int64_t promoted_hard_gate_count{0};
  bool opt_in_target_syntax_required{true};
  bool missing_geometry_fails_closed_for_gate{true};
  bool hard_gate_default_enabled{false};
  bool thresholds_justified_by_observed_data{false};
  bool default_readiness_unchanged{true};
  bool no_performance_gate_authority{true};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<lattice_representation_geometry_gate_review_metric_t>
      metric_summaries{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_representation_geometry_summary_t
lattice_representation_geometry_summary() {
  lattice_representation_geometry_summary_t out{};
  const auto vocabulary = lattice_representation_geometry_vocabulary();
  out.metric_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_metric = [&](const std::string &metric) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.metric == metric; });
  };

  const auto family_count = [&](const std::string &family) {
    return static_cast<std::int64_t>(std::count_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.statistic_family == family; }));
  };

  for (const auto &entry : vocabulary) {
    out.metric_names.push_back(entry.metric);
    if (entry.available_in_v1) {
      ++out.available_in_v1_count;
    }
    if (entry.geometry_metric) {
      ++out.geometry_metric_count;
    }
    if (entry.performance_gate) {
      ++out.performance_gate_count;
    }
    if (entry.future_hard_gate_candidate) {
      ++out.future_hard_gate_candidate_count;
    }
    if (entry.readiness_effect == "non_blocking_warning_only") {
      ++out.non_blocking_warning_count;
    }
    if (entry.v1_authority == "representation_health_facts") {
      ++out.representation_health_authority_count;
    }
    if (entry.metric.empty() || entry.statistic_family.empty() ||
        entry.required_runtime_fields.empty() || entry.unit.empty() ||
        entry.threshold_direction.empty() || entry.warning_kind.empty() ||
        entry.readiness_effect.empty() || entry.v1_authority.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto finite_parameter = find_metric("finite_parameter_check");
  const auto effective_rank = find_metric("representation_effective_rank");
  const auto condition_number = find_metric("representation_condition_number");
  const auto isotropy = find_metric("representation_isotropy_score");

  out.loss_component_metrics_present =
      family_count("vicreg_loss_component") == 3;
  out.optimization_health_metrics_present =
      family_count("optimization_health") == 2;
  out.projection_support_metric_present =
      family_count("projection_support") == 1;
  out.adapter_support_metric_present = family_count("adapter_support") == 1;
  out.augmented_view_support_metrics_present =
      family_count("augmented_view_support") == 3;
  out.finite_parameter_check_present =
      finite_parameter != vocabulary.end() &&
      finite_parameter->unit == "boolean" &&
      finite_parameter->future_hard_gate_candidate;
  out.embedding_spectrum_geometry_present =
      out.geometry_metric_count == 7 && effective_rank != vocabulary.end() &&
      effective_rank->geometry_metric && condition_number != vocabulary.end() &&
      condition_number->geometry_metric && isotropy != vocabulary.end() &&
      isotropy->geometry_metric;
  out.threshold_directions_present =
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        return entry.threshold_direction == "above" ||
               entry.threshold_direction == "below";
      });
  out.all_metrics_available_in_v1 =
      out.available_in_v1_count == out.metric_count && out.metric_count > 0;
  out.all_metrics_non_blocking_warnings =
      out.non_blocking_warning_count == out.metric_count &&
      out.metric_count > 0 &&
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        return entry.warning_kind == "representation_health";
      });
  out.all_metrics_use_representation_health_authority =
      out.representation_health_authority_count == out.metric_count &&
      out.metric_count > 0;
  out.no_performance_gates = out.performance_gate_count == 0;
  out.future_gate_candidates_are_not_active_gates =
      out.future_hard_gate_candidate_count == 11 && out.no_performance_gates;
  out.all_metrics_have_runtime_fields = out.empty_field_count == 0;

  if (out.metric_count != 18) {
    out.summary_issues.push_back(
        "representation geometry vocabulary must contain 18 metrics");
  }
  if (!out.loss_component_metrics_present) {
    out.summary_issues.push_back(
        "VICReg loss component metrics must include invariance, variance, and "
        "covariance");
  }
  if (!out.optimization_health_metrics_present) {
    out.summary_issues.push_back(
        "optimization health metrics must include last and max gradient norm");
  }
  if (!out.projection_support_metric_present) {
    out.summary_issues.push_back("projection support metric must be present");
  }
  if (!out.adapter_support_metric_present) {
    out.summary_issues.push_back("adapter support metric must be present");
  }
  if (!out.augmented_view_support_metrics_present) {
    out.summary_issues.push_back(
        "augmented-view support metrics must include count, valid fraction, "
        "and retention fraction");
  }
  if (!out.finite_parameter_check_present) {
    out.summary_issues.push_back(
        "finite parameter check must remain a future hard-gate candidate");
  }
  if (!out.embedding_spectrum_geometry_present) {
    out.summary_issues.push_back(
        "embedding-spectrum geometry metrics must include rank, conditioning, "
        "variance, and isotropy");
  }
  if (!out.threshold_directions_present) {
    out.summary_issues.push_back(
        "representation geometry metrics must declare above/below threshold "
        "directions");
  }
  if (!out.all_metrics_available_in_v1) {
    out.summary_issues.push_back(
        "representation geometry metrics must remain V1-visible");
  }
  if (!out.all_metrics_non_blocking_warnings) {
    out.summary_issues.push_back(
        "representation geometry metrics must remain non-blocking warnings");
  }
  if (!out.all_metrics_use_representation_health_authority) {
    out.summary_issues.push_back(
        "representation geometry metrics must use representation_health fact "
        "authority");
  }
  if (!out.no_performance_gates) {
    out.summary_issues.push_back(
        "representation geometry must not grant performance gate authority");
  }
  if (!out.future_gate_candidates_are_not_active_gates) {
    out.summary_issues.push_back(
        "future hard-gate candidates must stay inactive in V1");
  }
  if (!out.all_metrics_have_runtime_fields) {
    out.summary_issues.push_back(
        "representation geometry metrics must declare runtime fields, units, "
        "threshold directions, and authority");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::optional<lattice_representation_geometry_entry_t>
lattice_representation_geometry_metric_entry(const std::string &metric) {
  const auto vocabulary = lattice_representation_geometry_vocabulary();
  const auto found =
      std::find_if(vocabulary.begin(), vocabulary.end(),
                   [&](const auto &entry) { return entry.metric == metric; });
  if (found == vocabulary.end()) {
    return std::nullopt;
  }
  return *found;
}

[[nodiscard]] inline bool
lattice_representation_geometry_metric_high_is_bad(const std::string &metric) {
  static const std::set<std::string> high_is_bad{
      "mean_invariance_loss",
      "mean_variance_loss",
      "mean_covariance_loss",
      "last_grad_norm",
      "max_grad_norm",
      "representation_max_dimension_variance",
      "representation_condition_number",
  };
  return high_is_bad.count(metric) != 0;
}

[[nodiscard]] inline bool
lattice_representation_geometry_metric_low_is_bad(const std::string &metric) {
  static const std::set<std::string> low_is_bad{
      "total_valid_projection_rows",
      "mean_adapter_valid_channel_time_fraction",
      "augmented_valid_feature_count",
      "mean_augmented_valid_feature_fraction",
      "mean_augmented_feature_retention_fraction",
      "finite_parameter_check",
      "representation_embedding_dim",
      "representation_effective_rank",
      "representation_effective_rank_fraction",
      "representation_min_dimension_variance",
      "representation_isotropy_score",
  };
  return low_is_bad.count(metric) != 0;
}

[[nodiscard]] inline std::optional<double>
lattice_representation_geometry_metric_value(
    const cuwacunu::hero::lattice::exposure::lattice_exposure_fact_t &fact,
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

inline void require_representation_geometry_threshold_dimension(
    const std::string &metric, double value, const std::string &field,
    const std::string &target_id) {
  const auto require_finite = [&]() {
    if (!std::isfinite(value)) {
      throw std::runtime_error("[lattice_target] " + field +
                               " must be finite for " + target_id);
    }
  };
  const auto require_non_negative = [&]() {
    require_finite();
    if (value < 0.0) {
      throw std::runtime_error("[lattice_target] " + field +
                               " must be non-negative for " + target_id);
    }
  };
  if (metric == "mean_adapter_valid_channel_time_fraction" ||
      metric == "mean_augmented_valid_feature_fraction" ||
      metric == "mean_augmented_feature_retention_fraction" ||
      metric == "finite_parameter_check" ||
      metric == "representation_effective_rank_fraction" ||
      metric == "representation_isotropy_score") {
    require_finite();
    if (value < 0.0 || value > 1.0) {
      throw std::runtime_error("[lattice_target] " + field +
                               " must be a fraction in [0, 1] for " +
                               target_id);
    }
  } else if (metric == "total_valid_projection_rows" ||
             metric == "augmented_valid_feature_count" ||
             metric == "representation_embedding_dim") {
    require_non_negative();
    if (std::floor(value) != value) {
      throw std::runtime_error("[lattice_target] " + field +
                               " must be an integral count threshold for " +
                               target_id);
    }
  } else if (metric == "representation_condition_number") {
    require_finite();
    if (value < 1.0) {
      throw std::runtime_error("[lattice_target] " + field +
                               " must be a condition_number value >= 1 for " +
                               target_id);
    }
  } else {
    require_non_negative();
  }
}

[[nodiscard]] inline lattice_representation_geometry_gate_review_summary_t
summarize_representation_geometry_gate_review(
    const std::vector<
        cuwacunu::hero::lattice::exposure::lattice_exposure_fact_t>
        &facts) {
  lattice_representation_geometry_gate_review_summary_t out{};
  const auto vocabulary = lattice_representation_geometry_vocabulary();
  out.metric_count = static_cast<std::int64_t>(vocabulary.size());

  std::set<std::string> observed_jobs;
  for (const auto &entry : vocabulary) {
    lattice_representation_geometry_gate_review_metric_t metric{};
    metric.metric = entry.metric;
    metric.unit = entry.unit;
    metric.future_hard_gate_candidate = entry.future_hard_gate_candidate;
    metric.geometry_metric = entry.geometry_metric;
    if (entry.geometry_metric) {
      ++out.geometry_metric_count;
    }
    if (entry.future_hard_gate_candidate) {
      ++out.future_hard_gate_candidate_count;
    }
    double sum = 0.0;
    for (const auto &fact : facts) {
      if (fact.target_component_family_id !=
          "wikimyei.representation.encoding.vicreg") {
        continue;
      }
      if (fact.representation_health_available) {
        observed_jobs.insert(fact.job_id);
      }
      const auto value =
          lattice_representation_geometry_metric_value(fact, entry.metric);
      if (!value.has_value() || !std::isfinite(*value)) {
        continue;
      }
      ++metric.observed_count;
      ++out.observed_metric_sample_count;
      sum += *value;
      metric.min_value = std::isfinite(metric.min_value)
                             ? std::min(metric.min_value, *value)
                             : *value;
      metric.max_value = std::isfinite(metric.max_value)
                             ? std::max(metric.max_value, *value)
                             : *value;
    }
    if (metric.observed_count > 0) {
      metric.mean_value = sum / static_cast<double>(metric.observed_count);
      metric.observed_distribution_available = true;
      if (metric.future_hard_gate_candidate) {
        ++out.observed_candidate_metric_count;
      }
    }
    out.metric_summaries.push_back(metric);
  }

  for (const auto &fact : facts) {
    if (fact.target_component_family_id !=
        "wikimyei.representation.encoding.vicreg") {
      continue;
    }
    if (!fact.representation_health_available) {
      ++out.missing_geometry_fact_count;
      continue;
    }
    bool has_geometry_metric = false;
    for (const auto &entry : vocabulary) {
      if (!entry.geometry_metric && !entry.future_hard_gate_candidate) {
        continue;
      }
      const auto value =
          lattice_representation_geometry_metric_value(fact, entry.metric);
      if (value.has_value() && std::isfinite(*value)) {
        has_geometry_metric = true;
        break;
      }
    }
    if (has_geometry_metric) {
      ++out.observed_geometry_fact_count;
    } else {
      ++out.missing_geometry_fact_count;
    }
  }
  out.observed_run_count = static_cast<std::int64_t>(observed_jobs.size());

  out.proposed_threshold_count = static_cast<std::int64_t>(std::count_if(
      out.metric_summaries.begin(), out.metric_summaries.end(),
      [](const auto &metric) { return metric.proposed_threshold; }));
  out.promoted_hard_gate_count = static_cast<std::int64_t>(std::count_if(
      out.metric_summaries.begin(), out.metric_summaries.end(),
      [](const auto &metric) { return metric.promoted_hard_gate; }));
  out.thresholds_justified_by_observed_data = out.proposed_threshold_count == 0;

  if (out.metric_count != 18) {
    out.summary_issues.push_back(
        "representation geometry gate review must inspect 18 warning metrics");
  }
  if (out.geometry_metric_count != 7) {
    out.summary_issues.push_back(
        "representation geometry gate review must include 7 geometry metrics");
  }
  if (out.future_hard_gate_candidate_count != 11) {
    out.summary_issues.push_back(
        "representation geometry gate review must track 11 future hard-gate "
        "candidates");
  }
  if (!out.opt_in_target_syntax_required) {
    out.summary_issues.push_back(
        "representation geometry gates must require opt-in target syntax");
  }
  if (!out.missing_geometry_fails_closed_for_gate) {
    out.summary_issues.push_back(
        "missing representation geometry must fail an explicit gate closed");
  }
  if (out.hard_gate_default_enabled) {
    out.summary_issues.push_back(
        "representation geometry gates must not be enabled by default");
  }
  if (out.proposed_threshold_count != 0 || out.promoted_hard_gate_count != 0) {
    out.summary_issues.push_back(
        "V3-H review must not promote or propose thresholds without stronger "
        "observed history");
  }
  if (!out.default_readiness_unchanged || !out.no_performance_gate_authority) {
    out.summary_issues.push_back(
        "representation geometry review must not change default readiness or "
        "performance authority");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::vector<lattice_performance_uncertainty_policy_entry_t>
lattice_performance_uncertainty_policy_vocabulary() {
  return {
      {"support_fraction_wilson_v1",
       "binomial_support_fraction",
       {"valid_target_fraction", "coverage_fraction",
        "node_valid_target_fraction"},
       "wilson_score_interval_95",
       "use lower confidence bound for high-good support fractions",
       {"valid_target_success_count_for_uncertainty",
        "valid_target_opportunity_count_for_uncertainty",
        "valid_target_wilson_lower_95", "valid_target_wilson_upper_95"},
       "future gates must compare the conservative confidence bound, not the "
       "raw fraction estimate",
       "available V1 visibility through valid_target_uncertainty_vocabulary; "
       "non-blocking unless a future target explicitly gates it",
       "hard performance/readiness gates need an explicit target clause and "
       "selection-leakage policy",
       true,
       false,
       false,
       true,
       true},
      {"proper_scoring_loss_future",
       "proper_scoring_rule",
       {"mean_nll", "crps"},
       "bootstrap_or_standard_error",
       "use upper confidence bound for high-bad loss metrics",
       {"metric_mean", "metric_sample_count", "metric_standard_error",
        "or bootstrap_samples"},
       "future gates must compare the high-bad upper confidence bound against "
       "the threshold",
       "V1 exposes mean_nll only as non-blocking warning visibility and does "
       "not gate losses",
       "runtime must emit enough samples, standard errors, or bootstrap "
       "summaries under exact checkpoint/split identity",
       false,
       false,
       false,
       true,
       true},
      {"calibration_fraction_future",
       "distribution_calibration_fraction",
       {"predictive_interval_coverage_error", "tail_coverage_error",
        "calibration_slope_error"},
       "wilson_or_bootstrap_or_standard_error",
       "use upper confidence bound for high-bad calibration error metrics",
       {"calibration_trial_count", "calibration_success_or_error_count",
        "metric_standard_error", "or bootstrap_samples"},
       "future gates must compare conservative error bounds and report sample "
       "support",
       "V1 names these metrics as future MDN calibration warnings only",
       "runtime must emit calibration samples and uncertainty summaries before "
       "hard gates can be trusted",
       false,
       false,
       false,
       true,
       true},
      {"pit_uniformity_future",
       "probability_integral_transform",
       {"pit_ks_statistic"},
       "asymptotic_ks_or_bootstrap",
       "use confidence-aware KS threshold or bootstrap upper bound",
       {"pit_sample_count", "pit_ks_statistic",
        "pit_ks_p_value_or_bootstrap_upper_bound"},
       "future gates must account for PIT sample count before treating "
       "non-uniformity as failure",
       "V1 names PIT KS as a future non-gating calibration metric",
       "runtime must emit PIT samples/counts and a declared uncertainty method",
       false,
       false,
       false,
       true,
       true},
      {"node_stratified_performance_future",
       "node_stratified_metric",
       {"per_node_nll", "per_node_crps", "per_node_calibration_error"},
       "stratified_bootstrap_or_per_node_confidence_bounds",
       "report aggregate and weakest-node conservative bounds separately",
       {"node_id", "node_metric_mean", "node_metric_sample_count",
        "node_metric_uncertainty"},
       "future gates must state whether they require aggregate, weakest-node, "
       "or all-node bounds",
       "V1 provides MDN node support visibility, not per-node performance "
       "acceptance",
       "runtime must emit per-node performance metrics with uncertainty before "
       "node-local performance gates",
       false,
       false,
       false,
       true,
       true},
      {"selection_safe_performance_gate_future",
       "gate_policy",
       {"channel_mdn_validation_performance_ready",
        "future best_nondominated_satisfying selectors"},
       "conservative_bound_with_declared_selection_policy",
       "performance gates require uncertainty and selection-signal leakage "
       "protection",
       {"clean evaluation proof", "metric uncertainty proof",
        "selection_signal provenance policy"},
       "no future performance gate may be satisfied from a raw point estimate "
       "or selector path that can see protected holdout evidence",
       "V1 is readiness-only and keeps performance selection out of scope",
       "add first-class performance target clauses and selection-event "
       "provenance before making deployment-quality claims",
       false,
       false,
       false,
       true,
       true}};
}

struct lattice_performance_uncertainty_policy_summary_t {
  std::string schema{
      "kikijyeba.lattice.performance_uncertainty_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t available_in_v1_count{0};
  std::int64_t deferred_future_count{0};
  std::int64_t performance_gate_authority_count{0};
  std::int64_t point_estimate_gate_allowed_count{0};
  std::int64_t confidence_bound_required_count{0};
  std::int64_t selection_leakage_policy_required_count{0};
  std::int64_t empty_field_count{0};
  bool support_fraction_wilson_v1_available{false};
  bool proper_scoring_loss_deferred{false};
  bool calibration_fraction_deferred{false};
  bool pit_uniformity_deferred{false};
  bool node_stratified_performance_deferred{false};
  bool selection_safe_gate_policy_deferred{false};
  bool all_policies_require_confidence_bounds{false};
  bool all_policies_require_selection_leakage_policy{false};
  bool no_point_estimate_gates{false};
  bool no_v1_performance_gate_authority{false};
  bool future_policies_deferred{false};
  bool all_policies_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> policy_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_performance_uncertainty_policy_summary_t
lattice_performance_uncertainty_policy_summary() {
  lattice_performance_uncertainty_policy_summary_t out{};
  const auto vocabulary = lattice_performance_uncertainty_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };

  for (const auto &entry : vocabulary) {
    out.policy_names.push_back(entry.policy);
    if (entry.available_in_v1) {
      ++out.available_in_v1_count;
    } else {
      ++out.deferred_future_count;
    }
    if (entry.performance_gate_authority) {
      ++out.performance_gate_authority_count;
    }
    if (entry.point_estimate_gate_allowed) {
      ++out.point_estimate_gate_allowed_count;
    }
    if (entry.confidence_bound_required) {
      ++out.confidence_bound_required_count;
    }
    if (entry.selection_leakage_policy_required) {
      ++out.selection_leakage_policy_required_count;
    }
    if (entry.policy.empty() || entry.statistic_family.empty() ||
        entry.applies_to.empty() || entry.estimator.empty() ||
        entry.confidence_policy.empty() ||
        entry.required_runtime_fields.empty() || entry.gate_rule.empty() ||
        entry.v1_scope.empty() || entry.future_requirement.empty()) {
      ++out.empty_field_count;
    }
  }

  const auto support_policy = find_policy("support_fraction_wilson_v1");
  const auto scoring_policy = find_policy("proper_scoring_loss_future");
  const auto calibration_policy = find_policy("calibration_fraction_future");
  const auto pit_policy = find_policy("pit_uniformity_future");
  const auto node_policy = find_policy("node_stratified_performance_future");
  const auto selection_policy =
      find_policy("selection_safe_performance_gate_future");

  out.support_fraction_wilson_v1_available =
      support_policy != vocabulary.end() && support_policy->available_in_v1 &&
      !support_policy->performance_gate_authority &&
      support_policy->estimator == "wilson_score_interval_95" &&
      support_policy->gate_rule.find("conservative confidence bound") !=
          std::string::npos;
  out.proper_scoring_loss_deferred =
      scoring_policy != vocabulary.end() && !scoring_policy->available_in_v1 &&
      scoring_policy->confidence_policy.find("upper confidence bound") !=
          std::string::npos;
  out.calibration_fraction_deferred =
      calibration_policy != vocabulary.end() &&
      !calibration_policy->available_in_v1 &&
      calibration_policy->estimator.find("wilson") != std::string::npos;
  out.pit_uniformity_deferred =
      pit_policy != vocabulary.end() && !pit_policy->available_in_v1 &&
      !pit_policy->required_runtime_fields.empty() &&
      pit_policy->required_runtime_fields.front() == "pit_sample_count";
  out.node_stratified_performance_deferred =
      node_policy != vocabulary.end() && !node_policy->available_in_v1 &&
      node_policy->statistic_family == "node_stratified_metric";
  out.selection_safe_gate_policy_deferred =
      selection_policy != vocabulary.end() &&
      !selection_policy->available_in_v1 &&
      selection_policy->future_requirement.find("selection-event") !=
          std::string::npos;
  out.all_policies_require_confidence_bounds =
      out.confidence_bound_required_count == out.policy_count &&
      out.policy_count > 0;
  out.all_policies_require_selection_leakage_policy =
      out.selection_leakage_policy_required_count == out.policy_count &&
      out.policy_count > 0;
  out.no_point_estimate_gates = out.point_estimate_gate_allowed_count == 0;
  out.no_v1_performance_gate_authority =
      out.performance_gate_authority_count == 0;
  out.future_policies_deferred =
      out.available_in_v1_count == 1 && out.deferred_future_count == 5;
  out.all_policies_have_claim_text = out.empty_field_count == 0;

  if (out.policy_count != 6) {
    out.summary_issues.push_back(
        "performance uncertainty policy vocabulary must contain 6 policies");
  }
  if (!out.support_fraction_wilson_v1_available) {
    out.summary_issues.push_back(
        "support_fraction_wilson_v1 must be V1 support visibility only");
  }
  if (!out.proper_scoring_loss_deferred) {
    out.summary_issues.push_back(
        "proper scoring loss performance gates must remain future work");
  }
  if (!out.calibration_fraction_deferred) {
    out.summary_issues.push_back(
        "calibration fraction performance gates must remain future work");
  }
  if (!out.pit_uniformity_deferred) {
    out.summary_issues.push_back(
        "PIT uniformity performance gates must remain future work");
  }
  if (!out.node_stratified_performance_deferred) {
    out.summary_issues.push_back(
        "node-stratified performance gates must remain future work");
  }
  if (!out.selection_safe_gate_policy_deferred) {
    out.summary_issues.push_back(
        "selection-safe performance gate policy must remain deferred");
  }
  if (!out.all_policies_require_confidence_bounds) {
    out.summary_issues.push_back(
        "all performance uncertainty policies must require confidence bounds");
  }
  if (!out.all_policies_require_selection_leakage_policy) {
    out.summary_issues.push_back(
        "all performance uncertainty policies must require selection-leakage "
        "policy");
  }
  if (!out.no_point_estimate_gates) {
    out.summary_issues.push_back(
        "raw point-estimate performance gates must not be allowed");
  }
  if (!out.no_v1_performance_gate_authority) {
    out.summary_issues.push_back(
        "performance uncertainty policy must not grant V1 gate authority");
  }
  if (!out.future_policies_deferred) {
    out.summary_issues.push_back(
        "only support_fraction_wilson_v1 may be available in V1");
  }
  if (!out.all_policies_have_claim_text) {
    out.summary_issues.push_back(
        "performance uncertainty policies must declare fields, gate rule, V1 "
        "scope, and future requirement");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::vector<lattice_mdn_distribution_calibration_entry_t>
lattice_mdn_distribution_calibration_vocabulary() {
  return {
      {"mean_nll",
       "proper_scoring_rule",
       {"lattice.exposure.fact.mean_nll"},
       "nll",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "none_point_estimate_only",
       true,
       false},
      {"crps",
       "proper_scoring_rule",
       {"future_mdn_distribution_report.crps"},
       "loss",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "bootstrap_or_standard_error_future",
       false,
       false},
      {"pit_ks_statistic",
       "probability_integral_transform",
       {"future_mdn_distribution_report.pit_ks_statistic"},
       "fraction",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "asymptotic_or_bootstrap_future",
       false,
       false},
      {"predictive_interval_coverage_error",
       "interval_calibration",
       {"future_mdn_distribution_report.predictive_interval_coverage_error"},
       "fraction",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "wilson_or_bootstrap_future",
       false,
       false},
      {"tail_coverage_error",
       "tail_calibration",
       {"future_mdn_distribution_report.tail_coverage_error"},
       "fraction",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "bootstrap_future",
       false,
       false},
      {"calibration_slope_error",
       "calibration_regression",
       {"future_mdn_distribution_report.calibration_slope_error"},
       "absolute_error",
       "above",
       "mdn_distribution_calibration",
       "non_blocking_warning_only",
       "standard_error_future",
       false,
       false}};
}

struct lattice_mdn_distribution_calibration_summary_t {
  std::string schema{
      "kikijyeba.lattice.mdn_distribution_calibration.summary.v1"};
  std::int64_t metric_count{0};
  std::int64_t available_in_v1_count{0};
  std::int64_t deferred_future_count{0};
  std::int64_t performance_gate_count{0};
  std::int64_t non_blocking_warning_count{0};
  std::int64_t above_threshold_direction_count{0};
  std::int64_t future_uncertainty_method_count{0};
  std::int64_t empty_field_count{0};
  bool mean_nll_available_in_v1{false};
  bool crps_deferred_future{false};
  bool pit_ks_deferred_future{false};
  bool interval_calibration_deferred_future{false};
  bool tail_calibration_deferred_future{false};
  bool calibration_slope_deferred_future{false};
  bool all_metrics_non_blocking_warnings{false};
  bool all_metrics_high_bad_above{false};
  bool no_performance_gates{false};
  bool future_metrics_require_uncertainty_methods{false};
  bool only_mean_nll_available_in_v1{false};
  bool all_metrics_have_runtime_fields{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> metric_names{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_mdn_distribution_calibration_summary_t
lattice_mdn_distribution_calibration_summary() {
  lattice_mdn_distribution_calibration_summary_t out{};
  const auto vocabulary = lattice_mdn_distribution_calibration_vocabulary();
  out.metric_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_metric = [&](const std::string &metric) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.metric == metric; });
  };

  const auto mean_nll = find_metric("mean_nll");
  const auto crps = find_metric("crps");
  const auto pit_ks = find_metric("pit_ks_statistic");
  const auto interval = find_metric("predictive_interval_coverage_error");
  const auto tail = find_metric("tail_coverage_error");
  const auto slope = find_metric("calibration_slope_error");

  for (const auto &entry : vocabulary) {
    out.metric_names.push_back(entry.metric);
    if (entry.available_in_v1) {
      ++out.available_in_v1_count;
    } else {
      ++out.deferred_future_count;
    }
    if (entry.performance_gate) {
      ++out.performance_gate_count;
    }
    if (entry.readiness_effect == "non_blocking_warning_only") {
      ++out.non_blocking_warning_count;
    }
    if (entry.threshold_direction == "above") {
      ++out.above_threshold_direction_count;
    }
    if (entry.uncertainty_method.find("future") != std::string::npos) {
      ++out.future_uncertainty_method_count;
    }
    if (entry.metric.empty() || entry.statistic_family.empty() ||
        entry.required_runtime_fields.empty() || entry.unit.empty() ||
        entry.threshold_direction.empty() || entry.warning_kind.empty() ||
        entry.readiness_effect.empty() || entry.uncertainty_method.empty()) {
      ++out.empty_field_count;
    }
  }

  out.mean_nll_available_in_v1 =
      mean_nll != vocabulary.end() && mean_nll->available_in_v1 &&
      mean_nll->required_runtime_fields.size() == 1 &&
      mean_nll->required_runtime_fields.front() ==
          "lattice.exposure.fact.mean_nll" &&
      mean_nll->unit == "nll" &&
      mean_nll->uncertainty_method == "none_point_estimate_only";
  out.crps_deferred_future =
      crps != vocabulary.end() && !crps->available_in_v1 &&
      crps->statistic_family == "proper_scoring_rule" &&
      crps->uncertainty_method.find("future") != std::string::npos;
  out.pit_ks_deferred_future =
      pit_ks != vocabulary.end() && !pit_ks->available_in_v1 &&
      pit_ks->statistic_family == "probability_integral_transform" &&
      pit_ks->unit == "fraction" &&
      pit_ks->uncertainty_method.find("future") != std::string::npos;
  out.interval_calibration_deferred_future =
      interval != vocabulary.end() && !interval->available_in_v1 &&
      interval->statistic_family == "interval_calibration" &&
      interval->unit == "fraction";
  out.tail_calibration_deferred_future =
      tail != vocabulary.end() && !tail->available_in_v1 &&
      tail->statistic_family == "tail_calibration" && tail->unit == "fraction";
  out.calibration_slope_deferred_future =
      slope != vocabulary.end() && !slope->available_in_v1 &&
      slope->statistic_family == "calibration_regression" &&
      slope->unit == "absolute_error";
  out.all_metrics_non_blocking_warnings =
      out.non_blocking_warning_count == out.metric_count &&
      out.metric_count > 0 &&
      std::all_of(vocabulary.begin(), vocabulary.end(), [](const auto &entry) {
        return entry.warning_kind == "mdn_distribution_calibration";
      });
  out.all_metrics_high_bad_above =
      out.above_threshold_direction_count == out.metric_count &&
      out.metric_count > 0;
  out.no_performance_gates = out.performance_gate_count == 0;
  out.future_metrics_require_uncertainty_methods =
      out.future_uncertainty_method_count == out.deferred_future_count &&
      out.deferred_future_count == 5;
  out.only_mean_nll_available_in_v1 = out.available_in_v1_count == 1 &&
                                      out.deferred_future_count == 5 &&
                                      out.mean_nll_available_in_v1;
  out.all_metrics_have_runtime_fields = out.empty_field_count == 0;

  if (out.metric_count != 6) {
    out.summary_issues.push_back(
        "MDN distribution calibration vocabulary must contain 6 metrics");
  }
  if (!out.mean_nll_available_in_v1) {
    out.summary_issues.push_back(
        "mean_nll must be the V1 available MDN calibration warning metric");
  }
  if (!out.crps_deferred_future) {
    out.summary_issues.push_back("CRPS must remain a future metric");
  }
  if (!out.pit_ks_deferred_future) {
    out.summary_issues.push_back(
        "PIT KS statistic must remain a future metric");
  }
  if (!out.interval_calibration_deferred_future) {
    out.summary_issues.push_back(
        "predictive interval coverage error must remain future work");
  }
  if (!out.tail_calibration_deferred_future) {
    out.summary_issues.push_back("tail coverage error must remain future work");
  }
  if (!out.calibration_slope_deferred_future) {
    out.summary_issues.push_back(
        "calibration slope error must remain future work");
  }
  if (!out.all_metrics_non_blocking_warnings) {
    out.summary_issues.push_back(
        "MDN distribution calibration metrics must remain non-blocking "
        "warnings");
  }
  if (!out.all_metrics_high_bad_above) {
    out.summary_issues.push_back(
        "MDN distribution calibration warnings must use above as high-bad");
  }
  if (!out.no_performance_gates) {
    out.summary_issues.push_back(
        "MDN distribution calibration must not grant performance gate "
        "authority");
  }
  if (!out.future_metrics_require_uncertainty_methods) {
    out.summary_issues.push_back(
        "future MDN calibration metrics must declare future uncertainty "
        "methods");
  }
  if (!out.only_mean_nll_available_in_v1) {
    out.summary_issues.push_back("only mean_nll may be available in V1");
  }
  if (!out.all_metrics_have_runtime_fields) {
    out.summary_issues.push_back(
        "MDN calibration metrics must declare runtime fields, units, "
        "directions, and uncertainty policy");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::vector<
    lattice_mdn_distribution_calibration_diagnostic_entry_t>
lattice_mdn_distribution_calibration_diagnostic_vocabulary() {
  return {{"mean_nll",
           "proper_scoring_loss",
           "mdn_distribution_facts",
           {"lattice.exposure.fact.mean_nll"},
           "valid_target_count",
           "mean_nll",
           "none_point_estimate_only",
           {},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration METRIC=mean_nll",
           "available",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"mean_nll_per_channel_max",
           "channel_stratified_scoring_loss",
           "mdn_distribution_facts",
           {"lattice.exposure.fact.mean_nll_per_channel"},
           "valid_target_count",
           "max(mean_nll_per_channel)",
           "none_point_estimate_only",
           {},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=mean_nll_per_channel_max",
           "available_when_report_field_present",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"mean_nll_per_target_feature_max",
           "target_feature_stratified_scoring_loss",
           "mdn_distribution_facts",
           {"lattice.exposure.fact.mean_nll_per_target_feature"},
           "valid_target_count",
           "max(mean_nll_per_target_feature)",
           "none_point_estimate_only",
           {},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=mean_nll_per_target_feature_max",
           "available_when_report_field_present",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"mean_nll_per_channel_target_feature_max",
           "channel_target_feature_stratified_scoring_loss",
           "mdn_distribution_facts",
           {"lattice.exposure.fact.mean_nll_per_channel_target_feature"},
           "valid_target_count_per_channel_target_feature",
           "max(mean_nll_per_channel_target_feature)",
           "none_point_estimate_only",
           {},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=mean_nll_per_channel_target_feature_max",
           "available_when_report_field_present",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"per_node_mean_nll_max",
           "node_stratified_scoring_loss",
           "filtered_node_exposure_facts",
           {"lattice.node_exposure.fact.mean_nll"},
           "sum(valid_target_count over finite-node-NLL rows)",
           "max(node.mean_nll)",
           "none_point_estimate_only",
           {},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=per_node_mean_nll_max",
           "available_when_node_report_field_present",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"pit_ks_statistic",
           "probability_integral_transform",
           "future_mdn_distribution_report",
           {"future_mdn_distribution_report.pit_histogram",
            "future_mdn_distribution_report.pit_ks_statistic"},
           "pit_sample_count",
           "pit_ks_statistic",
           "asymptotic_or_bootstrap_future",
           {"pit_ks_upper_95"},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=pit_ks_statistic",
           "unavailable_until_runtime_emits_pit",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"predictive_interval_coverage_error",
           "interval_calibration",
           "future_mdn_distribution_report",
           {"future_mdn_distribution_report.predictive_interval_coverage"},
           "interval_trial_count",
           "predictive_interval_coverage_error",
           "wilson_or_bootstrap_future",
           {"interval_coverage_error_upper_95"},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=predictive_interval_coverage_error",
           "unavailable_until_runtime_emits_interval_coverage",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"tail_coverage_error",
           "tail_calibration",
           "future_mdn_distribution_report",
           {"future_mdn_distribution_report.tail_coverage"},
           "tail_trial_count",
           "tail_coverage_error",
           "bootstrap_future",
           {"tail_coverage_error_upper_95"},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=tail_coverage_error",
           "unavailable_until_runtime_emits_tail_coverage",
           true,
           true,
           true,
           true,
           true,
           true,
           false},
          {"calibration_slope_error",
           "calibration_regression",
           "future_mdn_distribution_report",
           {"future_mdn_distribution_report.calibration_slope_error"},
           "calibration_regression_sample_count",
           "calibration_slope_error",
           "standard_error_future",
           {"calibration_slope_error_upper_95"},
           "warning_result.diagnostic_evaluated_mdn_checkpoint",
           "warning_result.split|warning_anchor_range",
           "warning_result.diagnostic_active_*",
           "LATTICE_WARN KIND=mdn_distribution_calibration "
           "METRIC=calibration_slope_error",
           "unavailable_until_runtime_emits_calibration_regression",
           true,
           true,
           true,
           true,
           true,
           true,
           false}};
}

struct lattice_mdn_distribution_calibration_diagnostic_summary_t {
  std::string schema{
      "kikijyeba.lattice.mdn_distribution_calibration_diagnostic.summary.v1"};
  std::int64_t diagnostic_count{0};
  std::int64_t available_metric_count{0};
  std::int64_t future_metric_count{0};
  std::int64_t warning_only_count{0};
  std::int64_t performance_gate_count{0};
  std::int64_t validation_binding_required_count{0};
  std::int64_t exact_mdn_checkpoint_required_count{0};
  std::int64_t representation_checkpoint_required_count{0};
  std::int64_t split_policy_identity_required_count{0};
  std::int64_t active_identity_required_count{0};
  std::int64_t point_estimate_only_count{0};
  std::int64_t future_uncertainty_count{0};
  std::int64_t empty_field_count{0};
  bool finite_first_set_declared{false};
  bool proper_scoring_metrics_present{false};
  bool pit_histogram_summary_deferred{false};
  bool interval_and_tail_coverage_deferred{false};
  bool per_node_calibration_present{false};
  bool all_diagnostics_bind_identity_and_checkpoints{false};
  bool all_diagnostics_warning_only{false};
  bool no_hard_calibration_gates{false};
  bool available_point_estimates_are_warning_only{false};
  bool future_metrics_have_uncertainty_methods{false};
  bool all_diagnostics_have_runtime_fields{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> diagnostic_metrics{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_mdn_distribution_calibration_diagnostic_summary_t
lattice_mdn_distribution_calibration_diagnostic_summary() {
  lattice_mdn_distribution_calibration_diagnostic_summary_t out{};
  const auto vocabulary =
      lattice_mdn_distribution_calibration_diagnostic_vocabulary();
  out.diagnostic_count = static_cast<std::int64_t>(vocabulary.size());

  const auto find_metric = [&](const std::string &metric) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.metric == metric; });
  };

  const auto mean_nll = find_metric("mean_nll");
  const auto channel_nll = find_metric("mean_nll_per_channel_max");
  const auto feature_nll = find_metric("mean_nll_per_target_feature_max");
  const auto channel_feature_nll =
      find_metric("mean_nll_per_channel_target_feature_max");
  const auto per_node = find_metric("per_node_mean_nll_max");
  const auto pit = find_metric("pit_ks_statistic");
  const auto interval = find_metric("predictive_interval_coverage_error");
  const auto tail = find_metric("tail_coverage_error");

  for (const auto &entry : vocabulary) {
    out.diagnostic_metrics.push_back(entry.metric);
    if (entry.availability.find("available") == 0) {
      ++out.available_metric_count;
    } else {
      ++out.future_metric_count;
    }
    if (entry.warning_only) {
      ++out.warning_only_count;
    }
    if (entry.performance_gate) {
      ++out.performance_gate_count;
    }
    if (entry.validation_run_binding_required) {
      ++out.validation_binding_required_count;
    }
    if (entry.exact_mdn_checkpoint_required) {
      ++out.exact_mdn_checkpoint_required_count;
    }
    if (entry.representation_checkpoint_required) {
      ++out.representation_checkpoint_required_count;
    }
    if (entry.split_policy_identity_required) {
      ++out.split_policy_identity_required_count;
    }
    if (entry.active_identity_required) {
      ++out.active_identity_required_count;
    }
    if (entry.uncertainty_method == "none_point_estimate_only") {
      ++out.point_estimate_only_count;
    }
    if (entry.uncertainty_method.find("future") != std::string::npos) {
      ++out.future_uncertainty_count;
    }
    if (entry.metric.empty() || entry.diagnostic_family.empty() ||
        entry.evidence_basis.empty() || entry.runtime_fields.empty() ||
        entry.sample_count_field.empty() ||
        entry.point_estimate_field.empty() ||
        entry.uncertainty_method.empty() || entry.checkpoint_binding.empty() ||
        entry.split_binding.empty() || entry.active_identity_binding.empty() ||
        entry.warning_metric.empty() || entry.availability.empty()) {
      ++out.empty_field_count;
    }
  }

  out.finite_first_set_declared =
      out.diagnostic_count == 9 && mean_nll != vocabulary.end() &&
      channel_nll != vocabulary.end() && feature_nll != vocabulary.end() &&
      channel_feature_nll != vocabulary.end() && per_node != vocabulary.end() &&
      pit != vocabulary.end() && interval != vocabulary.end() &&
      tail != vocabulary.end();
  out.proper_scoring_metrics_present =
      mean_nll != vocabulary.end() &&
      mean_nll->diagnostic_family == "proper_scoring_loss" &&
      channel_nll != vocabulary.end() && feature_nll != vocabulary.end() &&
      channel_feature_nll != vocabulary.end();
  out.pit_histogram_summary_deferred =
      pit != vocabulary.end() &&
      pit->runtime_fields.front().find("pit_histogram") != std::string::npos &&
      pit->availability.find("unavailable") != std::string::npos;
  out.interval_and_tail_coverage_deferred =
      interval != vocabulary.end() && tail != vocabulary.end() &&
      interval->availability.find("unavailable") != std::string::npos &&
      tail->availability.find("unavailable") != std::string::npos;
  out.per_node_calibration_present =
      per_node != vocabulary.end() &&
      per_node->evidence_basis == "filtered_node_exposure_facts";
  out.all_diagnostics_bind_identity_and_checkpoints =
      out.validation_binding_required_count == out.diagnostic_count &&
      out.exact_mdn_checkpoint_required_count == out.diagnostic_count &&
      out.representation_checkpoint_required_count == out.diagnostic_count &&
      out.split_policy_identity_required_count == out.diagnostic_count &&
      out.active_identity_required_count == out.diagnostic_count &&
      out.diagnostic_count > 0;
  out.all_diagnostics_warning_only =
      out.warning_only_count == out.diagnostic_count &&
      out.diagnostic_count > 0;
  out.no_hard_calibration_gates = out.performance_gate_count == 0;
  out.available_point_estimates_are_warning_only =
      out.point_estimate_only_count == out.available_metric_count &&
      out.available_metric_count == 5 && out.no_hard_calibration_gates;
  out.future_metrics_have_uncertainty_methods =
      out.future_uncertainty_count == out.future_metric_count &&
      out.future_metric_count == 4;
  out.all_diagnostics_have_runtime_fields = out.empty_field_count == 0;

  if (!out.finite_first_set_declared) {
    out.summary_issues.push_back(
        "MDN calibration diagnostic vocabulary must declare the finite V3-D "
        "first set");
  }
  if (!out.proper_scoring_metrics_present) {
    out.summary_issues.push_back(
        "MDN calibration diagnostics must include NLL scoring metrics");
  }
  if (!out.pit_histogram_summary_deferred) {
    out.summary_issues.push_back(
        "PIT histogram/KS diagnostics must be declared as future runtime "
        "evidence");
  }
  if (!out.interval_and_tail_coverage_deferred) {
    out.summary_issues.push_back(
        "predictive interval and tail coverage diagnostics must be declared");
  }
  if (!out.per_node_calibration_present) {
    out.summary_issues.push_back(
        "per-node MDN calibration diagnostics must be present");
  }
  if (!out.all_diagnostics_bind_identity_and_checkpoints) {
    out.summary_issues.push_back(
        "all MDN calibration diagnostics must bind active identity, split "
        "policy, exact MDN checkpoint, and representation checkpoint");
  }
  if (!out.all_diagnostics_warning_only) {
    out.summary_issues.push_back(
        "MDN calibration diagnostics must remain warning-only in V3-D");
  }
  if (!out.no_hard_calibration_gates) {
    out.summary_issues.push_back(
        "V3-D must not grant hard calibration gate authority");
  }
  if (!out.available_point_estimates_are_warning_only) {
    out.summary_issues.push_back(
        "available point-estimate diagnostics must remain warning-only");
  }
  if (!out.future_metrics_have_uncertainty_methods) {
    out.summary_issues.push_back(
        "future calibration diagnostics must declare uncertainty methods");
  }
  if (!out.all_diagnostics_have_runtime_fields) {
    out.summary_issues.push_back(
        "all MDN calibration diagnostics must name runtime fields and binding "
        "surfaces");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_target_numeric_dimension_entry_t {
  std::string context{};
  std::string field{};
  std::string unit{};
  std::string numeric_kind{};
  bool has_minimum{false};
  double minimum{0.0};
  bool has_maximum{false};
  double maximum{0.0};
  bool integral{false};
  std::string threshold_direction{};
  std::string validation_rule{};
};

[[nodiscard]] inline std::vector<lattice_target_numeric_dimension_entry_t>
lattice_target_numeric_dimension_vocabulary() {
  return {
      {"LATTICE_TARGET", "MIN_OPTIMIZER_STEPS", "count", "non_negative_integer",
       true, 0.0, false, 0.0, true, "minimum",
       "optimizer effort thresholds are non-negative integral counts"},
      {"LATTICE_TARGET", "MIN_VALID_TARGET_FRACTION", "fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "minimum",
       "valid-target readiness thresholds are fractions in [0,1]"},
      {"LATTICE_TARGET", "MIN_OBSERVED_INPUT_COVERAGE", "coverage_fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "minimum",
       "observed-input readiness coverage is an idempotent cursor-coverage "
       "fraction"},
      {"LATTICE_TARGET", "MIN_TARGET_SUPERVISION_COVERAGE", "coverage_fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "minimum",
       "target-supervision readiness coverage is an idempotent cursor-coverage "
       "fraction"},
      {"LATTICE_TARGET", "MIN_EVALUATION_METRIC_COVERAGE", "coverage_fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "minimum",
       "evaluation-metric coverage is a non-mutating cursor-coverage fraction"},
      {"LATTICE_TARGET", "MAX_WAVES", "count", "non_negative_integer", true,
       0.0, false, 0.0, true, "planning_budget",
       "planning budgets are non-negative integral counts and do not execute"},
      {"LATTICE_TARGET", "REQUIRE_*_NODE_HEAD_COUNT", "count",
       "non_negative_integer", true, 0.0, false, 0.0, true, "minimum",
       "active, trained, and evaluated node-head requirements are "
       "non-negative integral counts"},
      {"LATTICE_REQUIRES.effort", "VALUE", "count", "non_negative_integer",
       true, 0.0, false, 0.0, true, "minimum",
       "optimizer_steps requirements are non-negative integral counts"},
      {"LATTICE_REQUIRES.valid_target_fraction", "VALUE", "fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "minimum",
       "valid-target fraction requirements are fractions in [0,1]"},
      {"LATTICE_REQUIRES.node_head_count", "VALUE", "count",
       "non_negative_integer", true, 0.0, false, 0.0, true, "minimum",
       "node head count requirements are non-negative integral counts"},
      {"LATTICE_REQUIRES.exposure_coverage", "CURSOR_EPOCHS",
       "coverage_fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "minimum",
       "v0 exposure coverage requirements are cursor-coverage fractions in "
       "[0,1]"},
      {"LATTICE_REQUIRES.representation_geometry.fraction_metrics", "VALUE",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "opt-in representation geometry fraction gates are bounded in [0,1]"},
      {"LATTICE_REQUIRES.representation_geometry.count_metrics", "VALUE",
       "count", "non_negative_integer", true, 0.0, false, 0.0, true,
       "metric_declared",
       "opt-in representation geometry count gates are non-negative integral "
       "counts"},
      {"LATTICE_REQUIRES.representation_geometry.representation_condition_"
       "number",
       "VALUE", "condition_number", "real_at_least_one", true, 1.0, false, 0.0,
       false, "metric_declared",
       "opt-in representation condition-number gates use finite values >= 1"},
      {"LATTICE_REQUIRES.representation_geometry.non_negative_metrics", "VALUE",
       "metric_specific", "non_negative_real", true, 0.0, false, 0.0, false,
       "metric_declared",
       "opt-in representation geometry loss, rank, and variance gates use "
       "non-negative metric-specific values"},
      {"LATTICE_WARN.exposure_load", "CURSOR_EPOCHS_ABOVE", "cursor_epoch",
       "non_negative_real", true, 0.0, false, 0.0, false, "above",
       "repeated exposure-load warnings compare additive cursor epochs"},
      {"LATTICE_WARN.effort_density", "ABOVE", "metric_specific_rate",
       "non_negative_real", true, 0.0, false, 0.0, false, "above",
       "effort-density warnings compare non-negative metric-specific rates"},
      {"LATTICE_WARN.anchor_domain_health", "ACCEPTED_FRACTION_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false, "below",
       "accepted anchor-domain fractions are bounded in [0,1]"},
      {"LATTICE_WARN.anchor_domain_health", "SKIPPED_FAILED_FETCH_PROBE_ABOVE",
       "count", "non_negative_integer", true, 0.0, false, 0.0, true, "above",
       "failed-fetch probe thresholds are non-negative counts"},
      {"LATTICE_WARN.forecast_baseline.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "forecast baseline directional accuracy thresholds are fractions in "
       "[0,1]"},
      {"LATTICE_WARN.forecast_baseline.count_metrics", "ABOVE_OR_BELOW",
       "count", "non_negative_integer", true, 0.0, false, 0.0, true,
       "metric_declared",
       "forecast baseline support thresholds are non-negative integral counts"},
      {"LATTICE_WARN.forecast_baseline.non_negative_metrics", "ABOVE_OR_BELOW",
       "metric_specific", "non_negative_real", true, 0.0, false, 0.0, false,
       "metric_declared",
       "forecast baseline loss and unsigned error thresholds are non-negative "
       "metric-specific values"},
      {"LATTICE_WARN.forecast_baseline.baseline_signed_error", "ABOVE_OR_BELOW",
       "expected_value_error", "finite_real", false, 0.0, false, 0.0, false,
       "metric_declared",
       "forecast baseline signed-error thresholds can be positive or negative "
       "finite values"},
      {"LATTICE_WARN.forecast_eval.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "forecast evaluation directional accuracy and calibration coverage "
       "thresholds are fractions in [0,1]"},
      {"LATTICE_WARN.forecast_eval.count_metrics", "ABOVE_OR_BELOW", "count",
       "non_negative_integer", true, 0.0, false, 0.0, true, "metric_declared",
       "forecast evaluation support thresholds are non-negative integral "
       "counts"},
      {"LATTICE_WARN.forecast_eval.non_negative_metrics", "ABOVE_OR_BELOW",
       "metric_specific", "non_negative_real", true, 0.0, false, 0.0, false,
       "metric_declared",
       "forecast evaluation loss and unsigned error thresholds are "
       "non-negative metric-specific values"},
      {"LATTICE_WARN.forecast_eval.signed_error", "ABOVE_OR_BELOW",
       "expected_value_error", "finite_real", false, 0.0, false, 0.0, false,
       "metric_declared",
       "forecast evaluation signed-error thresholds can be positive or "
       "negative "
       "finite values"},
      {"LATTICE_WARN.observer_belief.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "observer confidence, data quality, and liquidity thresholds are "
       "fractions in [0,1]"},
      {"LATTICE_WARN.observer_belief.boolean_flags", "ABOVE_OR_BELOW",
       "boolean", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "observer lineage, diagnostic completeness, and authority flags are "
       "warning-only 0/1 diagnostics"},
      {"LATTICE_WARN.allocation_engine.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "allocation reserve weight, turnover, and cost thresholds are fractions "
       "in [0,1]"},
      {"LATTICE_WARN.allocation_engine.non_negative_metrics", "ABOVE_OR_BELOW",
       "metric_specific", "non_negative_real", true, 0.0, false, 0.0, false,
       "metric_declared",
       "allocation CVaR/loss diagnostics are warning-only non-negative values"},
      {"LATTICE_WARN.allocation_engine.boolean_flags", "ABOVE_OR_BELOW",
       "boolean", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "allocation lineage, cap/growth-floor/fallback/de-risk, contract, and "
       "authority flags are warning-only 0/1 diagnostics"},
      {"LATTICE_WARN.node_support_balance.valid_target_count_gini", "ABOVE",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false, "above",
       "node support Gini imbalance is a fraction in [0,1]"},
      {"LATTICE_WARN.node_support_balance.valid_target_count_normalized_"
       "entropy",
       "BELOW", "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "below", "normalized node support entropy is a fraction in [0,1]"},
      {"LATTICE_WARN.node_support_floor.fraction_metrics", "BELOW", "fraction",
       "closed_unit_interval", true, 0.0, true, 1.0, false, "below",
       "min_valid_target_fraction and valid_target_wilson_lower_95 are "
       "fractions in [0,1]"},
      {"LATTICE_WARN.node_support_floor.count_metrics", "BELOW", "count",
       "non_negative_integer", true, 0.0, false, 0.0, true, "below",
       "node support count floors are non-negative integral counts"},
      {"LATTICE_WARN.representation_health.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "adapter support, finite-parameter check, effective-rank fraction, and "
       "isotropy thresholds are fractions in [0,1]"},
      {"LATTICE_WARN.representation_health.count_metrics", "ABOVE_OR_BELOW",
       "count", "non_negative_integer", true, 0.0, false, 0.0, true,
       "metric_declared",
       "valid projection rows and embedding dimension thresholds are "
       "non-negative integral counts"},
      {"LATTICE_WARN.representation_health.representation_condition_number",
       "ABOVE", "condition_number", "real_at_least_one", true, 1.0, false, 0.0,
       false, "above",
       "representation condition-number thresholds are real values >= 1 and "
       "use ABOVE"},
      {"LATTICE_WARN.representation_health.non_negative_metrics",
       "ABOVE_OR_BELOW", "metric_specific", "non_negative_real", true, 0.0,
       false, 0.0, false, "metric_declared",
       "loss, gradient, effective-rank, and variance thresholds are "
       "non-negative metric-specific values"},
      {"LATTICE_WARN.mdn_distribution_calibration.mean_nll", "ABOVE", "nll",
       "non_negative_real", true, 0.0, false, 0.0, false, "above",
       "MDN mean NLL warnings compare high-bad non-negative NLL values"},
      {"LATTICE_WARN.mdn_distribution_calibration.fraction_metrics", "ABOVE",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false, "above",
       "PIT, interval-coverage, and tail-coverage calibration warnings compare "
       "high-bad fractions in [0,1]"},
      {"LATTICE_WARN.mdn_distribution_calibration.non_negative_metrics",
       "ABOVE", "metric_specific", "non_negative_real", true, 0.0, false, 0.0,
       false, "above",
       "CRPS and calibration-slope-error warnings compare high-bad "
       "non-negative metric values"},
      {"LATTICE_WARN.runtime_health.boolean_metrics", "ABOVE_OR_BELOW",
       "boolean", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "runtime health boolean warnings compare 0/1 facts from Runtime "
       "terminal facts or exposure sidecars"},
      {"LATTICE_WARN.runtime_health.count_metrics", "ABOVE", "count",
       "non_negative_integer", true, 0.0, false, 0.0, true, "above",
       "runtime health count warnings compare non-negative counts"},
      {"LATTICE_WARN.runtime_health.fraction_metrics", "ABOVE_OR_BELOW",
       "fraction", "closed_unit_interval", true, 0.0, true, 1.0, false,
       "metric_declared",
       "runtime health fraction warnings compare fractions in [0,1]"},
      {"LATTICE_WARN.runtime_health.non_negative_metrics", "ABOVE_OR_BELOW",
       "metric_specific", "non_negative_real", true, 0.0, false, 0.0, false,
       "metric_declared",
       "runtime health gradient, sigma, and entropy warnings compare "
       "non-negative measurements"}};
}

struct lattice_target_numeric_dimension_summary_t {
  std::string schema{"kikijyeba.lattice.target_numeric_dimension.summary.v1"};
  std::int64_t dimension_count{0};
  std::int64_t unit_count{0};
  std::int64_t numeric_kind_count{0};
  std::int64_t closed_unit_interval_count{0};
  std::int64_t non_negative_integer_count{0};
  std::int64_t non_negative_real_count{0};
  std::int64_t real_at_least_one_count{0};
  std::int64_t integral_count{0};
  std::int64_t bounded_unit_interval_count{0};
  std::int64_t minimum_direction_count{0};
  std::int64_t above_direction_count{0};
  std::int64_t below_direction_count{0};
  std::int64_t metric_declared_direction_count{0};
  std::int64_t planning_budget_direction_count{0};
  std::int64_t empty_field_count{0};
  std::int64_t malformed_bound_count{0};
  std::int64_t duplicate_context_field_count{0};
  bool all_dimensions_have_units{false};
  bool all_dimensions_have_numeric_kind{false};
  bool all_context_fields_unique{false};
  bool bounds_are_well_formed{false};
  bool unit_interval_rows_are_bounded{false};
  bool integral_rows_are_count_like{false};
  bool threshold_directions_are_known{false};
  bool coverage_cursor_epochs_dimension_present{false};
  bool repeated_load_cursor_epoch_dimension_present{false};
  bool warning_anchor_fraction_dimension_present{false};
  bool representation_condition_number_dimension_present{false};
  bool node_support_floor_count_dimension_present{false};
  bool mdn_mean_nll_dimension_present{false};
  bool coverage_and_load_units_separated{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> units{};
  std::vector<std::string> numeric_kinds{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_target_numeric_dimension_summary_t
lattice_target_numeric_dimension_summary() {
  lattice_target_numeric_dimension_summary_t out{};
  const auto vocabulary = lattice_target_numeric_dimension_vocabulary();
  out.dimension_count = static_cast<std::int64_t>(vocabulary.size());

  const auto approx_equal = [](const double lhs, const double rhs) {
    return std::abs(lhs - rhs) < 1e-12;
  };
  const auto find_dimension = [&](const std::string &context,
                                  const std::string &field) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(), [&](const auto &entry) {
          return entry.context == context && entry.field == field;
        });
  };
  const auto has_unit_interval_bounds =
      [&](const lattice_target_numeric_dimension_entry_t &entry) {
        return entry.has_minimum && approx_equal(entry.minimum, 0.0) &&
               entry.has_maximum && approx_equal(entry.maximum, 1.0);
      };

  std::set<std::string> units{};
  std::set<std::string> numeric_kinds{};
  std::set<std::string> context_fields{};
  std::int64_t known_threshold_direction_count{0};
  std::int64_t rows_with_units{0};
  std::int64_t rows_with_numeric_kind{0};
  std::int64_t integral_count_like_count{0};

  for (const auto &entry : vocabulary) {
    if (!entry.unit.empty()) {
      units.insert(entry.unit);
      ++rows_with_units;
    }
    if (!entry.numeric_kind.empty()) {
      numeric_kinds.insert(entry.numeric_kind);
      ++rows_with_numeric_kind;
    }

    const auto [unused_it, inserted] =
        context_fields.insert(entry.context + "." + entry.field);
    (void)unused_it;
    if (!inserted) {
      ++out.duplicate_context_field_count;
    }

    if (entry.context.empty() || entry.field.empty() || entry.unit.empty() ||
        entry.numeric_kind.empty() || entry.threshold_direction.empty() ||
        entry.validation_rule.empty()) {
      ++out.empty_field_count;
    }

    if (entry.numeric_kind == "closed_unit_interval") {
      ++out.closed_unit_interval_count;
      if (has_unit_interval_bounds(entry)) {
        ++out.bounded_unit_interval_count;
      }
    } else if (entry.numeric_kind == "non_negative_integer") {
      ++out.non_negative_integer_count;
    } else if (entry.numeric_kind == "non_negative_real") {
      ++out.non_negative_real_count;
    } else if (entry.numeric_kind == "real_at_least_one") {
      ++out.real_at_least_one_count;
    }

    if (entry.integral) {
      ++out.integral_count;
      if (entry.unit == "count" &&
          entry.numeric_kind == "non_negative_integer") {
        ++integral_count_like_count;
      }
    }

    if (entry.threshold_direction == "minimum") {
      ++out.minimum_direction_count;
      ++known_threshold_direction_count;
    } else if (entry.threshold_direction == "above") {
      ++out.above_direction_count;
      ++known_threshold_direction_count;
    } else if (entry.threshold_direction == "below") {
      ++out.below_direction_count;
      ++known_threshold_direction_count;
    } else if (entry.threshold_direction == "metric_declared") {
      ++out.metric_declared_direction_count;
      ++known_threshold_direction_count;
    } else if (entry.threshold_direction == "planning_budget") {
      ++out.planning_budget_direction_count;
      ++known_threshold_direction_count;
    }

    bool malformed = false;
    if (entry.has_minimum && entry.has_maximum &&
        entry.maximum < entry.minimum) {
      malformed = true;
    }
    if (entry.numeric_kind == "closed_unit_interval" &&
        !has_unit_interval_bounds(entry)) {
      malformed = true;
    }
    if ((entry.numeric_kind == "non_negative_integer" ||
         entry.numeric_kind == "non_negative_real") &&
        (!entry.has_minimum || entry.minimum < 0.0)) {
      malformed = true;
    }
    if (entry.numeric_kind == "real_at_least_one" &&
        (!entry.has_minimum || entry.minimum < 1.0)) {
      malformed = true;
    }
    if (entry.integral && (entry.unit != "count" ||
                           entry.numeric_kind != "non_negative_integer")) {
      malformed = true;
    }
    if (malformed) {
      ++out.malformed_bound_count;
    }
  }

  out.units.assign(units.begin(), units.end());
  out.numeric_kinds.assign(numeric_kinds.begin(), numeric_kinds.end());
  out.unit_count = static_cast<std::int64_t>(out.units.size());
  out.numeric_kind_count = static_cast<std::int64_t>(out.numeric_kinds.size());

  const auto coverage =
      find_dimension("LATTICE_REQUIRES.exposure_coverage", "CURSOR_EPOCHS");
  const auto repeated_load =
      find_dimension("LATTICE_WARN.exposure_load", "CURSOR_EPOCHS_ABOVE");
  const auto anchor_fraction = find_dimension(
      "LATTICE_WARN.anchor_domain_health", "ACCEPTED_FRACTION_BELOW");
  const auto condition = find_dimension(
      "LATTICE_WARN.representation_health.representation_condition_number",
      "ABOVE");
  const auto node_floor_count =
      find_dimension("LATTICE_WARN.node_support_floor.count_metrics", "BELOW");
  const auto mdn_nll = find_dimension(
      "LATTICE_WARN.mdn_distribution_calibration.mean_nll", "ABOVE");

  out.coverage_cursor_epochs_dimension_present =
      coverage != vocabulary.end() && coverage->unit == "coverage_fraction" &&
      coverage->numeric_kind == "closed_unit_interval" &&
      coverage->threshold_direction == "minimum" &&
      has_unit_interval_bounds(*coverage) && !coverage->integral;
  out.repeated_load_cursor_epoch_dimension_present =
      repeated_load != vocabulary.end() &&
      repeated_load->unit == "cursor_epoch" &&
      repeated_load->numeric_kind == "non_negative_real" &&
      repeated_load->threshold_direction == "above" &&
      repeated_load->has_minimum && approx_equal(repeated_load->minimum, 0.0) &&
      !repeated_load->integral;
  out.warning_anchor_fraction_dimension_present =
      anchor_fraction != vocabulary.end() &&
      anchor_fraction->unit == "fraction" &&
      anchor_fraction->numeric_kind == "closed_unit_interval" &&
      anchor_fraction->threshold_direction == "below" &&
      has_unit_interval_bounds(*anchor_fraction);
  out.representation_condition_number_dimension_present =
      condition != vocabulary.end() && condition->unit == "condition_number" &&
      condition->numeric_kind == "real_at_least_one" &&
      condition->threshold_direction == "above" && condition->has_minimum &&
      approx_equal(condition->minimum, 1.0) && !condition->integral;
  out.node_support_floor_count_dimension_present =
      node_floor_count != vocabulary.end() &&
      node_floor_count->unit == "count" &&
      node_floor_count->numeric_kind == "non_negative_integer" &&
      node_floor_count->threshold_direction == "below" &&
      node_floor_count->has_minimum &&
      approx_equal(node_floor_count->minimum, 0.0) &&
      node_floor_count->integral;
  out.mdn_mean_nll_dimension_present =
      mdn_nll != vocabulary.end() && mdn_nll->unit == "nll" &&
      mdn_nll->numeric_kind == "non_negative_real" &&
      mdn_nll->threshold_direction == "above" && mdn_nll->has_minimum &&
      approx_equal(mdn_nll->minimum, 0.0) && !mdn_nll->integral;

  out.all_dimensions_have_units =
      rows_with_units == out.dimension_count && out.dimension_count > 0;
  out.all_dimensions_have_numeric_kind =
      rows_with_numeric_kind == out.dimension_count && out.dimension_count > 0;
  out.all_context_fields_unique = out.duplicate_context_field_count == 0;
  out.bounds_are_well_formed = out.malformed_bound_count == 0;
  out.unit_interval_rows_are_bounded =
      out.closed_unit_interval_count > 0 &&
      out.bounded_unit_interval_count == out.closed_unit_interval_count;
  out.integral_rows_are_count_like =
      out.integral_count > 0 && integral_count_like_count == out.integral_count;
  out.threshold_directions_are_known =
      known_threshold_direction_count == out.dimension_count &&
      out.dimension_count > 0;
  out.coverage_and_load_units_separated =
      out.coverage_cursor_epochs_dimension_present &&
      out.repeated_load_cursor_epoch_dimension_present &&
      coverage->unit != repeated_load->unit &&
      coverage->numeric_kind != repeated_load->numeric_kind;

  if (out.dimension_count != 37) {
    out.summary_issues.push_back(
        "target numeric dimension vocabulary must contain 37 V1 rows");
  }
  if (out.unit_count != 9) {
    out.summary_issues.push_back(
        "target numeric dimension vocabulary must expose 9 unit families");
  }
  if (out.numeric_kind_count != 4) {
    out.summary_issues.push_back(
        "target numeric dimension vocabulary must expose 4 numeric kinds");
  }
  if (!out.all_dimensions_have_units) {
    out.summary_issues.push_back("every numeric dimension must declare a unit");
  }
  if (!out.all_dimensions_have_numeric_kind) {
    out.summary_issues.push_back(
        "every numeric dimension must declare a numeric kind");
  }
  if (!out.all_context_fields_unique) {
    out.summary_issues.push_back(
        "numeric dimension context/field pairs must be unique");
  }
  if (!out.bounds_are_well_formed) {
    out.summary_issues.push_back(
        "numeric dimension bounds must be well formed");
  }
  if (!out.unit_interval_rows_are_bounded) {
    out.summary_issues.push_back(
        "closed-unit-interval rows must be bounded by [0,1]");
  }
  if (!out.integral_rows_are_count_like) {
    out.summary_issues.push_back(
        "integral rows must be non-negative count dimensions");
  }
  if (!out.threshold_directions_are_known) {
    out.summary_issues.push_back(
        "numeric dimension threshold directions must be known");
  }
  if (out.minimum_direction_count != 10 || out.above_direction_count != 9 ||
      out.below_direction_count != 4 ||
      out.metric_declared_direction_count != 13 ||
      out.planning_budget_direction_count != 1) {
    out.summary_issues.push_back(
        "numeric dimension threshold-direction counts must match lattice "
        "target table");
  }
  if (!out.coverage_cursor_epochs_dimension_present) {
    out.summary_issues.push_back(
        "coverage CURSOR_EPOCHS dimension must be a bounded coverage fraction");
  }
  if (!out.repeated_load_cursor_epoch_dimension_present) {
    out.summary_issues.push_back(
        "repeated exposure-load warning dimension must be cursor epochs");
  }
  if (!out.warning_anchor_fraction_dimension_present) {
    out.summary_issues.push_back(
        "anchor-domain accepted fraction warning dimension must be present");
  }
  if (!out.representation_condition_number_dimension_present) {
    out.summary_issues.push_back(
        "representation condition-number dimension must be present");
  }
  if (!out.node_support_floor_count_dimension_present) {
    out.summary_issues.push_back(
        "node support floor count dimension must be present");
  }
  if (!out.mdn_mean_nll_dimension_present) {
    out.summary_issues.push_back(
        "MDN mean NLL warning dimension must be present");
  }
  if (!out.coverage_and_load_units_separated) {
    out.summary_issues.push_back("coverage fractions and repeated load cursor "
                                 "epochs must stay separate");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_monotonicity_invariant_entry_t {
  std::string invariant{};
  std::string scope{};
  std::string order{};
  std::vector<std::string> stronger_dimensions{};
  std::vector<std::string> weaker_dimensions{};
  std::vector<std::string> neutral_dimensions{};
  std::string clean_growth_condition{};
  std::string failure_mode{};
  std::string proof_basis{};
  bool target_status_monotone{false};
};

struct lattice_warning_anchor_scope_policy_entry_t {
  std::string policy{};
  std::string surface{};
  std::string scope_resolution{};
  std::string overlap_basis{};
  std::string readiness_effect{};
  std::string identity_effect{};
  bool visibility_only{true};
  bool can_satisfy_readiness{false};
  bool contributes_to_certificate_digest{false};
};

[[nodiscard]] inline std::vector<lattice_warning_anchor_scope_policy_entry_t>
lattice_warning_anchor_scope_policy_vocabulary() {
  return {
      {"resolved_warning_anchor_range",
       "warning_results[].warning_anchor_range;"
       "explain_target.warning_scope_previews[].resolved_warning_anchor_range",
       "explicit ANCHOR_INDEX_BEGIN/END wins, otherwise SPLIT resolves through "
       "the active split policy, otherwise the target evidence range is used",
       "warning measurements are filtered to the resolved interval before "
       "threshold comparison",
       "non-blocking warning visibility only; status, plan-attempt accounting, "
       "and readiness coverage are unchanged",
       "excluded from protocol contract identity and target_spec_fingerprint",
       true, false, false},
      {"anchor_visibility_fallback", "warning_fact_overlaps_anchor_range",
       "same resolved warning interval",
       "trusted completed coverage is preferred; when it is unavailable, "
       "completed_anchor_range or declared anchor_range may expose warning "
       "visibility",
       "fallback overlap may trigger a warning basis but cannot contribute to "
       "proof_certificate.coverage or satisfy a target",
       "does not alter contract identity or promote untrusted requested ranges "
       "to proof authority",
       true, false, false},
      {"node_support_warning_matrix", "warning_results[].node_support_summary",
       "node-support warnings with SPLIT or explicit bounds derive a "
       "range-scoped support matrix for that warning",
       "node rows are intersected with the warning interval and filtered by "
       "USE/EFFECT before balance or floor metrics are computed",
       "MDN-head support warning visibility only in V1; VICReg per-node "
       "readiness still requires future representation support facts",
       "warning-scope matrix is excluded from target identity and certificate "
       "digest unless separately emitted as certificate node support",
       true, false, false}};
}

[[nodiscard]] inline std::vector<lattice_monotonicity_invariant_entry_t>
lattice_monotonicity_invariant_vocabulary() {
  return {
      {"clean_evidence_growth_target_satisfaction",
       "target_satisfied",
       "upper_set_over_clean_evidence",
       {"identity_match", "coverage_satisfied", "closure_complete",
        "leakage_passed", "proof_certificate_check_passed"},
       {"forbidden_overlap", "checkpoint_mismatch", "stale_identity",
        "blocking_deficit"},
       {"warning_result_count", "source_key_audit_count"},
       "new evidence preserves active identity, checkpoint binding, complete "
       "lineage, and forbidden-overlap absence",
       "forbidden or stale evidence is not clean growth and may turn a "
       "satisfied target into exposure_failed or blocked",
       "derived_query_rule_vocabulary.target_satisfied plus "
       "evidence_order_vector",
       true},
      {"coverage_union_monotone",
       "proof_certificate.coverage",
       "idempotent_interval_union",
       {"covered_intervals", "actual_fraction", "unique_covered_anchors"},
       {"missing_intervals", "missing_anchors"},
       {"cursor_exposure_load"},
       "added intervals must be completed, trusted, same split, same identity, "
       "and same use obligation",
       "requested or incomplete ranges are untrusted and do not strengthen "
       "coverage readiness",
       "coverage proof complement recomputation",
       false},
      {"load_additive_visibility",
       "exposure_summaries",
       "additive_interval_measure",
       {"cursor_exposure_load", "loaded_anchor_events",
        "optimizer_steps_total"},
       {"triggered_warning_count", "unavailable_warning_count"},
       {"warning_result_count"},
       "added load is clean only when it does not introduce warnings, "
       "deficits, "
       "or forbidden overlap",
       "higher repeated load with higher warnings is Pareto-incomparable "
       "rather "
       "than automatically stronger",
       "exposure_measure_algebra_vocabulary and evidence_order_vector",
       false},
      {"leakage_cleanliness_antitone",
       "proof_certificate.leakage",
       "forbidden_intersection_absence",
       {"leakage_passed"},
       {"overlap_witnesses", "blocking_deficit_count"},
       {},
       "new inherited exposure must have no forbidden-use intersection with "
       "the dilated protected split",
       "a new forbidden intersection weakens cleanliness and fails leakage",
       "leakage_rule_vocabulary and overlap witness recomputation",
       true},
      {"deficit_count_antitone",
       "deficits",
       "missing_proof_obligation_count",
       {"no_blocking_deficit"},
       {"blocking_deficit_count"},
       {"plan_basis"},
       "clean growth resolves or preserves proof obligations without adding "
       "new contradicted obligations",
       "new identity, certificate, dependency, model_state, artifact, "
       "closure, leakage, coverage, metric, or status deficits weaken "
       "evidence",
       "deficits plus deficit_priority_vocabulary",
       true},
      {"warning_count_antitone_for_order",
       "warning_summary",
       "non_blocking_order_dimension",
       {"clear_measured_warning_count"},
       {"triggered_warning_count", "unavailable_warning_count"},
       {"warning_result_count", "warning_count"},
       "warnings do not alter target status, but clean evidence should not add "
       "triggered or unavailable measurements",
       "more triggered or unavailable warnings weakens the evidence-order "
       "vector while preserving readiness status",
       "warning_summary and warning_threshold_relation_vocabulary",
       false},
      {"pareto_order_no_scalar",
       "evidence_order_vector",
       "pareto_partial_order",
       {"higher_is_stronger", "true_is_stronger", "lower_is_stronger"},
       {"incomparable_tradeoffs"},
       {"visibility_only", "alias_for_triggered_warning_count"},
       "dimensions are compared only through declared polarity and comparison "
       "membership",
       "tradeoffs across dimensions remain incomparable instead of being "
       "folded "
       "into one score",
       "compare_lattice_evidence_order_vectors",
       false},
      {"visibility_dimensions_do_not_dominate",
       "evidence_order_vector.dimension_vocabulary",
       "comparison_dimension_filter",
       {},
       {},
       {"source_key_audit_count", "warning_result_count", "warning_count"},
       "visibility-only and alias dimensions are emitted for audit/reporting "
       "but excluded from dominance",
       "visibility growth alone cannot prove stronger readiness",
       "lattice_evidence_order_dimension_vocabulary",
       false}};
}

struct lattice_proof_obligation_entry_t {
  std::string obligation{};
  std::string certificate_field{};
  std::string derived_relation{};
  std::string required_scope{};
  std::string failure_status{};
  std::string deficit_kind{};
  std::string non_claiming_condition{};
  std::string self_check_basis{};
  bool required_for_target_satisfaction{false};
  bool contributes_to_certificate_digest{true};
  bool planner_relevant{false};
};

[[nodiscard]] inline std::vector<lattice_proof_obligation_entry_t>
lattice_proof_obligation_vocabulary() {
  return {
      {"proof_context", "proof_certificate.proof_context", "identity_match",
       "all graph-anchor targets that require active identity",
       "stale_contract|stale_component|blocked", "proof_context",
       "missing active identity is blocking, not a stale-evidence claim",
       "required identity fields and match booleans recompute from active "
       "identity and evidence identity",
       true, true, true},
      {"dependency", "proof_certificate.dependencies[]", "dependency_satisfied",
       "targets with upstream target dependencies or exact checkpoint bindings",
       "blocked", "dependency|checkpoint_binding",
       "blocked upstream dependencies are explicit non-satisfied dependency "
       "proofs",
       "dependency kind/status vocabulary, duplicate rejection, and exact "
       "checkpoint binding checks",
       true, true, true},
      {"coverage", "proof_certificate.coverage[]", "coverage_satisfied",
       "targets with observed_input, target_supervision, or evaluation_metric "
       "coverage requirements",
       "exposure_failed", "coverage",
       "untrusted requested ranges do not claim readiness coverage",
       "interval union/complement recomputation, required/actual fraction "
       "arithmetic, and load-summary unit checks",
       true, true, true},
      {"closure", "proof_certificate.closure", "closure_complete",
       "targets whose selected checkpoint needs lineage for coverage, leakage, "
       "or checkpoint-source evidence",
       "blocked|exposure_failed", "closure",
       "unchecked closure is non-claiming; unresolved inputs make closure "
       "incomplete",
       "fact counts, unresolved inputs, causal checkpoint reachability, and "
       "producer assembly identity",
       true, true, true},
      {"leakage", "proof_certificate.leakage", "no_forbidden_overlap",
       "targets with FORBID/PROTECT split obligations", "exposure_failed",
       "leakage",
       "unchecked leakage is non-claiming and cannot prove protected-split "
       "cleanliness",
       "protected split dilation, forbidden-use membership, witness closure "
       "membership, and overlap recomputation",
       true, true, true},
      {"node_support", "proof_certificate.node_support_summaries[]",
       "node_support_summary_available",
       "MDN targets when inherited MDN node exposure facts are available",
       "none", "none",
       "missing node-support facts do not synthesize support and only affect "
       "visibility or warnings",
       "MDN-only scope, parent exposure closure backing, row-count/fraction "
       "arithmetic, weakest-node and balance metric recomputation",
       false, true, false},
      {"warnings", "warning_results[]|warning_summary", "warning_triggered",
       "targets with LATTICE_WARN clauses or scan-derived warning inputs",
       "none", "none",
       "unavailable measurements are structured non-blocking warning results",
       "threshold relation vocabulary, measurement_available bit, per-relation "
       "summary counts",
       false, false, false},
      {"deficits", "deficits[]", "plan_deficit",
       "all unsatisfied or contradicted proof dimensions", "status_specific",
       "identity|certificate|dependency|model_state|artifact|coverage|"
       "closure|leakage|metric|status",
       "no deficits are required for a satisfied target; unsatisfied targets "
       "must expose stable deficit keys",
       "deficit key, priority, priority_class, unit, range, and missing "
       "interval fields",
       false, false, true},
      {"plan_basis", "plan_basis", "plan_deficit",
       "plan-target output and unsatisfied evaluations", "none", "none",
       "planning is advisory and cannot satisfy a target by itself",
       "primary plan-relevant deficit projection, missing coverage intervals, "
       "and suggested action",
       false, false, false},
      {"certificate_check", "proof_certificate_check",
       "proof_certificate_check_passed", "all target evaluations", "blocked",
       "status",
       "well-formed non-claiming certificates may pass even when target status "
       "is unsatisfied",
       "schema, digest, canonical set ordering, vocabulary membership, and "
       "proof arithmetic",
       true, false, false}};
}

struct lattice_proof_certificate_digest_policy_entry_t {
  std::string surface{};
  std::string digest_role{};
  std::string included_fields{};
  std::string excluded_fields{};
  std::string self_check_rule{};
  std::string boundary{};
  bool contributes_to_certificate_digest{false};
  bool status_authority{false};
  bool advisory_only{false};
  bool visibility_only{false};
  bool audit_only{false};
};

[[nodiscard]] inline std::vector<
    lattice_proof_certificate_digest_policy_entry_t>
lattice_proof_certificate_digest_policy_vocabulary() {
  return {
      {"certificate_digest", "self-checking proof envelope digest",
       "none; the digest field is excluded from its own canonical text",
       "certificate_digest value itself; warning_results; warning_summary; "
       "warning_anchor_scope_policy_vocabulary; deficits; plan_basis; "
       "suggested_wave; evidence_order_vector; derived_query_results; policy "
       "vocabulary surfaces",
       "verify_lattice_target_proof_certificate requires certificate_digest, "
       "recomputes canonical_lattice_target_proof_certificate_text, and "
       "rejects digest mismatches",
       "binds the emitted proof object, not protocol contract identity, "
       "runtime model-state identity, scheduler state, or DB/cache rows",
       false, true, false, false, false},
      {"target_and_split_identity", "proof subject identity",
       "schema; target_id; target_spec_fingerprint; split_policy_fingerprint",
       "PLAN_INPUT_* values; warning clauses; runtime-loaded checkpoint paths "
       "outside dependency proof fields",
       "target fingerprint must be present; split policy fingerprint is copied "
       "from the active evaluation",
       "target proof identity is separate from protocol contract identity and "
       "from advisory plan inputs",
       true, true, false, false, false},
      {"proof_context", "active/evidence identity match proof",
       "proof_context.require_* flags; active/evidence contract, component, "
       "graph "
       "order, and source cursor token fields; match booleans",
       "checkpoint ids; checkpoint file digests; source receipt strings; "
       "runtime scan order",
       "required active/evidence identity fields and match booleans are "
       "checked for consistency",
       "stale or missing active identity blocks or stales readiness instead of "
       "claiming satisfaction",
       true, true, false, false, false},
      {"dependency_proofs",
       "upstream target and exact checkpoint binding proof",
       "dependencies[] kind; source_target_id; status; expected/actual MDN and "
       "representation checkpoint paths; checkpoint-match booleans",
       "PLAN_INPUT_* annotations that suggested the paths; warnings; "
       "non-loaded checkpoint candidates",
       "duplicate dependencies are rejected; satisfied checkpoint-source "
       "dependencies require non-empty exact path matches",
       "runtime model-state paths are accepted only as proven loaded-state "
       "evidence inside dependency proofs, never as contract identity",
       true, true, false, false, false},
      {"artifact_proofs", "fact-family artifact readiness proof",
       "artifacts[] proof_kind, fact_family, fact_schema, fact_type, "
       "fact_digest, fact-identity-contract binding, proof-template binding "
       "flag and claim, parent exposure digest, job/wave/protocol/contract/"
       "component/graph/source/split identity, anchor/completed row-index "
       "ranges, artifact/deterministic/visibility/authority-clean/lineage "
       "booleans, explicit false decision-authority flags, pass bit, and "
       "issues",
       "fact summaries outside the certificate; forecast quality metrics; "
       "allocation recommendations; runtime dispatch receipts",
       "duplicate artifact proofs are rejected; fact family, digest, identity, "
       "fact-identity-contract binding, proof-template binding and claim, "
       "deterministic artifact, visibility-only, authority-clean, explicit "
       "no-authority, lineage-bound, and pass fields are checked",
       "artifact proofs are narrow lineage/completeness claims over catalog "
       "facts, not training readiness, quality acceptance, allocation, market "
       "readiness, or runtime execution authority",
       true, true, false, false, false},
      {"coverage_proofs", "idempotent coverage and additive load proof",
       "coverage[] use, algebra, target range, contributing intervals, "
       "contributing fact digests, contributing fact witnesses, merged "
       "covered intervals, missing intervals, required/actual fractions, "
       "anchor counts, mutation filter, pass bit, and exposure load summary",
       "standalone exposure_summaries outside the certificate; warning "
       "threshold results",
       "self-check recomputes interval union, missing complement, fractions, "
       "fact-digest/witness bindings, and additive load arithmetic",
       "coverage proves readiness coverage; repeated load remains visible for "
       "warnings and audit",
       true, true, false, false, false},
      {"closure_causal_graph", "checkpoint lineage and causal exposure proof",
       "closure checked/complete flags, checkpoint path, fact count, "
       "unresolved inputs, fact digests, causal exposures, source-key window "
       "audit fields, optional runtime handoff id/digest and Marshal "
       "target-driver run id, output checkpoint, and input checkpoint paths",
       "source_file_receipts audit strings; DB/cache row ids; runtime scan "
       "ordering before canonical sort",
       "closure completeness, unresolved-input fail-closed policy, causal "
       "reachability, and canonical set ordering are checked",
       "V1 closure authority is path/exposure-fact based; source-key fields "
       "inside the causal proof are audit coordinates, not row-overlap "
       "authority",
       true, true, false, false, false},
      {"checkpoint_identity_previews",
       "checkpoint id/file digest audit preview bound to this proof object",
       "closure.checkpoint_identity_previews[] path, checkpoint_id, "
       "checkpoint_file_digest, component, assembly fingerprint, direct "
       "exposure digest, representation/context/output contract fields, "
       "created_by job, and created_by wave",
       "promotion of checkpoint_id or file digest to primary closure identity",
       "canonical text includes accepted preview fields and rejects duplicate "
       "or inconsistent checkpoint preview shapes through certificate checks",
       "preview fields are hashed for audit comparison but remain non-primary "
       "V1 closure identity",
       true, false, false, true, true},
      {"leakage_proof", "protected split dilation and forbidden-overlap proof",
       "leakage checked flag, rule, split, base/protected ranges, dilation "
       "widths, forbidden uses, mutation filter, overlap flag, overlap "
       "witnesses, and pass bit",
       "warnings about leakage risk not represented as overlap witnesses; "
       "future first-class selection-signal event streams",
       "self-check recomputes protected-set dilation, forbidden-use "
       "membership, overlap witnesses, and pass/overlap consistency",
       "leakage is temporal set intersection over the complete causal closure; "
       "unchecked leakage is non-claiming",
       true, true, false, false, false},
      {"node_support_summaries", "MDN node-support matrix visibility proof",
       "node_support_summaries[] split/use/job/node support counts, valid "
       "target fractions, Wilson intervals, weakest node, balance metrics, "
       "and aggregate support totals",
       "VICReg per-node readiness claims; synthetic backfilled node support; "
       "hard performance gates",
       "self-check verifies MDN-only scope, parent exposure backing, count and "
       "fraction arithmetic, uncertainty intervals, and balance metrics",
       "node support contributes to the certificate digest for audit but is "
       "visibility/warning scope in V1 unless a target explicitly requires it",
       true, false, false, true, false},
      {"warnings_deficits_and_plans",
       "excluded status explanation and advisory planning projections",
       "none in the certificate canonical digest; these are emitted beside "
       "proof_certificate",
       "warning_results; warning_summary; warning_anchor_scope_policy_"
       "vocabulary; warnings; deficits; plan_basis; suggested_wave; "
       "PLAN_INPUT_*; PLAN_MAX_ATTEMPTS/MAX_WAVES accounting",
       "warnings and deficits are derived from proof/status surfaces; plans "
       "must remain advisory and cannot repair or satisfy a proof",
       "warning visibility and plan advice are report surfaces, not proof "
       "content and not contract identity",
       false, false, true, true, false},
      {"evidence_order_and_policy_vocabularies",
       "excluded comparison and documentation metadata",
       "none in the certificate canonical digest; these are emitted beside "
       "proof_certificate",
       "evidence_order_vector; *_vocabulary Hero fields; DB/cache policy "
       "descriptions; mathematical-readiness crosswalk; derived_query_results",
       "certificate digest stays stable with respect to explanatory vocabulary "
       "surfaces and Pareto comparison projections",
       "policy tables explain the evaluator contract but are not runtime "
       "evidence and do not satisfy targets",
       false, false, false, true, true}};
}

struct lattice_proof_certificate_digest_policy_summary_t {
  std::string schema{
      "kikijyeba.lattice.proof_certificate_digest_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t digest_contributing_count{0};
  std::int64_t digest_excluded_count{0};
  std::int64_t status_authority_count{0};
  std::int64_t advisory_only_count{0};
  std::int64_t visibility_only_count{0};
  std::int64_t audit_only_count{0};
  std::int64_t digest_non_status_authority_count{0};
  std::int64_t status_authority_not_digest_count{0};
  std::int64_t advisory_digest_overlap_count{0};
  std::int64_t status_authority_visibility_overlap_count{0};
  std::int64_t empty_policy_field_count{0};
  bool all_required_surfaces_present{false};
  bool digest_has_core_status_authority_surfaces{false};
  bool certificate_digest_self_excluded{false};
  bool advisory_report_surfaces_outside_digest{false};
  bool visibility_surfaces_do_not_drive_status{false};
  bool digest_contributors_have_self_checks{false};
  bool all_policies_have_boundary_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> digest_contributing_surfaces{};
  std::vector<std::string> digest_excluded_surfaces{};
  std::vector<std::string> missing_required_surfaces{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_proof_certificate_digest_policy_summary_t
lattice_proof_certificate_digest_policy_summary() {
  lattice_proof_certificate_digest_policy_summary_t out{};
  const auto vocabulary = lattice_proof_certificate_digest_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());
  std::set<std::string> surfaces;
  bool all_digest_contributors_have_self_checks = true;

  for (const auto &entry : vocabulary) {
    surfaces.insert(entry.surface);
    if (entry.contributes_to_certificate_digest) {
      ++out.digest_contributing_count;
      out.digest_contributing_surfaces.push_back(entry.surface);
      if (!entry.status_authority) {
        ++out.digest_non_status_authority_count;
      }
      if (entry.self_check_rule.empty()) {
        all_digest_contributors_have_self_checks = false;
      }
    } else {
      ++out.digest_excluded_count;
      out.digest_excluded_surfaces.push_back(entry.surface);
    }
    if (entry.status_authority) {
      ++out.status_authority_count;
      if (!entry.contributes_to_certificate_digest) {
        ++out.status_authority_not_digest_count;
      }
    }
    if (entry.advisory_only) {
      ++out.advisory_only_count;
      if (entry.contributes_to_certificate_digest) {
        ++out.advisory_digest_overlap_count;
      }
    }
    if (entry.visibility_only) {
      ++out.visibility_only_count;
      if (entry.status_authority) {
        ++out.status_authority_visibility_overlap_count;
      }
    }
    if (entry.audit_only) {
      ++out.audit_only_count;
    }
    if (entry.surface.empty() || entry.digest_role.empty() ||
        entry.included_fields.empty() || entry.excluded_fields.empty() ||
        entry.self_check_rule.empty() || entry.boundary.empty()) {
      ++out.empty_policy_field_count;
    }
  }

  const std::vector<std::string> required_surfaces{
      "certificate_digest",
      "target_and_split_identity",
      "proof_context",
      "dependency_proofs",
      "artifact_proofs",
      "coverage_proofs",
      "closure_causal_graph",
      "checkpoint_identity_previews",
      "leakage_proof",
      "node_support_summaries",
      "warnings_deficits_and_plans",
      "evidence_order_and_policy_vocabularies"};
  for (const auto &surface : required_surfaces) {
    if (surfaces.find(surface) == surfaces.end()) {
      out.missing_required_surfaces.push_back(surface);
    }
  }

  const auto find_policy = [&](const std::string &surface) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.surface == surface; });
  };
  const auto self_policy = find_policy("certificate_digest");
  out.certificate_digest_self_excluded =
      self_policy != vocabulary.end() &&
      !self_policy->contributes_to_certificate_digest &&
      self_policy->status_authority;

  out.digest_has_core_status_authority_surfaces = true;
  const std::vector<std::string> core_status_surfaces{
      "target_and_split_identity",
      "proof_context",
      "dependency_proofs",
      "artifact_proofs",
      "coverage_proofs",
      "closure_causal_graph",
      "leakage_proof"};
  for (const auto &surface : core_status_surfaces) {
    const auto policy = find_policy(surface);
    if (policy == vocabulary.end() ||
        !policy->contributes_to_certificate_digest ||
        !policy->status_authority) {
      out.digest_has_core_status_authority_surfaces = false;
    }
  }

  out.all_required_surfaces_present = out.missing_required_surfaces.empty();
  out.advisory_report_surfaces_outside_digest =
      out.advisory_digest_overlap_count == 0;
  out.visibility_surfaces_do_not_drive_status =
      out.status_authority_visibility_overlap_count == 0;
  out.digest_contributors_have_self_checks =
      all_digest_contributors_have_self_checks;
  out.all_policies_have_boundary_text = out.empty_policy_field_count == 0;

  if (out.policy_count != 12) {
    out.summary_issues.push_back(
        "proof certificate digest policy vocabulary must contain 12 surfaces");
  }
  if (out.digest_contributing_count != 9) {
    out.summary_issues.push_back(
        "proof certificate digest must bind exactly 9 proof surfaces in V1");
  }
  if (out.digest_excluded_count != 3) {
    out.summary_issues.push_back(
        "proof certificate digest policy must exclude exactly 3 report or "
        "policy surfaces in V1");
  }
  if (out.status_authority_count != 8) {
    out.summary_issues.push_back(
        "proof certificate digest policy must expose 8 status-authority "
        "surfaces in V1");
  }
  if (out.digest_non_status_authority_count != 2) {
    out.summary_issues.push_back(
        "proof certificate digest must bind exactly 2 visibility/audit "
        "surfaces without making them status authority");
  }
  if (out.status_authority_not_digest_count != 1) {
    out.summary_issues.push_back(
        "only the certificate digest self-check surface should be status "
        "authority outside its own digest");
  }
  if (!out.all_required_surfaces_present) {
    out.summary_issues.push_back(
        "proof certificate digest policy vocabulary is missing required "
        "surfaces");
  }
  if (!out.certificate_digest_self_excluded) {
    out.summary_issues.push_back(
        "certificate_digest must be excluded from its own canonical digest");
  }
  if (!out.digest_has_core_status_authority_surfaces) {
    out.summary_issues.push_back(
        "core identity, dependency, coverage, closure, and leakage proof "
        "surfaces must contribute to the digest and status authority");
  }
  if (!out.advisory_report_surfaces_outside_digest) {
    out.summary_issues.push_back(
        "advisory warning, deficit, or plan surfaces must not contribute to "
        "the certificate digest");
  }
  if (!out.visibility_surfaces_do_not_drive_status) {
    out.summary_issues.push_back(
        "visibility-only surfaces must not become status authority");
  }
  if (!out.digest_contributors_have_self_checks) {
    out.summary_issues.push_back(
        "all digest-contributing proof surfaces must declare self-check rules");
  }
  if (!out.all_policies_have_boundary_text) {
    out.summary_issues.push_back(
        "all proof certificate digest policy rows must declare boundary text");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_checkpoint_identity_policy_entry_t {
  std::string policy{};
  std::string surface{};
  std::string v1_authority{};
  std::vector<std::string> preview_fields{};
  std::string binding_requirement{};
  std::string stale_sidecar_policy{};
  std::string v2_promotion{};
  std::string contract_identity_effect{};
  bool closure_authority{false};
  bool db_cache_key_ready{false};
};

[[nodiscard]] inline std::vector<lattice_checkpoint_identity_policy_entry_t>
lattice_checkpoint_identity_policy_vocabulary() {
  return {
      {"path_exposure_closure_v1",
       "proof_certificate.closure.checkpoint_path",
       "checkpoint path plus exposure-fact closure reachability",
       {"checkpoint_path", "fact_digests", "causal_exposures"},
       "closure root checkpoint must be produced by a reachable exposure fact; "
       "input checkpoints must resolve to producer facts",
       "missing or stale checkpoint sidecars do not replace exposure-fact "
       "closure and remain scan/audit warnings",
       "promote checkpoint_id and checkpoint_file_digest only after cached "
       "closure can be rebuilt and checked against runtime facts",
       "checkpoint paths and ids are runtime model-state evidence, not "
       "protocol "
       "contract identity",
       true,
       false},
      {"checkpoint_identity_preview_v1",
       "proof_certificate.closure.checkpoint_identity_previews[]",
       "audit/index preview bound to the current path-based closure",
       {"checkpoint_path", "checkpoint_id", "checkpoint_file_digest",
        "direct_exposure_digest", "representation_contract",
        "input_representation_assembly_id", "context_contract",
        "output_contract", "created_by_job_id", "created_by_wave_id"},
       "direct_exposure_digest must be present in the current closure fact "
       "digests before preview fields can be emitted",
       "sidecars whose direct exposure digest is not in closure are rejected "
       "from the proof preview",
       "use ids/digests as primary checkpoint identity only in a future "
       "identity-promoted closure layer",
       "preview fields do not alter target_spec_fingerprint or protocol "
       "contract fingerprint",
       false,
       false},
      {"exact_loaded_checkpoint_binding_v1",
       "proof_certificate.dependencies[]",
       "path equality for runtime-loaded MDN/representation checkpoints",
       {"expected_mdn_checkpoint", "actual_mdn_checkpoint",
        "expected_representation_checkpoint",
        "actual_representation_checkpoint", "mdn_checkpoint_match",
        "representation_checkpoint_match"},
       "validation/evaluation evidence must prove the exact runtime-loaded "
       "checkpoint paths selected by upstream no-leakage targets and each "
       "evaluation witness must carry exactly those checkpoint input edges",
       "vacuous, missing, or mismatched loaded-checkpoint paths block "
       "satisfaction",
       "future checkpoint-id binding may supplement path equality after "
       "checkpoint identity promotion",
       "runtime-loaded checkpoint paths are model-state input evidence, not "
       "contract identity",
       true,
       false},
      {"checkpoint_digest_cache_preview_v1",
       "lattice.checkpoint.fact",
       "sidecar digest metadata is rebuildable audit evidence",
       {"checkpoint_id", "checkpoint_file_digest", "closure_digest",
        "direct_exposure_digest"},
       "digest metadata must be regenerated from durable checkpoint and "
       "exposure evidence before being trusted as a cache key",
       "digest mismatches or unavailable checkpoint files prevent cache-key "
       "promotion and stay outside V1 closure authority",
       "v2 may make checkpoint_id/file_digest primary identity for DB/cache "
       "lookups while preserving runtime files as source of truth",
       "digest metadata does not split or modify protocol contract identity",
       false,
       false}};
}

struct lattice_checkpoint_selection_policy_entry_t {
  std::string policy{};
  std::string surface{};
  std::string selection_rule{};
  std::string allowed_claim{};
  std::string forbidden_claim{};
  std::string binding_requirement{};
  std::string future_extension{};
  bool deterministic_readiness_selector{false};
  bool requires_target_satisfied{true};
  bool exact_runtime_binding_required{false};
  bool performance_selector{false};
  bool pareto_optimizer{false};
  bool contract_identity_authority{false};
  bool runtime_executor{false};
};

[[nodiscard]] inline std::vector<lattice_checkpoint_selection_policy_entry_t>
lattice_checkpoint_selection_policy_vocabulary() {
  return {
      {"latest_satisfying_checkpoint_source",
       "CHECKPOINT_SOURCE=latest_satisfying:<target_id>",
       "evaluate the referenced target and use its newest satisfied checkpoint "
       "candidate under the active identity",
       "guard this target with a checkpoint that already satisfies the "
       "referenced readiness/leakage target",
       "best model, best metric, Pareto optimum, deployment recommendation, or "
       "performance ranking",
       "referenced target must be satisfied and expose an existing checkpoint "
       "artifact",
       "future selectors may add explicit best_nondominated or "
       "least_warning_satisfying modes without changing latest_satisfying",
       true, true, false, false, false, false, false},
      {"latest_satisfying_evaluated_checkpoint_source",
       "EVALUATED_CHECKPOINT_SOURCE=latest_satisfying:<target_id>",
       "select the referenced satisfied checkpoint as the model-state input "
       "that validation/evaluation evidence must load",
       "prove validation/evaluation inspected the exact trained checkpoint "
       "selected by the guard",
       "fresh initialized MDN, wrong checkpoint, best metric selector, or "
       "implicit performance approval",
       "runtime report and evaluation exposure witness must prove loaded MDN "
       "checkpoint and representation checkpoint match the selected source "
       "lineage without extra checkpoint input edges",
       "future checkpoint_id/file_digest binding may supplement path equality",
       true, true, true, false, false, false, false},
      {"latest_satisfying_plan_input_hint",
       "PLAN_INPUT_MDN_CHECKPOINT|PLAN_INPUT_REPRESENTATION_CHECKPOINT",
       "carry a symbolic latest_satisfying runtime model-state input hint "
       "without changing target proof identity",
       "advise Runtime Hero which satisfied checkpoint-source target to "
       "inspect "
       "before the next wave",
       "readiness proof, contract identity, execution authority, or evidence "
       "that the runtime actually loaded the checkpoint",
       "subsequent runtime report/facts must prove the checkpoint that was "
       "loaded",
       "future planning may surface richer model-state input receipts while "
       "remaining advisory",
       true, true, true, false, false, false, false},
      {"pareto_order_not_checkpoint_selector", "evidence_order_vector",
       "compare evidence vectors only when callers explicitly ask for Pareto "
       "dominance",
       "explain evidence tradeoffs without collapsing dimensions into one "
       "score",
       "implicit latest_satisfying tie-breaker, scalar lattice_score, or "
       "performance ranking",
       "checkpoint selectors must name their selection rule separately from "
       "the "
       "Pareto comparator",
       "future best_nondominated_satisfying selectors must report the vector "
       "and tie policy rather than hiding it behind latest_satisfying",
       false, true, false, false, false, false, false}};
}

struct lattice_checkpoint_selection_policy_summary_t {
  std::string schema{
      "kikijyeba.lattice.checkpoint_selection_policy.summary.v1"};
  std::int64_t policy_count{0};
  std::int64_t latest_satisfying_surface_count{0};
  std::int64_t deterministic_readiness_selector_count{0};
  std::int64_t requires_target_satisfied_count{0};
  std::int64_t exact_runtime_binding_required_count{0};
  std::int64_t performance_selector_count{0};
  std::int64_t pareto_optimizer_count{0};
  std::int64_t contract_identity_authority_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t empty_policy_field_count{0};
  bool all_policies_require_satisfied_target{false};
  bool latest_satisfying_is_readiness_selection{false};
  bool validation_eval_requires_exact_runtime_binding{false};
  bool plan_input_hint_is_runtime_model_state_advice{false};
  bool pareto_order_is_not_checkpoint_selector{false};
  bool no_performance_selector{false};
  bool no_pareto_optimizer{false};
  bool no_contract_identity_authority{false};
  bool no_runtime_executor{false};
  bool all_policies_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> deterministic_selector_policies{};
  std::vector<std::string> exact_runtime_binding_policies{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_checkpoint_selection_policy_summary_t
lattice_checkpoint_selection_policy_summary() {
  lattice_checkpoint_selection_policy_summary_t out{};
  const auto vocabulary = lattice_checkpoint_selection_policy_vocabulary();
  out.policy_count = static_cast<std::int64_t>(vocabulary.size());

  const auto contains_latest_satisfying = [](const auto &entry) {
    return entry.surface.find("latest_satisfying") != std::string::npos ||
           entry.selection_rule.find("latest_satisfying") !=
               std::string::npos ||
           entry.allowed_claim.find("latest_satisfying") != std::string::npos;
  };
  for (const auto &entry : vocabulary) {
    if (contains_latest_satisfying(entry)) {
      ++out.latest_satisfying_surface_count;
    }
    if (entry.deterministic_readiness_selector) {
      ++out.deterministic_readiness_selector_count;
      out.deterministic_selector_policies.push_back(entry.policy);
    }
    if (entry.requires_target_satisfied) {
      ++out.requires_target_satisfied_count;
    }
    if (entry.exact_runtime_binding_required) {
      ++out.exact_runtime_binding_required_count;
      out.exact_runtime_binding_policies.push_back(entry.policy);
    }
    if (entry.performance_selector) {
      ++out.performance_selector_count;
    }
    if (entry.pareto_optimizer) {
      ++out.pareto_optimizer_count;
    }
    if (entry.contract_identity_authority) {
      ++out.contract_identity_authority_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.policy.empty() || entry.surface.empty() ||
        entry.selection_rule.empty() || entry.allowed_claim.empty() ||
        entry.forbidden_claim.empty() || entry.binding_requirement.empty() ||
        entry.future_extension.empty()) {
      ++out.empty_policy_field_count;
    }
  }

  const auto find_policy = [&](const std::string &policy) {
    return std::find_if(
        vocabulary.begin(), vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto checkpoint_source =
      find_policy("latest_satisfying_checkpoint_source");
  const auto evaluated_checkpoint =
      find_policy("latest_satisfying_evaluated_checkpoint_source");
  const auto plan_input = find_policy("latest_satisfying_plan_input_hint");
  const auto pareto_not_selector =
      find_policy("pareto_order_not_checkpoint_selector");

  out.all_policies_require_satisfied_target =
      out.requires_target_satisfied_count == out.policy_count &&
      out.policy_count > 0;
  out.latest_satisfying_is_readiness_selection =
      out.latest_satisfying_surface_count == 3 &&
      out.deterministic_readiness_selector_count == 3 &&
      checkpoint_source != vocabulary.end() &&
      checkpoint_source->forbidden_claim.find("best metric") !=
          std::string::npos;
  out.validation_eval_requires_exact_runtime_binding =
      evaluated_checkpoint != vocabulary.end() &&
      evaluated_checkpoint->exact_runtime_binding_required &&
      evaluated_checkpoint->binding_requirement.find("loaded MDN") !=
          std::string::npos &&
      evaluated_checkpoint->forbidden_claim.find("fresh initialized MDN") !=
          std::string::npos;
  out.plan_input_hint_is_runtime_model_state_advice =
      plan_input != vocabulary.end() &&
      plan_input->exact_runtime_binding_required &&
      plan_input->selection_rule.find("symbolic") != std::string::npos &&
      plan_input->forbidden_claim.find("contract identity") !=
          std::string::npos;
  out.pareto_order_is_not_checkpoint_selector =
      pareto_not_selector != vocabulary.end() &&
      !pareto_not_selector->deterministic_readiness_selector &&
      pareto_not_selector->forbidden_claim.find("lattice_score") !=
          std::string::npos &&
      pareto_not_selector->future_extension.find("best_nondominated") !=
          std::string::npos;
  out.no_performance_selector = out.performance_selector_count == 0;
  out.no_pareto_optimizer = out.pareto_optimizer_count == 0;
  out.no_contract_identity_authority =
      out.contract_identity_authority_count == 0;
  out.no_runtime_executor = out.runtime_executor_count == 0;
  out.all_policies_have_claim_text = out.empty_policy_field_count == 0;

  if (out.policy_count != 4) {
    out.summary_issues.push_back(
        "checkpoint selection policy vocabulary must contain 4 policies");
  }
  if (!out.all_policies_require_satisfied_target) {
    out.summary_issues.push_back(
        "checkpoint selectors must require a satisfied source target");
  }
  if (!out.latest_satisfying_is_readiness_selection) {
    out.summary_issues.push_back(
        "latest_satisfying must remain deterministic readiness selection");
  }
  if (!out.validation_eval_requires_exact_runtime_binding) {
    out.summary_issues.push_back(
        "validation/evaluation checkpoint selection must require exact loaded "
        "checkpoint binding");
  }
  if (!out.plan_input_hint_is_runtime_model_state_advice) {
    out.summary_issues.push_back(
        "PLAN_INPUT checkpoint hints must remain symbolic runtime model-state "
        "advice");
  }
  if (!out.pareto_order_is_not_checkpoint_selector) {
    out.summary_issues.push_back(
        "Pareto evidence order must not be the implicit checkpoint selector");
  }
  if (!out.no_performance_selector) {
    out.summary_issues.push_back(
        "V1 checkpoint selection must not be performance selection");
  }
  if (!out.no_pareto_optimizer) {
    out.summary_issues.push_back(
        "V1 checkpoint selection must not be Pareto optimization");
  }
  if (!out.no_contract_identity_authority) {
    out.summary_issues.push_back(
        "checkpoint selection policy must not alter contract identity");
  }
  if (!out.no_runtime_executor) {
    out.summary_issues.push_back(
        "checkpoint selection policy must not execute runtime work");
  }
  if (!out.all_policies_have_claim_text) {
    out.summary_issues.push_back("checkpoint selection policy rows must "
                                 "declare claim and boundary text");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_plan_advice_scope_policy_entry_t {
  std::string policy{};
  std::string surface{};
  std::string source_relation{};
  std::string allowed_claim{};
  std::string forbidden_claim{};
  std::string execution_boundary{};
  bool advice_only{true};
  bool evidence_authority{false};
  bool scheduler_authority{false};
  bool lattice_executor{false};
  bool affects_contract_identity{false};
  bool attempt_budget_guard{false};
};

[[nodiscard]] inline std::vector<lattice_plan_advice_scope_policy_entry_t>
lattice_plan_advice_scope_policy_vocabulary() {
  return {
      {"plan_basis_deficit_projection", "plan_basis",
       "deficits[] plus suggested_wave",
       "explain the primary plan-relevant proof deficit, addressed deficit "
       "classes, and missing coverage intervals",
       "proof certificate source, target satisfaction, runtime evidence, or "
       "second status system",
       "Runtime Hero executes only after an explicit caller accepts advice",
       true, false, false, false, false, false},
      {"suggested_wave_advice_only", "suggested_wave",
       "target plan clauses and current proof deficits",
       "recommend a target/action/range/mode that could address missing "
       "evidence",
       "dispatch, schedule, mutate runtime state, satisfy a target, or consume "
       "attempt budget",
       "Lattice Hero emits advice; Runtime Hero owns execution policy", true,
       false, false, false, false, false},
      {"plan_input_model_state_hint",
       "PLAN_INPUT_MDN_CHECKPOINT|PLAN_INPUT_REPRESENTATION_CHECKPOINT",
       "symbolic latest_satisfying target references carried by suggested "
       "runtime inputs",
       "advise which checkpoint-source target Runtime Hero should inspect "
       "before asking the next wave to load model state",
       "evidence that the checkpoint was loaded, contract identity, or "
       "checkpoint-binding proof",
       "runtime reports, exposure facts, and dependency proofs must verify "
       "loaded checkpoint state",
       true, false, false, false, false, false},
      {"plan_attempt_budget_guard", "PLAN_MAX_ATTEMPTS|MAX_WAVES",
       "matching active train attempts under current identity",
       "limit whether Lattice Hero advertises another suggested wave",
       "proof obligation identity, runtime attempt mutation, or target "
       "satisfaction",
       "attempts are created by Runtime Hero/runtime jobs, not by planning",
       true, false, false, false, false, true},
      {"runtime_hero_executor_boundary", "hero.runtime.run",
       "accepted suggested wave plus Runtime Hero policy",
       "execute or dry-run the accepted runtime wave under Runtime Hero guards",
       "Lattice Hero execution authority or automatic scheduling from "
       "target_deficit",
       "Runtime Hero is the only executor; Lattice Hero remains read-only",
       false, false, true, false, false, false}};
}

struct lattice_operational_readiness_v1_scope_entry_t {
  std::string item{};
  std::string category{};
  std::string authority{};
  std::string allowed_claim{};
  std::string excluded_claim{};
  std::string future_work{};
  bool included_in_v1{false};
  bool deferred_beyond_v1{false};
  bool read_only_evidence_authority{false};
  bool runtime_executor_authority{false};
  bool performance_gate_authority{false};
  bool db_source_of_truth_authority{false};
};

[[nodiscard]] inline std::vector<lattice_operational_readiness_v1_scope_entry_t>
lattice_operational_readiness_v1_scope_vocabulary() {
  return {
      {"read_only_evidence_authority", "included", "lattice_read_model",
       "evaluate compiled graph-first contract readiness from runtime facts, "
       "sidecars, certificates, closures, warnings, deficits, and plan advice",
       "runtime execution, scheduler authority, model performance approval, or "
       "DB-backed "
       "source-of-truth claims",
       "future DB/cache may index derived relations while runtime files remain "
       "authoritative",
       true, false, true, false, false, false},
      {"train_core_readiness", "included", "target_satisfaction",
       "prove VICReg and node-MDN train-core readiness from completed normal "
       "runtime evidence under active identity",
       "debug-only evidence, stale identity, or validation performance quality",
       "future targets may add stronger representation or performance gates",
       true, false, true, false, false, false},
      {"checkpoint_lineage_and_holdout_cleanliness", "included",
       "checkpoint_closure_and_leakage_proof",
       "prove checkpoint closure completeness and validation/test holdout "
       "cleanliness with protected split dilation",
       "unresolved lineage, unchecked closure, or forbidden protected-split "
       "overlap",
       "future selection-event provenance can extend causal paths", true, false,
       true, false, false, false},
      {"validation_evaluation_readiness", "included",
       "evaluation_metric_coverage_and_checkpoint_binding",
       "prove clean run-mode validation evaluation of the exact selected MDN "
       "checkpoint and matching representation lineage",
       "model quality, deployability, or performance approval",
       "future performance target with explicit metrics and uncertainty", true,
       false, true, false, false, false},
      {"warning_and_support_visibility", "included", "non_blocking_visibility",
       "surface anchor-domain health, repeated load, optimizer effort, MDN "
       "node "
       "support, uncertainty, and representation/calibration warnings",
       "automatic target failure or scheduler action from warnings alone",
       "future targets may promote selected warnings into explicit gates", true,
       false, true, false, false, false},
      {"hero_read_plan_surface", "included",
       "read_explain_evaluate_plan_scan_closure",
       "let agents inspect status, evidence, warnings, closures, deficits, and "
       "suggested waves without executing",
       "Lattice Hero runtime execution or scheduler authority",
       "Runtime Hero remains the execution surface", true, false, true, false,
       false, false},
      {"db_index", "deferred", "future_cache_index",
       "none in V1 beyond policy vocabulary and rebuildable-cache semantics",
       "DB rows as source of truth or runtime execution dependency",
       "build DB/index as a cache over immutable runtime files and derived "
       "relations",
       false, true, false, false, false, false},
      {"performance_targets", "deferred", "future_performance_gate",
       "non-blocking visibility only for V1 metrics and warnings",
       "hard model-quality, deployability, or performance acceptance",
       "add explicit metrics, thresholds, uncertainty, and selection-leakage "
       "policy",
       false, true, false, false, false, false},
      {"stable_checkpoint_identity", "deferred", "future_checkpoint_identity",
       "checkpoint ids and file digests are audit/cache previews in V1",
       "checkpoint_id or file_digest as primary closure identity",
       "promote checkpoint_id/file_digest after runtime-file rebuild and "
       "closure checks are authoritative",
       false, true, false, false, false, false},
      {"structured_source_receipt_facts", "deferred",
       "future_source_provenance_fact",
       "compact source_file_receipts are audit metadata only in V1",
       "source receipts as coverage, leakage, or contract identity authority",
       "add normalized source receipt facts for audit/index work", false, true,
       false, false, false, false},
      {"first_class_selection_signal_events", "deferred",
       "future_selection_event_graph",
       "known selection_signal exposure labels are forbidden causal uses when "
       "protected",
       "structured model-selection event provenance",
       "add first-class selection-signal events and path queries", false, true,
       false, false, false, false},
      {"vicreg_node_support_facts", "deferred",
       "future_shared_representation_node_support",
       "MDN node-support summaries are MDN-head visibility only",
       "per-node VICReg readiness from downstream MDN node facts",
       "emit honest NodeLift or representation node-support facts", false, true,
       false, false, false, false}};
}

struct lattice_operational_readiness_v1_scope_summary_t {
  std::string schema{
      "kikijyeba.lattice.operational_readiness_v1.scope_summary.v1"};
  std::int64_t item_count{0};
  std::int64_t included_in_v1_count{0};
  std::int64_t deferred_beyond_v1_count{0};
  std::int64_t included_and_deferred_count{0};
  std::int64_t read_only_evidence_authority_count{0};
  std::int64_t runtime_executor_authority_count{0};
  std::int64_t performance_gate_authority_count{0};
  std::int64_t db_source_of_truth_authority_count{0};
  std::int64_t empty_claim_field_count{0};
  bool included_deferred_partition_complete{false};
  bool included_items_are_read_only{false};
  bool no_runtime_executor_authority{false};
  bool no_performance_gate_authority{false};
  bool no_db_source_of_truth_authority{false};
  bool all_scope_items_have_claim_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> included_items{};
  std::vector<std::string> deferred_items{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_operational_readiness_v1_scope_summary_t
lattice_operational_readiness_v1_scope_summary() {
  lattice_operational_readiness_v1_scope_summary_t out{};
  const auto vocabulary = lattice_operational_readiness_v1_scope_vocabulary();
  out.item_count = static_cast<std::int64_t>(vocabulary.size());
  for (const auto &entry : vocabulary) {
    if (entry.included_in_v1) {
      ++out.included_in_v1_count;
      out.included_items.push_back(entry.item);
    }
    if (entry.deferred_beyond_v1) {
      ++out.deferred_beyond_v1_count;
      out.deferred_items.push_back(entry.item);
    }
    if (entry.included_in_v1 && entry.deferred_beyond_v1) {
      ++out.included_and_deferred_count;
    }
    if (entry.read_only_evidence_authority) {
      ++out.read_only_evidence_authority_count;
    }
    if (entry.runtime_executor_authority) {
      ++out.runtime_executor_authority_count;
    }
    if (entry.performance_gate_authority) {
      ++out.performance_gate_authority_count;
    }
    if (entry.db_source_of_truth_authority) {
      ++out.db_source_of_truth_authority_count;
    }
    if (entry.allowed_claim.empty() || entry.excluded_claim.empty() ||
        entry.future_work.empty()) {
      ++out.empty_claim_field_count;
    }
  }

  out.included_deferred_partition_complete =
      out.item_count > 0 && out.included_and_deferred_count == 0 &&
      out.included_in_v1_count + out.deferred_beyond_v1_count == out.item_count;
  out.included_items_are_read_only =
      out.read_only_evidence_authority_count == out.included_in_v1_count &&
      out.included_in_v1_count > 0;
  out.no_runtime_executor_authority = out.runtime_executor_authority_count == 0;
  out.no_performance_gate_authority = out.performance_gate_authority_count == 0;
  out.no_db_source_of_truth_authority =
      out.db_source_of_truth_authority_count == 0;
  out.all_scope_items_have_claim_text = out.empty_claim_field_count == 0;

  if (out.item_count != 12) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope vocabulary must contain 12 items");
  }
  if (out.included_in_v1_count != 6) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope must include exactly 6 V1 claims");
  }
  if (out.deferred_beyond_v1_count != 6) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope must defer exactly 6 beyond-V1 items");
  }
  if (!out.included_deferred_partition_complete) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope rows must partition into included or "
        "deferred");
  }
  if (!out.included_items_are_read_only) {
    out.summary_issues.push_back(
        "included Operational Readiness V1 scope rows must be read-only");
  }
  if (!out.no_runtime_executor_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope must not grant runtime execution");
  }
  if (!out.no_performance_gate_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope must not be a performance gate");
  }
  if (!out.no_db_source_of_truth_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope must not make DB rows source of truth");
  }
  if (!out.all_scope_items_have_claim_text) {
    out.summary_issues.push_back(
        "Operational Readiness V1 scope rows must declare allowed, excluded, "
        "and future-work text");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_operational_readiness_v1_gate_entry_t {
  std::string gate{};
  std::string required_evidence{};
  std::vector<std::string> target_ids{};
  std::vector<std::string> hero_surfaces{};
  std::string pass_condition{};
  std::string fail_condition{};
  std::string v1_boundary{};
  bool required_for_v1{true};
  bool read_only_evidence_authority{true};
  bool runtime_executor_authority{false};
  bool performance_gate_authority{false};
  bool db_source_of_truth_authority{false};
};

[[nodiscard]] inline std::vector<lattice_operational_readiness_v1_gate_entry_t>
lattice_operational_readiness_v1_gate_vocabulary() {
  return {
      {"read_only_authority_boundary",
       "Lattice Hero evaluates, explains, scans, closes, compares, and plans "
       "over existing runtime evidence",
       {},
       {"operational_readiness_v1_scope_vocabulary",
        "plan_advice_scope_policy_vocabulary",
        "deficit_vector_planning_summary"},
       "Lattice surfaces status, proof certificates, warnings, deficits, "
       "closure, and suggested waves without executing runtime work",
       "Lattice Hero dispatches waves, claims scheduler authority, treats DB "
       "rows as source of truth, or accepts model performance",
       "Runtime Hero remains the only executor; DB/index and performance gates "
       "are outside V1",
       true,
       true,
       false,
       false,
       false},
      {"active_identity_binding",
       "active protocol contract fingerprint, graph order fingerprint, source "
       "cursor token, and component assembly fingerprints",
       {"vicreg_train_core_ready", "channel_mdn_train_core_ready",
        "channel_mdn_train_core_no_validation_leakage",
        "channel_mdn_train_core_no_test_leakage",
        "channel_mdn_validation_eval_ready"},
       {"proof_certificate.proof_context",
        "contract_identity_boundary_vocabulary", "proof_certificate_check"},
       "all graph-anchor targets prove active/evidence identity match under "
       "the compiled target spec and split policy",
       "missing active identity, stale contract, stale component, wrong graph "
       "order, or wrong source cursor satisfies a target",
       "identity binds proof comparability; runtime-loaded checkpoint paths do "
       "not alter contract identity",
       true,
       true,
       false,
       false,
       false},
      {"vicreg_train_core_ready",
       "completed normal train-core channel VICReg runtime evidence with "
       "checkpoint and observed-input coverage",
       {"vicreg_train_core_ready"},
       {"proof_certificate", "proof_certificate.coverage",
        "proof_certificate.closure", "proof_certificate_check",
        "representation_geometry_vocabulary",
        "representation_geometry_summary"},
       "target status is satisfied, proof certificate check passes, and the "
       "checkpoint belongs to the active VICReg component identity",
       "debug-only evidence, stale identity, missing checkpoint, missing "
       "coverage, unresolved closure, or malformed certificate is accepted",
       "VICReg readiness is shared representation evidence; per-node "
       "representation support remains future evidence, not inferred from MDN",
       true,
       true,
       false,
       false,
       false},
      {"channel_mdn_train_core_ready",
       "completed normal train-core MDN runtime evidence with target "
       "supervision coverage and exact representation checkpoint dependency",
       {"channel_mdn_train_core_ready"},
       {"proof_certificate", "proof_certificate.dependencies[]",
        "proof_certificate.coverage", "node_support_summaries",
        "proof_certificate_check"},
       "target status is satisfied, MDN coverage is credited from trusted "
       "completed evidence, and the loaded representation checkpoint matches "
       "VICReg lineage",
       "fresh or wrong representation checkpoint, untrusted requested range, "
       "stale identity, missing target coverage, or malformed certificate is "
       "accepted",
       "MDN node support is visibility/warning scope, not VICReg per-node "
       "readiness",
       true,
       true,
       false,
       false,
       false},
      {"train_core_no_validation_leakage",
       "complete MDN checkpoint closure checked against validation_holdout "
       "protected split",
       {"channel_mdn_train_core_no_validation_leakage"},
       {"proof_certificate.closure", "proof_certificate.leakage",
        "leakage_rule_vocabulary", "leakage_rule_summary",
        "proof_certificate_check"},
       "closure is complete and no forbidden exposure in the checkpoint "
       "lineage intersects the Hx/Hf-dilated validation protected range",
       "unchecked closure, unresolved checkpoint lineage, wrong protected "
       "range, or forbidden validation overlap passes",
       "leakage is set intersection after protected-split dilation over a "
       "labeled causal closure",
       true,
       true,
       false,
       false,
       false},
      {"train_core_no_test_leakage",
       "complete MDN checkpoint closure checked against test_holdout protected "
       "split",
       {"channel_mdn_train_core_no_test_leakage"},
       {"proof_certificate.closure", "proof_certificate.leakage",
        "leakage_rule_vocabulary", "selection_signal_policy_vocabulary",
        "selection_signal_policy_summary", "leakage_rule_summary",
        "proof_certificate_check"},
       "closure is complete and no forbidden observed_input, "
       "target_supervision, or selection_signal exposure intersects the "
       "Hx/Hf-dilated test protected range",
       "unchecked closure, unresolved checkpoint lineage, wrong protected "
       "range, forbidden test overlap, or known selection_signal leakage "
       "passes",
       "first-class selection-signal event streams remain future work, but the "
       "known use label is forbidden in V1",
       true,
       true,
       false,
       false,
       false},
      {"validation_eval_exact_checkpoint_binding",
       "run-mode validation_holdout evaluation evidence with zero optimizer "
       "mutation, no output checkpoint, exact loaded checkpoint bindings, "
       "and no extra checkpoint input edges",
       {"channel_mdn_validation_eval_ready"},
       {"proof_certificate.dependencies[]", "proof_certificate.coverage",
        "checkpoint_selection_policy_vocabulary",
        "checkpoint_selection_policy_summary",
        "validation_performance_evidence_policy_vocabulary",
        "validation_performance_evidence_policy_summary",
        "validation_performance_scope_policy_vocabulary",
        "validation_performance_scope_policy_summary",
        "proof_certificate_check"},
       "validation evaluation status is satisfied, evaluation_metric coverage "
       "is proven, optimizer mutation is absent, the exact MDN checkpoint from "
       "the no-test-leakage guard is loaded, and representation lineage "
       "matches; each evaluation witness has exactly those checkpoint input "
       "edges and no new checkpoint is written",
       "fresh MDN, wrong MDN checkpoint, wrong representation checkpoint, "
       "extra checkpoint input, training mutation, missing evaluation "
       "coverage, or performance-only evidence passes",
       "validation eval readiness is clean evaluation evidence, not model "
       "quality or deployability approval",
       true,
       true,
       false,
       false,
       false},
      {"warnings_visibility_nonblocking",
       "structured warning results and summaries for source health, repeated "
       "load, optimizer effort, representation health, and MDN calibration",
       {"vicreg_train_core_ready", "channel_mdn_train_core_ready",
        "channel_mdn_validation_eval_ready"},
       {"warning_results", "warning_summary",
        "warning_threshold_relation_vocabulary",
        "warning_anchor_scope_policy_vocabulary",
        "validation_performance_evidence_policy_vocabulary",
        "validation_performance_evidence_policy_summary",
        "performance_uncertainty_policy_vocabulary",
        "performance_uncertainty_policy_summary",
        "mdn_distribution_calibration_vocabulary",
        "mdn_distribution_calibration_summary",
        "mdn_distribution_calibration_diagnostic_vocabulary",
        "mdn_distribution_calibration_diagnostic_summary"},
       "warnings are visible with measurement availability, threshold "
       "relation, and summary counts while target status and plan attempts are "
       "unchanged by warnings alone",
       "warning text parsing changes status, warnings satisfy readiness, or "
       "unavailable warning measurements are treated as proof",
       "V1 warnings are non-blocking visibility; future performance gates need "
       "explicit metrics and uncertainty",
       true,
       true,
       false,
       false,
       false},
      {"mdn_node_support_visibility",
       "MDN node exposure facts summarized as node-by-use support, valid "
       "target uncertainty, weakest-node, and balance metrics",
       {"channel_mdn_train_core_ready", "channel_mdn_validation_eval_ready"},
       {"node_support_summaries", "node_support_scope_policy_vocabulary",
        "node_support_scope_policy_summary",
        "valid_target_uncertainty_vocabulary",
        "valid_target_uncertainty_summary"},
       "MDN support summaries expose routed/active/trained/evaluated rows, "
       "valid targets, Wilson support bounds, weakest node, and balance "
       "metrics",
       "missing node facts synthesize support, downstream MDN support is "
       "treated as VICReg per-node readiness, or node support becomes a hard "
       "performance gate in V1",
       "node support is MDN-head visibility in V1; VICReg node support facts "
       "are deferred",
       true,
       true,
       false,
       false,
       false},
      {"proof_receipt_and_hero_surface",
       "PASS receipt plus Hero status/list/explain/scan/evaluate/plan/closure "
       "surfaces over the same proof vocabulary",
       {"vicreg_train_core_ready", "channel_mdn_train_core_ready",
        "channel_mdn_train_core_no_validation_leakage",
        "channel_mdn_train_core_no_test_leakage",
        "channel_mdn_validation_eval_ready"},
       {"proof_certificate",
        "proof_certificate_check",
        "proof_certificate_digest_policy_vocabulary",
        "evidence_order_vector",
        "proof_certificate_digest_policy_summary",
        "plan_basis",
        "deficit_vector_planning_summary",
        "checkpoint_closure",
        "explain_target.operational_readiness_v1_scope_vocabulary",
        "explain_target.operational_readiness_v1_gate_vocabulary",
        "explain_target.operational_readiness_v1_gate_summary",
        "explain_target.mathematical_readiness_v1_vocabulary",
        "explain_target.proof_certificate_digest_policy_summary",
        "explain_target.deficit_vector_planning_summary",
        "explain_target.checkpoint_selection_policy_summary",
        "explain_target.node_support_scope_policy_summary",
        "explain_target.selection_signal_policy_summary",
        "explain_target.source_key_coordinate_policy_summary",
        "explain_target.source_receipt_policy_summary",
        "explain_target.valid_target_uncertainty_summary",
        "explain_target.validation_performance_evidence_policy_summary",
        "explain_target.validation_performance_scope_policy_summary",
        "explain_target.performance_uncertainty_policy_summary",
        "explain_target.mdn_distribution_calibration_summary",
        "explain_target.mdn_distribution_calibration_diagnostic_summary",
        "src/tests/fixtures/kikijyeba/lattice_operational_readiness_v1/"
        "PASS.md"},
       "agents can inspect target statuses, proof certificates, digest "
       "boundaries, deficits, warnings, closure completeness, and suggested "
       "waves without executing runtime work",
       "PASS depends on hidden state, lacks proof checks, requires Lattice "
       "Hero "
       "execution authority, or omits closure/warning/plan visibility",
       "the PASS artifact is an audit receipt for the local runtime evidence, "
       "not a portable runtime fixture",
       true,
       true,
       false,
       false,
       false}};
}

struct lattice_operational_readiness_v1_gate_summary_t {
  std::string schema{
      "kikijyeba.lattice.operational_readiness_v1.gate_summary.v1"};
  std::int64_t gate_count{0};
  std::int64_t required_for_v1_count{0};
  std::int64_t read_only_evidence_authority_count{0};
  std::int64_t runtime_executor_authority_count{0};
  std::int64_t performance_gate_authority_count{0};
  std::int64_t db_source_of_truth_authority_count{0};
  std::int64_t empty_hero_surface_count{0};
  std::int64_t empty_pass_condition_count{0};
  std::int64_t empty_fail_condition_count{0};
  std::int64_t unique_target_id_count{0};
  bool all_gates_required_for_v1{false};
  bool all_gates_read_only{false};
  bool no_runtime_executor_authority{false};
  bool no_performance_gate_authority{false};
  bool no_db_source_of_truth_authority{false};
  bool all_gates_have_hero_surface{false};
  bool all_gates_have_pass_fail_conditions{false};
  bool all_v1_targets_referenced{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> referenced_target_ids{};
  std::vector<std::string> missing_v1_target_ids{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_operational_readiness_v1_gate_summary_t
lattice_operational_readiness_v1_gate_summary() {
  lattice_operational_readiness_v1_gate_summary_t out{};
  const auto vocabulary = lattice_operational_readiness_v1_gate_vocabulary();
  out.gate_count = static_cast<std::int64_t>(vocabulary.size());
  std::set<std::string> referenced_target_ids;
  for (const auto &entry : vocabulary) {
    if (entry.required_for_v1) {
      ++out.required_for_v1_count;
    }
    if (entry.read_only_evidence_authority) {
      ++out.read_only_evidence_authority_count;
    }
    if (entry.runtime_executor_authority) {
      ++out.runtime_executor_authority_count;
    }
    if (entry.performance_gate_authority) {
      ++out.performance_gate_authority_count;
    }
    if (entry.db_source_of_truth_authority) {
      ++out.db_source_of_truth_authority_count;
    }
    if (entry.hero_surfaces.empty()) {
      ++out.empty_hero_surface_count;
    }
    if (entry.pass_condition.empty()) {
      ++out.empty_pass_condition_count;
    }
    if (entry.fail_condition.empty()) {
      ++out.empty_fail_condition_count;
    }
    referenced_target_ids.insert(entry.target_ids.begin(),
                                 entry.target_ids.end());
  }
  out.referenced_target_ids.assign(referenced_target_ids.begin(),
                                   referenced_target_ids.end());
  out.unique_target_id_count =
      static_cast<std::int64_t>(out.referenced_target_ids.size());

  const std::vector<std::string> required_targets{
      "vicreg_train_core_ready", "channel_mdn_train_core_ready",
      "channel_mdn_train_core_no_validation_leakage",
      "channel_mdn_train_core_no_test_leakage",
      "channel_mdn_validation_eval_ready"};
  for (const auto &target_id : required_targets) {
    if (referenced_target_ids.find(target_id) == referenced_target_ids.end()) {
      out.missing_v1_target_ids.push_back(target_id);
    }
  }

  out.all_gates_required_for_v1 =
      out.required_for_v1_count == out.gate_count && out.gate_count > 0;
  out.all_gates_read_only =
      out.read_only_evidence_authority_count == out.gate_count &&
      out.gate_count > 0;
  out.no_runtime_executor_authority = out.runtime_executor_authority_count == 0;
  out.no_performance_gate_authority = out.performance_gate_authority_count == 0;
  out.no_db_source_of_truth_authority =
      out.db_source_of_truth_authority_count == 0;
  out.all_gates_have_hero_surface = out.empty_hero_surface_count == 0;
  out.all_gates_have_pass_fail_conditions =
      out.empty_pass_condition_count == 0 &&
      out.empty_fail_condition_count == 0;
  out.all_v1_targets_referenced = out.missing_v1_target_ids.empty();

  if (out.gate_count != 10) {
    out.summary_issues.push_back(
        "Operational Readiness V1 gate vocabulary must contain 10 gates");
  }
  if (!out.all_gates_required_for_v1) {
    out.summary_issues.push_back(
        "all Operational Readiness V1 gates must be required");
  }
  if (!out.all_gates_read_only) {
    out.summary_issues.push_back(
        "all Operational Readiness V1 gates must be read-only evidence "
        "authority");
  }
  if (!out.no_runtime_executor_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 gates must not grant runtime execution");
  }
  if (!out.no_performance_gate_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 gates must not be performance gates");
  }
  if (!out.no_db_source_of_truth_authority) {
    out.summary_issues.push_back(
        "Operational Readiness V1 gates must not make DB rows source of truth");
  }
  if (!out.all_gates_have_hero_surface) {
    out.summary_issues.push_back(
        "every Operational Readiness V1 gate must name at least one Hero "
        "surface");
  }
  if (!out.all_gates_have_pass_fail_conditions) {
    out.summary_issues.push_back(
        "every Operational Readiness V1 gate must declare pass and fail "
        "conditions");
  }
  if (!out.all_v1_targets_referenced) {
    out.summary_issues.push_back("Operational Readiness V1 gates must "
                                 "reference every required V1 target");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_contract_identity_boundary_entry_t {
  std::string surface{};
  std::string category{};
  std::string participates_in{};
  std::string excluded_from{};
  std::string proof_authority{};
  std::string failure_policy{};
  bool protocol_contract_identity{false};
  bool target_spec_identity{false};
  bool active_runtime_identity{false};
  bool runtime_model_state{false};
  bool plan_advice{false};
  bool warning_visibility{false};
  bool audit_metadata{false};
};

struct lattice_mathematical_readiness_v1_entry_t {
  std::string item{};
  std::string mathematical_claim{};
  std::vector<std::string> hero_surfaces{};
  std::string v1_status{};
  std::string boundary{};
  std::string proof_basis{};
  std::string future_work{};
  bool included_in_v1{false};
  bool deferred_beyond_v1{false};
  bool read_only{true};
  bool runtime_executor{false};
  bool performance_gate{false};
  bool db_source_of_truth{false};
};

[[nodiscard]] inline std::vector<lattice_contract_identity_boundary_entry_t>
lattice_contract_identity_boundary_vocabulary() {
  return {
      {"protocol_contract_fingerprint", "protocol_contract_identity",
       "active identity and proof_certificate.proof_context contract match",
       "runtime model-state checkpoint paths, plan hints, warning clauses, and "
       "source receipts",
       "graph-anchor readiness evidence must match the active protocol "
       "contract fingerprint",
       "missing active contract blocks; mismatched evidence is stale_contract",
       true, false, true, false, false, false, false},
      {"graph_order_fingerprint|source_cursor_token", "active_runtime_identity",
       "row-index comparability, split overlap authority, and "
       "proof_certificate.proof_context graph-anchor match",
       "runtime model-state inputs, source receipts, and plan advice",
       "coverage and leakage proofs compare evidence only under the active "
       "graph order and source cursor token",
       "missing graph-anchor identity blocks; mismatched evidence is "
       "stale_contract",
       false, false, true, false, false, false, false},
      {"component_assembly_fingerprint", "active_runtime_identity",
       "component compatibility for VICReg and MDN proof obligations",
       "checkpoint file paths, plan hints, warning clauses, and audit receipts",
       "target evidence must match the active component assembly fingerprint "
       "when component matching is required",
       "mismatched evidence is stale_component; missing required identity "
       "blocks",
       false, false, true, false, false, false, false},
      {"target_spec_fingerprint", "target_proof_identity",
       "compiled target proof obligation and proof certificate digest",
       "advisory plan values, warning clauses/results, runtime checkpoint "
       "paths, and source receipts",
       "certificate checks bind status to the compiled proof obligation rather "
       "than to execution hints",
       "missing target_spec_fingerprint is a certificate issue", false, true,
       false, false, false, false, false},
      {"checkpoint_paths_and_loaded_model_state", "runtime_model_state",
       "checkpoint closure roots, exact loaded-checkpoint dependency proofs, "
       "and validation/evaluation binding",
       "protocol contract fingerprint, graph order fingerprint, source cursor "
       "token, component assembly fingerprints, and target_spec_fingerprint",
       "runtime reports, exposure facts, and checkpoint closure prove which "
       "MDN and representation checkpoints were loaded",
       "missing, unresolved, or mismatched loaded checkpoints block "
       "satisfaction",
       false, false, false, true, false, false, false},
      {"plan_inputs_and_wave_knobs", "plan_advice",
       "plan_basis, suggested_wave, and recommendation-attempt guard fields",
       "target satisfaction, proof evidence, protocol contract identity, and "
       "target_spec_fingerprint",
       "plan outputs may advise Runtime Hero which symbolic checkpoint-source "
       "hints or ranges to request next, but runtime reports must prove what "
       "happened",
       "plan advice cannot satisfy a target or mutate runtime state", false,
       false, false, false, true, false, false},
      {"warning_clauses_and_results", "warning_visibility",
       "non-blocking warning results, warning summary, and warning vocabulary",
       "target status, proof-obligation identity, protocol contract identity, "
       "and planning attempt accounting",
       "warnings expose suspicious evidence dimensions without changing target "
       "truth",
       "unavailable warning measurements are structured non-blocking results",
       false, false, false, false, false, true, false},
      {"source_receipts_and_key_windows", "audit_metadata",
       "source audit traceability and source-key coordinate previews",
       "row-index coverage authority, leakage authority, protocol contract "
       "identity, and target_spec_fingerprint",
       "row-index intervals remain coverage/leakage truth; receipts and key "
       "windows are audit coordinates",
       "malformed or missing audit metadata cannot prove readiness and must "
       "not "
       "split contracts",
       false, false, false, false, false, false, true}};
}

struct lattice_contract_identity_boundary_summary_t {
  std::string schema{"kikijyeba.lattice.contract_identity_boundary.summary.v1"};
  std::int64_t boundary_count{0};
  std::int64_t protocol_contract_identity_count{0};
  std::int64_t target_spec_identity_count{0};
  std::int64_t active_runtime_identity_count{0};
  std::int64_t runtime_model_state_count{0};
  std::int64_t plan_advice_count{0};
  std::int64_t warning_visibility_count{0};
  std::int64_t audit_metadata_count{0};
  std::int64_t runtime_model_state_identity_overlap_count{0};
  std::int64_t plan_advice_identity_overlap_count{0};
  std::int64_t warning_identity_overlap_count{0};
  std::int64_t audit_identity_overlap_count{0};
  std::int64_t empty_policy_field_count{0};
  bool model_state_is_not_contract_identity{false};
  bool advice_warning_audit_not_identity{false};
  bool active_identity_surfaces_present{false};
  bool proof_identity_surfaces_present{false};
  bool all_boundaries_have_policy_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> missing_required_surfaces{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_contract_identity_boundary_summary_t
lattice_contract_identity_boundary_summary() {
  lattice_contract_identity_boundary_summary_t out{};
  const auto vocabulary = lattice_contract_identity_boundary_vocabulary();
  out.boundary_count = static_cast<std::int64_t>(vocabulary.size());
  std::set<std::string> surfaces;
  for (const auto &entry : vocabulary) {
    surfaces.insert(entry.surface);
    if (entry.protocol_contract_identity) {
      ++out.protocol_contract_identity_count;
    }
    if (entry.target_spec_identity) {
      ++out.target_spec_identity_count;
    }
    if (entry.active_runtime_identity) {
      ++out.active_runtime_identity_count;
    }
    if (entry.runtime_model_state) {
      ++out.runtime_model_state_count;
      if (entry.protocol_contract_identity || entry.target_spec_identity) {
        ++out.runtime_model_state_identity_overlap_count;
      }
    }
    if (entry.plan_advice) {
      ++out.plan_advice_count;
      if (entry.protocol_contract_identity || entry.target_spec_identity) {
        ++out.plan_advice_identity_overlap_count;
      }
    }
    if (entry.warning_visibility) {
      ++out.warning_visibility_count;
      if (entry.protocol_contract_identity || entry.target_spec_identity) {
        ++out.warning_identity_overlap_count;
      }
    }
    if (entry.audit_metadata) {
      ++out.audit_metadata_count;
      if (entry.protocol_contract_identity || entry.target_spec_identity) {
        ++out.audit_identity_overlap_count;
      }
    }
    if (entry.participates_in.empty() || entry.excluded_from.empty() ||
        entry.proof_authority.empty() || entry.failure_policy.empty()) {
      ++out.empty_policy_field_count;
    }
  }

  const std::vector<std::string> required_surfaces{
      "protocol_contract_fingerprint",
      "graph_order_fingerprint|source_cursor_token",
      "component_assembly_fingerprint",
      "target_spec_fingerprint",
      "checkpoint_paths_and_loaded_model_state",
      "plan_inputs_and_wave_knobs",
      "warning_clauses_and_results",
      "source_receipts_and_key_windows"};
  for (const auto &surface : required_surfaces) {
    if (surfaces.find(surface) == surfaces.end()) {
      out.missing_required_surfaces.push_back(surface);
    }
  }

  out.model_state_is_not_contract_identity =
      out.runtime_model_state_count == 1 &&
      out.runtime_model_state_identity_overlap_count == 0;
  out.advice_warning_audit_not_identity =
      out.plan_advice_identity_overlap_count == 0 &&
      out.warning_identity_overlap_count == 0 &&
      out.audit_identity_overlap_count == 0;
  out.active_identity_surfaces_present =
      out.protocol_contract_identity_count == 1 &&
      out.active_runtime_identity_count == 3;
  out.proof_identity_surfaces_present = out.target_spec_identity_count == 1;
  out.all_boundaries_have_policy_text = out.empty_policy_field_count == 0;

  if (out.boundary_count != 8) {
    out.summary_issues.push_back(
        "contract identity boundary vocabulary must contain 8 boundaries");
  }
  if (!out.active_identity_surfaces_present) {
    out.summary_issues.push_back(
        "contract identity boundary must include protocol plus three active "
        "runtime identity surfaces");
  }
  if (!out.proof_identity_surfaces_present) {
    out.summary_issues.push_back(
        "contract identity boundary must include target proof identity");
  }
  if (!out.model_state_is_not_contract_identity) {
    out.summary_issues.push_back(
        "runtime model-state surfaces must not participate in protocol or "
        "target proof identity");
  }
  if (!out.advice_warning_audit_not_identity) {
    out.summary_issues.push_back(
        "plan advice, warning visibility, and audit metadata must not "
        "participate in protocol or target proof identity");
  }
  if (!out.all_boundaries_have_policy_text) {
    out.summary_issues.push_back(
        "contract identity boundary rows must include policy text fields");
  }
  if (!out.missing_required_surfaces.empty()) {
    out.summary_issues.push_back(
        "contract identity boundary vocabulary is missing required surfaces");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

[[nodiscard]] inline std::vector<lattice_mathematical_readiness_v1_entry_t>
lattice_mathematical_readiness_v1_vocabulary() {
  return {
      {"evidence_partial_order",
       "evidence is a product partial order with target satisfaction monotone "
       "only under clean evidence growth",
       {"product_evidence_state_vocabulary",
        "explain_target.product_evidence_state_vocabulary",
        "monotonicity_invariant_vocabulary",
        "explain_target.monotonicity_invariant_vocabulary",
        "join_law_vocabulary", "explain_target.join_law_vocabulary",
        "evidence_order_vector"},
       "implemented_read_side",
       "forbidden overlaps, stale identity, triggered warnings, and Pareto "
       "tradeoffs are weakening or incomparable dimensions, not clean growth",
       "product factors, monotonicity invariants, join laws, and Pareto vector",
       "future cache/index must preserve the same product order and identity "
       "scope",
       true,
       false,
       true,
       false,
       false,
       false},
      {"coverage_load_dual_algebra",
       "unique coverage is idempotent interval union while repeated exposure "
       "load is additive over distinct completed facts",
       {"exposure_measure_algebra_vocabulary",
        "exposure_measure_algebra_summary",
        "explain_target.exposure_measure_algebra_vocabulary",
        "explain_target.exposure_measure_algebra_summary",
        "proof_certificate.coverage[]", "exposure_summaries",
        "join_law_vocabulary", "explain_target.join_law_vocabulary"},
       "implemented_read_side",
       "coverage can satisfy readiness; load is visibility/warning evidence",
       "coverage proof intervals, merged union/complement, and load summaries",
       "future indexes must retain interval/fact identity before merging",
       true,
       false,
       true,
       false,
       false,
       false},
      {"protected_split_dilation",
       "holdout leakage is forbidden-use intersection after temporal "
       "protected-set dilation",
       {"leakage_rule_vocabulary", "explain_target.leakage_rule_vocabulary",
        "leakage_rule_summary", "explain_target.leakage_rule_summary",
        "proof_certificate.leakage",
        "proof_certificate.leakage.overlap_witnesses[]"},
       "implemented_read_side",
       "unchecked or incomplete closure cannot prove the empty witness set",
       "base range, protected range, dilation_left/right, forbidden uses, and "
       "witness recomputation",
       "future embargo variants can extend the dilation kernel explicitly",
       true,
       false,
       true,
       false,
       false,
       false},
      {"checkpoint_causal_graph",
       "checkpoint lineage is labeled causal reachability over source-row "
       "uses, loaded checkpoints, created checkpoints, and mutations",
       {"causal_edge_vocabulary", "explain_target.causal_edge_vocabulary",
        "causal_edge_summary", "explain_target.causal_edge_summary",
        "selection_signal_policy_vocabulary",
        "explain_target.selection_signal_policy_vocabulary",
        "selection_signal_policy_summary",
        "explain_target.selection_signal_policy_summary",
        "proof_certificate.closure.causal_exposures[]",
        "checkpoint_identity_policy_vocabulary",
        "explain_target.checkpoint_identity_policy_vocabulary"},
       "implemented_read_side",
       "path-based exposure closure is V1 authority; checkpoint ids/digests "
       "remain preview/cache metadata",
       "closure fact digests, causal exposures, checkpoint input/output edges, "
       "and unresolved-input checks",
       "v2 may promote checkpoint_id/file_digest after rebuildable closure "
       "checks",
       true,
       false,
       true,
       false,
       false,
       false},
      {"proof_certificates",
       "target status has a compact machine-checkable proof envelope with a "
       "deterministic certificate digest",
       {"proof_certificate", "proof_certificate_check",
        "proof_obligation_vocabulary",
        "proof_certificate_digest_policy_vocabulary",
        "proof_certificate_digest_policy_summary",
        "explain_target.proof_obligation_vocabulary",
        "explain_target.proof_certificate_digest_policy_vocabulary",
        "explain_target.proof_certificate_digest_policy_summary"},
       "implemented_read_side",
       "a passing certificate check proves local consistency, not target "
       "satisfaction when the target status is non-satisfied",
       "schema, digest, identity, dependencies, coverage, closure, leakage, "
       "node support, and certificate self-check",
       "future certificates may add stronger performance and selector proofs",
       true,
       false,
       true,
       false,
       false,
       false},
      {"abstract_interpretation",
       "runtime files/facts abstract into conservative proof summaries that "
       "future caches may rebuild",
       {"evidence_abstraction_vocabulary",
        "explain_target.evidence_abstraction_vocabulary",
        "db_cache_policy_vocabulary",
        "explain_target.db_cache_policy_vocabulary",
        "derived_query_rule_vocabulary",
        "explain_target.derived_query_rule_vocabulary"},
       "implemented_read_side",
       "runtime files remain source of truth; DB/cache rows are read models, "
       "not evidence writers or executors",
       "concrete inputs, abstract outputs, soundness conditions, conservative "
       "policies, and cache boundaries",
       "future DB/index can materialize derived relations only as rebuildable "
       "cache",
       true,
       false,
       true,
       false,
       false,
       false},
      {"dsl_dimensional_analysis",
       "target and warning numeric fields carry units, bounds, integrality, "
       "and bad-direction constraints",
       {"target_numeric_dimension_vocabulary",
        "target_numeric_dimension_summary",
        "explain_target.target_numeric_dimension_vocabulary",
        "explain_target.target_numeric_dimension_summary"},
       "implemented_read_side",
       "dimension checks reject authored nonsense but do not introduce new "
       "runtime behavior",
       "compiler-side dimension validation, Hero-visible dimension table, and "
       "summary self-check",
       "future clause types should extend the table before accepting new units",
       true,
       false,
       true,
       false,
       false,
       false},
      {"node_support_matrix",
       "MDN node evidence is a node-by-use support matrix with balance and "
       "Wilson visibility",
       {"node_support_summaries", "node_support_scope_policy_vocabulary",
        "explain_target.node_support_scope_policy_vocabulary",
        "node_support_scope_policy_summary",
        "explain_target.node_support_scope_policy_summary",
        "warning_anchor_scope_policy_vocabulary",
        "explain_target.warning_anchor_scope_policy_vocabulary",
        "join_law_vocabulary", "explain_target.join_law_vocabulary"},
       "implemented_visibility",
       "MDN node rows do not become VICReg per-node readiness and synthetic "
       "backfill is forbidden",
       "node support summaries, weakest-node metrics, balance metrics, and "
       "MDN-only scope policy",
       "future NodeLift or representation-emitted node-support facts are "
       "needed for shared encoder node-local claims",
       true,
       false,
       true,
       false,
       false,
       false},
      {"representation_geometry",
       "VICReg representation health exposes loss, projection, "
       "finite-parameter, "
       "and embedding-spectrum geometry metrics",
       {"representation_geometry_vocabulary",
        "explain_target.representation_geometry_vocabulary",
        "representation_geometry_summary",
        "explain_target.representation_geometry_summary", "warning_results"},
       "implemented_visibility",
       "geometry metrics are V1 non-blocking warnings, not performance or "
       "geometry readiness gates",
       "representation_health facts, units, threshold directions, and warning "
       "scope",
       "future targets may promote selected metrics with explicit gate clauses",
       true,
       false,
       true,
       false,
       false,
       false},
      {"performance_uncertainty",
       "future performance gates must compare conservative uncertainty bounds "
       "under declared methods, not raw point estimates",
       {"valid_target_uncertainty_vocabulary",
        "explain_target.valid_target_uncertainty_vocabulary",
        "valid_target_uncertainty_summary",
        "explain_target.valid_target_uncertainty_summary",
        "performance_uncertainty_policy_vocabulary",
        "explain_target.performance_uncertainty_policy_vocabulary",
        "performance_uncertainty_policy_summary",
        "explain_target.performance_uncertainty_policy_summary",
        "validation_performance_evidence_policy_vocabulary",
        "explain_target.validation_performance_evidence_policy_vocabulary",
        "validation_performance_evidence_policy_summary",
        "explain_target.validation_performance_evidence_policy_summary",
        "validation_performance_scope_policy_vocabulary",
        "explain_target.validation_performance_scope_policy_vocabulary",
        "validation_performance_scope_policy_summary",
        "explain_target.validation_performance_scope_policy_summary"},
       "visibility_now_future_gate_policy",
       "V1 support intervals are visibility only; model quality remains out of "
       "scope",
       "Wilson support intervals and future bootstrap/standard-error policy",
       "future performance targets need metric samples, uncertainty summaries, "
       "thresholds, and selection-leakage policy",
       true,
       false,
       true,
       false,
       false,
       false},
      {"mdn_distribution_calibration",
       "MDN distributional quality is represented as proper-scoring and "
       "calibration metrics before hard gates exist",
       {"mdn_distribution_calibration_vocabulary",
        "explain_target.mdn_distribution_calibration_vocabulary",
        "mdn_distribution_calibration_summary",
        "explain_target.mdn_distribution_calibration_summary",
        "mdn_distribution_calibration_diagnostic_vocabulary",
        "explain_target.mdn_distribution_calibration_diagnostic_vocabulary",
        "mdn_distribution_calibration_diagnostic_summary",
        "explain_target.mdn_distribution_calibration_diagnostic_summary",
        "warning_results"},
       "implemented_diagnostics",
       "V3-D can warn on aggregate, channel, target-feature, "
       "channel/target-feature, and per-node NLL point estimates while PIT, "
       "interval coverage, tail coverage, and calibration slope remain future "
       "non-gating metrics",
       "metric family vocabulary, diagnostic binding summary, and "
       "non-blocking warning results",
       "future runtime reports must emit calibration samples and uncertainty",
       true,
       false,
       true,
       false,
       false,
       false},
      {"deficit_vector_planning",
       "planning is a deterministic projection of proof deficits, not a "
       "scheduler or second proof source",
       {"deficits", "plan_basis", "deficit_priority_vocabulary",
        "deficit_vector_planning_summary",
        "explain_target.deficit_priority_vocabulary",
        "explain_target.deficit_vector_planning_summary",
        "plan_advice_scope_policy_vocabulary",
        "explain_target.plan_advice_scope_policy_vocabulary"},
       "implemented_read_side",
       "plans cannot execute waves, mutate runtime state, or satisfy targets",
       "stable deficit keys, priorities, missing intervals, and suggested-wave "
       "projection",
       "future planners may add richer advice while preserving Runtime Hero as "
       "executor",
       true,
       false,
       true,
       false,
       false,
       false},
      {"pareto_evidence_order",
       "evidence comparison is Pareto partial order over declared dimensions, "
       "not one scalar score",
       {"evidence_order_vector", "evidence_order_dimension_vocabulary",
        "explain_target.evidence_order_dimension_vocabulary",
        "evidence_order_relation_vocabulary",
        "explain_target.evidence_order_relation_vocabulary",
        "checkpoint_selection_policy_vocabulary",
        "checkpoint_selection_policy_summary",
        "explain_target.checkpoint_selection_policy_vocabulary",
        "explain_target.checkpoint_selection_policy_summary"},
       "implemented_read_side",
       "latest_satisfying remains deterministic readiness selection, not "
       "best-model, Pareto, performance, deployment, or scalar ranking",
       "evidence order dimensions, polarity, relation vocabulary, and selector "
       "scope policy",
       "future best_nondominated selectors must report vector and tie policy",
       true,
       false,
       true,
       false,
       false,
       false},
      {"datalog_derived_queries",
       "target evaluation relations can be named as read-only Datalog-style "
       "derived rules",
       {"derived_query_rule_vocabulary",
        "derived_query_projection_semantics_vocabulary",
        "explain_target.derived_query_rule_vocabulary",
        "explain_target.derived_query_projection_semantics_vocabulary",
        "derived_query_results"},
       "implemented_read_side",
       "derived rules are query/cache metadata and never execute runtime work",
       "identity, dependency, coverage, closure, leakage, warning, plan, and "
       "satisfaction rules plus read-only relation truth projections",
       "future DB/index may materialize these relations as rebuildable cache",
       true,
       false,
       true,
       false,
       false,
       false},
      {"source_key_coordinate_map",
       "source-key windows are audit coordinates over row-index truth with "
       "monotone/affine consistency checks",
       {"source_key_coordinate_policy_vocabulary", "source_key_window_audit",
        "explain_target.source_key_coordinate_policy_vocabulary",
        "source_key_coordinate_policy_summary",
        "explain_target.source_key_coordinate_policy_summary",
        "source_receipt_policy_vocabulary",
        "explain_target.source_receipt_policy_vocabulary",
        "source_receipt_policy_summary",
        "explain_target.source_receipt_policy_summary"},
       "implemented_audit_visibility",
       "row-index intervals remain coverage/leakage authority; source-key "
       "windows and receipts are audit metadata",
       "structured source-key audits, issue counts, affine checks, and receipt "
       "scope policy",
       "structured source receipt facts are available as audit-only rows; "
       "future DB/index work may materialize them as rebuildable cache",
       true,
       false,
       true,
       false,
       false,
       false},
      {"contract_identity_boundary",
       "runtime model-state inputs prove loaded state but do not split "
       "protocol or target proof identity",
       {"contract_identity_boundary_vocabulary",
        "explain_target.contract_identity_boundary_vocabulary",
        "checkpoint_identity_policy_vocabulary",
        "explain_target.checkpoint_identity_policy_vocabulary",
        "checkpoint_selection_policy_vocabulary",
        "checkpoint_selection_policy_summary",
        "explain_target.checkpoint_selection_policy_vocabulary",
        "explain_target.checkpoint_selection_policy_summary"},
       "implemented_read_side",
       "checkpoint paths and plan inputs are runtime state/advice, not "
       "contract identity",
       "identity boundary table, exact loaded checkpoint bindings, and "
       "path-based V1 closure",
       "future checkpoint-id promotion can strengthen binding without changing "
       "protocol contract identity",
       true,
       false,
       true,
       false,
       false,
       false}};
}

struct lattice_mathematical_readiness_v1_summary_t {
  std::string schema{"kikijyeba.lattice.mathematical_readiness_v1.summary.v1"};
  std::int64_t item_count{0};
  std::int64_t included_in_v1_count{0};
  std::int64_t deferred_beyond_v1_count{0};
  std::int64_t read_only_count{0};
  std::int64_t runtime_executor_count{0};
  std::int64_t performance_gate_count{0};
  std::int64_t db_source_of_truth_count{0};
  std::int64_t empty_hero_surface_count{0};
  bool all_items_included_in_v1{false};
  bool no_items_deferred_beyond_v1{false};
  bool all_items_read_only{false};
  bool no_runtime_executor{false};
  bool no_performance_gate{false};
  bool no_db_source_of_truth{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_mathematical_readiness_v1_summary_t
lattice_mathematical_readiness_v1_summary() {
  lattice_mathematical_readiness_v1_summary_t out{};
  const auto vocabulary = lattice_mathematical_readiness_v1_vocabulary();
  out.item_count = static_cast<std::int64_t>(vocabulary.size());
  for (const auto &entry : vocabulary) {
    if (entry.included_in_v1) {
      ++out.included_in_v1_count;
    }
    if (entry.deferred_beyond_v1) {
      ++out.deferred_beyond_v1_count;
    }
    if (entry.read_only) {
      ++out.read_only_count;
    }
    if (entry.runtime_executor) {
      ++out.runtime_executor_count;
    }
    if (entry.performance_gate) {
      ++out.performance_gate_count;
    }
    if (entry.db_source_of_truth) {
      ++out.db_source_of_truth_count;
    }
    if (entry.hero_surfaces.empty()) {
      ++out.empty_hero_surface_count;
    }
  }

  out.all_items_included_in_v1 =
      out.included_in_v1_count == out.item_count && out.item_count > 0;
  out.no_items_deferred_beyond_v1 = out.deferred_beyond_v1_count == 0;
  out.all_items_read_only =
      out.read_only_count == out.item_count && out.item_count > 0;
  out.no_runtime_executor = out.runtime_executor_count == 0;
  out.no_performance_gate = out.performance_gate_count == 0;
  out.no_db_source_of_truth = out.db_source_of_truth_count == 0;

  if (out.item_count != 16) {
    out.summary_issues.push_back(
        "mathematical readiness vocabulary must contain 16 V1 items");
  }
  if (!out.all_items_included_in_v1) {
    out.summary_issues.push_back(
        "all mathematical readiness items must be included in V1");
  }
  if (!out.no_items_deferred_beyond_v1) {
    out.summary_issues.push_back(
        "mathematical readiness summary must not include deferred V2 items");
  }
  if (!out.all_items_read_only) {
    out.summary_issues.push_back(
        "all mathematical readiness items must be read-only");
  }
  if (!out.no_runtime_executor) {
    out.summary_issues.push_back(
        "mathematical readiness items must not grant runtime execution");
  }
  if (!out.no_performance_gate) {
    out.summary_issues.push_back(
        "mathematical readiness items must not be performance gates");
  }
  if (!out.no_db_source_of_truth) {
    out.summary_issues.push_back(
        "mathematical readiness items must not make DB rows source of truth");
  }
  if (out.empty_hero_surface_count != 0) {
    out.summary_issues.push_back("every mathematical readiness item must name "
                                 "at least one Hero surface");
  }
  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

struct lattice_target_proof_certificate_t {
  std::string schema{"kikijyeba.lattice.target_certificate.v1"};
  std::string certificate_digest{};
  std::string target_id{};
  std::string target_spec_fingerprint{};
  std::string split_policy_fingerprint{};

  struct proof_context_t {
    bool require_contract_match{false};
    bool require_component_match{false};
    bool require_graph_anchor_identity{false};
    std::string active_contract_fingerprint{};
    std::string evidence_contract_fingerprint{};
    bool contract_match{false};
    std::string expected_component_fingerprint{};
    std::string evidence_component_fingerprint{};
    bool component_match{false};
    std::string active_graph_order_fingerprint{};
    std::string evidence_graph_order_fingerprint{};
    bool graph_order_match{false};
    std::string active_source_cursor_token{};
    std::string evidence_source_cursor_token{};
    bool source_cursor_match{false};
    std::string component_spawn_schema{"kikijyeba.component_spawn.v2"};
    std::string component_family_id{};
    std::string target_spec_fingerprint{};
    std::string split_policy_fingerprint{};
    std::string config_bundle_id{};
    std::string config_receipt_id{};
    std::string component_spawn_registry_id{};
    std::string component_spawn_id{};
    std::string component_spawn_label{};
    std::string evidence_component_spawn_fingerprint{};
    std::string computed_component_spawn_fingerprint{};
    bool config_receipt_available{false};
    bool spawn_id_readability_hint_only{true};
    std::vector<std::string> provenance_warnings{};
  };

  struct dependency_proof_t {
    std::string kind{};
    std::string source_target_id{};
    std::string status{};
    std::filesystem::path expected_mdn_checkpoint{};
    std::filesystem::path actual_mdn_checkpoint{};
    bool mdn_checkpoint_match{false};
    std::filesystem::path expected_representation_checkpoint{};
    std::filesystem::path actual_representation_checkpoint{};
    bool representation_checkpoint_match{false};
  };

  struct artifact_proof_t {
    std::string proof_kind{};
    bool proof_template_bound{false};
    std::string proof_template_claim{};
    std::string fact_family{};
    std::string fact_schema{};
    std::string fact_type{};
    std::string fact_digest{};
    std::string fact_identity_contract_schema{};
    std::string fact_identity_contract_id{};
    bool fact_identity_contract_bound{false};
    bool fact_identity_envelope_complete{false};
    bool row_index_interval_authority{false};
    bool source_key_window_audit_only{true};
    bool fact_identity_target_kind_authority{false};
    bool fact_identity_runtime_wave_authority{false};
    bool fact_identity_marshal_reachability{false};
    bool fact_identity_policy_gate_authority{false};
    std::string parent_exposure_fact_digest{};
    std::string job_id{};
    std::string wave_id{};
    std::string protocol_id{};
    std::string contract_fingerprint{};
    std::string component{};
    std::string component_assembly_fingerprint{};
    std::string graph_order_fingerprint{};
    std::string source_cursor_token{};
    std::string split_policy_fingerprint{};
    std::string split_name{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t anchor_range{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t
        completed_anchor_range{};
    bool identity_match{false};
    bool artifact_evidence{false};
    bool deterministic_artifact{false};
    bool visibility_only{false};
    bool authority_clean{false};
    bool readiness_authority{false};
    bool quality_authority{false};
    bool performance_authority{false};
    bool checkpoint_selector{false};
    bool coverage_authority{false};
    bool leakage_authority{false};
    bool contract_identity_authority{false};
    bool allocation_authority{false};
    bool execution_authority{false};
    bool market_readiness_authority{false};
    bool deployment_authority{false};
    bool policy_gate{false};
    bool target_dependency_authority{false};
    bool runtime_wave_authority{false};
    bool marshal_reachability{false};
    bool checkpoint_source_authority{false};
    bool plan_checkpoint_input_authority{false};
    bool model_state_mutation{false};
    bool raw_potential_tradable_return{false};
    bool replay_executor{false};
    bool lineage_bound{false};
    bool passed{false};
    std::vector<std::string> issues{};
  };

  struct coverage_proof_t {
    std::string use{};
    std::string algebra{
        cuwacunu::hero::lattice::exposure::k_unique_coverage_algebra};
    cuwacunu::hero::lattice::exposure::anchor_interval_t target_range{};
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        contributing_intervals{};
    std::vector<std::string> contributing_fact_digests{};
    std::vector<cuwacunu::hero::lattice::exposure::lattice_exposure_fact_t>
        contributing_facts{};
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        covered_intervals{};
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        missing_intervals{};
    double required_fraction{0.0};
    double actual_fraction{0.0};
    std::int64_t covered_anchors{0};
    std::int64_t missing_anchors{0};
    bool require_mutated_component{true};
    bool passed{false};
    cuwacunu::hero::lattice::exposure::exposure_load_summary_t
        load_summary{};
  };

  struct closure_proof_t {
    struct causal_exposure_t {
      std::string fact_digest{};
      std::string contract_fingerprint{};
      std::string graph_order_fingerprint{};
      std::string source_cursor_token{};
      std::string cursor_domain{};
      std::string component_assembly_fingerprint{};
      std::string split_policy_fingerprint{};
      std::string job_id{};
      std::string wave_id{};
      std::string wave_action{};
      std::string job_status{};
      std::string runtime_handoff_id{};
      std::string runtime_handoff_digest{};
      std::string marshal_target_driver_run_id{};
      std::string target_component_family_id{};
      std::string representation_architecture{};
      std::string representation_contract{};
      std::string representation_value_shape{};
      std::string representation_sequence_value_shape{};
      std::string channel_axis_policy{};
      std::string temporal_reduction{};
      std::string input_representation_assembly_id{};
      std::string context_mode{};
      std::string context_contract{};
      std::string context_value_shape{};
      std::string output_contract{};
      std::string output_value_shape{};
      std::vector<std::string> uses{};
      bool mutated_component{false};
      std::string split_name{};
      cuwacunu::hero::lattice::exposure::exposure_split_role_t split_role{
          cuwacunu::hero::lattice::exposure::exposure_split_role_t::
              unknown};
      cuwacunu::hero::lattice::exposure::anchor_interval_t anchor_range{};
      cuwacunu::hero::lattice::exposure::anchor_interval_t
          completed_anchor_range{};
      cuwacunu::hero::lattice::exposure::anchor_interval_t
          observed_footprint{};
      cuwacunu::hero::lattice::exposure::anchor_interval_t
          target_footprint{};
      std::string first_anchor_key{};
      std::string last_anchor_key{};
      std::string observed_source_key_begin{};
      std::string observed_source_key_end{};
      std::string target_source_key_begin{};
      std::string target_source_key_end{};
      std::string source_key_footprint_precision{};
      cuwacunu::hero::lattice::exposure::source_key_window_audit_t
          source_key_window_audit{};
      std::filesystem::path output_checkpoint{};
      std::vector<std::filesystem::path> input_checkpoints{};
    };

    struct checkpoint_identity_preview_t {
      std::filesystem::path checkpoint_path{};
      std::string checkpoint_id{};
      std::string checkpoint_file_digest{};
      std::string component{};
      std::string component_assembly_fingerprint{};
      std::string representation_architecture{};
      std::string representation_contract{};
      std::string representation_value_shape{};
      std::string representation_sequence_value_shape{};
      std::string channel_axis_policy{};
      std::string temporal_reduction{};
      std::string input_representation_assembly_id{};
      std::string context_mode{};
      std::string context_contract{};
      std::string context_value_shape{};
      std::string output_contract{};
      std::string output_value_shape{};
      std::string direct_exposure_digest{};
      std::string created_by_job_id{};
      std::string created_by_wave_id{};
    };

    bool checked{false};
    std::filesystem::path checkpoint_path{};
    bool complete{false};
    std::int64_t fact_count{0};
    std::string resolution_authority{};
    std::string root_checkpoint_id{};
    std::string root_checkpoint_file_digest{};
    std::vector<std::string> identity_mismatches{};
    std::vector<std::filesystem::path> unresolved_input_checkpoints{};
    std::vector<std::string> fact_digests{};
    std::vector<causal_exposure_t> causal_exposures{};
    std::vector<checkpoint_identity_preview_t> checkpoint_identity_previews{};
  };

  struct leakage_proof_t {
    bool checked{false};
    std::string rule{k_lattice_leakage_rule};
    std::string split_name{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t base_range{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t protected_range{};
    std::int64_t dilation_left{0};
    std::int64_t dilation_right{0};
    std::vector<std::string> forbidden_uses{};
    bool require_mutated_component{true};
    bool overlap_found{false};
    std::vector<
        cuwacunu::hero::lattice::exposure::forbidden_exposure_overlap_t>
        overlap_witnesses{};
    bool passed{true};
  };

  proof_context_t proof_context{};
  std::vector<dependency_proof_t> dependencies{};
  std::vector<artifact_proof_t> artifacts{};
  std::vector<coverage_proof_t> coverage{};
  closure_proof_t closure{};
  leakage_proof_t leakage{};
  std::vector<cuwacunu::hero::lattice::exposure::node_support_summary_t>
      node_support_summaries{};
};

struct lattice_target_proof_certificate_check_t {
  bool passed{true};
  std::vector<std::string> issues{};
};

struct lattice_target_evaluation_t {
  std::string target_id{};
  lattice_target_kind_t kind{lattice_target_kind_t::not_applicable};
  bool target_kind_applicable{true};
  std::string target_class{};
  std::string proof_kind{};
  std::string subject_fact_family{};
  std::string component{};
  std::string split_policy_fingerprint{};
  lattice_target_status_t status{lattice_target_status_t::missing_report};
  std::vector<std::string> reasons{};
  bool plan_ready{false};
  lattice_suggested_wave_t suggested_wave{};
  lattice_target_evidence_t evidence{};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_load_summary_t>
      exposure_summaries{};
  std::vector<cuwacunu::hero::lattice::exposure::node_support_summary_t>
      node_support_summaries{};
  std::vector<std::string> warnings{};
  struct warning_result_t {
    std::string warning_id{};
    std::string target_id{};
    std::string kind{};
    std::string warning_family{};
    std::string status{"clear"};
    std::string severity{"info"};
    std::string source{"lattice"};
    std::string component{"lattice_target"};
    bool blocking{false};
    bool threshold_triggered{false};
    std::string threshold_relation{"unavailable"};
    double measured_value{std::numeric_limits<double>::quiet_NaN()};
    double threshold{std::numeric_limits<double>::quiet_NaN()};
    std::string threshold_direction{};
    std::string unit{};
    std::string split{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t
        warning_anchor_range{};
    std::string use{};
    std::string scope{};
    std::string effect{};
    std::string metric{};
    std::string evidence_basis{"none"};
    std::string evidence_digest{};
    std::string fact_family{};
    std::string fact_digest{};
    std::vector<std::string> target_ids_observed_against{};
    std::string machine_reason_code{};
    std::string human_explanation{};
    std::string suggested_inspection_panel{};
    std::string readiness_effect{"non_blocking_warning_only"};
    std::string message{};
    bool measurement_available{false};
    std::string diagnostic_metric_family{};
    std::string diagnostic_uncertainty_method{};
    std::int64_t diagnostic_sample_count{0};
    std::filesystem::path diagnostic_evaluated_mdn_checkpoint{};
    std::filesystem::path diagnostic_representation_checkpoint{};
    bool diagnostic_exact_checkpoint_binding_proven{false};
    bool diagnostic_mdn_checkpoint_loaded{false};
    bool diagnostic_representation_checkpoint_loaded{false};
    std::string diagnostic_split_policy_fingerprint{};
    std::string diagnostic_active_contract_fingerprint{};
    std::string diagnostic_active_graph_order_fingerprint{};
    std::string diagnostic_active_source_cursor_token{};
    std::string diagnostic_validation_split{};
    bool diagnostic_validation_split_bound{false};
    std::string diagnostic_readiness_effect{};
    bool exposure_summary_available{false};
    bool node_support_summary_available{false};
    cuwacunu::hero::lattice::exposure::exposure_load_summary_t
        exposure_summary{};
    cuwacunu::hero::lattice::exposure::node_support_summary_t
        node_support_summary{};
  };
  std::vector<warning_result_t> warning_results{};
  struct warning_summary_t {
    std::int64_t warning_result_count{0};
    std::int64_t triggered_warning_count{0};
    std::int64_t unavailable_warning_count{0};
    std::int64_t clear_measured_warning_count{0};
    std::int64_t warning_count{0};
    std::int64_t blocking_warning_count{0};
    std::int64_t non_blocking_warning_count{0};
    bool all_warnings_non_blocking{true};
    std::int64_t above_threshold_count{0};
    std::int64_t not_above_threshold_count{0};
    std::int64_t below_threshold_count{0};
    std::int64_t not_below_threshold_count{0};
    std::int64_t unavailable_relation_count{0};
    std::int64_t no_threshold_relation_count{0};
    std::int64_t unknown_direction_relation_count{0};
  };
  warning_summary_t warning_summary{};
  struct representation_geometry_gate_result_t {
    std::string requirement_id{};
    std::string metric{};
    std::string op{};
    double threshold{std::numeric_limits<double>::quiet_NaN()};
    double measured_value{std::numeric_limits<double>::quiet_NaN()};
    std::string unit{};
    std::int64_t fact_count{0};
    bool measurement_available{false};
    bool passed{false};
    std::string status{"unchecked"};
    std::string message{};
  };
  std::vector<representation_geometry_gate_result_t>
      representation_geometry_gate_results{};
  bool fact_integrity_summary_available{false};
  cuwacunu::hero::lattice::exposure::lattice_fact_integrity_summary_t
      fact_integrity_summary{};
  lattice_target_proof_certificate_t proof_certificate{};
  lattice_target_proof_certificate_check_t proof_certificate_check{};
  struct proof_deficit_t {
    std::string key{};
    std::string kind{};
    std::string dimension{};
    std::string status{};
    std::int64_t priority{1000};
    std::string priority_class{"fallback"};
    std::string message{};
    double required{std::numeric_limits<double>::quiet_NaN()};
    double actual{std::numeric_limits<double>::quiet_NaN()};
    std::string unit{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t range{};
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        missing_intervals{};
    bool plan_relevant{true};
    std::vector<std::string> related_fact_integrity_issue_codes{};
  };
  std::vector<proof_deficit_t> deficits{};
  struct evidence_order_vector_t {
    std::string order{"pareto_partial_order"};
    bool available{false};
    bool satisfied{false};
    bool proof_certificate_check_passed{false};
    bool identity_contract_match{false};
    bool identity_component_match{false};
    bool identity_graph_order_match{false};
    bool identity_source_cursor_match{false};
    double min_unique_coverage_fraction{
        std::numeric_limits<double>::quiet_NaN()};
    double max_cursor_exposure_load{std::numeric_limits<double>::quiet_NaN()};
    bool closure_checked{false};
    bool closure_complete{false};
    bool leakage_checked{false};
    bool leakage_passed{false};
    bool selection_signal_leakage_found{false};
    double aggregate_valid_target_wilson_lower_95{
        std::numeric_limits<double>::quiet_NaN()};
    double weakest_node_valid_target_wilson_lower_95{
        std::numeric_limits<double>::quiet_NaN()};
    double max_node_support_coefficient_of_variation{
        std::numeric_limits<double>::quiet_NaN()};
    double max_node_support_gini{std::numeric_limits<double>::quiet_NaN()};
    std::int64_t source_key_audit_count{0};
    std::int64_t source_key_audit_issue_count{0};
    std::int64_t source_key_affine_inconsistent_count{0};
    std::int64_t blocking_deficit_count{0};
    std::int64_t unresolved_lineage_count{0};
    std::int64_t warning_result_count{0};
    std::int64_t triggered_warning_count{0};
    std::int64_t unavailable_warning_count{0};
    std::int64_t warning_count{0};
  };
  evidence_order_vector_t evidence_order_vector{};
  struct plan_basis_t {
    struct fact_preview_hint_t {
      std::string tool{"hero.lattice.inspect"};
      std::string marshal_tool{"hero.marshal.inspect"};
      std::string fact_family{};
      std::string fact_digest{};
      bool include_preview{true};
      bool include_lineage{true};
      bool read_only{true};
      bool target_proof{false};
      bool dispatchable{false};
      bool facts_used_for_target_satisfaction{false};
      bool checkpoint_selected{false};
      bool model_selector{false};
    };
    bool available{false};
    std::string reason{};
    std::string primary_deficit_key{};
    std::string primary_deficit_message{};
    std::int64_t primary_deficit_priority{1000};
    std::string primary_deficit_priority_class{"fallback"};
    std::vector<std::string> deficit_keys{};
    std::vector<std::string> deficit_messages{};
    std::vector<std::string> deficit_priority_classes{};
    cuwacunu::hero::lattice::exposure::anchor_interval_t target_range{};
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        missing_intervals{};
    std::string suggested_action{};
    std::vector<fact_preview_hint_t> fact_preview_hints{};
  };
  plan_basis_t plan_basis{};
};

struct lattice_deficit_priority_entry_t {
  std::string priority_class{};
  std::int64_t priority{1000};
};

struct lattice_warning_threshold_relation_entry_t {
  std::string relation{};
  bool measurement_available{false};
  bool triggered{false};
};

[[nodiscard]] inline std::vector<lattice_warning_threshold_relation_entry_t>
lattice_warning_threshold_relation_vocabulary() {
  return {
      {"above_threshold", true, true},    {"not_above_threshold", true, false},
      {"below_threshold", true, true},    {"not_below_threshold", true, false},
      {"unavailable", false, false},      {"no_threshold", false, false},
      {"unknown_direction", false, false}};
}

[[nodiscard]] inline std::vector<lattice_deficit_priority_entry_t>
lattice_deficit_priority_vocabulary() {
  return {
      {"proof_context", 10}, {"certificate", 15}, {"checkpoint_binding", 20},
      {"model_state", 25},   {"dependency", 30},  {"artifact", 35},
      {"lineage", 40},       {"leakage", 50},     {"coverage", 60},
      {"metric", 70},        {"status", 90},      {"fallback", 1000}};
}

[[nodiscard]] inline std::pair<std::int64_t, std::string>
lattice_deficit_priority_for(std::string_view kind,
                             std::string_view dimension) {
  if (kind == "proof_context") {
    return {10, "proof_context"};
  }
  if (kind == "certificate") {
    return {15, "certificate"};
  }
  if (kind == "dependency") {
    if (dimension == "mdn_checkpoint" ||
        dimension == "representation_checkpoint") {
      return {20, "checkpoint_binding"};
    }
    return {30, "dependency"};
  }
  if (kind == "model_state") {
    return {25, "model_state"};
  }
  if (kind == "artifact") {
    return {35, "artifact"};
  }
  if (kind == "closure") {
    return {40, "lineage"};
  }
  if (kind == "leakage") {
    return {50, "leakage"};
  }
  if (kind == "coverage") {
    return {60, "coverage"};
  }
  if (kind == "metric") {
    return {70, "metric"};
  }
  if (kind == "status") {
    return {90, "status"};
  }
  return {1000, "fallback"};
}

struct lattice_deficit_vector_planning_summary_t {
  std::string schema{"kikijyeba.lattice.deficit_vector_planning.summary.v1"};
  std::int64_t deficit_priority_class_count{0};
  std::int64_t plan_advice_policy_count{0};
  std::int64_t plan_advice_only_count{0};
  std::int64_t evidence_authority_count{0};
  std::int64_t scheduler_authority_count{0};
  std::int64_t lattice_executor_count{0};
  std::int64_t affects_contract_identity_count{0};
  std::int64_t attempt_budget_guard_count{0};
  std::int64_t duplicate_priority_count{0};
  std::int64_t empty_policy_field_count{0};
  bool lower_priority_is_primary{true};
  bool priority_order_strictly_increasing{false};
  bool fallback_priority_is_terminal{false};
  bool checkpoint_binding_outranks_dependency{false};
  bool identity_outranks_certificate{false};
  bool leakage_outranks_coverage{false};
  bool metric_outranks_status{false};
  bool plan_basis_advice_present{false};
  bool suggested_wave_advice_present{false};
  bool plan_input_model_state_hint_present{false};
  bool attempt_budget_guard_present{false};
  bool runtime_hero_executor_boundary_present{false};
  bool plan_surfaces_are_advice_not_evidence{false};
  bool runtime_hero_is_only_executor{false};
  bool plan_advice_does_not_affect_contract_identity{false};
  bool all_policies_have_boundary_text{false};
  bool summary_self_check_passed{false};
  std::int64_t summary_issue_count{0};
  std::vector<std::string> priority_classes{};
  std::vector<std::string> plan_advice_surfaces{};
  std::vector<std::string> summary_issues{};
};

[[nodiscard]] inline lattice_deficit_vector_planning_summary_t
lattice_deficit_vector_planning_summary() {
  lattice_deficit_vector_planning_summary_t out{};
  const auto priority_vocabulary = lattice_deficit_priority_vocabulary();
  const auto plan_vocabulary = lattice_plan_advice_scope_policy_vocabulary();
  out.deficit_priority_class_count =
      static_cast<std::int64_t>(priority_vocabulary.size());
  out.plan_advice_policy_count =
      static_cast<std::int64_t>(plan_vocabulary.size());

  std::set<std::int64_t> seen_priorities;
  std::int64_t previous_priority = std::numeric_limits<std::int64_t>::min();
  bool priorities_increase = !priority_vocabulary.empty();
  for (const auto &entry : priority_vocabulary) {
    out.priority_classes.push_back(entry.priority_class);
    if (!seen_priorities.insert(entry.priority).second) {
      ++out.duplicate_priority_count;
    }
    if (entry.priority <= previous_priority) {
      priorities_increase = false;
    }
    previous_priority = entry.priority;
  }

  const auto find_plan_policy = [&](const std::string &policy) {
    return std::find_if(
        plan_vocabulary.begin(), plan_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  for (const auto &entry : plan_vocabulary) {
    out.plan_advice_surfaces.push_back(entry.surface);
    if (entry.advice_only) {
      ++out.plan_advice_only_count;
    }
    if (entry.evidence_authority) {
      ++out.evidence_authority_count;
    }
    if (entry.scheduler_authority) {
      ++out.scheduler_authority_count;
    }
    if (entry.lattice_executor) {
      ++out.lattice_executor_count;
    }
    if (entry.affects_contract_identity) {
      ++out.affects_contract_identity_count;
    }
    if (entry.attempt_budget_guard) {
      ++out.attempt_budget_guard_count;
    }
    if (entry.policy.empty() || entry.surface.empty() ||
        entry.source_relation.empty() || entry.allowed_claim.empty() ||
        entry.forbidden_claim.empty() || entry.execution_boundary.empty()) {
      ++out.empty_policy_field_count;
    }
  }

  const auto [checkpoint_priority, checkpoint_class] =
      lattice_deficit_priority_for("dependency", "mdn_checkpoint");
  const auto [dependency_priority, dependency_class] =
      lattice_deficit_priority_for("dependency", "target_dependency");
  const auto [identity_priority, identity_class] =
      lattice_deficit_priority_for("proof_context", "");
  const auto [certificate_priority, certificate_class] =
      lattice_deficit_priority_for("certificate", "proof_certificate_check");
  const auto [leakage_priority, leakage_class] =
      lattice_deficit_priority_for("leakage", "forbidden_overlap");
  const auto [coverage_priority, coverage_class] =
      lattice_deficit_priority_for("coverage", "cursor_epochs");
  const auto [metric_priority, metric_class] =
      lattice_deficit_priority_for("metric", "optimizer_steps");
  const auto [status_priority, status_class] =
      lattice_deficit_priority_for("status", "not_completed");

  out.priority_order_strictly_increasing =
      priorities_increase && out.duplicate_priority_count == 0;
  out.fallback_priority_is_terminal =
      !priority_vocabulary.empty() &&
      priority_vocabulary.back().priority_class == "fallback" &&
      priority_vocabulary.back().priority == 1000;
  out.checkpoint_binding_outranks_dependency =
      checkpoint_priority < dependency_priority &&
      checkpoint_class == "checkpoint_binding" &&
      dependency_class == "dependency";
  out.identity_outranks_certificate =
      identity_priority < certificate_priority &&
      identity_class == "proof_context" && certificate_class == "certificate";
  out.leakage_outranks_coverage = leakage_priority < coverage_priority &&
                                  leakage_class == "leakage" &&
                                  coverage_class == "coverage";
  out.metric_outranks_status = metric_priority < status_priority &&
                               metric_class == "metric" &&
                               status_class == "status";

  const auto plan_basis_policy =
      find_plan_policy("plan_basis_deficit_projection");
  const auto suggested_wave_policy =
      find_plan_policy("suggested_wave_advice_only");
  const auto plan_input_policy =
      find_plan_policy("plan_input_model_state_hint");
  const auto attempt_budget_policy =
      find_plan_policy("plan_attempt_budget_guard");
  const auto runtime_executor_policy =
      find_plan_policy("runtime_hero_executor_boundary");

  out.plan_basis_advice_present = plan_basis_policy != plan_vocabulary.end() &&
                                  plan_basis_policy->advice_only &&
                                  !plan_basis_policy->evidence_authority;
  out.suggested_wave_advice_present =
      suggested_wave_policy != plan_vocabulary.end() &&
      suggested_wave_policy->advice_only &&
      suggested_wave_policy->forbidden_claim.find("dispatch") !=
          std::string::npos;
  out.plan_input_model_state_hint_present =
      plan_input_policy != plan_vocabulary.end() &&
      plan_input_policy->advice_only &&
      plan_input_policy->forbidden_claim.find("contract identity") !=
          std::string::npos;
  out.attempt_budget_guard_present =
      attempt_budget_policy != plan_vocabulary.end() &&
      attempt_budget_policy->attempt_budget_guard &&
      !attempt_budget_policy->evidence_authority;
  out.runtime_hero_executor_boundary_present =
      runtime_executor_policy != plan_vocabulary.end() &&
      !runtime_executor_policy->advice_only &&
      runtime_executor_policy->scheduler_authority &&
      runtime_executor_policy->execution_boundary.find(
          "Runtime Hero is the only executor") != std::string::npos;
  out.plan_surfaces_are_advice_not_evidence =
      out.plan_advice_only_count == 4 && out.evidence_authority_count == 0;
  out.runtime_hero_is_only_executor =
      out.scheduler_authority_count == 1 && out.lattice_executor_count == 0 &&
      out.runtime_hero_executor_boundary_present;
  out.plan_advice_does_not_affect_contract_identity =
      out.affects_contract_identity_count == 0;
  out.all_policies_have_boundary_text = out.empty_policy_field_count == 0;

  if (out.deficit_priority_class_count != 12) {
    out.summary_issues.push_back(
        "deficit priority vocabulary must contain 12 priority classes");
  }
  if (out.plan_advice_policy_count != 5) {
    out.summary_issues.push_back(
        "plan advice scope policy vocabulary must contain 5 policies");
  }
  if (!out.priority_order_strictly_increasing) {
    out.summary_issues.push_back(
        "deficit priorities must be unique and strictly increasing");
  }
  if (!out.fallback_priority_is_terminal) {
    out.summary_issues.push_back(
        "fallback deficit priority must be the terminal lowest-priority class");
  }
  if (!out.checkpoint_binding_outranks_dependency) {
    out.summary_issues.push_back(
        "checkpoint-binding deficits must outrank generic dependency deficits");
  }
  if (!out.identity_outranks_certificate) {
    out.summary_issues.push_back(
        "identity deficits must outrank certificate self-check deficits");
  }
  if (!out.leakage_outranks_coverage) {
    out.summary_issues.push_back(
        "leakage deficits must outrank coverage deficits");
  }
  if (!out.metric_outranks_status) {
    out.summary_issues.push_back(
        "metric deficits must outrank status deficits");
  }
  if (!out.plan_basis_advice_present) {
    out.summary_issues.push_back(
        "plan_basis deficit projection must remain advice, not evidence");
  }
  if (!out.suggested_wave_advice_present) {
    out.summary_issues.push_back(
        "suggested_wave must remain advisory and non-dispatching");
  }
  if (!out.plan_input_model_state_hint_present) {
    out.summary_issues.push_back(
        "PLAN_INPUT model-state hints must remain advisory and non-identity");
  }
  if (!out.attempt_budget_guard_present) {
    out.summary_issues.push_back(
        "plan attempt budget guard policy must remain visible");
  }
  if (!out.runtime_hero_executor_boundary_present) {
    out.summary_issues.push_back(
        "Runtime Hero executor boundary policy must be present");
  }
  if (!out.plan_surfaces_are_advice_not_evidence) {
    out.summary_issues.push_back(
        "plan surfaces must not become evidence authority");
  }
  if (!out.runtime_hero_is_only_executor) {
    out.summary_issues.push_back(
        "Lattice planning must not gain execution authority");
  }
  if (!out.plan_advice_does_not_affect_contract_identity) {
    out.summary_issues.push_back(
        "plan advice must not affect contract identity");
  }
  if (!out.all_policies_have_boundary_text) {
    out.summary_issues.push_back(
        "plan advice policy rows must declare boundary text");
  }

  out.summary_issue_count =
      static_cast<std::int64_t>(out.summary_issues.size());
  out.summary_self_check_passed = out.summary_issues.empty();
  return out;
}

enum class lattice_evidence_order_relation_t {
  unavailable,
  selector_leaked,
  equivalent,
  left_dominates,
  right_dominates,
  incomparable,
};

[[nodiscard]] inline const char *lattice_evidence_order_relation_name(
    lattice_evidence_order_relation_t relation) {
  switch (relation) {
  case lattice_evidence_order_relation_t::unavailable:
    return "unavailable";
  case lattice_evidence_order_relation_t::selector_leaked:
    return "selector_leaked";
  case lattice_evidence_order_relation_t::equivalent:
    return "equivalent";
  case lattice_evidence_order_relation_t::left_dominates:
    return "left_dominates";
  case lattice_evidence_order_relation_t::right_dominates:
    return "right_dominates";
  case lattice_evidence_order_relation_t::incomparable:
    return "incomparable";
  }
  throw std::runtime_error("[lattice_target] unknown evidence order relation");
}

[[nodiscard]] inline std::vector<lattice_evidence_order_relation_t>
lattice_evidence_order_relation_vocabulary() {
  return {lattice_evidence_order_relation_t::unavailable,
          lattice_evidence_order_relation_t::selector_leaked,
          lattice_evidence_order_relation_t::equivalent,
          lattice_evidence_order_relation_t::left_dominates,
          lattice_evidence_order_relation_t::right_dominates,
          lattice_evidence_order_relation_t::incomparable};
}

struct lattice_evidence_order_dimension_entry_t {
  std::string dimension{};
  std::string polarity{};
  bool comparison_dimension{true};
};

[[nodiscard]] inline std::vector<lattice_evidence_order_dimension_entry_t>
lattice_evidence_order_dimension_vocabulary() {
  return {
      {"satisfied", "true_is_stronger", true},
      {"proof_certificate_check_passed", "true_is_stronger", true},
      {"identity_contract_match", "true_is_stronger", true},
      {"identity_component_match", "true_is_stronger", true},
      {"identity_graph_order_match", "true_is_stronger", true},
      {"identity_source_cursor_match", "true_is_stronger", true},
      {"min_unique_coverage_fraction", "higher_is_stronger", true},
      {"max_cursor_exposure_load", "higher_is_stronger_clean_load", true},
      {"closure_checked", "true_is_stronger", true},
      {"closure_complete", "true_is_stronger", true},
      {"leakage_checked", "true_is_stronger", true},
      {"leakage_passed", "true_is_stronger", true},
      {"selection_signal_leakage_found", "false_required_for_participation",
       true},
      {"aggregate_valid_target_wilson_lower_95", "higher_is_stronger", true},
      {"weakest_node_valid_target_wilson_lower_95", "higher_is_stronger", true},
      {"max_node_support_coefficient_of_variation", "lower_is_stronger", true},
      {"max_node_support_gini", "lower_is_stronger", true},
      {"source_key_audit_count", "visibility_only", false},
      {"source_key_audit_issue_count", "lower_is_stronger", true},
      {"source_key_affine_inconsistent_count", "lower_is_stronger", true},
      {"blocking_deficit_count", "lower_is_stronger", true},
      {"unresolved_lineage_count", "lower_is_stronger", true},
      {"warning_result_count", "visibility_only", false},
      {"triggered_warning_count", "lower_is_stronger", true},
      {"unavailable_warning_count", "lower_is_stronger", true},
      {"warning_count", "alias_for_triggered_warning_count", false}};
}

[[nodiscard]] inline lattice_evidence_order_relation_t
compare_lattice_evidence_order_vectors(
    const lattice_target_evaluation_t::evidence_order_vector_t &left,
    const lattice_target_evaluation_t::evidence_order_vector_t &right) {
  if (!left.available || !right.available ||
      left.order != "pareto_partial_order" ||
      right.order != "pareto_partial_order") {
    return lattice_evidence_order_relation_t::unavailable;
  }
  if (left.selection_signal_leakage_found ||
      right.selection_signal_leakage_found) {
    return lattice_evidence_order_relation_t::selector_leaked;
  }

  bool left_better = false;
  bool right_better = false;
  const auto note_comparison = [&](int cmp) {
    if (cmp > 0) {
      left_better = true;
    } else if (cmp < 0) {
      right_better = true;
    }
  };
  const auto compare_bool = [&](bool lhs, bool rhs) {
    if (lhs == rhs) {
      return 0;
    }
    return lhs ? 1 : -1;
  };
  const auto compare_count_lower_is_stronger = [](std::int64_t lhs,
                                                  std::int64_t rhs) {
    if (lhs == rhs) {
      return 0;
    }
    return lhs < rhs ? 1 : -1;
  };
  const auto compare_double_higher_is_stronger = [](double lhs, double rhs) {
    const bool lhs_finite = std::isfinite(lhs);
    const bool rhs_finite = std::isfinite(rhs);
    if (!lhs_finite && !rhs_finite) {
      return 0;
    }
    if (lhs_finite != rhs_finite) {
      return lhs_finite ? 1 : -1;
    }
    if (std::abs(lhs - rhs) <= 1e-9) {
      return 0;
    }
    return lhs > rhs ? 1 : -1;
  };
  const auto compare_double_lower_is_stronger = [](double lhs, double rhs) {
    const bool lhs_finite = std::isfinite(lhs);
    const bool rhs_finite = std::isfinite(rhs);
    if (!lhs_finite && !rhs_finite) {
      return 0;
    }
    if (lhs_finite != rhs_finite) {
      return lhs_finite ? 1 : -1;
    }
    if (std::abs(lhs - rhs) <= 1e-9) {
      return 0;
    }
    return lhs < rhs ? 1 : -1;
  };

  note_comparison(compare_bool(left.satisfied, right.satisfied));
  note_comparison(compare_bool(left.proof_certificate_check_passed,
                               right.proof_certificate_check_passed));
  note_comparison(compare_bool(left.identity_contract_match,
                               right.identity_contract_match));
  note_comparison(compare_bool(left.identity_component_match,
                               right.identity_component_match));
  note_comparison(compare_bool(left.identity_graph_order_match,
                               right.identity_graph_order_match));
  note_comparison(compare_bool(left.identity_source_cursor_match,
                               right.identity_source_cursor_match));
  note_comparison(compare_double_higher_is_stronger(
      left.min_unique_coverage_fraction, right.min_unique_coverage_fraction));
  note_comparison(compare_double_higher_is_stronger(
      left.max_cursor_exposure_load, right.max_cursor_exposure_load));
  note_comparison(compare_bool(left.closure_checked, right.closure_checked));
  note_comparison(compare_bool(left.closure_complete, right.closure_complete));
  note_comparison(compare_bool(left.leakage_checked, right.leakage_checked));
  note_comparison(compare_bool(left.leakage_passed, right.leakage_passed));
  note_comparison(compare_double_higher_is_stronger(
      left.aggregate_valid_target_wilson_lower_95,
      right.aggregate_valid_target_wilson_lower_95));
  note_comparison(compare_double_higher_is_stronger(
      left.weakest_node_valid_target_wilson_lower_95,
      right.weakest_node_valid_target_wilson_lower_95));
  note_comparison(compare_double_lower_is_stronger(
      left.max_node_support_coefficient_of_variation,
      right.max_node_support_coefficient_of_variation));
  note_comparison(compare_double_lower_is_stronger(
      left.max_node_support_gini, right.max_node_support_gini));
  note_comparison(compare_count_lower_is_stronger(
      left.source_key_audit_issue_count, right.source_key_audit_issue_count));
  note_comparison(compare_count_lower_is_stronger(
      left.source_key_affine_inconsistent_count,
      right.source_key_affine_inconsistent_count));
  note_comparison(compare_count_lower_is_stronger(
      left.blocking_deficit_count, right.blocking_deficit_count));
  note_comparison(compare_count_lower_is_stronger(
      left.unresolved_lineage_count, right.unresolved_lineage_count));
  note_comparison(compare_count_lower_is_stronger(
      left.triggered_warning_count, right.triggered_warning_count));
  note_comparison(compare_count_lower_is_stronger(
      left.unavailable_warning_count, right.unavailable_warning_count));

  if (left_better && right_better) {
    return lattice_evidence_order_relation_t::incomparable;
  }
  if (left_better) {
    return lattice_evidence_order_relation_t::left_dominates;
  }
  if (right_better) {
    return lattice_evidence_order_relation_t::right_dominates;
  }
  return lattice_evidence_order_relation_t::equivalent;
}

struct lattice_target_validation_report_t {
  std::vector<std::string> warnings{};

  [[nodiscard]] bool ok() const { return warnings.empty(); }
};

[[nodiscard]] inline const char *
lattice_target_kind_name(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    return "not_applicable";
  case lattice_target_kind_t::vicreg_ready:
    return "vicreg_ready";
  case lattice_target_kind_t::mtf_representation_ready:
    return "mtf_representation_ready";
  case lattice_target_kind_t::channel_mdn_ready:
    return "channel_mdn_ready";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline lattice_target_kind_t
parse_lattice_target_kind(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value == "not_applicable" || value == "none") {
    throw std::runtime_error(
        std::string("[lattice_target] TARGET_KIND ") + value +
        " is internal to artifact_readiness; omit TARGET_KIND for artifact "
        "targets");
  }
  if (value == "representation_ready") {
    throw std::runtime_error(
        "[lattice_target] TARGET_KIND representation_ready was removed from "
        "the active vocabulary; use vicreg_ready or mtf_representation_ready");
  }
  if (value == "node_mdn_ready") {
    throw std::runtime_error(
        "[lattice_target] TARGET_KIND node_mdn_ready was removed from the "
        "active vocabulary; use channel_mdn_ready");
  }
  if (value == "vicreg_ready") {
    return lattice_target_kind_t::vicreg_ready;
  }
  if (value == "mtf_representation_ready" ||
      value == "mtf_jepa_mae_vicreg_ready") {
    return lattice_target_kind_t::mtf_representation_ready;
  }
  if (value == "channel_mdn_ready") {
    return lattice_target_kind_t::channel_mdn_ready;
  }
  throw std::runtime_error("[lattice_target] invalid TARGET_KIND: " + value);
}

[[nodiscard]] inline bool
lattice_target_kind_is_mdn(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
  case lattice_target_kind_t::mtf_representation_ready:
    return false;
  case lattice_target_kind_t::channel_mdn_ready:
    return true;
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
upstream_representation_component_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
  case lattice_target_kind_t::mtf_representation_ready:
  case lattice_target_kind_t::channel_mdn_ready:
    return "wikimyei.representation.encoding.vicreg";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
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
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
    return "wikimyei.representation.encoding.vicreg";
  case lattice_target_kind_t::mtf_representation_ready:
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  case lattice_target_kind_t::channel_mdn_ready:
    return "wikimyei.inference.expected_value.mdn";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
job_kind_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
    return "channel_representation_vicreg";
  case lattice_target_kind_t::mtf_representation_ready:
    return "channel_representation_mtf_jepa_mae_vicreg";
  case lattice_target_kind_t::channel_mdn_ready:
    return "channel_inference_mdn";
  }
  throw std::runtime_error("[lattice_target] unknown target kind");
}

[[nodiscard]] inline std::string
component_fingerprint_key_for_target_kind(lattice_target_kind_t kind) {
  switch (kind) {
  case lattice_target_kind_t::not_applicable:
    throw std::runtime_error("[lattice_target] target kind is not applicable");
  case lattice_target_kind_t::vicreg_ready:
    return "vicreg_assembly_fingerprint";
  case lattice_target_kind_t::mtf_representation_ready:
    return "mtf_jepa_mae_vicreg_assembly_fingerprint";
  case lattice_target_kind_t::channel_mdn_ready:
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

inline void require_finite_dimension(double value, const std::string &field,
                                     const std::string &unit,
                                     const std::string &target_id) {
  if (!std::isfinite(value)) {
    throw std::runtime_error("[lattice_target] " + field +
                             " must be a finite " + unit + " value for " +
                             target_id);
  }
}

inline void require_non_negative_dimension(double value,
                                           const std::string &field,
                                           const std::string &unit,
                                           const std::string &target_id) {
  require_finite_dimension(value, field, unit, target_id);
  if (value < 0.0) {
    throw std::runtime_error("[lattice_target] " + field +
                             " must be a non-negative " + unit + " value for " +
                             target_id);
  }
}

inline void require_dimension_at_least(double value, const std::string &field,
                                       const std::string &unit, double minimum,
                                       const std::string &target_id) {
  require_finite_dimension(value, field, unit, target_id);
  if (value < minimum) {
    std::ostringstream minimum_text;
    minimum_text << minimum;
    throw std::runtime_error("[lattice_target] " + field + " must be a " +
                             unit + " value >= " + minimum_text.str() +
                             " for " + target_id);
  }
}

inline void require_fraction_dimension(double value, const std::string &field,
                                       const std::string &target_id) {
  require_finite_dimension(value, field, "fraction", target_id);
  if (value < 0.0 || value > 1.0) {
    throw std::runtime_error("[lattice_target] " + field +
                             " must be a fraction in [0, 1] for " + target_id);
  }
}

inline void require_non_negative_count(std::int64_t value,
                                       const std::string &field,
                                       const std::string &target_id) {
  if (value < 0) {
    throw std::runtime_error("[lattice_target] " + field +
                             " must be a non-negative count for " + target_id);
  }
}

inline void require_non_negative_count_threshold(double value,
                                                 const std::string &field,
                                                 const std::string &target_id) {
  require_non_negative_dimension(value, field, "count", target_id);
  if (std::floor(value) != value) {
    throw std::runtime_error("[lattice_target] " + field +
                             " must be an integral count threshold for " +
                             target_id);
  }
}

inline void validate_lattice_target_class(const std::string &target_class,
                                          const std::string &target_id) {
  if (target_class.empty() || target_class == "readiness" ||
      target_class == "evaluation_readiness" ||
      target_class == "leakage_guard" || target_class == "artifact_readiness") {
    return;
  }
  if (target_class == "policy_gate" ||
      target_class == "validation_performance" ||
      target_class == "performance_acceptance") {
    throw std::runtime_error(
        "[lattice_target] TARGET_CLASS=" + target_class +
        " is reserved for future explicit policy gates; current targets must "
        "use readiness, evaluation_readiness, leakage_guard, "
        "artifact_readiness, and LATTICE_WARN visibility until thresholds, "
        "uncertainty, baselines, selector split, and leakage policy are "
        "declared for " +
        target_id);
  }
  if (target_class == "market_readiness" ||
      target_class == "deployment_readiness") {
    throw std::runtime_error(
        "[lattice_target] TARGET_CLASS=" + target_class +
        " is not a Lattice "
        "proof-core target class; market/deployment readiness must remain "
        "outside Lattice or behind a separately declared disabled policy gate "
        "for " +
        target_id);
  }
  throw std::runtime_error("[lattice_target] unsupported TARGET_CLASS=" +
                           target_class + " for " + target_id);
}

[[nodiscard]] inline bool
is_artifact_readiness_target_class(const std::string &target_class) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  return kv::lowercase(kv::trim(target_class)) == "artifact_readiness";
}

[[nodiscard]] inline bool
lattice_target_kind_applicable(const lattice_target_spec_t &spec) {
  return spec.target_kind_applicable &&
         !is_artifact_readiness_target_class(spec.target_class) &&
         spec.kind != lattice_target_kind_t::not_applicable;
}

[[nodiscard]] inline bool
lattice_target_kind_applicable(const lattice_target_evaluation_t &evaluation) {
  return evaluation.target_kind_applicable &&
         !is_artifact_readiness_target_class(evaluation.target_class) &&
         evaluation.kind != lattice_target_kind_t::not_applicable;
}

[[nodiscard]] inline std::string
lattice_target_effective_kind_name(const lattice_target_spec_t &spec) {
  return lattice_target_kind_applicable(spec)
             ? lattice_target_kind_name(spec.kind)
             : "not_applicable";
}

[[nodiscard]] inline std::string lattice_target_effective_kind_name(
    const lattice_target_evaluation_t &evaluation) {
  return lattice_target_kind_applicable(evaluation)
             ? lattice_target_kind_name(evaluation.kind)
             : "not_applicable";
}

[[nodiscard]] inline std::string
lattice_target_effective_kind_selector(const lattice_target_spec_t &spec) {
  return lattice_target_kind_applicable(spec)
             ? lattice_target_kind_name(spec.kind)
             : "none";
}

[[nodiscard]] inline std::string lattice_target_effective_kind_selector(
    const lattice_target_evaluation_t &evaluation) {
  return lattice_target_kind_applicable(evaluation)
             ? lattice_target_kind_name(evaluation.kind)
             : "none";
}

struct lattice_artifact_readiness_proof_template_t {
  std::string subject_fact_family{};
  std::string proof_kind{};
  std::string proof_claim{};
};

[[nodiscard]] inline const std::vector<
    lattice_artifact_readiness_proof_template_t> &
lattice_artifact_readiness_proof_templates() {
  static const std::vector<lattice_artifact_readiness_proof_template_t>
      templates{
          {"target_transform", "target_transform_contract_bound",
           "target transform contract existence, identity, units, mask policy, "
           "and support-surface binding"},
          {"forecast_baseline", "forecast_baseline_artifact_bound",
           "forecast baseline artifact existence, identity, target-transform "
           "binding, and support-surface binding"},
          {"forecast_eval", "forecast_eval_artifact_bound",
           "forecast evaluation artifact existence, checkpoint lineage, "
           "target-transform binding, baseline binding, selection-signal "
           "audit binding, and support counts"},
          {"observer_belief", "observer_belief_artifact_bound",
           "observer belief artifact existence, forecast lineage, raw-vs-"
           "allocation belief typing, scenario/covariance diagnostics, and "
           "deterministic-output binding"},
          {"allocation_engine", "allocation_artifact_bound",
           "allocation artifact existence, observer-belief binding, forecast "
           "lineage, reserve-node binding, objective/constraint diagnostics, "
           "and fallback/de-risk reasons"},
          {"replay_environment", "replay_environment_artifact_bound",
           "replay environment report existence, lineage, digest identity, "
           "bounded reset/step evidence, time-law cleanliness, projection "
           "coverage, Cajtucu execution traces, execution profile binding, and "
           "policy-set binding"},
          {"policy_training", "policy_training_artifact_bound",
           "policy training artifact identity, checkpoint lineage, train/"
           "validation/test range separation, training-only normalization and "
           "replay-buffer binding, selector policy, sealed-test declaration, "
           "environment/action/reward/execution-profile binding, and parent "
           "replay evidence binding"}};
  return templates;
}

[[nodiscard]] inline const lattice_artifact_readiness_proof_template_t *
artifact_readiness_proof_template_for_subject_fact_family(
    const std::string &fact_family) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto normalized = kv::lowercase(kv::trim(fact_family));
  for (const auto &proof_template :
       lattice_artifact_readiness_proof_templates()) {
    if (proof_template.subject_fact_family == normalized) {
      return &proof_template;
    }
  }
  return nullptr;
}

inline void validate_artifact_readiness_proof_template(
    const std::string &subject_fact_family, const std::string &proof_kind,
    const std::string &target_id) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto family = kv::lowercase(kv::trim(subject_fact_family));
  const auto proof = kv::lowercase(kv::trim(proof_kind));
  const auto *proof_template =
      artifact_readiness_proof_template_for_subject_fact_family(family);
  if (proof_template == nullptr) {
    throw std::runtime_error(
        "[lattice_target] TARGET_CLASS=artifact_readiness SUBJECT_FACT_FAMILY "
        "" +
        family + " has no artifact proof template for " + target_id +
        "; add a first-class proof template before promoting fact-family "
        "evidence to target proof");
  }
  if (proof != proof_template->proof_kind) {
    throw std::runtime_error(
        "[lattice_target] TARGET_CLASS=artifact_readiness PROOF_KIND " + proof +
        " does not match SUBJECT_FACT_FAMILY " + family + " for " + target_id +
        "; expected " + proof_template->proof_kind);
  }
}

[[nodiscard]] inline bool
is_latest_satisfying_checkpoint_selectable_target_class(
    const std::string &target_class) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto normalized = kv::lowercase(kv::trim(target_class));
  return normalized.empty() || normalized == "readiness" ||
         normalized == "leakage_guard";
}

[[nodiscard]] inline std::string
latest_satisfying_reference_target_id(const std::string &source) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const std::string prefix = "latest_satisfying:";
  const auto trimmed = kv::trim(source);
  if (kv::lowercase(trimmed).rfind(prefix, 0) != 0) {
    return {};
  }
  return kv::trim(trimmed.substr(prefix.size()));
}

inline void validate_artifact_readiness_no_training_knobs(
    const lattice_target_spec_t &spec) {
  if (!is_artifact_readiness_target_class(spec.target_class)) {
    return;
  }
  if (spec.require_checkpoint_exists) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require "
        "checkpoint existence for " +
        spec.target_id);
  }
  if (spec.require_finite_loss) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require finite "
        "loss for " +
        spec.target_id);
  }
  if (spec.min_optimizer_steps > 0) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require "
        "optimizer steps for " +
        spec.target_id);
  }
  if (spec.min_valid_target_fraction > 0.0) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require "
        "valid-target readiness thresholds for " +
        spec.target_id);
  }
  if (spec.require_active_node_head_count > 0 ||
      spec.require_trained_node_head_count > 0 ||
      spec.require_evaluated_node_head_count > 0) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require MDN node "
        "head counts for " +
        spec.target_id);
  }
  if (spec.min_observed_input_coverage > 0.0 ||
      spec.min_target_supervision_coverage > 0.0 ||
      spec.min_evaluation_metric_coverage > 0.0) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot require exposure "
        "coverage thresholds for " +
        spec.target_id);
  }
  if (!spec.upstream_target_id.empty()) {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets cannot declare "
        "UPSTREAM_TARGET_ID or LATTICE_DEPENDS for " +
        spec.target_id);
  }
}

[[nodiscard]] inline std::string
default_proof_kind_for_subject_fact_family(const std::string &fact_family) {
  const auto *proof_template =
      artifact_readiness_proof_template_for_subject_fact_family(fact_family);
  if (proof_template != nullptr) {
    return proof_template->proof_kind;
  }
  return {};
}

[[nodiscard]] inline std::string
normalize_lattice_subject_fact_family(const std::string &value,
                                      const std::string &target_id) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto trimmed = kv::trim(value);
  if (trimmed.empty()) {
    return {};
  }
  const auto parsed =
      cuwacunu::hero::lattice::exposure::parse_lattice_fact_family(
          trimmed);
  if (!parsed.has_value()) {
    throw std::runtime_error("[lattice_target] unsupported "
                             "SUBJECT_FACT_FAMILY " +
                             trimmed + " for " + target_id);
  }
  return cuwacunu::hero::lattice::exposure::lattice_fact_family_name(
      *parsed);
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
    uses << cuwacunu::hero::lattice::exposure::exposure_use_name(
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

[[nodiscard]] inline std::string
canonical_lattice_policy_gate_text(const lattice_policy_gate_spec_t &gate) {
  std::ostringstream out;
  out << "policy_id=" << gate.policy_id << "\n";
  out << "policy_kind=" << gate.policy_kind << "\n";
  out << "target_id=" << gate.target_id << "\n";
  out << "metric=" << gate.metric << "\n";
  out << "metric_definition=" << gate.metric_definition << "\n";
  out << "baseline=" << gate.baseline << "\n";
  out << "baseline_definition=" << gate.baseline_definition << "\n";
  out << "threshold=" << gate.threshold << "\n";
  out << "uncertainty_policy=" << gate.uncertainty_policy << "\n";
  out << "uncertainty_model=" << gate.uncertainty_model << "\n";
  out << "support_minimum=" << gate.support_minimum << "\n";
  out << "selector_split=" << gate.selector_split << "\n";
  out << "anti_leakage_policy=" << gate.anti_leakage_policy << "\n";
  out << "tie_policy=" << gate.tie_policy << "\n";
  out << "negative_tests=" << gate.negative_tests << "\n";
  out << "calibration_requirements=" << gate.calibration_requirements << "\n";
  out << "holdout_declaration=" << gate.holdout_declaration << "\n";
  out << "threshold_selection_audit=" << gate.threshold_selection_audit << "\n";
  out << "enabled=" << (gate.enabled ? "true" : "false") << "\n";
  out << "status=" << gate.status << "\n";
  return out.str();
}

[[nodiscard]] inline std::string
lattice_policy_gate_fingerprint(const lattice_policy_gate_spec_t &gate) {
  return cuwacunu::hero::lattice::exposure::exposure_digest_for_text(
      std::string("kikijyeba.lattice.policy_gate.reserved.v1\n") +
      canonical_lattice_policy_gate_text(gate));
}

[[nodiscard]] inline bool
is_reserved_lattice_policy_gate_kind(const std::string &policy_kind) {
  return policy_kind == "forecast_quality_acceptance" ||
         policy_kind == "allocation_acceptance";
}

[[nodiscard]] inline std::string
expected_subject_fact_family_for_policy_gate_kind(
    const std::string &policy_kind) {
  if (policy_kind == "forecast_quality_acceptance") {
    return "forecast_eval";
  }
  if (policy_kind == "allocation_acceptance") {
    return "allocation_engine";
  }
  return {};
}

[[nodiscard]] inline std::vector<std::string>
lattice_policy_gate_required_input_fields() {
  return {"POLICY_ID",
          "POLICY_KIND",
          "TARGET_ID",
          "METRIC",
          "METRIC_DEFINITION",
          "BASELINE",
          "BASELINE_DEFINITION",
          "THRESHOLD",
          "UNCERTAINTY_POLICY",
          "UNCERTAINTY_MODEL",
          "SUPPORT_MINIMUM",
          "SELECTOR_SPLIT",
          "ANTI_LEAKAGE_POLICY",
          "TIE_POLICY",
          "NEGATIVE_TESTS",
          "CALIBRATION_REQUIREMENTS",
          "HOLDOUT_DECLARATION",
          "THRESHOLD_SELECTION_AUDIT",
          "ENABLED"};
}

[[nodiscard]] inline std::vector<std::string>
lattice_policy_gate_missing_required_inputs(
    const lattice_policy_gate_spec_t &gate) {
  std::vector<std::string> missing;
  const auto require_text = [&](const std::string &field,
                                const std::string &value) {
    if (value.empty()) {
      missing.push_back(field);
    }
  };
  require_text("POLICY_ID", gate.policy_id);
  require_text("POLICY_KIND", gate.policy_kind);
  require_text("TARGET_ID", gate.target_id);
  require_text("METRIC", gate.metric);
  require_text("METRIC_DEFINITION", gate.metric_definition);
  require_text("BASELINE", gate.baseline);
  require_text("BASELINE_DEFINITION", gate.baseline_definition);
  if (!std::isfinite(gate.threshold)) {
    missing.push_back("THRESHOLD");
  }
  require_text("UNCERTAINTY_POLICY", gate.uncertainty_policy);
  require_text("UNCERTAINTY_MODEL", gate.uncertainty_model);
  if (!std::isfinite(gate.support_minimum) || gate.support_minimum < 0.0) {
    missing.push_back("SUPPORT_MINIMUM");
  }
  require_text("SELECTOR_SPLIT", gate.selector_split);
  require_text("ANTI_LEAKAGE_POLICY", gate.anti_leakage_policy);
  require_text("TIE_POLICY", gate.tie_policy);
  require_text("NEGATIVE_TESTS", gate.negative_tests);
  require_text("CALIBRATION_REQUIREMENTS", gate.calibration_requirements);
  require_text("HOLDOUT_DECLARATION", gate.holdout_declaration);
  require_text("THRESHOLD_SELECTION_AUDIT", gate.threshold_selection_audit);
  return missing;
}

[[nodiscard]] inline bool lattice_policy_gate_input_contract_complete(
    const lattice_policy_gate_spec_t &gate) {
  return lattice_policy_gate_missing_required_inputs(gate).empty();
}

[[nodiscard]] inline lattice_policy_gate_spec_t
decode_lattice_policy_gate_spec_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  if (block.name != "LATTICE_POLICY_GATE") {
    throw std::runtime_error(
        "[lattice_target] expected LATTICE_POLICY_GATE block");
  }

  lattice_policy_gate_spec_t out{};
  out.fields = lattice_clause_fields_from_block(block);
  out.policy_id = kv::required(block, "POLICY_ID");
  out.policy_kind = kv::lowercase(kv::required(block, "POLICY_KIND"));
  out.target_id = kv::required(block, "TARGET_ID");
  out.metric = kv::lowercase(kv::required(block, "METRIC"));
  out.metric_definition = kv::required(block, "METRIC_DEFINITION");
  out.baseline = kv::required(block, "BASELINE");
  out.baseline_definition = kv::required(block, "BASELINE_DEFINITION");
  out.threshold = kv::parse_double(kv::required(block, "THRESHOLD"));
  out.uncertainty_policy =
      kv::lowercase(kv::required(block, "UNCERTAINTY_POLICY"));
  out.uncertainty_model =
      kv::lowercase(kv::required(block, "UNCERTAINTY_MODEL"));
  out.support_minimum =
      kv::parse_double(kv::required(block, "SUPPORT_MINIMUM"));
  out.selector_split = kv::required(block, "SELECTOR_SPLIT");
  out.anti_leakage_policy =
      kv::lowercase(kv::required(block, "ANTI_LEAKAGE_POLICY"));
  out.tie_policy = kv::lowercase(kv::required(block, "TIE_POLICY"));
  out.negative_tests = kv::required(block, "NEGATIVE_TESTS");
  out.calibration_requirements =
      kv::required(block, "CALIBRATION_REQUIREMENTS");
  out.holdout_declaration = kv::required(block, "HOLDOUT_DECLARATION");
  out.threshold_selection_audit =
      kv::required(block, "THRESHOLD_SELECTION_AUDIT");
  out.enabled = kv::parse_bool(kv::required(block, "ENABLED"));
  out.policy_fingerprint = kv::optional(block, "POLICY_FINGERPRINT", "");

  if (out.policy_id.empty()) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE requires non-empty POLICY_ID");
  }
  if (out.target_id.empty()) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE requires non-empty TARGET_ID");
  }
  if (!is_reserved_lattice_policy_gate_kind(out.policy_kind)) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE POLICY_KIND is unsupported for " +
        out.policy_id +
        "; supported disabled reservation kinds are "
        "forecast_quality_acceptance and allocation_acceptance; "
        "performance_acceptance, market_readiness, and deployment_readiness "
        "remain fail-closed future decision surfaces");
  }
  if (!std::isfinite(out.threshold)) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE THRESHOLD must be finite for " +
        out.policy_id);
  }
  if (!std::isfinite(out.support_minimum) || out.support_minimum < 0.0) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE SUPPORT_MINIMUM must be "
        "finite and non-negative for " +
        out.policy_id);
  }
  const auto missing_inputs = lattice_policy_gate_missing_required_inputs(out);
  if (!missing_inputs.empty()) {
    std::ostringstream message;
    message << "[lattice_target] LATTICE_POLICY_GATE " << out.policy_id
            << " is missing required policy input";
    if (missing_inputs.size() != 1) {
      message << "s";
    }
    message << ": ";
    for (std::size_t i = 0; i < missing_inputs.size(); ++i) {
      if (i != 0) {
        message << ",";
      }
      message << missing_inputs[i];
    }
    throw std::runtime_error(message.str());
  }

  if (out.enabled) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE is reserved and disabled; "
        "future policy gates must remain ENABLED=false until policy "
        "authority, metric/baseline definitions, uncertainty model, selector "
        "split, anti-leakage tests, calibration requirements, holdout "
        "declaration, and threshold-selection audit are explicit for " +
        out.policy_id);
  }

  const auto expected_policy_fingerprint = lattice_policy_gate_fingerprint(out);
  if (out.policy_fingerprint.empty()) {
    out.policy_fingerprint = expected_policy_fingerprint;
  } else if (out.policy_fingerprint != expected_policy_fingerprint) {
    throw std::runtime_error(
        "[lattice_target] LATTICE_POLICY_GATE POLICY_FINGERPRINT mismatch "
        "for " +
        out.policy_id +
        "; supplied fingerprint does not match the reserved "
        "policy declaration");
  }
  return out;
}

[[nodiscard]] inline std::vector<lattice_policy_gate_spec_t>
decode_lattice_policy_gates_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  std::vector<lattice_policy_gate_spec_t> out;
  std::set<std::string> policy_ids;
  for (const auto &block : kv::parse_blocks(dsl_text)) {
    if (block.name != "LATTICE_POLICY_GATE") {
      continue;
    }
    auto gate = decode_lattice_policy_gate_spec_block(block);
    if (!policy_ids.insert(gate.policy_id).second) {
      throw std::runtime_error("[lattice_target] duplicate POLICY_ID: " +
                               gate.policy_id);
    }
    out.push_back(std::move(gate));
  }
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
  validate_lattice_target_class(out.target_class, out.target_id);
  const bool artifact_readiness =
      is_artifact_readiness_target_class(out.target_class);
  if (artifact_readiness) {
    out.target_kind_applicable = false;
    out.checkpoint_source = "none";
    out.plan_mode = "none";
    out.require_component_match = false;
    out.require_checkpoint_exists = false;
    out.require_finite_loss = false;
    out.max_waves = 0;
  }
  const auto target_kind_raw = kv::optional(block, "TARGET_KIND", "");
  if (kv::trim(target_kind_raw).empty()) {
    if (!artifact_readiness) {
      throw std::runtime_error("[lattice_target] TARGET_KIND is required for " +
                               out.target_id);
    }
  } else {
    if (artifact_readiness) {
      throw std::runtime_error(
          "[lattice_target] TARGET_CLASS=artifact_readiness cannot declare "
          "TARGET_KIND for " +
          out.target_id);
    }
    out.target_kind_applicable = true;
    out.kind = parse_lattice_target_kind(target_kind_raw);
  }
  out.proof_kind = kv::lowercase(kv::optional(block, "PROOF_KIND", ""));
  out.subject_fact_family = normalize_lattice_subject_fact_family(
      optional_alias(block, "FACT_FAMILY", "SUBJECT_FACT_FAMILY", "",
                     out.language_warnings, out.target_id),
      out.target_id);
  if (artifact_readiness && out.subject_fact_family.empty()) {
    throw std::runtime_error(
        "[lattice_target] TARGET_CLASS=artifact_readiness requires "
        "SUBJECT_FACT_FAMILY for " +
        out.target_id);
  }
  if (artifact_readiness && out.proof_kind.empty()) {
    out.proof_kind =
        default_proof_kind_for_subject_fact_family(out.subject_fact_family);
    if (out.proof_kind.empty()) {
      throw std::runtime_error(
          "[lattice_target] TARGET_CLASS=artifact_readiness has no default "
          "PROOF_KIND for SUBJECT_FACT_FAMILY " +
          out.subject_fact_family + " on " + out.target_id);
    }
  }
  if (artifact_readiness) {
    validate_artifact_readiness_proof_template(out.subject_fact_family,
                                               out.proof_kind, out.target_id);
  }
  out.protocol_id = kv::optional(block, "PROTOCOL_ID", "");
  out.component = optional_alias(
      block, "COMPONENT", "SUBJECT_COMPONENT",
      artifact_readiness ? "" : default_component_for_target_kind(out.kind),
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
  out.require_component_match = kv::parse_bool(kv::optional(
      block, "REQUIRE_COMPONENT_MATCH", artifact_readiness ? "false" : "true"));
  out.require_checkpoint_exists =
      kv::parse_bool(kv::optional(block, "REQUIRE_CHECKPOINT_EXISTS",
                                  artifact_readiness ? "false" : "true"));
  out.require_finite_loss = kv::parse_bool(kv::optional(
      block, "REQUIRE_FINITE_LOSS", artifact_readiness ? "false" : "true"));
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
        cuwacunu::hero::lattice::exposure::parse_exposure_use_list(
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
                                     artifact_readiness ? "0" : "1",
                                     out.language_warnings, out.target_id);
  if (out.target_id.empty()) {
    throw std::runtime_error("[lattice_target] TARGET_ID is required");
  }
  if (out.protocol_id != kv::trim(out.protocol_id)) {
    throw std::runtime_error(
        "[lattice_target] PROTOCOL_ID must be trimmed for " + out.target_id);
  }
  if (!artifact_readiness &&
      out.component != default_component_for_target_kind(out.kind)) {
    throw std::runtime_error(
        "[lattice_target] COMPONENT does not match TARGET_KIND for " +
        out.target_id);
  }
  const auto source_range =
      kv::lowercase(kv::trim(std::string(out.source_range)));
  const auto checkpoint_source =
      kv::lowercase(kv::trim(std::string(out.checkpoint_source)));
  const std::string latest_prefix = "latest_satisfying:";
  if (artifact_readiness && checkpoint_source != "none") {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets must use "
        "CHECKPOINT_SOURCE=none for " +
        out.target_id);
  }
  if (!artifact_readiness && checkpoint_source != "output_checkpoint" &&
      checkpoint_source.rfind(latest_prefix, 0) != 0) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE must be output_checkpoint or "
        "latest_satisfying:<target_id> for " +
        out.target_id);
  }
  if (!artifact_readiness && checkpoint_source.rfind(latest_prefix, 0) == 0 &&
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
  if (artifact_readiness) {
    if (kv::lowercase(kv::trim(out.plan_mode)) != "none") {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must use "
          "WAVE_MODE=none for " +
          out.target_id);
    }
    if (out.max_waves != 0) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must use "
          "PLAN_MAX_ATTEMPTS=0 for " +
          out.target_id);
    }
    if (!kv::trim(out.evaluated_checkpoint_source).empty() ||
        !kv::trim(out.plan_input_mdn_checkpoint).empty() ||
        !kv::trim(out.plan_input_representation_checkpoint).empty()) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets cannot declare "
          "checkpoint source or plan checkpoint inputs for " +
          out.target_id);
    }
  } else {
    validate_latest_satisfying(out.evaluated_checkpoint_source,
                               "EVALUATED_CHECKPOINT_SOURCE");
    validate_latest_satisfying(out.plan_input_mdn_checkpoint,
                               "PLAN_INPUT_MDN_CHECKPOINT");
    validate_latest_satisfying(out.plan_input_representation_checkpoint,
                               "PLAN_INPUT_REPRESENTATION_CHECKPOINT");
  }
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
          "[lattice_target] SOURCE_RANGE=anchor_index requires either "
          "OVER_SPLIT or ANCHOR_INDEX_BEGIN/END for " +
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
      throw std::runtime_error("[lattice_target] OVER_SPLIT requires "
                               "SOURCE_RANGE=anchor_index for " +
                               out.target_id);
    }
    if (out.anchor_index_begin.has_value() ||
        out.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[lattice_target] OVER_SPLIT and ANCHOR_INDEX_BEGIN/END are "
          "mutually exclusive for " +
          out.target_id);
    }
  }
  require_non_negative_count(out.min_optimizer_steps, "MIN_OPTIMIZER_STEPS",
                             out.target_id);
  require_fraction_dimension(out.min_valid_target_fraction,
                             "MIN_VALID_TARGET_FRACTION", out.target_id);
  require_fraction_dimension(out.min_observed_input_coverage,
                             "MIN_OBSERVED_INPUT_COVERAGE", out.target_id);
  require_fraction_dimension(out.min_target_supervision_coverage,
                             "MIN_TARGET_SUPERVISION_COVERAGE", out.target_id);
  require_fraction_dimension(out.min_evaluation_metric_coverage,
                             "MIN_EVALUATION_METRIC_COVERAGE", out.target_id);
  require_non_negative_count(out.max_waves, "MAX_WAVES", out.target_id);
  require_non_negative_count(out.require_active_node_head_count,
                             "REQUIRE_ACTIVE_NODE_HEAD_COUNT", out.target_id);
  require_non_negative_count(out.require_trained_node_head_count,
                             "REQUIRE_TRAINED_NODE_HEAD_COUNT", out.target_id);
  require_non_negative_count(out.require_evaluated_node_head_count,
                             "REQUIRE_EVALUATED_NODE_HEAD_COUNT",
                             out.target_id);
  validate_artifact_readiness_no_training_knobs(out);
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
    out.uses = cuwacunu::hero::lattice::exposure::parse_exposure_use_list(
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
    require_fraction_dimension(value, "valid_target_fraction VALUE",
                               spec.target_id);
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
    const auto use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::required(block, "USE"));
    const auto split = kv::optional(block, "SPLIT", "");
    if (!split.empty()) {
      assign_or_confirm_string(spec.train_split, split, "OVER_SPLIT",
                               spec.target_id);
      require_anchor_index_source_range(spec, "exposure_coverage");
    }
    const auto evidence_scope = kv::lowercase(
        kv::optional(block, "EVIDENCE_SCOPE", "output_checkpoint_closure"));
    const auto scope = kv::lowercase(kv::optional(
        block, "COMPONENT_SCOPE",
        kv::optional(block, "SCOPE", "target_component_family_id")));
    const auto effect =
        kv::lowercase(kv::optional(block, "EFFECT", "mutated_component"));
    const auto coordinate = kv::lowercase(
        kv::optional(block, "COORDINATE", "graph_anchor_coverage"));
    if ((evidence_scope != "output_checkpoint_closure" &&
         evidence_scope != "target_component_family_id") ||
        scope != "target_component_family_id" ||
        coordinate != "graph_anchor_coverage") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES exposure_coverage v0 requires "
          "EVIDENCE_SCOPE=output_checkpoint_closure or "
          "target_component_family_id, "
          "COMPONENT_SCOPE=target_component_family_id, and "
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
    require_fraction_dimension(min_coverage, "exposure_coverage CURSOR_EPOCHS",
                               spec.target_id);
    if (use == cuwacunu::hero::lattice::exposure::exposure_use_t::
                   observed_input) {
      if (effect != "mutated_component") {
        throw std::runtime_error(
            "[lattice_target] observed_input exposure_coverage v0 requires "
            "EFFECT=mutated_component for " +
            spec.target_id);
      }
      spec.min_observed_input_coverage =
          std::max(spec.min_observed_input_coverage, min_coverage);
    } else if (use == cuwacunu::hero::lattice::exposure::exposure_use_t::
                          target_supervision) {
      if (effect != "mutated_component") {
        throw std::runtime_error(
            "[lattice_target] target_supervision exposure_coverage v0 "
            "requires EFFECT=mutated_component for " +
            spec.target_id);
      }
      spec.min_target_supervision_coverage =
          std::max(spec.min_target_supervision_coverage, min_coverage);
    } else if (use == cuwacunu::hero::lattice::exposure::exposure_use_t::
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
  if (kind == "representation_geometry") {
    if (spec.kind != lattice_target_kind_t::vicreg_ready &&
        spec.kind != lattice_target_kind_t::mtf_representation_ready) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry only "
          "supports representation targets for " +
          spec.target_id);
    }
    lattice_representation_geometry_requirement_spec_t requirement{};
    requirement.requirement_id = kv::optional(
        block, "REQUIREMENT_ID",
        auto_clause_id("requires_geometry", spec.target_id,
                       spec.representation_geometry_requirements.size()));
    requirement.metric = kv::lowercase(kv::required(block, "METRIC"));
    const auto metric_entry =
        lattice_representation_geometry_metric_entry(requirement.metric);
    if (!metric_entry.has_value()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry METRIC "
          "is unsupported for " +
          spec.target_id);
    }
    if (!metric_entry->future_hard_gate_candidate) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry METRIC "
          "is not a future hard-gate candidate for " +
          spec.target_id);
    }
    const auto default_op =
        lattice_representation_geometry_metric_high_is_bad(requirement.metric)
            ? "le"
            : "ge";
    requirement.op = kv::lowercase(kv::optional(block, "OP", default_op));
    if (requirement.op == "gte") {
      requirement.op = "ge";
    } else if (requirement.op == "lte") {
      requirement.op = "le";
    }
    if (requirement.op != "ge" && requirement.op != "le") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry only "
          "supports OP=ge or OP=le for " +
          spec.target_id);
    }
    if (lattice_representation_geometry_metric_high_is_bad(
            requirement.metric) &&
        requirement.op != "le") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry " +
          requirement.metric + " uses OP=le for " + spec.target_id);
    }
    if (lattice_representation_geometry_metric_low_is_bad(requirement.metric) &&
        requirement.op != "ge") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry " +
          requirement.metric + " uses OP=ge for " + spec.target_id);
    }
    const auto value_raw =
        kv::optional(block, "VALUE", kv::optional(block, "MIN_VALUE", ""));
    if (kv::trim(value_raw).empty()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry "
          "requires VALUE for " +
          spec.target_id);
    }
    requirement.value = kv::parse_double(value_raw);
    require_representation_geometry_threshold_dimension(
        requirement.metric, requirement.value, "VALUE", spec.target_id);
    const auto effect =
        kv::lowercase(kv::optional(block, "EFFECT", "mutated_component"));
    if (effect == "mutated_component") {
      requirement.require_mutated_component = true;
    } else if (effect == "any" || effect == "none") {
      requirement.require_mutated_component = false;
    } else {
      throw std::runtime_error(
          "[lattice_target] LATTICE_REQUIRES representation_geometry EFFECT "
          "must be mutated_component, any, or none for " +
          spec.target_id);
    }
    spec.representation_geometry_requirements.push_back(std::move(requirement));
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

inline void
validate_lattice_warning_dimensions(const lattice_warning_spec_t &warning,
                                    const std::string &target_id) {
  const auto validate_representation_health_dimension =
      [&target_id](const std::string &metric, double value,
                   const std::string &field) {
        if (metric == "mean_adapter_valid_channel_time_fraction" ||
            metric == "mean_augmented_valid_feature_fraction" ||
            metric == "mean_augmented_feature_retention_fraction" ||
            metric == "finite_parameter_check" ||
            metric == "gradients_finite" || metric == "sample_valid_fraction" ||
            metric == "channel_valid_fraction" ||
            metric == "representation_effective_rank_fraction" ||
            metric == "representation_isotropy_score") {
          require_fraction_dimension(value, field, target_id);
        } else if (metric == "total_valid_projection_rows" ||
                   metric == "augmented_valid_feature_count" ||
                   metric == "sample_valid_count" ||
                   metric == "channel_valid_count" ||
                   metric == "valid_latent_rows" || metric == "tf_pair_count" ||
                   metric == "tf_pair_valid_count" ||
                   metric == "vicreg_global_valid_rows" ||
                   metric == "vicreg_channel_valid_rows" ||
                   metric == "context_starved_sample_count" ||
                   metric == "reduced_targets_for_context_count" ||
                   metric == "min_context_satisfied_count" ||
                   metric == "representation_embedding_dim") {
          require_non_negative_count_threshold(value, field, target_id);
        } else if (metric == "representation_condition_number") {
          require_dimension_at_least(value, field, "condition_number", 1.0,
                                     target_id);
        } else {
          require_non_negative_dimension(value, field, metric, target_id);
        }
      };
  const auto validate_representation_health_direction =
      [&target_id](const std::string &metric, bool uses_above) {
        const std::set<std::string> high_is_bad{
            "mean_invariance_loss",
            "mean_variance_loss",
            "mean_covariance_loss",
            "loss_jepa_mean",
            "loss_mae_time_mean",
            "loss_mae_frequency_mean",
            "loss_tf_align_mean",
            "loss_vicreg_global_mean",
            "loss_vicreg_channel_mean",
            "last_grad_norm",
            "max_grad_norm",
            "context_starved_sample_count",
            "reduced_targets_for_context_count",
            "target_ema_distance",
            "latent_norm",
            "representation_max_dimension_variance",
            "representation_condition_number",
        };
        const std::set<std::string> low_is_bad{
            "total_valid_projection_rows",
            "mean_adapter_valid_channel_time_fraction",
            "augmented_valid_feature_count",
            "mean_augmented_valid_feature_fraction",
            "mean_augmented_feature_retention_fraction",
            "finite_parameter_check",
            "gradients_finite",
            "sample_valid_count",
            "sample_valid_fraction",
            "channel_valid_count",
            "channel_valid_fraction",
            "valid_latent_rows",
            "tf_pair_count",
            "tf_pair_valid_count",
            "vicreg_global_valid_rows",
            "vicreg_channel_valid_rows",
            "min_context_satisfied_count",
            "latent_std",
            "representation_embedding_dim",
            "representation_effective_rank",
            "representation_effective_rank_fraction",
            "representation_min_dimension_variance",
            "representation_isotropy_score",
        };
        if (high_is_bad.count(metric) && !uses_above) {
          throw std::runtime_error(
              "[lattice_target] LATTICE_WARN representation_health " + metric +
              " uses ABOVE for " + target_id);
        }
        if (low_is_bad.count(metric) && uses_above) {
          throw std::runtime_error(
              "[lattice_target] LATTICE_WARN representation_health " + metric +
              " uses BELOW for " + target_id);
        }
      };

  if (warning.kind == "exposure_load") {
    require_non_negative_dimension(warning.cursor_epochs_above,
                                   "CURSOR_EPOCHS_ABOVE", "cursor_epoch",
                                   target_id);
    return;
  }
  if (warning.kind == "effort_density") {
    require_non_negative_dimension(warning.above, "ABOVE", warning.metric,
                                   target_id);
    return;
  }
  if (warning.kind == "source_analytics") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> fraction_metrics{
        "sample_validity_fraction",
        "missingness_fraction",
    };
    const std::set<std::string> count_metrics{
        "duplicate_sample_count",
        "source_receipt_fact_count",
    };
    if (fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else if (count_metrics.count(warning.metric)) {
      require_non_negative_count_threshold(value, field, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "forecast_baseline") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> fraction_metrics{
        "baseline_directional_accuracy",
    };
    const std::set<std::string> count_metrics{
        "support_count",
        "valid_count",
        "missing_count",
        "baseline_kind_count",
        "computed_metric_fact_count",
        "partial_metric_fact_count",
        "deferred_metric_fact_count",
        "computed_metric_value_count",
        "metric_status_mismatch_count",
    };
    if (fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else if (count_metrics.count(warning.metric)) {
      require_non_negative_count_threshold(value, field, target_id);
    } else if (warning.metric == "baseline_signed_error") {
      require_finite_dimension(value, field, warning.metric, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "forecast_eval") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> fraction_metrics{
        "directional_accuracy",
        "calibration_coverage",
    };
    const std::set<std::string> count_metrics{
        "support_count",
        "valid_count",
        "missing_count",
        "weakest_support_rows",
        "weakest_horizon_support_rows",
    };
    if (fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else if (count_metrics.count(warning.metric)) {
      require_non_negative_count_threshold(value, field, target_id);
    } else if (warning.metric == "signed_error" ||
               warning.metric == "skill_vs_baseline" ||
               warning.metric == "skill_vs_naive" ||
               warning.metric == "skill_vs_baseline_mean_nll" ||
               warning.metric == "skill_vs_baseline_ev_mae" ||
               warning.metric == "skill_vs_baseline_ev_rmse" ||
               warning.metric == "directional_accuracy_delta_vs_baseline") {
      require_finite_dimension(value, field, warning.metric, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "observer_belief") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> fraction_metrics{
        "confidence",
        "data_quality",
        "liquidity",
        "raw_potential_tradable_return",
        "allocation_authority",
        "forecast_lineage_bound",
        "channel_consensus_bound",
        "potential_surface_diagnostics_bound",
        "nodelift_return_projection_bound",
        "covariance_coupling_bound",
        "scenario_bank_bound",
        "nodelift_residual_quality_bound",
        "projection_validation_scores_bound",
        "feature_semantics_bound",
        "dock_binding_bound",
    };
    if (fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "allocation_engine") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> fraction_metrics{
        "reserve_weight",
        "turnover",
        "transaction_cost_estimate",
        "deterministic_artifact",
        "visibility_only",
        "allocation_authority",
        "execution_authority",
        "market_readiness_authority",
        "deployment_authority",
        "reserve_node_bound",
        "reserve_node_from_base_policy",
        "reserve_node_base_policy_match",
        "reserve_node_graph_bound",
        "observer_belief_bound",
        "forecast_artifact_bound",
        "base_policy_bound",
        "cap_diagnostics_bound",
        "scenario_growth_floor_status_bound",
        "scenario_growth_floor_met",
        "scenario_growth_floor_attention",
        "fallback_declared",
        "fallback_active",
        "derisk_declared",
        "derisk_active",
    };
    if (fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "anchor_domain_health") {
    if (std::isfinite(warning.accepted_fraction_below)) {
      require_fraction_dimension(warning.accepted_fraction_below,
                                 "ACCEPTED_FRACTION_BELOW", target_id);
    }
    if (warning.skipped_failed_fetch_probe_above.has_value()) {
      require_non_negative_count(*warning.skipped_failed_fetch_probe_above,
                                 "SKIPPED_FAILED_FETCH_PROBE_ABOVE", target_id);
    }
    return;
  }
  if (warning.kind == "node_support_balance") {
    if (warning.metric == "valid_target_count_gini") {
      require_fraction_dimension(warning.above, "ABOVE", target_id);
    } else if (warning.metric == "valid_target_count_normalized_entropy") {
      require_fraction_dimension(warning.below, "BELOW", target_id);
    } else {
      require_non_negative_dimension(warning.above, "ABOVE", warning.metric,
                                     target_id);
    }
    return;
  }
  if (warning.kind == "node_support_floor") {
    if (warning.metric == "min_valid_target_fraction" ||
        warning.metric == "valid_target_wilson_lower_95") {
      require_fraction_dimension(warning.below, "BELOW", target_id);
    } else {
      require_non_negative_count_threshold(warning.below, "BELOW", target_id);
    }
    return;
  }
  if (warning.kind == "representation_health") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    validate_representation_health_dimension(warning.metric, value, field);
    validate_representation_health_direction(warning.metric, uses_above);
    return;
  }
  if (warning.kind == "runtime_health") {
    const bool uses_above = !std::isnan(warning.above);
    const auto field = uses_above ? "ABOVE" : "BELOW";
    const auto value = uses_above ? warning.above : warning.below;
    const std::set<std::string> boolean_metrics{
        "finite_parameter_check", "checkpoint_written",
        "model_state_mutated",    "representation_checkpoint_loaded",
        "mdn_checkpoint_loaded",
    };
    const std::set<std::string> fraction_metrics{
        "valid_target_fraction",
    };
    const std::set<std::string> count_metrics{
        "nonfinite_output_count",
    };
    if (boolean_metrics.count(warning.metric) ||
        fraction_metrics.count(warning.metric)) {
      require_fraction_dimension(value, field, target_id);
    } else if (count_metrics.count(warning.metric)) {
      require_non_negative_count_threshold(value, field, target_id);
    } else {
      require_non_negative_dimension(value, field, warning.metric, target_id);
    }
    return;
  }
  if (warning.kind == "mdn_distribution_calibration") {
    if (warning.metric == "pit_ks_statistic" ||
        warning.metric == "predictive_interval_coverage_error" ||
        warning.metric == "tail_coverage_error") {
      require_fraction_dimension(warning.above, "ABOVE", target_id);
    } else {
      require_non_negative_dimension(warning.above, "ABOVE", warning.metric,
                                     target_id);
    }
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
                   kv::optional(block, "SCOPE", "target_component_family_id")));
  if (warning.scope != "target_component_family_id" &&
      warning.scope != "all_components") {
    throw std::runtime_error(
        "[lattice_target] LATTICE_WARN SCOPE must be "
        "target_component_family_id or all_components for " +
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

  if (has_lattice_key(block, "ANCHOR_INDEX_BEGIN")) {
    const auto value = kv::parse_i64(kv::required(block, "ANCHOR_INDEX_BEGIN"));
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

  if (warning.kind == "exposure_load" || warning.kind == "effort_density") {
    warning.use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::required(block, "USE"));
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
    const bool has_accepted_fraction =
        std::isfinite(warning.accepted_fraction_below);
    const bool has_skipped_failed_fetch_probe =
        warning.skipped_failed_fetch_probe_above.has_value();
    if (has_accepted_fraction == has_skipped_failed_fetch_probe) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN anchor_domain_health requires "
          "ACCEPTED_FRACTION_BELOW or SKIPPED_FAILED_FETCH_PROBE_ABOVE, "
          "exactly one for " +
          spec.target_id);
    }
  } else if (warning.kind == "source_analytics") {
    warning.require_mutated_component = false;
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "entropy",
        "entropy_rate",
        "information_density",
        "compression_ratio",
        "power_spectrum_entropy",
        "sample_validity_fraction",
        "missingness_fraction",
        "duplicate_sample_count",
        "source_receipt_fact_count",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN source_analytics METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN source_analytics requires exactly "
          "one of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "forecast_baseline") {
    warning.require_mutated_component = false;
    using cuwacunu::hero::lattice::exposure::exposure_use_t;
    warning.use = exposure_use_t::evaluation_metric;
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "baseline_mean_nll",
        "baseline_ev_mae",
        "baseline_ev_rmse",
        "baseline_signed_error",
        "baseline_directional_accuracy",
        "support_count",
        "valid_count",
        "missing_count",
        "baseline_kind_count",
        "computed_metric_fact_count",
        "partial_metric_fact_count",
        "deferred_metric_fact_count",
        "computed_metric_value_count",
        "metric_status_mismatch_count",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN forecast_baseline METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN forecast_baseline requires exactly "
          "one of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "forecast_eval") {
    warning.require_mutated_component = false;
    using cuwacunu::hero::lattice::exposure::exposure_use_t;
    warning.use = exposure_use_t::evaluation_metric;
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "mean_nll",
        "ev_mae",
        "ev_rmse",
        "signed_error",
        "directional_accuracy",
        "calibration_coverage",
        "support_count",
        "valid_count",
        "missing_count",
        "weakest_support_rows",
        "weakest_horizon_support_rows",
        "mean_nll_per_horizon_max",
        "skill_vs_baseline",
        "skill_vs_naive",
        "skill_vs_baseline_mean_nll",
        "skill_vs_baseline_ev_mae",
        "skill_vs_baseline_ev_rmse",
        "directional_accuracy_delta_vs_baseline",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN forecast_eval METRIC is unsupported "
          "for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN forecast_eval requires exactly one "
          "of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "observer_belief") {
    warning.require_mutated_component = false;
    using cuwacunu::hero::lattice::exposure::exposure_use_t;
    warning.use = exposure_use_t::evaluation_metric;
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "confidence",
        "data_quality",
        "liquidity",
        "raw_potential_tradable_return",
        "allocation_authority",
        "forecast_lineage_bound",
        "channel_consensus_bound",
        "potential_surface_diagnostics_bound",
        "nodelift_return_projection_bound",
        "covariance_coupling_bound",
        "scenario_bank_bound",
        "nodelift_residual_quality_bound",
        "projection_validation_scores_bound",
        "feature_semantics_bound",
        "dock_binding_bound",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN observer_belief METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN observer_belief requires exactly "
          "one of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "allocation_engine") {
    warning.require_mutated_component = false;
    using cuwacunu::hero::lattice::exposure::exposure_use_t;
    warning.use = exposure_use_t::evaluation_metric;
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "reserve_weight",
        "turnover",
        "cvar_loss",
        "transaction_cost_estimate",
        "deterministic_artifact",
        "visibility_only",
        "allocation_authority",
        "execution_authority",
        "market_readiness_authority",
        "deployment_authority",
        "reserve_node_bound",
        "reserve_node_from_base_policy",
        "reserve_node_base_policy_match",
        "reserve_node_graph_bound",
        "observer_belief_bound",
        "forecast_artifact_bound",
        "base_policy_bound",
        "cap_diagnostics_bound",
        "scenario_growth_floor_status_bound",
        "scenario_growth_floor_met",
        "scenario_growth_floor_attention",
        "fallback_declared",
        "fallback_active",
        "derisk_declared",
        "derisk_active",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN allocation_engine METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN allocation_engine requires exactly "
          "one of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "node_support_balance") {
    warning.use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::optional(block, "USE", "target_supervision"));
    warning.metric =
        kv::lowercase(kv::optional(block, "METRIC", "valid_target_count_gini"));
    if (warning.metric != "valid_target_count_gini" &&
        warning.metric != "valid_target_count_coefficient_of_variation" &&
        warning.metric != "valid_target_count_normalized_entropy") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN node_support_balance METRIC must be "
          "valid_target_count_gini, "
          "valid_target_count_coefficient_of_variation, or "
          "valid_target_count_normalized_entropy for " +
          spec.target_id);
    }
    if (warning.metric == "valid_target_count_normalized_entropy") {
      if (!kv::trim(kv::optional(block, "ABOVE", "")).empty()) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN node_support_balance normalized "
            "entropy uses BELOW for " +
            spec.target_id);
      }
      warning.below = kv::parse_double(kv::required(block, "BELOW"));
      if (warning.below < 0.0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN BELOW must be non-negative for " +
            spec.target_id);
      }
    } else {
      warning.above = kv::parse_double(kv::required(block, "ABOVE"));
      if (warning.above < 0.0) {
        throw std::runtime_error(
            "[lattice_target] LATTICE_WARN ABOVE must be non-negative for " +
            spec.target_id);
      }
    }
  } else if (warning.kind == "node_support_floor") {
    warning.use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::optional(block, "USE", "target_supervision"));
    warning.metric = kv::lowercase(
        kv::optional(block, "METRIC", "weakest_valid_target_count"));
    if (warning.metric != "weakest_valid_target_count" &&
        warning.metric != "min_valid_target_fraction" &&
        warning.metric != "valid_target_wilson_lower_95") {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN node_support_floor METRIC must be "
          "weakest_valid_target_count, min_valid_target_fraction, or "
          "valid_target_wilson_lower_95 for " +
          spec.target_id);
    }
    warning.below = kv::parse_double(kv::required(block, "BELOW"));
    if (warning.below < 0.0) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN BELOW must be non-negative for " +
          spec.target_id);
    }
  } else if (warning.kind == "representation_health") {
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "mean_invariance_loss",
        "mean_variance_loss",
        "mean_covariance_loss",
        "last_grad_norm",
        "max_grad_norm",
        "total_valid_projection_rows",
        "mean_adapter_valid_channel_time_fraction",
        "augmented_valid_feature_count",
        "mean_augmented_valid_feature_fraction",
        "mean_augmented_feature_retention_fraction",
        "finite_parameter_check",
        "gradients_finite",
        "sample_valid_count",
        "sample_valid_fraction",
        "channel_valid_count",
        "channel_valid_fraction",
        "valid_latent_rows",
        "tf_pair_count",
        "tf_pair_valid_count",
        "vicreg_global_valid_rows",
        "vicreg_channel_valid_rows",
        "context_starved_sample_count",
        "reduced_targets_for_context_count",
        "min_context_satisfied_count",
        "target_ema_distance",
        "latent_std",
        "latent_norm",
        "loss_jepa_mean",
        "loss_mae_time_mean",
        "loss_mae_frequency_mean",
        "loss_tf_align_mean",
        "loss_vicreg_global_mean",
        "loss_vicreg_channel_mean",
        "representation_embedding_dim",
        "representation_effective_rank",
        "representation_effective_rank_fraction",
        "representation_min_dimension_variance",
        "representation_max_dimension_variance",
        "representation_condition_number",
        "representation_isotropy_score",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN representation_health METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN representation_health requires "
          "exactly one of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
      if (warning.metric == "mean_adapter_valid_channel_time_fraction" ||
          warning.metric == "finite_parameter_check" ||
          warning.metric == "gradients_finite" ||
          warning.metric == "sample_valid_fraction" ||
          warning.metric == "channel_valid_fraction" ||
          warning.metric == "representation_effective_rank_fraction" ||
          warning.metric == "representation_isotropy_score") {
        require_fraction_dimension(warning.above, "ABOVE", spec.target_id);
      } else if (warning.metric == "total_valid_projection_rows" ||
                 warning.metric == "sample_valid_count" ||
                 warning.metric == "channel_valid_count" ||
                 warning.metric == "valid_latent_rows" ||
                 warning.metric == "tf_pair_count" ||
                 warning.metric == "tf_pair_valid_count" ||
                 warning.metric == "vicreg_global_valid_rows" ||
                 warning.metric == "vicreg_channel_valid_rows" ||
                 warning.metric == "context_starved_sample_count" ||
                 warning.metric == "reduced_targets_for_context_count" ||
                 warning.metric == "min_context_satisfied_count" ||
                 warning.metric == "representation_embedding_dim") {
        require_non_negative_count_threshold(warning.above, "ABOVE",
                                             spec.target_id);
      } else {
        require_non_negative_dimension(warning.above, "ABOVE", warning.metric,
                                       spec.target_id);
      }
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
      if (warning.metric == "mean_adapter_valid_channel_time_fraction" ||
          warning.metric == "finite_parameter_check" ||
          warning.metric == "gradients_finite" ||
          warning.metric == "sample_valid_fraction" ||
          warning.metric == "channel_valid_fraction" ||
          warning.metric == "representation_effective_rank_fraction" ||
          warning.metric == "representation_isotropy_score") {
        require_fraction_dimension(warning.below, "BELOW", spec.target_id);
      } else if (warning.metric == "total_valid_projection_rows" ||
                 warning.metric == "sample_valid_count" ||
                 warning.metric == "channel_valid_count" ||
                 warning.metric == "valid_latent_rows" ||
                 warning.metric == "tf_pair_count" ||
                 warning.metric == "tf_pair_valid_count" ||
                 warning.metric == "vicreg_global_valid_rows" ||
                 warning.metric == "vicreg_channel_valid_rows" ||
                 warning.metric == "context_starved_sample_count" ||
                 warning.metric == "reduced_targets_for_context_count" ||
                 warning.metric == "min_context_satisfied_count" ||
                 warning.metric == "representation_embedding_dim") {
        require_non_negative_count_threshold(warning.below, "BELOW",
                                             spec.target_id);
      } else {
        require_non_negative_dimension(warning.below, "BELOW", warning.metric,
                                       spec.target_id);
      }
    }
  } else if (warning.kind == "runtime_health") {
    warning.use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::optional(block, "USE", "target_supervision"));
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "finite_parameter_check",
        "nonfinite_output_count",
        "checkpoint_written",
        "model_state_mutated",
        "representation_checkpoint_loaded",
        "mdn_checkpoint_loaded",
        "grad_norm_max_pre_clip",
        "grad_norm_clip_ratio",
        "max_grad_norm",
        "sigma_min",
        "sigma_mean",
        "sigma_max",
        "sigma_min_valid",
        "sigma_mean_valid",
        "sigma_max_valid",
        "mixture_entropy",
        "valid_target_fraction",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN runtime_health METRIC is "
          "unsupported for " +
          spec.target_id);
    }
    const auto above_raw = kv::optional(block, "ABOVE", "");
    const auto below_raw = kv::optional(block, "BELOW", "");
    const bool has_above = !kv::trim(above_raw).empty();
    const bool has_below = !kv::trim(below_raw).empty();
    if (has_above == has_below) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN runtime_health requires exactly one "
          "of ABOVE or BELOW for " +
          spec.target_id);
    }
    if (has_above) {
      warning.above = kv::parse_double(above_raw);
    }
    if (has_below) {
      warning.below = kv::parse_double(below_raw);
    }
  } else if (warning.kind == "mdn_distribution_calibration") {
    warning.use = cuwacunu::hero::lattice::exposure::parse_exposure_use(
        kv::optional(block, "USE", "evaluation_metric"));
    warning.metric = kv::lowercase(kv::required(block, "METRIC"));
    const std::set<std::string> allowed_metrics{
        "mean_nll",
        "mean_nll_per_channel_max",
        "mean_nll_per_target_feature_max",
        "mean_nll_per_channel_target_feature_max",
        "per_node_mean_nll_max",
        "crps",
        "pit_ks_statistic",
        "predictive_interval_coverage_error",
        "tail_coverage_error",
        "calibration_slope_error",
    };
    if (!allowed_metrics.count(warning.metric)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN mdn_distribution_calibration METRIC "
          "is unsupported for " +
          spec.target_id);
    }
    if (!kv::trim(kv::optional(block, "BELOW", "")).empty()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_WARN mdn_distribution_calibration uses "
          "ABOVE for " +
          spec.target_id);
    }
    warning.above = kv::parse_double(kv::required(block, "ABOVE"));
    if (warning.metric == "pit_ks_statistic" ||
        warning.metric == "predictive_interval_coverage_error" ||
        warning.metric == "tail_coverage_error") {
      require_fraction_dimension(warning.above, "ABOVE", spec.target_id);
    } else {
      require_non_negative_dimension(warning.above, "ABOVE", warning.metric,
                                     spec.target_id);
    }
  } else {
    throw std::runtime_error("[lattice_target] unsupported LATTICE_WARN KIND " +
                             warning.kind + " for " + spec.target_id);
  }

  validate_lattice_warning_dimensions(warning, spec.target_id);
  spec.warning_specs.push_back(std::move(warning));
}

inline void
validate_lattice_target_spec_lowered(const lattice_target_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  validate_lattice_target_class(kv::lowercase(kv::trim(spec.target_class)),
                                spec.target_id);
  const bool artifact_readiness =
      is_artifact_readiness_target_class(spec.target_class);
  if (artifact_readiness) {
    if (spec.target_kind_applicable) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must set "
          "target_kind_applicable=false for " +
          spec.target_id);
    }
    if (spec.kind != lattice_target_kind_t::not_applicable) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must use "
          "kind=not_applicable for " +
          spec.target_id);
    }
    if (spec.subject_fact_family.empty()) {
      throw std::runtime_error(
          "[lattice_target] TARGET_CLASS=artifact_readiness requires "
          "SUBJECT_FACT_FAMILY for " +
          spec.target_id);
    }
    if (spec.proof_kind.empty()) {
      throw std::runtime_error(
          "[lattice_target] TARGET_CLASS=artifact_readiness requires "
          "PROOF_KIND for " +
          spec.target_id);
    }
    validate_artifact_readiness_proof_template(spec.subject_fact_family,
                                               spec.proof_kind, spec.target_id);
    if (kv::lowercase(kv::trim(spec.plan_mode)) != "none") {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must use "
          "WAVE_MODE=none for " +
          spec.target_id);
    }
    if (spec.max_waves != 0) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets must use "
          "PLAN_MAX_ATTEMPTS=0 for " +
          spec.target_id);
    }
  } else if (!spec.target_kind_applicable ||
             spec.kind == lattice_target_kind_t::not_applicable) {
    throw std::runtime_error("[lattice_target] non-artifact targets must set "
                             "target_kind_applicable=true and a concrete "
                             "TARGET_KIND for " +
                             spec.target_id);
  }
  const auto source_range = kv::lowercase(kv::trim(spec.source_range));
  const auto checkpoint_source =
      kv::lowercase(kv::trim(std::string(spec.checkpoint_source)));
  const std::string latest_prefix = "latest_satisfying:";
  if (artifact_readiness && checkpoint_source != "none") {
    throw std::runtime_error(
        "[lattice_target] artifact_readiness targets must use "
        "CHECKPOINT_SOURCE=none for " +
        spec.target_id);
  }
  if (!artifact_readiness && checkpoint_source != "output_checkpoint" &&
      checkpoint_source.rfind(latest_prefix, 0) != 0) {
    throw std::runtime_error(
        "[lattice_target] CHECKPOINT_SOURCE must be output_checkpoint or "
        "latest_satisfying:<target_id> for " +
        spec.target_id);
  }
  if (!artifact_readiness && checkpoint_source.rfind(latest_prefix, 0) == 0 &&
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
  if (artifact_readiness) {
    if (!kv::trim(spec.evaluated_checkpoint_source).empty() ||
        !kv::trim(spec.plan_input_mdn_checkpoint).empty() ||
        !kv::trim(spec.plan_input_representation_checkpoint).empty()) {
      throw std::runtime_error(
          "[lattice_target] artifact_readiness targets cannot declare "
          "checkpoint source or plan checkpoint inputs for " +
          spec.target_id);
    }
  } else {
    validate_latest_satisfying(spec.evaluated_checkpoint_source,
                               "EVALUATED_CHECKPOINT_SOURCE");
    validate_latest_satisfying(spec.plan_input_mdn_checkpoint,
                               "PLAN_INPUT_MDN_CHECKPOINT");
    validate_latest_satisfying(spec.plan_input_representation_checkpoint,
                               "PLAN_INPUT_REPRESENTATION_CHECKPOINT");
  }
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
  require_non_negative_count(spec.min_optimizer_steps, "MIN_OPTIMIZER_STEPS",
                             spec.target_id);
  require_fraction_dimension(spec.min_valid_target_fraction,
                             "MIN_VALID_TARGET_FRACTION", spec.target_id);
  require_fraction_dimension(spec.min_observed_input_coverage,
                             "MIN_OBSERVED_INPUT_COVERAGE", spec.target_id);
  require_fraction_dimension(spec.min_target_supervision_coverage,
                             "MIN_TARGET_SUPERVISION_COVERAGE", spec.target_id);
  require_fraction_dimension(spec.min_evaluation_metric_coverage,
                             "MIN_EVALUATION_METRIC_COVERAGE", spec.target_id);
  require_non_negative_count(spec.max_waves, "MAX_WAVES", spec.target_id);
  require_non_negative_count(spec.require_active_node_head_count,
                             "REQUIRE_ACTIVE_NODE_HEAD_COUNT", spec.target_id);
  require_non_negative_count(spec.require_trained_node_head_count,
                             "REQUIRE_TRAINED_NODE_HEAD_COUNT", spec.target_id);
  require_non_negative_count(spec.require_evaluated_node_head_count,
                             "REQUIRE_EVALUATED_NODE_HEAD_COUNT",
                             spec.target_id);
  validate_artifact_readiness_no_training_knobs(spec);
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
    validate_lattice_warning_dimensions(warning, spec.target_id);
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
append_ordered_string_vector_text(std::ostringstream &out,
                                  const std::string &name,
                                  const std::vector<std::string> &values) {
  out << name << "=";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  out << "\n";
}

inline void append_path_vector_text(std::ostringstream &out,
                                    const std::string &name,
                                    std::vector<std::filesystem::path> values) {
  std::vector<std::string> strings;
  strings.reserve(values.size());
  for (const auto &value : values) {
    strings.push_back(value.lexically_normal().string());
  }
  append_string_vector_text(out, name, std::move(strings));
}

inline void append_interval_text(
    std::ostringstream &out, const std::string &name,
    cuwacunu::hero::lattice::exposure::anchor_interval_t interval) {
  out << name << ".begin=" << interval.begin << "\n";
  out << name << ".end=" << interval.end << "\n";
  out << name << ".length=" << interval.length() << "\n";
}

inline void append_interval_vector_text(
    std::ostringstream &out, const std::string &name,
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &intervals) {
  out << name << ".count=" << intervals.size() << "\n";
  for (std::size_t i = 0; i < intervals.size(); ++i) {
    append_interval_text(out, name + "." + std::to_string(i), intervals[i]);
  }
}

inline void append_exposure_fact_vector_text(
    std::ostringstream &out, const std::string &name,
    const std::vector<
        cuwacunu::hero::lattice::exposure::lattice_exposure_fact_t>
        &facts) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  out << name << ".count=" << facts.size() << "\n";
  for (std::size_t i = 0; i < facts.size(); ++i) {
    const auto prefix = name + "." + std::to_string(i);
    out << prefix << ".digest=" << exposure::exposure_fact_digest(facts[i])
        << "\n";
    out << prefix << ".canonical.begin\n";
    out << exposure::canonical_exposure_fact_text(facts[i]);
    out << prefix << ".canonical.end\n";
  }
}

inline void append_load_summary_text(
    std::ostringstream &out, const std::string &prefix,
    const cuwacunu::hero::lattice::exposure::exposure_load_summary_t
        &summary) {
  append_interval_text(out, prefix + ".target_range", summary.target_range);
  out << prefix << ".use="
      << cuwacunu::hero::lattice::exposure::exposure_use_name(summary.use)
      << "\n";
  out << prefix << ".component_scope=" << summary.component_scope << "\n";
  out << prefix
      << ".target_component_family_id=" << summary.target_component_family_id
      << "\n";
  out << prefix << ".require_mutated_component="
      << (summary.require_mutated_component ? "true" : "false") << "\n";
  out << prefix
      << ".unique_coverage_algebra=" << summary.unique_coverage_algebra << "\n";
  out << prefix << ".load_algebra=" << summary.load_algebra << "\n";
  out << prefix << ".unique_coverage_unit=" << summary.unique_coverage_unit
      << "\n";
  out << prefix << ".load_unit=" << summary.load_unit << "\n";
  out << prefix << ".anchor_event_unit=" << summary.anchor_event_unit << "\n";
  out << prefix << ".unique_covered_anchors=" << summary.unique_covered_anchors
      << "\n";
  out << prefix
      << ".unique_coverage_fraction=" << summary.unique_coverage_fraction
      << "\n";
  out << prefix << ".loaded_anchor_events=" << summary.loaded_anchor_events
      << "\n";
  out << prefix << ".cursor_exposure_load=" << summary.cursor_exposure_load
      << "\n";
  out << prefix << ".fact_count=" << summary.fact_count << "\n";
  out << prefix << ".optimizer_steps_total=" << summary.optimizer_steps_total
      << "\n";
  out << prefix << ".optimizer_steps_per_unique_anchor="
      << summary.optimizer_steps_per_unique_anchor << "\n";
  out << prefix << ".optimizer_steps_per_cursor_epoch="
      << summary.optimizer_steps_per_cursor_epoch << "\n";
  out << prefix
      << ".valid_target_count_total=" << summary.valid_target_count_total
      << "\n";
  out << prefix
      << ".valid_target_cursor_epochs=" << summary.valid_target_cursor_epochs
      << "\n";
  out << prefix << ".valid_target_success_count_for_uncertainty="
      << summary.valid_target_success_count_for_uncertainty << "\n";
  out << prefix << ".valid_target_opportunity_count_for_uncertainty="
      << summary.valid_target_opportunity_count_for_uncertainty << "\n";
  out << prefix << ".valid_target_fraction_estimate="
      << summary.valid_target_fraction_estimate << "\n";
  out << prefix << ".valid_target_wilson_lower_95="
      << summary.valid_target_wilson_lower_95 << "\n";
  out << prefix << ".valid_target_wilson_upper_95="
      << summary.valid_target_wilson_upper_95 << "\n";
}

inline void append_exposure_use_set_text(
    std::ostringstream &out, const std::string &prefix,
    const cuwacunu::hero::lattice::exposure::exposure_use_set_t &use) {
  out << prefix << ".observed_input=" << (use.observed_input ? "true" : "false")
      << "\n";
  out << prefix
      << ".target_supervision=" << (use.target_supervision ? "true" : "false")
      << "\n";
  out << prefix
      << ".evaluation_metric=" << (use.evaluation_metric ? "true" : "false")
      << "\n";
  out << prefix
      << ".selection_signal=" << (use.selection_signal ? "true" : "false")
      << "\n";
  out << prefix
      << ".mutated_component=" << (use.mutated_component ? "true" : "false")
      << "\n";
}

inline void append_source_key_window_audit_text(
    std::ostringstream &out, const std::string &prefix,
    const cuwacunu::hero::lattice::exposure::source_key_window_audit_t
        &audit) {
  out << prefix << ".schema=" << audit.schema << "\n";
  out << prefix << ".available=" << (audit.available ? "true" : "false")
      << "\n";
  out << prefix << ".precision=" << audit.precision << "\n";
  out << prefix << ".complete=" << (audit.complete ? "true" : "false") << "\n";
  out << prefix << ".numeric=" << (audit.numeric ? "true" : "false") << "\n";
  out << prefix << ".internally_monotone="
      << (audit.internally_monotone ? "true" : "false") << "\n";
  out << prefix
      << ".order_preserving=" << (audit.order_preserving ? "true" : "false")
      << "\n";
  out << prefix << ".affine_step_available="
      << (audit.affine_step_available ? "true" : "false") << "\n";
  out << prefix << ".reference_key_step=" << audit.reference_key_step << "\n";
  out << prefix
      << ".affine_consistent=" << (audit.affine_consistent ? "true" : "false")
      << "\n";
  out << prefix
      << ".missing_endpoint_pair_count=" << audit.missing_endpoint_pair_count
      << "\n";
  out << prefix
      << ".irregular_key_warning_count=" << audit.irregular_key_warning_count
      << "\n";
  out << prefix
      << ".source_key_gap_warning_count=" << audit.source_key_gap_warning_count
      << "\n";
  out << prefix << ".row_source_key_mismatch_count="
      << audit.row_source_key_mismatch_count << "\n";
  out << prefix << ".source_key_gap_found="
      << (audit.source_key_gap_found ? "true" : "false") << "\n";
  out << prefix << ".row_source_key_mismatch_found="
      << (audit.row_source_key_mismatch_found ? "true" : "false") << "\n";
  append_string_vector_text(out, prefix + ".issues", audit.issues);
  out << prefix << ".endpoints.count=" << audit.endpoints.size() << "\n";
  for (std::size_t i = 0; i < audit.endpoints.size(); ++i) {
    const auto &endpoint = audit.endpoints[i];
    const auto endpoint_prefix = prefix + ".endpoint." + std::to_string(i);
    out << endpoint_prefix << ".label=" << endpoint.label << "\n";
    out << endpoint_prefix << ".row=" << endpoint.row << "\n";
    out << endpoint_prefix << ".key=" << endpoint.key << "\n";
    out << endpoint_prefix
        << ".numeric=" << (endpoint.numeric ? "true" : "false") << "\n";
    out << endpoint_prefix << ".numeric_key=" << endpoint.numeric_key << "\n";
  }
}

inline void append_node_support_row_text(
    std::ostringstream &out, const std::string &prefix,
    const cuwacunu::hero::lattice::exposure::node_support_row_t &row) {
  out << prefix
      << ".parent_exposure_fact_digest=" << row.parent_exposure_fact_digest
      << "\n";
  out << prefix << ".node_id=" << row.node_id << "\n";
  out << prefix << ".node_index=" << row.node_index << "\n";
  append_exposure_use_set_text(out, prefix + ".use", row.use);
  out << prefix << ".routed_row_count=" << row.routed_row_count << "\n";
  out << prefix << ".active_row_count=" << row.active_row_count << "\n";
  out << prefix << ".trained_row_count=" << row.trained_row_count << "\n";
  out << prefix << ".evaluated_row_count=" << row.evaluated_row_count << "\n";
  out << prefix << ".valid_target_count=" << row.valid_target_count << "\n";
  out << prefix << ".valid_target_denominator=" << row.valid_target_denominator
      << "\n";
  out << prefix << ".valid_target_fraction=" << row.valid_target_fraction
      << "\n";
}

inline void append_node_support_summary_text(
    std::ostringstream &out, const std::string &prefix,
    const cuwacunu::hero::lattice::exposure::node_support_summary_t
        &summary) {
  out << prefix << ".schema=" << summary.schema << "\n";
  out << prefix
      << ".target_component_family_id=" << summary.target_component_family_id
      << "\n";
  out << prefix << ".split_name=" << summary.split_name << "\n";
  append_exposure_use_set_text(out, prefix + ".use", summary.use);
  out << prefix << ".support_rows.count=" << summary.support_rows.size()
      << "\n";
  for (std::size_t i = 0; i < summary.support_rows.size(); ++i) {
    append_node_support_row_text(out,
                                 prefix + ".support_row." + std::to_string(i),
                                 summary.support_rows[i]);
  }
  out << prefix << ".node_count=" << summary.node_count << "\n";
  out << prefix << ".unique_node_count=" << summary.unique_node_count << "\n";
  out << prefix << ".observed_input_support_row_count="
      << summary.observed_input_support_row_count << "\n";
  out << prefix << ".target_supervision_support_row_count="
      << summary.target_supervision_support_row_count << "\n";
  out << prefix << ".evaluation_metric_support_row_count="
      << summary.evaluation_metric_support_row_count << "\n";
  out << prefix << ".selection_signal_support_row_count="
      << summary.selection_signal_support_row_count << "\n";
  out << prefix
      << ".mutating_support_row_count=" << summary.mutating_support_row_count
      << "\n";
  out << prefix << ".non_mutating_support_row_count="
      << summary.non_mutating_support_row_count << "\n";
  out << prefix << ".routed_row_count_total=" << summary.routed_row_count_total
      << "\n";
  out << prefix << ".active_row_count_total=" << summary.active_row_count_total
      << "\n";
  out << prefix
      << ".trained_row_count_total=" << summary.trained_row_count_total << "\n";
  out << prefix
      << ".evaluated_row_count_total=" << summary.evaluated_row_count_total
      << "\n";
  out << prefix
      << ".valid_target_count_total=" << summary.valid_target_count_total
      << "\n";
  out << prefix << ".valid_target_opportunity_count_total="
      << summary.valid_target_opportunity_count_total << "\n";
  out << prefix << ".valid_target_fraction_estimate="
      << summary.valid_target_fraction_estimate << "\n";
  out << prefix << ".valid_target_wilson_lower_95="
      << summary.valid_target_wilson_lower_95 << "\n";
  out << prefix << ".valid_target_wilson_upper_95="
      << summary.valid_target_wilson_upper_95 << "\n";
  out << prefix << ".finite_valid_target_fraction_count="
      << summary.finite_valid_target_fraction_count << "\n";
  out << prefix
      << ".min_valid_target_fraction=" << summary.min_valid_target_fraction
      << "\n";
  out << prefix
      << ".max_valid_target_fraction=" << summary.max_valid_target_fraction
      << "\n";
  out << prefix
      << ".mean_valid_target_fraction=" << summary.mean_valid_target_fraction
      << "\n";
  out << prefix << ".weakest_valid_target_node_id="
      << summary.weakest_valid_target_node_id << "\n";
  out << prefix << ".weakest_valid_target_node_index="
      << summary.weakest_valid_target_node_index << "\n";
  out << prefix
      << ".weakest_valid_target_count=" << summary.weakest_valid_target_count
      << "\n";
  out << prefix << ".weakest_valid_target_denominator="
      << summary.weakest_valid_target_denominator << "\n";
  out << prefix << ".weakest_valid_target_fraction="
      << summary.weakest_valid_target_fraction << "\n";
  out << prefix << ".weakest_valid_target_wilson_lower_95="
      << summary.weakest_valid_target_wilson_lower_95 << "\n";
  out << prefix << ".weakest_valid_target_wilson_upper_95="
      << summary.weakest_valid_target_wilson_upper_95 << "\n";
  out << prefix
      << ".valid_target_count_mean=" << summary.valid_target_count_mean << "\n";
  out << prefix << ".valid_target_count_coefficient_of_variation="
      << summary.valid_target_count_coefficient_of_variation << "\n";
  out << prefix
      << ".valid_target_count_gini=" << summary.valid_target_count_gini << "\n";
  out << prefix << ".valid_target_count_normalized_entropy="
      << summary.valid_target_count_normalized_entropy << "\n";
}

[[nodiscard]] inline std::string
canonical_lattice_target_proof_certificate_text(
    const lattice_target_proof_certificate_t &proof) {
  std::ostringstream out;
  out << "schema=" << proof.schema << "\n";
  out << "target_id=" << proof.target_id << "\n";
  out << "target_spec_fingerprint=" << proof.target_spec_fingerprint << "\n";
  out << "split_policy_fingerprint=" << proof.split_policy_fingerprint << "\n";

  const auto &proof_context = proof.proof_context;
  out << "proof_context.require_contract_match="
      << (proof_context.require_contract_match ? "true" : "false") << "\n";
  out << "proof_context.require_component_match="
      << (proof_context.require_component_match ? "true" : "false") << "\n";
  out << "proof_context.require_graph_anchor_identity="
      << (proof_context.require_graph_anchor_identity ? "true" : "false")
      << "\n";
  out << "proof_context.active_contract_fingerprint="
      << proof_context.active_contract_fingerprint << "\n";
  out << "proof_context.evidence_contract_fingerprint="
      << proof_context.evidence_contract_fingerprint << "\n";
  out << "proof_context.contract_match="
      << (proof_context.contract_match ? "true" : "false") << "\n";
  out << "proof_context.expected_component_fingerprint="
      << proof_context.expected_component_fingerprint << "\n";
  out << "proof_context.evidence_component_fingerprint="
      << proof_context.evidence_component_fingerprint << "\n";
  out << "proof_context.component_match="
      << (proof_context.component_match ? "true" : "false") << "\n";
  out << "proof_context.active_graph_order_fingerprint="
      << proof_context.active_graph_order_fingerprint << "\n";
  out << "proof_context.evidence_graph_order_fingerprint="
      << proof_context.evidence_graph_order_fingerprint << "\n";
  out << "proof_context.graph_order_match="
      << (proof_context.graph_order_match ? "true" : "false") << "\n";
  out << "proof_context.active_source_cursor_token="
      << proof_context.active_source_cursor_token << "\n";
  out << "proof_context.evidence_source_cursor_token="
      << proof_context.evidence_source_cursor_token << "\n";
  out << "proof_context.source_cursor_match="
      << (proof_context.source_cursor_match ? "true" : "false") << "\n";
  out << "proof_context.component_spawn_schema="
      << proof_context.component_spawn_schema << "\n";
  out << "proof_context.component_family_id="
      << proof_context.component_family_id << "\n";
  out << "proof_context.target_spec_fingerprint="
      << proof_context.target_spec_fingerprint << "\n";
  out << "proof_context.split_policy_fingerprint="
      << proof_context.split_policy_fingerprint << "\n";
  out << "proof_context.config_bundle_id=" << proof_context.config_bundle_id
      << "\n";
  out << "proof_context.config_receipt_id=" << proof_context.config_receipt_id
      << "\n";
  out << "proof_context.component_spawn_registry_id="
      << proof_context.component_spawn_registry_id << "\n";
  out << "proof_context.component_spawn_id=" << proof_context.component_spawn_id
      << "\n";
  out << "proof_context.component_spawn_label="
      << proof_context.component_spawn_label << "\n";
  out << "proof_context.evidence_component_spawn_fingerprint="
      << proof_context.evidence_component_spawn_fingerprint << "\n";
  out << "proof_context.computed_component_spawn_fingerprint="
      << proof_context.computed_component_spawn_fingerprint << "\n";
  out << "proof_context.config_receipt_available="
      << (proof_context.config_receipt_available ? "true" : "false") << "\n";
  out << "proof_context.spawn_id_readability_hint_only="
      << (proof_context.spawn_id_readability_hint_only ? "true" : "false")
      << "\n";
  append_string_vector_text(out, "proof_context.provenance_warnings",
                            proof_context.provenance_warnings);

  auto dependencies = proof.dependencies;
  std::sort(
      dependencies.begin(), dependencies.end(),
      [](const auto &a, const auto &b) {
        auto key = [](const auto &dependency) {
          std::ostringstream out;
          out << dependency.kind << "\n"
              << dependency.source_target_id << "\n"
              << dependency.status << "\n"
              << dependency.expected_mdn_checkpoint.lexically_normal().string()
              << "\n"
              << dependency.actual_mdn_checkpoint.lexically_normal().string()
              << "\n"
              << (dependency.mdn_checkpoint_match ? "1" : "0") << "\n"
              << dependency.expected_representation_checkpoint
                     .lexically_normal()
                     .string()
              << "\n"
              << dependency.actual_representation_checkpoint.lexically_normal()
                     .string()
              << "\n"
              << (dependency.representation_checkpoint_match ? "1" : "0");
          return out.str();
        };
        return key(a) < key(b);
      });
  out << "dependencies.count=" << dependencies.size() << "\n";
  for (std::size_t i = 0; i < dependencies.size(); ++i) {
    const auto &dependency = dependencies[i];
    const auto prefix = "dependency." + std::to_string(i);
    out << prefix << ".kind=" << dependency.kind << "\n";
    out << prefix << ".source_target_id=" << dependency.source_target_id
        << "\n";
    out << prefix << ".status=" << dependency.status << "\n";
    out << prefix << ".expected_mdn_checkpoint="
        << dependency.expected_mdn_checkpoint.lexically_normal().string()
        << "\n";
    out << prefix << ".actual_mdn_checkpoint="
        << dependency.actual_mdn_checkpoint.lexically_normal().string() << "\n";
    out << prefix << ".mdn_checkpoint_match="
        << (dependency.mdn_checkpoint_match ? "true" : "false") << "\n";
    out << prefix << ".expected_representation_checkpoint="
        << dependency.expected_representation_checkpoint.lexically_normal()
               .string()
        << "\n";
    out << prefix << ".actual_representation_checkpoint="
        << dependency.actual_representation_checkpoint.lexically_normal()
               .string()
        << "\n";
    out << prefix << ".representation_checkpoint_match="
        << (dependency.representation_checkpoint_match ? "true" : "false")
        << "\n";
  }

  auto artifacts = proof.artifacts;
  std::sort(
      artifacts.begin(), artifacts.end(), [](const auto &a, const auto &b) {
        auto key = [](const auto &artifact) {
          std::ostringstream out;
          out << artifact.proof_kind << "\n"
              << (artifact.proof_template_bound ? "1" : "0") << "\n"
              << artifact.proof_template_claim << "\n"
              << artifact.fact_family << "\n"
              << artifact.fact_schema << "\n"
              << artifact.fact_type << "\n"
              << artifact.fact_digest << "\n"
              << artifact.fact_identity_contract_schema << "\n"
              << artifact.fact_identity_contract_id << "\n"
              << (artifact.fact_identity_contract_bound ? "1" : "0") << "\n"
              << (artifact.fact_identity_envelope_complete ? "1" : "0") << "\n"
              << (artifact.row_index_interval_authority ? "1" : "0") << "\n"
              << (artifact.source_key_window_audit_only ? "1" : "0") << "\n"
              << (artifact.fact_identity_target_kind_authority ? "1" : "0")
              << "\n"
              << (artifact.fact_identity_runtime_wave_authority ? "1" : "0")
              << "\n"
              << (artifact.fact_identity_marshal_reachability ? "1" : "0")
              << "\n"
              << (artifact.fact_identity_policy_gate_authority ? "1" : "0")
              << "\n"
              << artifact.parent_exposure_fact_digest << "\n"
              << artifact.job_id << "\n"
              << artifact.wave_id << "\n"
              << artifact.protocol_id << "\n"
              << artifact.contract_fingerprint << "\n"
              << artifact.component << "\n"
              << artifact.component_assembly_fingerprint << "\n"
              << artifact.graph_order_fingerprint << "\n"
              << artifact.source_cursor_token << "\n"
              << artifact.split_policy_fingerprint << "\n"
              << artifact.split_name << "\n";
          append_interval_text(out, "anchor_range", artifact.anchor_range);
          append_interval_text(out, "completed_anchor_range",
                               artifact.completed_anchor_range);
          out << (artifact.identity_match ? "1" : "0") << "\n"
              << (artifact.artifact_evidence ? "1" : "0") << "\n"
              << (artifact.deterministic_artifact ? "1" : "0") << "\n"
              << (artifact.visibility_only ? "1" : "0") << "\n"
              << (artifact.authority_clean ? "1" : "0") << "\n"
              << (artifact.readiness_authority ? "1" : "0") << "\n"
              << (artifact.quality_authority ? "1" : "0") << "\n"
              << (artifact.performance_authority ? "1" : "0") << "\n"
              << (artifact.checkpoint_selector ? "1" : "0") << "\n"
              << (artifact.coverage_authority ? "1" : "0") << "\n"
              << (artifact.leakage_authority ? "1" : "0") << "\n"
              << (artifact.contract_identity_authority ? "1" : "0") << "\n"
              << (artifact.allocation_authority ? "1" : "0") << "\n"
              << (artifact.execution_authority ? "1" : "0") << "\n"
              << (artifact.market_readiness_authority ? "1" : "0") << "\n"
              << (artifact.deployment_authority ? "1" : "0") << "\n"
              << (artifact.policy_gate ? "1" : "0") << "\n"
              << (artifact.target_dependency_authority ? "1" : "0") << "\n"
              << (artifact.runtime_wave_authority ? "1" : "0") << "\n"
              << (artifact.marshal_reachability ? "1" : "0") << "\n"
              << (artifact.checkpoint_source_authority ? "1" : "0") << "\n"
              << (artifact.plan_checkpoint_input_authority ? "1" : "0") << "\n"
              << (artifact.model_state_mutation ? "1" : "0") << "\n"
              << (artifact.raw_potential_tradable_return ? "1" : "0") << "\n"
              << (artifact.replay_executor ? "1" : "0") << "\n"
              << (artifact.lineage_bound ? "1" : "0") << "\n"
              << (artifact.passed ? "1" : "0") << "\n";
          append_string_vector_text(out, "issues", artifact.issues);
          return out.str();
        };
        return key(a) < key(b);
      });
  out << "artifacts.count=" << artifacts.size() << "\n";
  for (std::size_t i = 0; i < artifacts.size(); ++i) {
    const auto &artifact = artifacts[i];
    const auto prefix = "artifact." + std::to_string(i);
    out << prefix << ".proof_kind=" << artifact.proof_kind << "\n";
    out << prefix << ".proof_template_bound="
        << (artifact.proof_template_bound ? "true" : "false") << "\n";
    out << prefix << ".proof_template_claim=" << artifact.proof_template_claim
        << "\n";
    out << prefix << ".fact_family=" << artifact.fact_family << "\n";
    out << prefix << ".fact_schema=" << artifact.fact_schema << "\n";
    out << prefix << ".fact_type=" << artifact.fact_type << "\n";
    out << prefix << ".fact_digest=" << artifact.fact_digest << "\n";
    out << prefix << ".fact_identity_contract_schema="
        << artifact.fact_identity_contract_schema << "\n";
    out << prefix
        << ".fact_identity_contract_id=" << artifact.fact_identity_contract_id
        << "\n";
    out << prefix << ".fact_identity_contract_bound="
        << (artifact.fact_identity_contract_bound ? "true" : "false") << "\n";
    out << prefix << ".fact_identity_envelope_complete="
        << (artifact.fact_identity_envelope_complete ? "true" : "false")
        << "\n";
    out << prefix << ".row_index_interval_authority="
        << (artifact.row_index_interval_authority ? "true" : "false") << "\n";
    out << prefix << ".source_key_window_audit_only="
        << (artifact.source_key_window_audit_only ? "true" : "false") << "\n";
    out << prefix << ".fact_identity_target_kind_authority="
        << (artifact.fact_identity_target_kind_authority ? "true" : "false")
        << "\n";
    out << prefix << ".fact_identity_runtime_wave_authority="
        << (artifact.fact_identity_runtime_wave_authority ? "true" : "false")
        << "\n";
    out << prefix << ".fact_identity_marshal_reachability="
        << (artifact.fact_identity_marshal_reachability ? "true" : "false")
        << "\n";
    out << prefix << ".fact_identity_policy_gate_authority="
        << (artifact.fact_identity_policy_gate_authority ? "true" : "false")
        << "\n";
    out << prefix << ".parent_exposure_fact_digest="
        << artifact.parent_exposure_fact_digest << "\n";
    out << prefix << ".job_id=" << artifact.job_id << "\n";
    out << prefix << ".wave_id=" << artifact.wave_id << "\n";
    out << prefix << ".protocol_id=" << artifact.protocol_id << "\n";
    out << prefix << ".contract_fingerprint=" << artifact.contract_fingerprint
        << "\n";
    out << prefix << ".component=" << artifact.component << "\n";
    out << prefix << ".component_assembly_fingerprint="
        << artifact.component_assembly_fingerprint << "\n";
    out << prefix
        << ".graph_order_fingerprint=" << artifact.graph_order_fingerprint
        << "\n";
    out << prefix << ".source_cursor_token=" << artifact.source_cursor_token
        << "\n";
    out << prefix
        << ".split_policy_fingerprint=" << artifact.split_policy_fingerprint
        << "\n";
    out << prefix << ".split_name=" << artifact.split_name << "\n";
    append_interval_text(out, prefix + ".anchor_range", artifact.anchor_range);
    append_interval_text(out, prefix + ".completed_anchor_range",
                         artifact.completed_anchor_range);
    out << prefix
        << ".identity_match=" << (artifact.identity_match ? "true" : "false")
        << "\n";
    out << prefix << ".artifact_evidence="
        << (artifact.artifact_evidence ? "true" : "false") << "\n";
    out << prefix << ".deterministic_artifact="
        << (artifact.deterministic_artifact ? "true" : "false") << "\n";
    out << prefix
        << ".visibility_only=" << (artifact.visibility_only ? "true" : "false")
        << "\n";
    out << prefix
        << ".authority_clean=" << (artifact.authority_clean ? "true" : "false")
        << "\n";
    out << prefix << ".readiness_authority="
        << (artifact.readiness_authority ? "true" : "false") << "\n";
    out << prefix << ".quality_authority="
        << (artifact.quality_authority ? "true" : "false") << "\n";
    out << prefix << ".performance_authority="
        << (artifact.performance_authority ? "true" : "false") << "\n";
    out << prefix << ".checkpoint_selector="
        << (artifact.checkpoint_selector ? "true" : "false") << "\n";
    out << prefix << ".coverage_authority="
        << (artifact.coverage_authority ? "true" : "false") << "\n";
    out << prefix << ".leakage_authority="
        << (artifact.leakage_authority ? "true" : "false") << "\n";
    out << prefix << ".contract_identity_authority="
        << (artifact.contract_identity_authority ? "true" : "false") << "\n";
    out << prefix << ".allocation_authority="
        << (artifact.allocation_authority ? "true" : "false") << "\n";
    out << prefix << ".execution_authority="
        << (artifact.execution_authority ? "true" : "false") << "\n";
    out << prefix << ".market_readiness_authority="
        << (artifact.market_readiness_authority ? "true" : "false") << "\n";
    out << prefix << ".deployment_authority="
        << (artifact.deployment_authority ? "true" : "false") << "\n";
    out << prefix
        << ".policy_gate=" << (artifact.policy_gate ? "true" : "false") << "\n";
    out << prefix << ".target_dependency_authority="
        << (artifact.target_dependency_authority ? "true" : "false") << "\n";
    out << prefix << ".runtime_wave_authority="
        << (artifact.runtime_wave_authority ? "true" : "false") << "\n";
    out << prefix << ".marshal_reachability="
        << (artifact.marshal_reachability ? "true" : "false") << "\n";
    out << prefix << ".checkpoint_source_authority="
        << (artifact.checkpoint_source_authority ? "true" : "false") << "\n";
    out << prefix << ".plan_checkpoint_input_authority="
        << (artifact.plan_checkpoint_input_authority ? "true" : "false")
        << "\n";
    out << prefix << ".model_state_mutation="
        << (artifact.model_state_mutation ? "true" : "false") << "\n";
    out << prefix << ".raw_potential_tradable_return="
        << (artifact.raw_potential_tradable_return ? "true" : "false") << "\n";
    out << prefix
        << ".replay_executor=" << (artifact.replay_executor ? "true" : "false")
        << "\n";
    out << prefix
        << ".lineage_bound=" << (artifact.lineage_bound ? "true" : "false")
        << "\n";
    out << prefix << ".passed=" << (artifact.passed ? "true" : "false") << "\n";
    append_string_vector_text(out, prefix + ".issues", artifact.issues);
  }

  auto coverage_proofs = proof.coverage;
  std::sort(
      coverage_proofs.begin(), coverage_proofs.end(),
      [](const auto &a, const auto &b) {
        auto key = [](const auto &coverage) {
          std::ostringstream out;
          out << coverage.use << "\n" << coverage.algebra << "\n";
          append_interval_text(out, "target_range", coverage.target_range);
          append_interval_vector_text(out, "contributing_intervals",
                                      coverage.contributing_intervals);
          append_ordered_string_vector_text(out, "contributing_fact_digests",
                                            coverage.contributing_fact_digests);
          append_exposure_fact_vector_text(out, "contributing_facts",
                                           coverage.contributing_facts);
          append_interval_vector_text(out, "covered_intervals",
                                      coverage.covered_intervals);
          append_interval_vector_text(out, "missing_intervals",
                                      coverage.missing_intervals);
          out << coverage.required_fraction << "\n"
              << coverage.actual_fraction << "\n"
              << coverage.covered_anchors << "\n"
              << coverage.missing_anchors << "\n"
              << (coverage.require_mutated_component ? "1" : "0") << "\n"
              << (coverage.passed ? "1" : "0") << "\n";
          append_load_summary_text(out, "load_summary", coverage.load_summary);
          return out.str();
        };
        return key(a) < key(b);
      });
  out << "coverage.count=" << coverage_proofs.size() << "\n";
  for (std::size_t i = 0; i < coverage_proofs.size(); ++i) {
    const auto &coverage = coverage_proofs[i];
    const auto prefix = "coverage." + std::to_string(i);
    out << prefix << ".use=" << coverage.use << "\n";
    out << prefix << ".algebra=" << coverage.algebra << "\n";
    append_interval_text(out, prefix + ".target_range", coverage.target_range);
    append_interval_vector_text(out, prefix + ".contributing_intervals",
                                coverage.contributing_intervals);
    append_ordered_string_vector_text(out,
                                      prefix + ".contributing_fact_digests",
                                      coverage.contributing_fact_digests);
    append_exposure_fact_vector_text(out, prefix + ".contributing_facts",
                                     coverage.contributing_facts);
    append_interval_vector_text(out, prefix + ".covered_intervals",
                                coverage.covered_intervals);
    append_interval_vector_text(out, prefix + ".missing_intervals",
                                coverage.missing_intervals);
    out << prefix << ".required_fraction=" << coverage.required_fraction
        << "\n";
    out << prefix << ".actual_fraction=" << coverage.actual_fraction << "\n";
    out << prefix << ".covered_anchors=" << coverage.covered_anchors << "\n";
    out << prefix << ".missing_anchors=" << coverage.missing_anchors << "\n";
    out << prefix << ".require_mutated_component="
        << (coverage.require_mutated_component ? "true" : "false") << "\n";
    out << prefix << ".passed=" << (coverage.passed ? "true" : "false") << "\n";
    append_load_summary_text(out, prefix + ".load_summary",
                             coverage.load_summary);
  }

  const auto &closure = proof.closure;
  out << "closure.checked=" << (closure.checked ? "true" : "false") << "\n";
  out << "closure.checkpoint_path="
      << closure.checkpoint_path.lexically_normal().string() << "\n";
  out << "closure.complete=" << (closure.complete ? "true" : "false") << "\n";
  out << "closure.fact_count=" << closure.fact_count << "\n";
  out << "closure.resolution_authority=" << closure.resolution_authority
      << "\n";
  out << "closure.root_checkpoint_id=" << closure.root_checkpoint_id << "\n";
  out << "closure.root_checkpoint_file_digest="
      << closure.root_checkpoint_file_digest << "\n";
  append_string_vector_text(out, "closure.identity_mismatches",
                            closure.identity_mismatches);
  append_path_vector_text(out, "closure.unresolved_input_checkpoints",
                          closure.unresolved_input_checkpoints);
  append_string_vector_text(out, "closure.fact_digests", closure.fact_digests);
  out << "closure.causal_exposures.count=" << closure.causal_exposures.size()
      << "\n";
  auto causal_exposures = closure.causal_exposures;
  std::sort(
      causal_exposures.begin(), causal_exposures.end(),
      [](const auto &a, const auto &b) {
        auto key = [](const auto &causal) {
          std::ostringstream out;
          out << causal.fact_digest << "\n"
              << causal.contract_fingerprint << "\n"
              << causal.graph_order_fingerprint << "\n"
              << causal.source_cursor_token << "\n"
              << causal.cursor_domain << "\n"
              << causal.component_assembly_fingerprint << "\n"
              << causal.split_policy_fingerprint << "\n"
              << causal.job_id << "\n"
              << causal.wave_id << "\n"
              << causal.wave_action << "\n"
              << causal.job_status << "\n"
              << causal.runtime_handoff_id << "\n"
              << causal.runtime_handoff_digest << "\n"
              << causal.marshal_target_driver_run_id << "\n"
              << causal.target_component_family_id << "\n"
              << causal.representation_architecture << "\n"
              << causal.representation_contract << "\n"
              << causal.representation_value_shape << "\n"
              << causal.representation_sequence_value_shape << "\n"
              << causal.channel_axis_policy << "\n"
              << causal.temporal_reduction << "\n"
              << causal.input_representation_assembly_id << "\n"
              << causal.context_mode << "\n"
              << causal.context_contract << "\n"
              << causal.context_value_shape << "\n"
              << causal.output_contract << "\n"
              << causal.output_value_shape << "\n";
          append_string_vector_text(out, "uses", causal.uses);
          out << (causal.mutated_component ? "1" : "0") << "\n"
              << causal.split_name << "\n"
              << cuwacunu::hero::lattice::exposure::
                     exposure_split_role_name(causal.split_role)
              << "\n";
          append_interval_text(out, "anchor_range", causal.anchor_range);
          append_interval_text(out, "completed_anchor_range",
                               causal.completed_anchor_range);
          append_interval_text(out, "observed_footprint",
                               causal.observed_footprint);
          append_interval_text(out, "target_footprint",
                               causal.target_footprint);
          out << causal.first_anchor_key << "\n"
              << causal.last_anchor_key << "\n"
              << causal.observed_source_key_begin << "\n"
              << causal.observed_source_key_end << "\n"
              << causal.target_source_key_begin << "\n"
              << causal.target_source_key_end << "\n"
              << causal.source_key_footprint_precision << "\n";
          append_source_key_window_audit_text(out, "source_key_window_audit",
                                              causal.source_key_window_audit);
          out << causal.output_checkpoint.lexically_normal().string() << "\n";
          append_path_vector_text(out, "input_checkpoints",
                                  causal.input_checkpoints);
          return out.str();
        };
        return key(a) < key(b);
      });
  for (std::size_t i = 0; i < causal_exposures.size(); ++i) {
    const auto &causal = causal_exposures[i];
    const auto prefix = "closure.causal_exposure." + std::to_string(i);
    out << prefix << ".fact_digest=" << causal.fact_digest << "\n";
    out << prefix << ".contract_fingerprint=" << causal.contract_fingerprint
        << "\n";
    out << prefix
        << ".graph_order_fingerprint=" << causal.graph_order_fingerprint
        << "\n";
    out << prefix << ".source_cursor_token=" << causal.source_cursor_token
        << "\n";
    out << prefix << ".cursor_domain=" << causal.cursor_domain << "\n";
    out << prefix << ".component_assembly_fingerprint="
        << causal.component_assembly_fingerprint << "\n";
    out << prefix
        << ".split_policy_fingerprint=" << causal.split_policy_fingerprint
        << "\n";
    out << prefix << ".job_id=" << causal.job_id << "\n";
    out << prefix << ".wave_id=" << causal.wave_id << "\n";
    out << prefix << ".wave_action=" << causal.wave_action << "\n";
    out << prefix << ".job_status=" << causal.job_status << "\n";
    out << prefix << ".runtime_handoff_id=" << causal.runtime_handoff_id
        << "\n";
    out << prefix << ".runtime_handoff_digest=" << causal.runtime_handoff_digest
        << "\n";
    out << prefix << ".marshal_target_driver_run_id="
        << causal.marshal_target_driver_run_id << "\n";
    out << prefix
        << ".target_component_family_id=" << causal.target_component_family_id
        << "\n";
    out << prefix
        << ".representation_architecture=" << causal.representation_architecture
        << "\n";
    out << prefix
        << ".representation_contract=" << causal.representation_contract
        << "\n";
    out << prefix
        << ".representation_value_shape=" << causal.representation_value_shape
        << "\n";
    out << prefix << ".representation_sequence_value_shape="
        << causal.representation_sequence_value_shape << "\n";
    out << prefix << ".channel_axis_policy=" << causal.channel_axis_policy
        << "\n";
    out << prefix << ".temporal_reduction=" << causal.temporal_reduction
        << "\n";
    out << prefix << ".input_representation_assembly_id="
        << causal.input_representation_assembly_id << "\n";
    out << prefix << ".context_mode=" << causal.context_mode << "\n";
    out << prefix << ".context_contract=" << causal.context_contract << "\n";
    out << prefix << ".context_value_shape=" << causal.context_value_shape
        << "\n";
    out << prefix << ".output_contract=" << causal.output_contract << "\n";
    out << prefix << ".output_value_shape=" << causal.output_value_shape
        << "\n";
    append_string_vector_text(out, prefix + ".uses", causal.uses);
    out << prefix << ".mutated_component="
        << (causal.mutated_component ? "true" : "false") << "\n";
    out << prefix << ".split_name=" << causal.split_name << "\n";
    out << prefix << ".split_role="
        << cuwacunu::hero::lattice::exposure::exposure_split_role_name(
               causal.split_role)
        << "\n";
    append_interval_text(out, prefix + ".anchor_range", causal.anchor_range);
    append_interval_text(out, prefix + ".completed_anchor_range",
                         causal.completed_anchor_range);
    append_interval_text(out, prefix + ".observed_footprint",
                         causal.observed_footprint);
    append_interval_text(out, prefix + ".target_footprint",
                         causal.target_footprint);
    out << prefix << ".first_anchor_key=" << causal.first_anchor_key << "\n";
    out << prefix << ".last_anchor_key=" << causal.last_anchor_key << "\n";
    out << prefix
        << ".observed_source_key_begin=" << causal.observed_source_key_begin
        << "\n";
    out << prefix
        << ".observed_source_key_end=" << causal.observed_source_key_end
        << "\n";
    out << prefix
        << ".target_source_key_begin=" << causal.target_source_key_begin
        << "\n";
    out << prefix << ".target_source_key_end=" << causal.target_source_key_end
        << "\n";
    out << prefix << ".source_key_footprint_precision="
        << causal.source_key_footprint_precision << "\n";
    append_source_key_window_audit_text(out,
                                        prefix + ".source_key_window_audit",
                                        causal.source_key_window_audit);
    out << prefix << ".output_checkpoint="
        << causal.output_checkpoint.lexically_normal().string() << "\n";
    append_path_vector_text(out, prefix + ".input_checkpoints",
                            causal.input_checkpoints);
  }
  auto checkpoint_previews = closure.checkpoint_identity_previews;
  std::sort(checkpoint_previews.begin(), checkpoint_previews.end(),
            [](const auto &a, const auto &b) {
              const auto path_a = a.checkpoint_path.lexically_normal().string();
              const auto path_b = b.checkpoint_path.lexically_normal().string();
              if (path_a != path_b) {
                return path_a < path_b;
              }
              return a.checkpoint_id < b.checkpoint_id;
            });
  out << "closure.checkpoint_identity_previews.count="
      << checkpoint_previews.size() << "\n";
  for (std::size_t i = 0; i < checkpoint_previews.size(); ++i) {
    const auto &preview = checkpoint_previews[i];
    const auto prefix =
        "closure.checkpoint_identity_preview." + std::to_string(i);
    out << prefix << ".checkpoint_path="
        << preview.checkpoint_path.lexically_normal().string() << "\n";
    out << prefix << ".checkpoint_id=" << preview.checkpoint_id << "\n";
    out << prefix
        << ".checkpoint_file_digest=" << preview.checkpoint_file_digest << "\n";
    out << prefix << ".component=" << preview.component << "\n";
    out << prefix << ".component_assembly_fingerprint="
        << preview.component_assembly_fingerprint << "\n";
    out << prefix << ".representation_architecture="
        << preview.representation_architecture << "\n";
    out << prefix
        << ".representation_contract=" << preview.representation_contract
        << "\n";
    out << prefix
        << ".representation_value_shape=" << preview.representation_value_shape
        << "\n";
    out << prefix << ".representation_sequence_value_shape="
        << preview.representation_sequence_value_shape << "\n";
    out << prefix << ".channel_axis_policy=" << preview.channel_axis_policy
        << "\n";
    out << prefix << ".temporal_reduction=" << preview.temporal_reduction
        << "\n";
    out << prefix << ".input_representation_assembly_id="
        << preview.input_representation_assembly_id << "\n";
    out << prefix << ".context_mode=" << preview.context_mode << "\n";
    out << prefix << ".context_contract=" << preview.context_contract << "\n";
    out << prefix << ".context_value_shape=" << preview.context_value_shape
        << "\n";
    out << prefix << ".output_contract=" << preview.output_contract << "\n";
    out << prefix << ".output_value_shape=" << preview.output_value_shape
        << "\n";
    out << prefix
        << ".direct_exposure_digest=" << preview.direct_exposure_digest << "\n";
    out << prefix << ".created_by_job_id=" << preview.created_by_job_id << "\n";
    out << prefix << ".created_by_wave_id=" << preview.created_by_wave_id
        << "\n";
  }

  const auto &leakage = proof.leakage;
  out << "leakage.checked=" << (leakage.checked ? "true" : "false") << "\n";
  out << "leakage.rule=" << leakage.rule << "\n";
  out << "leakage.split_name=" << leakage.split_name << "\n";
  append_interval_text(out, "leakage.base_range", leakage.base_range);
  append_interval_text(out, "leakage.protected_range", leakage.protected_range);
  out << "leakage.dilation_left=" << leakage.dilation_left << "\n";
  out << "leakage.dilation_right=" << leakage.dilation_right << "\n";
  append_string_vector_text(out, "leakage.forbidden_uses",
                            leakage.forbidden_uses);
  out << "leakage.require_mutated_component="
      << (leakage.require_mutated_component ? "true" : "false") << "\n";
  out << "leakage.overlap_found=" << (leakage.overlap_found ? "true" : "false")
      << "\n";
  out << "leakage.overlap_witnesses.count=" << leakage.overlap_witnesses.size()
      << "\n";
  auto overlap_witnesses = leakage.overlap_witnesses;
  std::sort(overlap_witnesses.begin(), overlap_witnesses.end(),
            [](const auto &a, const auto &b) {
              auto key = [](const auto &witness) {
                std::ostringstream out;
                out << witness.fact_digest << "\n"
                    << witness.job_id << "\n"
                    << witness.wave_id << "\n"
                    << witness.target_component_family_id << "\n"
                    << witness.split_name << "\n"
                    << static_cast<int>(witness.use) << "\n"
                    << (witness.mutated_component ? "1" : "0") << "\n";
                out << witness.selector_id << "\n"
                    << witness.selector_kind << "\n"
                    << witness.selection_event_digest << "\n"
                    << witness.selected_checkpoint.lexically_normal().string()
                    << "\n";
                append_interval_text(out, "source_footprint",
                                     witness.source_footprint);
                append_interval_text(out, "protected_range",
                                     witness.protected_range);
                append_interval_text(out, "intersection", witness.intersection);
                return out.str();
              };
              return key(a) < key(b);
            });
  for (std::size_t i = 0; i < overlap_witnesses.size(); ++i) {
    const auto &witness = overlap_witnesses[i];
    const auto prefix = "leakage.overlap_witness." + std::to_string(i);
    out << prefix << ".fact_digest=" << witness.fact_digest << "\n";
    out << prefix << ".job_id=" << witness.job_id << "\n";
    out << prefix << ".wave_id=" << witness.wave_id << "\n";
    out << prefix
        << ".target_component_family_id=" << witness.target_component_family_id
        << "\n";
    out << prefix << ".split_name=" << witness.split_name << "\n";
    out << prefix << ".use="
        << cuwacunu::hero::lattice::exposure::exposure_use_name(
               witness.use)
        << "\n";
    out << prefix << ".mutated_component="
        << (witness.mutated_component ? "true" : "false") << "\n";
    out << prefix << ".selector_id=" << witness.selector_id << "\n";
    out << prefix << ".selector_kind=" << witness.selector_kind << "\n";
    out << prefix
        << ".selection_event_digest=" << witness.selection_event_digest << "\n";
    out << prefix << ".selected_checkpoint="
        << witness.selected_checkpoint.lexically_normal().string() << "\n";
    append_interval_text(out, prefix + ".source_footprint",
                         witness.source_footprint);
    append_interval_text(out, prefix + ".protected_range",
                         witness.protected_range);
    append_interval_text(out, prefix + ".intersection", witness.intersection);
  }
  out << "leakage.passed=" << (leakage.passed ? "true" : "false") << "\n";

  auto node_support_summaries = proof.node_support_summaries;
  std::sort(node_support_summaries.begin(), node_support_summaries.end(),
            [](const auto &a, const auto &b) {
              auto key = [](const auto &summary) {
                std::ostringstream out;
                append_node_support_summary_text(out, "summary", summary);
                return out.str();
              };
              return key(a) < key(b);
            });
  out << "node_support_summaries.count=" << node_support_summaries.size()
      << "\n";
  for (std::size_t i = 0; i < node_support_summaries.size(); ++i) {
    append_node_support_summary_text(
        out, "node_support_summary." + std::to_string(i),
        node_support_summaries[i]);
  }

  return out.str();
}

[[nodiscard]] inline std::string lattice_target_proof_certificate_digest(
    const lattice_target_proof_certificate_t &proof) {
  return cuwacunu::hero::lattice::exposure::exposure_digest_for_text(
      std::string("kikijyeba.lattice.target_certificate.v1\n") +
      canonical_lattice_target_proof_certificate_text(proof));
}

[[nodiscard]] inline bool
certificate_names_concrete_split(const std::string &split_name) {
  return !split_name.empty() && split_name != "unknown";
}

[[nodiscard]] inline std::int64_t interval_vector_length(
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &intervals) {
  std::int64_t out = 0;
  for (const auto interval : intervals) {
    out += interval.length();
  }
  return out;
}

[[nodiscard]] inline bool interval_vectors_equal(
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &a,
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i].begin != b[i].begin || a[i].end != b[i].end) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::vector<
    cuwacunu::hero::lattice::exposure::anchor_interval_t>
certificate_merged_intervals(
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        intervals) {
  std::sort(intervals.begin(), intervals.end(), [](const auto a, const auto b) {
    return a.begin == b.begin ? a.end < b.end : a.begin < b.begin;
  });
  std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t> out;
  for (const auto interval : intervals) {
    if (interval.empty()) {
      continue;
    }
    if (out.empty() || out.back().end < interval.begin) {
      out.push_back(interval);
    } else {
      out.back().end = std::max(out.back().end, interval.end);
    }
  }
  return out;
}

[[nodiscard]] inline std::vector<
    cuwacunu::hero::lattice::exposure::anchor_interval_t>
certificate_sorted_intervals(
    std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        intervals) {
  std::sort(intervals.begin(), intervals.end(), [](const auto a, const auto b) {
    return a.begin == b.begin ? a.end < b.end : a.begin < b.begin;
  });
  return intervals;
}

[[nodiscard]] inline std::vector<
    cuwacunu::hero::lattice::exposure::anchor_interval_t>
certificate_missing_complement(
    cuwacunu::hero::lattice::exposure::anchor_interval_t target_range,
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &covered_intervals) {
  std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t> out;
  if (target_range.empty()) {
    return out;
  }
  auto cursor = target_range.begin;
  for (const auto interval : covered_intervals) {
    if (cursor < interval.begin) {
      out.push_back({.begin = cursor, .end = interval.begin});
    }
    cursor = std::max(cursor, interval.end);
  }
  if (cursor < target_range.end) {
    out.push_back({.begin = cursor, .end = target_range.end});
  }
  return out;
}

[[nodiscard]] inline bool certificate_interval_covered_by(
    cuwacunu::hero::lattice::exposure::anchor_interval_t interval,
    const std::vector<cuwacunu::hero::lattice::exposure::anchor_interval_t>
        &covering_intervals) {
  if (interval.empty()) {
    return true;
  }
  auto cursor = interval.begin;
  for (const auto covering : covering_intervals) {
    if (covering.end <= cursor) {
      continue;
    }
    if (covering.begin > cursor) {
      return false;
    }
    cursor = std::max(cursor, covering.end);
    if (cursor >= interval.end) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline cuwacunu::hero::lattice::exposure::anchor_interval_t
certificate_source_footprint_for_causal_use(
    const lattice_target_proof_certificate_t::closure_proof_t::causal_exposure_t
        &causal,
    cuwacunu::hero::lattice::exposure::exposure_use_t use) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  switch (use) {
  case exposure::exposure_use_t::observed_input:
    return causal.observed_footprint;
  case exposure::exposure_use_t::target_supervision:
    return causal.target_footprint;
  case exposure::exposure_use_t::evaluation_metric:
  case exposure::exposure_use_t::selection_signal:
    return causal.anchor_range;
  }
  return {};
}

[[nodiscard]] inline cuwacunu::hero::lattice::exposure::
    source_key_window_audit_t
    certificate_source_key_window_audit_from_causal(
        const lattice_target_proof_certificate_t::closure_proof_t::
            causal_exposure_t &causal) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  exposure::lattice_exposure_fact_t fact{};
  fact.anchor_range = causal.anchor_range;
  fact.observed_footprint = causal.observed_footprint;
  fact.target_footprint = causal.target_footprint;
  fact.first_anchor_key = causal.first_anchor_key;
  fact.last_anchor_key = causal.last_anchor_key;
  fact.observed_source_key_begin = causal.observed_source_key_begin;
  fact.observed_source_key_end = causal.observed_source_key_end;
  fact.target_source_key_begin = causal.target_source_key_begin;
  fact.target_source_key_end = causal.target_source_key_end;
  fact.source_key_footprint_precision = causal.source_key_footprint_precision;
  return exposure::audit_source_key_window(fact);
}

[[nodiscard]] inline bool certificate_source_key_window_audits_equal(
    const cuwacunu::hero::lattice::exposure::source_key_window_audit_t &a,
    const cuwacunu::hero::lattice::exposure::source_key_window_audit_t
        &b) {
  if (a.schema != b.schema || a.available != b.available ||
      a.precision != b.precision || a.complete != b.complete ||
      a.numeric != b.numeric ||
      a.internally_monotone != b.internally_monotone ||
      a.order_preserving != b.order_preserving ||
      a.affine_step_available != b.affine_step_available ||
      a.reference_key_step != b.reference_key_step ||
      a.affine_consistent != b.affine_consistent ||
      a.missing_endpoint_pair_count != b.missing_endpoint_pair_count ||
      a.irregular_key_warning_count != b.irregular_key_warning_count ||
      a.source_key_gap_warning_count != b.source_key_gap_warning_count ||
      a.row_source_key_mismatch_count != b.row_source_key_mismatch_count ||
      a.source_key_gap_found != b.source_key_gap_found ||
      a.row_source_key_mismatch_found != b.row_source_key_mismatch_found ||
      a.issues != b.issues || a.endpoints.size() != b.endpoints.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.endpoints.size(); ++i) {
    const auto &lhs = a.endpoints[i];
    const auto &rhs = b.endpoints[i];
    if (lhs.label != rhs.label || lhs.row != rhs.row || lhs.key != rhs.key ||
        lhs.numeric != rhs.numeric || lhs.numeric_key != rhs.numeric_key) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::string certificate_leakage_witness_signature(
    const cuwacunu::hero::lattice::exposure::forbidden_exposure_overlap_t
        &witness) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  std::ostringstream out;
  out << witness.fact_digest << "|" << witness.job_id << "|" << witness.wave_id
      << "|" << witness.target_component_family_id << "|" << witness.split_name
      << "|" << exposure::exposure_use_name(witness.use) << "|"
      << (witness.mutated_component ? "mutated" : "read_only") << "|"
      << witness.selector_id << "|" << witness.selector_kind << "|"
      << witness.selection_event_digest << "|"
      << witness.selected_checkpoint.lexically_normal().string() << "|"
      << witness.source_footprint.begin << "," << witness.source_footprint.end
      << "|" << witness.protected_range.begin << ","
      << witness.protected_range.end << "|" << witness.intersection.begin << ","
      << witness.intersection.end;
  return out.str();
}

inline void
append_certificate_issue(lattice_target_proof_certificate_check_t &check,
                         std::string issue) {
  check.passed = false;
  check.issues.push_back(std::move(issue));
}

[[nodiscard]] inline bool certificate_fraction_in_unit_interval(double value) {
  return std::isfinite(value) && value >= -1e-12 && value <= 1.0 + 1e-12;
}

[[nodiscard]] inline bool doubles_close(double a, double b, double eps = 1e-9) {
  if (std::isnan(a) && std::isnan(b)) {
    return true;
  }
  if (!std::isfinite(a) || !std::isfinite(b)) {
    return a == b;
  }
  return std::abs(a - b) <= eps;
}

[[nodiscard]] inline bool
certificate_paths_equal(const std::filesystem::path &a,
                        const std::filesystem::path &b) {
  return a.lexically_normal().string() == b.lexically_normal().string();
}

[[nodiscard]] inline std::vector<std::string> certificate_model_state_path_keys(
    const std::vector<std::filesystem::path> &paths, bool &has_empty,
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
certificate_expected_model_state_input_keys(
    const std::filesystem::path &expected_mdn_checkpoint,
    const std::filesystem::path &expected_representation_checkpoint) {
  std::vector<std::filesystem::path> expected;
  if (!expected_representation_checkpoint.empty()) {
    expected.push_back(expected_representation_checkpoint);
  }
  if (!expected_mdn_checkpoint.empty()) {
    expected.push_back(expected_mdn_checkpoint);
  }
  bool has_empty = false;
  bool has_duplicate = false;
  return certificate_model_state_path_keys(expected, has_empty, has_duplicate);
}

[[nodiscard]] inline bool
certificate_dependency_status_known(const std::string &status) {
  if (status == "missing") {
    return true;
  }
  static const std::set<std::string> allowed_statuses{
      lattice_target_status_name(lattice_target_status_t::satisfied),
      lattice_target_status_name(lattice_target_status_t::missing_report),
      lattice_target_status_name(lattice_target_status_t::stale_contract),
      lattice_target_status_name(lattice_target_status_t::stale_component),
      lattice_target_status_name(lattice_target_status_t::missing_checkpoint),
      lattice_target_status_name(lattice_target_status_t::metric_failed),
      lattice_target_status_name(lattice_target_status_t::exposure_failed),
      lattice_target_status_name(lattice_target_status_t::blocked),
  };
  return allowed_statuses.count(status) != 0;
}

[[nodiscard]] inline bool
certificate_dependency_kind_known(const std::string &kind) {
  static const std::set<std::string> allowed_kinds{
      "upstream_target",
      "checkpoint_source",
      "evaluated_checkpoint_source",
  };
  return allowed_kinds.count(kind) != 0;
}

[[nodiscard]] inline std::optional<
    cuwacunu::hero::lattice::exposure::exposure_use_t>
certificate_exposure_use_from_name(const std::string &use) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  if (use == "observed_input") {
    return exposure::exposure_use_t::observed_input;
  }
  if (use == "target_supervision") {
    return exposure::exposure_use_t::target_supervision;
  }
  if (use == "evaluation_metric") {
    return exposure::exposure_use_t::evaluation_metric;
  }
  if (use == "selection_signal") {
    return exposure::exposure_use_t::selection_signal;
  }
  return std::nullopt;
}

[[nodiscard]] inline bool
certificate_exposure_use_known(const std::string &use) {
  static const std::set<std::string> allowed_uses{
      "observed_input",
      "target_supervision",
      "evaluation_metric",
      "selection_signal",
  };
  return allowed_uses.count(use) != 0;
}

[[nodiscard]] inline bool certificate_causal_exposure_has_use_set(
    const lattice_target_proof_certificate_t::closure_proof_t::causal_exposure_t
        &causal,
    const cuwacunu::hero::lattice::exposure::exposure_use_set_t &use) {
  const auto has_use = [&](const std::string &name) {
    return std::find(causal.uses.begin(), causal.uses.end(), name) !=
           causal.uses.end();
  };
  return (!use.observed_input || has_use("observed_input")) &&
         (!use.target_supervision || has_use("target_supervision")) &&
         (!use.evaluation_metric || has_use("evaluation_metric")) &&
         (!use.selection_signal || has_use("selection_signal")) &&
         causal.mutated_component == use.mutated_component;
}

[[nodiscard]] inline lattice_target_proof_certificate_check_t
verify_lattice_target_proof_certificate(
    const lattice_target_proof_certificate_t &proof) {
  namespace exposure = cuwacunu::hero::lattice::exposure;

  lattice_target_proof_certificate_check_t check{};
  if (proof.schema != "kikijyeba.lattice.target_certificate.v1") {
    append_certificate_issue(check, "certificate schema mismatch");
  }
  if (proof.target_id.empty()) {
    append_certificate_issue(check, "missing target_id");
  }
  if (proof.target_spec_fingerprint.empty()) {
    append_certificate_issue(check, "missing target_spec_fingerprint");
  }
  const bool certificate_digest_missing = proof.certificate_digest.empty();
  if (!certificate_digest_missing &&
      proof.certificate_digest !=
          lattice_target_proof_certificate_digest(proof)) {
    append_certificate_issue(check, "certificate_digest mismatch");
  }

  const auto &proof_context = proof.proof_context;
  if (proof_context.require_contract_match &&
      (proof_context.active_contract_fingerprint.empty() ||
       proof_context.evidence_contract_fingerprint.empty())) {
    append_certificate_issue(check, "identity missing required contract");
  }
  if (proof_context.require_component_match &&
      (proof_context.expected_component_fingerprint.empty() ||
       proof_context.evidence_component_fingerprint.empty())) {
    append_certificate_issue(check, "identity missing required component");
  }
  if (proof_context.require_graph_anchor_identity &&
      (proof_context.active_graph_order_fingerprint.empty() ||
       proof_context.evidence_graph_order_fingerprint.empty())) {
    append_certificate_issue(check, "identity missing required graph order");
  }
  if (proof_context.require_graph_anchor_identity &&
      (proof_context.active_source_cursor_token.empty() ||
       proof_context.evidence_source_cursor_token.empty())) {
    append_certificate_issue(check, "identity missing required source cursor");
  }
  const auto expected_identity_match = [](bool required,
                                          const std::string &left,
                                          const std::string &right) -> bool {
    if (!required) {
      return !(left.empty() && right.empty());
    }
    return !left.empty() && !right.empty() && left == right;
  };
  const bool expected_contract_match =
      expected_identity_match(proof_context.require_contract_match,
                              proof_context.active_contract_fingerprint,
                              proof_context.evidence_contract_fingerprint);
  if (proof_context.contract_match != expected_contract_match) {
    append_certificate_issue(check, "identity contract_match mismatch");
  }
  const bool expected_component_match =
      expected_identity_match(proof_context.require_component_match,
                              proof_context.expected_component_fingerprint,
                              proof_context.evidence_component_fingerprint);
  if (proof_context.component_match != expected_component_match) {
    append_certificate_issue(check, "identity component_match mismatch");
  }
  const bool expected_graph_order_match =
      expected_identity_match(proof_context.require_graph_anchor_identity,
                              proof_context.active_graph_order_fingerprint,
                              proof_context.evidence_graph_order_fingerprint);
  if (proof_context.graph_order_match != expected_graph_order_match) {
    append_certificate_issue(check, "identity graph_order_match mismatch");
  }
  const bool expected_source_cursor_match =
      expected_identity_match(proof_context.require_graph_anchor_identity,
                              proof_context.active_source_cursor_token,
                              proof_context.evidence_source_cursor_token);
  if (proof_context.source_cursor_match != expected_source_cursor_match) {
    append_certificate_issue(check, "identity source_cursor_match mismatch");
  }

  std::set<std::string> dependency_signatures;
  for (std::size_t i = 0; i < proof.dependencies.size(); ++i) {
    const auto &dependency = proof.dependencies[i];
    const auto prefix = "dependency[" + std::to_string(i) + "]";
    const auto dependency_signature =
        dependency.kind + "|" + dependency.source_target_id;
    if (!dependency_signatures.insert(dependency_signature).second) {
      append_certificate_issue(check, prefix + " duplicate dependency");
    }
    if (dependency.kind.empty()) {
      append_certificate_issue(check, prefix + " missing kind");
    } else if (!certificate_dependency_kind_known(dependency.kind)) {
      append_certificate_issue(check, prefix + " unknown kind");
    }
    if (dependency.source_target_id.empty()) {
      append_certificate_issue(check, prefix + " missing source target id");
    }
    if (dependency.status.empty()) {
      append_certificate_issue(check, prefix + " missing status");
    } else if (!certificate_dependency_status_known(dependency.status)) {
      append_certificate_issue(check, prefix + " unknown status");
    }
    if (dependency.kind == "upstream_target" &&
        (!dependency.expected_mdn_checkpoint.empty() ||
         !dependency.actual_mdn_checkpoint.empty() ||
         dependency.mdn_checkpoint_match ||
         !dependency.expected_representation_checkpoint.empty() ||
         !dependency.actual_representation_checkpoint.empty() ||
         dependency.representation_checkpoint_match)) {
      append_certificate_issue(
          check, prefix + " upstream target carries checkpoint binding");
    }
    const bool dependency_satisfied =
        dependency.status ==
        lattice_target_status_name(lattice_target_status_t::satisfied);
    if (dependency_satisfied &&
        (dependency.kind == "checkpoint_source" ||
         dependency.kind == "evaluated_checkpoint_source")) {
      if (dependency.expected_mdn_checkpoint.empty() ||
          dependency.actual_mdn_checkpoint.empty()) {
        append_certificate_issue(
            check, prefix + " satisfied checkpoint source missing checkpoint "
                            "path");
      } else if (!dependency.mdn_checkpoint_match) {
        append_certificate_issue(
            check, prefix + " satisfied checkpoint source checkpoint mismatch");
      }
    }
    if (dependency_satisfied &&
        dependency.kind == "evaluated_checkpoint_source" &&
        !dependency.expected_representation_checkpoint.empty()) {
      if (dependency.actual_representation_checkpoint.empty()) {
        append_certificate_issue(
            check, prefix + " satisfied evaluated checkpoint source missing "
                            "representation checkpoint path");
      } else if (!dependency.representation_checkpoint_match) {
        append_certificate_issue(
            check, prefix + " satisfied evaluated checkpoint source "
                            "representation checkpoint mismatch");
      }
    }
    if (!dependency.expected_mdn_checkpoint.empty() &&
        !dependency.actual_mdn_checkpoint.empty()) {
      const bool expected_match = certificate_paths_equal(
          dependency.expected_mdn_checkpoint, dependency.actual_mdn_checkpoint);
      if (dependency.mdn_checkpoint_match != expected_match) {
        append_certificate_issue(check,
                                 prefix + " MDN checkpoint match mismatch");
      }
    } else if (dependency.mdn_checkpoint_match) {
      append_certificate_issue(check, prefix +
                                          " MDN checkpoint match with missing "
                                          "path");
    }
    if (!dependency.expected_representation_checkpoint.empty() &&
        !dependency.actual_representation_checkpoint.empty()) {
      const bool expected_match =
          certificate_paths_equal(dependency.expected_representation_checkpoint,
                                  dependency.actual_representation_checkpoint);
      if (dependency.representation_checkpoint_match != expected_match) {
        append_certificate_issue(
            check, prefix + " representation checkpoint match mismatch");
      }
    } else if (dependency.representation_checkpoint_match) {
      append_certificate_issue(
          check, prefix + " representation checkpoint match with missing path");
    }
  }
  const auto evaluated_checkpoint_dependency = std::find_if(
      proof.dependencies.begin(), proof.dependencies.end(),
      [](const lattice_target_proof_certificate_t::dependency_proof_t
             &dependency) {
        return dependency.kind == "evaluated_checkpoint_source" &&
               dependency.status == lattice_target_status_name(
                                        lattice_target_status_t::satisfied);
      });
  const auto fact_has_input_checkpoint =
      [](const exposure::lattice_exposure_fact_t &fact,
         const std::filesystem::path &checkpoint_path) {
        if (checkpoint_path.empty()) {
          return true;
        }
        return std::any_of(
            fact.input_checkpoints.begin(), fact.input_checkpoints.end(),
            [&](const std::filesystem::path &candidate) {
              return certificate_paths_equal(candidate, checkpoint_path);
            });
      };

  std::set<std::string> artifact_signatures;
  for (std::size_t i = 0; i < proof.artifacts.size(); ++i) {
    const auto &artifact = proof.artifacts[i];
    const auto prefix = "artifact[" + std::to_string(i) + "]";
    const auto signature = artifact.fact_family + "|" + artifact.fact_digest;
    if (!artifact_signatures.insert(signature).second) {
      append_certificate_issue(check, prefix + " duplicate artifact proof");
    }
    if (artifact.proof_kind.empty()) {
      append_certificate_issue(check, prefix + " missing proof_kind");
    }
    if (artifact.fact_family.empty()) {
      append_certificate_issue(check, prefix + " missing fact_family");
    }
    std::optional<exposure::lattice_fact_family_t> parsed_fact_family{
        std::nullopt};
    std::optional<exposure::lattice_fact_family_descriptor_t>
        fact_family_descriptor{std::nullopt};
    if (!artifact.fact_family.empty()) {
      parsed_fact_family =
          exposure::parse_lattice_fact_family(artifact.fact_family);
      if (parsed_fact_family.has_value()) {
        fact_family_descriptor =
            exposure::lattice_fact_family_descriptor(*parsed_fact_family);
      }
    }
    if (!artifact.fact_family.empty() && !parsed_fact_family.has_value()) {
      append_certificate_issue(check, prefix + " unknown fact_family");
    } else if (const auto *proof_template =
                   artifact_readiness_proof_template_for_subject_fact_family(
                       artifact.fact_family)) {
      const bool proof_kind_matches_template =
          artifact.proof_kind == proof_template->proof_kind;
      if (!proof_kind_matches_template) {
        append_certificate_issue(
            check,
            prefix + " proof_kind does not match artifact proof template");
      }
      if (artifact.proof_template_bound != proof_kind_matches_template) {
        append_certificate_issue(check,
                                 prefix + " proof_template_bound mismatch");
      }
      if (artifact.proof_template_claim != proof_template->proof_claim) {
        append_certificate_issue(check,
                                 prefix + " proof_template_claim mismatch");
      }
    } else {
      append_certificate_issue(check,
                               prefix + " missing artifact proof template");
      if (artifact.proof_template_bound) {
        append_certificate_issue(check,
                                 prefix + " proof_template_bound mismatch");
      }
    }
    if (artifact.fact_schema.empty()) {
      append_certificate_issue(check, prefix + " missing fact_schema");
    }
    if (artifact.fact_type.empty()) {
      append_certificate_issue(check, prefix + " missing fact_type");
    }
    if (artifact.fact_identity_contract_schema !=
        k_lattice_fact_identity_contract_schema_v1) {
      append_certificate_issue(
          check, prefix + " fact identity contract schema mismatch");
    }
    if (artifact.fact_identity_contract_id !=
        k_lattice_fact_identity_contract_id_v1) {
      append_certificate_issue(check,
                               prefix + " fact identity contract id mismatch");
    }
    const bool expected_fact_identity_contract_bound =
        fact_family_descriptor.has_value() &&
        fact_family_descriptor->fact_schema == artifact.fact_schema &&
        artifact.fact_identity_contract_schema ==
            k_lattice_fact_identity_contract_schema_v1 &&
        artifact.fact_identity_contract_id ==
            k_lattice_fact_identity_contract_id_v1;
    if (artifact.fact_identity_contract_bound !=
        expected_fact_identity_contract_bound) {
      append_certificate_issue(
          check, prefix + " fact_identity_contract_bound mismatch");
    }
    if (!artifact.fact_identity_contract_bound) {
      append_certificate_issue(check,
                               prefix + " fact identity contract not bound");
    }
    if (artifact.fact_digest.empty()) {
      append_certificate_issue(check, prefix + " missing fact_digest");
    }
    if (artifact.parent_exposure_fact_digest.empty()) {
      append_certificate_issue(check,
                               prefix + " missing parent exposure digest");
    }
    if (artifact.job_id.empty()) {
      append_certificate_issue(check, prefix + " missing job_id");
    }
    if (artifact.wave_id.empty()) {
      append_certificate_issue(check, prefix + " missing wave_id");
    }
    if (artifact.protocol_id.empty()) {
      append_certificate_issue(check, prefix + " missing protocol_id");
    }
    if (artifact.contract_fingerprint.empty()) {
      append_certificate_issue(check, prefix + " missing contract fingerprint");
    }
    if (artifact.graph_order_fingerprint.empty()) {
      append_certificate_issue(check,
                               prefix + " missing graph order fingerprint");
    }
    if (artifact.source_cursor_token.empty()) {
      append_certificate_issue(check, prefix + " missing source cursor token");
    }
    if (artifact.split_policy_fingerprint.empty()) {
      append_certificate_issue(check,
                               prefix + " missing split policy fingerprint");
    }
    if (artifact.split_name.empty()) {
      append_certificate_issue(check, prefix + " missing split_name");
    }
    if (artifact.anchor_range.empty()) {
      append_certificate_issue(check, prefix + " missing anchor_range");
    }
    if (artifact.completed_anchor_range.empty()) {
      append_certificate_issue(check,
                               prefix + " missing completed_anchor_range");
    }
    const bool expected_fact_identity_envelope_complete =
        !artifact.fact_schema.empty() && !artifact.fact_type.empty() &&
        !artifact.fact_digest.empty() &&
        !artifact.parent_exposure_fact_digest.empty() &&
        !artifact.job_id.empty() && !artifact.wave_id.empty() &&
        !artifact.protocol_id.empty() &&
        !artifact.contract_fingerprint.empty() && !artifact.component.empty() &&
        !artifact.component_assembly_fingerprint.empty() &&
        !artifact.graph_order_fingerprint.empty() &&
        !artifact.source_cursor_token.empty() &&
        !artifact.split_policy_fingerprint.empty() &&
        !artifact.split_name.empty() && !artifact.anchor_range.empty() &&
        !artifact.completed_anchor_range.empty();
    if (artifact.fact_identity_envelope_complete !=
        expected_fact_identity_envelope_complete) {
      append_certificate_issue(
          check, prefix + " fact_identity_envelope_complete mismatch");
    }
    if (!artifact.fact_identity_envelope_complete) {
      append_certificate_issue(check,
                               prefix + " fact identity envelope incomplete");
    }
    if (!artifact.row_index_interval_authority) {
      append_certificate_issue(check,
                               prefix + " row-index interval authority absent");
    }
    if (!artifact.source_key_window_audit_only) {
      append_certificate_issue(check,
                               prefix + " source-key window not audit-only");
    }
    if (artifact.fact_identity_target_kind_authority) {
      append_certificate_issue(
          check, prefix + " fact identity target-kind authority present");
    }
    if (artifact.fact_identity_runtime_wave_authority) {
      append_certificate_issue(
          check, prefix + " fact identity Runtime-wave authority present");
    }
    if (artifact.fact_identity_marshal_reachability) {
      append_certificate_issue(
          check, prefix + " fact identity Marshal reachability present");
    }
    if (artifact.fact_identity_policy_gate_authority) {
      append_certificate_issue(
          check, prefix + " fact identity policy-gate authority present");
    }
    bool expected_identity_match = true;
    if (proof_context.require_contract_match) {
      expected_identity_match = expected_identity_match &&
                                artifact.contract_fingerprint ==
                                    proof_context.active_contract_fingerprint;
    }
    if (proof_context.require_graph_anchor_identity) {
      expected_identity_match =
          expected_identity_match &&
          artifact.graph_order_fingerprint ==
              proof_context.active_graph_order_fingerprint &&
          artifact.source_cursor_token ==
              proof_context.active_source_cursor_token;
    }
    if (!proof_context.component_family_id.empty() &&
        !artifact.component.empty()) {
      expected_identity_match =
          expected_identity_match &&
          artifact.component == proof_context.component_family_id;
    }
    if (artifact.identity_match != expected_identity_match) {
      append_certificate_issue(check, prefix + " identity_match mismatch");
    }
    if (!artifact.artifact_evidence) {
      append_certificate_issue(check, prefix + " missing artifact evidence");
    }
    if (!artifact.proof_template_bound) {
      append_certificate_issue(check, prefix + " proof template not bound");
    }
    if (!artifact.fact_identity_contract_bound) {
      append_certificate_issue(check, prefix + " fact identity not bound");
    }
    if (!artifact.deterministic_artifact) {
      append_certificate_issue(check,
                               prefix + " missing deterministic artifact");
    }
    if (!artifact.visibility_only) {
      append_certificate_issue(check, prefix + " missing visibility-only flag");
    }
    const bool authority_drift =
        artifact.readiness_authority || artifact.quality_authority ||
        artifact.performance_authority || artifact.checkpoint_selector ||
        artifact.coverage_authority || artifact.leakage_authority ||
        artifact.contract_identity_authority || artifact.allocation_authority ||
        artifact.execution_authority || artifact.market_readiness_authority ||
        artifact.deployment_authority || artifact.policy_gate ||
        artifact.target_dependency_authority ||
        artifact.runtime_wave_authority || artifact.marshal_reachability ||
        artifact.checkpoint_source_authority ||
        artifact.plan_checkpoint_input_authority ||
        artifact.model_state_mutation ||
        artifact.raw_potential_tradable_return || artifact.replay_executor ||
        artifact.fact_identity_target_kind_authority ||
        artifact.fact_identity_runtime_wave_authority ||
        artifact.fact_identity_marshal_reachability ||
        artifact.fact_identity_policy_gate_authority;
    if (artifact.authority_clean != !authority_drift) {
      append_certificate_issue(check, prefix + " authority_clean mismatch");
    }
    if (artifact.readiness_authority) {
      append_certificate_issue(check, prefix + " readiness authority present");
    }
    if (artifact.quality_authority) {
      append_certificate_issue(check, prefix + " quality authority present");
    }
    if (artifact.performance_authority) {
      append_certificate_issue(check,
                               prefix + " performance authority present");
    }
    if (artifact.checkpoint_selector) {
      append_certificate_issue(check, prefix + " checkpoint selector present");
    }
    if (artifact.coverage_authority) {
      append_certificate_issue(check, prefix + " coverage authority present");
    }
    if (artifact.leakage_authority) {
      append_certificate_issue(check, prefix + " leakage authority present");
    }
    if (artifact.contract_identity_authority) {
      append_certificate_issue(check,
                               prefix + " contract identity authority present");
    }
    if (artifact.allocation_authority) {
      append_certificate_issue(check, prefix + " allocation authority present");
    }
    if (artifact.execution_authority) {
      append_certificate_issue(check, prefix + " execution authority present");
    }
    if (artifact.market_readiness_authority) {
      append_certificate_issue(check,
                               prefix + " market readiness authority present");
    }
    if (artifact.deployment_authority) {
      append_certificate_issue(check, prefix + " deployment authority present");
    }
    if (artifact.policy_gate) {
      append_certificate_issue(check, prefix + " policy gate present");
    }
    if (artifact.target_dependency_authority) {
      append_certificate_issue(check,
                               prefix + " target dependency authority present");
    }
    if (artifact.runtime_wave_authority) {
      append_certificate_issue(check,
                               prefix + " runtime wave authority present");
    }
    if (artifact.marshal_reachability) {
      append_certificate_issue(check, prefix + " Marshal reachability present");
    }
    if (artifact.checkpoint_source_authority) {
      append_certificate_issue(check,
                               prefix + " checkpoint source authority present");
    }
    if (artifact.plan_checkpoint_input_authority) {
      append_certificate_issue(
          check, prefix + " plan checkpoint input authority present");
    }
    if (artifact.model_state_mutation) {
      append_certificate_issue(check, prefix + " model state mutation present");
    }
    if (artifact.raw_potential_tradable_return) {
      append_certificate_issue(
          check, prefix + " raw potential tradable return present");
    }
    if (artifact.replay_executor) {
      append_certificate_issue(check, prefix + " replay executor present");
    }
    if (!artifact.authority_clean) {
      append_certificate_issue(check, prefix + " authority drift present");
    }
    if (!artifact.lineage_bound) {
      append_certificate_issue(check, prefix + " lineage not bound");
    }
    if (!artifact.issues.empty()) {
      append_certificate_issue(check, prefix + " carries unresolved issues");
    }
    if (!artifact.passed) {
      append_certificate_issue(check, prefix + " did not pass");
    }
  }

  std::set<std::string> coverage_obligation_signatures;
  for (std::size_t i = 0; i < proof.coverage.size(); ++i) {
    const auto &coverage = proof.coverage[i];
    const auto prefix = "coverage[" + std::to_string(i) + "]";
    const auto &load = coverage.load_summary;
    std::ostringstream coverage_signature;
    coverage_signature << coverage.use << "|" << coverage.target_range.begin
                       << "," << coverage.target_range.end << "|"
                       << (coverage.require_mutated_component ? "mutated"
                                                              : "any");
    if (!coverage_obligation_signatures.insert(coverage_signature.str())
             .second) {
      append_certificate_issue(check,
                               prefix + " duplicate coverage obligation");
    }
    if (coverage.algebra != exposure::k_unique_coverage_algebra) {
      append_certificate_issue(check, prefix + " coverage algebra mismatch");
    }
    const auto coverage_use = certificate_exposure_use_from_name(coverage.use);
    if (!certificate_exposure_use_known(coverage.use)) {
      append_certificate_issue(check, prefix + " unknown coverage use");
    }
    if (!certificate_fraction_in_unit_interval(coverage.required_fraction)) {
      append_certificate_issue(check,
                               prefix + " required fraction out of range");
    }
    if (coverage.required_fraction <= 0.0) {
      append_certificate_issue(check,
                               prefix + " non-positive required fraction");
    }
    if (!certificate_fraction_in_unit_interval(coverage.actual_fraction)) {
      append_certificate_issue(check, prefix + " actual fraction out of range");
    }
    if (coverage.target_range.empty()) {
      append_certificate_issue(check, prefix + " empty target range");
    }
    if (coverage.target_range.empty() &&
        (!coverage.contributing_intervals.empty() ||
         !coverage.contributing_fact_digests.empty() ||
         !coverage.contributing_facts.empty() ||
         !coverage.covered_intervals.empty() ||
         !coverage.missing_intervals.empty())) {
      append_certificate_issue(check,
                               prefix + " empty target has interval evidence");
    }
    if (coverage.contributing_fact_digests.size() !=
        coverage.contributing_intervals.size()) {
      append_certificate_issue(
          check, prefix + " contributing digest/interval count mismatch");
    }
    if (coverage.contributing_facts.size() !=
        coverage.contributing_intervals.size()) {
      append_certificate_issue(
          check, prefix + " contributing fact/interval count mismatch");
    }
    std::set<std::string> contributing_fact_digests;
    for (std::size_t j = 0; j < coverage.contributing_fact_digests.size();
         ++j) {
      const auto &digest = coverage.contributing_fact_digests[j];
      const auto digest_prefix =
          prefix + ".contributing_fact_digests[" + std::to_string(j) + "]";
      if (digest.empty()) {
        append_certificate_issue(check, digest_prefix + " empty digest");
        continue;
      }
      if (!contributing_fact_digests.insert(digest).second) {
        append_certificate_issue(check, prefix + " duplicate contributing "
                                                 "fact digest");
      }
    }
    for (std::size_t j = 0; j < coverage.contributing_facts.size() &&
                            j < coverage.contributing_intervals.size() &&
                            j < coverage.contributing_fact_digests.size();
         ++j) {
      const auto &fact = coverage.contributing_facts[j];
      const auto fact_prefix =
          prefix + ".contributing_facts[" + std::to_string(j) + "]";
      const auto fact_digest = exposure::exposure_fact_digest(fact);
      if (fact_digest != coverage.contributing_fact_digests[j]) {
        append_certificate_issue(check,
                                 fact_prefix + " digest binding mismatch");
      }
      if (fact.target_component_family_id != load.target_component_family_id) {
        append_certificate_issue(check,
                                 fact_prefix + " target component mismatch");
      }
      if (proof_context.require_contract_match &&
          fact.contract_fingerprint !=
              proof_context.active_contract_fingerprint) {
        append_certificate_issue(check,
                                 fact_prefix + " contract identity mismatch");
      }
      if (proof_context.require_graph_anchor_identity &&
          fact.graph_order_fingerprint !=
              proof_context.active_graph_order_fingerprint) {
        append_certificate_issue(check, fact_prefix +
                                            " graph-order identity mismatch");
      }
      if (proof_context.require_graph_anchor_identity &&
          fact.source_cursor_token !=
              proof_context.active_source_cursor_token) {
        append_certificate_issue(check, fact_prefix +
                                            " source-cursor identity mismatch");
      }
      if (proof_context.require_component_match &&
          fact.component_assembly_fingerprint !=
              proof_context.expected_component_fingerprint) {
        append_certificate_issue(check,
                                 fact_prefix + " component assembly mismatch");
      }
      if (!proof.split_policy_fingerprint.empty() &&
          fact.split_policy_fingerprint.empty() &&
          certificate_names_concrete_split(fact.split_name)) {
        append_certificate_issue(
            check, fact_prefix + " missing split-policy fingerprint");
      } else if (!proof.split_policy_fingerprint.empty() &&
                 !fact.split_policy_fingerprint.empty() &&
                 fact.split_policy_fingerprint !=
                     proof.split_policy_fingerprint) {
        append_certificate_issue(check, fact_prefix + " split-policy mismatch");
      }
      if (fact.job_status != "completed") {
        append_certificate_issue(check,
                                 fact_prefix + " job_status not completed");
      }
      if (fact.cursor_domain != "ujcamei.graph_anchor") {
        append_certificate_issue(check,
                                 fact_prefix + " cursor_domain mismatch");
      }
      if (!coverage_use.has_value() ||
          !exposure::fact_has_use(fact, *coverage_use)) {
        append_certificate_issue(check, fact_prefix + " use mismatch");
      }
      if (coverage.require_mutated_component && !fact.use.mutated_component) {
        append_certificate_issue(check,
                                 fact_prefix + " missing required mutation");
      }
      if (coverage.use == "evaluation_metric" &&
          !coverage.require_mutated_component) {
        if (fact.use.mutated_component) {
          append_certificate_issue(
              check, fact_prefix + " evaluation coverage mutates component");
        }
        if (fact.optimizer_steps != 0) {
          append_certificate_issue(
              check, fact_prefix + " evaluation coverage has optimizer steps");
        }
        if (!fact.output_checkpoint.empty()) {
          append_certificate_issue(
              check,
              fact_prefix + " evaluation coverage has output checkpoint");
        }
        if (evaluated_checkpoint_dependency != proof.dependencies.end()) {
          if (!fact_has_input_checkpoint(
                  fact,
                  evaluated_checkpoint_dependency->actual_mdn_checkpoint)) {
            append_certificate_issue(
                check, fact_prefix +
                           " evaluation coverage missing evaluated MDN input "
                           "checkpoint");
          }
          if (!fact_has_input_checkpoint(
                  fact, evaluated_checkpoint_dependency
                            ->actual_representation_checkpoint)) {
            append_certificate_issue(
                check, fact_prefix + " evaluation coverage missing evaluated "
                                     "representation input checkpoint");
          }
          bool actual_has_empty = false;
          bool actual_has_duplicate = false;
          const auto actual_input_keys = certificate_model_state_path_keys(
              fact.input_checkpoints, actual_has_empty, actual_has_duplicate);
          const auto expected_input_keys =
              certificate_expected_model_state_input_keys(
                  evaluated_checkpoint_dependency->actual_mdn_checkpoint,
                  evaluated_checkpoint_dependency
                      ->actual_representation_checkpoint);
          if (actual_has_empty) {
            append_certificate_issue(
                check, fact_prefix +
                           " evaluation coverage has empty input checkpoint");
          }
          if (actual_has_duplicate) {
            append_certificate_issue(
                check,
                fact_prefix +
                    " evaluation coverage has duplicate input checkpoint");
          }
          if (actual_input_keys != expected_input_keys) {
            append_certificate_issue(
                check, fact_prefix +
                           " evaluation coverage input checkpoint set "
                           "mismatch");
          }
        }
      }
      if (coverage_use.has_value()) {
        const auto expected_interval = exposure::interval_intersection(
            exposure::anchor_coverage_for_use(fact, *coverage_use),
            coverage.target_range);
        const auto actual_interval = coverage.contributing_intervals[j];
        if (expected_interval.begin != actual_interval.begin ||
            expected_interval.end != actual_interval.end) {
          append_certificate_issue(check, fact_prefix + " interval mismatch");
        }
      }
    }
    for (std::size_t j = 0; j < coverage.contributing_intervals.size(); ++j) {
      const auto interval = coverage.contributing_intervals[j];
      const auto interval_prefix =
          prefix + ".contributing_intervals[" + std::to_string(j) + "]";
      if (interval.empty()) {
        append_certificate_issue(check, interval_prefix + " empty interval");
      }
      if (!coverage.target_range.empty() &&
          (interval.begin < coverage.target_range.begin ||
           interval.end > coverage.target_range.end)) {
        append_certificate_issue(check,
                                 interval_prefix + " outside target range");
      }
      if (j > 0) {
        const auto previous = coverage.contributing_intervals[j - 1];
        if (previous.begin > interval.begin ||
            (previous.begin == interval.begin && previous.end > interval.end)) {
          append_certificate_issue(check,
                                   prefix + " contributing intervals unsorted");
        }
      }
    }
    const auto expected_covered_intervals =
        certificate_merged_intervals(coverage.contributing_intervals);
    if (!interval_vectors_equal(coverage.covered_intervals,
                                expected_covered_intervals)) {
      append_certificate_issue(check,
                               prefix + " covered interval union mismatch");
    }
    for (std::size_t j = 0; j < coverage.covered_intervals.size(); ++j) {
      const auto interval = coverage.covered_intervals[j];
      const auto interval_prefix =
          prefix + ".covered_intervals[" + std::to_string(j) + "]";
      if (interval.empty()) {
        append_certificate_issue(check, interval_prefix + " empty interval");
      }
      if (!coverage.target_range.empty() &&
          (interval.begin < coverage.target_range.begin ||
           interval.end > coverage.target_range.end)) {
        append_certificate_issue(check,
                                 interval_prefix + " outside target range");
      }
      if (j > 0 && coverage.covered_intervals[j - 1].end >= interval.begin) {
        append_certificate_issue(check,
                                 prefix + " covered intervals not canonical");
      }
    }
    const auto expected_missing_intervals = certificate_missing_complement(
        coverage.target_range, coverage.covered_intervals);
    if (!interval_vectors_equal(coverage.missing_intervals,
                                expected_missing_intervals)) {
      append_certificate_issue(check, prefix + " missing interval complement "
                                               "mismatch");
    }
    for (std::size_t j = 0; j < coverage.missing_intervals.size(); ++j) {
      const auto interval = coverage.missing_intervals[j];
      const auto interval_prefix =
          prefix + ".missing_intervals[" + std::to_string(j) + "]";
      if (interval.empty()) {
        append_certificate_issue(check, interval_prefix + " empty interval");
      }
      if (!coverage.target_range.empty() &&
          (interval.begin < coverage.target_range.begin ||
           interval.end > coverage.target_range.end)) {
        append_certificate_issue(check,
                                 interval_prefix + " outside target range");
      }
      if (j > 0 && coverage.missing_intervals[j - 1].end >= interval.begin) {
        append_certificate_issue(check,
                                 prefix + " missing intervals not canonical");
      }
    }
    const auto covered_length =
        interval_vector_length(coverage.covered_intervals);
    if (coverage.covered_anchors != covered_length) {
      append_certificate_issue(check, prefix + " covered_anchors mismatch");
    }
    const auto missing_length =
        interval_vector_length(coverage.missing_intervals);
    if (coverage.missing_anchors != missing_length) {
      append_certificate_issue(check, prefix + " missing_anchors mismatch");
    }
    if (!coverage.target_range.empty()) {
      if (coverage.covered_anchors + coverage.missing_anchors !=
          coverage.target_range.length()) {
        append_certificate_issue(
            check, prefix + " coverage/missing complement mismatch");
      }
      const auto expected_fraction =
          static_cast<double>(coverage.covered_anchors) /
          static_cast<double>(coverage.target_range.length());
      if (!doubles_close(coverage.actual_fraction, expected_fraction)) {
        append_certificate_issue(check, prefix + " actual_fraction mismatch");
      }
    }
    const bool expected_passed =
        coverage.actual_fraction + 1e-12 >= coverage.required_fraction;
    if (coverage.passed != expected_passed) {
      append_certificate_issue(check, prefix + " passed flag mismatch");
    }
    if (load.unique_coverage_algebra != exposure::k_unique_coverage_algebra ||
        load.load_algebra != exposure::k_load_algebra ||
        load.unique_coverage_unit != exposure::k_unique_coverage_unit ||
        load.load_unit != exposure::k_load_unit ||
        load.anchor_event_unit != exposure::k_anchor_event_unit) {
      append_certificate_issue(check, prefix + " load summary unit mismatch");
    }
    if (exposure::exposure_use_name(load.use) != coverage.use) {
      append_certificate_issue(check, prefix + " load summary use mismatch");
    }
    if (load.require_mutated_component != coverage.require_mutated_component) {
      append_certificate_issue(check,
                               prefix + " load mutation requirement mismatch");
    }
    if (load.component_scope != "target_component_family_id" &&
        load.component_scope != "all_components") {
      append_certificate_issue(check, prefix + " load component scope unknown");
    }
    if (load.component_scope != "target_component_family_id") {
      append_certificate_issue(
          check,
          prefix + " coverage load scope must be target_component_family_id");
    }
    if (load.component_scope == "target_component_family_id" &&
        load.target_component_family_id.empty()) {
      append_certificate_issue(check,
                               prefix + " load target component missing");
    }
    if (load.component_scope == "all_components" &&
        !load.target_component_family_id.empty()) {
      append_certificate_issue(
          check, prefix + " all-components load has target component");
    }
    if (load.target_range.begin != coverage.target_range.begin ||
        load.target_range.end != coverage.target_range.end) {
      append_certificate_issue(check,
                               prefix + " load summary target range mismatch");
    }
    if (proof.closure.checked && !coverage.contributing_fact_digests.empty()) {
      std::set<std::string> closure_fact_digests(
          proof.closure.fact_digests.begin(), proof.closure.fact_digests.end());
      for (std::size_t j = 0; j < coverage.contributing_fact_digests.size() &&
                              j < coverage.contributing_intervals.size();
           ++j) {
        const auto &digest = coverage.contributing_fact_digests[j];
        if (digest.empty()) {
          continue;
        }
        const auto causal = std::find_if(
            proof.closure.causal_exposures.begin(),
            proof.closure.causal_exposures.end(), [&](const auto &candidate) {
              return candidate.fact_digest == digest;
            });
        if (causal == proof.closure.causal_exposures.end()) {
          if (coverage.require_mutated_component) {
            append_certificate_issue(
                check, prefix + " contributing fact digest not in closure");
          }
          continue;
        }
        if (closure_fact_digests.count(digest) == 0) {
          append_certificate_issue(
              check, prefix + " contributing digest missing from closure list");
        }
        if (load.component_scope == "target_component_family_id" &&
            causal->target_component_family_id !=
                load.target_component_family_id) {
          append_certificate_issue(
              check, prefix + " contributing digest component mismatch");
        }
        if (load.component_scope == "target_component_family_id" &&
            proof_context.require_component_match &&
            causal->component_assembly_fingerprint !=
                proof_context.expected_component_fingerprint) {
          append_certificate_issue(
              check, prefix + " contributing digest assembly mismatch");
        }
        if (coverage.require_mutated_component && !causal->mutated_component) {
          append_certificate_issue(
              check, prefix + " contributing digest missing mutation");
        }
        if (std::find(causal->uses.begin(), causal->uses.end(), coverage.use) ==
            causal->uses.end()) {
          append_certificate_issue(
              check, prefix + " contributing digest use mismatch");
        }
        const auto expected_interval = exposure::interval_intersection(
            causal->completed_anchor_range, coverage.target_range);
        const auto actual_interval = coverage.contributing_intervals[j];
        if (expected_interval.begin != actual_interval.begin ||
            expected_interval.end != actual_interval.end) {
          append_certificate_issue(
              check, prefix + " contributing digest interval mismatch");
        }
      }
    }
    if (proof.closure.checked && coverage.require_mutated_component &&
        !coverage.contributing_intervals.empty()) {
      std::vector<exposure::anchor_interval_t> causal_support_intervals;
      for (const auto &causal : proof.closure.causal_exposures) {
        if (load.component_scope == "target_component_family_id" &&
            causal.target_component_family_id !=
                load.target_component_family_id) {
          continue;
        }
        if (load.component_scope == "target_component_family_id" &&
            proof_context.require_component_match &&
            causal.component_assembly_fingerprint !=
                proof_context.expected_component_fingerprint) {
          continue;
        }
        if (coverage.require_mutated_component && !causal.mutated_component) {
          continue;
        }
        if (std::find(causal.uses.begin(), causal.uses.end(), coverage.use) ==
            causal.uses.end()) {
          continue;
        }
        const auto interval = exposure::interval_intersection(
            causal.completed_anchor_range, coverage.target_range);
        if (!interval.empty()) {
          causal_support_intervals.push_back(interval);
        }
      }
      const auto merged_causal_support =
          certificate_merged_intervals(causal_support_intervals);
      if (!interval_vectors_equal(
              certificate_sorted_intervals(coverage.contributing_intervals),
              certificate_sorted_intervals(causal_support_intervals))) {
        append_certificate_issue(
            check, prefix + " contributing interval causal multiset mismatch");
      }
      for (std::size_t j = 0; j < coverage.contributing_intervals.size(); ++j) {
        if (!certificate_interval_covered_by(coverage.contributing_intervals[j],
                                             merged_causal_support)) {
          append_certificate_issue(
              check, prefix + " contributing interval not supported by "
                              "closure causal exposure");
          break;
        }
      }
    }
    const auto contributing_length =
        interval_vector_length(coverage.contributing_intervals);
    if (load.loaded_anchor_events != contributing_length) {
      append_certificate_issue(
          check,
          prefix + " load anchor events/contributing intervals mismatch");
    }
    if (load.fact_count !=
        static_cast<std::int64_t>(coverage.contributing_intervals.size())) {
      append_certificate_issue(
          check, prefix + " load fact count/contributing intervals mismatch");
    }
    if (load.unique_covered_anchors != coverage.covered_anchors) {
      append_certificate_issue(
          check, prefix + " load unique_covered_anchors mismatch");
    }
    if (load.loaded_anchor_events < load.unique_covered_anchors) {
      append_certificate_issue(
          check, prefix + " load anchor events below unique coverage");
    }
    if (coverage.use == "evaluation_metric" &&
        !coverage.require_mutated_component &&
        load.optimizer_steps_total != 0) {
      append_certificate_issue(
          check, prefix + " evaluation coverage load has optimizer steps");
    }
    if (load.unique_covered_anchors < 0 || load.loaded_anchor_events < 0 ||
        load.fact_count < 0 || load.optimizer_steps_total < 0 ||
        load.valid_target_count_total < 0 ||
        load.valid_target_cursor_epochs < -1e-12) {
      append_certificate_issue(check, prefix + " load summary negative count");
    }
    if (!std::isfinite(load.unique_coverage_fraction) ||
        !std::isfinite(load.cursor_exposure_load) ||
        !std::isfinite(load.optimizer_steps_per_unique_anchor) ||
        !std::isfinite(load.optimizer_steps_per_cursor_epoch) ||
        !std::isfinite(load.valid_target_cursor_epochs)) {
      append_certificate_issue(check, prefix + " non-finite load measure");
    }
    if (load.unique_coverage_fraction < -1e-12 ||
        load.cursor_exposure_load < -1e-12 ||
        load.optimizer_steps_per_unique_anchor < -1e-12 ||
        load.optimizer_steps_per_cursor_epoch < -1e-12 ||
        load.valid_target_cursor_epochs < -1e-12) {
      append_certificate_issue(check, prefix + " negative load measure");
    }
    if (!load.target_range.empty()) {
      const auto target_length =
          static_cast<double>(load.target_range.length());
      const auto expected_unique_fraction =
          static_cast<double>(load.unique_covered_anchors) / target_length;
      if (!doubles_close(load.unique_coverage_fraction,
                         expected_unique_fraction)) {
        append_certificate_issue(
            check, prefix + " load unique coverage fraction mismatch");
      }
      const auto expected_cursor_load =
          static_cast<double>(load.loaded_anchor_events) / target_length;
      if (!doubles_close(load.cursor_exposure_load, expected_cursor_load)) {
        append_certificate_issue(check,
                                 prefix + " cursor exposure load mismatch");
      }
    }
    if (load.unique_covered_anchors > 0) {
      if (!certificate_fraction_in_unit_interval(
              load.unique_coverage_fraction)) {
        append_certificate_issue(
            check, prefix + " load unique coverage fraction out of range");
      }
      const auto expected_density =
          static_cast<double>(load.optimizer_steps_total) /
          static_cast<double>(load.unique_covered_anchors);
      if (!doubles_close(load.optimizer_steps_per_unique_anchor,
                         expected_density)) {
        append_certificate_issue(
            check, prefix + " optimizer density per unique anchor mismatch");
      }
    } else if (!doubles_close(load.optimizer_steps_per_unique_anchor, 0.0)) {
      append_certificate_issue(
          check,
          prefix + " optimizer density per unique anchor without anchors");
    }
    if (load.cursor_exposure_load > 0.0) {
      const auto expected_density =
          static_cast<double>(load.optimizer_steps_total) /
          load.cursor_exposure_load;
      if (!doubles_close(load.optimizer_steps_per_cursor_epoch,
                         expected_density)) {
        append_certificate_issue(
            check, prefix + " optimizer density per cursor epoch mismatch");
      }
    } else if (!doubles_close(load.optimizer_steps_per_cursor_epoch, 0.0)) {
      append_certificate_issue(
          check, prefix + " optimizer density per cursor epoch without load");
    }
    if (load.valid_target_cursor_epochs - 1e-12 > load.cursor_exposure_load) {
      append_certificate_issue(
          check, prefix + " valid-target cursor epochs exceed exposure load");
    }
    if (load.valid_target_success_count_for_uncertainty < 0 ||
        load.valid_target_opportunity_count_for_uncertainty < 0 ||
        load.valid_target_success_count_for_uncertainty >
            load.valid_target_opportunity_count_for_uncertainty) {
      append_certificate_issue(check,
                               prefix + " valid-target uncertainty counts out "
                                        "of range");
    }
    if (load.valid_target_success_count_for_uncertainty >
        load.valid_target_count_total) {
      append_certificate_issue(
          check, prefix + " valid-target uncertainty successes exceed total");
    }
    if (load.valid_target_opportunity_count_for_uncertainty > 0) {
      const auto expected_fraction =
          static_cast<double>(load.valid_target_success_count_for_uncertainty) /
          static_cast<double>(
              load.valid_target_opportunity_count_for_uncertainty);
      if (!doubles_close(load.valid_target_fraction_estimate,
                         expected_fraction)) {
        append_certificate_issue(
            check, prefix + " valid-target fraction estimate mismatch");
      }
      if (!certificate_fraction_in_unit_interval(
              load.valid_target_fraction_estimate)) {
        append_certificate_issue(
            check, prefix + " valid-target fraction estimate out of range");
      }
      const auto [expected_lower, expected_upper] =
          exposure::wilson_score_interval_95(
              load.valid_target_success_count_for_uncertainty,
              load.valid_target_opportunity_count_for_uncertainty);
      if (!doubles_close(load.valid_target_wilson_lower_95, expected_lower) ||
          !doubles_close(load.valid_target_wilson_upper_95, expected_upper)) {
        append_certificate_issue(
            check, prefix + " valid-target Wilson interval mismatch");
      }
      if (!certificate_fraction_in_unit_interval(
              load.valid_target_wilson_lower_95) ||
          !certificate_fraction_in_unit_interval(
              load.valid_target_wilson_upper_95) ||
          load.valid_target_wilson_lower_95 >
              load.valid_target_wilson_upper_95) {
        append_certificate_issue(
            check, prefix + " valid-target Wilson interval out of range");
      }
    } else if (std::isfinite(load.valid_target_fraction_estimate) ||
               std::isfinite(load.valid_target_wilson_lower_95) ||
               std::isfinite(load.valid_target_wilson_upper_95)) {
      append_certificate_issue(
          check, prefix + " valid-target uncertainty estimate without trials");
    }
  }

  if (!proof.closure.checked) {
    if (!proof.closure.checkpoint_path.empty() || proof.closure.complete ||
        proof.closure.fact_count != 0 ||
        !proof.closure.resolution_authority.empty() ||
        !proof.closure.root_checkpoint_id.empty() ||
        !proof.closure.root_checkpoint_file_digest.empty() ||
        !proof.closure.identity_mismatches.empty() ||
        !proof.closure.unresolved_input_checkpoints.empty() ||
        !proof.closure.fact_digests.empty() ||
        !proof.closure.causal_exposures.empty() ||
        !proof.closure.checkpoint_identity_previews.empty()) {
      append_certificate_issue(check, "closure unchecked with proof fields");
    }
  } else {
    if (proof.closure.checkpoint_path.empty()) {
      append_certificate_issue(check,
                               "closure checked without checkpoint path");
    }
    if (proof.closure.fact_count !=
        static_cast<std::int64_t>(proof.closure.fact_digests.size())) {
      append_certificate_issue(check,
                               "closure fact_count/fact_digests mismatch");
    }
    if (proof.closure.fact_count !=
        static_cast<std::int64_t>(proof.closure.causal_exposures.size())) {
      append_certificate_issue(check,
                               "closure fact_count/causal_exposures mismatch");
    }
    if (proof.closure.complete &&
        !proof.closure.unresolved_input_checkpoints.empty()) {
      append_certificate_issue(check,
                               "closure complete with unresolved checkpoints");
    }
    if (proof.closure.complete && !proof.closure.identity_mismatches.empty()) {
      append_certificate_issue(check,
                               "closure complete with identity mismatches");
    }
    if (proof.closure.complete && proof.closure.fact_count == 0) {
      append_certificate_issue(check, "closure complete without facts");
    }
    if (!proof.closure.complete &&
        proof.closure.unresolved_input_checkpoints.empty() &&
        proof.closure.identity_mismatches.empty()) {
      append_certificate_issue(
          check,
          "closure incomplete without unresolved checkpoints or identity "
          "mismatches");
    }
    if (proof.closure.resolution_authority.empty()) {
      append_certificate_issue(check,
                               "closure checked without resolution authority");
    } else if (proof.closure.resolution_authority !=
                   "checkpoint_id_file_digest" &&
               proof.closure.resolution_authority != "unresolved_lineage" &&
               proof.closure.resolution_authority !=
                   "checkpoint_identity_failed") {
      append_certificate_issue(check, "closure unknown resolution authority");
    }
    if (proof.closure.resolution_authority == "checkpoint_id_file_digest") {
      if (proof.closure.root_checkpoint_id.empty() ||
          proof.closure.root_checkpoint_file_digest.empty()) {
        append_certificate_issue(
            check, "checkpoint_id_file_digest authority missing root identity");
      }
    }
    if (proof.closure.resolution_authority == "checkpoint_identity_failed" &&
        proof.closure.identity_mismatches.empty()) {
      append_certificate_issue(
          check, "checkpoint_identity_failed without identity mismatches");
    }
    for (std::size_t i = 0; i < proof.closure.identity_mismatches.size(); ++i) {
      if (proof.closure.identity_mismatches[i].empty()) {
        append_certificate_issue(check, "closure identity_mismatch[" +
                                            std::to_string(i) +
                                            "] empty message");
      }
    }
    for (std::size_t i = 0;
         i < proof.closure.unresolved_input_checkpoints.size(); ++i) {
      if (proof.closure.unresolved_input_checkpoints[i].empty()) {
        append_certificate_issue(check, "closure unresolved checkpoint[" +
                                            std::to_string(i) + "] empty path");
      }
    }
    std::vector<std::string> unresolved_checkpoint_paths;
    unresolved_checkpoint_paths.reserve(
        proof.closure.unresolved_input_checkpoints.size());
    for (const auto &path : proof.closure.unresolved_input_checkpoints) {
      unresolved_checkpoint_paths.push_back(path.lexically_normal().string());
    }
    std::sort(unresolved_checkpoint_paths.begin(),
              unresolved_checkpoint_paths.end());
    if (std::adjacent_find(unresolved_checkpoint_paths.begin(),
                           unresolved_checkpoint_paths.end()) !=
        unresolved_checkpoint_paths.end()) {
      append_certificate_issue(check,
                               "closure duplicate unresolved checkpoint");
    }
    bool root_checkpoint_producer_found = false;
    bool root_checkpoint_producer_assembly_found = false;
    std::vector<std::string> causal_digests;
    std::vector<std::string> causal_input_edge_paths;
    std::vector<std::string> causal_output_checkpoint_paths;
    std::unordered_map<std::string, std::vector<std::string>>
        causal_inputs_by_output_checkpoint;
    causal_digests.reserve(proof.closure.causal_exposures.size());
    for (std::size_t i = 0; i < proof.closure.causal_exposures.size(); ++i) {
      const auto &causal = proof.closure.causal_exposures[i];
      const auto prefix = "closure.causal_exposure[" + std::to_string(i) + "]";
      if (!proof.closure.checkpoint_path.empty() &&
          !causal.output_checkpoint.empty() &&
          certificate_paths_equal(causal.output_checkpoint,
                                  proof.closure.checkpoint_path)) {
        root_checkpoint_producer_found = true;
        if (!proof_context.require_component_match ||
            causal.component_assembly_fingerprint ==
                proof_context.expected_component_fingerprint) {
          root_checkpoint_producer_assembly_found = true;
        }
      }
      if (causal.fact_digest.empty()) {
        append_certificate_issue(check, prefix + " missing fact digest");
      }
      causal_digests.push_back(causal.fact_digest);
      if (causal.cursor_domain != "ujcamei.graph_anchor") {
        append_certificate_issue(check, prefix + " cursor_domain mismatch");
      }
      if (proof_context.require_contract_match &&
          causal.contract_fingerprint !=
              proof_context.active_contract_fingerprint) {
        append_certificate_issue(check, prefix + " contract identity mismatch");
      }
      if (proof_context.require_graph_anchor_identity &&
          causal.graph_order_fingerprint !=
              proof_context.active_graph_order_fingerprint) {
        append_certificate_issue(check,
                                 prefix + " graph-order identity mismatch");
      }
      if (proof_context.require_graph_anchor_identity &&
          causal.source_cursor_token !=
              proof_context.active_source_cursor_token) {
        append_certificate_issue(check,
                                 prefix + " source-cursor identity mismatch");
      }
      if (!proof.split_policy_fingerprint.empty() &&
          causal.split_policy_fingerprint.empty() &&
          certificate_names_concrete_split(causal.split_name)) {
        append_certificate_issue(check,
                                 prefix + " missing split-policy identity");
      } else if (!proof.split_policy_fingerprint.empty() &&
                 !causal.split_policy_fingerprint.empty() &&
                 causal.split_policy_fingerprint !=
                     proof.split_policy_fingerprint) {
        append_certificate_issue(check,
                                 prefix + " split-policy identity mismatch");
      }
      const auto expected_source_key_audit =
          certificate_source_key_window_audit_from_causal(causal);
      if (!certificate_source_key_window_audits_equal(
              causal.source_key_window_audit, expected_source_key_audit)) {
        append_certificate_issue(check,
                                 prefix + " source-key window audit mismatch");
      }
      if (causal.job_id.empty()) {
        append_certificate_issue(check, prefix + " missing job_id");
      }
      if (causal.wave_id.empty()) {
        append_certificate_issue(check, prefix + " missing wave_id");
      }
      if (causal.wave_action.empty()) {
        append_certificate_issue(check, prefix + " missing wave_action");
      }
      if (causal.job_status != "completed") {
        append_certificate_issue(check, prefix + " job_status not completed");
      }
      const bool has_handoff_id = !causal.runtime_handoff_id.empty();
      const bool has_handoff_digest = !causal.runtime_handoff_digest.empty();
      if (has_handoff_id != has_handoff_digest) {
        append_certificate_issue(
            check, prefix + " incomplete runtime handoff binding");
      }
      if (has_handoff_id && has_handoff_digest &&
          causal.runtime_handoff_id !=
              "runtime_handoff_" + causal.runtime_handoff_digest) {
        append_certificate_issue(
            check, prefix + " runtime handoff id/digest mismatch");
      }
      if (causal.target_component_family_id.empty()) {
        append_certificate_issue(
            check, prefix + " missing target_component_family_id");
      }
      if (causal.component_assembly_fingerprint.empty()) {
        append_certificate_issue(check, prefix + " missing component assembly");
      }
      if (causal.uses.empty()) {
        append_certificate_issue(check, prefix + " missing exposure uses");
      }
      std::set<std::string> causal_uses;
      for (const auto &use : causal.uses) {
        if (!certificate_exposure_use_known(use)) {
          append_certificate_issue(check, prefix + " unknown exposure use");
        }
        if (!causal_uses.insert(use).second) {
          append_certificate_issue(check, prefix + " duplicate exposure use");
        }
      }
      if (causal.anchor_range.empty()) {
        append_certificate_issue(check, prefix + " empty anchor range");
      }
      if (causal.completed_anchor_range.empty()) {
        append_certificate_issue(check, prefix + " empty completed range");
      }
      if (!causal.completed_anchor_range.empty() &&
          (causal.completed_anchor_range.begin < causal.anchor_range.begin ||
           causal.completed_anchor_range.end > causal.anchor_range.end)) {
        append_certificate_issue(
            check, prefix + " completed range outside anchor range");
      }
      if (causal_uses.count("observed_input") != 0 &&
          causal.observed_footprint.empty()) {
        append_certificate_issue(check,
                                 prefix + " observed_input empty footprint");
      }
      if (causal_uses.count("target_supervision") != 0 &&
          causal.target_footprint.empty()) {
        append_certificate_issue(
            check, prefix + " target_supervision empty footprint");
      }
      if ((causal_uses.count("evaluation_metric") != 0 ||
           causal_uses.count("selection_signal") != 0) &&
          causal.anchor_range.empty()) {
        append_certificate_issue(check,
                                 prefix + " metric/selection empty footprint");
      }
      if (causal.output_checkpoint.empty()) {
        append_certificate_issue(check, prefix + " missing output checkpoint");
      }
      if (!causal.output_checkpoint.empty()) {
        const auto output_checkpoint_path =
            causal.output_checkpoint.lexically_normal().string();
        causal_output_checkpoint_paths.push_back(output_checkpoint_path);
        causal_inputs_by_output_checkpoint[output_checkpoint_path];
      }
      if (causal.output_checkpoint.empty() &&
          causal.input_checkpoints.empty()) {
        append_certificate_issue(check,
                                 prefix + " has no checkpoint edge labels");
      }
      for (std::size_t j = 0; j < causal.input_checkpoints.size(); ++j) {
        if (causal.input_checkpoints[j].empty()) {
          append_certificate_issue(check, prefix + " input_checkpoint[" +
                                              std::to_string(j) +
                                              "] empty path");
        }
      }
      std::vector<std::string> input_checkpoint_paths;
      input_checkpoint_paths.reserve(causal.input_checkpoints.size());
      for (const auto &path : causal.input_checkpoints) {
        const auto normalized = path.lexically_normal().string();
        input_checkpoint_paths.push_back(normalized);
        if (!path.empty()) {
          causal_input_edge_paths.push_back(normalized);
          if (!causal.output_checkpoint.empty() &&
              certificate_paths_equal(path, causal.output_checkpoint)) {
            append_certificate_issue(
                check, prefix + " input checkpoint equals output checkpoint");
          }
          if (!proof.closure.checkpoint_path.empty() &&
              certificate_paths_equal(path, proof.closure.checkpoint_path)) {
            append_certificate_issue(
                check, prefix + " input checkpoint equals closure root");
          }
        }
      }
      if (!causal.output_checkpoint.empty()) {
        const auto output_checkpoint_path =
            causal.output_checkpoint.lexically_normal().string();
        auto &checkpoint_inputs =
            causal_inputs_by_output_checkpoint[output_checkpoint_path];
        checkpoint_inputs.insert(checkpoint_inputs.end(),
                                 input_checkpoint_paths.begin(),
                                 input_checkpoint_paths.end());
      }
      std::sort(input_checkpoint_paths.begin(), input_checkpoint_paths.end());
      if (std::adjacent_find(input_checkpoint_paths.begin(),
                             input_checkpoint_paths.end()) !=
          input_checkpoint_paths.end()) {
        append_certificate_issue(check,
                                 prefix + " duplicate input checkpoint edge");
      }
    }
    std::sort(causal_input_edge_paths.begin(), causal_input_edge_paths.end());
    causal_input_edge_paths.erase(std::unique(causal_input_edge_paths.begin(),
                                              causal_input_edge_paths.end()),
                                  causal_input_edge_paths.end());
    std::sort(causal_output_checkpoint_paths.begin(),
              causal_output_checkpoint_paths.end());
    causal_output_checkpoint_paths.erase(
        std::unique(causal_output_checkpoint_paths.begin(),
                    causal_output_checkpoint_paths.end()),
        causal_output_checkpoint_paths.end());
    std::set<std::string> reachable_checkpoint_paths;
    std::set<std::string> active_checkpoint_paths;
    if (!proof.closure.checkpoint_path.empty()) {
      const auto root_checkpoint_path =
          proof.closure.checkpoint_path.lexically_normal().string();
      const auto visit_checkpoint =
          [&](auto &&self, const std::string &checkpoint_path) -> void {
        if (checkpoint_path.empty()) {
          return;
        }
        if (active_checkpoint_paths.count(checkpoint_path) != 0) {
          append_certificate_issue(check, "closure causal checkpoint cycle");
          return;
        }
        if (reachable_checkpoint_paths.count(checkpoint_path) != 0) {
          return;
        }
        reachable_checkpoint_paths.insert(checkpoint_path);
        const auto producer =
            causal_inputs_by_output_checkpoint.find(checkpoint_path);
        if (producer == causal_inputs_by_output_checkpoint.end()) {
          return;
        }
        active_checkpoint_paths.insert(checkpoint_path);
        for (const auto &input_path : producer->second) {
          if (causal_inputs_by_output_checkpoint.count(input_path) != 0) {
            self(self, input_path);
          }
        }
        active_checkpoint_paths.erase(checkpoint_path);
      };
      visit_checkpoint(visit_checkpoint, root_checkpoint_path);
    }
    for (const auto &input_path : causal_input_edge_paths) {
      const bool resolved_by_producer =
          std::binary_search(causal_output_checkpoint_paths.begin(),
                             causal_output_checkpoint_paths.end(), input_path);
      const bool recorded_unresolved =
          std::binary_search(unresolved_checkpoint_paths.begin(),
                             unresolved_checkpoint_paths.end(), input_path);
      if (!resolved_by_producer && !recorded_unresolved) {
        append_certificate_issue(
            check,
            "closure input checkpoint not resolved by producer or unresolved "
            "witness");
      }
    }
    for (const auto &unresolved_path : unresolved_checkpoint_paths) {
      if (unresolved_path.empty()) {
        continue;
      }
      if (std::binary_search(causal_output_checkpoint_paths.begin(),
                             causal_output_checkpoint_paths.end(),
                             unresolved_path)) {
        append_certificate_issue(check,
                                 "closure unresolved checkpoint has producer");
      }
      if (!std::binary_search(causal_input_edge_paths.begin(),
                              causal_input_edge_paths.end(), unresolved_path)) {
        append_certificate_issue(
            check, "closure unresolved checkpoint not reached by input edge");
      }
    }
    for (const auto &output_path : causal_output_checkpoint_paths) {
      if (reachable_checkpoint_paths.count(output_path) == 0) {
        append_certificate_issue(
            check, "closure output checkpoint not reachable from root");
      }
    }
    for (std::size_t i = 0; i < proof.closure.causal_exposures.size(); ++i) {
      const auto &causal = proof.closure.causal_exposures[i];
      const auto prefix = "closure.causal_exposure[" + std::to_string(i) + "]";
      if (causal.output_checkpoint.empty() ||
          certificate_paths_equal(causal.output_checkpoint,
                                  proof.closure.checkpoint_path)) {
        continue;
      }
      const auto output_key =
          causal.output_checkpoint.lexically_normal().string();
      if (!std::binary_search(causal_input_edge_paths.begin(),
                              causal_input_edge_paths.end(), output_key)) {
        append_certificate_issue(
            check, prefix + " output checkpoint not reached by input edge");
      }
    }
    if (!root_checkpoint_producer_found) {
      append_certificate_issue(check,
                               "closure missing root checkpoint producer");
    } else if (proof_context.require_component_match &&
               !root_checkpoint_producer_assembly_found) {
      append_certificate_issue(
          check, "closure root checkpoint producer assembly mismatch");
    }
    auto fact_digests = proof.closure.fact_digests;
    std::sort(fact_digests.begin(), fact_digests.end());
    std::sort(causal_digests.begin(), causal_digests.end());
    if (fact_digests != causal_digests) {
      append_certificate_issue(
          check, "closure fact_digests/causal_exposure digests mismatch");
    }
    if (std::adjacent_find(fact_digests.begin(), fact_digests.end()) !=
        fact_digests.end()) {
      append_certificate_issue(check, "closure duplicate fact digest");
    }
    std::set<std::string> checkpoint_preview_paths;
    std::set<std::string> checkpoint_preview_ids;
    for (std::size_t i = 0;
         i < proof.closure.checkpoint_identity_previews.size(); ++i) {
      const auto &preview = proof.closure.checkpoint_identity_previews[i];
      const auto prefix =
          "closure.checkpoint_identity_preview[" + std::to_string(i) + "]";
      const auto checkpoint_path =
          preview.checkpoint_path.lexically_normal().string();
      if (checkpoint_path.empty()) {
        append_certificate_issue(check, prefix + " missing checkpoint path");
      } else if (!checkpoint_preview_paths.insert(checkpoint_path).second) {
        append_certificate_issue(check, prefix + " duplicate checkpoint path");
      } else if (!std::binary_search(causal_output_checkpoint_paths.begin(),
                                     causal_output_checkpoint_paths.end(),
                                     checkpoint_path)) {
        append_certificate_issue(check, prefix + " path not in causal closure");
      }
      if (preview.checkpoint_id.empty()) {
        append_certificate_issue(check, prefix + " missing checkpoint id");
      } else if (!checkpoint_preview_ids.insert(preview.checkpoint_id).second) {
        append_certificate_issue(check, prefix + " duplicate checkpoint id");
      }
      if (preview.checkpoint_file_digest.empty()) {
        append_certificate_issue(check,
                                 prefix + " missing checkpoint file digest");
      }
      if (preview.direct_exposure_digest.empty()) {
        append_certificate_issue(check,
                                 prefix + " missing direct exposure digest");
      } else if (!std::binary_search(fact_digests.begin(), fact_digests.end(),
                                     preview.direct_exposure_digest)) {
        append_certificate_issue(
            check, prefix + " direct exposure digest not in closure");
      }
      if (preview.component.empty()) {
        append_certificate_issue(check, prefix + " missing component");
      }
      if (preview.component_assembly_fingerprint.empty()) {
        append_certificate_issue(check, prefix + " missing component assembly");
      }
      const auto causal = std::find_if(
          proof.closure.causal_exposures.begin(),
          proof.closure.causal_exposures.end(), [&](const auto &candidate) {
            return candidate.fact_digest == preview.direct_exposure_digest ||
                   (!checkpoint_path.empty() &&
                    candidate.output_checkpoint.lexically_normal().string() ==
                        checkpoint_path);
          });
      if (causal != proof.closure.causal_exposures.end()) {
        if (preview.component != causal->target_component_family_id) {
          append_certificate_issue(check, prefix + " component mismatch");
        }
        if (preview.component_assembly_fingerprint !=
            causal->component_assembly_fingerprint) {
          append_certificate_issue(check,
                                   prefix + " component assembly mismatch");
        }
        const auto append_optional_mismatch =
            [&](const std::string &field_name, const std::string &preview_value,
                const std::string &causal_value) {
              if (!preview_value.empty() && !causal_value.empty() &&
                  preview_value != causal_value) {
                append_certificate_issue(check, prefix + " " + field_name +
                                                    " mismatch");
              }
            };
        append_optional_mismatch("representation architecture",
                                 preview.representation_architecture,
                                 causal->representation_architecture);
        append_optional_mismatch("representation contract",
                                 preview.representation_contract,
                                 causal->representation_contract);
        append_optional_mismatch("representation value shape",
                                 preview.representation_value_shape,
                                 causal->representation_value_shape);
        append_optional_mismatch("representation sequence value shape",
                                 preview.representation_sequence_value_shape,
                                 causal->representation_sequence_value_shape);
        append_optional_mismatch("channel axis policy",
                                 preview.channel_axis_policy,
                                 causal->channel_axis_policy);
        append_optional_mismatch("temporal reduction",
                                 preview.temporal_reduction,
                                 causal->temporal_reduction);
        append_optional_mismatch("input representation id",
                                 preview.input_representation_assembly_id,
                                 causal->input_representation_assembly_id);
        append_optional_mismatch("context mode", preview.context_mode,
                                 causal->context_mode);
        append_optional_mismatch("context contract", preview.context_contract,
                                 causal->context_contract);
        append_optional_mismatch("context value shape",
                                 preview.context_value_shape,
                                 causal->context_value_shape);
        append_optional_mismatch("output contract", preview.output_contract,
                                 causal->output_contract);
        append_optional_mismatch("output value shape",
                                 preview.output_value_shape,
                                 causal->output_value_shape);
        if (!preview.created_by_job_id.empty() &&
            preview.created_by_job_id != causal->job_id) {
          append_certificate_issue(check, prefix + " job id mismatch");
        }
        if (!preview.created_by_wave_id.empty() &&
            preview.created_by_wave_id != causal->wave_id) {
          append_certificate_issue(check, prefix + " wave id mismatch");
        }
      }
    }
  }

  const auto &leakage = proof.leakage;
  if (!leakage.checked) {
    if (!leakage.split_name.empty() || !leakage.base_range.empty() ||
        !leakage.protected_range.empty() || leakage.dilation_left != 0 ||
        leakage.dilation_right != 0 || !leakage.forbidden_uses.empty() ||
        leakage.overlap_found || !leakage.overlap_witnesses.empty() ||
        !leakage.passed) {
      append_certificate_issue(check, "leakage unchecked with proof fields");
    }
  } else {
    const std::set<std::string> closure_fact_digests(
        proof.closure.fact_digests.begin(), proof.closure.fact_digests.end());
    if (!proof.closure.checked) {
      append_certificate_issue(check, "leakage checked without closure proof");
    } else if (!proof.closure.complete) {
      append_certificate_issue(check,
                               "leakage checked with incomplete closure");
    }
    if (leakage.rule != k_lattice_leakage_rule) {
      append_certificate_issue(check, "leakage rule mismatch");
    }
    if (!leakage.split_name.empty() && proof.split_policy_fingerprint.empty()) {
      append_certificate_issue(check,
                               "leakage missing split policy fingerprint");
    }
    if (leakage.protected_range.empty()) {
      append_certificate_issue(check, "leakage protected range empty");
    }
    if (leakage.base_range.empty()) {
      append_certificate_issue(check, "leakage base range empty");
    }
    if (leakage.dilation_left < 0 || leakage.dilation_right < 0) {
      append_certificate_issue(check, "leakage negative dilation");
    }
    if (!leakage.base_range.empty()) {
      const exposure::anchor_interval_t expected_protected_range{
          .begin = leakage.base_range.begin - leakage.dilation_left,
          .end = leakage.base_range.end + leakage.dilation_right,
      };
      if (expected_protected_range.begin != leakage.protected_range.begin ||
          expected_protected_range.end != leakage.protected_range.end) {
        append_certificate_issue(check,
                                 "leakage protected range dilation mismatch");
      }
      if (leakage.protected_range.begin > leakage.base_range.begin ||
          leakage.protected_range.end < leakage.base_range.end) {
        append_certificate_issue(check,
                                 "leakage protected range does not contain "
                                 "base range");
      }
    }
    if (leakage.forbidden_uses.empty()) {
      append_certificate_issue(check, "leakage missing forbidden uses");
    }
    std::set<std::string> forbidden_uses;
    for (const auto &use : leakage.forbidden_uses) {
      if (!certificate_exposure_use_known(use)) {
        append_certificate_issue(check, "leakage unknown forbidden use");
      }
      if (!forbidden_uses.insert(use).second) {
        append_certificate_issue(check, "leakage duplicate forbidden use");
      }
    }
    if (leakage.overlap_found != !leakage.overlap_witnesses.empty()) {
      append_certificate_issue(check, "leakage overlap flag/witness mismatch");
    }
    if (leakage.passed != !leakage.overlap_found) {
      append_certificate_issue(check, "leakage passed flag mismatch");
    }
    std::vector<std::string> expected_witness_signatures;
    for (const auto &causal : proof.closure.causal_exposures) {
      if (leakage.require_mutated_component && !causal.mutated_component) {
        continue;
      }
      for (const auto &use_name : forbidden_uses) {
        if (std::find(causal.uses.begin(), causal.uses.end(), use_name) ==
            causal.uses.end()) {
          continue;
        }
        const auto use = certificate_exposure_use_from_name(use_name);
        if (!use.has_value()) {
          continue;
        }
        const auto footprint =
            certificate_source_footprint_for_causal_use(causal, *use);
        const auto intersection =
            exposure::interval_intersection(footprint, leakage.protected_range);
        if (intersection.empty()) {
          continue;
        }
        exposure::forbidden_exposure_overlap_t expected{};
        expected.fact_digest = causal.fact_digest;
        expected.job_id = causal.job_id;
        expected.wave_id = causal.wave_id;
        expected.target_component_family_id = causal.target_component_family_id;
        expected.split_name = causal.split_name;
        expected.use = *use;
        expected.mutated_component = causal.mutated_component;
        expected.source_footprint = footprint;
        expected.protected_range = leakage.protected_range;
        expected.intersection = intersection;
        if (*use == exposure::exposure_use_t::selection_signal) {
          exposure::lattice_exposure_fact_t selection_fact{};
          selection_fact.contract_fingerprint = causal.contract_fingerprint;
          selection_fact.graph_order_fingerprint =
              causal.graph_order_fingerprint;
          selection_fact.source_cursor_token = causal.source_cursor_token;
          selection_fact.split_policy_fingerprint =
              causal.split_policy_fingerprint;
          selection_fact.component_assembly_fingerprint =
              causal.component_assembly_fingerprint;
          selection_fact.target_component_family_id =
              causal.target_component_family_id;
          selection_fact.job_id = causal.job_id;
          selection_fact.wave_id = causal.wave_id;
          selection_fact.job_status = causal.job_status;
          selection_fact.wave_action = causal.wave_action;
          selection_fact.cursor_domain = causal.cursor_domain;
          selection_fact.split_name = causal.split_name;
          selection_fact.split_role = causal.split_role;
          selection_fact.anchor_range = causal.anchor_range;
          selection_fact.completed_anchor_range = causal.completed_anchor_range;
          selection_fact.observed_footprint = causal.observed_footprint;
          selection_fact.target_footprint = causal.target_footprint;
          selection_fact.first_anchor_key = causal.first_anchor_key;
          selection_fact.last_anchor_key = causal.last_anchor_key;
          selection_fact.observed_source_key_begin =
              causal.observed_source_key_begin;
          selection_fact.observed_source_key_end =
              causal.observed_source_key_end;
          selection_fact.target_source_key_begin =
              causal.target_source_key_begin;
          selection_fact.target_source_key_end = causal.target_source_key_end;
          selection_fact.source_key_footprint_precision =
              causal.source_key_footprint_precision;
          selection_fact.coverage_precision = "contiguous_completed_range_v1";
          selection_fact.output_checkpoint = causal.output_checkpoint;
          selection_fact.input_checkpoints = causal.input_checkpoints;
          selection_fact.use.selection_signal = true;
          selection_fact.use.mutated_component = causal.mutated_component;
          auto selection_event =
              exposure::make_selection_signal_fact_from_exposure_fact(
                  selection_fact);
          selection_event.parent_exposure_fact_digest = causal.fact_digest;
          expected.selector_id = selection_event.selector_id;
          expected.selector_kind = selection_event.selector_kind;
          expected.selection_event_digest =
              exposure::selection_signal_fact_digest(selection_event);
          expected.selected_checkpoint = selection_event.selected_checkpoint;
        }
        expected_witness_signatures.push_back(
            certificate_leakage_witness_signature(expected));
      }
    }
    std::vector<std::string> actual_witness_signatures;
    actual_witness_signatures.reserve(leakage.overlap_witnesses.size());
    for (const auto &witness : leakage.overlap_witnesses) {
      actual_witness_signatures.push_back(
          certificate_leakage_witness_signature(witness));
    }
    std::sort(expected_witness_signatures.begin(),
              expected_witness_signatures.end());
    std::sort(actual_witness_signatures.begin(),
              actual_witness_signatures.end());
    if (std::adjacent_find(actual_witness_signatures.begin(),
                           actual_witness_signatures.end()) !=
        actual_witness_signatures.end()) {
      append_certificate_issue(check, "leakage duplicate overlap witness");
    }
    if (actual_witness_signatures != expected_witness_signatures) {
      append_certificate_issue(check,
                               "leakage witness set does not match causal "
                               "closure overlaps");
    }
    for (std::size_t i = 0; i < leakage.overlap_witnesses.size(); ++i) {
      const auto &witness = leakage.overlap_witnesses[i];
      const auto prefix = "leakage.overlap_witness[" + std::to_string(i) + "]";
      if (witness.fact_digest.empty()) {
        append_certificate_issue(check, prefix + " missing fact digest");
      } else if (proof.closure.checked &&
                 closure_fact_digests.count(witness.fact_digest) == 0) {
        append_certificate_issue(check, prefix + " fact digest not in closure");
      }
      if (witness.job_id.empty()) {
        append_certificate_issue(check, prefix + " missing job_id");
      }
      if (witness.wave_id.empty()) {
        append_certificate_issue(check, prefix + " missing wave_id");
      }
      if (witness.target_component_family_id.empty()) {
        append_certificate_issue(check, prefix + " missing target component");
      }
      const auto witness_use = exposure::exposure_use_name(witness.use);
      const auto causal_exposure = std::find_if(
          proof.closure.causal_exposures.begin(),
          proof.closure.causal_exposures.end(), [&](const auto &causal) {
            return causal.fact_digest == witness.fact_digest;
          });
      if (causal_exposure != proof.closure.causal_exposures.end()) {
        if (witness.job_id != causal_exposure->job_id) {
          append_certificate_issue(check, prefix + " causal job mismatch");
        }
        if (witness.wave_id != causal_exposure->wave_id) {
          append_certificate_issue(check, prefix + " causal wave mismatch");
        }
        if (witness.target_component_family_id !=
            causal_exposure->target_component_family_id) {
          append_certificate_issue(check,
                                   prefix + " causal component mismatch");
        }
        if (witness.split_name != causal_exposure->split_name) {
          append_certificate_issue(check, prefix + " causal split mismatch");
        }
        if (witness.mutated_component != causal_exposure->mutated_component) {
          append_certificate_issue(check, prefix + " causal mutation mismatch");
        }
        if (std::find(causal_exposure->uses.begin(),
                      causal_exposure->uses.end(),
                      witness_use) == causal_exposure->uses.end()) {
          append_certificate_issue(check, prefix + " causal use mismatch");
        }
        exposure::anchor_interval_t expected_source_footprint{};
        expected_source_footprint = certificate_source_footprint_for_causal_use(
            *causal_exposure, witness.use);
        if (expected_source_footprint.begin != witness.source_footprint.begin ||
            expected_source_footprint.end != witness.source_footprint.end) {
          append_certificate_issue(check,
                                   prefix + " causal footprint mismatch");
        }
      }
      if (forbidden_uses.count(witness_use) == 0) {
        append_certificate_issue(check, prefix + " use not forbidden");
      }
      if (leakage.require_mutated_component && !witness.mutated_component) {
        append_certificate_issue(check, prefix + " missing required mutation");
      }
      if (witness.use == exposure::exposure_use_t::selection_signal) {
        if (witness.selector_id.empty()) {
          append_certificate_issue(check, prefix + " missing selector id");
        }
        if (witness.selector_kind.empty()) {
          append_certificate_issue(check, prefix + " missing selector kind");
        }
        if (witness.selection_event_digest.empty()) {
          append_certificate_issue(check,
                                   prefix + " missing selection event digest");
        }
        if (witness.selected_checkpoint.empty()) {
          append_certificate_issue(check,
                                   prefix + " missing selected checkpoint");
        }
      }
      if (witness.protected_range.begin != leakage.protected_range.begin ||
          witness.protected_range.end != leakage.protected_range.end) {
        append_certificate_issue(check, prefix + " protected range mismatch");
      }
      const auto expected_intersection = exposure::interval_intersection(
          witness.source_footprint, leakage.protected_range);
      if (expected_intersection.begin != witness.intersection.begin ||
          expected_intersection.end != witness.intersection.end) {
        append_certificate_issue(check, prefix + " intersection mismatch");
      }
      if (witness.intersection.empty()) {
        append_certificate_issue(check, prefix + " empty intersection");
      }
    }
  }

  std::set<std::string> node_support_summary_signatures;
  for (std::size_t i = 0; i < proof.node_support_summaries.size(); ++i) {
    const auto &summary = proof.node_support_summaries[i];
    const auto prefix = "node_support_summary[" + std::to_string(i) + "]";
    std::ostringstream signature;
    signature << summary.target_component_family_id << "\n"
              << summary.split_name << "\n"
              << (summary.use.observed_input ? "1" : "0")
              << (summary.use.target_supervision ? "1" : "0")
              << (summary.use.evaluation_metric ? "1" : "0")
              << (summary.use.selection_signal ? "1" : "0")
              << (summary.use.mutated_component ? "1" : "0");
    if (!node_support_summary_signatures.insert(signature.str()).second) {
      append_certificate_issue(check,
                               prefix + " duplicate node-support summary");
    }
    if (summary.schema != "kikijyeba.lattice.node_support_summary.v1") {
      append_certificate_issue(check, prefix + " schema mismatch");
    }
    if (static_cast<std::int64_t>(summary.support_rows.size()) !=
        summary.node_count) {
      append_certificate_issue(check, prefix + " support row count mismatch");
    }
    std::int64_t row_routed_total = 0;
    std::int64_t row_active_total = 0;
    std::int64_t row_trained_total = 0;
    std::int64_t row_evaluated_total = 0;
    std::int64_t row_valid_target_total = 0;
    std::int64_t row_valid_target_opportunity_total = 0;
    std::int64_t row_finite_fraction_count = 0;
    std::int64_t row_observed_input_support_row_count = 0;
    std::int64_t row_target_supervision_support_row_count = 0;
    std::int64_t row_evaluation_metric_support_row_count = 0;
    std::int64_t row_selection_signal_support_row_count = 0;
    std::int64_t row_mutating_support_row_count = 0;
    std::int64_t row_non_mutating_support_row_count = 0;
    double row_fraction_sum = 0.0;
    double row_min_fraction = std::numeric_limits<double>::quiet_NaN();
    double row_max_fraction = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> row_valid_counts;
    bool have_row_weakest = false;
    exposure::node_support_row_t row_weakest{};
    bool have_previous_support_row = false;
    std::int64_t previous_support_row_index = 0;
    std::string previous_support_row_id;
    std::string previous_support_row_parent_digest;
    std::set<std::pair<std::int64_t, std::string>> row_unique_nodes;
    const std::set<std::string> node_closure_fact_digests(
        proof.closure.fact_digests.begin(), proof.closure.fact_digests.end());
    for (std::size_t row_i = 0; row_i < summary.support_rows.size(); ++row_i) {
      const auto &row = summary.support_rows[row_i];
      const auto row_prefix =
          prefix + ".support_row[" + std::to_string(row_i) + "]";
      if (row.parent_exposure_fact_digest.empty()) {
        append_certificate_issue(check, row_prefix +
                                            " missing parent exposure digest");
      } else if (proof.closure.checked && row.use.mutated_component &&
                 node_closure_fact_digests.count(
                     row.parent_exposure_fact_digest) == 0) {
        append_certificate_issue(check, row_prefix +
                                            " mutating parent exposure not in "
                                            "closure");
      }
      if (row.node_id.empty()) {
        append_certificate_issue(check, row_prefix + " missing node id");
      }
      row_unique_nodes.insert({row.node_index, row.node_id});
      if (!row.use.observed_input && !row.use.target_supervision &&
          !row.use.evaluation_metric && !row.use.selection_signal) {
        append_certificate_issue(check, row_prefix + " missing exposure use");
      }
      if (row.use.observed_input) {
        ++row_observed_input_support_row_count;
      }
      if (row.use.target_supervision) {
        ++row_target_supervision_support_row_count;
      }
      if (row.use.evaluation_metric) {
        ++row_evaluation_metric_support_row_count;
      }
      if (row.use.selection_signal) {
        ++row_selection_signal_support_row_count;
      }
      if (row.use.mutated_component) {
        ++row_mutating_support_row_count;
      } else {
        ++row_non_mutating_support_row_count;
      }
      if (row.node_index < 0 ||
          (summary.node_count >= 0 && row.node_index >= summary.node_count)) {
        append_certificate_issue(check,
                                 row_prefix + " node index out of range");
      }
      if (have_previous_support_row &&
          (row.node_index < previous_support_row_index ||
           (row.node_index == previous_support_row_index &&
            (row.node_id < previous_support_row_id ||
             (row.node_id == previous_support_row_id &&
              row.parent_exposure_fact_digest <
                  previous_support_row_parent_digest))))) {
        append_certificate_issue(check, prefix + " support rows not canonical");
      }
      have_previous_support_row = true;
      previous_support_row_index = row.node_index;
      previous_support_row_id = row.node_id;
      previous_support_row_parent_digest = row.parent_exposure_fact_digest;
      if (row.routed_row_count < 0 || row.active_row_count < 0 ||
          row.trained_row_count < 0 || row.evaluated_row_count < 0 ||
          row.valid_target_count < 0 || row.valid_target_denominator < 0) {
        append_certificate_issue(check, row_prefix + " negative count");
      }
      if (row.routed_row_count > 0 &&
          row.active_row_count > row.routed_row_count) {
        append_certificate_issue(check, row_prefix +
                                            " active rows exceed routed rows");
      }
      if (std::isfinite(row.valid_target_fraction) &&
          !certificate_fraction_in_unit_interval(row.valid_target_fraction)) {
        append_certificate_issue(check,
                                 row_prefix + " valid fraction out of range");
      }
      if (row.trained_row_count > row.active_row_count) {
        append_certificate_issue(check, row_prefix +
                                            " trained rows exceed active rows");
      }
      if (row.evaluated_row_count > row.active_row_count) {
        append_certificate_issue(
            check, row_prefix + " evaluated rows exceed active rows");
      }
      const auto expected_denominator =
          exposure::node_support_valid_target_denominator(
              row.trained_row_count, row.evaluated_row_count,
              row.active_row_count, row.routed_row_count,
              row.valid_target_count, row.valid_target_fraction);
      const bool zero_success_opportunity_claim =
          row.valid_target_count == 0 && row.valid_target_denominator > 0 &&
          std::isfinite(row.valid_target_fraction) &&
          doubles_close(row.valid_target_fraction, 0.0);
      if (!zero_success_opportunity_claim &&
          row.valid_target_denominator != expected_denominator) {
        append_certificate_issue(check,
                                 row_prefix + " valid denominator mismatch");
      }
      if (row.valid_target_denominator > 0 &&
          std::isfinite(row.valid_target_fraction)) {
        const auto expected_fraction =
            static_cast<double>(row.valid_target_count) /
            static_cast<double>(row.valid_target_denominator);
        if (!doubles_close(row.valid_target_fraction, expected_fraction)) {
          append_certificate_issue(check,
                                   row_prefix + " valid fraction mismatch");
        }
      }
      row_routed_total += row.routed_row_count;
      row_active_total += row.active_row_count;
      row_trained_total += row.trained_row_count;
      row_evaluated_total += row.evaluated_row_count;
      row_valid_target_total += row.valid_target_count;
      row_valid_target_opportunity_total += row.valid_target_denominator;
      row_valid_counts.push_back(static_cast<double>(row.valid_target_count));
      if (std::isfinite(row.valid_target_fraction)) {
        if (row_finite_fraction_count == 0) {
          row_min_fraction = row.valid_target_fraction;
          row_max_fraction = row.valid_target_fraction;
        } else {
          row_min_fraction =
              std::min(row_min_fraction, row.valid_target_fraction);
          row_max_fraction =
              std::max(row_max_fraction, row.valid_target_fraction);
        }
        row_fraction_sum += row.valid_target_fraction;
        ++row_finite_fraction_count;
      }
      const bool weaker_count =
          !have_row_weakest ||
          row.valid_target_count < row_weakest.valid_target_count;
      const bool tied_count_lower_fraction =
          have_row_weakest &&
          row.valid_target_count == row_weakest.valid_target_count &&
          std::isfinite(row.valid_target_fraction) &&
          (!std::isfinite(row_weakest.valid_target_fraction) ||
           row.valid_target_fraction < row_weakest.valid_target_fraction);
      if (weaker_count || tied_count_lower_fraction) {
        have_row_weakest = true;
        row_weakest = row;
      }
    }
    if (!summary.support_rows.empty()) {
      if (static_cast<std::int64_t>(row_unique_nodes.size()) !=
          summary.unique_node_count) {
        append_certificate_issue(check, prefix + " unique node count mismatch");
      }
      if (row_observed_input_support_row_count !=
              summary.observed_input_support_row_count ||
          row_target_supervision_support_row_count !=
              summary.target_supervision_support_row_count ||
          row_evaluation_metric_support_row_count !=
              summary.evaluation_metric_support_row_count ||
          row_selection_signal_support_row_count !=
              summary.selection_signal_support_row_count ||
          row_mutating_support_row_count !=
              summary.mutating_support_row_count ||
          row_non_mutating_support_row_count !=
              summary.non_mutating_support_row_count) {
        append_certificate_issue(check,
                                 prefix + " support row use count mismatch");
      }
      if (summary.use.observed_input !=
              (summary.observed_input_support_row_count > 0) ||
          summary.use.target_supervision !=
              (summary.target_supervision_support_row_count > 0) ||
          summary.use.evaluation_metric !=
              (summary.evaluation_metric_support_row_count > 0) ||
          summary.use.selection_signal !=
              (summary.selection_signal_support_row_count > 0) ||
          summary.use.mutated_component !=
              (summary.mutating_support_row_count > 0)) {
        append_certificate_issue(check, prefix + " support use flag mismatch");
      }
      if (row_routed_total != summary.routed_row_count_total ||
          row_active_total != summary.active_row_count_total ||
          row_trained_total != summary.trained_row_count_total ||
          row_evaluated_total != summary.evaluated_row_count_total ||
          row_valid_target_total != summary.valid_target_count_total ||
          row_valid_target_opportunity_total !=
              summary.valid_target_opportunity_count_total) {
        append_certificate_issue(check,
                                 prefix + " support row totals mismatch");
      }
      if (row_finite_fraction_count !=
          summary.finite_valid_target_fraction_count) {
        append_certificate_issue(check,
                                 prefix + " finite fraction count mismatch");
      }
      if (row_finite_fraction_count > 0) {
        const auto row_mean_fraction =
            row_fraction_sum / static_cast<double>(row_finite_fraction_count);
        if (!doubles_close(summary.min_valid_target_fraction,
                           row_min_fraction) ||
            !doubles_close(summary.max_valid_target_fraction,
                           row_max_fraction) ||
            !doubles_close(summary.mean_valid_target_fraction,
                           row_mean_fraction)) {
          append_certificate_issue(
              check, prefix + " finite fraction summary mismatch");
        }
      }
      if (have_row_weakest) {
        const auto expected_weakest_fraction =
            row_weakest.valid_target_denominator > 0
                ? static_cast<double>(row_weakest.valid_target_count) /
                      static_cast<double>(row_weakest.valid_target_denominator)
                : row_weakest.valid_target_fraction;
        if (summary.weakest_valid_target_node_id != row_weakest.node_id ||
            summary.weakest_valid_target_node_index != row_weakest.node_index ||
            summary.weakest_valid_target_count !=
                row_weakest.valid_target_count ||
            summary.weakest_valid_target_denominator !=
                row_weakest.valid_target_denominator ||
            !doubles_close(summary.weakest_valid_target_fraction,
                           expected_weakest_fraction)) {
          append_certificate_issue(check,
                                   prefix + " weakest support row mismatch");
        }
      }
      const auto row_node_count =
          static_cast<double>(summary.support_rows.size());
      const auto row_valid_target_sum = std::accumulate(
          row_valid_counts.begin(), row_valid_counts.end(), 0.0);
      const auto expected_count_mean = row_valid_target_sum / row_node_count;
      if (!doubles_close(summary.valid_target_count_mean,
                         expected_count_mean)) {
        append_certificate_issue(check,
                                 prefix + " support row count mean mismatch");
      }
      if (expected_count_mean > 0.0) {
        double variance = 0.0;
        for (const double value : row_valid_counts) {
          const double delta = value - expected_count_mean;
          variance += delta * delta;
        }
        const auto expected_cv =
            std::sqrt(variance / row_node_count) / expected_count_mean;
        if (!doubles_close(summary.valid_target_count_coefficient_of_variation,
                           expected_cv)) {
          append_certificate_issue(
              check, prefix + " support row coefficient of variation mismatch");
        }
        std::sort(row_valid_counts.begin(), row_valid_counts.end());
        double weighted_sum = 0.0;
        for (std::size_t row_i = 0; row_i < row_valid_counts.size(); ++row_i) {
          weighted_sum +=
              static_cast<double>(row_i + 1) * row_valid_counts[row_i];
        }
        const auto expected_gini =
            (2.0 * weighted_sum) / (row_node_count * row_valid_target_sum) -
            (row_node_count + 1.0) / row_node_count;
        if (!doubles_close(summary.valid_target_count_gini, expected_gini)) {
          append_certificate_issue(check,
                                   prefix + " support row gini mismatch");
        }
        const auto expected_entropy = [&]() {
          if (row_valid_counts.size() <= 1) {
            return 0.0;
          }
          double entropy = 0.0;
          for (const double value : row_valid_counts) {
            if (value <= 0.0) {
              continue;
            }
            const double probability = value / row_valid_target_sum;
            entropy -= probability * std::log(probability);
          }
          return entropy / std::log(row_node_count);
        }();
        if (!doubles_close(summary.valid_target_count_normalized_entropy,
                           expected_entropy)) {
          append_certificate_issue(check,
                                   prefix + " support row entropy mismatch");
        }
      } else {
        if (!doubles_close(summary.valid_target_count_coefficient_of_variation,
                           0.0) ||
            !doubles_close(summary.valid_target_count_gini, 0.0) ||
            !doubles_close(summary.valid_target_count_normalized_entropy,
                           0.0)) {
          append_certificate_issue(check,
                                   prefix + " support row balance mismatch");
        }
      }
    }
    if (summary.node_count < 0 || summary.unique_node_count < 0 ||
        summary.observed_input_support_row_count < 0 ||
        summary.target_supervision_support_row_count < 0 ||
        summary.evaluation_metric_support_row_count < 0 ||
        summary.selection_signal_support_row_count < 0 ||
        summary.mutating_support_row_count < 0 ||
        summary.non_mutating_support_row_count < 0 ||
        summary.routed_row_count_total < 0 ||
        summary.active_row_count_total < 0 ||
        summary.trained_row_count_total < 0 ||
        summary.evaluated_row_count_total < 0 ||
        summary.valid_target_count_total < 0 ||
        summary.valid_target_opportunity_count_total < 0) {
      append_certificate_issue(check, prefix + " negative count");
    }
    if (summary.node_count == 0) {
      append_certificate_issue(check, prefix + " empty node-support summary");
    }
    if (summary.unique_node_count == 0 ||
        summary.unique_node_count > summary.node_count) {
      append_certificate_issue(check,
                               prefix + " unique node count out of range");
    }
    if (summary.observed_input_support_row_count > summary.node_count ||
        summary.target_supervision_support_row_count > summary.node_count ||
        summary.evaluation_metric_support_row_count > summary.node_count ||
        summary.selection_signal_support_row_count > summary.node_count) {
      append_certificate_issue(check,
                               prefix + " exposure-use count out of range");
    }
    if (summary.mutating_support_row_count +
            summary.non_mutating_support_row_count !=
        summary.node_count) {
      append_certificate_issue(check, prefix + " mutation-use count mismatch");
    }
    if (summary.target_component_family_id !=
        "wikimyei.inference.expected_value.mdn") {
      append_certificate_issue(check, prefix + " non-MDN target component");
    }
    if (proof.closure.checked && summary.use.mutated_component) {
      const auto causal_mdn_exposure = std::find_if(
          proof.closure.causal_exposures.begin(),
          proof.closure.causal_exposures.end(),
          [&](const lattice_target_proof_certificate_t::closure_proof_t::
                  causal_exposure_t &causal) {
            if (causal.target_component_family_id !=
                summary.target_component_family_id) {
              return false;
            }
            if (!certificate_causal_exposure_has_use_set(causal, summary.use)) {
              return false;
            }
            if (!summary.split_name.empty() &&
                causal.split_name != summary.split_name) {
              return false;
            }
            if (proof_context.require_component_match &&
                causal.component_assembly_fingerprint !=
                    proof_context.expected_component_fingerprint) {
              return false;
            }
            return true;
          });
      if (causal_mdn_exposure == proof.closure.causal_exposures.end()) {
        append_certificate_issue(check,
                                 prefix + " missing causal MDN exposure");
      }
    }
    if (summary.node_count > 0) {
      if (summary.target_component_family_id.empty()) {
        append_certificate_issue(check, prefix + " missing target component");
      }
      if (!summary.use.observed_input && !summary.use.target_supervision &&
          !summary.use.evaluation_metric && !summary.use.selection_signal) {
        append_certificate_issue(check, prefix + " missing exposure use");
      }
      if (summary.routed_row_count_total > 0 &&
          summary.active_row_count_total > summary.routed_row_count_total) {
        append_certificate_issue(check,
                                 prefix + " active rows exceed routed rows");
      }
      if (summary.trained_row_count_total > summary.active_row_count_total) {
        append_certificate_issue(check,
                                 prefix + " trained rows exceed active rows");
      }
      if (summary.evaluated_row_count_total > summary.active_row_count_total) {
        append_certificate_issue(check,
                                 prefix + " evaluated rows exceed active rows");
      }
    }
    if (summary.finite_valid_target_fraction_count < 0 ||
        summary.finite_valid_target_fraction_count > summary.node_count) {
      append_certificate_issue(check,
                               prefix + " finite fraction count out of range");
    }
    if (summary.valid_target_opportunity_count_total > 0) {
      if (summary.valid_target_count_total >
          summary.valid_target_opportunity_count_total) {
        append_certificate_issue(
            check, prefix + " valid-target successes exceed opportunities");
      }
      const auto expected_fraction =
          static_cast<double>(summary.valid_target_count_total) /
          static_cast<double>(summary.valid_target_opportunity_count_total);
      if (!doubles_close(summary.valid_target_fraction_estimate,
                         expected_fraction)) {
        append_certificate_issue(
            check, prefix + " valid-target fraction estimate mismatch");
      }
      if (!certificate_fraction_in_unit_interval(
              summary.valid_target_fraction_estimate)) {
        append_certificate_issue(
            check, prefix + " valid-target fraction estimate out of range");
      }
      const auto [expected_lower, expected_upper] =
          exposure::wilson_score_interval_95(
              summary.valid_target_count_total,
              summary.valid_target_opportunity_count_total);
      if (!doubles_close(summary.valid_target_wilson_lower_95,
                         expected_lower) ||
          !doubles_close(summary.valid_target_wilson_upper_95,
                         expected_upper)) {
        append_certificate_issue(check, prefix +
                                            " valid-target Wilson interval "
                                            "mismatch");
      }
      if (!certificate_fraction_in_unit_interval(
              summary.valid_target_wilson_lower_95) ||
          !certificate_fraction_in_unit_interval(
              summary.valid_target_wilson_upper_95) ||
          summary.valid_target_wilson_lower_95 >
              summary.valid_target_wilson_upper_95) {
        append_certificate_issue(
            check, prefix + " valid-target Wilson interval out of range");
      }
    } else if (std::isfinite(summary.valid_target_fraction_estimate) ||
               std::isfinite(summary.valid_target_wilson_lower_95) ||
               std::isfinite(summary.valid_target_wilson_upper_95)) {
      append_certificate_issue(
          check, prefix + " valid-target uncertainty estimate without trials");
    }
    if (summary.node_count > 0) {
      const auto expected_mean =
          static_cast<double>(summary.valid_target_count_total) /
          static_cast<double>(summary.node_count);
      if (!doubles_close(summary.valid_target_count_mean, expected_mean)) {
        append_certificate_issue(check,
                                 prefix + " valid target count mean mismatch");
      }
      if (summary.weakest_valid_target_node_index < 0 ||
          summary.weakest_valid_target_node_index >= summary.node_count) {
        append_certificate_issue(check,
                                 prefix + " weakest node index out of range");
      }
      if (summary.weakest_valid_target_node_id.empty()) {
        append_certificate_issue(check, prefix + " missing weakest node id");
      }
      if (summary.weakest_valid_target_count < 0 ||
          summary.weakest_valid_target_count >
              summary.valid_target_count_total) {
        append_certificate_issue(check, prefix + " weakest count out of range");
      }
      if (summary.weakest_valid_target_count * summary.node_count >
          summary.valid_target_count_total) {
        append_certificate_issue(check, prefix + " weakest count exceeds mean");
      }
      if (summary.weakest_valid_target_denominator < 0 ||
          summary.weakest_valid_target_denominator <
              summary.weakest_valid_target_count) {
        append_certificate_issue(check,
                                 prefix + " weakest denominator out of range");
      }
      if (summary.weakest_valid_target_denominator > 0) {
        const auto expected_fraction =
            static_cast<double>(summary.weakest_valid_target_count) /
            static_cast<double>(summary.weakest_valid_target_denominator);
        if (!doubles_close(summary.weakest_valid_target_fraction,
                           expected_fraction)) {
          append_certificate_issue(check,
                                   prefix + " weakest valid fraction mismatch");
        }
        const auto [expected_lower, expected_upper] =
            exposure::wilson_score_interval_95(
                summary.weakest_valid_target_count,
                summary.weakest_valid_target_denominator);
        if (!doubles_close(summary.weakest_valid_target_wilson_lower_95,
                           expected_lower) ||
            !doubles_close(summary.weakest_valid_target_wilson_upper_95,
                           expected_upper)) {
          append_certificate_issue(
              check, prefix + " weakest Wilson interval mismatch");
        }
      } else {
        if (std::isfinite(summary.weakest_valid_target_fraction) &&
            !doubles_close(summary.weakest_valid_target_fraction, 0.0)) {
          append_certificate_issue(
              check, prefix + " finite weakest fraction without denominator");
        }
        if (std::isfinite(summary.weakest_valid_target_wilson_lower_95) ||
            std::isfinite(summary.weakest_valid_target_wilson_upper_95)) {
          append_certificate_issue(
              check, prefix + " weakest Wilson interval without denominator");
        }
      }
    }
    if (summary.finite_valid_target_fraction_count > 0) {
      if (!certificate_fraction_in_unit_interval(
              summary.min_valid_target_fraction) ||
          !certificate_fraction_in_unit_interval(
              summary.max_valid_target_fraction) ||
          !certificate_fraction_in_unit_interval(
              summary.mean_valid_target_fraction)) {
        append_certificate_issue(check,
                                 prefix + " valid fraction out of range");
      }
      if (summary.min_valid_target_fraction >
          summary.max_valid_target_fraction) {
        append_certificate_issue(check,
                                 prefix + " valid fraction min/max inverted");
      }
      if (std::isfinite(summary.mean_valid_target_fraction) &&
          (summary.mean_valid_target_fraction + 1e-12 <
               summary.min_valid_target_fraction ||
           summary.mean_valid_target_fraction - 1e-12 >
               summary.max_valid_target_fraction)) {
        append_certificate_issue(check,
                                 prefix + " mean valid fraction out of bounds");
      }
    } else if (std::isfinite(summary.min_valid_target_fraction) ||
               std::isfinite(summary.max_valid_target_fraction) ||
               std::isfinite(summary.mean_valid_target_fraction)) {
      append_certificate_issue(
          check, prefix + " finite fraction summary without finite fractions");
    }
    if (std::isfinite(summary.valid_target_count_gini) &&
        (summary.valid_target_count_gini < -1e-12 ||
         summary.valid_target_count_gini > 1.0 + 1e-12)) {
      append_certificate_issue(check, prefix + " gini out of range");
    }
    if (std::isfinite(summary.valid_target_count_normalized_entropy) &&
        (summary.valid_target_count_normalized_entropy < -1e-12 ||
         summary.valid_target_count_normalized_entropy > 1.0 + 1e-12)) {
      append_certificate_issue(check,
                               prefix + " normalized entropy out of range");
    }
    if (std::isfinite(summary.valid_target_count_coefficient_of_variation) &&
        summary.valid_target_count_coefficient_of_variation < -1e-12) {
      append_certificate_issue(check,
                               prefix + " coefficient of variation negative");
    }
    if (summary.node_count > 0 && summary.valid_target_count_total > 0) {
      if (!std::isfinite(summary.valid_target_count_gini)) {
        append_certificate_issue(check, prefix + " missing finite gini");
      }
      if (!std::isfinite(summary.valid_target_count_coefficient_of_variation)) {
        append_certificate_issue(
            check, prefix + " missing finite coefficient of variation");
      }
      if (!std::isfinite(summary.valid_target_count_normalized_entropy)) {
        append_certificate_issue(check,
                                 prefix + " missing finite normalized entropy");
      }
    }
    if (summary.node_count == 1 || summary.valid_target_count_total == 0) {
      if (std::isfinite(summary.valid_target_count_gini) &&
          !doubles_close(summary.valid_target_count_gini, 0.0)) {
        append_certificate_issue(check, prefix + " degenerate gini mismatch");
      }
      if (std::isfinite(summary.valid_target_count_coefficient_of_variation) &&
          !doubles_close(summary.valid_target_count_coefficient_of_variation,
                         0.0)) {
        append_certificate_issue(
            check, prefix + " degenerate coefficient of variation mismatch");
      }
      if (std::isfinite(summary.valid_target_count_normalized_entropy) &&
          !doubles_close(summary.valid_target_count_normalized_entropy, 0.0)) {
        append_certificate_issue(
            check, prefix + " degenerate normalized entropy mismatch");
      }
    }
  }

  if (certificate_digest_missing && check.passed) {
    append_certificate_issue(check, "missing certificate_digest");
  }
  return check;
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
  out << "target_kind_applicable="
      << (lattice_target_kind_applicable(spec) ? "true" : "false") << "\n";
  out << "kind=" << lattice_target_effective_kind_name(spec) << "\n";
  out << "target_kind_effective="
      << lattice_target_effective_kind_selector(spec) << "\n";
  out << "proof_kind=" << spec.proof_kind << "\n";
  out << "subject_fact_family=" << spec.subject_fact_family << "\n";
  out << "protocol_id=" << spec.protocol_id << "\n";
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
    out << cuwacunu::hero::lattice::exposure::exposure_use_name(
        spec.forbid_exposure_uses[i]);
  }
  out << "\nforbid_exposure_uses_from_split_policy="
      << (spec.forbid_exposure_uses_from_split_policy ? "true" : "false")
      << "\n";
  out << "forbid_exposure_requires_mutated_component="
      << (spec.forbid_exposure_requires_mutated_component ? "true" : "false")
      << "\n";
  for (const auto &requirement : spec.representation_geometry_requirements) {
    out << "representation_geometry_requirement=" << requirement.requirement_id
        << "\n";
    out << "representation_geometry_requirement.metric=" << requirement.metric
        << "\n";
    out << "representation_geometry_requirement.op=" << requirement.op << "\n";
    out << "representation_geometry_requirement.value=" << requirement.value
        << "\n";
    out << "representation_geometry_requirement.require_mutated_component="
        << (requirement.require_mutated_component ? "true" : "false") << "\n";
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
  return out.str();
}

[[nodiscard]] inline std::string
lattice_target_spec_fingerprint(const lattice_target_compiled_t &target) {
  return cuwacunu::hero::lattice::exposure::exposure_digest_for_text(
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
  std::set<std::string> policy_gate_ids;
  std::vector<lattice_policy_gate_spec_t> policy_gates;
  std::vector<lattice_target_compiled_t> out;
  const auto blocks = kv::parse_blocks(dsl_text);
  const auto append_target_clause =
      [](std::unordered_map<std::string, std::vector<kv::block_t>> &clauses,
         const kv::block_t &block) {
        clauses[kv::required(block, "TARGET_ID")].push_back(block);
      };
  for (const auto &block : blocks) {
    if (block.name == "LATTICE_POLICY_GATE") {
      const auto gate = decode_lattice_policy_gate_spec_block(block);
      if (!policy_gate_ids.insert(gate.policy_id).second) {
        throw std::runtime_error("[lattice_target] duplicate POLICY_ID: " +
                                 gate.policy_id);
      }
      policy_gates.push_back(gate);
      continue;
    }
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
    compiled.lowered_v0.target_spec_fingerprint =
        compiled.target_spec_fingerprint;
    out.push_back(std::move(compiled));
  }
  std::set<std::string> target_ids;
  for (const auto &target : out) {
    target_ids.insert(target.lowered_v0.target_id);
  }
  std::unordered_map<std::string, const lattice_target_spec_t *> target_by_id;
  target_by_id.reserve(out.size());
  for (const auto &target : out) {
    target_by_id.emplace(target.lowered_v0.target_id, &target.lowered_v0);
  }
  for (const auto &gate : policy_gates) {
    const auto target_it = target_by_id.find(gate.target_id);
    if (target_it == target_by_id.end()) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_POLICY_GATE " + gate.policy_id +
          " references unknown TARGET_ID: " + gate.target_id);
    }
    const auto &target_spec = *target_it->second;
    if (!is_artifact_readiness_target_class(target_spec.target_class)) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_POLICY_GATE " + gate.policy_id +
          " must reference TARGET_CLASS=artifact_readiness target " +
          gate.target_id +
          "; reserved policy gates consume artifact proof certificates, not "
          "runtime readiness targets");
    }
    const auto expected_subject =
        expected_subject_fact_family_for_policy_gate_kind(gate.policy_kind);
    if (!expected_subject.empty() &&
        target_spec.subject_fact_family != expected_subject) {
      throw std::runtime_error(
          "[lattice_target] LATTICE_POLICY_GATE " + gate.policy_id +
          " POLICY_KIND=" + gate.policy_kind +
          " must reference SUBJECT_FACT_FAMILY=" + expected_subject +
          " target " + gate.target_id);
    }
  }
  const auto check_latest_satisfying_reference =
      [&target_by_id](const lattice_target_spec_t &owner,
                      const std::string &field_name,
                      const std::string &source) {
        const auto source_target_id =
            latest_satisfying_reference_target_id(source);
        if (source_target_id.empty()) {
          return;
        }
        const auto source_it = target_by_id.find(source_target_id);
        if (source_it == target_by_id.end()) {
          return;
        }
        const auto &source_spec = *source_it->second;
        if (is_latest_satisfying_checkpoint_selectable_target_class(
                source_spec.target_class)) {
          return;
        }
        throw std::runtime_error(
            "[lattice_target] " + owner.target_id + " " + field_name +
            " latest_satisfying cannot reference TARGET_CLASS=" +
            source_spec.target_class + " target " + source_spec.target_id +
            "; latest_satisfying selects checkpoint-producing "
            "readiness/leakage_guard targets");
      };
  for (const auto &target : out) {
    const auto &spec = target.lowered_v0;
    check_latest_satisfying_reference(spec, "CHECKPOINT_SOURCE",
                                      spec.checkpoint_source);
    check_latest_satisfying_reference(spec, "EVALUATED_CHECKPOINT_SOURCE",
                                      spec.evaluated_checkpoint_source);
    check_latest_satisfying_reference(spec, "PLAN_INPUT_MDN_CHECKPOINT",
                                      spec.plan_input_mdn_checkpoint);
    check_latest_satisfying_reference(
        spec, "PLAN_INPUT_REPRESENTATION_CHECKPOINT",
        spec.plan_input_representation_checkpoint);
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
  out << lattice_target_effective_kind_selector(spec) << "|";
  out << spec.proof_kind << "|";
  out << spec.subject_fact_family << "|";
  out << spec.protocol_id << "|";
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

} // namespace cuwacunu::hero::lattice::target
