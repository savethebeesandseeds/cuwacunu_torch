#include "kikijyeba/marshal/dispatch_operation.h"
#include "kikijyeba/marshal/dispatch_receipt.h"
#include "kikijyeba/marshal/performance_budget.h"
#include "kikijyeba/marshal/tool_schema.h"

#include "hero/runtime_hero/hero_runtime_tools.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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
                   ("cuwacunu_marshal_perf_" + label + "_" +
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

marshal::marshal_active_identity_t active_identity() {
  marshal::marshal_active_identity_t identity{};
  identity.protocol_contract_fingerprint = "contract_1";
  identity.graph_order_fingerprint = "graph_1";
  identity.target_spec_fingerprint = "target_spec_1";
  identity.split_policy_fingerprint = "split_policy_1";
  return identity;
}

marshal::marshal_dispatch_advice_t
valid_advice(const std::filesystem::path &config_path,
             const std::filesystem::path &runtime_root) {
  marshal::marshal_dispatch_advice_t advice{};
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.target_id = "channel_mdn_validation_eval_ready";
  advice.target_status = "blocked";
  advice.active_identity = active_identity();
  advice.plan_basis.present = true;
  advice.plan_basis.available = true;
  advice.plan_basis.source_range = "anchor_index";
  advice.plan_basis.target_anchor_index_begin = 1800;
  advice.plan_basis.target_anchor_index_end = 2050;
  advice.plan_basis.primary_deficit_key = "coverage:evaluation_metric";
  advice.plan_basis.deficit_keys = {"coverage:evaluation_metric"};
  advice.plan_basis.deficit_priority_classes = {"coverage"};
  advice.suggested_wave.target = "wikimyei.inference.expected_value.mdn";
  advice.suggested_wave.mode = "run|debug";
  advice.suggested_wave.source_range = "anchor_index";
  advice.suggested_wave.anchor_index_begin = 1800;
  advice.suggested_wave.anchor_index_end = 2050;
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      (runtime_root / "jobs/mdn/checkpoint.pt").string();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_REPRESENTATION_CHECKPOINT"] =
      (runtime_root / "jobs/vicreg/checkpoint.pt").string();
  advice.max_waves = 1;
  advice.recommendation_attempt_count = 0;
  advice.required_plan_inputs = {"PLAN_INPUT_MDN_CHECKPOINT",
                                 "PLAN_INPUT_REPRESENTATION_CHECKPOINT"};
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.source_lattice_timestamp = "2026-05-23T00:00:00Z";
  return advice;
}

marshal::marshal_dispatch_request_t
request_for(const marshal::marshal_dispatch_advice_t &advice) {
  marshal::marshal_dispatch_request_t request{};
  request.requested_mode = marshal::marshal_dispatch_mode_t::dry_run;
  request.target_id = advice.target_id;
  request.config_path = advice.config_path;
  request.runtime_root = advice.runtime_root;
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  return request;
}

marshal::marshal_dispatch_validation_context_t
context_for(const std::filesystem::path &config_path,
            const std::filesystem::path &runtime_root) {
  marshal::marshal_dispatch_validation_context_t ctx{};
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  ctx.active_identity = active_identity();
  ctx.supported_wave_targets.insert("wikimyei.inference.expected_value.mdn");
  ctx.allowed_model_state_roots.push_back(runtime_root.string());
  ctx.freshness_check_timestamp_utc = "2026-05-23T00:10:00Z";
  return ctx;
}

marshal::marshal_runtime_policy_snapshot_t
policy_for(const std::filesystem::path &runtime_root) {
  marshal::marshal_runtime_policy_snapshot_t policy{};
  policy.runtime_hero_available = true;
  policy.runtime_exec_exists = true;
  policy.runtime_exec_executable = true;
  policy.default_dry_run = true;
  policy.runtime_root = runtime_root.string();
  return policy;
}

marshal::marshal_runtime_wave_snapshot_t
wave_for(const marshal::marshal_dispatch_advice_t &advice) {
  marshal::marshal_runtime_wave_snapshot_t wave{};
  wave.available = true;
  wave.target_component_family_id = advice.suggested_wave.target;
  wave.mode = advice.suggested_wave.mode;
  wave.source_range = advice.suggested_wave.source_range;
  wave.anchor_index_begin = advice.suggested_wave.anchor_index_begin;
  wave.anchor_index_end = advice.suggested_wave.anchor_index_end;
  wave.model_state_inputs = advice.suggested_wave.plan_inputs;
  return wave;
}

} // namespace

int main() {
  const auto root = make_tmp_dir("budget");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.runtime.dsl";
  const auto wave_path = root / "wave.dsl";
  std::filesystem::create_directories(runtime_root);
  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "[KIKIJYEBA]\n"
                              "kikijyeba_settings_wave_dsl_path = " +
                              wave_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = /bin/true\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = true\n"
                              "dev_nuke_backup_root:path = " +
                              (root / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");
  const std::string wave_text =
      "KIKIJYEBA_WAVE {\n"
      "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
      "  TARGET = wikimyei.inference.expected_value.mdn;\n"
      "  MODE = run|debug;\n"
      "  SOURCE_CURSOR_KIND = graph_anchor;\n"
      "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
      "  SOURCE_RANGE = anchor_index;\n"
      "  ANCHOR_INDEX_BEGIN = 1800;\n"
      "  ANCHOR_INDEX_END = 2050;\n"
      "  INPUT_MDN_CHECKPOINT = " +
      (runtime_root / "jobs/mdn/checkpoint.pt").string() +
      ";\n"
      "  INPUT_REPRESENTATION_CHECKPOINT = " +
      (runtime_root / "jobs/vicreg/checkpoint.pt").string() +
      ";\n"
      "};\n";
  write_text(wave_path, wave_text);

  const auto advice = valid_advice(config_path, runtime_root);
  const auto request = request_for(advice);
  const auto ctx = context_for(config_path, runtime_root);
  const auto policy = policy_for(runtime_root);
  const auto wave = wave_for(advice);
  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;

  marshal::marshal_performance_report_t report{};
  marshal::add_stage(&report, marshal::measure_stage_repeated(
                                  "advice_validation", "library", 10.0, 5, [&] {
                                    const auto result =
                                        marshal::validate_dispatch_advice(
                                            advice, request, ctx);
                                    check(result.dispatchable,
                                          "advice validation fixture failed");
                                  }));
  marshal::add_stage(
      &report, marshal::measure_stage_repeated(
                   "dry_run_preview", "library", 10.0, 5, [&] {
                     const auto decision =
                         marshal::build_runtime_dry_run_dispatch_preview(
                             advice, request, ctx, policy, wave);
                     check(decision.accepted, "dry-run preview fixture failed");
                   }));
  marshal::add_stage(
      &report,
      marshal::measure_stage_repeated(
          "runtime_policy_read", "runtime_hero_policy", 25.0, 5, [&] {
            cuwacunu::hero::runtime::runtime_policy_t loaded_policy{};
            std::string error;
            check(cuwacunu::hero::runtime::load_runtime_policy(
                      policy_path, config_path, &loaded_policy, &error),
                  "Runtime Hero policy read fixture failed: " + error);
          }));
  marshal::add_stage(
      &report, marshal::measure_stage_repeated(
                   "tool_schema_catalog", "long_lived_mcp", 10.0, 5, [&] {
                     check(marshal::validate_marshal_tool_schemas().empty(),
                           "schema catalog fixture failed");
                   }));
  marshal::add_stage(
      &report, marshal::measure_stage_repeated(
                   "cli_tool_catalog", "direct_cli", 10.0, 5, [&] {
                     check(marshal::direct_cli_and_mcp_expose_same_primitives(),
                           "CLI/MCP catalog mismatch");
                   }));

  marshal::marshal_dry_run_dispatch_response_t response{};
  marshal::add_stage(
      &report, marshal::measure_stage_repeated(
                   "runtime_hero_handoff", "runtime_hero", 1000.0, 3, [&] {
                     response = marshal::run_marshal_dry_run_dispatch(
                         advice, request, ctx, policy, wave, options);
                     check(response.accepted,
                           "Runtime Hero handoff fixture failed");
                   }));
  marshal::add_stage(
      &report,
      marshal::measure_stage_repeated(
          "receipt_write", "receipt_io", 25.0, 5, [&] {
            const auto receipt = marshal::make_dry_run_dispatch_receipt(
                advice, response, "2026-05-23T00:00:00Z");
            marshal::write_dispatch_receipt(
                root / ".marshal_dispatch" / "budget_receipt.lls", receipt);
          }));

  check(report.passed(), marshal::performance_budget_failure_hint(report));
  check(
      marshal::canonical_performance_report_text(report).find(
          "performance benchmark speed is not target satisfaction evidence") !=
          std::string::npos,
      "performance report should state non-authority");
  check(marshal::canonical_performance_report_text(report).find("p95_ms") !=
            std::string::npos,
        "performance report should include percentile timing");
  check(marshal::canonical_performance_report_text(report).find(
            "runtime_policy_read") != std::string::npos,
        "performance report should separate Runtime Hero policy-read timing");
  check(std::all_of(report.stages.begin(), report.stages.end(),
                    [](const marshal::marshal_performance_stage_t &stage) {
                      return stage.sample_count > 1U;
                    }),
        "all Marshal performance stages should use repeated samples");

  marshal::marshal_performance_report_t failed{};
  failed.stages.push_back(
      marshal::marshal_performance_stage_t{.name = "synthetic",
                                           .layer = "library",
                                           .elapsed_ms = 2.0,
                                           .budget_ms = 1.0,
                                           .passed = false});
  check(marshal::performance_budget_failure_hint(failed).find("library") !=
            std::string::npos,
        "budget failure hint should identify the failed layer");

  std::cout << marshal::canonical_performance_report_text(report);
  std::filesystem::remove_all(root);
  return 0;
}
