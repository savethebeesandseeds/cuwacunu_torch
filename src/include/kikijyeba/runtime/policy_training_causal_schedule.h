// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::runtime {

inline constexpr const char *k_policy_training_causal_schedule_schema_v1 =
    "kikijyeba.runtime.policy_training_causal_schedule.v1";
inline constexpr const char *k_policy_training_schedule_mode_causal_v1 =
    "causal_walk_forward_training.v1";
inline constexpr const char *k_policy_training_schedule_mode_offline_research =
    "offline_full_window_research";
inline constexpr const char *k_policy_training_schedule_mode_final_refit =
    "batch_final_refit_candidate";

inline constexpr const char
    *k_policy_training_schedule_no_future_snapshot_source_v1 =
        "derived_from_artifact_fit_use_ledgers";

enum class cursor_key_kind_t {
  numeric_anchor_index,
  numeric_source_key,
  fixed_width_source_key,
  timestamp_ms,
  opaque_unsortable,
};

struct artifact_fit_ledger_t {
  std::string artifact_digest{};
  std::string artifact_kind{};
  std::string artifact_schema_id{};
  std::string producer_job_id{};
  std::string producer_runtime_run_id{};
  std::string component_family_id{};
  std::string component_assembly_id{};
  std::string protocol_contract_digest{};
  std::string graph_order_fingerprint{};
  std::string dock_binding_digest{};

  std::string fit_cursor_begin{};
  std::string fit_cursor_end{};
  std::string fit_input_cutoff_key{};
  std::string fit_target_cutoff_key{};
  std::string normalization_cutoff_key{};
  std::string calibration_cutoff_key{};
  std::string covariance_fit_cutoff_key{};
  std::string replay_buffer_cutoff_key{};
  std::string fit_cutoff_key{};
  std::string target_label_available_from_key{};
  std::string reward_available_from_key{};
  std::string trajectory_usable_from_key{};
  std::string usable_from_key{};
  std::string embargo_policy_id{};
  std::int64_t embargo_steps{0};
  std::string embargo_reason{};
  std::string source_range_digest{};
  std::string training_block_id{};
  std::string snapshot_generation{};
  std::vector<std::string> parent_artifact_digests{};

  [[nodiscard]] std::string to_text() const;
};

struct artifact_use_ledger_t {
  std::string block_id{};
  std::string block_cursor_begin{};
  std::string block_cursor_end{};
  std::string trajectory_digest{};
  std::string snapshot_family_digest_used{};
  std::string representation_snapshot_digest_used{};
  std::string mdn_snapshot_digest_used{};
  std::string observer_belief_snapshot_digest_used{};
  std::string policy_snapshot_digest_used{};
  std::string normalization_snapshot_digest_used{};
  std::string calibration_snapshot_digest_used{};
  std::string covariance_coupler_snapshot_digest_used{};
  std::string replay_buffer_snapshot_digest_used{};
  std::string reward_baseline_snapshot_digest_used{};
  std::string cajtucu_execution_profile_digest{};
  std::string reward_contract_digest{};
  std::string no_future_snapshot_use_source{
      k_policy_training_schedule_no_future_snapshot_source_v1};
  bool no_future_snapshot_use{false};

  [[nodiscard]] std::vector<std::string> used_artifact_digests() const;
  [[nodiscard]] std::string to_text() const;
};

struct snapshot_family_t {
  std::string snapshot_family_digest{};
  std::string representation_snapshot_digest{};
  std::string mdn_snapshot_digest{};
  std::string observer_belief_snapshot_digest{};
  std::string policy_snapshot_digest{};
  std::string normalization_snapshot_digest{};
  std::string calibration_snapshot_digest{};
  std::string covariance_coupler_snapshot_digest{};
  std::string replay_buffer_snapshot_digest{};
  std::string reward_baseline_snapshot_digest{};

  [[nodiscard]] std::string to_text() const;
};

struct causal_training_block_t {
  std::string block_id{};
  std::string block_cursor_begin{};
  std::string block_cursor_end{};
  std::string block_digest{};
  std::int64_t observation_count{0};
  artifact_use_ledger_t artifact_use{};

  [[nodiscard]] std::string to_text() const;
};

struct causal_policy_training_schedule_t {
  std::string schema_version{k_policy_training_causal_schedule_schema_v1};
  std::string schedule_id{};
  std::string training_schedule_mode{k_policy_training_schedule_mode_causal_v1};
  std::string cursor_key_kind{"numeric_anchor_index"};
  std::string training_range_digest{};
  std::string validation_range_digest{};
  std::string test_range_digest{};
  std::vector<artifact_fit_ledger_t> artifact_fits{};
  std::vector<snapshot_family_t> snapshot_families{};
  std::vector<causal_training_block_t> blocks{};
  bool readiness_eligible{true};
  bool offline_full_window_research_allowed{false};
  bool block_close_joint_update_allowed{true};
  bool strict_staggered_generations{false};
  bool final_refit_uses_validation{false};
  bool validation_no_longer_proof{false};
  bool sealed_test_required{false};

  [[nodiscard]] std::string to_text() const;
};

[[nodiscard]] inline bool
policy_training_schedule_mode_supported(const std::string &mode) {
  return mode == k_policy_training_schedule_mode_causal_v1 ||
         mode == k_policy_training_schedule_mode_offline_research ||
         mode == k_policy_training_schedule_mode_final_refit;
}

[[nodiscard]] inline const char *cursor_key_kind_name(cursor_key_kind_t kind) {
  switch (kind) {
  case cursor_key_kind_t::numeric_anchor_index:
    return "numeric_anchor_index";
  case cursor_key_kind_t::numeric_source_key:
    return "numeric_source_key";
  case cursor_key_kind_t::fixed_width_source_key:
    return "fixed_width_source_key";
  case cursor_key_kind_t::timestamp_ms:
    return "timestamp_ms";
  case cursor_key_kind_t::opaque_unsortable:
    return "opaque_unsortable";
  }
  return "opaque_unsortable";
}

[[nodiscard]] inline cursor_key_kind_t
parse_cursor_key_kind(const std::string &value) {
  if (value == "numeric_anchor_index") {
    return cursor_key_kind_t::numeric_anchor_index;
  }
  if (value == "numeric_source_key") {
    return cursor_key_kind_t::numeric_source_key;
  }
  if (value == "fixed_width_source_key") {
    return cursor_key_kind_t::fixed_width_source_key;
  }
  if (value == "timestamp_ms") {
    return cursor_key_kind_t::timestamp_ms;
  }
  return cursor_key_kind_t::opaque_unsortable;
}

namespace policy_training_schedule_detail {

[[nodiscard]] inline bool parse_i64_exact(std::string_view text,
                                          std::int64_t *out) {
  if (text.empty()) {
    return false;
  }
  std::int64_t parsed = 0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] inline bool key_less_equal(cursor_key_kind_t kind,
                                         const std::string &lhs,
                                         const std::string &rhs) {
  if (lhs.empty() || rhs.empty()) {
    return false;
  }
  if (kind == cursor_key_kind_t::opaque_unsortable) {
    return false;
  }
  if (kind == cursor_key_kind_t::fixed_width_source_key) {
    return lhs.size() == rhs.size() && lhs <= rhs;
  }
  std::int64_t lhs_i = 0;
  std::int64_t rhs_i = 0;
  if (parse_i64_exact(lhs, &lhs_i) && parse_i64_exact(rhs, &rhs_i)) {
    return lhs_i <= rhs_i;
  }
  return false;
}

inline void append_string_vector(std::ostringstream &out,
                                 const std::string &prefix,
                                 const std::vector<std::string> &values) {
  out << prefix << ".count=" << values.size() << "\n";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out << prefix << "." << i << "=" << values[i] << "\n";
  }
}

} // namespace policy_training_schedule_detail

[[nodiscard]] inline std::string artifact_fit_ledger_t::to_text() const {
  std::ostringstream out;
  out << "artifact_digest=" << artifact_digest << "\n";
  out << "artifact_kind=" << artifact_kind << "\n";
  out << "artifact_schema_id=" << artifact_schema_id << "\n";
  out << "producer_job_id=" << producer_job_id << "\n";
  out << "producer_runtime_run_id=" << producer_runtime_run_id << "\n";
  out << "component_family_id=" << component_family_id << "\n";
  out << "component_assembly_id=" << component_assembly_id << "\n";
  out << "protocol_contract_digest=" << protocol_contract_digest << "\n";
  out << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
  out << "dock_binding_digest=" << dock_binding_digest << "\n";
  out << "fit_cursor_begin=" << fit_cursor_begin << "\n";
  out << "fit_cursor_end=" << fit_cursor_end << "\n";
  out << "fit_input_cutoff_key=" << fit_input_cutoff_key << "\n";
  out << "fit_target_cutoff_key=" << fit_target_cutoff_key << "\n";
  out << "normalization_cutoff_key=" << normalization_cutoff_key << "\n";
  out << "calibration_cutoff_key=" << calibration_cutoff_key << "\n";
  out << "covariance_fit_cutoff_key=" << covariance_fit_cutoff_key << "\n";
  out << "replay_buffer_cutoff_key=" << replay_buffer_cutoff_key << "\n";
  out << "fit_cutoff_key=" << fit_cutoff_key << "\n";
  out << "target_label_available_from_key=" << target_label_available_from_key
      << "\n";
  out << "reward_available_from_key=" << reward_available_from_key << "\n";
  out << "trajectory_usable_from_key=" << trajectory_usable_from_key << "\n";
  out << "usable_from_key=" << usable_from_key << "\n";
  out << "embargo_policy_id=" << embargo_policy_id << "\n";
  out << "embargo_steps=" << embargo_steps << "\n";
  out << "embargo_reason=" << embargo_reason << "\n";
  out << "source_range_digest=" << source_range_digest << "\n";
  out << "training_block_id=" << training_block_id << "\n";
  out << "snapshot_generation=" << snapshot_generation << "\n";
  policy_training_schedule_detail::append_string_vector(
      out, "parent_artifact_digest", parent_artifact_digests);
  return out.str();
}

[[nodiscard]] inline std::vector<std::string>
artifact_use_ledger_t::used_artifact_digests() const {
  std::vector<std::string> out;
  const auto append = [&out](const std::string &digest) {
    if (!digest.empty()) {
      out.push_back(digest);
    }
  };
  append(representation_snapshot_digest_used);
  append(mdn_snapshot_digest_used);
  append(observer_belief_snapshot_digest_used);
  append(policy_snapshot_digest_used);
  append(normalization_snapshot_digest_used);
  append(calibration_snapshot_digest_used);
  append(covariance_coupler_snapshot_digest_used);
  append(replay_buffer_snapshot_digest_used);
  append(reward_baseline_snapshot_digest_used);
  return out;
}

[[nodiscard]] inline std::string artifact_use_ledger_t::to_text() const {
  std::ostringstream out;
  out << "block_id=" << block_id << "\n";
  out << "block_cursor_begin=" << block_cursor_begin << "\n";
  out << "block_cursor_end=" << block_cursor_end << "\n";
  out << "trajectory_digest=" << trajectory_digest << "\n";
  out << "snapshot_family_digest_used=" << snapshot_family_digest_used << "\n";
  out << "representation_snapshot_digest_used="
      << representation_snapshot_digest_used << "\n";
  out << "mdn_snapshot_digest_used=" << mdn_snapshot_digest_used << "\n";
  out << "observer_belief_snapshot_digest_used="
      << observer_belief_snapshot_digest_used << "\n";
  out << "policy_snapshot_digest_used=" << policy_snapshot_digest_used << "\n";
  out << "normalization_snapshot_digest_used="
      << normalization_snapshot_digest_used << "\n";
  out << "calibration_snapshot_digest_used=" << calibration_snapshot_digest_used
      << "\n";
  out << "covariance_coupler_snapshot_digest_used="
      << covariance_coupler_snapshot_digest_used << "\n";
  out << "replay_buffer_snapshot_digest_used="
      << replay_buffer_snapshot_digest_used << "\n";
  out << "reward_baseline_snapshot_digest_used="
      << reward_baseline_snapshot_digest_used << "\n";
  out << "cajtucu_execution_profile_digest=" << cajtucu_execution_profile_digest
      << "\n";
  out << "reward_contract_digest=" << reward_contract_digest << "\n";
  out << "no_future_snapshot_use_source=" << no_future_snapshot_use_source
      << "\n";
  out << "no_future_snapshot_use="
      << (no_future_snapshot_use ? "true" : "false") << "\n";
  return out.str();
}

[[nodiscard]] inline std::string snapshot_family_t::to_text() const {
  std::ostringstream out;
  out << "snapshot_family_digest=" << snapshot_family_digest << "\n";
  out << "representation_snapshot_digest=" << representation_snapshot_digest
      << "\n";
  out << "mdn_snapshot_digest=" << mdn_snapshot_digest << "\n";
  out << "observer_belief_snapshot_digest=" << observer_belief_snapshot_digest
      << "\n";
  out << "policy_snapshot_digest=" << policy_snapshot_digest << "\n";
  out << "normalization_snapshot_digest=" << normalization_snapshot_digest
      << "\n";
  out << "calibration_snapshot_digest=" << calibration_snapshot_digest << "\n";
  out << "covariance_coupler_snapshot_digest="
      << covariance_coupler_snapshot_digest << "\n";
  out << "replay_buffer_snapshot_digest=" << replay_buffer_snapshot_digest
      << "\n";
  out << "reward_baseline_snapshot_digest=" << reward_baseline_snapshot_digest
      << "\n";
  return out.str();
}

[[nodiscard]] inline std::string causal_training_block_t::to_text() const {
  std::ostringstream out;
  out << "block_id=" << block_id << "\n";
  out << "block_cursor_begin=" << block_cursor_begin << "\n";
  out << "block_cursor_end=" << block_cursor_end << "\n";
  out << "block_digest=" << block_digest << "\n";
  out << "observation_count=" << observation_count << "\n";
  out << "artifact_use {\n" << artifact_use.to_text() << "}\n";
  return out.str();
}

[[nodiscard]] inline std::string
causal_policy_training_schedule_t::to_text() const {
  std::ostringstream out;
  out << "schema_version=" << schema_version << "\n";
  out << "schedule_id=" << schedule_id << "\n";
  out << "training_schedule_mode=" << training_schedule_mode << "\n";
  out << "cursor_key_kind=" << cursor_key_kind << "\n";
  out << "training_range_digest=" << training_range_digest << "\n";
  out << "validation_range_digest=" << validation_range_digest << "\n";
  out << "test_range_digest=" << test_range_digest << "\n";
  out << "readiness_eligible=" << (readiness_eligible ? "true" : "false")
      << "\n";
  out << "offline_full_window_research_allowed="
      << (offline_full_window_research_allowed ? "true" : "false") << "\n";
  out << "block_close_joint_update_allowed="
      << (block_close_joint_update_allowed ? "true" : "false") << "\n";
  out << "strict_staggered_generations="
      << (strict_staggered_generations ? "true" : "false") << "\n";
  out << "final_refit_uses_validation="
      << (final_refit_uses_validation ? "true" : "false") << "\n";
  out << "validation_no_longer_proof="
      << (validation_no_longer_proof ? "true" : "false") << "\n";
  out << "sealed_test_required=" << (sealed_test_required ? "true" : "false")
      << "\n";
  out << "artifact_fit.count=" << artifact_fits.size() << "\n";
  for (std::size_t i = 0; i < artifact_fits.size(); ++i) {
    out << "artifact_fit." << i << " {\n"
        << artifact_fits[i].to_text() << "}\n";
  }
  out << "snapshot_family.count=" << snapshot_families.size() << "\n";
  for (std::size_t i = 0; i < snapshot_families.size(); ++i) {
    out << "snapshot_family." << i << " {\n"
        << snapshot_families[i].to_text() << "}\n";
  }
  out << "block.count=" << blocks.size() << "\n";
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    out << "block." << i << " {\n" << blocks[i].to_text() << "}\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string
policy_training_causal_schedule_digest_for_text(std::string_view text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.runtime.policy_training_causal_schedule.digest.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string policy_training_causal_schedule_digest(
    const causal_policy_training_schedule_t &schedule) {
  return policy_training_causal_schedule_digest_for_text(schedule.to_text());
}

[[nodiscard]] inline std::vector<std::string>
validate_causal_policy_training_schedule(
    const causal_policy_training_schedule_t &schedule) {
  namespace detail = policy_training_schedule_detail;
  std::vector<std::string> issues;
  const auto require_nonempty = [&issues](const std::string &value,
                                          const char *field) {
    if (value.empty()) {
      issues.emplace_back(std::string("missing_") + field);
    }
  };

  require_nonempty(schedule.schedule_id, "schedule_id");
  require_nonempty(schedule.training_schedule_mode, "training_schedule_mode");
  require_nonempty(schedule.cursor_key_kind, "cursor_key_kind");
  require_nonempty(schedule.training_range_digest, "training_range_digest");
  require_nonempty(schedule.validation_range_digest, "validation_range_digest");
  require_nonempty(schedule.test_range_digest, "test_range_digest");
  const auto cursor_kind = parse_cursor_key_kind(schedule.cursor_key_kind);
  if (cursor_kind == cursor_key_kind_t::opaque_unsortable) {
    issues.emplace_back("opaque_or_unsupported_cursor_key_kind");
  }
  if (!policy_training_schedule_mode_supported(
          schedule.training_schedule_mode)) {
    issues.emplace_back("unsupported_training_schedule_mode");
  }
  if (schedule.training_schedule_mode ==
      k_policy_training_schedule_mode_offline_research) {
    if (schedule.readiness_eligible) {
      issues.emplace_back(
          "offline_full_window_research_not_readiness_eligible");
    }
    if (!schedule.offline_full_window_research_allowed) {
      issues.emplace_back(
          "offline_full_window_research_requires_explicit_allow");
    }
  }
  if (schedule.training_schedule_mode ==
      k_policy_training_schedule_mode_final_refit) {
    if (schedule.readiness_eligible) {
      issues.emplace_back("batch_final_refit_not_readiness_proof");
    }
    if (!schedule.final_refit_uses_validation) {
      issues.emplace_back("batch_final_refit_must_declare_validation_use");
    }
    if (!schedule.validation_no_longer_proof) {
      issues.emplace_back("final_refit_validation_no_longer_proof_required");
    }
    if (!schedule.sealed_test_required) {
      issues.emplace_back("final_refit_requires_sealed_test");
    }
  }

  std::unordered_map<std::string, const artifact_fit_ledger_t *> artifacts;
  std::set<std::string> seen_artifacts;
  bool derived_no_future_snapshot_use = true;
  const auto mark_future_issue = [&derived_no_future_snapshot_use,
                                  &issues](std::string issue) {
    derived_no_future_snapshot_use = false;
    issues.push_back(std::move(issue));
  };
  for (const auto &artifact : schedule.artifact_fits) {
    require_nonempty(artifact.artifact_digest, "artifact_digest");
    require_nonempty(artifact.artifact_kind, "artifact_kind");
    require_nonempty(artifact.fit_cutoff_key, "fit_cutoff_key");
    require_nonempty(artifact.usable_from_key, "usable_from_key");
    require_nonempty(artifact.training_block_id, "artifact_training_block_id");
    if (!artifact.artifact_digest.empty()) {
      if (!seen_artifacts.insert(artifact.artifact_digest).second) {
        issues.emplace_back("duplicate_artifact_digest:" +
                            artifact.artifact_digest);
      }
      artifacts.emplace(artifact.artifact_digest, &artifact);
    }
    if (artifact.embargo_policy_id.empty()) {
      issues.emplace_back("missing_embargo_policy_id:" +
                          artifact.artifact_digest);
    }
    if (artifact.embargo_steps < 0) {
      issues.emplace_back("negative_embargo_steps:" + artifact.artifact_digest);
    }
    if (!artifact.fit_cutoff_key.empty() && !artifact.usable_from_key.empty() &&
        !detail::key_less_equal(cursor_kind, artifact.fit_cutoff_key,
                                artifact.usable_from_key)) {
      issues.emplace_back("artifact_usable_before_fit_cutoff:" +
                          artifact.artifact_digest);
    }
    const auto check_cutoff_before_fit = [&](const std::string &cutoff,
                                             const char *issue) {
      if (!cutoff.empty() && !artifact.fit_cutoff_key.empty() &&
          !detail::key_less_equal(cursor_kind, cutoff,
                                  artifact.fit_cutoff_key)) {
        issues.emplace_back(std::string(issue) + ":" +
                            artifact.artifact_digest);
      }
    };
    check_cutoff_before_fit(artifact.fit_input_cutoff_key,
                            "fit_input_after_fit_cutoff");
    check_cutoff_before_fit(artifact.fit_target_cutoff_key,
                            "fit_target_after_fit_cutoff");
    check_cutoff_before_fit(artifact.normalization_cutoff_key,
                            "normalization_after_fit_cutoff");
    check_cutoff_before_fit(artifact.calibration_cutoff_key,
                            "calibration_after_fit_cutoff");
    check_cutoff_before_fit(artifact.covariance_fit_cutoff_key,
                            "covariance_after_fit_cutoff");
    check_cutoff_before_fit(artifact.replay_buffer_cutoff_key,
                            "replay_buffer_after_fit_cutoff");
    const auto check_available_before_usable =
        [&](const std::string &available_from, const char *issue) {
          if (!available_from.empty() && !artifact.usable_from_key.empty() &&
              !detail::key_less_equal(cursor_kind, available_from,
                                      artifact.usable_from_key)) {
            issues.emplace_back(std::string(issue) + ":" +
                                artifact.artifact_digest);
          }
        };
    check_available_before_usable(artifact.target_label_available_from_key,
                                  "target_label_available_after_usable_from");
    check_available_before_usable(artifact.reward_available_from_key,
                                  "reward_available_after_usable_from");
    check_available_before_usable(artifact.trajectory_usable_from_key,
                                  "trajectory_available_after_usable_from");
  }

  if (schedule.training_schedule_mode ==
          k_policy_training_schedule_mode_causal_v1 &&
      schedule.blocks.empty()) {
    issues.emplace_back("missing_causal_training_blocks");
  }

  for (const auto &block : schedule.blocks) {
    require_nonempty(block.block_id, "block_id");
    require_nonempty(block.block_cursor_begin, "block_cursor_begin");
    require_nonempty(block.block_cursor_end, "block_cursor_end");
    if (!block.block_cursor_begin.empty() && !block.block_cursor_end.empty() &&
        !detail::key_less_equal(cursor_kind, block.block_cursor_begin,
                                block.block_cursor_end)) {
      issues.emplace_back("block_cursor_range_invalid:" + block.block_id);
    }
    if (block.observation_count <= 0) {
      issues.emplace_back("nonpositive_block_observation_count:" +
                          block.block_id);
    }
    if (block.artifact_use.block_id != block.block_id) {
      issues.emplace_back("artifact_use_block_id_mismatch:" + block.block_id);
    }
    if (!block.artifact_use.no_future_snapshot_use_source.empty() &&
        block.artifact_use.no_future_snapshot_use_source !=
            k_policy_training_schedule_no_future_snapshot_source_v1) {
      issues.emplace_back("unsupported_no_future_snapshot_use_source:" +
                          block.block_id);
    }
    for (const auto &digest : block.artifact_use.used_artifact_digests()) {
      const auto it = artifacts.find(digest);
      if (it == artifacts.end()) {
        mark_future_issue("used_snapshot_not_in_fit_ledger:" + digest);
        continue;
      }
      const auto &artifact = *it->second;
      if (artifact.training_block_id == block.block_id) {
        mark_future_issue("same_block_snapshot_use:" + artifact.artifact_kind +
                          ":" + digest);
      }
      if (!detail::key_less_equal(cursor_kind, artifact.usable_from_key,
                                  block.block_cursor_begin)) {
        mark_future_issue("future_snapshot_use:" + artifact.artifact_kind +
                          ":" + digest);
      }
    }
  }
  if (!derived_no_future_snapshot_use) {
    issues.emplace_back("derived_no_future_snapshot_use_false");
  }
  return issues;
}

[[nodiscard]] inline bool
causal_policy_training_schedule_no_future_snapshot_use(
    const causal_policy_training_schedule_t &schedule) {
  const auto issues = validate_causal_policy_training_schedule(schedule);
  return std::none_of(issues.begin(), issues.end(), [](const auto &issue) {
    return issue.find("future_snapshot_use") != std::string::npos ||
           issue.find("same_block_snapshot_use") != std::string::npos ||
           issue.find("used_snapshot_not_in_fit_ledger") != std::string::npos ||
           issue == "opaque_or_unsupported_cursor_key_kind" ||
           issue == "derived_no_future_snapshot_use_false";
  });
}

[[nodiscard]] inline bool causal_policy_training_schedule_valid(
    const causal_policy_training_schedule_t &schedule) {
  return validate_causal_policy_training_schedule(schedule).empty();
}

} // namespace cuwacunu::kikijyeba::runtime
