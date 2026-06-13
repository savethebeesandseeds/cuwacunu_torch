// SPDX-License-Identifier: MIT

#include "hero/environment_hero/hero_environment_tools.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <unistd.h>

namespace environment = cuwacunu::hero::environment;

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

} // namespace

int main() {
  try {
    test_environment_rollout_direct_validate();
    std::cout << "hero Environment rollout direct tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
