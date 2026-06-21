#include "hero/marshal_hero/hero_marshal_tools.h"

#include "hero/config_path_defaults.h"
#include "hero/lattice_hero/hero_lattice_tools.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <unistd.h>

namespace {

std::filesystem::path g_global_config_path{};

bool call_lattice_tool(const std::string &tool_name,
                       const std::string &arguments_json,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  static cuwacunu::hero::lattice::lattice_context_t context{};
  static std::filesystem::path initialized_global_config{};
  static bool initialized = false;

  if (!initialized || initialized_global_config != g_global_config_path) {
    context = cuwacunu::hero::lattice::lattice_context_t{};
    context.global_config_path = g_global_config_path;
    context.policy_path =
        cuwacunu::hero::lattice::resolve_lattice_hero_dsl_path(
            g_global_config_path);
    std::string error;
    if (!cuwacunu::hero::lattice::load_lattice_policy(
            context.policy_path, context.global_config_path, &context.policy,
            &error)) {
      if (out_error_message) {
        *out_error_message = error;
      }
      return false;
    }
    initialized_global_config = g_global_config_path;
    initialized = true;
  }

  return cuwacunu::hero::lattice::execute_tool_json(
      tool_name, arguments_json, &context, out_tool_result_json,
      out_error_message);
}

void print_help(const char *argv0) {
  const auto default_global_config_path =
      cuwacunu::hero::config_paths::default_global_config_path(argv0);
  const auto default_global_config = default_global_config_path.string();
  std::cout
      << "Usage: " << argv0 << " [options]\n"
      << "  --global-config PATH   global Cuwacunu config; default "
      << default_global_config << "\n"
      << "  --tool NAME            run one Marshal tool directly\n"
      << "  --args-json JSON       JSON object passed to --tool; default '{}'\n"
      << "  --list-tools           compact human catalog\n"
      << "  --list-tools-json      MCP tools/list JSON schema catalog\n"
      << "  --help, -h             show this operator guide\n"
      << "\n"
      << "Default mode:\n"
      << "  JSON-RPC over stdio. The direct --tool mode is for shell checks,\n"
      << "  smoke tests, and operator handoffs.\n"
      << "\n"
      << "Marshal role and authority boundary:\n"
      << "  Marshal is a coordinator. It prepares bounded handoffs, checks\n"
      << "  request shape, records receipts, and explains evidence. Runtime "
         "Hero\n"
      << "  executes allowed waves/replay. Lattice Hero proves target "
         "satisfaction.\n"
      << "  Marshal does not become proof authority, market-readiness "
         "authority,\n"
      << "  deployment authority, checkpoint selector, or allocation "
         "authority.\n"
      << "\n"
      << "Tool families:\n"
      << "  Read-only visibility:\n"
      << "    hero.marshal.status and hero.marshal.inspect.*\n"
      << "  Target-dispatch coordination:\n"
      << "    hero.marshal.prepare.* tools start from one dispatchable "
         "Lattice target\n"
      << "    and may delegate dry-run/execute work through Runtime Hero.\n"
      << "  Environment rollout coordination:\n"
      << "    hero.marshal.rollout starts from completed Runtime job "
         "evidence\n"
      << "    and may delegate bounded replay through Environment/Runtime "
         "replay handoff.\n"
      << "  Paper-online session handoff coordination:\n"
      << "    hero.marshal.paper_online.session_handoff starts from "
         "readiness/admission evidence\n"
      << "    and prepares Environment-compatible admission/session payloads "
         "or delegates the bounded Environment run.\n"
      << "  Boundary:\n"
      << "    Marshal coordinates, validates, delegates, records, and "
         "explains.\n"
      << "    It does not prove targets, run policies itself, select "
         "winners,\n"
      << "    or authorize live execution.\n"
      << "\n"
      << "Public tools:\n"
      << "\n"
      << "  hero.marshal.status\n"
      << "    Purpose: quick read-only health and boundary summary for "
         "Marshal.\n"
      << "    Use when: you want to know which receipts/advice surfaces are "
         "visible\n"
      << "      and confirm Marshal is not claiming execution/proof "
         "authority.\n"
      << "    Key args:\n"
      << "      none\n"
      << "    Returns: read-only status flags, recent receipt count, last "
         "refusal,\n"
      << "      and explicit authority-denial fields.\n"
      << "    Example:\n"
      << "      " << argv0 << " --global-config " << default_global_config
      << " --tool "
         "hero.marshal.status --args-json '{}'\n"
      << "\n"
      << "  hero.marshal.prepare.train\n"
      << "  hero.marshal.prepare.evaluate\n"
      << "    Purpose: prepare a bounded Runtime handoff for one Lattice "
         "target.\n"
      << "    Use when: a dispatchable target, normally train/evaluate "
         "readiness,\n"
      << "      needs a finite next step under Runtime policy. "
         "Artifact-readiness\n"
      << "      targets route to inspect. Replay uses hero.marshal.rollout.\n"
      << "      Policy-training is reserved until it has a concrete Runtime "
         "job\n"
      << "      contract. Choose train/evaluate in the tool name; the "
         "single-wave\n"
      << "      or bounded driver behavior comes from the selected Marshal "
         "profile.\n"
      << "    Core model:\n"
      << "      Lattice target/advice -> Marshal validation -> Runtime dry-run "
         "or\n"
      << "      execution handoff packet. Lattice must re-read evidence "
         "later.\n"
      << "    Key args:\n"
      << "      target_id                 required target identity\n"
      << "      mode                       plan, dry_run, or execute\n"
      << "      profile                   optional Marshal prepare profile\n"
      << "      include_machine_payload   optional response verbosity flag\n"
      << "    Marshal derives Lattice/Runtime evidence at call time. Driver "
         "limits,\n"
      << "      materialization behavior, validation context, and timeout live "
         "in\n"
      << "      hero.marshal.dsl MARSHAL_PREPARE_PROFILE blocks.\n"
      << "    Returns: target-driver packet, dispatchability/refusal reasons,\n"
      << "      optional Runtime dry-run handoff, and next safe actions.\n"
      << "    Example:\n"
      << "      " << argv0 << " --global-config " << default_global_config
      << " --tool "
         "hero.marshal.prepare.train --args-json "
         "'{\"target_id\":\"channel_mdn_train_core_ready\","
         "\"mode\":\"plan\"}'\n"
      << "\n"
      << "  hero.marshal.rollout\n"
      << "    Purpose: prepare or execute a bounded historical replay rollout "
         "of\n"
      << "      a policy set against completed Runtime job artifacts.\n"
      << "    Use when: trained components already produced a Runtime job "
         "directory\n"
      << "      and you want Kikijyeba replay plus Cajtucu paper execution to "
         "emit\n"
      << "      trajectory evidence. This is the RL/environment rollout "
         "surface.\n"
      << "    Core model:\n"
      << "      completed Runtime job + policy set + replay environment + "
         "Cajtucu\n"
      << "      paper execution profile -> Runtime replay report/evidence.\n"
      << "    Key args:\n"
      << "      mode                       plan or execute\n"
      << "      runtime_job_dir            completed Runtime job directory\n"
      << "      rollout_id                 rollout identity\n"
      << "      rollout_attempt_id         attempt identity and idempotency "
         "key\n"
      << "      target_node_ids            ordered target graph nodes\n"
      << "      profile                    optional Marshal rollout profile\n"
      << "      include_machine_payload   optional response verbosity flag\n"
      << "    Profile/derived fields:\n"
      << "      policy_set, max_steps, max_parallel_jobs, runtime_exec_path,\n"
      << "      timeout_seconds, and Cajtucu execution profile come from the\n"
      << "      Marshal rollout profile. Replay index and graph fingerprint "
         "are\n"
      << "      read from the Runtime job; asset-universe digest is derived.\n"
      << "    Returns: rollout plan in plan mode; in execute, Runtime replay "
         "handoff\n"
      << "      result digests and dispatch state. Runtime reports are written "
         "by\n"
      << "      Runtime; inspection summaries come from later inspect.\n"
      << "    Example plan:\n"
      << "      " << argv0 << " --global-config " << default_global_config
      << " --tool "
         "hero.marshal.rollout --args-json "
         "'{\"mode\":\"plan\",\"runtime_job_dir\":\"/tmp/runtime_job\","
         "\"rollout_id\":\"holdout_rollout_v1\","
         "\"rollout_attempt_id\":\"holdout_rollout_v1_attempt_001\","
         "\"target_node_ids\":[\"USDT\",\"BTC\",\"ETH\"]}'\n"
      << "\n"
      << "  hero.marshal.paper_online.session_handoff\n"
      << "    Purpose: prepare a bounded paper-online session handoff from "
         "existing readiness/admission evidence or delegate its bounded "
         "Environment run.\n"
      << "    Use when: paper_online_readiness_contract_ready exists and an "
         "operator wants\n"
      << "      Environment-compatible admission/session payloads plus a "
         "durable Marshal receipt before running.\n"
      << "    Core model:\n"
      << "      readiness sidecar + handoff request -> Environment admission "
         "check;\n"
      << "      after admission evidence exists, Environment session validate. "
         "In run mode Marshal repeats those gates and delegates Environment "
         "session run.\n"
      << "    Key args:\n"
      << "      mode                       plan, dry_run, or run\n"
      << "      handoff_request            compact request object\n"
      << "      handoff_request_path       compact request file path\n"
      << "      include_machine_payload   optional response verbosity flag\n"
      << "    Returns: handoff digest, Environment request digests, validation "
         "and run results,\n"
      << "      receipt path/digest, authority denials, and next safe "
         "actions.\n"
      << "    Example dry-run:\n"
      << "      " << argv0 << " --global-config " << default_global_config
      << " --tool "
         "hero.marshal.paper_online.session_handoff --args-json "
         "'{\"mode\":\"dry_run\",\"handoff_request_path\":\"/cuwacunu/.runtime/"
         "cuwacunu_exec/operator_requests/paper_online_session_handoff.request."
         "json\"}'\n"
      << "\n"
      << "  hero.marshal.inspect.run.latest_chain\n"
      << "  hero.marshal.inspect.run.training_state\n"
      << "  hero.marshal.inspect.run.single_job\n"
      << "  hero.marshal.inspect.run.compare\n"
      << "  hero.marshal.inspect.target\n"
      << "  hero.marshal.inspect.protocol.report\n"
      << "  hero.marshal.inspect.protocol.strict\n"
      << "  hero.marshal.inspect.spawn.by_id\n"
      << "  hero.marshal.inspect.spawn.by_label\n"
      << "  hero.marshal.inspect.spawn.by_registry\n"
      << "  hero.marshal.inspect.spawn.by_fingerprint\n"
      << "  hero.marshal.inspect.spawn.by_job\n"
      << "  hero.marshal.inspect.component\n"
      << "  hero.marshal.inspect.facts.summary\n"
      << "  hero.marshal.inspect.facts.lineage\n"
      << "  hero.marshal.inspect.facts.preview\n"
      << "    Purpose: read-only operator inspection over "
         "Runtime/Lattice/Marshal\n"
      << "      evidence. It explains what exists and what failed without "
         "writing,\n"
      << "      dispatching, proving, or selecting winners.\n"
      << "    Surfaces:\n"
      << "      inspect.run.*      latest_chain, training_state, single_job, "
         "compare\n"
      << "      inspect.target     Lattice target proof/deficit context\n"
      << "      inspect.protocol.* protocol identity with report or strict "
         "match "
         "context\n"
      << "      inspect.spawn.*    component-spawn registry identity context\n"
      << "      inspect.component  component family/status context\n"
      << "      inspect.facts.*    fact-family summaries/previews/lineage\n"
      << "    Key args:\n"
      << "      runtime_root, config_path, job_id/job_ids, "
         "target_id/target_ids,\n"
      << "      baseline_job_id, candidate_job_id, fact_family_id, "
         "fact_digest,\n"
      << "      fact_digest_prefix, fact_index, include_facts, "
         "include_lineage,\n"
      << "      include_machine_payload, and strict protocol identity "
         "fingerprints.\n"
      << "    Returns: compact operator report, Lattice quoted proof/fact "
         "panels,\n"
      << "      warnings, blockers, comparison deltas, and next safe actions.\n"
      << "    Example:\n"
      << "      " << argv0 << " --global-config " << default_global_config
      << " --tool "
         "hero.marshal.inspect.run.latest_chain --args-json '{}'\n"
      << "\n"
      << "Operational notes:\n"
      << "  Use --list-tools-json when a client needs exact MCP schemas.\n"
      << "  Use --list-tools for a short catalog. Use --help for this "
         "runbook.\n"
      << "  Rollout execute mode delegates bounded replay through "
         "Environment/Runtime;\n"
      << "  Marshal records handoff state and digests, but Runtime remains "
         "the executor.\n"
      << "  Deeper docs: src/include/hero/marshal_hero/marshal/README.md and\n"
      << "  src/include/hero/marshal_hero/marshal/ROADMAP.md.\n";
}

} // namespace

int main(int argc, char **argv) {
  g_global_config_path =
      cuwacunu::hero::config_paths::default_global_config_path(argv[0]);
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;
  std::string direct_tool_name;
  std::string direct_tool_args_json = "{}";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      g_global_config_path = argv[++i];
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

  cuwacunu::hero::marshal::set_marshal_lattice_tool_callback(call_lattice_tool);

  if (list_tools_json) {
    std::cout << cuwacunu::hero::marshal::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::marshal::build_tools_list_human_text();
    return 0;
  }

  cuwacunu::hero::marshal::marshal_context_t ctx{};
  ctx.global_config_path = g_global_config_path;
  ctx.policy_path = cuwacunu::hero::marshal::resolve_marshal_hero_dsl_path(
      g_global_config_path);
  std::string policy_error;
  if (!cuwacunu::hero::marshal::load_marshal_policy(
          ctx.policy_path, ctx.global_config_path, &ctx.policy,
          &policy_error)) {
    std::cerr << "failed to load Marshal Hero policy: " << policy_error << "\n";
    return 1;
  }

  if (direct_tool_mode) {
    std::string result;
    std::string error;
    const bool ok = cuwacunu::hero::marshal::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &ctx, &result, &error);
    if (!result.empty()) {
      std::cout << result << "\n";
    }
    if (!ok && !error.empty()) {
      std::cerr << "tool execution failed: " << error << "\n";
    }
    return ok ? 0 : 1;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[hero_marshal_mcp] no --tool provided and stdin is a "
                 "terminal; default mode expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::marshal::run_jsonrpc_stdio_loop(&ctx);
  return 0;
}
