// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/output/experience_trace.h"
#include "kikijyeba/environment/policy/allocation.h"
#include "kikijyeba/environment/policy/baseline.h"
#include "kikijyeba/environment/run/experiment_runner.h"
#include "kikijyeba/environment/runtime/replay_source.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/config_provenance.h"
#include "kikijyeba/runtime/job_layout.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::environment {

inline constexpr const char *kRuntimeReplayExperimentIndexSchema =
    "kikijyeba.environment.replay.runtime_experiment_index.v1";

struct runtime_job_replay_driver_options_t {
  std::filesystem::path job_dir{};
  std::string config_path{};
  std::string experiment_id{};
  std::string environment_run_id{};
  std::string execution_profile_digest{};
  std::string policy_set_digest{};

  std::string base_reserve_node_id{};
  std::vector<std::string> risky_node_ids{};

  double initial_equity_base{1.0};
  double min_base_reserve_weight{0.0};
  double max_risky_weight{1.0};
  double max_turnover_l1{1.0};
  double lambda_cvar{0.10};
  double lambda_concentration{0.01};
  double lambda_uncertainty{0.0};
  double lambda_turnover{0.0};

  bool include_base_reserve_policy{true};
  bool include_equal_weight_policy{false};
  bool include_current_weight_policy{false};
  bool include_spot_distributional_utility_policy{true};

  replay_experiment_options_t experiment_options{};
  replay::replay_frame_build_options_t frame_options{
      .realization_channel_index = -2};
  replay::replay_world_options_t world_options{};
  replay::replay_observation_artifact_options_t artifact_options{};
  bool require_observation_artifacts{true};

  bool write_report{true};
  std::filesystem::path report_path{};
  bool write_experience_trace{true};
  std::filesystem::path experience_trace_path{};
};

struct runtime_job_replay_driver_result_t {
  replay_experiment_report_t report{};
  std::filesystem::path report_path{};
  std::filesystem::path experience_trace_path{};
  std::filesystem::path experiment_index_path{};
  std::string config_path{};
  std::size_t replay_bundle_count{0};
};

struct runtime_replay_experiment_index_entry_t {
  std::string experiment_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::size_t policy_count{0};
  std::size_t replay_bundle_count{0};
  std::size_t attempted_count{0};
  std::size_t completed_count{0};
  std::filesystem::path report_path{};
  std::string report_digest{};
};

namespace replay_driver_detail {

namespace protocol = cuwacunu::kikijyeba::protocol;
namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace runtime_layout = cuwacunu::kikijyeba::runtime::job_layout;
namespace sdu =
    cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;

[[nodiscard]] inline std::string clean_value(std::string value) {
  for (auto &ch : value) {
    if (ch == '\n' || ch == '\r') {
      ch = ' ';
    }
  }
  return value;
}

inline void write_kv(std::ostream &out, const std::string &key,
                     std::string value) {
  out << key << "=" << clean_value(std::move(value)) << "\n";
}

[[nodiscard]] inline std::string
join_report_tokens(const std::vector<std::string> &values) {
  if (values.empty()) {
    return "none";
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] inline std::string
read_text_file_or_throw(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[runtime_job_replay_driver] unable to read " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::string
replay_report_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash,
      std::string("kikijyeba.environment.replay.report_digest.v1\n") + text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string
replay_report_digest_for_path(const std::filesystem::path &path) {
  return replay_report_digest_for_text(read_text_file_or_throw(path));
}

[[nodiscard]] inline std::string resolve_driver_config_path(
    const runtime_job_replay_driver_options_t &options,
    const replay::runtime_replay_job_evidence_t &evidence) {
  if (!options.config_path.empty()) {
    return options.config_path;
  }
  if (!evidence.manifest.config_path.empty()) {
    return evidence.manifest.config_path;
  }
  throw std::runtime_error(
      "[runtime_job_replay_driver] config_path is required when job.manifest "
      "does not record one");
}

inline void validate_driver_replay_environment_contract(
    const replay_environment_spec_t &replay_spec,
    const runtime_job_replay_driver_options_t &options) {
  validate_replay_environment_spec(replay_spec);
  if (replay_spec.require_projection_validation) {
    if (!options.world_options.require_projection_validation) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] replay environment contract requires "
          "projection validation");
    }
    if (!options.require_observation_artifacts) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] replay environment contract requires "
          "time-t observation artifacts for projection validation");
    }
  }
  if (replay_spec.require_no_future_leakage &&
      !options.artifact_options.require_artifact_time_not_after_observation) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] replay environment contract forbids "
        "future-dated observation artifacts");
  }
}

[[nodiscard]] inline portfolio::PortfolioConstraints make_driver_constraints(
    std::int64_t A, const sdu::spot_distributional_utility_spec_t &method_spec,
    const runtime_job_replay_driver_options_t &options) {
  if (A <= 0) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] risky asset count must be positive");
  }
  if (options.max_risky_weight < 0.0 || options.max_risky_weight > 1.0) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] max_risky_weight must be in [0,1]");
  }
  portfolio::PortfolioConstraints constraints{};
  constraints.min_base_reserve_weight = options.min_base_reserve_weight;
  constraints.max_weight =
      torch::full({A}, options.max_risky_weight, torch::kFloat64);
  constraints.min_weight = torch::zeros({A}, torch::kFloat64);
  constraints.max_turnover_l1 = options.max_turnover_l1;
  constraints.cvar_alpha = method_spec.cvar_alpha;
  constraints.lambda_cvar = options.lambda_cvar;
  constraints.lambda_concentration = options.lambda_concentration;
  constraints.lambda_uncertainty = options.lambda_uncertainty;
  constraints.lambda_turnover = options.lambda_turnover;
  constraints.scenario_growth_floor = method_spec.scenario_growth_floor;
  portfolio::validate_portfolio_constraints(constraints, A);
  return constraints;
}

[[nodiscard]] inline episode_spec_t make_driver_base_spec(
    const protocol::channel_graph_first_protocol_contract_t &contract,
    const graph::market_graph_t &market_graph,
    const runtime_job_replay_driver_options_t &options) {
  market_graph.validate();
  const std::string base_reserve_node_id =
      options.base_reserve_node_id.empty()
          ? replay::infer_base_reserve_node_id(market_graph)
          : options.base_reserve_node_id;
  if (!replay::node_index_by_id(market_graph.node_ids, base_reserve_node_id)
           .has_value()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] base reserve node must appear in graph");
  }

  auto risky_node_ids =
      options.risky_node_ids.empty()
          ? replay::default_risky_node_ids(market_graph, base_reserve_node_id)
          : options.risky_node_ids;
  for (const auto &node_id : risky_node_ids) {
    if (node_id == base_reserve_node_id) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] base reserve node must not be in risky "
          "node_ids");
    }
    if (!replay::node_index_by_id(market_graph.node_ids, node_id).has_value()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] risky node is not present in graph: " +
          node_id);
    }
  }

  const auto A = static_cast<std::int64_t>(risky_node_ids.size());
  episode_spec_t spec{};
  spec.protocol_id = contract.protocol_variant.protocol_id;
  spec.environment_run_id = options.environment_run_id;
  spec.graph_order_fingerprint =
      market_graph.computed_graph_order_fingerprint();
  spec.graph_node_ids = market_graph.node_ids;
  spec.risky_node_ids = std::move(risky_node_ids);
  spec.base_policy = {.accounting_numeraire_id = base_reserve_node_id,
                      .settlement_asset_id = base_reserve_node_id,
                      .reserve_asset_id = base_reserve_node_id,
                      .projection_reference_node_id = base_reserve_node_id};
  spec.constraints =
      make_driver_constraints(A, contract.spot_distributional_utility, options);
  spec.initial_equity_base = options.initial_equity_base;
  spec.require_projection_validation =
      options.world_options.require_projection_validation;
  return spec;
}

[[nodiscard]] inline replay_policy_factory_t
make_driver_base_reserve_policy_factory() {
  return {
      .policy_id = "base_reserve_only.v1",
      .policy_kind = policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            baseline_policy_config_t config{};
            config.policy_id = "base_reserve_only.v1";
            config.node_ids = bundle.spec.risky_node_ids;
            config.base_reserve_node_id =
                bundle.spec.base_policy.reserve_asset_id;
            return std::make_unique<base_reserve_policy_t>(std::move(config));
          },
  };
}

[[nodiscard]] inline replay_policy_factory_t
make_driver_equal_weight_policy_factory(double base_reserve_weight) {
  return {
      .policy_id = "equal_weight.v1",
      .policy_kind = policy_kind_t::baseline,
      .make_policy =
          [base_reserve_weight](const replay::replay_episode_bundle_t &bundle) {
            baseline_policy_config_t config{};
            config.policy_id = "equal_weight.v1";
            config.node_ids = bundle.spec.risky_node_ids;
            config.base_reserve_node_id =
                bundle.spec.base_policy.reserve_asset_id;
            return std::make_unique<equal_weight_policy_t>(std::move(config),
                                                           base_reserve_weight);
          },
  };
}

[[nodiscard]] inline replay_policy_factory_t
make_driver_current_weight_policy_factory() {
  return {
      .policy_id = "current_weight_no_trade.v1",
      .policy_kind = policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            baseline_policy_config_t config{};
            config.policy_id = "current_weight_no_trade.v1";
            config.node_ids = bundle.spec.risky_node_ids;
            config.base_reserve_node_id =
                bundle.spec.base_policy.reserve_asset_id;
            return std::make_unique<current_weight_policy_t>(std::move(config));
          },
  };
}

[[nodiscard]] inline replay_policy_factory_t
make_driver_spot_distributional_utility_policy_factory(
    portfolio::PortfolioConstraints constraints,
    const sdu::spot_distributional_utility_spec_t &method_spec) {
  return {
      .policy_id = kSpotDistributionalUtilityPolicyId,
      .policy_kind = policy_kind_t::deterministic_allocator,
      .make_policy =
          [constraints = std::move(constraints),
           iterations = method_spec.iterations,
           learning_rate = method_spec.learning_rate](
              const replay::replay_episode_bundle_t &) {
            spot_distributional_utility_policy_config_t config{};
            config.constraints = constraints;
            config.solver_options.iterations = iterations;
            config.solver_options.learning_rate = learning_rate;
            return std::make_unique<spot_distributional_utility_policy_t>(
                std::move(config));
          },
  };
}

[[nodiscard]] inline std::vector<replay_policy_factory_t>
make_driver_policy_factories(
    const episode_spec_t &base_spec,
    const sdu::spot_distributional_utility_spec_t &method_spec,
    const runtime_job_replay_driver_options_t &options) {
  std::vector<replay_policy_factory_t> factories;
  if (options.include_base_reserve_policy) {
    factories.push_back(make_driver_base_reserve_policy_factory());
  }
  if (options.include_equal_weight_policy) {
    factories.push_back(make_driver_equal_weight_policy_factory(
        base_spec.constraints.min_base_reserve_weight));
  }
  if (options.include_current_weight_policy) {
    factories.push_back(make_driver_current_weight_policy_factory());
  }
  if (options.include_spot_distributional_utility_policy) {
    factories.push_back(make_driver_spot_distributional_utility_policy_factory(
        base_spec.constraints, method_spec));
  }
  if (factories.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] at least one replay policy must be "
        "enabled");
  }
  return factories;
}

[[nodiscard]] inline std::filesystem::path
default_report_path(const std::filesystem::path &job_dir) {
  return replay::runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiment.report";
}

[[nodiscard]] inline std::filesystem::path
default_experience_trace_path_for_report(
    const std::filesystem::path &report_path) {
  if (report_path.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] report_path is required for experience "
        "trace sidecar path");
  }
  const auto filename =
      report_path.stem().string() + ".experience_trace" +
      (report_path.extension().empty() ? std::string{}
                                       : report_path.extension().string());
  return report_path.parent_path() / filename;
}

[[nodiscard]] inline std::size_t
parse_size_or(const std::unordered_map<std::string, std::string> &map,
              const std::string &key, std::size_t fallback = 0) {
  const auto value = runtime_layout::map_get(map, key);
  if (value.empty()) {
    return fallback;
  }
  return static_cast<std::size_t>(std::stoull(value));
}

[[nodiscard]] inline std::filesystem::path
normalize_report_path(std::filesystem::path path) {
  if (!path.empty() && path.parent_path().empty()) {
    return std::filesystem::current_path() / path;
  }
  return path;
}

[[nodiscard]] inline protocol::config_provenance::config_bundle_file_t
capture_replay_report_config_identity(const std::string &config_path) {
  if (detail::blank(config_path)) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] config_path is required for replay "
        "experiment reports");
  }
  auto file = protocol::config_provenance::make_config_bundle_file(
      "replay_report_config_path", std::filesystem::path(config_path),
      std::filesystem::current_path());
  if (!file.exists || !file.is_regular_file || file.content_digest.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] config_path must resolve to a regular "
        "file with a content digest for replay experiment reports");
  }
  return file;
}

inline void validate_replay_experiment_report_bundle_count(
    const replay_experiment_report_t &report, std::size_t replay_bundle_count) {
  if (replay_bundle_count == 0) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] replay_bundle_count must be positive");
  }
  const auto policy_count = report.policy_summaries.size();
  if (policy_count == 0) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] policy summary count must be positive");
  }
  if (replay_bundle_count >
      std::numeric_limits<std::size_t>::max() / policy_count) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] replay report attempted count overflow");
  }
  const auto expected_attempts = replay_bundle_count * policy_count;
  if (report.attempted_count != static_cast<std::uint64_t>(expected_attempts)) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] replay_bundle_count must match "
        "attempted_count / policy_summary_count");
  }
}

struct replay_report_time_law_summary_t {
  std::uint64_t expected_step_count{0};
  std::uint64_t observation_step_count{0};
  std::uint64_t action_step_count{0};
  std::uint64_t execution_step_count{0};
  std::uint64_t realization_after_action_count{0};
  std::uint64_t future_observation_violation_count{0};
  std::uint64_t mixed_future_realization_key_count{0};
  std::uint64_t projection_validation_step_count{0};
};

[[nodiscard]] inline replay_report_time_law_summary_t
summarize_replay_report_time_law(const replay_experiment_report_t &report) {
  replay_report_time_law_summary_t out{};
  for (const auto &episode : report.episode_reports) {
    out.expected_step_count +=
        static_cast<std::uint64_t>(episode.step_reports.size());
    const auto accepted_anchor_keys =
        episode_runner_detail::split_tokens(episode.accepted_anchor_keys);
    for (const auto &step : episode.step_reports) {
      const bool observation_bound = step.observation_anchor_index >= 0 &&
                                     !step.anchor_key.empty() &&
                                     step.knowledge_timestamp_ms > 0;
      if (observation_bound) {
        ++out.observation_step_count;
      }

      const bool action_bound =
          step.action_schema_id == kTargetWeightsActionSchema &&
          !step.target_risky_node_weights.empty() &&
          !step.target_base_reserve_node_id.empty() &&
          step.action_decision_timestamp_ms >= step.knowledge_timestamp_ms &&
          step.action_decision_timestamp_ms <
              step.realization_available_after_timestamp_ms;
      if (action_bound) {
        ++out.action_step_count;
      }

      const bool execution_bound =
          std::isfinite(step.portfolio_equity_before) &&
          std::isfinite(step.portfolio_equity_after) &&
          std::isfinite(step.realized_log_growth) &&
          std::isfinite(step.reward_total);
      if (execution_bound) {
        ++out.execution_step_count;
      }

      const bool realization_after_action =
          step.next_realization_anchor_index ==
              step.observation_anchor_index + 1 &&
          step.realization_available_after_timestamp_ms >
              step.knowledge_timestamp_ms;
      if (realization_after_action) {
        ++out.realization_after_action_count;
      } else {
        ++out.future_observation_violation_count;
      }

      const auto offset = step.accepted_cursor_offset;
      if (offset < 0 ||
          static_cast<std::size_t>(offset) >= accepted_anchor_keys.size() ||
          step.anchor_key !=
              accepted_anchor_keys[static_cast<std::size_t>(offset)]) {
        ++out.mixed_future_realization_key_count;
      }

      if (std::isfinite(step.projection_mae) &&
          std::isfinite(step.projection_rmse) &&
          std::isfinite(step.projection_signed_bias) &&
          std::isfinite(step.projection_correlation) &&
          std::isfinite(step.projection_directional_accuracy) &&
          std::isfinite(step.projection_interval_coverage)) {
        ++out.projection_validation_step_count;
      }
    }
  }
  return out;
}

inline void validate_replay_report_time_law_summary(
    const replay_report_time_law_summary_t &summary,
    const replay_environment_spec_t &replay_spec) {
  if (!replay_spec.require_no_future_leakage &&
      !replay_spec.require_projection_validation) {
    return;
  }
  if (replay_spec.require_no_future_leakage) {
    if (summary.observation_step_count != summary.expected_step_count ||
        summary.action_step_count != summary.expected_step_count ||
        summary.execution_step_count != summary.expected_step_count ||
        summary.realization_after_action_count != summary.expected_step_count ||
        summary.future_observation_violation_count != 0 ||
        summary.mixed_future_realization_key_count != 0) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] replay report does not satisfy "
          "required no-future-leakage counters");
    }
  }
  if (replay_spec.require_projection_validation &&
      summary.projection_validation_step_count != summary.expected_step_count) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] replay report does not satisfy required "
        "projection-validation counters");
  }
}

inline void write_replay_experiment_report(
    const replay_experiment_report_t &report, const std::filesystem::path &path,
    std::string config_path, std::size_t replay_bundle_count,
    replay_environment_spec_t replay_spec = {}) {
  if (path.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] report_path is required");
  }
  const auto config_identity =
      capture_replay_report_config_identity(config_path);
  validate_replay_environment_spec(replay_spec);
  experiment_runner_detail::validate_replay_experiment_report(report);
  validate_replay_experiment_report_bundle_count(report, replay_bundle_count);
  const auto time_law_summary = summarize_replay_report_time_law(report);
  validate_replay_report_time_law_summary(time_law_summary, replay_spec);
  std::ostringstream out;
  write_kv(out, "schema", report.experiment_artifact_schema_id);
  write_kv(out, "experiment_id", report.experiment_id);
  write_kv(out, "runtime_run_id", report.runtime_run_id);
  write_kv(out, "environment_run_id", report.environment_run_id);
  write_kv(out, "execution_profile_digest", report.execution_profile_digest);
  write_kv(out, "policy_set_digest", report.policy_set_digest);
  write_kv(out, "config_path", config_path);
  write_kv(out, "config_path_resolved",
           config_identity.path.lexically_normal().string());
  out << "config_path_exists=" << (config_identity.exists ? "true" : "false")
      << "\n";
  out << "config_path_is_regular_file="
      << (config_identity.is_regular_file ? "true" : "false") << "\n";
  out << "config_path_size_known="
      << (config_identity.size_known ? "true" : "false") << "\n";
  out << "config_path_size=" << config_identity.size << "\n";
  write_kv(out, "config_path_content_digest", config_identity.content_digest);
  write_kv(out, "replay_environment_version", replay_spec.version_token);
  write_kv(out, "replay_environment_component_assembly_id",
           replay_spec.component_assembly_id);
  write_kv(out, "replay_environment_world_mode", replay_spec.world_mode);
  write_kv(out, "replay_environment_api_contract", replay_spec.api_contract);
  write_kv(out, "replay_environment_spawn_model", replay_spec.spawn_model);
  write_kv(out, "replay_environment_range_source", replay_spec.range_source);
  write_kv(out, "replay_environment_source_range_policy",
           replay_spec.source_range_policy);
  write_kv(out, "replay_environment_source_order_policy",
           replay_spec.source_order_policy);
  write_kv(out, "replay_environment_range_resolution",
           replay_spec.range_resolution);
  write_kv(out, "replay_environment_observation_time_law",
           replay_spec.observation_time_law);
  write_kv(out, "replay_environment_realization_reveal",
           replay_spec.realization_reveal);
  write_kv(out, "replay_environment_realization_key_policy",
           replay_spec.realization_key_policy);
  write_kv(out, "replay_environment_action_kind", replay_spec.action_kind);
  write_kv(out, "replay_environment_action_schema_id",
           replay_spec.action_schema_id);
  write_kv(out, "replay_environment_action_time_policy",
           replay_spec.action_time_policy);
  write_kv(out, "replay_environment_reserve_node_policy",
           replay_spec.reserve_node_policy);
  write_kv(out, "replay_environment_graph_node_universe_policy",
           replay_spec.graph_node_universe_policy);
  write_kv(out, "replay_environment_reward_policy", replay_spec.reward_policy);
  write_kv(out, "replay_environment_projection_validation",
           replay_spec.projection_validation);
  write_kv(out, "replay_environment_policy_surface",
           replay_spec.policy_surface);
  write_kv(out, "replay_environment_action_policy_identity",
           replay_spec.action_policy_identity);
  write_kv(out, "replay_environment_initial_policy_kind",
           replay_spec.initial_policy_kind);
  write_kv(out, "replay_environment_experiment_task_identity",
           replay_spec.experiment_task_identity);
  write_kv(out, "replay_environment_experiment_run_identity",
           replay_spec.experiment_run_identity);
  write_kv(out, "replay_environment_step_artifact_identity",
           replay_spec.step_artifact_identity);
  write_kv(out, "replay_environment_experiment_report_count_policy",
           replay_spec.experiment_report_count_policy);
  write_kv(out, "replay_environment_artifact_schema",
           replay_spec.artifact_schema);
  write_kv(out, "replay_environment_lattice_fact_family",
           replay_spec.lattice_fact_family);
  write_kv(out, "replay_environment_lattice_target",
           replay_spec.lattice_target);
  out << "replay_environment_require_resolved_cursor="
      << replay_spec.require_resolved_cursor << "\n";
  out << "replay_environment_require_no_future_leakage="
      << replay_spec.require_no_future_leakage << "\n";
  out << "replay_environment_require_projection_validation="
      << replay_spec.require_projection_validation << "\n";
  out << "replay_environment_default_max_parallel_jobs="
      << replay_spec.default_max_parallel_jobs << "\n";
  out << "replay_bundle_count=" << replay_bundle_count << "\n";
  out << "experiment_requested_max_parallel_jobs="
      << report.requested_max_parallel_jobs << "\n";
  out << "experiment_resolved_parallelism=" << report.resolved_parallelism
      << "\n";
  write_kv(out, "top_level_metric_scope",
           "mean_over_completed_episode_reports_across_policies");
  write_kv(out, "policy_metric_scope",
           "mean_over_completed_episode_reports_for_policy");
  out << "attempted_count=" << report.attempted_count << "\n";
  out << "completed_count=" << report.completed_count << "\n";
  out << "failed_count=" << report.failures.size() << "\n";
  out << "mean_total_reward=" << report.mean_total_reward() << "\n";
  out << "mean_total_log_growth=" << report.mean_total_log_growth() << "\n";
  out << "mean_final_equity_base=" << report.mean_final_equity_base() << "\n";
  out << "mean_max_drawdown=" << report.mean_max_drawdown() << "\n";
  out << "mean_total_turnover=" << report.mean_total_turnover() << "\n";
  out << "mean_total_transaction_cost_base="
      << report.mean_total_transaction_cost_base() << "\n";
  out << "cajtucu_valid_trace_count=" << report.cajtucu_valid_trace_count()
      << "\n";
  out << "cajtucu_invalid_trace_count=" << report.cajtucu_invalid_trace_count()
      << "\n";
  out << "cajtucu_missing_direct_reserve_edge_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_missing_direct_reserve_edge_count;
             })
      << "\n";
  out << "cajtucu_nontradable_edge_reject_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_nontradable_edge_reject_count;
             })
      << "\n";
  out << "cajtucu_below_min_notional_reject_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_below_min_notional_reject_count;
             })
      << "\n";
  out << "cajtucu_above_max_notional_reject_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_above_max_notional_reject_count;
             })
      << "\n";
  out << "cajtucu_insufficient_reserve_reject_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_insufficient_reserve_reject_count;
             })
      << "\n";
  out << "cajtucu_insufficient_units_reject_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_insufficient_units_reject_count;
             })
      << "\n";
  out << "cajtucu_invalid_sell_price_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_invalid_sell_price_count;
             })
      << "\n";
  out << "cajtucu_large_equity_mismatch_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.cajtucu_large_equity_mismatch_count;
             })
      << "\n";
  out << "cajtucu_synthetic_market_step_count="
      << report.cajtucu_synthetic_market_step_count() << "\n";
  out << "requested_order_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.requested_order_count;
             })
      << "\n";
  out << "executed_order_count="
      << experiment_runner_detail::sum_episode_count(
             report.episode_reports,
             [](const episode_report_t &episode) {
               return episode.executed_order_count;
             })
      << "\n";
  out << "rejected_order_count=" << report.cajtucu_rejected_order_count()
      << "\n";
  out << "partial_order_count=" << report.cajtucu_partial_order_count() << "\n";
  out << "requested_notional_base=" << report.requested_notional_base() << "\n";
  out << "executed_notional_base=" << report.executed_notional_base() << "\n";
  out << "rejected_notional_base=" << report.rejected_notional_base() << "\n";
  out << "fill_ratio=" << report.fill_ratio() << "\n";
  out << "fee_cost_base=" << report.fee_cost_base() << "\n";
  out << "spread_cost_base=" << report.spread_cost_base() << "\n";
  out << "slippage_cost_base=" << report.slippage_cost_base() << "\n";
  out << "mean_target_weight_error_l1=" << report.mean_target_weight_error_l1()
      << "\n";
  out << "mean_target_weight_error_linf="
      << report.mean_target_weight_error_linf() << "\n";
  out << "mean_projection_mae=" << report.mean_projection_mae() << "\n";
  out << "mean_projection_rmse=" << report.mean_projection_rmse() << "\n";
  out << "mean_projection_signed_bias=" << report.mean_projection_signed_bias()
      << "\n";
  out << "mean_projection_correlation=" << report.mean_projection_correlation()
      << "\n";
  out << "mean_projection_directional_accuracy="
      << report.mean_projection_directional_accuracy() << "\n";
  out << "mean_projection_interval_coverage="
      << report.mean_projection_interval_coverage() << "\n";
  out << "mean_projection_interval_width="
      << report.mean_projection_interval_width() << "\n";
  out << "mean_zero_return_baseline_mae="
      << report.mean_zero_return_baseline_mae() << "\n";
  out << "mean_zero_return_baseline_rmse="
      << report.mean_zero_return_baseline_rmse() << "\n";
  out << "mean_model_skill_vs_zero_mae="
      << report.mean_model_skill_vs_zero_mae() << "\n";
  out << "mean_model_skill_vs_zero_rmse="
      << report.mean_model_skill_vs_zero_rmse() << "\n";
  out << "time_law_expected_step_count=" << time_law_summary.expected_step_count
      << "\n";
  out << "time_law_observation_step_count="
      << time_law_summary.observation_step_count << "\n";
  out << "time_law_action_step_count=" << time_law_summary.action_step_count
      << "\n";
  out << "time_law_execution_step_count="
      << time_law_summary.execution_step_count << "\n";
  out << "time_law_realization_after_action_count="
      << time_law_summary.realization_after_action_count << "\n";
  out << "time_law_future_observation_violation_count="
      << time_law_summary.future_observation_violation_count << "\n";
  out << "mixed_future_realization_key_count="
      << time_law_summary.mixed_future_realization_key_count << "\n";
  out << "projection_validation_step_count="
      << time_law_summary.projection_validation_step_count << "\n";
  out << "artifact_evidence=true\n";
  out << "visibility_only=true\n";
  out << "replay_executor=false\n";
  out << "allocation_authority=false\n";
  out << "execution_authority=false\n";
  out << "readiness_authority=false\n";
  out << "quality_authority=false\n";
  out << "performance_authority=false\n";
  out << "market_readiness_authority=false\n";
  out << "deployment_authority=false\n";
  out << "checkpoint_selector=false\n";
  out << "coverage_authority=false\n";
  out << "leakage_authority=false\n";
  out << "contract_identity_authority=false\n";

  out << "policy_summary_count=" << report.policy_summaries.size() << "\n";
  for (std::size_t i = 0; i < report.policy_summaries.size(); ++i) {
    const auto &summary = report.policy_summaries[i];
    const auto prefix = std::string("policy_") + std::to_string(i) + "_";
    write_kv(out, prefix + "schema", summary.policy_summary_schema_id);
    write_kv(out, prefix + "id", summary.policy_id);
    write_kv(out, prefix + "kind", policy_kind_name(summary.policy_kind));
    out << prefix << "attempted_count=" << summary.attempted_count << "\n";
    out << prefix << "completed_count=" << summary.completed_count << "\n";
    out << prefix << "failed_count=" << summary.failed_count << "\n";
    out << prefix << "mean_total_reward=" << summary.mean_total_reward << "\n";
    out << prefix << "mean_total_log_growth=" << summary.mean_total_log_growth
        << "\n";
    out << prefix << "mean_final_equity_base=" << summary.mean_final_equity_base
        << "\n";
    out << prefix << "mean_max_drawdown=" << summary.mean_max_drawdown << "\n";
    out << prefix << "mean_total_turnover=" << summary.mean_total_turnover
        << "\n";
    out << prefix << "mean_total_transaction_cost_base="
        << summary.mean_total_transaction_cost_base << "\n";
    out << prefix
        << "cajtucu_valid_trace_count=" << summary.cajtucu_valid_trace_count
        << "\n";
    out << prefix
        << "cajtucu_invalid_trace_count=" << summary.cajtucu_invalid_trace_count
        << "\n";
    out << prefix << "cajtucu_missing_direct_reserve_edge_count="
        << summary.cajtucu_missing_direct_reserve_edge_count << "\n";
    out << prefix << "cajtucu_nontradable_edge_reject_count="
        << summary.cajtucu_nontradable_edge_reject_count << "\n";
    out << prefix << "cajtucu_below_min_notional_reject_count="
        << summary.cajtucu_below_min_notional_reject_count << "\n";
    out << prefix << "cajtucu_above_max_notional_reject_count="
        << summary.cajtucu_above_max_notional_reject_count << "\n";
    out << prefix << "cajtucu_insufficient_reserve_reject_count="
        << summary.cajtucu_insufficient_reserve_reject_count << "\n";
    out << prefix << "cajtucu_insufficient_units_reject_count="
        << summary.cajtucu_insufficient_units_reject_count << "\n";
    out << prefix << "cajtucu_invalid_sell_price_count="
        << summary.cajtucu_invalid_sell_price_count << "\n";
    out << prefix << "cajtucu_large_equity_mismatch_count="
        << summary.cajtucu_large_equity_mismatch_count << "\n";
    out << prefix << "cajtucu_synthetic_market_step_count="
        << summary.cajtucu_synthetic_market_step_count << "\n";
    out << prefix << "requested_order_count=" << summary.requested_order_count
        << "\n";
    out << prefix << "executed_order_count=" << summary.executed_order_count
        << "\n";
    out << prefix << "rejected_order_count=" << summary.rejected_order_count
        << "\n";
    out << prefix << "partial_order_count=" << summary.partial_order_count
        << "\n";
    out << prefix
        << "requested_notional_base=" << summary.requested_notional_base
        << "\n";
    out << prefix << "executed_notional_base=" << summary.executed_notional_base
        << "\n";
    out << prefix << "rejected_notional_base=" << summary.rejected_notional_base
        << "\n";
    out << prefix << "partial_notional_base=" << summary.partial_notional_base
        << "\n";
    out << prefix << "fill_ratio=" << summary.fill_ratio << "\n";
    out << prefix << "fee_cost_base=" << summary.fee_cost_base << "\n";
    out << prefix << "spread_cost_base=" << summary.spread_cost_base << "\n";
    out << prefix << "slippage_cost_base=" << summary.slippage_cost_base
        << "\n";
    out << prefix
        << "cost_as_fraction_of_equity=" << summary.cost_as_fraction_of_equity
        << "\n";
    out << prefix << "cost_as_fraction_of_gross_return="
        << summary.cost_as_fraction_of_gross_return << "\n";
    out << prefix
        << "mean_target_weight_error_l1=" << summary.mean_target_weight_error_l1
        << "\n";
    out << prefix << "mean_target_weight_error_linf="
        << summary.mean_target_weight_error_linf << "\n";
    out << prefix << "mean_post_execution_risky_weight_sum="
        << summary.mean_post_execution_risky_weight_sum << "\n";
    out << prefix << "mean_post_execution_base_reserve_weight="
        << summary.mean_post_execution_base_reserve_weight << "\n";
    out << prefix
        << "reserve_shortfall_count=" << summary.reserve_shortfall_count
        << "\n";
    out << prefix
        << "mean_ledger_before_equity=" << summary.mean_ledger_before_equity
        << "\n";
    out << prefix << "mean_ledger_after_execution_equity="
        << summary.mean_ledger_after_execution_equity << "\n";
    out << prefix << "mean_ledger_after_realization_equity="
        << summary.mean_ledger_after_realization_equity << "\n";
    out << prefix << "max_ledger_equity_reconciliation_error="
        << summary.max_ledger_equity_reconciliation_error << "\n";
    out << prefix
        << "base_reserve_units_before=" << summary.base_reserve_units_before
        << "\n";
    out << prefix
        << "base_reserve_units_after=" << summary.base_reserve_units_after
        << "\n";
    out << prefix << "unit_nonnegativity_violation_count="
        << summary.unit_nonnegativity_violation_count << "\n";
    out << prefix << "mean_projection_mae=" << summary.mean_projection_mae
        << "\n";
    out << prefix << "mean_projection_rmse=" << summary.mean_projection_rmse
        << "\n";
    out << prefix
        << "mean_projection_signed_bias=" << summary.mean_projection_signed_bias
        << "\n";
    out << prefix
        << "mean_projection_correlation=" << summary.mean_projection_correlation
        << "\n";
    out << prefix << "mean_projection_directional_accuracy="
        << summary.mean_projection_directional_accuracy << "\n";
    out << prefix << "mean_projection_interval_coverage="
        << summary.mean_projection_interval_coverage << "\n";
    out << prefix << "mean_projection_interval_width="
        << summary.mean_projection_interval_width << "\n";
    out << prefix << "mean_zero_return_baseline_mae="
        << summary.mean_zero_return_baseline_mae << "\n";
    out << prefix << "mean_zero_return_baseline_rmse="
        << summary.mean_zero_return_baseline_rmse << "\n";
    out << prefix << "mean_model_skill_vs_zero_mae="
        << summary.mean_model_skill_vs_zero_mae << "\n";
    out << prefix << "mean_model_skill_vs_zero_rmse="
        << summary.mean_model_skill_vs_zero_rmse << "\n";
  }

  out << "episode_count=" << report.episode_reports.size() << "\n";
  for (std::size_t i = 0; i < report.episode_reports.size(); ++i) {
    const auto &episode = report.episode_reports[i];
    experiment_runner_detail::validate_episode_step_report_identity(episode);
    const auto prefix = std::string("episode_") + std::to_string(i) + "_";
    write_kv(out, prefix + "schema", episode.episode_artifact_schema_id);
    write_kv(out, prefix + "id", episode.episode_id);
    write_kv(out, prefix + "protocol_id", episode.protocol_id);
    write_kv(out, prefix + "runtime_run_id", episode.runtime_run_id);
    write_kv(out, prefix + "environment_run_id", episode.environment_run_id);
    write_kv(out, prefix + "policy_id", episode.policy_id);
    write_kv(out, prefix + "method_id", episode.method_id);
    write_kv(out, prefix + "policy_kind",
             policy_kind_name(episode.policy_kind));
    write_kv(out, prefix + "world_mode", world_mode_name(episode.world_mode));
    write_kv(out, prefix + "graph_order_fingerprint",
             episode.graph_order_fingerprint);
    write_kv(out, prefix + "graph_node_ids", episode.graph_node_ids);
    write_kv(out, prefix + "risky_node_ids", episode.risky_node_ids);
    write_kv(out, prefix + "base_reserve_node_id",
             episode.base_reserve_node_id);
    out << prefix << "experiment_task_index=" << episode.experiment_task_index
        << "\n";
    out << prefix
        << "experiment_bundle_index=" << episode.experiment_bundle_index
        << "\n";
    out << prefix
        << "experiment_policy_index=" << episode.experiment_policy_index
        << "\n";
    out << prefix << "requested_anchor_index_begin="
        << episode.requested_anchor_index_begin << "\n";
    out << prefix
        << "requested_anchor_index_end=" << episode.requested_anchor_index_end
        << "\n";
    out << prefix
        << "requested_source_key_begin=" << episode.requested_source_key_begin
        << "\n";
    out << prefix
        << "requested_source_key_end=" << episode.requested_source_key_end
        << "\n";
    write_kv(out, prefix + "accepted_cursor_wave_id",
             episode.accepted_cursor_wave_id);
    write_kv(out, prefix + "accepted_cursor_wave_mode",
             episode.accepted_cursor_wave_mode);
    write_kv(out, prefix + "accepted_cursor_kind",
             episode.accepted_cursor_kind);
    write_kv(out, prefix + "accepted_cursor_scope",
             episode.accepted_cursor_scope);
    write_kv(out, prefix + "accepted_source_range_policy",
             episode.accepted_source_range_policy);
    write_kv(out, prefix + "accepted_source_order_policy",
             episode.accepted_source_order_policy);
    write_kv(out, prefix + "accepted_batch_cursor_token",
             episode.accepted_batch_cursor_token);
    out << prefix
        << "accepted_anchor_index_begin=" << episode.accepted_anchor_index_begin
        << "\n";
    out << prefix
        << "accepted_anchor_index_end=" << episode.accepted_anchor_index_end
        << "\n";
    write_kv(out, prefix + "accepted_anchor_keys",
             episode.accepted_anchor_keys);
    write_kv(out, prefix + "realized_return_truth_id",
             episode.realized_return_truth_id);
    write_kv(out, prefix + "realized_return_projection_reference_node_id",
             episode.realized_return_projection_reference_node_id);
    write_kv(out, prefix + "realized_return_projection_edge_ids",
             episode.realized_return_projection_edge_ids);
    write_kv(out, prefix + "realized_return_projection_signs",
             episode.realized_return_projection_signs);
    out << prefix << "transition_count=" << episode.transition_count << "\n";
    out << prefix << "step_count=" << episode.step_reports.size() << "\n";
    out << prefix << "total_reward=" << episode.total_reward << "\n";
    out << prefix << "total_log_growth=" << episode.total_log_growth << "\n";
    out << prefix << "final_equity_base=" << episode.final_equity_base << "\n";
    out << prefix << "max_drawdown=" << episode.max_drawdown << "\n";
    out << prefix << "total_turnover=" << episode.total_turnover << "\n";
    out << prefix
        << "total_transaction_cost_base=" << episode.total_transaction_cost_base
        << "\n";
    out << prefix
        << "cajtucu_valid_trace_count=" << episode.cajtucu_valid_trace_count
        << "\n";
    out << prefix
        << "cajtucu_invalid_trace_count=" << episode.cajtucu_invalid_trace_count
        << "\n";
    out << prefix << "cajtucu_missing_direct_reserve_edge_count="
        << episode.cajtucu_missing_direct_reserve_edge_count << "\n";
    out << prefix << "cajtucu_nontradable_edge_reject_count="
        << episode.cajtucu_nontradable_edge_reject_count << "\n";
    out << prefix << "cajtucu_below_min_notional_reject_count="
        << episode.cajtucu_below_min_notional_reject_count << "\n";
    out << prefix << "cajtucu_above_max_notional_reject_count="
        << episode.cajtucu_above_max_notional_reject_count << "\n";
    out << prefix << "cajtucu_insufficient_reserve_reject_count="
        << episode.cajtucu_insufficient_reserve_reject_count << "\n";
    out << prefix << "cajtucu_insufficient_units_reject_count="
        << episode.cajtucu_insufficient_units_reject_count << "\n";
    out << prefix << "cajtucu_invalid_sell_price_count="
        << episode.cajtucu_invalid_sell_price_count << "\n";
    out << prefix << "cajtucu_large_equity_mismatch_count="
        << episode.cajtucu_large_equity_mismatch_count << "\n";
    out << prefix << "cajtucu_synthetic_market_step_count="
        << episode.cajtucu_synthetic_market_step_count << "\n";
    out << prefix << "requested_order_count=" << episode.requested_order_count
        << "\n";
    out << prefix << "executed_order_count=" << episode.executed_order_count
        << "\n";
    out << prefix << "rejected_order_count=" << episode.rejected_order_count
        << "\n";
    out << prefix << "partial_order_count=" << episode.partial_order_count
        << "\n";
    out << prefix
        << "requested_notional_base=" << episode.requested_notional_base
        << "\n";
    out << prefix << "executed_notional_base=" << episode.executed_notional_base
        << "\n";
    out << prefix << "rejected_notional_base=" << episode.rejected_notional_base
        << "\n";
    out << prefix << "partial_notional_base=" << episode.partial_notional_base
        << "\n";
    out << prefix << "fill_ratio=" << episode.fill_ratio << "\n";
    out << prefix << "fee_cost_base=" << episode.fee_cost_base << "\n";
    out << prefix << "spread_cost_base=" << episode.spread_cost_base << "\n";
    out << prefix << "slippage_cost_base=" << episode.slippage_cost_base
        << "\n";
    out << prefix
        << "cost_as_fraction_of_equity=" << episode.cost_as_fraction_of_equity
        << "\n";
    out << prefix << "cost_as_fraction_of_gross_return="
        << episode.cost_as_fraction_of_gross_return << "\n";
    out << prefix
        << "mean_target_weight_error_l1=" << episode.mean_target_weight_error_l1
        << "\n";
    out << prefix << "mean_target_weight_error_linf="
        << episode.mean_target_weight_error_linf << "\n";
    out << prefix << "mean_post_execution_risky_weight_sum="
        << episode.mean_post_execution_risky_weight_sum << "\n";
    out << prefix << "mean_post_execution_base_reserve_weight="
        << episode.mean_post_execution_base_reserve_weight << "\n";
    out << prefix
        << "reserve_shortfall_count=" << episode.reserve_shortfall_count
        << "\n";
    out << prefix
        << "mean_ledger_before_equity=" << episode.mean_ledger_before_equity
        << "\n";
    out << prefix << "mean_ledger_after_execution_equity="
        << episode.mean_ledger_after_execution_equity << "\n";
    out << prefix << "mean_ledger_after_realization_equity="
        << episode.mean_ledger_after_realization_equity << "\n";
    out << prefix << "max_ledger_equity_reconciliation_error="
        << episode.max_ledger_equity_reconciliation_error << "\n";
    out << prefix
        << "base_reserve_units_before=" << episode.base_reserve_units_before
        << "\n";
    out << prefix
        << "base_reserve_units_after=" << episode.base_reserve_units_after
        << "\n";
    out << prefix << "unit_nonnegativity_violation_count="
        << episode.unit_nonnegativity_violation_count << "\n";
    write_kv(out, prefix + "allocation_target_risky_node_weights",
             episode.allocation_target_risky_node_weights);
    write_kv(out, prefix + "allocation_reserve_node_id",
             episode.allocation_reserve_node_id);
    out << prefix
        << "allocation_reserve_weight=" << episode.allocation_reserve_weight
        << "\n";
    out << prefix << "allocation_turnover=" << episode.allocation_turnover
        << "\n";
    write_kv(out, prefix + "allocation_objective_terms",
             episode.allocation_objective_terms);
    out << prefix << "allocation_cvar_loss=" << episode.allocation_cvar_loss
        << "\n";
    out << prefix << "allocation_transaction_cost_estimate="
        << episode.allocation_transaction_cost_estimate << "\n";
    write_kv(out, prefix + "allocation_constraint_diagnostics",
             episode.allocation_constraint_diagnostics);
    write_kv(out, prefix + "allocation_cap_diagnostics",
             episode.allocation_cap_diagnostics);
    write_kv(out, prefix + "allocation_scenario_growth_floor_status",
             episode.allocation_scenario_growth_floor_status);
    write_kv(out, prefix + "allocation_fallback_reasons",
             episode.allocation_fallback_reasons);
    write_kv(out, prefix + "allocation_derisk_reasons",
             episode.allocation_derisk_reasons);
    out << prefix << "projection_mae=" << episode.projection_mae << "\n";
    out << prefix << "projection_rmse=" << episode.projection_rmse << "\n";
    out << prefix << "projection_signed_bias=" << episode.projection_signed_bias
        << "\n";
    out << prefix << "projection_correlation=" << episode.projection_correlation
        << "\n";
    out << prefix << "projection_directional_accuracy="
        << episode.projection_directional_accuracy << "\n";
    out << prefix << "projection_interval_coverage="
        << episode.projection_interval_coverage << "\n";
    out << prefix << "projection_mean_interval_width="
        << episode.projection_mean_interval_width << "\n";
    out << prefix
        << "zero_return_baseline_mae=" << episode.zero_return_baseline_mae
        << "\n";
    out << prefix
        << "zero_return_baseline_rmse=" << episode.zero_return_baseline_rmse
        << "\n";
    out << prefix << "zero_return_baseline_signed_bias="
        << episode.zero_return_baseline_signed_bias << "\n";
    out << prefix << "zero_return_baseline_directional_accuracy="
        << episode.zero_return_baseline_directional_accuracy << "\n";
    out << prefix
        << "model_skill_vs_zero_mae=" << episode.model_skill_vs_zero_mae
        << "\n";
    out << prefix
        << "model_skill_vs_zero_rmse=" << episode.model_skill_vs_zero_rmse
        << "\n";
    write_kv(out, prefix + "per_node_projection_mae",
             episode.per_node_projection_mae);
    write_kv(out, prefix + "per_node_projection_rmse",
             episode.per_node_projection_rmse);
    write_kv(out, prefix + "per_node_projection_signed_bias",
             episode.per_node_projection_signed_bias);
    write_kv(out, prefix + "per_node_projection_correlation",
             episode.per_node_projection_correlation);
    write_kv(out, prefix + "per_node_projection_directional_accuracy",
             episode.per_node_projection_directional_accuracy);
    write_kv(out, prefix + "per_node_projection_interval_coverage",
             episode.per_node_projection_interval_coverage);
    write_kv(out, prefix + "per_node_projection_mean_interval_width",
             episode.per_node_projection_mean_interval_width);
    write_kv(out, prefix + "per_node_realized_volatility",
             episode.per_node_realized_volatility);
    write_kv(out, prefix + "per_node_normalized_projection_mae",
             episode.per_node_normalized_projection_mae);
    write_kv(out, prefix + "per_node_normalized_projection_rmse",
             episode.per_node_normalized_projection_rmse);
    write_kv(out, prefix + "per_node_zero_return_baseline_mae",
             episode.per_node_zero_return_baseline_mae);
    write_kv(out, prefix + "per_node_zero_return_baseline_rmse",
             episode.per_node_zero_return_baseline_rmse);
    write_kv(out, prefix + "per_node_model_skill_vs_zero_mae",
             episode.per_node_model_skill_vs_zero_mae);
    write_kv(out, prefix + "per_node_model_skill_vs_zero_rmse",
             episode.per_node_model_skill_vs_zero_rmse);
    out << prefix << "warning_count=" << episode.warnings.size() << "\n";
    write_kv(out, prefix + "warnings", join_report_tokens(episode.warnings));
    out << prefix << "failure_count=" << episode.failures.size() << "\n";
    write_kv(out, prefix + "failures", join_report_tokens(episode.failures));

    for (std::size_t step_i = 0; step_i < episode.step_reports.size();
         ++step_i) {
      const auto &step = episode.step_reports[step_i];
      const auto step_prefix = prefix + "step_" + std::to_string(step_i) + "_";
      write_kv(out, step_prefix + "schema", step.step_artifact_schema_id);
      out << step_prefix << "step_index=" << step.step_index << "\n";
      write_kv(out, step_prefix + "episode_id", step.episode_id);
      write_kv(out, step_prefix + "protocol_id", step.protocol_id);
      write_kv(out, step_prefix + "runtime_run_id", step.runtime_run_id);
      write_kv(out, step_prefix + "environment_run_id",
               step.environment_run_id);
      write_kv(out, step_prefix + "policy_id", step.policy_id);
      write_kv(out, step_prefix + "method_id", step.method_id);
      write_kv(out, step_prefix + "policy_kind",
               policy_kind_name(step.policy_kind));
      write_kv(out, step_prefix + "world_mode",
               world_mode_name(step.world_mode));
      write_kv(out, step_prefix + "anchor_key", step.anchor_key);
      out << step_prefix
          << "observation_anchor_index=" << step.observation_anchor_index
          << "\n";
      out << step_prefix << "next_realization_anchor_index="
          << step.next_realization_anchor_index << "\n";
      out << step_prefix
          << "knowledge_timestamp_ms=" << step.knowledge_timestamp_ms << "\n";
      out << step_prefix << "realization_available_after_timestamp_ms="
          << step.realization_available_after_timestamp_ms << "\n";
      write_kv(out, step_prefix + "accepted_cursor_wave_id",
               step.accepted_cursor_wave_id);
      write_kv(out, step_prefix + "accepted_cursor_wave_mode",
               step.accepted_cursor_wave_mode);
      write_kv(out, step_prefix + "accepted_cursor_kind",
               step.accepted_cursor_kind);
      write_kv(out, step_prefix + "accepted_cursor_scope",
               step.accepted_cursor_scope);
      write_kv(out, step_prefix + "accepted_source_range_policy",
               step.accepted_source_range_policy);
      write_kv(out, step_prefix + "accepted_source_order_policy",
               step.accepted_source_order_policy);
      write_kv(out, step_prefix + "accepted_batch_cursor_token",
               step.accepted_batch_cursor_token);
      out << step_prefix
          << "accepted_anchor_index_begin=" << step.accepted_anchor_index_begin
          << "\n";
      out << step_prefix
          << "accepted_anchor_index_end=" << step.accepted_anchor_index_end
          << "\n";
      out << step_prefix
          << "accepted_cursor_offset=" << step.accepted_cursor_offset << "\n";
      write_kv(out, step_prefix + "action_schema_id", step.action_schema_id);
      out << step_prefix << "action_decision_timestamp_ms="
          << step.action_decision_timestamp_ms << "\n";
      write_kv(out, step_prefix + "target_risky_node_weights",
               step.target_risky_node_weights);
      write_kv(out, step_prefix + "target_base_reserve_node_id",
               step.target_base_reserve_node_id);
      out << step_prefix
          << "target_base_reserve_weight=" << step.target_base_reserve_weight
          << "\n";
      out << step_prefix
          << "portfolio_equity_before=" << step.portfolio_equity_before << "\n";
      out << step_prefix
          << "portfolio_equity_after=" << step.portfolio_equity_after << "\n";
      out << step_prefix << "realized_log_growth=" << step.realized_log_growth
          << "\n";
      out << step_prefix
          << "realized_arithmetic_return=" << step.realized_arithmetic_return
          << "\n";
      out << step_prefix
          << "transaction_cost_base=" << step.transaction_cost_base << "\n";
      out << step_prefix << "turnover=" << step.turnover << "\n";
      out << step_prefix
          << "invalid_action=" << (step.invalid_action ? "true" : "false")
          << "\n";
      out << step_prefix << "risk_gate_evaluated="
          << (step.risk_gate_evaluated ? "true" : "false") << "\n";
      out << step_prefix << "risk_gate_allow_trading="
          << (step.risk_gate_allow_trading ? "true" : "false") << "\n";
      out << step_prefix << "risk_gate_force_base_reserve_fallback="
          << (step.risk_gate_force_base_reserve_fallback ? "true" : "false")
          << "\n";
      out << step_prefix
          << "risk_gate_reason_count=" << step.risk_gate_reason_count << "\n";
      write_kv(out, step_prefix + "risk_gate_reasons", step.risk_gate_reasons);
      out << step_prefix
          << "risk_gate_warning_count=" << step.risk_gate_warning_count << "\n";
      write_kv(out, step_prefix + "risk_gate_warnings",
               step.risk_gate_warnings);
      out << step_prefix << "rebalance_plan_valid="
          << (step.rebalance_plan_valid ? "true" : "false") << "\n";
      write_kv(out, step_prefix + "rebalance_plan_source",
               step.rebalance_plan_source);
      out << step_prefix << "rebalance_plan_enforced="
          << (step.rebalance_plan_enforced ? "true" : "false") << "\n";
      write_kv(out, step_prefix + "execution_model", step.execution_model);
      out << step_prefix
          << "rebalance_order_count=" << step.rebalance_order_count << "\n";
      out << step_prefix
          << "rebalance_skipped_count=" << step.rebalance_skipped_count << "\n";
      out << step_prefix << "rebalance_requested_turnover_weight="
          << step.rebalance_requested_turnover_weight << "\n";
      out << step_prefix << "rebalance_routed_turnover_weight="
          << step.rebalance_routed_turnover_weight << "\n";
      out << step_prefix << "rebalance_estimated_transaction_cost_base="
          << step.rebalance_estimated_transaction_cost_base << "\n";
      out << step_prefix << "fill_count=" << step.fill_count << "\n";
      out << step_prefix
          << "fill_gross_notional_base=" << step.fill_gross_notional_base
          << "\n";
      out << step_prefix << "fill_fee_base=" << step.fill_fee_base << "\n";
      out << step_prefix << "cajtucu_execution_trace_available="
          << (step.cajtucu_execution_trace_available ? "true" : "false")
          << "\n";
      write_kv(out, step_prefix + "cajtucu_backend_id",
               step.cajtucu_backend_id);
      write_kv(out, step_prefix + "cajtucu_trace_id", step.cajtucu_trace_id);
      write_kv(out, step_prefix + "cajtucu_market_source_id",
               step.cajtucu_market_source_id);
      out << step_prefix << "cajtucu_synthetic_direct_edges="
          << (step.cajtucu_synthetic_direct_edges ? "true" : "false") << "\n";
      out << step_prefix << "cajtucu_trace_valid="
          << (step.cajtucu_trace_valid ? "true" : "false") << "\n";
      out << step_prefix
          << "cajtucu_failure_count=" << step.cajtucu_failure_count << "\n";
      out << step_prefix << "cajtucu_ledger_intent_equity_difference_base="
          << step.cajtucu_ledger_intent_equity_difference_base << "\n";
      out << step_prefix << "cajtucu_ledger_intent_equity_mismatch="
          << (step.cajtucu_ledger_intent_equity_mismatch ? "true" : "false")
          << "\n";
      out << step_prefix << "cajtucu_order_count=" << step.cajtucu_order_count
          << "\n";
      out << step_prefix << "cajtucu_executed_order_count="
          << step.cajtucu_executed_order_count << "\n";
      out << step_prefix << "cajtucu_fill_count=" << step.cajtucu_fill_count
          << "\n";
      out << step_prefix
          << "cajtucu_rejected_fill_count=" << step.cajtucu_rejected_fill_count
          << "\n";
      out << step_prefix
          << "cajtucu_partial_fill_count=" << step.cajtucu_partial_fill_count
          << "\n";
      out << step_prefix << "cajtucu_missing_direct_reserve_edge_count="
          << step.cajtucu_missing_direct_reserve_edge_count << "\n";
      out << step_prefix << "cajtucu_nontradable_edge_reject_count="
          << step.cajtucu_nontradable_edge_reject_count << "\n";
      out << step_prefix << "cajtucu_below_min_notional_reject_count="
          << step.cajtucu_below_min_notional_reject_count << "\n";
      out << step_prefix << "cajtucu_above_max_notional_reject_count="
          << step.cajtucu_above_max_notional_reject_count << "\n";
      out << step_prefix << "cajtucu_insufficient_reserve_reject_count="
          << step.cajtucu_insufficient_reserve_reject_count << "\n";
      out << step_prefix << "cajtucu_insufficient_units_reject_count="
          << step.cajtucu_insufficient_units_reject_count << "\n";
      out << step_prefix << "cajtucu_invalid_sell_price_count="
          << step.cajtucu_invalid_sell_price_count << "\n";
      out << step_prefix << "cajtucu_large_equity_mismatch_count="
          << step.cajtucu_large_equity_mismatch_count << "\n";
      out << step_prefix << "cajtucu_requested_notional_base="
          << step.cajtucu_requested_notional_base << "\n";
      out << step_prefix << "cajtucu_executed_notional_base="
          << step.cajtucu_executed_notional_base << "\n";
      out << step_prefix << "cajtucu_rejected_notional_base="
          << step.cajtucu_rejected_notional_base << "\n";
      out << step_prefix << "cajtucu_partial_notional_base="
          << step.cajtucu_partial_notional_base << "\n";
      out << step_prefix << "cajtucu_fill_ratio=" << step.cajtucu_fill_ratio
          << "\n";
      out << step_prefix
          << "cajtucu_total_fee_base=" << step.cajtucu_total_fee_base << "\n";
      out << step_prefix << "cajtucu_total_spread_cost_base="
          << step.cajtucu_total_spread_cost_base << "\n";
      out << step_prefix
          << "cajtucu_total_slippage_base=" << step.cajtucu_total_slippage_base
          << "\n";
      out << step_prefix << "cajtucu_total_transaction_cost_base="
          << step.cajtucu_total_transaction_cost_base << "\n";
      out << step_prefix
          << "target_weight_error_l1=" << step.target_weight_error_l1 << "\n";
      out << step_prefix
          << "target_weight_error_linf=" << step.target_weight_error_linf
          << "\n";
      out << step_prefix << "post_execution_risky_weight_sum="
          << step.post_execution_risky_weight_sum << "\n";
      out << step_prefix << "post_execution_base_reserve_weight="
          << step.post_execution_base_reserve_weight << "\n";
      out << step_prefix
          << "reserve_shortfall_count=" << step.reserve_shortfall_count << "\n";
      out << step_prefix << "ledger_before_equity=" << step.ledger_before_equity
          << "\n";
      out << step_prefix << "ledger_after_execution_equity="
          << step.ledger_after_execution_equity << "\n";
      out << step_prefix << "ledger_after_realization_equity="
          << step.ledger_after_realization_equity << "\n";
      out << step_prefix << "ledger_equity_reconciliation_error="
          << step.ledger_equity_reconciliation_error << "\n";
      out << step_prefix
          << "base_reserve_units_before=" << step.base_reserve_units_before
          << "\n";
      out << step_prefix
          << "base_reserve_units_after=" << step.base_reserve_units_after
          << "\n";
      out << step_prefix << "unit_nonnegativity_violation_count="
          << step.unit_nonnegativity_violation_count << "\n";
      out << step_prefix << "reward_log_growth=" << step.reward_log_growth
          << "\n";
      out << step_prefix
          << "reward_drawdown_penalty=" << step.reward_drawdown_penalty << "\n";
      out << step_prefix << "reward_transaction_cost_penalty="
          << step.reward_transaction_cost_penalty << "\n";
      out << step_prefix
          << "reward_turnover_penalty=" << step.reward_turnover_penalty << "\n";
      out << step_prefix << "reward_invalid_action_penalty="
          << step.reward_invalid_action_penalty << "\n";
      out << step_prefix << "reward_total=" << step.reward_total << "\n";
      out << step_prefix << "projection_mae=" << step.projection_mae << "\n";
      out << step_prefix << "projection_rmse=" << step.projection_rmse << "\n";
      out << step_prefix
          << "projection_signed_bias=" << step.projection_signed_bias << "\n";
      out << step_prefix
          << "projection_correlation=" << step.projection_correlation << "\n";
      out << step_prefix << "projection_directional_accuracy="
          << step.projection_directional_accuracy << "\n";
      out << step_prefix << "projection_interval_coverage="
          << step.projection_interval_coverage << "\n";
      out << step_prefix << "projection_mean_interval_width="
          << step.projection_mean_interval_width << "\n";
      out << step_prefix
          << "zero_return_baseline_mae=" << step.zero_return_baseline_mae
          << "\n";
      out << step_prefix
          << "zero_return_baseline_rmse=" << step.zero_return_baseline_rmse
          << "\n";
      out << step_prefix << "zero_return_baseline_signed_bias="
          << step.zero_return_baseline_signed_bias << "\n";
      out << step_prefix << "zero_return_baseline_directional_accuracy="
          << step.zero_return_baseline_directional_accuracy << "\n";
      out << step_prefix
          << "model_skill_vs_zero_mae=" << step.model_skill_vs_zero_mae << "\n";
      out << step_prefix
          << "model_skill_vs_zero_rmse=" << step.model_skill_vs_zero_rmse
          << "\n";
      out << step_prefix << "residual_quality_available="
          << (step.residual_quality_available ? "true" : "false") << "\n";
      out << step_prefix << "residual_quality_valid="
          << (step.residual_quality_valid ? "true" : "false") << "\n";
      out << step_prefix << "residual_mean_residual_energy="
          << step.residual_mean_residual_energy << "\n";
      out << step_prefix << "residual_max_residual_energy="
          << step.residual_max_residual_energy << "\n";
      write_kv(out, step_prefix + "decision_report_schema_id",
               step.decision_report_schema_id);
      out << step_prefix
          << "decision_report_text_bytes=" << step.decision_report_text_bytes
          << "\n";
      out << step_prefix << "warning_count=" << step.warning_count << "\n";
      write_kv(out, step_prefix + "warnings", step.warnings);
      out << step_prefix << "failure_count=" << step.failure_count << "\n";
      write_kv(out, step_prefix + "failures", step.failures);
    }
  }

  out << "warning_count=" << report.warnings.size() << "\n";
  for (std::size_t i = 0; i < report.warnings.size(); ++i) {
    write_kv(out, "warning_" + std::to_string(i), report.warnings[i]);
  }
  out << "failure_count=" << report.failures.size() << "\n";
  for (std::size_t i = 0; i < report.failures.size(); ++i) {
    write_kv(out, "failure_" + std::to_string(i), report.failures[i]);
  }

  runtime_layout::write_text_file_atomically(path, out.str());
}

} // namespace replay_driver_detail

[[nodiscard]] inline std::filesystem::path
runtime_replay_experiment_index_path(const std::filesystem::path &job_dir) {
  return replay::runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiments.index";
}

[[nodiscard]] inline std::filesystem::path
runtime_replay_experiment_report_path_for_index(
    const runtime_replay_experiment_index_entry_t &entry,
    const std::filesystem::path &index_dir) {
  if (entry.report_path.empty()) {
    return {};
  }
  if (entry.report_path.is_absolute() || index_dir.empty()) {
    return entry.report_path;
  }
  return index_dir / entry.report_path;
}

inline void validate_runtime_replay_experiment_index_report_binding(
    const runtime_replay_experiment_index_entry_t &entry,
    const std::filesystem::path &resolved_report_path) {
  namespace layout = replay_driver_detail::runtime_layout;
  const auto report_text =
      replay_driver_detail::read_text_file_or_throw(resolved_report_path);
  const auto actual_report_digest =
      replay_driver_detail::replay_report_digest_for_text(report_text);
  if (entry.report_digest.empty() ||
      entry.report_digest != actual_report_digest) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] experiment index report_digest must match "
        "replay report content");
  }
  const auto kv = layout::parse_kv_file(resolved_report_path);
  if (kv.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] experiment index report_path could not be "
        "read as replay report evidence");
  }
  if (layout::map_get(kv, "schema") != kReplayExperimentArtifactSchema) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] experiment index report schema mismatch");
  }
  const auto require_match = [&](const std::string &field,
                                 const std::string &expected) {
    const auto actual = layout::map_get(kv, field);
    if (actual != expected) {
      throw std::runtime_error("[runtime_job_replay_driver] experiment index " +
                               field + " does not match replay report");
    }
  };
  require_match("experiment_id", entry.experiment_id);
  require_match("runtime_run_id", entry.runtime_run_id);
  require_match("environment_run_id", entry.environment_run_id);

  const auto report_bundle_count =
      replay_driver_detail::parse_size_or(kv, "replay_bundle_count");
  const auto report_policy_count =
      replay_driver_detail::parse_size_or(kv, "policy_summary_count");
  const auto report_attempted_count =
      replay_driver_detail::parse_size_or(kv, "attempted_count");
  const auto report_completed_count =
      replay_driver_detail::parse_size_or(kv, "completed_count");
  const auto report_failed_count = replay_driver_detail::parse_size_or(
      kv, "failed_count",
      replay_driver_detail::parse_size_or(kv, "failure_count"));
  const auto report_episode_count =
      replay_driver_detail::parse_size_or(kv, "episode_count");
  if (report_bundle_count != entry.replay_bundle_count ||
      report_policy_count != entry.policy_count ||
      report_attempted_count != entry.attempted_count ||
      report_completed_count != entry.completed_count ||
      report_failed_count != entry.attempted_count - entry.completed_count ||
      report_episode_count != entry.completed_count) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] experiment index counts do not match "
        "replay report evidence");
  }
}

inline void validate_runtime_replay_experiment_index_entries(
    const std::vector<runtime_replay_experiment_index_entry_t> &entries,
    const std::filesystem::path &index_dir = {}) {
  std::unordered_set<std::string> seen_experiment_ids;
  seen_experiment_ids.reserve(entries.size());
  for (const auto &entry : entries) {
    if (entry.experiment_id.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index entry missing "
          "experiment_id");
    }
    if (!seen_experiment_ids.insert(entry.experiment_id).second) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] duplicate experiment index id: " +
          entry.experiment_id);
    }
    if (entry.runtime_run_id.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index entry missing "
          "runtime_run_id");
    }
    if (entry.environment_run_id.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index entry missing "
          "environment_run_id");
    }
    if (entry.report_path.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index entry missing "
          "report_path");
    }
    if (cuwacunu::kikijyeba::environment::detail::
            path_contains_parent_reference(entry.report_path)) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index report_path must not "
          "contain parent-directory components");
    }
    if (entry.report_digest.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index entry missing "
          "report_digest");
    }
    const auto resolved_report_path =
        runtime_replay_experiment_report_path_for_index(entry, index_dir);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(resolved_report_path, ec) || ec) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index report_path must point "
          "to an existing replay report file");
    }
    validate_runtime_replay_experiment_index_report_binding(
        entry, resolved_report_path);
    if (entry.policy_count == 0 || entry.replay_bundle_count == 0) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index policy and bundle "
          "counts must be positive");
    }
    if (entry.replay_bundle_count >
        std::numeric_limits<std::size_t>::max() / entry.policy_count) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index attempted count "
          "overflow");
    }
    const auto expected_attempts =
        entry.policy_count * entry.replay_bundle_count;
    if (entry.attempted_count != expected_attempts) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index attempted_count must "
          "match policy_count * replay_bundle_count");
    }
    if (entry.completed_count > entry.attempted_count) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] experiment index completed_count cannot "
          "exceed attempted_count");
    }
  }
}

inline void write_runtime_replay_experiment_index_at(
    const std::filesystem::path &index_path,
    const std::vector<runtime_replay_experiment_index_entry_t> &entries) {
  if (index_path.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] runtime replay experiment index path is "
        "required");
  }
  if (entries.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] runtime replay experiment index entries "
        "are required");
  }
  validate_runtime_replay_experiment_index_entries(entries,
                                                   index_path.parent_path());
  std::ostringstream out;
  replay_driver_detail::write_kv(out, "schema",
                                 kRuntimeReplayExperimentIndexSchema);
  out << "entry_count=" << entries.size() << "\n";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    const auto prefix = std::string("entry_") + std::to_string(i) + "_";
    replay_driver_detail::write_kv(out, prefix + "experiment_id",
                                   entry.experiment_id);
    replay_driver_detail::write_kv(out, prefix + "runtime_run_id",
                                   entry.runtime_run_id);
    replay_driver_detail::write_kv(out, prefix + "environment_run_id",
                                   entry.environment_run_id);
    out << prefix << "policy_count=" << entry.policy_count << "\n";
    out << prefix << "replay_bundle_count=" << entry.replay_bundle_count
        << "\n";
    out << prefix << "attempted_count=" << entry.attempted_count << "\n";
    out << prefix << "completed_count=" << entry.completed_count << "\n";
    replay_driver_detail::write_kv(out, prefix + "report_path",
                                   entry.report_path.generic_string());
    replay_driver_detail::write_kv(out, prefix + "report_digest",
                                   entry.report_digest);
  }
  replay_driver_detail::runtime_layout::write_text_file_atomically(index_path,
                                                                   out.str());
}

inline void write_runtime_replay_experiment_index(
    const std::filesystem::path &job_dir,
    const std::vector<runtime_replay_experiment_index_entry_t> &entries) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] job_dir is required for runtime replay "
        "experiment index");
  }
  write_runtime_replay_experiment_index_at(
      runtime_replay_experiment_index_path(job_dir), entries);
}

[[nodiscard]] inline std::vector<runtime_replay_experiment_index_entry_t>
read_runtime_replay_experiment_index_at(
    const std::filesystem::path &index_path) {
  namespace layout = replay_driver_detail::runtime_layout;
  if (index_path.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] runtime replay experiment index path is "
        "required");
  }
  const auto kv = layout::parse_kv_file(index_path);
  if (kv.empty()) {
    return {};
  }
  if (layout::map_get(kv, "schema") != kRuntimeReplayExperimentIndexSchema) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] runtime replay experiment index schema "
        "mismatch");
  }
  const auto entry_count = replay_driver_detail::parse_size_or(
      kv, "entry_count", static_cast<std::size_t>(0));
  if (entry_count == 0) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] runtime replay experiment index entries "
        "are required");
  }
  std::vector<runtime_replay_experiment_index_entry_t> entries;
  entries.reserve(entry_count);
  for (std::size_t i = 0; i < entry_count; ++i) {
    const auto prefix = std::string("entry_") + std::to_string(i) + "_";
    runtime_replay_experiment_index_entry_t entry{};
    entry.experiment_id = layout::map_get(kv, prefix + "experiment_id");
    entry.runtime_run_id = layout::map_get(kv, prefix + "runtime_run_id");
    entry.environment_run_id =
        layout::map_get(kv, prefix + "environment_run_id");
    entry.policy_count =
        replay_driver_detail::parse_size_or(kv, prefix + "policy_count");
    entry.replay_bundle_count =
        replay_driver_detail::parse_size_or(kv, prefix + "replay_bundle_count");
    entry.attempted_count =
        replay_driver_detail::parse_size_or(kv, prefix + "attempted_count");
    entry.completed_count =
        replay_driver_detail::parse_size_or(kv, prefix + "completed_count");
    const std::filesystem::path raw_report_path =
        layout::map_get(kv, prefix + "report_path");
    if (cuwacunu::kikijyeba::environment::detail::
            path_contains_parent_reference(raw_report_path)) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] runtime replay experiment index "
          "report_path must not contain parent-directory components");
    }
    entry.report_path = replay::resolve_runtime_replay_path(
        raw_report_path, index_path.parent_path());
    entry.report_digest = layout::map_get(kv, prefix + "report_digest");
    if (entry.runtime_run_id.empty() && !entry.report_path.empty()) {
      const auto report_kv = layout::parse_kv_file(entry.report_path);
      entry.runtime_run_id = layout::map_get(report_kv, "runtime_run_id");
    }
    if (entry.report_digest.empty() && !entry.report_path.empty()) {
      entry.report_digest = replay_driver_detail::replay_report_digest_for_path(
          entry.report_path);
    }
    if (entry.experiment_id.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] runtime replay experiment index entry "
          "missing experiment_id");
    }
    if (entry.report_path.empty()) {
      throw std::runtime_error(
          "[runtime_job_replay_driver] runtime replay experiment index entry "
          "missing report_path");
    }
    entries.push_back(std::move(entry));
  }
  validate_runtime_replay_experiment_index_entries(entries);
  return entries;
}

[[nodiscard]] inline std::vector<runtime_replay_experiment_index_entry_t>
read_runtime_replay_experiment_index(const std::filesystem::path &job_dir) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[runtime_job_replay_driver] job_dir is required for runtime replay "
        "experiment index");
  }
  return read_runtime_replay_experiment_index_at(
      runtime_replay_experiment_index_path(job_dir));
}

inline void append_runtime_replay_experiment_index_entry(
    const std::filesystem::path &job_dir,
    runtime_replay_experiment_index_entry_t entry) {
  auto entries = read_runtime_replay_experiment_index(job_dir);
  entries.push_back(std::move(entry));
  write_runtime_replay_experiment_index(job_dir, entries);
}

[[nodiscard]] inline runtime_job_replay_driver_result_t
run_runtime_job_replay_experiment(runtime_job_replay_driver_options_t options) {
  if (options.job_dir.empty()) {
    throw std::runtime_error("[runtime_job_replay_driver] job_dir is required");
  }
  const auto evidence =
      replay::read_runtime_replay_job_evidence(options.job_dir);
  auto config_path =
      replay_driver_detail::resolve_driver_config_path(options, evidence);
  auto contract = replay_driver_detail::protocol::
      load_channel_graph_first_protocol_contract_from_config(config_path);
  replay_driver_detail::validate_driver_replay_environment_contract(
      contract.replay_environment, options);
  auto market_graph = contract.source_plan.market_graph;
  auto base_spec = replay_driver_detail::make_driver_base_spec(
      contract, market_graph, options);
  if (base_spec.environment_run_id.empty()) {
    base_spec.environment_run_id =
        evidence.manifest.job_id + ".environment_replay";
  }

  auto source =
      replay::make_runtime_graph_anchor_replay_bundle_source_from_job_dir<
          std::int64_t>(
          evidence.manifest.job_id + ".runtime_replay_bundle_source",
          options.job_dir, market_graph, base_spec, options.frame_options,
          options.world_options, options.artifact_options,
          options.require_observation_artifacts);
  const auto replay_bundle_count = source.size();

  const std::string experiment_id =
      options.experiment_id.empty()
          ? evidence.manifest.job_id + ".runtime_replay_experiment"
          : options.experiment_id;
  auto factories = replay_driver_detail::make_driver_policy_factories(
      base_spec, contract.spot_distributional_utility, options);
  auto report = run_replay_experiment(experiment_id, source, factories,
                                      options.experiment_options);
  report.execution_profile_digest = options.execution_profile_digest;
  report.policy_set_digest = options.policy_set_digest;

  std::filesystem::path report_path{};
  std::filesystem::path experience_trace_path{};
  std::filesystem::path experiment_index_path{};
  if (options.write_report) {
    report_path = replay_driver_detail::normalize_report_path(
        options.report_path.empty()
            ? replay_driver_detail::default_report_path(options.job_dir)
            : options.report_path);
    replay_driver_detail::write_replay_experiment_report(
        report, report_path, config_path, replay_bundle_count,
        contract.replay_environment);
    if (options.write_experience_trace) {
      experience_trace_path = replay_driver_detail::normalize_report_path(
          options.experience_trace_path.empty()
              ? replay_driver_detail::default_experience_trace_path_for_report(
                    report_path)
              : options.experience_trace_path);
      output::write_experience_trace_report(
          output::make_experience_trace(report), experience_trace_path);
    }
    append_runtime_replay_experiment_index_entry(
        options.job_dir,
        {.experiment_id = report.experiment_id,
         .runtime_run_id = report.runtime_run_id,
         .environment_run_id = base_spec.environment_run_id,
         .policy_count = factories.size(),
         .replay_bundle_count = replay_bundle_count,
         .attempted_count = report.attempted_count,
         .completed_count = report.completed_count,
         .report_path = report_path,
         .report_digest =
             replay_driver_detail::replay_report_digest_for_path(report_path)});
    experiment_index_path =
        runtime_replay_experiment_index_path(options.job_dir);
  }

  return {
      .report = std::move(report),
      .report_path = std::move(report_path),
      .experience_trace_path = std::move(experience_trace_path),
      .experiment_index_path = std::move(experiment_index_path),
      .config_path = std::move(config_path),
      .replay_bundle_count = replay_bundle_count,
  };
}

} // namespace cuwacunu::kikijyeba::environment
