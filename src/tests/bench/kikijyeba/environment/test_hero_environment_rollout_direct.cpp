// SPDX-License-Identifier: MIT

#include "hero/environment_hero/hero_environment_tools.h"
#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace environment = cuwacunu::hero::environment;
namespace exposure = cuwacunu::hero::lattice::exposure;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_environment_rollout_" + label + "_" +
                    std::to_string(static_cast<long long>(::getpid())));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open output file: " + path.string());
  out << text;
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  check(in.is_open(), "failed to read file: " + path.string());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string json_quote(const std::string &value) {
  std::ostringstream out;
  out << '"';
  for (const char c : value) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      out << c;
      break;
    }
  }
  out << '"';
  return out.str();
}

std::string json_string_field(const std::string &json,
                              const std::string &field) {
  const std::string prefix = "\"" + field + "\":\"";
  const auto begin = json.find(prefix);
  check(begin != std::string::npos, "missing JSON string field: " + field);
  std::size_t pos = begin + prefix.size();
  std::string out;
  while (pos < json.size()) {
    const char ch = json[pos++];
    if (ch == '"') {
      return out;
    }
    if (ch == '\\' && pos < json.size()) {
      out.push_back(json[pos++]);
    } else {
      out.push_back(ch);
    }
  }
  throw std::runtime_error("unterminated JSON string field: " + field);
}

std::string paper_online_readiness_fact_text() {
  return "schema=kikijyeba.lattice.paper_online_readiness.v1\n"
         "fact_type=paper_online_readiness\n"
         "parent_exposure_fact_digest=digest:parent_exposure_fixture\n"
         "contract_fingerprint=contract_fingerprint_fixture\n"
         "protocol_id=protocol_fixture\n"
         "graph_order_fingerprint=graph_order_fixture\n"
         "source_cursor_token=source_cursor_fixture\n"
         "split_policy_fingerprint=split_policy_fixture\n"
         "component_assembly_fingerprint=component_assembly_fixture\n"
         "target_component_family_id=kikijyeba.paper_online.readiness\n"
         "job_id=paper_online_readiness_job_fixture\n"
         "wave_id=paper_online_readiness\n"
         "job_status=completed\n"
         "wave_action=paper_online_readiness_contract\n"
         "split_name=validation_holdout\n"
         "split_role=validation\n"
         "anchor_begin=10\n"
         "anchor_end=20\n"
         "completed_anchor_begin=10\n"
         "completed_anchor_end=20\n"
         "readiness_id=paper_online_readiness_fixture_v1\n"
         "readiness_contract_id=paper_online_readiness_contract.v1\n"
         "policy_acceptance_fact_digest=digest:policy_acceptance_fixture\n"
         "policy_acceptance_target_id=policy_acceptance_contract_ready\n"
         "policy_acceptance_proof_certificate_digest="
         "digest:policy_acceptance_proof_fixture\n"
         "tsodao_settings_protection_fact_digest=digest:tsodao_fixture\n"
         "protected_settings_bundle_digest=digest:settings_bundle_fixture\n"
         "accepted_policy_id=policy_fixture\n"
         "accepted_policy_kind=reinforcement_learning\n"
         "accepted_policy_family_id=policy_family_fixture\n"
         "accepted_actor_checkpoint_digest=digest:actor_checkpoint_fixture\n"
         "accepted_checkpoint_digest=digest:checkpoint_fixture\n"
         "policy_quality_report_digest=digest:quality_report_fixture\n"
         "validation_rollout_report_digest=digest:validation_rollout_fixture\n"
         "reward_contract_id=kikijyeba.environment.reward."
         "post_execution_ledger_log_growth_cost_drawdown.v1\n"
         "execution_profile_digest=digest:execution_profile_fixture\n"
         "accounting_numeraire_node_id=USDT\n"
         "paper_online_environment_contract_id="
         "kikijyeba.environment.paper_online.v1\n"
         "paper_online_profile_digest=digest:paper_online_profile_fixture\n"
         "direct_edge_universe_digest=digest:direct_edge_universe_fixture\n"
         "direct_edge_universe_validation_policy_id="
         "direct_graph_node_pair_universe_required.v1\n"
         "missing_direct_pair_policy=skip_pair_warn\n"
         "synthetic_execution_market_policy_id="
         "synthetic_execution_markets_forbidden.v1\n"
         "locked_execution_profile_digest=digest:execution_profile_fixture\n"
         "persistent_paper_ledger_recovery_policy_id="
         "recover_persistent_paper_ledger_before_session.v1\n"
         "paper_ledger_storage_policy_id=durable_paper_ledger_artifacts.v1\n"
         "session_state_schema_id=kikijyeba.paper_online.session_state.v1\n"
         "session_lifecycle_policy_id=operator_start_stop_required.v1\n"
         "idempotency_policy_id=execution_intent_idempotency.v1\n"
         "execution_intent_id_policy=stable_intent_id_from_target_digest.v1\n"
         "duplicate_action_policy=reject_duplicate_action_digest.v1\n"
         "duplicate_execution_intent_policy="
         "reject_duplicate_execution_intent_id.v1\n"
         "clock_policy_id=monotonic_runtime_clock_required.v1\n"
         "timestamp_policy_id=exchange_event_time_then_runtime_receive.v1\n"
         "market_data_staleness_policy_id=max_receive_age_before_action.v1\n"
         "max_market_data_staleness_ms=5000\n"
         "clock_skew_tolerance_ms=250\n"
         "reward_report_artifact_policy_id="
         "durable_reward_and_report_artifacts.v1\n"
         "operator_abort_policy_id=operator_abort_closes_session.v1\n"
         "kill_switch_policy_id=kill_switch_no_new_intents.v1\n"
         "policy_acceptance_ready=true\n"
         "tsodao_settings_protection_ready=true\n"
         "accepted_policy_bound=true\n"
         "protected_settings_bound=true\n"
         "direct_edge_universe_validated=true\n"
         "locked_execution_profile_bound=true\n"
         "persistent_paper_ledger_recovery_bound=true\n"
         "idempotency_bound=true\n"
         "duplicate_action_protection_bound=true\n"
         "session_lifecycle_bound=true\n"
         "clock_timestamp_policy_bound=true\n"
         "market_data_staleness_bound=true\n"
         "reward_report_artifact_path_bound=true\n"
         "operator_abort_bound=true\n"
         "kill_switch_bound=true\n"
         "synthetic_execution_markets_allowed=false\n"
         "numeraire_fallback_allowed=false\n"
         "paper_online_execution_allowed=false\n"
         "live_execution_allowed=false\n"
         "broker_execution_allowed=false\n"
         "direct_policy_to_broker_allowed=false\n"
         "direct_policy_to_cajtucu_allowed=false\n"
         "artifact_evidence=true\n"
         "visibility_only=true\n"
         "paper_online_readiness_artifact=true\n"
         "policy_selector=false\n"
         "checkpoint_selector=false\n"
         "optimizer=false\n"
         "allocation_authority=false\n"
         "execution_authority=false\n"
         "readiness_authority=false\n"
         "quality_authority=false\n"
         "performance_authority=false\n"
         "market_readiness_authority=false\n"
         "deployment_authority=false\n"
         "live_execution_authority=false\n"
         "coverage_authority=false\n"
         "leakage_authority=false\n"
         "contract_identity_authority=false\n";
}

std::string paper_online_session_admission_request_json(
    const std::filesystem::path &job_dir, long long requested_at_ms,
    const std::string &readiness_fact_digest = {}) {
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.environment."
         "paper_online_session_admission_request.v1\","
      << "\"readiness\":{\"job_dir\":" << json_quote(job_dir.string())
      << ",\"target_id\":\"paper_online_readiness_contract_ready\","
         "\"proof_certificate_digest\":"
         "\"digest:paper_online_readiness_proof_fixture\","
         "\"proof_checked_at_ms\":100000,"
         "\"max_proof_age_ms\":60000";
  if (!readiness_fact_digest.empty()) {
    out << ",\"readiness_fact_digest\":" << json_quote(readiness_fact_digest);
  }
  out << "},\"session\":{\"admission_id\":\"session_admission_fixture\","
      << "\"requested_at_ms\":" << requested_at_ms << "}}";
  return out.str();
}

std::string paper_online_session_run_request_json(
    const std::filesystem::path &admission_job_dir,
    const std::filesystem::path &session_root, const std::string &session_id,
    long long requested_at_ms, long long max_steps = 3,
    long long market_data_receive_lag_ms = 100,
    bool recover_persistent_ledger = true,
    long long duplicate_action_at_step = -1,
    long long duplicate_execution_intent_at_step = -1) {
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.environment."
         "paper_online_session_run_request.v1\","
      << "\"admission\":{\"job_dir\":" << json_quote(admission_job_dir.string())
      << "},"
      << "\"session\":{\"session_id\":" << json_quote(session_id)
      << ",\"session_root\":" << json_quote(session_root.string())
      << ",\"requested_at_ms\":" << requested_at_ms
      << ",\"max_steps\":" << max_steps << ",\"step_interval_ms\":1000"
      << ",\"market_data_receive_lag_ms\":" << market_data_receive_lag_ms
      << ",\"target_node_ids\":[\"USDT\",\"BTC\",\"ETH\"]"
      << ",\"recover_persistent_ledger\":"
      << (recover_persistent_ledger ? "true" : "false")
      << ",\"operator_abort_at_step\":-1"
      << ",\"kill_switch_at_step\":-1"
      << ",\"duplicate_action_at_step\":" << duplicate_action_at_step
      << ",\"duplicate_execution_intent_at_step\":"
      << duplicate_execution_intent_at_step << "}}";
  return out.str();
}

void test_environment_rollout_direct_validate() {
  const auto root = make_tmp_dir("direct_validate");
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.environment.dsl";
  const auto runtime_exec = root / "bin/cuwacunu_exec";
  const auto job_dir = root / "jobs/rollout_ready";
  const auto replay_index = job_dir /
                            "artifacts/kikijyeba.environment.replay.v1/"
                            "runtime_replay_batches.index";

  write_text(runtime_exec, "#!/bin/sh\nexit 0\n");
  std::filesystem::permissions(runtime_exec,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write);
  write_text(replay_index, "bundle_count=2\n");
  write_text(job_dir / "job.manifest",
             "job_id=rollout_ready\n"
             "graph_order_fingerprint=graph_environment_rollout_direct\n");
  write_text(job_dir / "job.state", "status=completed\n"
                                    "replay_artifacts_written=true\n"
                                    "replay_batch_index_path=" +
                                        replay_index.string() + "\n");
  write_text(config_path, "[HERO]\n"
                          "environment_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "[ACCOUNTING]\n"
                              "accounting_numeraire_node_id = USDT\n");
  write_text(policy_path,
             "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
             "environment_profile:enum = operator_default\n"
             "default_config_path:path = " +
                 config_path.string() +
                 "\n"
                 "runtime_root:path = " +
                 root.string() +
                 "\n"
                 "allowed_job_roots:path_list = " +
                 root.string() +
                 "\n"
                 "allow_certify_issue:bool = true\n"
                 "allow_rollout_replay:bool = true\n"
                 "max_capture_bytes:int = 4096\n"
                 "max_runtime_seconds:int = 30\n"
                 "rollout_policy_set:string = equal_weight,sdu\n"
                 "rollout_max_steps:int = 25\n"
                 "rollout_max_parallel_jobs:int = 2\n"
                 "rollout_runtime_exec_path:path = " +
                 runtime_exec.string() +
                 "\n"
                 "rollout_execution_backend_id:string = "
                 "cajtucu.execution.paper.v1\n"
                 "rollout_cost_model_id:string = "
                 "linear_transaction_cost_rate.v1\n"
                 "rollout_allow_synthetic_direct_edges:bool = false\n"
                 "rollout_synthetic_edge_research_reason:string =\n"
                 "rollout_linear_transaction_cost_rate:number = 0.001\n"
                 "rollout_allow_partial_fills:bool = false\n"
                 "rollout_equity_mismatch_tolerance:number = 0.000001\n"
                 "rollout_equity_mismatch_fail_tolerance:number = 0.01\n"
                 "rollout_live_execution_allowed:bool = false\n");

  environment::environment_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(environment::load_environment_policy(policy_path, config_path,
                                             &ctx.policy, &error),
        "failed to load Environment policy: " + error);

  const std::string args =
      "{\"mode\":\"validate\","
      "\"runtime_job_dir\":" +
      json_quote(job_dir.string()) +
      ",\"rollout_id\":\"environment_rollout_direct_v1\","
      "\"rollout_attempt_id\":\"environment_rollout_direct_v1_attempt_001\","
      "\"target_node_ids\":[\"USDT\",\"BTC\",\"ETH\"]}";

  std::string result;
  error.clear();
  check(environment::execute_tool_json("hero.environment.rollout", args, &ctx,
                                       &result, &error),
        "direct Environment rollout validate should succeed: " + error);
  check(result.find("\"tool\":\"hero.environment.rollout\"") !=
            std::string::npos,
        "direct Environment rollout response should name the tool");
  check(result.find("\"requested_mode\":\"validate\"") != std::string::npos,
        "direct Environment rollout should preserve validate mode");
  check(result.find("\"dispatch_state\":\"validated\"") != std::string::npos,
        "direct Environment rollout should validate the completed Runtime job");
  check(result.find("\"rollout_request_digest\":") != std::string::npos,
        "direct Environment rollout should report derived request digest");
  check(result.find("\"args_path\"") == std::string::npos &&
            result.find("\"args_digest\"") == std::string::npos,
        "direct Environment rollout should not expose retired file-backed "
        "arguments");
  check(result.find("\"runtime_replay\":{\"attempted\":false") !=
            std::string::npos,
        "validate mode should not attempt Runtime replay");
}

void test_environment_schema_inspection_reports_paper_online_session_contract() {
  const auto root = make_tmp_dir("schema_inspection");
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.environment.dsl";
  write_text(config_path, "[HERO]\n"
                          "environment_hero_dsl_path = " +
                              policy_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "environment_profile:enum = operator_default\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              root.string() +
                              "\n"
                              "allow_certify_issue:bool = true\n"
                              "allow_rollout_replay:bool = true\n");

  environment::environment_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(environment::load_environment_policy(policy_path, config_path,
                                             &ctx.policy, &error),
        "failed to load Environment policy for schema inspection: " + error);

  std::string result;
  error.clear();
  check(environment::execute_tool_json("hero.environment.inspect.schema", "{}",
                                       &ctx, &result, &error),
        "Environment schema inspection should succeed: " + error);
  check(result.find("\"paper_online_session_contract\":") != std::string::npos,
        "Environment schema inspection exposes paper-online session contract");
  check(result.find("\"contract_id\":\"paper_online_session_contract.v1\"") !=
            std::string::npos,
        "Environment schema inspection exposes paper-online session contract "
        "id");
  check(result.find("\"readiness_target_id\":"
                    "\"paper_online_readiness_contract_ready\"") !=
            std::string::npos,
        "Environment schema inspection exposes readiness target prerequisite");
  check(result.find("\"relative_path\":\"session.state\"") != std::string::npos,
        "Environment schema inspection exposes durable session state path");
  check(result.find("\"session_runner_implemented\":false") !=
            std::string::npos,
        "Environment schema inspection keeps admission contract from claiming "
        "runner authority");
  check(result.find("\"broker_execution_allowed\":false") != std::string::npos,
        "Environment schema inspection denies broker execution authority");
  check(result.find("\"validator_issues\":[]") != std::string::npos,
        "Environment schema inspection reports clean default session "
        "contract");
  check(result.find("\"session_runner_contract\":") != std::string::npos,
        "Environment schema inspection exposes paper-online runner contract");
  check(result.find("\"runner_contract_id\":\"paper_online_session_runner."
                    "v1\"") != std::string::npos,
        "Environment schema inspection exposes runner contract id");
  check(result.find("\"execution_backend_id\":\"cajtucu.execution.paper."
                    "v1\"") != std::string::npos,
        "Environment schema inspection binds runner to Cajtucu paper "
        "execution");
  check(result.find("\"paper_execution_allowed\":true") != std::string::npos,
        "Environment schema inspection allows bounded paper execution in the "
        "runner contract");
  const auto tools_json = environment::build_tools_list_result_json();
  check(tools_json.find("\"hero.environment.certify.paper_online_session_"
                        "admission\"") != std::string::npos,
        "Environment tool list exposes paper-online session admission "
        "certification");
  check(tools_json.find("\"hero.environment.paper_online.session\"") !=
            std::string::npos,
        "Environment tool list exposes bounded paper-online session runner");
  check(tools_json.find("\"hero.environment.admit.paper_online_session\"") ==
            std::string::npos,
        "Environment tool list does not expose the retired admit verb");
}

void test_environment_paper_online_session_admission_check() {
  const auto root = make_tmp_dir("paper_online_session_admission");
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.environment.dsl";
  const auto job_dir = root / "jobs/paper_online_readiness";
  write_text(config_path, "[HERO]\n"
                          "environment_hero_dsl_path = " +
                              policy_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "environment_profile:enum = operator_default\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              root.string() +
                              "\n"
                              "allow_certify_issue:bool = true\n"
                              "allow_rollout_replay:bool = true\n"
                              "allow_paper_online_session_run:bool = true\n"
                              "paper_online_session_max_steps:int = 8\n");
  write_text(job_dir / "job.manifest",
             "job_id=paper_online_readiness_job_fixture\n"
             "job_kind=channel_inference_mdn\n"
             "protocol_id=protocol_fixture\n"
             "protocol_contract_fingerprint=contract_fingerprint_fixture\n"
             "graph_order_fingerprint=graph_order_fixture\n"
             "target_component_family_id=kikijyeba.paper_online.readiness\n");
  write_text(job_dir / "job.state", "status=completed\n");
  write_text(job_dir / "lattice.paper_online_readiness.fact",
             paper_online_readiness_fact_text());

  environment::environment_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(environment::load_environment_policy(policy_path, config_path,
                                             &ctx.policy, &error),
        "failed to load Environment policy for admission check: " + error);

  const auto request =
      paper_online_session_admission_request_json(job_dir, 110000);
  const std::string args =
      "{\"mode\":\"check\",\"admission_request\":" + request + "}";

  std::string result;
  error.clear();
  check(environment::execute_tool_json("hero.environment.certify.paper_online_"
                                       "session_admission",
                                       args, &ctx, &result, &error),
        "paper-online session admission check should succeed: " + error);
  check(result.find("\"tool\":\"hero.environment.certify.paper_online_session_"
                    "admission\"") != std::string::npos,
        "admission response should name the tool");
  check(result.find("\"requested_mode\":\"check\"") != std::string::npos,
        "admission check should report check mode");
  check(result.find("\"paper_online_session_admission_fact_written\":false") !=
            std::string::npos,
        "admission check should not write the sidecar");
  check(result.find("\"admission_ready\":true") != std::string::npos,
        "fresh readiness evidence should pass admission");
  check(result.find("\"readiness_fact_issues\":[]") != std::string::npos,
        "fresh readiness sidecar should have no fact issues");
  check(result.find("\"issues\":[]") != std::string::npos,
        "fresh admission should have no issues");
  check(result.find("\"environment_runs_paper_online_session\":false") !=
            std::string::npos,
        "admission check must not run paper-online sessions");
  check(result.find("\"environment_executes_broker_orders\":false") !=
            std::string::npos,
        "admission check must not authorize broker execution");
  check(result.find("\"preview_digest\":") != std::string::npos,
        "admission check should return a preview digest");
  const auto preview_digest = json_string_field(result, "preview_digest");

  const std::string issue_args =
      "{\"mode\":\"issue\",\"admission_request\":" + request +
      ",\"expected_preview_digest\":" + json_quote(preview_digest) + "}";
  result.clear();
  error.clear();
  check(environment::execute_tool_json("hero.environment.certify.paper_online_"
                                       "session_admission",
                                       issue_args, &ctx, &result, &error),
        "paper-online session admission issue should succeed: " + error);
  check(result.find("\"requested_mode\":\"issue\"") != std::string::npos,
        "admission issue should report issue mode");
  check(result.find("\"paper_online_session_admission_fact_written\":true") !=
            std::string::npos,
        "admission issue should write the sidecar");
  const auto admission_fact_path =
      job_dir / "lattice.paper_online_session_admission.fact";
  check(std::filesystem::exists(admission_fact_path),
        "admission issue should create a durable sidecar");
  const auto admission_fact_text = read_text(admission_fact_path);
  check(admission_fact_text.find(
            "schema=kikijyeba.lattice.paper_online_session_admission.v1") !=
            std::string::npos,
        "admission sidecar records its schema");
  check(admission_fact_text.find("admission_id=session_admission_fixture") !=
            std::string::npos,
        "admission sidecar records admission id");
  check(admission_fact_text.find("readiness_fact_digest=") != std::string::npos,
        "admission sidecar records readiness fact digest");

  const auto session_root = root / "jobs/paper_online_session/session_001";
  const auto session_request = paper_online_session_run_request_json(
      job_dir, session_root, "paper_session_fixture_001", 111000);
  const auto session_validate_args =
      "{\"mode\":\"validate\",\"session_request\":" + session_request +
      ",\"include_machine_payload\":true}";
  result.clear();
  error.clear();
  check(environment::execute_tool_json("hero.environment.paper_online.session",
                                       session_validate_args, &ctx, &result,
                                       &error),
        "paper-online session validate should succeed: " + error);
  check(result.find("\"requested_mode\":\"validate\"") != std::string::npos,
        "session validate should report validate mode");
  check(result.find("\"session_ready\":true") != std::string::npos,
        "admitted readiness evidence should allow a paper session");
  check(result.find("\"session_artifacts_written\":false") != std::string::npos,
        "session validate must not write artifacts");
  check(result.find("\"environment_runs_bounded_paper_session\":false") !=
            std::string::npos,
        "session validate must not run the bounded paper session");
  check(result.find("\"environment_executes_broker_orders\":false") !=
            std::string::npos,
        "session validate must not authorize broker execution");
  check(result.find("\"environment_executes_live_capital\":false") !=
            std::string::npos,
        "session validate must not authorize live capital");
  check(result.find("\"session_request_target_node_ids\":[\"USDT\",\"BTC\","
                    "\"ETH\"]") != std::string::npos,
        "session validate should echo target nodes when machine payload is "
        "requested");

  const auto session_run_args =
      "{\"mode\":\"run\",\"session_request\":" + session_request + "}";
  result.clear();
  error.clear();
  check(environment::execute_tool_json("hero.environment.paper_online.session",
                                       session_run_args, &ctx, &result, &error),
        "paper-online session run should succeed: " + error);
  check(result.find("\"requested_mode\":\"run\"") != std::string::npos,
        "session run should report run mode");
  check(result.find("\"session_ready\":true") != std::string::npos,
        "session run should report the request as ready");
  check(result.find("\"session_artifacts_written\":true") != std::string::npos,
        "session run should write artifacts");
  check(result.find("\"terminal_state\":\"stopped\"") != std::string::npos,
        "session run should stop cleanly");
  check(result.find("\"completed_step_count\":3") != std::string::npos,
        "session run should complete the bounded step count");
  check(result.find("\"environment_runs_bounded_paper_session\":true") !=
            std::string::npos,
        "session run should report bounded paper execution");
  check(result.find("\"environment_executes_cajtucu_paper\":true") !=
            std::string::npos,
        "session run should bind execution to Cajtucu paper");
  check(result.find("\"environment_executes_broker_orders\":false") !=
            std::string::npos,
        "session run must not authorize broker execution");
  check(result.find("\"environment_executes_live_capital\":false") !=
            std::string::npos,
        "session run must not authorize live capital");
  check(std::filesystem::exists(session_root / "session.manifest"),
        "session run should write session manifest");
  check(std::filesystem::exists(session_root / "session.state"),
        "session run should write session state");
  check(std::filesystem::exists(session_root / "session.events.lls"),
        "session run should write session events");
  check(std::filesystem::exists(session_root / "market_events.lls"),
        "session run should write market events");
  check(std::filesystem::exists(session_root / "action_intents.lls"),
        "session run should write action intents");
  check(std::filesystem::exists(session_root / "execution_intents.lls"),
        "session run should write execution intents");
  check(std::filesystem::exists(session_root / "paper_ledger.lls"),
        "session run should write paper ledger");
  check(std::filesystem::exists(session_root / "reward_reports.lls"),
        "session run should write reward reports");
  check(std::filesystem::exists(session_root / "lattice.exposure.fact"),
        "session run should write Lattice exposure fact");
  check(std::filesystem::exists(session_root /
                                "lattice.paper_online_session.fact"),
        "session run should write Lattice session fact");
  const auto session_state_text = read_text(session_root / "session.state");
  check(session_state_text.find("terminal_state=stopped") != std::string::npos,
        "session state should record stopped terminal state");
  check(session_state_text.find("completed_step_count=3") != std::string::npos,
        "session state should record completed step count");
  const auto session_fact_text =
      read_text(session_root / "lattice.paper_online_session.fact");
  check(session_fact_text.find(
            "schema=kikijyeba.lattice.paper_online_session.v1") !=
            std::string::npos,
        "session fact should record its schema");
  check(session_fact_text.find("session_runner_contract_id="
                               "paper_online_session_runner.v1") !=
            std::string::npos,
        "session fact should bind runner contract");
  check(session_fact_text.find("broker_execution_allowed=false") !=
            std::string::npos,
        "session fact should deny broker execution");
  check(session_fact_text.find("live_execution_allowed=false") !=
            std::string::npos,
        "session fact should deny live execution");

  const auto parent_exposure_fact =
      exposure::make_exposure_fact_from_sidecar_text(
          read_text(session_root / "lattice.exposure.fact"), session_root);
  const auto session_facts =
      exposure::make_paper_online_session_facts_from_job_dir(
          session_root, parent_exposure_fact);
  check(!session_facts.empty(),
        "Lattice exposure helpers should discover the paper-online session "
        "fact");
  std::vector<std::string> session_fact_issues;
  bool session_fact_found = false;
  for (const auto &fact : session_facts) {
    if (fact.session_id == "paper_session_fixture_001") {
      session_fact_found = true;
      session_fact_issues = exposure::paper_online_session_fact_issues(fact);
    }
  }
  check(session_fact_found,
        "Lattice exposure scan should discover the paper-online session fact");
  check(session_fact_issues.empty(),
        "scanned paper-online session fact should validate cleanly");

  result.clear();
  error.clear();
  check(!environment::execute_tool_json("hero.environment.paper_online.session",
                                        session_run_args, &ctx, &result,
                                        &error),
        "duplicate paper-online session run should fail");
  check(error.find("E_ENVIRONMENT_PAPER_ONLINE_SESSION_JOB_EXISTS") !=
            std::string::npos,
        "duplicate paper-online session run should refuse overwrite");

  const auto duplicate_action_request = paper_online_session_run_request_json(
      job_dir, root / "jobs/paper_online_session/duplicate_action",
      "paper_session_duplicate_action", 111000, 3, 100, true, 1);
  result.clear();
  error.clear();
  check(environment::execute_tool_json(
            "hero.environment.paper_online.session",
            "{\"mode\":\"validate\",\"session_request\":" +
                duplicate_action_request + "}",
            &ctx, &result, &error),
        "duplicate-action session validate should return a packet: " + error);
  check(result.find("\"session_ready\":false") != std::string::npos,
        "duplicate action digest should block session readiness");
  check(result.find("duplicate_action_digest_rejected") != std::string::npos,
        "duplicate action validation should report its policy issue");

  const auto stale_market_request = paper_online_session_run_request_json(
      job_dir, root / "jobs/paper_online_session/stale_market",
      "paper_session_stale_market", 111000, 3, 6000);
  result.clear();
  error.clear();
  check(environment::execute_tool_json(
            "hero.environment.paper_online.session",
            "{\"mode\":\"validate\",\"session_request\":" +
                stale_market_request + "}",
            &ctx, &result, &error),
        "stale-market session validate should return a packet: " + error);
  check(result.find("\"session_ready\":false") != std::string::npos,
        "market data staleness should block session readiness");
  check(result.find("market_data_staleness_exceeded") != std::string::npos,
        "stale market validation should report its policy issue");

  const auto unrecovered_ledger_request = paper_online_session_run_request_json(
      job_dir, root / "jobs/paper_online_session/unrecovered_ledger",
      "paper_session_unrecovered_ledger", 111000, 3, 100, false);
  result.clear();
  error.clear();
  check(environment::execute_tool_json(
            "hero.environment.paper_online.session",
            "{\"mode\":\"validate\",\"session_request\":" +
                unrecovered_ledger_request + "}",
            &ctx, &result, &error),
        "unrecovered-ledger session validate should return a packet: " + error);
  check(result.find("\"session_ready\":false") != std::string::npos,
        "missing ledger recovery should block session readiness");
  check(result.find("persistent_paper_ledger_recovery_missing") !=
            std::string::npos,
        "unrecovered ledger validation should report its policy issue");

  result.clear();
  error.clear();
  check(!environment::execute_tool_json("hero.environment.certify.paper_online_"
                                        "session_admission",
                                        issue_args, &ctx, &result, &error),
        "duplicate admission issue should fail");
  check(
      error.find("E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_FACT_EXISTS") !=
          std::string::npos,
      "duplicate admission issue should refuse overwrite");

  const auto request_path = root / "requests/session_admission.request.json";
  write_text(request_path, request);
  result.clear();
  error.clear();
  check(environment::execute_tool_json(
            "hero.environment.certify.paper_online_session_admission",
            "{\"mode\":\"check\",\"admission_request_path\":" +
                json_quote(request_path.string()) + "}",
            &ctx, &result, &error),
        "paper-online session admission path check should succeed: " + error);
  check(result.find("\"admission_ready\":true") != std::string::npos,
        "request path admission should pass");

  const auto stale_request =
      paper_online_session_admission_request_json(job_dir, 170001);
  result.clear();
  error.clear();
  check(environment::execute_tool_json(
            "hero.environment.certify.paper_online_session_admission",
            "{\"mode\":\"check\",\"admission_request\":" + stale_request + "}",
            &ctx, &result, &error),
        "stale paper-online session admission check should return a packet: " +
            error);
  check(result.find("\"admission_ready\":false") != std::string::npos,
        "stale readiness proof should fail admission");
  check(result.find("stale_readiness_proof") != std::string::npos,
        "stale admission should report stale_readiness_proof");

  const auto mismatch_request = paper_online_session_admission_request_json(
      job_dir, 110000, "digest:not_the_readiness_fact");
  result.clear();
  error.clear();
  check(
      environment::execute_tool_json(
          "hero.environment.certify.paper_online_session_admission",
          "{\"mode\":\"check\",\"admission_request\":" + mismatch_request + "}",
          &ctx, &result, &error),
      "mismatched readiness digest admission check should return a packet: " +
          error);
  check(result.find("\"admission_ready\":false") != std::string::npos,
        "mismatched readiness fact digest should fail admission");
  check(result.find("readiness_fact_digest_mismatch") != std::string::npos,
        "mismatched admission should report readiness_fact_digest_mismatch");
}

} // namespace

int main() {
  try {
    test_environment_rollout_direct_validate();
    test_environment_schema_inspection_reports_paper_online_session_contract();
    test_environment_paper_online_session_admission_check();
    std::cout << "hero Environment rollout direct tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
