// SPDX-License-Identifier: MIT

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "hero/config_path_defaults.h"
#include "hero/runtime_hero/runtime/job_runner.h"
#include "kikijyeba/environment/runtime/experiment_driver.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "ujcamei/source/registry/types/data.h"

namespace env = cuwacunu::kikijyeba::environment;
namespace protocol = cuwacunu::kikijyeba::protocol;
namespace runtime = cuwacunu::hero::runtime;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

void print_usage(const char *argv0) {
  const auto default_config_path =
      cuwacunu::hero::config_paths::default_global_config_path(argv0).string();
  std::cerr << "usage: " << argv0
            << " [--config PATH] [--job-dir PATH] [--dry-run]\n"
            << "       [--replay-from-job-dir PATH]\n"
            << "       [--force-rebuild-cache]\n"
            << "       [--source-range all|anchor_index|source_key]\n"
            << "       [--anchor-index-begin N] [--anchor-index-end N]\n"
            << "       [--source-key-begin K] [--source-key-end K]\n"
            << "       [--runtime-handoff-id ID]\n"
            << "       [--runtime-handoff-digest DIGEST]\n"
            << "       [--marshal-target-driver-run-id ID]\n"
            << "       [--input-representation-checkpoint PATH]\n"
            << "       [--input-mdn-checkpoint PATH]\n"
            << "       [--no-replay-artifacts]\n"
            << "       [--replay-accounting-numeraire-node NODE]\n"
            << "       [--replay-target-nodes CSV]\n"
            << "       [--replay-experiment-id ID]\n"
            << "       [--replay-report-path PATH]\n"
            << "       [--replay-initial-equity-numeraire VALUE]\n"
            << "       [--replay-max-node-weight VALUE]\n"
            << "       [--replay-max-turnover-l1 VALUE]\n"
            << "       [--replay-max-steps N]\n"
            << "       [--replay-max-parallel-jobs N]\n"
            << "       [--replay-linear-transaction-cost-rate VALUE]\n"
            << "       [--replay-execution-profile-digest DIGEST]\n"
            << "       [--replay-policy-set-digest DIGEST]\n"
            << "       [--replay-allow-synthetic-direct-edges]\n"
            << "       [--replay-include-equal-weight]\n"
            << "       [--replay-include-current-weight]\n"
            << "       [--replay-include-graph-node-allocation-policy]\n"
            << "       [--replay-on-policy-sample]\n"
            << "       [--replay-action-distribution-id ID]\n"
            << "       [--replay-policy-artifact-digest DIGEST]\n"
            << "       [--replay-policy-checkpoint-path PATH]\n"
            << "       [--replay-causal-schedule-digest DIGEST]\n"
            << "       [--replay-snapshot-family-digest DIGEST]\n"
            << "       [--replay-no-numeraire-only-policy]\n"
            << "       [--replay-no-sdu-policy]\n"
            << "default config: " << default_config_path << "\n"
            << "default accounting numeraire: "
               "[ACCOUNTING].accounting_numeraire_node_id from .config "
               "(override with --replay-accounting-numeraire-node for "
               "debugging/recovery).\n"
            << "warning: direct non-dry-run or replay launches are for "
               "debugging/recovery. They do not prove Marshal validated "
               "Lattice advice, Runtime policy, active wave/replay bounds, "
               "or execution-profile identity, so resulting artifacts may be "
               "unsuitable for readiness claims. Prefer hero.marshal.* or "
               "hero.runtime.* for normal operator use.\n";
}

[[nodiscard]] std::string require_next_arg(int argc, char **argv, int *index,
                                           const std::string &flag) {
  if (*index + 1 >= argc) {
    throw std::runtime_error("[cuwacunu_exec] missing value for " + flag);
  }
  ++(*index);
  return argv[*index];
}

[[nodiscard]] std::size_t parse_size_arg(const std::string &value,
                                         const std::string &flag) {
  std::size_t consumed = 0;
  const unsigned long long parsed = std::stoull(value, &consumed);
  if (consumed != value.size() ||
      parsed > static_cast<unsigned long long>(
                   std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error("[cuwacunu_exec] invalid value for " + flag);
  }
  return static_cast<std::size_t>(parsed);
}

[[nodiscard]] std::int64_t parse_i64_arg(const std::string &value,
                                         const std::string &flag) {
  std::size_t consumed = 0;
  const long long parsed = std::stoll(value, &consumed);
  if (consumed != value.size()) {
    throw std::runtime_error("[cuwacunu_exec] invalid value for " + flag);
  }
  return static_cast<std::int64_t>(parsed);
}

[[nodiscard]] double parse_double_arg(const std::string &value,
                                      const std::string &flag) {
  std::size_t consumed = 0;
  const double parsed = std::stod(value, &consumed);
  if (consumed != value.size()) {
    throw std::runtime_error("[cuwacunu_exec] invalid value for " + flag);
  }
  return parsed;
}

[[nodiscard]] std::vector<std::string> parse_csv_arg(std::string value) {
  std::vector<std::string> out;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, ',')) {
    const auto first = item.find_first_not_of(" \t\r\n");
    const auto last = item.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) {
      continue;
    }
    out.push_back(item.substr(first, last - first + 1U));
  }
  return out;
}

void print_replay_result(
    const env::runtime_job_replay_driver_result_t &result) {
  std::cout << "replay_experiment_id=" << result.report.experiment_id << "\n";
  std::cout << "replay_config_path=" << result.config_path << "\n";
  std::cout << "replay_bundle_count=" << result.replay_bundle_count << "\n";
  std::cout << "replay_attempted_count=" << result.report.attempted_count
            << "\n";
  std::cout << "replay_completed_count=" << result.report.completed_count
            << "\n";
  std::cout << "replay_mean_total_reward=" << result.report.mean_total_reward()
            << "\n";
  std::cout << "replay_mean_total_log_growth="
            << result.report.mean_total_log_growth() << "\n";
  std::cout << "replay_mean_final_equity_numeraire="
            << result.report.mean_final_equity_numeraire() << "\n";
  if (!result.report_path.empty()) {
    std::cout << "replay_report_path=" << result.report_path.string() << "\n";
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string default_config_path =
        cuwacunu::hero::config_paths::default_global_config_path(argv[0])
            .string();
    std::string config_path{default_config_path};
    runtime::job_runner_options_t options{};
    env::runtime_job_replay_driver_options_t replay_options{};
    bool replay_job_dir_mode{false};

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        print_usage(argv[0]);
        return 0;
      }
      if (arg == "--config") {
        config_path = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-from-job-dir") {
        replay_options.job_dir = require_next_arg(argc, argv, &i, arg);
        replay_job_dir_mode = true;
      } else if (arg == "--job-dir") {
        options.job_dir = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--dry-run") {
        options.dry_run = true;
      } else if (arg == "--force-rebuild-cache") {
        options.force_rebuild_cache = true;
      } else if (arg == "--source-range") {
        options.source_range_override_enabled = true;
        options.source_range_override = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--anchor-index-begin") {
        options.source_range_override_enabled = true;
        options.anchor_index_begin_override =
            parse_size_arg(require_next_arg(argc, argv, &i, arg), arg);
        if (options.source_range_override.empty()) {
          options.source_range_override = "anchor_index";
        }
      } else if (arg == "--anchor-index-end") {
        options.source_range_override_enabled = true;
        options.anchor_index_end_override =
            parse_size_arg(require_next_arg(argc, argv, &i, arg), arg);
        if (options.source_range_override.empty()) {
          options.source_range_override = "anchor_index";
        }
      } else if (arg == "--source-key-begin") {
        options.source_range_override_enabled = true;
        options.source_key_begin_override =
            parse_i64_arg(require_next_arg(argc, argv, &i, arg), arg);
        if (options.source_range_override.empty()) {
          options.source_range_override = "source_key";
        }
      } else if (arg == "--source-key-end") {
        options.source_range_override_enabled = true;
        options.source_key_end_override =
            parse_i64_arg(require_next_arg(argc, argv, &i, arg), arg);
        if (options.source_range_override.empty()) {
          options.source_range_override = "source_key";
        }
      } else if (arg == "--runtime-handoff-id") {
        options.runtime_handoff_id = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--runtime-handoff-digest") {
        options.runtime_handoff_digest = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--marshal-target-driver-run-id") {
        options.marshal_target_driver_run_id =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--input-representation-checkpoint") {
        options.input_representation_checkpoint_path =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--input-mdn-checkpoint") {
        options.input_mdn_checkpoint_path =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--no-replay-artifacts") {
        options.write_replay_artifacts = false;
      } else if (arg == "--replay-accounting-numeraire-node") {
        options.replay_accounting_numeraire_node_id =
            require_next_arg(argc, argv, &i, arg);
        replay_options.accounting_numeraire_node_id =
            options.replay_accounting_numeraire_node_id;
      } else if (arg == "--replay-target-nodes") {
        options.replay_target_node_ids =
            parse_csv_arg(require_next_arg(argc, argv, &i, arg));
        replay_options.target_node_ids = options.replay_target_node_ids;
      } else if (arg == "--replay-experiment-id") {
        replay_options.experiment_id = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-report-path") {
        replay_options.report_path = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-initial-equity-numeraire") {
        replay_options.initial_equity_numeraire =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-max-node-weight") {
        replay_options.max_node_weight =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-max-turnover-l1") {
        replay_options.max_turnover_l1 =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-max-steps") {
        replay_options.experiment_options.episode_options.max_steps =
            parse_size_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-max-parallel-jobs") {
        replay_options.experiment_options.max_parallel_jobs =
            parse_size_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-linear-transaction-cost-rate") {
        replay_options.world_options.linear_transaction_cost_rate =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-execution-profile-digest") {
        replay_options.execution_profile_digest =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-policy-set-digest") {
        replay_options.policy_set_digest =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-allow-synthetic-direct-edges") {
        replay_options.world_options.paper_execution_options
            .allow_synthetic_direct_edges = true;
      } else if (arg == "--replay-include-equal-weight") {
        replay_options.include_equal_weight_policy = true;
      } else if (arg == "--replay-include-current-weight") {
        replay_options.include_current_weight_policy = true;
      } else if (arg == "--replay-include-graph-node-allocation-policy") {
        replay_options.include_graph_node_allocation_policy = true;
      } else if (arg == "--replay-on-policy-sample") {
        replay_options.experiment_options.episode_options.policy_action_mode =
            env::episode_policy_action_mode_t::on_policy_sample;
      } else if (arg == "--replay-action-distribution-id") {
        replay_options.graph_node_allocation_action_distribution_id =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-policy-artifact-digest") {
        replay_options.graph_node_allocation_policy_artifact_digest =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-policy-checkpoint-path") {
        replay_options.graph_node_allocation_policy_checkpoint_path =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-causal-schedule-digest") {
        replay_options.causal_schedule_digest =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-snapshot-family-digest") {
        replay_options.snapshot_family_digest =
            require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-no-numeraire-only-policy") {
        replay_options.include_numeraire_only_policy = false;
      } else if (arg == "--replay-no-sdu-policy") {
        replay_options.include_spot_distributional_utility_policy = false;
      } else {
        throw std::runtime_error("[cuwacunu_exec] unknown argument: " + arg);
      }
    }

    if (replay_job_dir_mode) {
      replay_options.accounting_numeraire_node_id =
          protocol::resolve_accounting_numeraire_node_id(
              replay_options.accounting_numeraire_node_id, config_path);
      options.replay_accounting_numeraire_node_id =
          replay_options.accounting_numeraire_node_id;
      if (!options.job_dir.empty()) {
        throw std::runtime_error(
            "[cuwacunu_exec] --job-dir writes a new Runtime job and cannot be "
            "combined with --replay-from-job-dir");
      }
      if (options.dry_run || options.force_rebuild_cache ||
          options.source_range_override_enabled) {
        throw std::runtime_error(
            "[cuwacunu_exec] replay-from-job mode cannot be combined with "
            "Runtime launch-only flags");
      }
      replay_options.config_path =
          (config_path == default_config_path) ? std::string{} : config_path;
      std::cerr
          << "[cuwacunu_exec] WARNING: direct --replay-from-job-dir launch is "
             "for debugging/recovery. It does not prove Marshal validated "
             "rollout bounds, Lattice advice/readiness context, Runtime "
             "policy, or Cajtucu execution-profile identity. Resulting replay "
             "artifacts may be unsuitable for readiness claims. Prefer "
             "hero.marshal.rollout or hero.runtime.run operation=replay for "
             "operator evidence.\n";
      const auto replay_result =
          env::run_runtime_job_replay_experiment(replay_options);
      print_replay_result(replay_result);
      return 0;
    }

    if (options.runtime_handoff_id.empty() !=
        options.runtime_handoff_digest.empty()) {
      throw std::runtime_error(
          "[cuwacunu_exec] --runtime-handoff-id and --runtime-handoff-digest "
          "must be supplied together");
    }
    if (!options.dry_run && options.runtime_handoff_id.empty()) {
      std::cerr
          << "[cuwacunu_exec] WARNING: direct non-dry-run launch without "
             "Runtime/Marshal handoff identity is for debugging/recovery. It "
             "does not prove Marshal validated Lattice advice, Runtime policy, "
             "active wave bounds, checkpoint/source identity, or handoff "
             "lineage. Resulting artifacts may be unsuitable for readiness "
             "claims. Prefer hero.marshal.prepare -> hero.runtime.run "
             "operation=wave for operator launches.\n";
    }

    const auto result =
        runtime::run_graph_first_job<types::kline_t>(config_path, options);
    std::cout << "job_id=" << result.manifest.job_id << "\n";
    std::cout << "job_stable_id=" << result.manifest.job_stable_id << "\n";
    std::cout << "job_attempt_id=" << result.manifest.job_attempt_id << "\n";
    std::cout << "job_attempt_policy=" << result.manifest.job_attempt_policy
              << "\n";
    std::cout << "job_kind=" << result.manifest.job_kind << "\n";
    std::cout << "target_component_family_id="
              << result.manifest.target_component_family_id << "\n";
    std::cout << "wave_action=" << result.manifest.wave_action << "\n";
    std::cout << "status=" << result.state.status << "\n";
    std::cout << "wave_id=" << result.wave_plan.wave_id << "\n";
    std::cout << "runtime_handoff_id=" << result.manifest.runtime_handoff_id
              << "\n";
    std::cout << "runtime_handoff_digest="
              << result.manifest.runtime_handoff_digest << "\n";
    std::cout << "marshal_target_driver_run_id="
              << result.manifest.marshal_target_driver_run_id << "\n";
    std::cout << "resolved_anchor_count="
              << result.wave_plan.resolved_anchor_count() << "\n";
    if (!result.manifest_path.empty()) {
      std::cout << "manifest_path=" << result.manifest_path.string() << "\n";
    }
    if (!result.state_path.empty()) {
      std::cout << "state_path=" << result.state_path.string() << "\n";
    }
    if (!result.delegated_report_path.empty()) {
      std::cout << "report_path=" << result.delegated_report_path.string()
                << "\n";
    }
    std::cout << "replay_artifacts_written="
              << (result.state.replay_artifacts_written ? "true" : "false")
              << "\n";
    if (!result.state.replay_batch_index_path.empty()) {
      std::cout << "replay_batch_index_path="
                << result.state.replay_batch_index_path << "\n";
    }
    if (!result.state.replay_artifact_path_index_path.empty()) {
      std::cout << "replay_artifact_path_index_path="
                << result.state.replay_artifact_path_index_path << "\n";
    }
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    print_usage(argv[0]);
    return 1;
  }
}
