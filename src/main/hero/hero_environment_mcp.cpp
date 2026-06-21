#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "hero/config_path_defaults.h"
#include "hero/environment_hero/hero_environment_tools.h"

namespace {

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

void print_help(const char *argv0) {
  const auto default_global_config_path =
      cuwacunu::hero::config_paths::default_global_config_path(argv0);
  const auto default_global_config = default_global_config_path.string();
  const auto default_policy_path =
      cuwacunu::hero::config_paths::default_config_sibling_path(
          default_global_config_path, "hero.environment.dsl")
          .string();
  std::cout
      << "Usage: " << argv0
      << " [--global-config <path>] [--config <hero_environment_dsl>]"
         " [--profile <environment_profile>]"
         " [--tool <name>] [--args-json <json>]"
         " [--list-tools] [--list-tools-json]\n"
      << "  default mode: JSON-RPC over stdio\n"
      << "  default policy path: [HERO].environment_hero_dsl_path in "
         "--global-config, then "
      << default_policy_path << "\n"
      << "  default global config: " << default_global_config << "\n"
      << "\n"
      << "Authority groups:\n"
      << "  Read-only Environment visibility:\n"
      << "    hero.environment.status\n"
      << "    hero.environment.inspect.schema\n"
      << "    hero.environment.inspect.job job_dir=... "
         "[include_text=...] [max_bytes=...]\n"
      << "  Environment admission and rollout:\n"
      << "    hero.environment.certify.policy_acceptance "
         "mode=check|issue policy_training_job_dir=... acceptance_id=... "
         "certification_evidence={...} "
         "[expected_preview_digest=...]\n"
      << "    hero.environment.certify.paper_online_readiness "
         "mode=check|issue policy_acceptance_job_dir=... readiness_id=... "
         "certification_evidence={...} "
         "[expected_preview_digest=...]\n"
      << "      check validates prerequisite evidence without writing; issue "
         "writes the Lattice sidecar after validation and preview digest "
         "binding.\n"
      << "    hero.environment.rollout mode=validate|replay "
         "runtime_job_dir=... rollout_id=... rollout_attempt_id=... "
         "target_node_ids=[...] [include_machine_payload=true]\n"
      << "      validate checks the rollout request; replay delegates "
         "low-level replay execution to Runtime. Policy set, limits, runtime "
         "exec path, timeout, and Cajtucu execution profile come from the "
         "active Environment profile.\n"
      << "  Boundary:\n"
      << "    Environment Hero owns environment-admission facts and "
         "environment-run surfaces.\n"
      << "    Runtime Hero remains the low-level executor, Lattice Hero "
         "proves evidence,\n"
      << "    and Marshal Hero orchestrates high-level movement. Environment "
         "does not\n"
      << "    select policies, prove targets, authorize live capital, or "
         "deploy.\n";
}

} // namespace

int main(int argc, char **argv) {
  std::filesystem::path global_config_path =
      cuwacunu::hero::config_paths::default_global_config_path(argv[0]);
  std::filesystem::path policy_path;
  std::string profile_override;
  bool policy_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  std::string direct_tool_name;
  std::string direct_tool_args_json = "{}";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--config" && i + 1 < argc) {
      policy_path = argv[++i];
      policy_overridden = true;
      continue;
    }
    if (arg == "--profile" && i + 1 < argc) {
      profile_override = argv[++i];
      continue;
    }
    if (arg == "--tool" && i + 1 < argc) {
      direct_tool_name = argv[++i];
      direct_tool_mode = true;
      continue;
    }
    if (arg == "--args-json" && i + 1 < argc) {
      direct_tool_args_json = argv[++i];
      direct_tool_args_overridden = true;
      continue;
    }
    if (arg == "--list-tools") {
      list_tools = true;
      continue;
    }
    if (arg == "--list-tools-json") {
      list_tools_json = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    std::cerr << "--args-json requires --tool\n";
    return 2;
  }
  if (list_tools && list_tools_json) {
    std::cerr << "--list-tools and --list-tools-json are mutually exclusive\n";
    return 2;
  }
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    std::cerr << "--list-tools cannot be combined with --tool\n";
    return 2;
  }

  if (!policy_overridden) {
    policy_path =
        cuwacunu::hero::environment::resolve_environment_hero_dsl_path(
            global_config_path);
  }
  if (trim_ascii(policy_path.string()).empty()) {
    std::cerr << "empty Environment Hero policy path\n";
    return 2;
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::environment::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::environment::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::environment::environment_context_t ctx{};
  ctx.global_config_path = global_config_path;
  ctx.policy_path = policy_path;
  std::string error;
  if (!cuwacunu::hero::environment::load_environment_policy(
          policy_path, global_config_path, &ctx.policy, &error,
          profile_override)) {
    std::cerr << "failed to load Environment Hero policy: " << error << "\n";
    return 1;
  }

  if (direct_tool_mode) {
    std::string result;
    std::string tool_error;
    const bool ok = cuwacunu::hero::environment::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &ctx, &result, &tool_error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok || cuwacunu::hero::environment::tool_result_is_error(result)) {
      if (!tool_error.empty()) {
        std::cerr << "tool execution failed: " << tool_error << "\n";
      }
      return 1;
    }
    return 0;
  }

  cuwacunu::hero::environment::run_jsonrpc_stdio_loop(&ctx);
  return 0;
}
