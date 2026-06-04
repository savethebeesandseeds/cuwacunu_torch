#include "kikijyeba/marshal/rollout_marshal.h"
#include "kikijyeba/marshal/tool_handler.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace marshal = cuwacunu::kikijyeba::marshal;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_marshal_rollout_" + label + "_" +
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

bool has_refusal(const std::vector<std::string> &refusals,
                 const std::string &token) {
  return std::find(refusals.begin(), refusals.end(), token) != refusals.end();
}

marshal::marshal_rollout_request_t
valid_rollout_request(const std::filesystem::path &root) {
  const auto runtime_exec = root / "bin/cuwacunu_exec";
  const auto job_dir = root / "jobs/mdn_validation";
  const auto replay_index = job_dir /
                            "artifacts/kikijyeba.environment.replay.v1/"
                            "runtime_replay_batches.index";
  const std::string graph_order_fingerprint = "graph_rollout_001";
  write_text(runtime_exec, "#!/bin/sh\nexit 0\n");
  write_text(replay_index, "bundle_count=4\n");
  write_text(job_dir / "job.manifest",
             "job_id=mdn_validation\n"
             "graph_order_fingerprint=" +
                 graph_order_fingerprint +
                 "\n"
                 "target_component_family_id=wikimyei.inference.expected_value."
                 "mdn\n");
  write_text(job_dir / "job.state", "status=completed\n"
                                    "replay_artifacts_written=true\n"
                                    "replay_batch_index_path=" +
                                        replay_index.string() + "\n");

  marshal::marshal_rollout_request_t request{};
  request.rollout_id = "rollout_validation_costed_001";
  request.rollout_attempt_id = "rollout_validation_costed_001_attempt_001";
  request.idempotency_key =
      "rollout_validation_costed_001_attempt_001_idempotency";
  request.experiment_id = "validation_costed_001";
  request.runtime_job_dir = job_dir;
  request.replay_batch_index_path = replay_index;
  request.runtime_exec_path = runtime_exec;
  request.report_path = root / "reports/validation_costed_001.report";
  request.graph_order_fingerprint = graph_order_fingerprint;
  request.base_reserve_node_id = "USDT";
  request.risky_node_ids = {"BTC", "ETH"};
  request.asset_universe_digest = marshal::rollout_asset_universe_digest(
      request.base_reserve_node_id, request.risky_node_ids);
  request.policy_tokens = {"base_reserve", "current_weight", "equal_weight",
                           "sdu"};
  request.max_steps = 250;
  request.max_parallel_jobs = 4;
  request.execution_profile.linear_transaction_cost_rate = 0.001;
  request.execution_profile.allow_synthetic_direct_edges = true;
  request.execution_profile.synthetic_edge_research_reason =
      "research replay fixture without direct execution edge feed";
  return request;
}

void test_rollout_plan_accepts_completed_runtime_job() {
  const auto root = make_tmp_dir("accepted");
  const auto request = valid_rollout_request(root);
  const auto plan = marshal::prepare_rollout_plan(request);

  check(plan.accepted, "valid rollout should be accepted");
  check(plan.refusal_reasons.empty(), "valid rollout should have no refusals");
  check(!plan.target_satisfaction_claimed,
        "rollout plan must not claim target satisfaction");
  check(!plan.runtime_executor, "rollout plan must not execute Runtime");
  check(!plan.lattice_proof_authority,
        "rollout plan must not be Lattice proof authority");
  check(!plan.policy_training_authority,
        "rollout plan must not train policies");
  check(!plan.live_execution_authority,
        "rollout plan must not authorize live execution");
  check(marshal::marshal_digest_is_strong_hex(plan.request_digest),
        "request digest should be strong hex");
  check(marshal::marshal_digest_is_strong_hex(plan.plan_digest),
        "plan digest should be strong hex");
  check(marshal::marshal_digest_is_strong_hex(plan.execution_profile_digest),
        "execution profile digest should be strong hex");
  check(marshal::marshal_digest_is_strong_hex(plan.policy_set_digest),
        "policy set digest should be strong hex");
  check(plan.requested_mode == "plan",
        "rollout plan should use plan mode by default");
  check(plan.rollout_attempt_id == "rollout_validation_costed_001_attempt_001",
        "rollout plan should preserve attempt identity");
  check(plan.idempotency_key ==
            "rollout_validation_costed_001_attempt_001_idempotency",
        "rollout plan should preserve idempotency key");
  check(plan.replay_batch_index_path == request.replay_batch_index_path,
        "rollout plan should preserve replay batch index path");
  check(plan.graph_order_fingerprint == request.graph_order_fingerprint,
        "rollout plan should preserve graph fingerprint");
  check(plan.asset_universe_digest == request.asset_universe_digest,
        "rollout plan should preserve asset-universe digest");
  check(plan.resolved_policy_ids.size() == 4,
        "policy tokens should resolve to four policies");
  check(plan.replay_command_template.find("--replay-from-job-dir") !=
            std::string::npos,
        "rollout command should use replay-from-job-dir");
  check(plan.replay_command_template.find("--config") != std::string::npos,
        "rollout command should bind Runtime config path");
  check(plan.replay_command_template.find("--replay-experiment-id "
                                          "'validation_costed_001'") !=
            std::string::npos,
        "rollout command should bind experiment id");
  check(plan.replay_command_template.find("--replay-report-path") !=
            std::string::npos,
        "rollout command should bind report path");
  check(plan.replay_command_template.find(
            "--replay-base-reserve-node 'USDT'") != std::string::npos,
        "rollout command should bind graph reserve node");
  check(plan.replay_command_template.find("--replay-risky-nodes 'BTC,ETH'") !=
            std::string::npos,
        "rollout command should bind risky node universe");
  check(plan.replay_command_template.find("--replay-include-equal-weight") !=
            std::string::npos,
        "rollout command should include equal-weight policy");
  check(plan.replay_command_template.find("--replay-include-current-weight") !=
            std::string::npos,
        "rollout command should include current-weight policy");
  check(plan.replay_command_template.find(
            "--replay-allow-synthetic-direct-edges") != std::string::npos,
        "synthetic execution edges must be explicit in the command");
  check(plan.replay_command_template.find(
            "--replay-linear-transaction-cost-rate 0.001") != std::string::npos,
        "rollout command should carry Cajtucu cost profile");
  check(plan.replay_command_template.find(
            "--replay-execution-profile-digest") != std::string::npos,
        "rollout command should carry execution profile digest");
  check(plan.replay_command_template.find("--replay-policy-set-digest") !=
            std::string::npos,
        "rollout command should carry policy set digest");

  const auto plan_json = marshal::rollout_plan_json(plan);
  check(plan_json.find("\"receipt_produced\"") == std::string::npos,
        "rollout plan JSON must not pretend to be a receipt");
  check(plan_json.find("\"accepted\":true") != std::string::npos,
        "rollout plan JSON should mark accepted plan");
  check(plan_json.find("\"rollout_attempt_id\":"
                       "\"rollout_validation_costed_001_attempt_001\"") !=
            std::string::npos,
        "rollout plan JSON should expose attempt identity");
  check(plan_json.find("\"idempotency_key\":"
                       "\"rollout_validation_costed_001_attempt_001_"
                       "idempotency\"") != std::string::npos,
        "rollout plan JSON should expose idempotency key");
  check(plan_json.find("\"replay_batch_index_path\"") != std::string::npos,
        "rollout plan JSON should expose replay batch index path");
  check(plan_json.find("\"graph_order_fingerprint\":\"graph_rollout_001\"") !=
            std::string::npos,
        "rollout plan JSON should expose graph fingerprint");
  check(plan_json.find("\"asset_universe_digest\":\"" +
                       request.asset_universe_digest + "\"") !=
            std::string::npos,
        "rollout plan JSON should expose asset-universe digest");
  check(plan_json.find("\"cost_model_id\":"
                       "\"linear_transaction_cost_rate.v1\"") !=
            std::string::npos,
        "rollout plan JSON should expose Cajtucu cost model id");
  check(plan_json.find("\"execution_profile_digest\"") != std::string::npos,
        "rollout plan JSON should expose execution profile digest");
  check(plan_json.find("\"policy_set_digest\"") != std::string::npos,
        "rollout plan JSON should expose policy set digest");
  check(plan_json.find("\"idempotency\":{\"scope\":"
                       "\"request_digest_binding\"") != std::string::npos &&
            plan_json.find("\"durable_duplicate_handoff_ledger\":false") !=
                std::string::npos,
        "rollout plan JSON should expose honest idempotency scope");
  check(plan_json.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "rollout plan JSON should expose non-authority flags");
}

void test_rollout_requires_public_identity_fields() {
  const auto root = make_tmp_dir("identity_fields");
  auto request = valid_rollout_request(root);

  auto missing_attempt = request;
  missing_attempt.rollout_attempt_id.clear();
  auto refusals = marshal::validate_rollout_request(missing_attempt);
  check(has_refusal(refusals, "missing_rollout_attempt_id"),
        "rollout should require rollout_attempt_id");

  auto missing_idempotency = request;
  missing_idempotency.idempotency_key.clear();
  refusals = marshal::validate_rollout_request(missing_idempotency);
  check(has_refusal(refusals, "missing_idempotency_key"),
        "rollout should require idempotency_key");

  auto missing_replay_batch = request;
  missing_replay_batch.replay_batch_index_path.clear();
  refusals = marshal::validate_rollout_request(missing_replay_batch);
  check(has_refusal(refusals, "missing_replay_batch_index_path"),
        "rollout should require replay_batch_index_path");
  check(has_refusal(refusals, "replay_batch_index_missing"),
        "missing replay batch path should fail artifact validation");

  auto missing_graph = request;
  missing_graph.graph_order_fingerprint.clear();
  refusals = marshal::validate_rollout_request(missing_graph);
  check(has_refusal(refusals, "missing_graph_order_fingerprint"),
        "rollout should require graph_order_fingerprint");

  auto missing_asset_universe = request;
  missing_asset_universe.asset_universe_digest.clear();
  refusals = marshal::validate_rollout_request(missing_asset_universe);
  check(has_refusal(refusals, "missing_asset_universe_digest"),
        "rollout should require asset_universe_digest");

  auto missing_policy_set = request;
  missing_policy_set.policy_tokens.clear();
  refusals = marshal::validate_rollout_request(missing_policy_set);
  check(has_refusal(refusals, "missing_policy_set") &&
            has_refusal(refusals, "empty_policy_set"),
        "rollout should require an explicit non-empty policy_set");

  auto missing_max_steps = request;
  missing_max_steps.max_steps = 0;
  refusals = marshal::validate_rollout_request(missing_max_steps);
  check(has_refusal(refusals, "missing_max_steps") &&
            has_refusal(refusals, "nonpositive_max_steps"),
        "rollout should require a positive finite max_steps");

  auto negative_max_steps = request;
  negative_max_steps.max_steps = -1;
  refusals = marshal::validate_rollout_request(negative_max_steps);
  check(has_refusal(refusals, "nonpositive_max_steps"),
        "rollout should reject negative max_steps");
}

void test_rollout_validates_graph_and_asset_universe_identity() {
  const auto root = make_tmp_dir("graph_asset_identity");
  auto request = valid_rollout_request(root);

  auto graph_mismatch = request;
  graph_mismatch.graph_order_fingerprint = "other_graph";
  auto refusals = marshal::validate_rollout_request(graph_mismatch);
  check(has_refusal(refusals, "runtime_job_graph_order_fingerprint_mismatch"),
        "rollout should reject Runtime job graph fingerprint mismatch");

  auto asset_mismatch = request;
  asset_mismatch.asset_universe_digest = std::string(64, '0');
  refusals = marshal::validate_rollout_request(asset_mismatch);
  check(has_refusal(refusals, "asset_universe_digest_mismatch"),
        "rollout should reject asset-universe digest mismatch");
}

void test_rollout_validates_typed_execution_profile() {
  const auto root = make_tmp_dir("typed_profile");
  auto request = valid_rollout_request(root);

  auto unsupported_cost_model = request;
  unsupported_cost_model.execution_profile.cost_model_id =
      "nonlinear_cost_model.v1";
  auto refusals = marshal::validate_rollout_request(unsupported_cost_model);
  check(has_refusal(refusals, "unsupported_cost_model_id"),
        "rollout should reject unsupported Cajtucu cost model id");

  auto zero_cost_validation = request;
  zero_cost_validation.execution_profile.linear_transaction_cost_rate = 0.0;
  refusals = marshal::validate_rollout_request(zero_cost_validation);
  check(has_refusal(refusals, "nonzero_cost_required_for_validation_rollout"),
        "cost-aware validation rollout should reject zero-cost profiles");

  auto partial_fills = request;
  partial_fills.execution_profile.allow_partial_fills = true;
  refusals = marshal::validate_rollout_request(partial_fills);
  check(
      has_refusal(refusals, "partial_fills_not_supported_by_runtime_replay_v1"),
      "rollout should reject partial fills until Runtime replay supports "
      "forwarding them");
}

void test_rollout_rejects_legacy_dry_run_mode() {
  const auto root = make_tmp_dir("legacy_dry_run");
  auto request = valid_rollout_request(root);
  request.requested_mode = "dry_run";
  const auto refusals = marshal::validate_rollout_request(request);
  check(has_refusal(refusals, "unsupported_requested_mode"),
        "rollout should reject legacy requested_mode=dry_run");
}

void test_rollout_rejects_synthetic_edges_without_reason() {
  const auto root = make_tmp_dir("synthetic_reason");
  auto request = valid_rollout_request(root);
  request.execution_profile.synthetic_edge_research_reason.clear();
  const auto refusals = marshal::validate_rollout_request(request);
  check(has_refusal(refusals, "missing_synthetic_edge_research_reason"),
        "synthetic edge opt-in should require a research reason");
}

void test_rollout_rejects_missing_replay_artifacts() {
  const auto root = make_tmp_dir("missing_replay");
  auto request = valid_rollout_request(root);
  write_text(request.runtime_job_dir / "job.state",
             "status=completed\n"
             "replay_artifacts_written=false\n");
  const auto refusals = marshal::validate_rollout_request(request);
  check(has_refusal(refusals, "replay_artifacts_not_written"),
        "rollout should require replay artifact evidence");
  check(has_refusal(refusals, "replay_batch_index_missing"),
        "rollout should require replay batch index evidence");
}

void test_rollout_tool_is_plan_only_without_lattice_callback() {
  const auto root = make_tmp_dir("tool");
  const auto request = valid_rollout_request(root);
  marshal::set_marshal_lattice_tool_callback(nullptr);

  const std::string args =
      "{"
      "\"rollout_id\":\"" +
      request.rollout_id +
      "\","
      "\"rollout_attempt_id\":\"" +
      request.rollout_attempt_id +
      "\","
      "\"idempotency_key\":\"" +
      request.idempotency_key +
      "\","
      "\"experiment_id\":\"" +
      request.experiment_id +
      "\","
      "\"runtime_job_dir\":\"" +
      request.runtime_job_dir.string() +
      "\","
      "\"replay_batch_index_path\":\"" +
      request.replay_batch_index_path.string() +
      "\","
      "\"runtime_exec_path\":\"" +
      request.runtime_exec_path.string() +
      "\","
      "\"report_path\":\"" +
      request.report_path.string() +
      "\","
      "\"requested_mode\":\"plan\","
      "\"graph_order_fingerprint\":\"" +
      request.graph_order_fingerprint +
      "\","
      "\"asset_universe_digest\":\"" +
      request.asset_universe_digest +
      "\","
      "\"base_reserve_node_id\":\"USDT\","
      "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
      "\"policy_set\":[\"base_reserve\",\"equal_weight\",\"sdu\"],"
      "\"max_steps\":250,"
      "\"max_parallel_jobs\":4,"
      "\"execution_profile\":{"
      "\"cost_model_id\":\"linear_transaction_cost_rate.v1\","
      "\"allow_synthetic_direct_edges\":true,"
      "\"synthetic_edge_research_reason\":\"research replay fixture\","
      "\"linear_transaction_cost_rate\":0.001"
      "}"
      "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.rollout", args,
                                           &result, &error),
        "rollout tool should not require Lattice callback: " + error);
  check(result.find("\"tool\":\"hero.marshal.rollout\"") != std::string::npos,
        "rollout response should identify rollout tool");
  check(result.find("\"schema_version\":\"kikijyeba.marshal.rollout."
                    "v2.5b\"") != std::string::npos &&
            result.find("\"next_safe_actions\":[\"execute_rollout_via_"
                        "marshal\",\"inspect_rollout_plan\"]") !=
                std::string::npos &&
            result.find("\"authority\":{\"marshal_proves_target\":false") !=
                std::string::npos,
        "rollout response should expose the V2.5b shared dispatch envelope "
        "supplement");
  check(result.find("\"dispatch_state\":\"prepared\"") != std::string::npos,
        "rollout response should return prepared state");
  check(result.find("\"next_action\"") == std::string::npos,
        "rollout response should not expose legacy top-level next_action");
  check(result.find("\"idempotency\":{\"scope\":"
                    "\"request_digest_binding\"") != std::string::npos,
        "rollout response should expose idempotency as identity binding only");
  check(result.find("\"receipt_produced\":false") != std::string::npos,
        "rollout response should not produce a receipt");
  check(result.find("\"rollout_plan\"") != std::string::npos,
        "rollout response should include rollout plan");
  check(result.find("hero.lattice") == std::string::npos,
        "rollout tool should not call through Lattice target driver");
}

void test_rollout_tool_rejects_prepare_only_field() {
  const auto root = make_tmp_dir("prepare_only");
  const auto request = valid_rollout_request(root);
  const std::string args = "{"
                           "\"rollout_id\":\"" +
                           request.rollout_id +
                           "\","
                           "\"rollout_attempt_id\":\"" +
                           request.rollout_attempt_id +
                           "\","
                           "\"idempotency_key\":\"" +
                           request.idempotency_key +
                           "\","
                           "\"runtime_job_dir\":\"" +
                           request.runtime_job_dir.string() +
                           "\","
                           "\"replay_batch_index_path\":\"" +
                           request.replay_batch_index_path.string() +
                           "\","
                           "\"requested_mode\":\"plan\","
                           "\"base_reserve_node_id\":\"USDT\","
                           "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
                           "\"prepare_only\":true"
                           "}";

  std::string result;
  std::string error;
  check(!marshal::execute_marshal_tool_json("hero.marshal.rollout", args,
                                            &result, &error),
        "rollout tool should reject legacy prepare_only field");
  check(error.find("hero.marshal.rollout unknown field: prepare_only") !=
            std::string::npos,
        "prepare_only rejection should be explicit");
}

void test_rollout_tool_rejects_unknown_execution_profile_field() {
  const auto root = make_tmp_dir("unknown_profile_field");
  const auto request = valid_rollout_request(root);
  const std::string args = "{"
                           "\"rollout_id\":\"" +
                           request.rollout_id +
                           "\","
                           "\"rollout_attempt_id\":\"" +
                           request.rollout_attempt_id +
                           "\","
                           "\"idempotency_key\":\"" +
                           request.idempotency_key +
                           "\","
                           "\"runtime_job_dir\":\"" +
                           request.runtime_job_dir.string() +
                           "\","
                           "\"replay_batch_index_path\":\"" +
                           request.replay_batch_index_path.string() +
                           "\","
                           "\"requested_mode\":\"plan\","
                           "\"graph_order_fingerprint\":\"" +
                           request.graph_order_fingerprint +
                           "\","
                           "\"asset_universe_digest\":\"" +
                           request.asset_universe_digest +
                           "\","
                           "\"base_reserve_node_id\":\"USDT\","
                           "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
                           "\"execution_profile\":{\"unsupported\":true}"
                           "}";

  std::string result;
  std::string error;
  check(!marshal::execute_marshal_tool_json("hero.marshal.rollout", args,
                                            &result, &error),
        "rollout tool should reject unknown execution_profile fields");
  check(error.find("rollout execution_profile unknown field: unsupported") !=
            std::string::npos,
        "unknown execution_profile field rejection should be explicit");
}

void test_rollout_execute_calls_runtime_replay() {
  const auto root = make_tmp_dir("execute");
  auto request = valid_rollout_request(root);
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.runtime.dsl";
  const auto fake_exec = root / "fake_cuwacunu_exec.sh";
  request.config_path = config_path;
  request.runtime_exec_path = fake_exec;

  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() + "\n");
  write_text(fake_exec,
             "#!/bin/sh\n"
             "printf 'replay_experiment_id=validation_costed_001\\n'\n"
             "printf 'replay_completed_count=2\\n'\n"
             "printf 'replay_report_path=%s\\n' \"" +
                 request.report_path.string() + "\"\n");
  std::filesystem::permissions(fake_exec,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write);
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = " +
                              fake_exec.string() +
                              "\n"
                              "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = false\n"
                              "dev_nuke_backup_root:path = " +
                              (root / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  const std::string args =
      "{"
      "\"requested_mode\":\"execute\","
      "\"rollout_id\":\"" +
      request.rollout_id +
      "\","
      "\"rollout_attempt_id\":\"" +
      request.rollout_attempt_id +
      "\","
      "\"idempotency_key\":\"" +
      request.idempotency_key +
      "\","
      "\"experiment_id\":\"" +
      request.experiment_id +
      "\","
      "\"config_path\":\"" +
      config_path.string() +
      "\","
      "\"runtime_job_dir\":\"" +
      request.runtime_job_dir.string() +
      "\","
      "\"replay_batch_index_path\":\"" +
      request.replay_batch_index_path.string() +
      "\","
      "\"runtime_exec_path\":\"" +
      fake_exec.string() +
      "\","
      "\"report_path\":\"" +
      request.report_path.string() +
      "\","
      "\"graph_order_fingerprint\":\"" +
      request.graph_order_fingerprint +
      "\","
      "\"asset_universe_digest\":\"" +
      request.asset_universe_digest +
      "\","
      "\"base_reserve_node_id\":\"USDT\","
      "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
      "\"policy_set\":[\"equal_weight\",\"sdu\"],"
      "\"max_steps\":250,"
      "\"max_parallel_jobs\":4,"
      "\"timeout_seconds\":5,"
      "\"include_machine_payload\":true,"
      "\"execution_profile\":{"
      "\"cost_model_id\":\"linear_transaction_cost_rate.v1\","
      "\"allow_synthetic_direct_edges\":true,"
      "\"synthetic_edge_research_reason\":\"research replay fixture\","
      "\"linear_transaction_cost_rate\":0.001"
      "}"
      "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.rollout", args,
                                           &result, &error),
        "rollout execute should call Runtime replay: " + error);
  check(result.find("\"dispatch_state\":\"executed\"") != std::string::npos,
        "execute rollout should report executed dispatch state: " + result);
  check(result.find("\"runtime_replay\":{\"attempted\":true") !=
            std::string::npos,
        "execute rollout should attempt Runtime replay");
  check(result.find("\"tool_name\":\"hero.runtime.run\"") != std::string::npos,
        "execute rollout should name collapsed Runtime run tool");
  check(result.find("\"tool_result_error\":false") != std::string::npos,
        "execute rollout Runtime result should not be an error");
  check(result.find("replay_completed_count") != std::string::npos,
        "execute rollout should expose Runtime replay stdout evidence");
  check(result.find("allow_synthetic_direct_edges") != std::string::npos,
        "execute rollout should pass synthetic edge policy to Runtime");
  check(result.find("linear_transaction_cost_rate") != std::string::npos,
        "execute rollout should pass transaction cost profile to Runtime");
  check(result.find("execution_profile_digest") != std::string::npos &&
            result.find("policy_set_digest") != std::string::npos,
        "execute rollout should pass profile and policy identity digests to "
        "Runtime");
}

void test_prepare_rejects_rollout_intent() {
  marshal::set_marshal_lattice_tool_callback(nullptr);
  const std::string args = "{"
                           "\"intent\":\"rollout\","
                           "\"target_id\":\"kikijyeba.environment.replay.v1\","
                           "\"drive_mode\":\"one_step\""
                           "}";
  std::string result;
  std::string error;
  check(!marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                            &result, &error),
        "prepare should reject legacy rollout intent");
  check(error.find("use hero.marshal.rollout") != std::string::npos,
        "prepare should route rollout intent to hero.marshal.rollout");
}

} // namespace

int main() {
  test_rollout_plan_accepts_completed_runtime_job();
  test_rollout_requires_public_identity_fields();
  test_rollout_validates_graph_and_asset_universe_identity();
  test_rollout_validates_typed_execution_profile();
  test_rollout_rejects_legacy_dry_run_mode();
  test_rollout_rejects_synthetic_edges_without_reason();
  test_rollout_rejects_missing_replay_artifacts();
  test_rollout_tool_is_plan_only_without_lattice_callback();
  test_rollout_tool_rejects_prepare_only_field();
  test_rollout_tool_rejects_unknown_execution_profile_field();
  test_rollout_execute_calls_runtime_replay();
  test_prepare_rejects_rollout_intent();
  return 0;
}
