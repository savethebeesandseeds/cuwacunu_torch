// SPDX-License-Identifier: MIT

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

#include "kikijyeba/runtime/job_runner.h"
#include "ujcamei/source/registry/types/data.h"

namespace runtime = cuwacunu::kikijyeba::runtime;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

constexpr const char *kDefaultConfigPath = "/cuwacunu/src/config/.config";

void print_usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " [--config PATH] [--job-dir PATH] [--dry-run]\n"
            << "       [--force-rebuild-cache]\n"
            << "       [--source-range all|anchor_index|source_key]\n"
            << "       [--anchor-index-begin N] [--anchor-index-end N]\n"
            << "       [--source-key-begin K] [--source-key-end K]\n"
            << "       [--runtime-handoff-id ID]\n"
            << "       [--runtime-handoff-digest DIGEST]\n"
            << "       [--marshal-target-driver-run-id ID]\n"
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

} // namespace

int main(int argc, char **argv) {
  try {
    std::string config_path{kDefaultConfigPath};
    runtime::job_runner_options_t options{};

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        print_usage(argv[0]);
        return 0;
      }
      if (arg == "--config") {
        config_path = require_next_arg(argc, argv, &i, arg);
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
      } else {
        throw std::runtime_error("[cuwacunu_exec] unknown argument: " + arg);
      }
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
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    print_usage(argv[0]);
    return 1;
  }
}
