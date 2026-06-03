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
  write_text(runtime_exec, "#!/bin/sh\nexit 0\n");
  write_text(replay_index, "bundle_count=4\n");
  write_text(job_dir / "job.state", "status=completed\n"
                                    "replay_artifacts_written=true\n"
                                    "replay_batch_index_path=" +
                                        replay_index.string() + "\n");

  marshal::marshal_rollout_request_t request{};
  request.rollout_id = "rollout_validation_costed_001";
  request.experiment_id = "validation_costed_001";
  request.runtime_job_dir = job_dir;
  request.runtime_exec_path = runtime_exec;
  request.report_path = root / "reports/validation_costed_001.report";
  request.base_reserve_node_id = "USDT";
  request.risky_node_ids = {"BTC", "ETH"};
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
  check(plan.resolved_policy_ids.size() == 4,
        "policy tokens should resolve to four policies");
  check(plan.replay_command_template.find("--replay-from-job-dir") !=
            std::string::npos,
        "rollout command should use replay-from-job-dir");
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

  const auto plan_json = marshal::rollout_plan_json(plan);
  check(plan_json.find("\"receipt_produced\"") == std::string::npos,
        "rollout plan JSON must not pretend to be a receipt");
  check(plan_json.find("\"accepted\":true") != std::string::npos,
        "rollout plan JSON should mark accepted plan");
  check(plan_json.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "rollout plan JSON should expose non-authority flags");
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

void test_prepare_rollout_tool_is_plan_only_without_lattice_callback() {
  const auto root = make_tmp_dir("tool");
  const auto request = valid_rollout_request(root);
  marshal::set_marshal_lattice_tool_callback(nullptr);

  const std::string args =
      "{"
      "\"intent\":\"rollout\","
      "\"rollout_id\":\"" +
      request.rollout_id +
      "\","
      "\"experiment_id\":\"" +
      request.experiment_id +
      "\","
      "\"runtime_job_dir\":\"" +
      request.runtime_job_dir.string() +
      "\","
      "\"runtime_exec_path\":\"" +
      request.runtime_exec_path.string() +
      "\","
      "\"report_path\":\"" +
      request.report_path.string() +
      "\","
      "\"base_reserve_node_id\":\"USDT\","
      "\"risky_node_ids\":[\"BTC\",\"ETH\"],"
      "\"policy_set\":[\"base_reserve\",\"equal_weight\",\"sdu\"],"
      "\"max_steps\":250,"
      "\"max_parallel_jobs\":4,"
      "\"execution_profile\":{"
      "\"allow_synthetic_direct_edges\":true,"
      "\"synthetic_edge_research_reason\":\"research replay fixture\","
      "\"linear_transaction_cost_rate\":0.001"
      "}"
      "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "rollout prepare should not require Lattice callback: " + error);
  check(result.find("\"intent\":\"rollout\"") != std::string::npos,
        "prepare response should identify rollout intent");
  check(result.find("\"dispatch_state\":\"prepared\"") != std::string::npos,
        "prepare response should return prepared state");
  check(result.find("\"receipt_produced\":false") != std::string::npos,
        "prepare response should not produce a receipt");
  check(result.find("\"rollout_plan\"") != std::string::npos,
        "prepare response should include rollout plan");
  check(result.find("hero.lattice") == std::string::npos,
        "rollout prepare should not call through Lattice target driver");
}

} // namespace

int main() {
  test_rollout_plan_accepts_completed_runtime_job();
  test_rollout_rejects_synthetic_edges_without_reason();
  test_rollout_rejects_missing_replay_artifacts();
  test_prepare_rollout_tool_is_plan_only_without_lattice_callback();
  return 0;
}
