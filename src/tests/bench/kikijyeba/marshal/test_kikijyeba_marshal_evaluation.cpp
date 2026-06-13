#include "hero/marshal_hero/marshal/evaluation_marshal.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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
                   ("cuwacunu_marshal_eval_" + label + "_" +
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
  check(in.is_open(), "failed to open input file: " + path.string());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string json_string_field(const std::string &json,
                              const std::string &field) {
  const std::string needle = "\"" + field + "\":\"";
  const auto begin = json.find(needle);
  check(begin != std::string::npos, "missing JSON field: " + field);
  const auto value_begin = begin + needle.size();
  const auto value_end = json.find('"', value_begin);
  check(value_end != std::string::npos, "unterminated JSON field: " + field);
  return json.substr(value_begin, value_end - value_begin);
}

std::string assignment_value(const std::string &text, const std::string &key) {
  const std::string needle = key + "=";
  const auto begin = text.find(needle);
  check(begin != std::string::npos, "missing assignment: " + key);
  const auto value_begin = begin + needle.size();
  const auto value_end = text.find('\n', value_begin);
  return text.substr(value_begin, value_end == std::string::npos
                                     ? std::string::npos
                                     : value_end - value_begin);
}

std::string hex_digest(char value) { return std::string(64U, value); }

bool has_refusal(const std::vector<std::string> &refusals,
                 const std::string &token) {
  return std::find(refusals.begin(), refusals.end(), token) != refusals.end();
}

marshal::marshal_evaluation_request_t
valid_request(const std::filesystem::path &root) {
  const auto runtime_exec = root / "bin/cuwacunu_exec";
  const auto config_path = root / ".config";
  const auto mdn_checkpoint = root / "model/channel_mdn.pt";
  const auto representation_checkpoint = root / "model/representation.pt";
  write_text(runtime_exec, "#!/bin/sh\nexit 0\n");
  write_text(config_path, "[KIKIJYEBA]\n");
  write_text(mdn_checkpoint, "mdn checkpoint placeholder\n");
  write_text(representation_checkpoint,
             "representation checkpoint placeholder\n");

  marshal::marshal_evaluation_request_t request{};
  request.evaluation_id = "eval_mdn_validation_1800_2050";
  request.wave_id = "cwu_02v_channel_validation_eval_mdn_1800_2050";
  request.runtime_exec_path = runtime_exec;
  request.evaluation_config_path = config_path;
  request.input_mdn_checkpoint = mdn_checkpoint;
  request.input_representation_checkpoint = representation_checkpoint;
  request.trained_range = {.split_id = "train_core",
                           .anchor_index_begin = 0,
                           .anchor_index_end = 1600};
  request.validation_range = {.split_id = "validation_holdout",
                              .anchor_index_begin = 1800,
                              .anchor_index_end = 2050};
  request.trained_range_evidence_digest = hex_digest('a');
  request.validation_split_policy_digest = hex_digest('b');
  request.accounting_numeraire_node_id = "USDT";
  request.target_node_ids = {"USDT", "BTC", "ETH", "SOL"};
  request.max_replay_steps = 128;
  request.max_parallel_replay_jobs = 4;
  return request;
}

void test_evaluation_plan_accepts_out_of_sample_request() {
  const auto root = make_tmp_dir("accepts");
  const auto request = valid_request(root);
  const auto request_digest = marshal::evaluation_request_digest(request);
  check(marshal::marshal_digest_is_strong_hex(request_digest),
        "request digest should be strong hex");

  const auto plan = marshal::prepare_evaluation_plan(request);
  check(plan.accepted, "valid evaluation request should be accepted");
  check(plan.refusal_reasons.empty(), "valid plan should have no refusals");
  check(plan.validation_range_out_of_sample,
        "accepted plan should mark validation range out-of-sample");
  check(!plan.target_satisfaction_claimed,
        "Marshal plan must not claim target satisfaction");
  check(!plan.runtime_executor, "Marshal plan must not be runtime executor");
  check(!plan.lattice_proof_authority,
        "Marshal plan must not be lattice proof authority");
  check(marshal::marshal_digest_is_strong_hex(plan.plan_digest),
        "plan digest should be strong hex");
  check(plan.runtime_run_command.find("--anchor-index-begin 1800") !=
            std::string::npos,
        "runtime command should bind validation begin");
  check(plan.runtime_run_command.find("--anchor-index-end 2050") !=
            std::string::npos,
        "runtime command should bind validation end");
  check(plan.runtime_run_command.find("--replay-accounting-numeraire-node 'USDT'") !=
            std::string::npos,
        "runtime command should bind accounting numeraire node");
  check(plan.runtime_replay_command_template.find("--replay-experiment-id "
                                                  "'eval_mdn_validation_1800_"
                                                  "2050'") != std::string::npos,
        "replay command should bind evaluation id");
  check(plan.runtime_replay_command_template.find("--replay-max-steps 128") !=
            std::string::npos,
        "replay command should bind max steps");
  check(plan.runtime_replay_command_template.find(
            "--replay-max-parallel-jobs 4") != std::string::npos,
        "replay command should bind max parallel jobs");

  const auto handoff =
      marshal::prepare_evaluation_runtime_handoff(request, plan);
  check(handoff.accepted, "accepted plan should produce runtime handoff");
  check(handoff.dry_run, "evaluation runtime handoff must remain dry-run");
  check(!handoff.confirm_execute,
        "evaluation runtime handoff must not confirm execution");
  check(!handoff.target_satisfaction_claimed,
        "evaluation handoff must not claim target satisfaction");
  check(!handoff.runtime_executor,
        "evaluation handoff must not become runtime executor");
  check(!handoff.lattice_proof_authority,
        "evaluation handoff must not become lattice proof authority");
  check(handoff.runtime_tool_name == "hero.runtime.run",
        "evaluation handoff should target collapsed Runtime Hero run");
  check(marshal::marshal_digest_is_strong_hex(
            handoff.runtime_execute_args_digest),
        "runtime execute args digest should be strong hex");
  check(marshal::marshal_digest_is_strong_hex(handoff.handoff_digest),
        "runtime handoff digest should be strong hex");
  check(handoff.runtime_execute_args_json.find("\"subject\":\"wave\"") !=
            std::string::npos,
        "runtime handoff args should target wave subject");
  check(handoff.runtime_execute_args_json.find("\"mode\":\"dry_run\"") !=
            std::string::npos,
        "runtime handoff args should force mode=dry_run");
  check(handoff.runtime_execute_args_json.find("\"confirm_execute\"") ==
            std::string::npos,
        "runtime handoff args should not expose retired confirm_execute");
  check(handoff.runtime_execute_args_json.find("\"wave_overlay\"") ==
            std::string::npos,
        "runtime handoff args should not expose retired wave_overlay");
  check(handoff.runtime_execute_args_json.find("\"marshal_expected_wave\"") ==
            std::string::npos,
        "runtime handoff args should not expose retired marshal_expected_wave");
  check(handoff.runtime_execute_args_json.find("\"args_path\"") !=
            std::string::npos,
        "runtime handoff args should carry launch data through args_path");
  const std::string run_request_text =
      read_text(json_string_field(handoff.runtime_execute_args_json,
                                  "args_path"));
  check(run_request_text.find("execution_request_path=") != std::string::npos,
        "runtime handoff request should carry launch range through an "
        "execution request");
  check(run_request_text.find("runtime_handoff_path=") != std::string::npos,
        "runtime handoff request should include handoff file path");
  const std::string runtime_handoff_text =
      read_text(assignment_value(run_request_text, "runtime_handoff_path"));
  check(runtime_handoff_text.find(request.evaluation_id) != std::string::npos,
        "runtime handoff file should bind evaluation id");
  check(runtime_handoff_text.find("runtime_replay_command_template") !=
            std::string::npos,
        "runtime handoff file should preserve replay command template");

  marshal::marshal_evaluation_receipt_t receipt{};
  receipt.request_digest = plan.request_digest;
  receipt.plan_digest = plan.plan_digest;
  receipt.evaluation_id = request.evaluation_id;
  receipt.runtime_job_dir = root / "runtime_job";
  receipt.replay_report_path = root / "runtime_job/artifacts/replay.report";
  receipt.replay_bundle_count = 4;
  receipt.attempted_count = 16;
  receipt.completed_count = 16;
  receipt.failed_count = 0;
  receipt.execution_backend_id = "cajtucu.execution.paper.v1";
  receipt.cajtucu_execution_step_count = 1000;
  receipt.cajtucu_rejected_fill_count = 2;
  receipt.cajtucu_partial_fill_count = 1;
  receipt.mean_projection_mae = 0.0232059;
  receipt.mean_projection_rmse = 0.0255562;
  receipt.mean_projection_correlation = -0.046875;
  receipt.mean_projection_directional_accuracy = 0.520137;
  receipt.mean_projection_interval_coverage = 0.966393;
  receipt.mean_projection_interval_width = 0.06125;
  receipt.mean_zero_return_numeraireline_mae = 0.0241;
  receipt.mean_zero_return_numeraireline_rmse = 0.0273;
  receipt.mean_model_skill_vs_zero_mae = 0.0458;
  receipt.mean_model_skill_vs_zero_rmse = 0.0837;
  receipt.mean_total_log_growth = 0.01744;
  receipt.mean_final_equity_numeraire = 1.01744;
  receipt.mean_total_transaction_cost_numeraire = 0.0;
  receipt.mean_cajtucu_order_count = 1.75;
  receipt.mean_cajtucu_fill_count = 1.73;
  receipt.mean_cajtucu_total_fee_numeraire = 0.0011;
  receipt.mean_cajtucu_total_spread_cost_numeraire = 0.0007;
  receipt.mean_cajtucu_total_slippage_numeraire = 0.0005;
  receipt.mean_cajtucu_total_transaction_cost_numeraire = 0.0023;
  receipt.time_law_future_observation_violation_count = 0;
  receipt.mixed_future_realization_key_count = 0;
  receipt.projection_validation_step_count = 1000;
  receipt.best_policy_by_final_equity = "equal_weight.v1";
  receipt.best_policy_by_after_cost_return = "equal_weight.v1";
  receipt.best_policy_by_drawdown_adjusted_return =
      "kikijyeba.environment.policy.spot_distributional_utility.v1";
  marshal::marshal_evaluation_policy_summary_t reserve{};
  reserve.policy_id = "numeraire_only.v1";
  reserve.policy_kind = "baseline";
  reserve.execution_backend_id = "cajtucu.execution.paper.v1";
  reserve.attempted_count = 4;
  reserve.completed_count = 4;
  reserve.cajtucu_execution_step_count = 250;
  reserve.mean_final_equity_numeraire = 1.0;
  marshal::marshal_evaluation_policy_summary_t sdu{};
  sdu.policy_id = "kikijyeba.environment.policy."
                  "spot_distributional_utility.v1";
  sdu.policy_kind = "deterministic_allocator";
  sdu.execution_backend_id = "cajtucu.execution.paper.v1";
  sdu.attempted_count = 4;
  sdu.completed_count = 4;
  sdu.cajtucu_execution_step_count = 250;
  sdu.cajtucu_rejected_fill_count = 2;
  sdu.cajtucu_partial_fill_count = 1;
  sdu.mean_total_log_growth = 0.0160804;
  sdu.mean_final_equity_numeraire = 1.01744;
  sdu.mean_max_drawdown = 0.036356;
  sdu.mean_total_turnover = 4.10059;
  sdu.mean_projection_mae = 0.0232059;
  sdu.mean_projection_rmse = 0.0255562;
  sdu.mean_projection_correlation = -0.046875;
  sdu.mean_projection_directional_accuracy = 0.520137;
  sdu.mean_projection_interval_coverage = 0.966393;
  sdu.mean_projection_interval_width = 0.06125;
  sdu.mean_zero_return_numeraireline_mae = 0.0241;
  sdu.mean_zero_return_numeraireline_rmse = 0.0273;
  sdu.mean_model_skill_vs_zero_mae = 0.0458;
  sdu.mean_model_skill_vs_zero_rmse = 0.0837;
  sdu.mean_cajtucu_order_count = 2.1;
  sdu.mean_cajtucu_fill_count = 2.05;
  sdu.mean_cajtucu_total_fee_numeraire = 0.0014;
  sdu.mean_cajtucu_total_spread_cost_numeraire = 0.0009;
  sdu.mean_cajtucu_total_slippage_numeraire = 0.0007;
  sdu.mean_cajtucu_total_transaction_cost_numeraire = 0.003;
  receipt.policy_summaries = {reserve, sdu};
  receipt = marshal::finalize_evaluation_receipt(std::move(receipt));
  check(marshal::marshal_digest_is_strong_hex(receipt.receipt_digest),
        "receipt digest should be strong hex");
  check(receipt.policy_summary_count == 2,
        "receipt should bind embedded policy summary count");
  const auto receipt_text = marshal::canonical_evaluation_receipt_text(receipt);
  check(receipt_text.find("top_level_metric_scope=mean_over_completed_"
                          "episode_reports_across_policies") !=
            std::string::npos,
        "receipt should describe aggregate metric scope");
  check(receipt_text.find("projection_validation_step_count=1000") !=
            std::string::npos,
        "receipt should carry projection validation count");
  check(receipt_text.find("execution_backend_id=cajtucu.execution.paper.v1") !=
            std::string::npos,
        "receipt should carry Cajtucu backend identity");
  check(receipt_text.find("cajtucu_execution_step_count=1000") !=
            std::string::npos,
        "receipt should carry Cajtucu execution step count");
  check(receipt_text.find("mean_cajtucu_total_transaction_cost_numeraire=") !=
            std::string::npos,
        "receipt should carry Cajtucu transaction cost summary");
  check(receipt_text.find("mean_zero_return_numeraireline_mae=") !=
            std::string::npos,
        "receipt should carry projection baseline metrics");
  check(receipt_text.find("mean_model_skill_vs_zero_mae=") != std::string::npos,
        "receipt should carry projection skill metrics");
  check(receipt_text.find("embedded_policy_1_id=kikijyeba.environment.policy."
                          "spot_distributional_utility.v1") !=
            std::string::npos,
        "receipt should carry policy-wise summaries");
  check(receipt_text.find("embedded_policy_1_mean_zero_return_numeraireline_mae=") !=
            std::string::npos,
        "receipt should carry policy-wise baseline metrics");
  check(receipt_text.find("embedded_policy_1_mean_model_skill_vs_zero_mae=") !=
            std::string::npos,
        "receipt should carry policy-wise skill metrics");
  check(receipt_text.find("embedded_policy_1_execution_backend_id="
                          "cajtucu.execution.paper.v1") != std::string::npos,
        "receipt should carry policy-wise Cajtucu backend identity");
  check(receipt_text.find(
            "embedded_policy_1_mean_cajtucu_total_transaction_cost_numeraire=") !=
            std::string::npos,
        "receipt should carry policy-wise Cajtucu cost summary");
  check(!receipt.target_satisfaction_claimed,
        "Marshal receipt must not claim target satisfaction");
  check(!receipt.runtime_executor, "Marshal receipt must not be executor");
  check(!receipt.lattice_proof_authority,
        "Marshal receipt must not be proof authority");
}

void test_evaluation_plan_rejects_leaky_or_ill_formed_requests() {
  const auto root = make_tmp_dir("rejects");

  auto overlap = valid_request(root / "overlap");
  overlap.trained_range.anchor_index_end = 1900;
  auto plan = marshal::prepare_evaluation_plan(overlap);
  check(!plan.accepted, "overlapping trained/validation ranges must reject");
  check(has_refusal(plan.refusal_reasons, "trained_validation_range_overlap"),
        "overlap refusal should be explicit");
  const auto rejected_handoff =
      marshal::prepare_evaluation_runtime_handoff(overlap, plan);
  check(!rejected_handoff.accepted,
        "rejected plan must not produce accepted Runtime handoff");
  check(rejected_handoff.runtime_execute_args_json.empty(),
        "rejected handoff must not carry Runtime execute args");
  check(has_refusal(rejected_handoff.refusal_reasons,
                    "trained_validation_range_overlap"),
        "rejected handoff should preserve plan refusal reasons");
  check(marshal::marshal_digest_is_strong_hex(rejected_handoff.handoff_digest),
        "rejected handoff should still be digestible");

  auto malformed_digest = valid_request(root / "malformed_digest");
  malformed_digest.validation_split_policy_digest = "not-a-digest";
  plan = marshal::prepare_evaluation_plan(malformed_digest);
  check(!plan.accepted, "malformed split-policy digest must reject");
  check(has_refusal(plan.refusal_reasons,
                    "malformed_validation_split_policy_digest"),
        "malformed digest refusal should be explicit");

  auto missing_checkpoint = valid_request(root / "missing_checkpoint");
  missing_checkpoint.input_mdn_checkpoint =
      root / "missing_checkpoint/model/missing.pt";
  plan = marshal::prepare_evaluation_plan(missing_checkpoint);
  check(!plan.accepted, "missing checkpoint must reject");
  check(has_refusal(plan.refusal_reasons, "input_mdn_checkpoint_missing"),
        "missing checkpoint refusal should be explicit");

  auto bad_nodes = valid_request(root / "bad_nodes");
  bad_nodes.target_node_ids = {"BTC", "ETH", "BTC"};
  plan = marshal::prepare_evaluation_plan(bad_nodes);
  check(!plan.accepted, "bad target universe must reject");
  check(has_refusal(plan.refusal_reasons,
                    "accounting_numeraire_node_missing_from_target_nodes"),
        "accounting numeraire membership refusal should be explicit");
  check(has_refusal(plan.refusal_reasons, "duplicate_target_node_id"),
        "duplicate target node refusal should be explicit");

  auto unsupported = valid_request(root / "unsupported_component");
  unsupported.target_component_family_id = "wikimyei.policy.rl.ppo";
  plan = marshal::prepare_evaluation_plan(unsupported);
  check(!plan.accepted, "unsupported target component must reject");
  check(has_refusal(plan.refusal_reasons,
                    "unsupported_target_component_family_id"),
        "unsupported target component refusal should be explicit");
}

} // namespace

int main() {
  test_evaluation_plan_accepts_out_of_sample_request();
  test_evaluation_plan_rejects_leaky_or_ill_formed_requests();
  return 0;
}
