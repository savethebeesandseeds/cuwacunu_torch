// SPDX-License-Identifier: MIT

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "kikijyeba/environment/runtime/experiment_driver.h"
#include "kikijyeba/runtime/job_runner.h"
#include "ujcamei/source/registry/types/data.h"

namespace env = cuwacunu::kikijyeba::environment;
namespace runtime = cuwacunu::kikijyeba::runtime;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

constexpr const char *kDefaultConfigPath = "/cuwacunu/src/config/.config";

void print_usage(const char *argv0) {
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
            << "       [--no-replay-artifacts]\n"
            << "       [--replay-base-reserve-node NODE]\n"
            << "       [--replay-risky-nodes CSV]\n"
            << "       [--replay-experiment-id ID]\n"
            << "       [--replay-report-path PATH]\n"
            << "       [--replay-initial-equity-base VALUE]\n"
            << "       [--replay-min-base-reserve-weight VALUE]\n"
            << "       [--replay-max-risky-weight VALUE]\n"
            << "       [--replay-max-turnover-l1 VALUE]\n"
            << "       [--replay-max-steps N]\n"
            << "       [--replay-max-parallel-jobs N]\n"
            << "       [--replay-linear-transaction-cost-rate VALUE]\n"
            << "       [--replay-allow-synthetic-direct-edges]\n"
            << "       [--replay-include-equal-weight]\n"
            << "       [--replay-include-current-weight]\n"
            << "       [--replay-no-base-reserve-policy]\n"
            << "       [--replay-no-sdu-policy]\n"
            << "default config: " << kDefaultConfigPath << "\n";
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
  std::cout << "replay_mean_final_equity_base="
            << result.report.mean_final_equity_base() << "\n";
  if (!result.report_path.empty()) {
    std::cout << "replay_report_path=" << result.report_path.string() << "\n";
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::string config_path{kDefaultConfigPath};
    runtime::job_runner_options_t options{};
    env::runtime_job_replay_driver_options_t replay_options{};
    bool replay_from_job_dir{false};

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
        replay_from_job_dir = true;
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
      } else if (arg == "--no-replay-artifacts") {
        options.write_replay_artifacts = false;
      } else if (arg == "--replay-base-reserve-node") {
        options.replay_base_reserve_node_id =
            require_next_arg(argc, argv, &i, arg);
        replay_options.base_reserve_node_id =
            options.replay_base_reserve_node_id;
      } else if (arg == "--replay-risky-nodes") {
        options.replay_risky_node_ids =
            parse_csv_arg(require_next_arg(argc, argv, &i, arg));
        replay_options.risky_node_ids = options.replay_risky_node_ids;
      } else if (arg == "--replay-experiment-id") {
        replay_options.experiment_id = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-report-path") {
        replay_options.report_path = require_next_arg(argc, argv, &i, arg);
      } else if (arg == "--replay-initial-equity-base") {
        replay_options.initial_equity_base =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-min-base-reserve-weight") {
        replay_options.min_base_reserve_weight =
            parse_double_arg(require_next_arg(argc, argv, &i, arg), arg);
      } else if (arg == "--replay-max-risky-weight") {
        replay_options.max_risky_weight =
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
      } else if (arg == "--replay-allow-synthetic-direct-edges") {
        replay_options.world_options.paper_execution_options
            .allow_synthetic_direct_edges = true;
      } else if (arg == "--replay-include-equal-weight") {
        replay_options.include_equal_weight_policy = true;
      } else if (arg == "--replay-include-current-weight") {
        replay_options.include_current_weight_policy = true;
      } else if (arg == "--replay-no-base-reserve-policy") {
        replay_options.include_base_reserve_policy = false;
      } else if (arg == "--replay-no-sdu-policy") {
        replay_options.include_spot_distributional_utility_policy = false;
      } else {
        throw std::runtime_error("[cuwacunu_exec] unknown argument: " + arg);
      }
    }

    if (replay_from_job_dir) {
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
          (config_path == kDefaultConfigPath) ? std::string{} : config_path;
      const auto replay_result =
          env::run_runtime_job_replay_experiment(replay_options);
      print_replay_result(replay_result);
      return 0;
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
