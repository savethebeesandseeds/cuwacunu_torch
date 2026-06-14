// SPDX-License-Identifier: MIT

#include "hero/marshal_hero/marshal/paper_online_session_handoff.h"
#include "hero/marshal_hero/marshal/tool_handler.h"
#include "hero/mcp_cli_client.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace marshal = cuwacunu::hero::marshal;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_marshal_paper_online_handoff_" + label + "_" +
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

struct fixture_t {
  std::filesystem::path root;
  std::filesystem::path config_path;
  std::filesystem::path environment_policy_path;
  std::filesystem::path readiness_job_dir;
  std::filesystem::path session_root;
  std::filesystem::path receipt_path;
};

fixture_t write_fixture(const std::string &label) {
  fixture_t fixture{};
  fixture.root = make_tmp_dir(label);
  fixture.config_path = fixture.root / ".config";
  fixture.environment_policy_path = fixture.root / "hero.environment.dsl";
  fixture.readiness_job_dir = fixture.root / "jobs/paper_online_readiness";
  fixture.session_root = fixture.root / "jobs/paper_online_session/session_001";
  fixture.receipt_path =
      fixture.readiness_job_dir /
      "marshal.paper_online_session_handoff.paper_session_fixture_001.receipt."
      "json";

  write_text(fixture.config_path, "[HERO]\n"
                                  "environment_hero_dsl_path = " +
                                      fixture.environment_policy_path.string() +
                                      "\n");
  write_text(fixture.environment_policy_path,
             "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
             "environment_profile:enum = operator_default\n"
             "default_config_path:path = " +
                 fixture.config_path.string() +
                 "\n"
                 "runtime_root:path = " +
                 fixture.root.string() +
                 "\n"
                 "allowed_job_roots:path_list = " +
                 fixture.root.string() +
                 "\n"
                 "allow_certify_issue:bool = true\n"
                 "allow_rollout_replay:bool = true\n"
                 "allow_paper_online_session_run:bool = true\n"
                 "paper_online_session_max_steps:int = 8\n");
  write_text(fixture.readiness_job_dir / "job.manifest",
             "job_id=paper_online_readiness_job_fixture\n"
             "job_kind=channel_inference_mdn\n"
             "protocol_id=protocol_fixture\n"
             "protocol_contract_fingerprint=contract_fingerprint_fixture\n"
             "graph_order_fingerprint=graph_order_fixture\n"
             "target_component_family_id=kikijyeba.paper_online.readiness\n");
  write_text(fixture.readiness_job_dir / "job.state", "status=completed\n");
  write_text(fixture.readiness_job_dir / "lattice.paper_online_readiness.fact",
             paper_online_readiness_fact_text());
  return fixture;
}

marshal::marshal_paper_online_session_handoff_request_t
valid_request(const fixture_t &fixture) {
  marshal::marshal_paper_online_session_handoff_request_t request{};
  request.config_path = fixture.config_path;
  request.runtime_root = fixture.root;
  request.readiness_job_dir = fixture.readiness_job_dir;
  request.readiness_proof_certificate_digest =
      "digest:paper_online_readiness_proof_fixture";
  request.readiness_proof_checked_at_ms = 100000;
  request.max_readiness_proof_age_ms = 60000;
  request.admission_id = "session_admission_fixture";
  request.admission_requested_at_ms = 110000;
  request.session_id = "paper_session_fixture_001";
  request.session_root = fixture.session_root;
  request.session_requested_at_ms = 111000;
  request.max_steps = 3;
  request.step_interval_ms = 1000;
  request.market_data_receive_lag_ms = 100;
  request.target_node_ids = {"USDT", "BTC", "ETH"};
  request.recover_persistent_ledger = true;
  request.receipt_path = fixture.receipt_path;
  marshal::normalize_paper_online_session_handoff_request(&request);
  return request;
}

std::string handoff_request_json(
    const marshal::marshal_paper_online_session_handoff_request_t &request) {
  std::ostringstream out;
  out << "{\"schema\":"
      << json_quote(
             marshal::k_marshal_paper_online_session_handoff_request_schema_v1)
      << ",\"config_path\":" << json_quote(request.config_path.string())
      << ",\"runtime_root\":" << json_quote(request.runtime_root.string())
      << ",\"readiness\":{\"job_dir\":"
      << json_quote(request.readiness_job_dir.string())
      << ",\"target_id\":" << json_quote(request.readiness_target_id)
      << ",\"proof_certificate_digest\":"
      << json_quote(request.readiness_proof_certificate_digest)
      << ",\"proof_checked_at_ms\":" << request.readiness_proof_checked_at_ms
      << ",\"max_proof_age_ms\":" << request.max_readiness_proof_age_ms
      << "},\"session\":{\"admission_id\":" << json_quote(request.admission_id)
      << ",\"admission_requested_at_ms\":" << request.admission_requested_at_ms
      << ",\"session_id\":" << json_quote(request.session_id)
      << ",\"session_root\":" << json_quote(request.session_root.string())
      << ",\"session_requested_at_ms\":" << request.session_requested_at_ms
      << ",\"max_steps\":" << request.max_steps
      << ",\"step_interval_ms\":" << request.step_interval_ms
      << ",\"market_data_receive_lag_ms\":"
      << request.market_data_receive_lag_ms
      << ",\"target_node_ids\":[\"USDT\",\"BTC\",\"ETH\"]"
      << ",\"recover_persistent_ledger\":"
      << (request.recover_persistent_ledger ? "true" : "false") << "}"
      << ",\"receipt_path\":" << json_quote(request.receipt_path.string())
      << ",\"handoff_id\":" << json_quote(request.handoff_id)
      << ",\"timeout_seconds\":" << request.timeout_seconds << "}";
  return out.str();
}

std::string marshal_args(const std::string &mode,
                         const std::string &request_json,
                         bool include_machine_payload = false) {
  return "{\"mode\":" + json_quote(mode) +
         ",\"handoff_request\":" + request_json +
         ",\"include_machine_payload\":" +
         (include_machine_payload ? "true" : "false") + "}";
}

bool fake_lattice_summary_callback(const std::string &tool_name,
                                   const std::string &arguments_json,
                                   std::string *out_tool_result_json,
                                   std::string *out_error_message) {
  (void)arguments_json;
  if (tool_name != "hero.lattice.inspect.facts.summary") {
    if (out_error_message != nullptr) {
      *out_error_message = "unexpected lattice tool: " + tool_name;
    }
    return false;
  }
  if (out_tool_result_json != nullptr) {
    *out_tool_result_json =
        "{\"structuredContent\":{\"ok\":true,"
        "\"schema\":\"kikijyeba.lattice.fact_summary.v1\","
        "\"family\":\"paper_online_readiness\","
        "\"paper_online_readiness_fact_count\":1},\"isError\":false}";
  }
  return true;
}

void issue_environment_admission(
    const marshal::marshal_paper_online_session_handoff_request_t &request) {
  const std::string admission_request_json =
      marshal::paper_online_session_handoff_admission_request_json(request);
  const std::string check_args =
      "{\"mode\":\"check\",\"admission_request\":" + admission_request_json +
      "}";
  const auto check_call = cuwacunu::hero::mcp_cli::call_environment_tool(
      request.config_path, {},
      "hero.environment.certify.paper_online_session_admission", check_args,
      600, 1048576);
  check(check_call.process_ok,
        "Environment admission check process should succeed: " +
            check_call.error_message);
  check(check_call.result_json.find("\"admission_ready\":true") !=
            std::string::npos,
        "Environment admission check should be ready");
  const std::string preview_digest =
      json_string_field(check_call.result_json, "preview_digest");
  const std::string issue_args =
      "{\"mode\":\"issue\",\"admission_request\":" + admission_request_json +
      ",\"expected_preview_digest\":" + json_quote(preview_digest) + "}";
  const auto issue_call = cuwacunu::hero::mcp_cli::call_environment_tool(
      request.config_path, {},
      "hero.environment.certify.paper_online_session_admission", issue_args,
      600, 1048576);
  check(issue_call.process_ok,
        "Environment admission issue process should succeed: " +
            issue_call.error_message);
  check(issue_call.result_json.find(
            "\"paper_online_session_admission_fact_written\":true") !=
            std::string::npos,
        "Environment admission issue should write the admission fact");
}

void test_plan_and_dry_run_handoff() {
  const auto fixture = write_fixture("plan_and_dry_run");
  const auto request = valid_request(fixture);
  const std::string request_json = handoff_request_json(request);

  std::string result;
  std::string error;
  marshal::set_marshal_lattice_tool_callback(nullptr);
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("plan", request_json, true), &result, &error),
        "Marshal handoff plan should succeed: " + error);
  check(result.find("\"dispatch_state\":\"planned\"") != std::string::npos,
        "plan should report planned dispatch state");
  check(result.find("\"receipt_produced\":false") != std::string::npos,
        "plan should not write a receipt");
  check(result.find("\"environment_admission_check\":{\"attempted\":false") !=
            std::string::npos,
        "plan should not call Environment admission check");
  check(result.find("\"admission_request_json\":") != std::string::npos &&
            result.find("\"session_request_json\":") != std::string::npos,
        "plan machine payload should include Environment-compatible requests");
  check(!std::filesystem::exists(fixture.receipt_path),
        "plan should leave receipt path untouched");

  marshal::set_marshal_lattice_tool_callback(fake_lattice_summary_callback);
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("dry_run", request_json, true), &result, &error),
        "Marshal handoff dry-run before admission should succeed: " + error);
  check(result.find("\"dispatch_state\":\"admission_ready\"") !=
            std::string::npos,
        "dry-run before admission should stop at admission-ready");
  check(result.find("\"handoff_ready\":false") != std::string::npos,
        "dry-run before admission should not report handoff ready");
  check(result.find("\"environment_admission_check\":{\"attempted\":true") !=
                std::string::npos &&
            result.find("\"environment_admission_check\":{\"attempted\":true,"
                        "\"tool_name\":\"hero.environment.certify.paper_"
                        "online_session_admission\",\"ok\":true") !=
                std::string::npos,
        "dry-run should call Environment admission check successfully");
  check(result.find("\"environment_session_validate\":{\"attempted\":false") !=
            std::string::npos,
        "dry-run should not validate session before admission fact exists");
  check(result.find("\"receipt_produced\":true") != std::string::npos,
        "dry-run should write a receipt");
  check(std::filesystem::exists(fixture.receipt_path),
        "dry-run receipt should exist");
  const auto receipt_text = read_text(fixture.receipt_path);
  check(receipt_text.find("\"schema_version\":\"kikijyeba.marshal."
                          "paper_online_session_handoff_receipt.v1\"") !=
            std::string::npos,
        "receipt should record its schema");
  check(!std::filesystem::exists(fixture.session_root / "job.manifest"),
        "Marshal dry-run must not run the session");

  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("run", request_json, true), &result, &error),
        "Marshal handoff run before admission should return an auditable "
        "packet: " +
            error);
  check(result.find("\"requested_mode\":\"run\"") != std::string::npos,
        "run before admission should report run mode");
  check(result.find("\"dispatch_state\":\"admission_ready\"") !=
            std::string::npos,
        "run before admission should stop at admission-ready");
  check(result.find("\"session_executed\":false") != std::string::npos,
        "run before admission must not execute the session");
  check(result.find("\"environment_session_validate\":{\"attempted\":false") !=
            std::string::npos,
        "run before admission should not validate session");
  check(result.find("\"environment_session_run\":{\"attempted\":false") !=
            std::string::npos,
        "run before admission should not call Environment session run");
  check(!std::filesystem::exists(fixture.session_root / "job.manifest"),
        "run before admission must not create session artifacts");

  issue_environment_admission(request);

  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("dry_run", request_json, true), &result, &error),
        "Marshal handoff dry-run after admission should succeed: " + error);
  check(result.find("\"dispatch_state\":\"prepared\"") != std::string::npos,
        "dry-run after admission should prepare the handoff");
  check(result.find("\"handoff_ready\":true") != std::string::npos,
        "dry-run after admission should report handoff ready");
  check(result.find("\"environment_session_validate\":{\"attempted\":true") !=
                std::string::npos &&
            result.find("\"environment_session_validate\":{\"attempted\":true,"
                        "\"tool_name\":\"hero.environment.paper_online."
                        "session\",\"ok\":true") != std::string::npos,
        "dry-run after admission should validate the session successfully");
  check(!std::filesystem::exists(fixture.session_root / "job.manifest"),
        "Marshal prepared handoff must still not run the session");

  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("run", request_json, true), &result, &error),
        "Marshal handoff run after admission should execute through "
        "Environment: " +
            error);
  check(result.find("\"requested_mode\":\"run\"") != std::string::npos,
        "run after admission should report run mode");
  check(result.find("\"dispatch_state\":\"executed\"") != std::string::npos,
        "run after admission should report executed dispatch state: " + result);
  check(result.find("\"handoff_ready\":true") != std::string::npos,
        "run after admission should keep the handoff gate ready");
  check(result.find("\"session_executed\":true") != std::string::npos,
        "run after admission should report session execution");
  check(result.find("\"environment_session_validate\":{\"attempted\":true") !=
            std::string::npos,
        "run after admission should validate before execution");
  check(result.find("\"environment_session_run\":{\"attempted\":true") !=
            std::string::npos,
        "run after admission should call Environment session run");
  check(result.find("\"environment_session_run\":{\"attempted\":true,"
                    "\"tool_name\":\"hero.environment.paper_online."
                    "session\",\"ok\":true") != std::string::npos,
        "Environment session run should succeed");
  check(result.find("\"marshal_delegates_environment_paper_online_session_"
                    "run\":true") != std::string::npos,
        "Marshal run should record delegated Environment session execution");
  check(result.find("\"marshal_executes_broker_orders\":false") !=
            std::string::npos,
        "Marshal run must not claim broker execution authority");
  check(result.find("\"marshal_executes_live_capital\":false") !=
            std::string::npos,
        "Marshal run must not claim live-capital authority");
  check(std::filesystem::exists(fixture.session_root / "job.manifest"),
        "Marshal run should cause Environment to write session job manifest");
  check(std::filesystem::exists(fixture.session_root / "session.manifest"),
        "Marshal run should cause Environment to write session manifest");
  check(std::filesystem::exists(fixture.session_root /
                                "lattice.paper_online_session.fact"),
        "Marshal run should cause Environment to write session fact");
  const auto run_receipt_text = read_text(fixture.receipt_path);
  check(run_receipt_text.find("\"requested_mode\":\"run\"") !=
            std::string::npos,
        "run receipt should record run mode");
  check(run_receipt_text.find("\"dispatch_state\":\"executed\"") !=
            std::string::npos,
        "run receipt should record executed state");
  check(run_receipt_text.find(
            "\"environment_session_run\":{\"attempted\":true") !=
            std::string::npos,
        "run receipt should record Environment session run call");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_invalid_handoff_blocks_before_environment_call() {
  const auto fixture = write_fixture("invalid_blocks");
  auto request = valid_request(fixture);
  request.recover_persistent_ledger = false;
  const std::string request_json = handoff_request_json(request);

  marshal::set_marshal_lattice_tool_callback(fake_lattice_summary_callback);
  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("dry_run", request_json, false), &result, &error),
        "invalid handoff should return an auditable blocked packet: " + error);
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "invalid handoff should be blocked");
  check(result.find("recover_persistent_ledger_required") != std::string::npos,
        "invalid handoff should explain persistent-ledger recovery issue");
  check(result.find("\"environment_admission_check\":{\"attempted\":false") !=
            std::string::npos,
        "invalid local handoff should not call Environment admission check");
  check(!std::filesystem::exists(fixture.session_root / "job.manifest"),
        "invalid handoff must not run the session");

  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("run", request_json, false), &result, &error),
        "invalid run handoff should return an auditable blocked packet: " +
            error);
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "invalid run handoff should be blocked");
  check(result.find("\"environment_admission_check\":{\"attempted\":false") !=
            std::string::npos,
        "invalid run handoff should not call Environment admission check");
  check(result.find("\"environment_session_run\":{\"attempted\":false") !=
            std::string::npos,
        "invalid run handoff should not call Environment session run");
  check(!std::filesystem::exists(fixture.session_root / "job.manifest"),
        "invalid run handoff must not run the session");
  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_receipt_path_outside_readiness_job_is_blocked() {
  const auto fixture = write_fixture("receipt_path_blocked");
  auto request = valid_request(fixture);
  const auto outside_receipt = fixture.root / "outside_handoff.receipt.json";
  request.receipt_path = outside_receipt;
  const std::string request_json = handoff_request_json(request);

  marshal::set_marshal_lattice_tool_callback(fake_lattice_summary_callback);
  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.paper_online.session_handoff",
            marshal_args("dry_run", request_json, false), &result, &error),
        "unsafe receipt path should return an auditable blocked packet: " +
            error);
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "unsafe receipt path should be blocked");
  check(result.find("receipt_path_outside_readiness_job_dir") !=
            std::string::npos,
        "unsafe receipt path should explain the readiness-job boundary");
  check(result.find("\"receipt_produced\":false") != std::string::npos,
        "unsafe receipt path must not produce a receipt");
  check(!std::filesystem::exists(outside_receipt),
        "unsafe receipt path must not be written");
  marshal::set_marshal_lattice_tool_callback(nullptr);
}

} // namespace

int main() {
  try {
    test_plan_and_dry_run_handoff();
    test_invalid_handoff_blocks_before_environment_call();
    test_receipt_path_outside_readiness_job_is_blocked();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }
  return 0;
}
