#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "hero/lattice_hero/hero_lattice_tools.h"

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
      << " [--global-config <path>] [--config <hero_lattice_dsl>]"
         " [--tool <name>] [--args-json <json>]"
         " [--list-tools] [--list-tools-json]\n"
      << "  default mode: JSON-RPC over stdio\n"
      << "  default policy path: [HERO].lattice_hero_dsl_path in "
         "--global-config, then /cuwacunu/src/config/hero.lattice.dsl\n"
      << "\n"
      << "Agent runbook:\n"
      << "  Lattice Hero is read-only. It scans runtime evidence, "
         "explains/evaluates targets,\n"
      << "  plans a suggested wave, and inspects checkpoint lineage; "
         "it never executes waves.\n"
      << "  The active global config points to one targets DSL and one "
         "splits DSL. Each file may\n"
      << "  contain many LATTICE_TARGET or LATTICE_SPLIT blocks.\n"
      << "  Start with status, then list_targets, explain_target, "
         "scan_exposure, evaluate/plan.\n"
      << "  Report warnings and blocked/exposure_failed reasons "
         "verbatim. Empty runtime roots\n"
      << "  need explicit active identity for graph-anchor targets. "
         "Runtime Hero executes plans.\n"
      << "  scan_exposure previews anchor_domain_health: accepted/candidate "
         "anchors, skip reasons,\n"
      << "  common/reference key bounds, and source-domain warning level.\n"
      << "  evaluate_target reports exposure_summaries for coverage "
         "targets: unique coverage,\n"
      << "  repeated cursor-epoch load, and optimizer-step density. "
         "LATTICE_WARN returns non-blocking\n"
      << "  warning_results without changing target status or planning.\n"
      << "  Use --list-tools for the tool catalog.\n"
      << "\n"
      << "Authority groups:\n"
      << "  Target proof and advice:\n"
      << "    hero.lattice.list_targets, explain_target, evaluate_target,\n"
      << "    evaluate_targets, target_deficit, "
         "latest_satisfying_checkpoint,\n"
      << "    compare_evidence, checkpoint_closure\n"
      << "  Evidence, facts, and audit indexes:\n"
      << "    hero.lattice.scan_exposure, list_fact_families, scan_facts,\n"
      << "    fact_summary, fact_lineage, fact_preview, index_status,\n"
      << "    index_query, derived_query\n"
      << "  Boundary:\n"
      << "    Lattice Hero is read-only. It proves or refuses targets from "
         "durable\n"
      << "    evidence, but never executes Runtime, edits Config, selects "
         "policies,\n"
      << "    or authorizes market/deployment decisions.\n"
      << "\n"
      << "Common direct calls:\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.status --args-json '{}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.list_targets --args-json '{}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.explain_target --args-json "
         "'{\"target_id\":\"channel_mdn_train_core_ready\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.scan_exposure --args-json "
         "'{\"runtime_root\":\"/tmp/cuwacunu_train_core_lattice_v0\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.scan_facts --args-json "
         "'{\"runtime_root\":\"/tmp/cuwacunu_train_core_lattice_v0\","
         "\"family\":\"allocation_engine\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.fact_lineage --args-json "
         "'{\"runtime_root\":\"/tmp/cuwacunu_train_core_lattice_v0\","
         "\"family\":\"forecast_eval\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.index_status --args-json "
         "'{\"runtime_root\":\"/tmp/cuwacunu_train_core_lattice_v0\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.index_query --args-json "
         "'{\"relation\":\"checkpoint\",\"compare_live_scan\":true}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.index_query --args-json "
         "'{\"relation\":\"checkpoint\",\"compare_live_scan\":false,"
         "\"allow_unproven_cache\":true,"
         "\"validation_strength\":\"header_only\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.evaluate_targets --args-json "
         "'{\"target_ids\":[\"vicreg_train_core_ready\","
         "\"channel_mdn_train_core_ready\"]}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.compare_evidence --args-json "
         "'{\"left_target_id\":\"channel_mdn_train_core_no_validation_"
         "leakage\","
         "\"right_target_id\":\"channel_mdn_train_core_no_test_leakage\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.derived_query --args-json "
         "'{\"relation\":\"target_satisfied\","
         "\"target_id\":\"channel_mdn_validation_eval_ready\"}'\n"
      << "  " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.lattice.latest_satisfying_checkpoint --args-json "
         "'{\"symbolic_hint\":\"latest_satisfying:vicreg_train_core_ready\"}'\n"
      << "\n"
      << "Tool roles:\n"
      << "  hero.lattice.status             policy, configured paths, "
         "counts, warnings, inferred identity\n"
      << "  hero.lattice.schema             Lattice Hero policy key "
         "schema\n"
      << "  hero.lattice.list_targets       compiled target summaries "
         "from targets DSL\n"
      << "  hero.lattice.explain_target     proof-object explanation, "
         "identity boundaries, static math, readiness crosswalks, derived "
         "rules, deficit/evidence-order vocabularies, numeric dimensions, "
         "and warning-scope previews without evidence scan\n"
      << "  hero.lattice.scan_exposure      runtime artifacts -> "
         "exposure/checkpoint fact and anchor-domain-health preview\n"
      << "  hero.lattice.list_fact_families generic fact-family registry; "
         "fact families are not target kinds\n"
      << "  hero.lattice.scan_facts         generic fact catalog scan, "
         "optionally filtered by family; no target proof or dispatch\n"
      << "  hero.lattice.fact_summary       generic fact-family summaries "
         "over runtime evidence\n"
      << "  hero.lattice.fact_lineage       read-only fact-family lineage "
         "rows plus integrity rollups; no target proof or dispatch\n"
      << "  hero.lattice.fact_preview       read-only concrete fact-row "
         "preview by family/digest/index; no target proof or selection\n"
      << "  hero.lattice.index_status       rebuildable cache status; "
         "valid cache rows or live-scan fallback\n"
      << "  hero.lattice.index_query        read-only audit index query with "
         "cache/live-scan parity proof or explicit unproven cache fast path\n"
      << "  hero.lattice.derived_query      read-only named rule query with "
         "target, leakage, closure, lineage, or cache-validation witnesses\n"
      << "  hero.lattice.evaluate_target    target proof over runtime "
         "evidence\n"
      << "  hero.lattice.evaluate_targets   batch target proof over one "
         "runtime evidence scan\n"
      << "  hero.lattice.compare_evidence   Pareto comparison of two clean "
         "satisfying checkpoint evidence vectors; no scalar score or "
         "deployment decision\n"
      << "  hero.lattice.target_deficit     target status, proof deficits, "
         "and any target-authored suggested wave; read-only\n"
      << "  hero.lattice.latest_satisfying_checkpoint report one "
         "latest_satisfying checkpoint candidate with Lattice-owned proof "
         "semantics\n"
      << "  hero.lattice.checkpoint_closure checkpoint exposure lineage "
         "and unresolved inputs\n";
}

} // namespace

int main(int argc, char **argv) {
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path policy_path;
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
    policy_path = cuwacunu::hero::lattice::resolve_lattice_hero_dsl_path(
        global_config_path);
  }
  if (trim_ascii(policy_path.string()).empty()) {
    std::cerr << "empty Lattice Hero policy path\n";
    return 2;
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::lattice::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::lattice::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::lattice::lattice_context_t ctx{};
  ctx.global_config_path = global_config_path;
  ctx.policy_path = policy_path;
  std::string error;
  if (!cuwacunu::hero::lattice::load_lattice_policy(
          policy_path, global_config_path, &ctx.policy, &error)) {
    std::cerr << "failed to load Lattice Hero policy: " << error << "\n";
    return 1;
  }

  if (direct_tool_mode) {
    std::string result;
    std::string tool_error;
    const bool ok = cuwacunu::hero::lattice::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &ctx, &result, &tool_error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok || cuwacunu::hero::lattice::tool_result_is_error(result)) {
      if (!tool_error.empty()) {
        std::cerr << "tool execution failed: " << tool_error << "\n";
      }
      return 1;
    }
    return 0;
  }

  cuwacunu::hero::lattice::run_jsonrpc_stdio_loop(&ctx);
  return 0;
}
