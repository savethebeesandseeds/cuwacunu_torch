#include "hero/marshal_hero/hero_marshal_tools.h"

#include "hero/lattice_hero/hero_lattice_tools.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <unistd.h>

namespace {

std::filesystem::path g_global_config_path{"/cuwacunu/src/config/.config"};

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
  std::cout
      << "Usage: " << argv0 << " [options]\n"
      << "  --global-config PATH   global Cuwacunu config; default "
         "/cuwacunu/src/config/.config\n"
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
      << "    hero.marshal.status, hero.marshal.inspect\n"
      << "  Target-dispatch coordination:\n"
      << "    hero.marshal.prepare starts from one dispatchable Lattice "
         "target\n"
      << "    and may delegate dry-run/execute work through Runtime Hero.\n"
      << "  Environment rollout coordination:\n"
      << "    hero.marshal.rollout starts from completed Runtime job "
         "evidence\n"
      << "    and may delegate bounded replay to hero.runtime.run "
         "operation=replay.\n"
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
      << "      receipt_root       optional receipt directory to scan\n"
      << "      limit              max recent receipts to summarize\n"
      << "      receipts           optional caller-supplied test receipts\n"
      << "      runtime_policy     optional caller-supplied policy snapshot\n"
      << "    Returns: read-only status flags, recent receipt count, last "
         "refusal,\n"
      << "      and explicit authority-denial fields.\n"
      << "    Example:\n"
      << "      " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.marshal.status --args-json '{}'\n"
      << "\n"
      << "  hero.marshal.prepare\n"
      << "    Purpose: prepare a bounded Runtime handoff for one Lattice "
         "target.\n"
      << "    Use when: a dispatchable target, normally train/evaluate "
         "readiness,\n"
      << "      needs a finite next step under Runtime policy. "
         "Artifact-readiness\n"
      << "      targets route to inspect. Replay uses hero.marshal.rollout.\n"
      << "      Policy-training is reserved until it has a concrete Runtime "
         "job\n"
      << "      contract. The default intent is train.\n"
      << "    Core model:\n"
      << "      Lattice target/advice -> Marshal validation -> Runtime dry-run "
         "or\n"
      << "      execution handoff packet. Lattice must re-read evidence "
         "later.\n"
      << "    Key args:\n"
      << "      target_id                 required target identity\n"
      << "      intent                     train or evaluate for dispatchable "
         "target\n"
      << "                                 movement; other recognized values "
         "are\n"
      << "                                 reserved/routing semantics\n"
      << "      requested_mode             plan, dry_run, or execute\n"
      << "      drive_mode                 one_step or budgeted\n"
      << "      runtime_root               Runtime evidence root\n"
      << "      max_waves                  finite wave budget for budgeted "
         "mode\n"
      << "      context                    operator context object\n"
      << "      materialize_plan_inputs    resolve Lattice hints into concrete "
         "paths\n"
      << "      include_runtime_dry_run    legacy compatibility; "
         "dry_run/execute "
         "now\n"
      << "                                 request Runtime dry-run by mode\n"
      << "      include_machine_payload    include raw handoff details\n"
      << "      timeout_seconds            finite Runtime tool timeout\n"
      << "    Returns: target-driver packet, dispatchability/refusal reasons,\n"
      << "      optional Runtime dry-run handoff, and next safe actions.\n"
      << "    Example:\n"
      << "      " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.marshal.prepare --args-json "
         "'{\"target_id\":\"channel_mdn_train_core_ready\","
         "\"requested_mode\":\"plan\",\"drive_mode\":\"one_step\"}'\n"
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
      << "      rollout_id                 stable rollout identity\n"
      << "      rollout_attempt_id         per-invocation rollout identity\n"
      << "      idempotency_key            audit identity binding for this "
         "checkpoint;\n"
      << "                                 durable duplicate-handoff "
         "suppression is future work\n"
      << "      experiment_id              report/trajectory grouping id\n"
      << "      runtime_job_dir            completed Runtime job directory\n"
      << "      replay_batch_index_path    Runtime replay batch index "
         "evidence\n"
      << "      requested_mode             plan or execute\n"
      << "      environment_mode           historical_replay in V1\n"
      << "      environment_assembly_id    kikijyeba.environment.replay.v1\n"
      << "      graph_order_fingerprint    graph identity bound to the Runtime "
         "job\n"
      << "      asset_universe_digest      digest of reserve/risky node order\n"
      << "      base_reserve_node_id       reserve/reference graph node\n"
      << "      risky_node_ids             risky allocatable graph nodes\n"
      << "      policy_set                 required non-empty set: reserve, "
         "current_weight, "
         "equal_weight,\n"
      << "                                 sdu, or canonical policy ids\n"
      << "      max_steps                  required positive finite replay "
         "step "
         "cap\n"
      << "      max_parallel_jobs          finite parallel rollout workers\n"
      << "      execution_profile          Cajtucu paper options; includes\n"
      << "                                 execution_backend_id,\n"
      << "                                 cost_model_id,\n"
      << "                                 allow_synthetic_direct_edges=false "
         "(synthetic\n"
      << "                                 edges are forbidden for validation "
         "rollout),\n"
      << "                                 linear_transaction_cost_rate,\n"
      << "                                 allow_partial_fills\n"
      << "      require_completed_runtime_job / require_replay_artifacts\n"
      << "      include_machine_payload    include Runtime replay args/result "
         "JSON\n"
      << "    Returns: rollout plan in plan mode; in execute, Runtime replay "
         "handoff\n"
      << "      result digests and dispatch state. Runtime reports are written "
         "by\n"
      << "      Runtime; inspection summaries come from later inspect.\n"
      << "    Example plan:\n"
      << "      " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.marshal.rollout --args-json "
         "'{\"rollout_id\":\"holdout_rollout_v1\","
         "\"rollout_attempt_id\":\"holdout_rollout_v1_attempt_001\","
         "\"idempotency_key\":\"holdout_rollout_v1_attempt_001\","
         "\"runtime_job_dir\":\"/tmp/runtime_job\","
         "\"replay_batch_index_path\":\"/tmp/runtime_job/artifacts/"
         "kikijyeba.environment.replay.v1/runtime_replay_batches.index\","
         "\"requested_mode\":\"plan\","
         "\"graph_order_fingerprint\":\"<graph_fingerprint>\","
         "\"asset_universe_digest\":\"<asset_universe_digest>\","
         "\"base_reserve_node_id\":\"USDT\","
         "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
         "\"policy_set\":[\"reserve\",\"equal_weight\",\"sdu\"],"
         "\"max_steps\":250,\"max_parallel_jobs\":4}'\n"
      << "\n"
      << "  hero.marshal.inspect\n"
      << "    Purpose: read-only operator inspection over "
         "Runtime/Lattice/Marshal\n"
      << "      evidence. It explains what exists and what failed without "
         "writing,\n"
      << "      dispatching, proving, or selecting winners.\n"
      << "    Subjects:\n"
      << "      run        latest_chain, training_state, single_job, compare\n"
      << "      target     Lattice target proof/deficit context\n"
      << "      protocol   protocol identity with report or strict match "
         "context\n"
      << "      spawn      component-spawn registry identity context\n"
      << "      component  component family/status context\n"
      << "      facts      fact-family summaries/previews/lineage\n"
      << "    Key args:\n"
      << "      subject                   required subject enum\n"
      << "      mode                      subject-specific mode\n"
      << "      runtime_root              Runtime evidence root\n"
      << "      job_id / job_ids          job identities for run inspection\n"
      << "      baseline_job_id / candidate_job_id for compare mode\n"
      << "      target_id / target_ids    Lattice target identities\n"
      << "      fact_family_id            fact-family filter\n"
      << "      fact_digest / fact_digest_prefix / fact_index for fact "
         "preview\n"
      << "      include_facts / include_lineage / include_preview\n"
      << "      include_machine_payload   include raw panels for tooling\n"
      << "    Returns: compact operator report, Lattice quoted proof/fact "
         "panels,\n"
      << "      warnings, blockers, comparison deltas, and next safe actions.\n"
      << "    Example:\n"
      << "      " << argv0
      << " --global-config /cuwacunu/src/config/.config --tool "
         "hero.marshal.inspect --args-json "
         "'{\"subject\":\"run\",\"mode\":\"latest_chain\","
         "\"runtime_root\":\"/tmp/cuwacunu_runtime\"}'\n"
      << "\n"
      << "Operational notes:\n"
      << "  Use --list-tools-json when a client needs exact MCP schemas.\n"
      << "  Use --list-tools for a short catalog. Use --help for this "
         "runbook.\n"
      << "  Rollout execute mode delegates to hero.runtime.run "
         "operation=replay;\n"
      << "  Marshal\n"
      << "  records handoff state and digests, but Runtime remains the "
         "executor.\n"
      << "  Deeper docs: src/include/kikijyeba/marshal/README.md and\n"
      << "  src/include/kikijyeba/marshal/ROADMAP.md.\n";
}

} // namespace

int main(int argc, char **argv) {
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

  if (direct_tool_mode) {
    std::string result;
    std::string error;
    const bool ok = cuwacunu::hero::marshal::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &result, &error);
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

  cuwacunu::hero::marshal::run_jsonrpc_stdio_loop();
  return 0;
}
