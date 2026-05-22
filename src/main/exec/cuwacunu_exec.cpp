// SPDX-License-Identifier: MIT

#include <exception>
#include <filesystem>
#include <iostream>
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
      } else {
        throw std::runtime_error("[cuwacunu_exec] unknown argument: " + arg);
      }
    }

    const auto result =
        runtime::run_graph_first_job<types::kline_t>(config_path, options);
    std::cout << "job_id=" << result.manifest.job_id << "\n";
    std::cout << "job_kind=" << result.manifest.job_kind << "\n";
    std::cout << "target_component=" << result.manifest.target_component
              << "\n";
    std::cout << "wave_action=" << result.manifest.wave_action << "\n";
    std::cout << "status=" << result.state.status << "\n";
    std::cout << "wave_id=" << result.wave_plan.wave_id << "\n";
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
