#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "hero/runtime_hero/hero_runtime_tools.h"

namespace {

constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";

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
  std::cout
      << "Usage: " << argv0
      << " [--global-config <path>] [--config <hero_runtime_dsl>]"
         " [--profile <runtime_profile>]"
         " [--tool <name>] [--args-json <json>]"
         " [--list-tools] [--list-tools-json]\n"
      << "  default mode: JSON-RPC over stdio\n"
      << "  default policy path: [HERO].runtime_hero_dsl_path in "
         "--global-config, then /cuwacunu/src/config/hero.runtime.dsl\n"
      << "  default profile: --profile, then [HERO].runtime_hero_profile, "
         "then runtime_profile in the policy file\n"
      << "  internal: --runtime-replay-delegate --args-json <json> runs the "
         "non-catalog replay executor used by Environment rollout\n"
      << "\n"
      << "Authority groups:\n"
      << "  Read-only Runtime visibility:\n"
      << "    hero.runtime.status\n"
      << "    hero.runtime.inspect.schema\n"
      << "    hero.runtime.inspect.wave [config_path=...]\n"
      << "    hero.runtime.inspect.jobs [root=...] [limit=...] "
         "[include_artifacts=...]\n"
      << "    hero.runtime.inspect.job job_id=...|job_dir=... "
         "[include_text=...] [max_bytes=...]\n"
      << "    hero.runtime.inspect.artifact.job "
         "[job_id=...|job_dir=...] [artifact=...] [max_bytes=...]\n"
      << "    hero.runtime.inspect.artifact.path path=... [max_bytes=...]\n"
      << "  Runtime execution/delegation:\n"
      << "    hero.runtime.run mode=dry_run|execute "
         "[runtime_handoff_path=...] [runtime_handoff_digest=...] "
         "[contract_path=...] [contract_digest=...]\n"
      << "      wave launch evidence lives in a Runtime handoff artifact; "
         "policy-training execution lives in a contract artifact.\n"
      << "      Replay is admitted through hero.environment.rollout, which "
         "delegates to a non-catalog Runtime replay executor.\n"
      << "  Guarded developer reset:\n"
      << "    hero.runtime.reset mode=plan|execute "
         "[runtime_root=...] [backup=...]\n"
      << "      execution is policy-gated by allow_dev_nuke and "
         "allowed_dev_nuke_roots.\n"
      << "  Boundary:\n"
      << "    Runtime Hero executes allowed waves/replay, can run the "
         "pre-PPO\n"
      << "    noop policy-training smoke path, and reads job "
         "evidence.\n"
      << "    Policy-training execute is limited to "
         "noop_policy_training.v1;\n"
      << "    PPO remains refused until a dedicated Runtime trainer exists. "
         "Readiness-grade\n"
      << "    policy training must bind "
         "causal_walk_forward_training.v1 schedule identity.\n"
      << "    It does not prove Lattice targets, select checkpoints, "
         "or edit\n"
      << "    Config Hero policy files.\n";
}

} // namespace

int main(int argc, char **argv) {
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path policy_path;
  std::string profile_override;
  bool policy_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  bool direct_tool_mode = false;
  bool replay_delegate_mode = false;
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
    if (arg == "--runtime-replay-delegate") {
      replay_delegate_mode = true;
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

  if (direct_tool_args_overridden && !direct_tool_mode &&
      !replay_delegate_mode) {
    std::cerr << "--args-json requires --tool or --runtime-replay-delegate\n";
    return 2;
  }
  if (direct_tool_mode && replay_delegate_mode) {
    std::cerr << "--tool cannot be combined with --runtime-replay-delegate\n";
    return 2;
  }
  if (list_tools && list_tools_json) {
    std::cerr << "--list-tools and --list-tools-json are mutually exclusive\n";
    return 2;
  }
  if ((list_tools || list_tools_json) &&
      (direct_tool_mode || replay_delegate_mode)) {
    std::cerr
        << "--list-tools cannot be combined with --tool or replay delegate\n";
    return 2;
  }

  if (!policy_overridden) {
    policy_path = cuwacunu::hero::runtime::resolve_runtime_hero_dsl_path(
        global_config_path);
  }
  if (trim_ascii(policy_path.string()).empty()) {
    std::cerr << "empty Runtime Hero policy path\n";
    return 2;
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::runtime::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::runtime::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::runtime::runtime_context_t ctx{};
  ctx.global_config_path = global_config_path;
  ctx.policy_path = policy_path;
  std::string error;
  if (!cuwacunu::hero::runtime::load_runtime_policy(
          policy_path, global_config_path, &ctx.policy, &error,
          profile_override)) {
    std::cerr << "failed to load Runtime Hero policy: " << error << "\n";
    return 1;
  }

  if (direct_tool_mode) {
    std::string result;
    std::string tool_error;
    const bool ok = cuwacunu::hero::runtime::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &ctx, &result, &tool_error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok || cuwacunu::hero::runtime::tool_result_is_error(result)) {
      if (!tool_error.empty()) {
        std::cerr << "tool execution failed: " << tool_error << "\n";
      }
      return 1;
    }
    return 0;
  }

  if (replay_delegate_mode) {
    std::string result;
    std::string delegate_error;
    const bool ok = cuwacunu::hero::runtime::execute_replay_delegate_json(
        direct_tool_args_json, &ctx, &result, &delegate_error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok || cuwacunu::hero::runtime::tool_result_is_error(result)) {
      if (!delegate_error.empty()) {
        std::cerr << "replay delegate failed: " << delegate_error << "\n";
      }
      return 1;
    }
    return 0;
  }

  cuwacunu::hero::runtime::run_jsonrpc_stdio_loop(&ctx);
  return 0;
}
