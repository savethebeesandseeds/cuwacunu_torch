// SPDX-License-Identifier: MIT
#pragma once

#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "hero/marshal_hero/marshal/operational_report.h"

namespace cuwacunu::hero::marshal {

inline constexpr const char *k_marshal_run_compare_schema_v1 =
    "kikijyeba.marshal.run_compare.v1";

struct marshal_run_compare_options_t {
  std::filesystem::path runtime_root{"/cuwacunu/.runtime/cuwacunu_exec"};
  std::filesystem::path config_path{"/cuwacunu/src/config/.config"};
  std::string baseline_job_id{};
  std::string candidate_job_id{};
  bool include_machine_payload{false};
};

namespace run_compare_detail {

using operational_report_detail::get;
using operational_report_detail::job_summary_t;

[[nodiscard]] inline std::string
first_field(const job_summary_t &job,
            std::initializer_list<std::string_view> keys) {
  for (const auto key : keys) {
    const std::string key_text(key);
    for (const auto *source :
         {&job.result_fact, &job.health_fact, &job.checkpoint_io_fact,
          &job.report, &job.state, &job.manifest}) {
      const auto value = get(*source, key_text);
      if (!value.empty()) {
        return value;
      }
    }
  }
  return {};
}

[[nodiscard]] inline bool parse_finite_double(const std::string &value,
                                              double *out) {
  if (!operational_report_detail::json_number_is_valid(value)) {
    return false;
  }
  char *end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0') {
    return false;
  }
  if (out != nullptr) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] inline const job_summary_t *
find_job(const std::vector<job_summary_t> &jobs,
         const std::string &job_id_or_path) {
  for (const auto &job : jobs) {
    if (get(job.state, "job_id") == job_id_or_path ||
        job.job_dir.string() == job_id_or_path) {
      return &job;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::string
string_delta_json(const std::string &key, const job_summary_t &baseline,
                  const job_summary_t &candidate,
                  std::initializer_list<std::string_view> aliases) {
  const auto baseline_value = first_field(baseline, aliases);
  const auto candidate_value = first_field(candidate, aliases);
  std::ostringstream out;
  out << detail::json_quote(key)
      << ":{\"baseline\":" << detail::json_quote(baseline_value)
      << ",\"candidate\":" << detail::json_quote(candidate_value)
      << ",\"changed\":"
      << (baseline_value != candidate_value ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] inline std::string
scalar_delta_json(const std::string &key, const job_summary_t &baseline,
                  const job_summary_t &candidate,
                  std::initializer_list<std::string_view> aliases) {
  const auto baseline_value = first_field(baseline, aliases);
  const auto candidate_value = first_field(candidate, aliases);
  double baseline_number = 0.0;
  double candidate_number = 0.0;
  const bool baseline_ok =
      parse_finite_double(baseline_value, &baseline_number);
  const bool candidate_ok =
      parse_finite_double(candidate_value, &candidate_number);
  std::ostringstream out;
  out << detail::json_quote(key) << ":{\"baseline\":"
      << operational_report_detail::json_number_or_null(baseline_value)
      << ",\"candidate\":"
      << operational_report_detail::json_number_or_null(candidate_value)
      << ",\"delta\":";
  if (baseline_ok && candidate_ok) {
    out << (candidate_number - baseline_number);
  } else {
    out << "null";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
config_identity_deltas_json(const job_summary_t &baseline,
                            const job_summary_t &candidate) {
  const std::vector<std::string> rows = {
      string_delta_json("config_path", baseline, candidate, {"config_path"}),
      string_delta_json("config_bundle_id", baseline, candidate,
                        {"config_bundle_id"}),
      string_delta_json("config_receipt_id", baseline, candidate,
                        {"config_receipt_id"}),
      string_delta_json("protocol_contract_fingerprint", baseline, candidate,
                        {"protocol_contract_fingerprint"}),
      string_delta_json("graph_order_fingerprint", baseline, candidate,
                        {"graph_order_fingerprint"}),
      string_delta_json("source_cursor_token", baseline, candidate,
                        {"source_cursor_token"})};
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << rows[i];
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
wave_range_deltas_json(const job_summary_t &baseline,
                       const job_summary_t &candidate) {
  const std::vector<std::string> rows = {
      string_delta_json("wave_id", baseline, candidate, {"wave_id"}),
      string_delta_json("wave_action", baseline, candidate, {"wave_action"}),
      string_delta_json("wave_mode", baseline, candidate, {"wave_mode"}),
      string_delta_json(
          "source_range_policy", baseline, candidate,
          {"source_range_policy", "wave_source_range_policy", "source_range"}),
      string_delta_json(
          "source_order_policy", baseline, candidate,
          {"source_order_policy", "wave_source_order_policy", "source_order"}),
      scalar_delta_json("anchor_index_begin", baseline, candidate,
                        {"resolved_anchor_index_begin"}),
      scalar_delta_json("anchor_index_end", baseline, candidate,
                        {"resolved_anchor_index_end"}),
      scalar_delta_json("accepted_anchor_count", baseline, candidate,
                        {"accepted_anchor_count"})};
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << rows[i];
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
metric_deltas_json(const job_summary_t &baseline,
                   const job_summary_t &candidate) {
  const std::vector<std::string> rows = {
      scalar_delta_json("optimizer_steps", baseline, candidate,
                        {"optimizer_steps"}),
      scalar_delta_json("final_loss", baseline, candidate,
                        {"final_loss", "last_loss"}),
      scalar_delta_json("mean_loss", baseline, candidate, {"mean_loss"}),
      scalar_delta_json("valid_target_fraction", baseline, candidate,
                        {"valid_target_fraction", "mean_valid_target_fraction",
                         "last_valid_target_fraction"}),
      scalar_delta_json("total_valid_target_count", baseline, candidate,
                        {"total_valid_target_count"}),
      scalar_delta_json("sigma_mean", baseline, candidate,
                        {"sigma_mean", "mean_sigma_mean"}),
      scalar_delta_json("sigma_mean_valid", baseline, candidate,
                        {"sigma_mean_valid", "mean_sigma_mean_valid"}),
      scalar_delta_json("mixture_entropy", baseline, candidate,
                        {"mixture_entropy", "mean_mixture_entropy"}),
      scalar_delta_json("nonfinite_output_count", baseline, candidate,
                        {"nonfinite_output_count"}),
      scalar_delta_json("grad_norm_max_pre_clip", baseline, candidate,
                        {"grad_norm_max_pre_clip", "max_grad_norm"}),
      scalar_delta_json("representation_effective_rank_fraction", baseline,
                        candidate, {"representation_effective_rank_fraction"})};
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << rows[i];
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
array_field_json(const job_summary_t &job,
                 std::initializer_list<std::string_view> aliases) {
  const auto value = first_field(job, aliases);
  if (value.empty()) {
    return "[]";
  }
  return operational_report_detail::json_number_array_or_string_array(value);
}

[[nodiscard]] inline std::string sigma_health_json(const job_summary_t &job) {
  std::ostringstream out;
  out << "{\"sigma_min\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_min", "min_sigma_min"}))
      << ",\"sigma_mean\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_mean", "mean_sigma_mean"}))
      << ",\"sigma_max\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_max", "max_sigma_max"}))
      << ",\"sigma_min_valid\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_min_valid", "min_sigma_min_valid"}))
      << ",\"sigma_mean_valid\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_mean_valid", "mean_sigma_mean_valid"}))
      << ",\"sigma_max_valid\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"sigma_max_valid", "max_sigma_max_valid"}))
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
checkpoint_lineage_json(const job_summary_t &job) {
  const auto produced = first_field(job, {"checkpoint_path"});
  const auto representation =
      first_field(job, {"representation_checkpoint_path"});
  const auto mdn = first_field(job, {"mdn_checkpoint_path"});
  std::ostringstream out;
  out << "{\"produced_checkpoint_path\":" << detail::json_quote(produced)
      << ",\"representation_checkpoint_path\":"
      << detail::json_quote(representation)
      << ",\"representation_checkpoint_loaded\":"
      << operational_report_detail::json_bool_or_null(
             first_field(job, {"representation_checkpoint_loaded"}))
      << ",\"mdn_checkpoint_path\":" << detail::json_quote(mdn)
      << ",\"mdn_checkpoint_loaded\":"
      << operational_report_detail::json_bool_or_null(
             first_field(job, {"mdn_checkpoint_loaded"}))
      << ",\"checkpoint_written\":"
      << operational_report_detail::json_bool_or_null(
             first_field(job, {"checkpoint_written"}))
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
train_eval_split_binding_json(const job_summary_t &job) {
  std::ostringstream out;
  out << "{\"wave_action\":"
      << detail::json_quote(first_field(job, {"wave_action"}))
      << ",\"source_range\":\"anchor_index\""
      << ",\"anchor_index_begin\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"resolved_anchor_index_begin"}))
      << ",\"anchor_index_end\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"resolved_anchor_index_end"}))
      << ",\"optimizer_steps\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"optimizer_steps"}))
      << ",\"checkpoint_written\":"
      << operational_report_detail::json_bool_or_null(
             first_field(job, {"checkpoint_written"}))
      << ",\"model_state_mutated\":"
      << (operational_report_detail::job_mutated_model_state(job) ? "true"
                                                                  : "false")
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
performance_panel_json(const job_summary_t &job) {
  std::ostringstream out;
  out << "{\"loss\":{\"final_loss\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"final_loss", "last_loss"}))
      << ",\"mean_loss\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"mean_loss"}))
      << "},\"valid_support\":{\"valid_target_fraction\":"
      << operational_report_detail::json_number_or_null(first_field(
             job, {"valid_target_fraction", "mean_valid_target_fraction",
                   "last_valid_target_fraction"}))
      << ",\"last_valid_target_count\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"last_valid_target_count"}))
      << ",\"total_valid_target_count\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"total_valid_target_count"}))
      << ",\"by_channel\":"
      << array_field_json(job, {"valid_target_count_per_channel"})
      << ",\"by_target_feature\":"
      << array_field_json(job, {"valid_target_count_per_target_feature"})
      << ",\"by_channel_target_feature\":"
      << array_field_json(job,
                          {"valid_target_count_per_channel_target_feature"})
      << "},\"nll\":{\"per_channel\":"
      << array_field_json(job, {"mean_nll_per_channel"})
      << ",\"per_target_feature\":"
      << array_field_json(job, {"mean_nll_per_target_feature"})
      << ",\"per_channel_target_feature\":"
      << array_field_json(job, {"mean_nll_per_channel_target_feature"})
      << "},\"sigma_health\":" << sigma_health_json(job)
      << ",\"mixture\":{\"entropy\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"mixture_entropy", "mean_mixture_entropy"}))
      << ",\"usage\":"
      << array_field_json(job, {"mixture_usage", "mean_mixture_usage"})
      << "},\"runtime_health\":{\"nonfinite_output_count\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"nonfinite_output_count"}))
      << ",\"finite_parameter_check\":"
      << operational_report_detail::json_bool_or_null(
             first_field(job, {"finite_parameter_check"}))
      << ",\"grad_norm_max_pre_clip\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"grad_norm_max_pre_clip", "max_grad_norm"}))
      << ",\"grad_clip_norm\":"
      << operational_report_detail::json_number_or_null(
             first_field(job, {"grad_clip_norm"}))
      << "},\"checkpoint_lineage\":" << checkpoint_lineage_json(job)
      << ",\"train_eval_split_binding\":" << train_eval_split_binding_json(job)
      << "}";
  return out.str();
}

[[nodiscard]] inline std::vector<std::string>
health_warning_ids(const job_summary_t &job) {
  std::vector<std::string> out;
  const auto nonfinite = first_field(job, {"nonfinite_output_count"});
  double nonfinite_count = 0.0;
  if (parse_finite_double(nonfinite, &nonfinite_count) &&
      nonfinite_count > 0.0) {
    out.push_back("nonfinite_output_count_above_zero");
  }
  const auto finite = first_field(job, {"finite_parameter_check"});
  if (!finite.empty() && !operational_report_detail::bool_value(finite)) {
    out.push_back("finite_parameter_check_failed");
  }
  double grad = 0.0;
  double clip = 0.0;
  if (parse_finite_double(
          first_field(job, {"grad_norm_max_pre_clip", "max_grad_norm"}),
          &grad) &&
      parse_finite_double(first_field(job, {"grad_clip_norm"}), &clip) &&
      clip > 0.0 && grad > clip * 100.0) {
    out.push_back("high_pre_clip_grad_norm");
  }
  return out;
}

[[nodiscard]] inline std::string
warning_delta_json(const job_summary_t &baseline,
                   const job_summary_t &candidate) {
  const auto baseline_ids = health_warning_ids(baseline);
  const auto candidate_ids = health_warning_ids(candidate);
  const std::set<std::string> baseline_set(baseline_ids.begin(),
                                           baseline_ids.end());
  const std::set<std::string> candidate_set(candidate_ids.begin(),
                                            candidate_ids.end());
  std::vector<std::string> added;
  std::vector<std::string> cleared;
  std::vector<std::string> unchanged;
  for (const auto &id : candidate_set) {
    if (baseline_set.find(id) == baseline_set.end()) {
      added.push_back(id);
    } else {
      unchanged.push_back(id);
    }
  }
  for (const auto &id : baseline_set) {
    if (candidate_set.find(id) == candidate_set.end()) {
      cleared.push_back(id);
    }
  }
  return "{\"added\":" + operational_report_detail::json_string_array(added) +
         ",\"cleared\":" +
         operational_report_detail::json_string_array(cleared) +
         ",\"unchanged\":" +
         operational_report_detail::json_string_array(unchanged) + "}";
}

inline void compare_field(const job_summary_t &baseline,
                          const job_summary_t &candidate,
                          const std::string &field, const std::string &reason,
                          std::vector<std::string> *reasons,
                          std::vector<std::string> *blockers) {
  const auto baseline_value = first_field(baseline, {field});
  const auto candidate_value = first_field(candidate, {field});
  if (!baseline_value.empty() && baseline_value == candidate_value) {
    reasons->push_back(reason);
    return;
  }
  blockers->push_back(field + "_differs");
}

[[nodiscard]] inline std::string
comparability_json(const job_summary_t &baseline,
                   const job_summary_t &candidate) {
  std::vector<std::string> reasons;
  std::vector<std::string> blockers;
  compare_field(baseline, candidate, "job_kind", "same component family",
                &reasons, &blockers);
  compare_field(baseline, candidate, "wave_action", "same wave action",
                &reasons, &blockers);
  compare_field(baseline, candidate, "target_component_family_id",
                "same target component", &reasons, &blockers);
  compare_field(baseline, candidate, "resolved_anchor_index_begin",
                "same anchor begin", &reasons, &blockers);
  compare_field(baseline, candidate, "resolved_anchor_index_end",
                "same anchor end", &reasons, &blockers);
  const bool same_rep_role =
      first_field(baseline, {"representation_checkpoint_path"}).empty() ==
      first_field(candidate, {"representation_checkpoint_path"}).empty();
  const bool same_mdn_role =
      first_field(baseline, {"mdn_checkpoint_path"}).empty() ==
      first_field(candidate, {"mdn_checkpoint_path"}).empty();
  if (same_rep_role && same_mdn_role) {
    reasons.push_back("same checkpoint input roles");
  } else {
    blockers.push_back("checkpoint_input_roles_differ");
  }
  std::ostringstream out;
  out << "{\"comparable\":" << (blockers.empty() ? "true" : "false")
      << ",\"reasons\":"
      << operational_report_detail::json_string_array(reasons)
      << ",\"blockers\":"
      << operational_report_detail::json_string_array(blockers) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
checkpoint_delta_summary_json(const job_summary_t &baseline,
                              const job_summary_t &candidate) {
  const auto baseline_produced = first_field(baseline, {"checkpoint_path"});
  const auto candidate_produced = first_field(candidate, {"checkpoint_path"});
  const auto baseline_rep =
      first_field(baseline, {"representation_checkpoint_path"});
  const auto candidate_rep =
      first_field(candidate, {"representation_checkpoint_path"});
  const auto baseline_mdn = first_field(baseline, {"mdn_checkpoint_path"});
  const auto candidate_mdn = first_field(candidate, {"mdn_checkpoint_path"});
  std::ostringstream out;
  out << "{\"produced_checkpoint_changed\":"
      << (baseline_produced != candidate_produced ? "true" : "false")
      << ",\"representation_checkpoint_changed\":"
      << (baseline_rep != candidate_rep ? "true" : "false")
      << ",\"mdn_checkpoint_changed\":"
      << (baseline_mdn != candidate_mdn ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] inline std::string
checkpoint_delta_detail_json(const job_summary_t &baseline,
                             const job_summary_t &candidate) {
  std::ostringstream out;
  out << "{\"summary\":" << checkpoint_delta_summary_json(baseline, candidate)
      << ",\"baseline\":" << checkpoint_lineage_json(baseline)
      << ",\"candidate\":" << checkpoint_lineage_json(candidate) << "}";
  return out.str();
}

[[nodiscard]] inline std::string run_pair_json(const job_summary_t &baseline,
                                               const job_summary_t &candidate) {
  std::ostringstream out;
  out << "{\"baseline\":"
      << operational_report_detail::compact_job_json(baseline)
      << ",\"candidate\":"
      << operational_report_detail::compact_job_json(candidate) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
operator_summary_json(const job_summary_t &baseline,
                      const job_summary_t &candidate) {
  const bool same_component =
      first_field(baseline, {"target_component_family_id"}) ==
      first_field(candidate, {"target_component_family_id"});
  const bool same_action = first_field(baseline, {"wave_action"}) ==
                           first_field(candidate, {"wave_action"});
  std::ostringstream out;
  out << "{\"headline\":\"Run comparison ready.\""
      << ",\"baseline_job_id\":"
      << detail::json_quote(get(baseline.state, "job_id"))
      << ",\"candidate_job_id\":"
      << detail::json_quote(get(candidate.state, "job_id"))
      << ",\"same_component\":" << (same_component ? "true" : "false")
      << ",\"same_wave_action\":" << (same_action ? "true" : "false")
      << ",\"next_safe_action\":\"inspect_metric_deltas\""
      << ",\"descriptive_only\":true}";
  return out.str();
}

[[nodiscard]] inline std::string stop_reason_json() {
  return "{\"terminal_state\":\"report_ready\""
         ",\"human_reason\":\"Read-only run comparison generated.\""
         ",\"blocking_owner\":\"none\""
         ",\"next_safe_action\":\"inspect_metric_deltas\"}";
}

[[nodiscard]] inline std::string
runtime_panel_json(const job_summary_t &baseline,
                   const job_summary_t &candidate) {
  std::ostringstream out;
  out << "{\"baseline\":{\"job_id\":"
      << detail::json_quote(get(baseline.state, "job_id"))
      << ",\"status\":" << detail::json_quote(get(baseline.state, "status"))
      << ",\"component\":"
      << detail::json_quote(
             first_field(baseline, {"target_component_family_id"}))
      << ",\"wave_action\":"
      << detail::json_quote(first_field(baseline, {"wave_action"}))
      << ",\"model_state_mutated\":"
      << (operational_report_detail::job_mutated_model_state(baseline)
              ? "true"
              : "false")
      << "},\"candidate\":{\"job_id\":"
      << detail::json_quote(get(candidate.state, "job_id"))
      << ",\"status\":" << detail::json_quote(get(candidate.state, "status"))
      << ",\"component\":"
      << detail::json_quote(
             first_field(candidate, {"target_component_family_id"}))
      << ",\"wave_action\":"
      << detail::json_quote(first_field(candidate, {"wave_action"}))
      << ",\"model_state_mutated\":"
      << (operational_report_detail::job_mutated_model_state(candidate)
              ? "true"
              : "false")
      << "},\"comparability\":" << comparability_json(baseline, candidate)
      << ",\"checkpoint_deltas\":"
      << checkpoint_delta_summary_json(baseline, candidate) << "}";
  return out.str();
}

[[nodiscard]] inline std::string lattice_panel_json() {
  return "{\"source\":\"not_evaluated_by_compare\""
         ",\"target_satisfaction_claimed\":false"
         ",\"requires_lattice_replay\":true"
         ",\"proof_status\":\"not_claimed\"}";
}

[[nodiscard]] inline std::string
audit_panel_json(const marshal_run_compare_options_t &options,
                 const std::string &comparison_id,
                 const std::string &generated_at) {
  std::ostringstream out;
  out << "{\"comparison_id\":" << detail::json_quote(comparison_id)
      << ",\"generated_at\":" << detail::json_quote(generated_at)
      << ",\"runtime_root\":"
      << detail::json_quote(options.runtime_root.string())
      << ",\"config_path\":" << detail::json_quote(options.config_path.string())
      << ",\"full_payload_available\":true"
      << ",\"machine_payload_included\":"
      << (options.include_machine_payload ? "true" : "false")
      << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"target_satisfaction_claimed\":false"
      << ",\"model_selector\":false"
      << ",\"checkpoint_selected\":false"
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement) << "}";
  return out.str();
}

} // namespace run_compare_detail

[[nodiscard]] inline std::string build_marshal_run_comparison_json(
    const marshal_run_compare_options_t &options) {
  marshal_operational_report_options_t report_options{};
  report_options.runtime_root = options.runtime_root;
  report_options.config_path = options.config_path;
  report_options.job_ids = {options.baseline_job_id, options.candidate_job_id};
  const auto jobs = operational_report_detail::discover_jobs(report_options);
  const auto *baseline =
      run_compare_detail::find_job(jobs, options.baseline_job_id);
  const auto *candidate =
      run_compare_detail::find_job(jobs, options.candidate_job_id);

  if (baseline == nullptr || candidate == nullptr) {
    std::vector<std::string> missing;
    if (baseline == nullptr) {
      missing.push_back("baseline_job_not_found");
    }
    if (candidate == nullptr) {
      missing.push_back("candidate_job_not_found");
    }
    return "{\"ok\":false,\"report_kind\":\"run_compare\""
           ",\"schema_version\":" +
           detail::json_quote(k_marshal_run_compare_schema_v1) +
           ",\"read_only\":true,\"target_satisfaction_claimed\":false"
           ",\"refusal_reasons\":" +
           operational_report_detail::json_string_array(missing) + "}";
  }

  std::ostringstream digest_basis;
  digest_basis << "runtime_root=" << options.runtime_root.string() << "\n"
               << "baseline="
               << run_compare_detail::get(baseline->state, "job_id") << "|"
               << run_compare_detail::get(baseline->state, "status") << "\n"
               << "candidate="
               << run_compare_detail::get(candidate->state, "job_id") << "|"
               << run_compare_detail::get(candidate->state, "status") << "\n";
  const auto comparison_id = marshal_digest_for_text(
      "kikijyeba.marshal.run_compare_id.v1", digest_basis.str());
  const auto generated_at = operational_report_detail::current_utc_timestamp();

  std::ostringstream out;
  out << "{\"ok\":true"
      << ",\"report_kind\":\"run_compare\""
      << ",\"schema_version\":"
      << detail::json_quote(k_marshal_run_compare_schema_v1)
      << ",\"comparison_id\":" << detail::json_quote(comparison_id)
      << ",\"generated_at\":" << detail::json_quote(generated_at)
      << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"target_satisfaction_claimed\":false"
      << ",\"model_selector\":false"
      << ",\"checkpoint_selected\":false"
      << ",\"evidence_scope\":{\"runtime_root\":"
      << detail::json_quote(options.runtime_root.string())
      << ",\"config_path\":" << detail::json_quote(options.config_path.string())
      << ",\"baseline_job_id\":"
      << detail::json_quote(run_compare_detail::get(baseline->state, "job_id"))
      << ",\"candidate_job_id\":"
      << detail::json_quote(run_compare_detail::get(candidate->state, "job_id"))
      << "}"
      << ",\"operator_summary\":"
      << run_compare_detail::operator_summary_json(*baseline, *candidate)
      << ",\"stop_reason\":" << run_compare_detail::stop_reason_json()
      << ",\"runtime_panel\":"
      << run_compare_detail::runtime_panel_json(*baseline, *candidate)
      << ",\"lattice_panel\":" << run_compare_detail::lattice_panel_json()
      << ",\"audit_panel\":"
      << run_compare_detail::audit_panel_json(options, comparison_id,
                                              generated_at)
      << ",\"run_pair\":"
      << run_compare_detail::run_pair_json(*baseline, *candidate)
      << ",\"comparability\":"
      << run_compare_detail::comparability_json(*baseline, *candidate)
      << ",\"config_identity_deltas\":"
      << run_compare_detail::config_identity_deltas_json(*baseline, *candidate)
      << ",\"wave_range_deltas\":"
      << run_compare_detail::wave_range_deltas_json(*baseline, *candidate)
      << ",\"metric_deltas\":"
      << run_compare_detail::metric_deltas_json(*baseline, *candidate)
      << ",\"checkpoint_deltas\":"
      << run_compare_detail::checkpoint_delta_summary_json(*baseline,
                                                           *candidate)
      << ",\"warning_deltas\":"
      << run_compare_detail::warning_delta_json(*baseline, *candidate)
      << ",\"proof_context\":{\"source\":\"not_evaluated_by_compare\""
      << ",\"lattice_cleanliness\":\"not_claimed\""
      << ",\"requires_lattice_replay\":true"
      << ",\"target_satisfaction_claimed\":false}"
      << ",\"verdict\":{\"type\":\"descriptive\",\"text\":"
      << detail::json_quote("Compare does not choose. Compare explains.") << "}"
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (options.include_machine_payload) {
    out << ",\"machine_payload\":{\"job_count\":" << jobs.size()
        << ",\"baseline_run\":"
        << operational_report_detail::job_json(*baseline)
        << ",\"candidate_run\":"
        << operational_report_detail::job_json(*candidate)
        << ",\"performance_visibility\":{\"baseline\":"
        << run_compare_detail::performance_panel_json(*baseline)
        << ",\"candidate\":"
        << run_compare_detail::performance_panel_json(*candidate) << "}"
        << ",\"checkpoint_lineage_deltas\":"
        << run_compare_detail::checkpoint_delta_detail_json(*baseline,
                                                            *candidate)
        << ",\"baseline_metrics\":"
        << operational_report_detail::metric_json_for_job(*baseline)
        << ",\"candidate_metrics\":"
        << operational_report_detail::metric_json_for_job(*candidate) << "}";
  }
  out << "}";
  return out.str();
}

} // namespace cuwacunu::hero::marshal
