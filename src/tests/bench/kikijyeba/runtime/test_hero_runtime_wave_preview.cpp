#include "hero/runtime_hero/hero_runtime_tools.h"
#include "kikijyeba/marshal/digest.h"
#include "kikijyeba/runtime/policy_training_causal_schedule.h"
#include "kikijyeba/runtime/wave_settings.h"
#include "wikimyei/assembly.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

namespace hero_runtime = cuwacunu::hero::runtime;
namespace wave_settings = cuwacunu::kikijyeba::settings;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_hero_runtime_wave_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())) + "_" +
              std::to_string(counter++));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open text output");
  out << text;
}

std::string digest_file_for_handoff(const std::filesystem::path &path,
                                    const std::string &domain) {
  std::ifstream in(path, std::ios::binary);
  check(in.is_open(), "failed to open digest input");
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return cuwacunu::kikijyeba::marshal::marshal_digest_for_text(domain,
                                                               buffer.str());
}

std::string replay_report_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash,
      std::string("kikijyeba.environment.replay.report_digest.v1\n") + text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

std::string run_wave_preview(const std::string &target, const std::string &mode,
                             const std::string &source_order = {},
                             const std::string &protocol_text = {},
                             const std::string &compatible_protocols = {}) {
  const auto dir = make_tmp_dir(target);
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto protocol_path = dir / "kikijyeba.protocol.dsl";
  const auto config_path = dir / ".config";

  std::ostringstream wave;
  wave << "WAVE_SETTINGS {\n"
       << "  WAVE_ID = test_wave;\n"
       << (compatible_protocols.empty()
               ? ""
               : "  COMPATIBLE_PROTOCOLS = " + compatible_protocols + ";\n")
       << "  TARGET = " << target << ";\n"
       << "  MODE = " << mode << ";\n"
       << "  SOURCE_CURSOR_KIND = graph_anchor;\n"
       << "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
       << "  SOURCE_RANGE = anchor_index;\n"
       << (source_order.empty() ? ""
                                : "  SOURCE_ORDER = " + source_order + ";\n")
       << "  ANCHOR_INDEX_BEGIN = 1;\n"
       << "  ANCHOR_INDEX_END = 3;\n"
       << "};\n";
  write_text(wave_path, wave.str());
  if (!protocol_text.empty()) {
    write_text(protocol_path, protocol_text);
  }

  std::ostringstream config;
  config << "[KIKIJYEBA]\n"
         << "kikijyeba_settings_wave_dsl_path = " << wave_path.string() << "\n";
  if (!protocol_text.empty()) {
    config << "kikijyeba_protocol_dsl_path = " << protocol_path.string()
           << "\n";
  }
  write_text(config_path, config.str());

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;

  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.inspect subject=wave returned error: " + result);
  std::filesystem::remove_all(dir);
  return result;
}

void require_contains(const std::string &text, std::string_view needle,
                      const std::string &message) {
  check(text.find(needle) != std::string::npos,
        message + " missing needle: " + std::string(needle) + "\n" + text);
}

bool has_issue_containing(const std::vector<std::string> &issues,
                          const std::string &needle) {
  return std::any_of(issues.begin(), issues.end(), [&](const auto &issue) {
    return issue.find(needle) != std::string::npos;
  });
}

std::string cwu02_mtf_protocol_text() {
  return std::string("PROTOCOL {\n"
                     "  PROTOCOL_ID = cwu_02v;\n"
                     "  PROTOCOL_KIND = channel_graph_first;\n"
                     "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
                     "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
                     "  REPRESENTATION = "
                     "wikimyei.representation.encoding.mtf_jepa_mae_vicreg;\n"
                     "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
                     "  REPRESENTATION_CONTRACT = "
                     "graph_order.channel_node_representation.v1;\n"
                     "};\n");
}

void test_wave_settings_train_defaults_to_random_source_order() {
  const auto implicit_train = wave_settings::decode_wave_settings_from_dsl(
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = train_default;\n"
      "  COMPATIBLE_PROTOCOLS = cwu_01v,cwu_02v;\n"
      "  TARGET = wikimyei.representation.encoding.vicreg;\n"
      "  MODE = train;\n"
      "  SOURCE_RANGE = all;\n"
      "};\n");
  check(implicit_train.source_order_policy ==
            wave_settings::wave_source_order_policy_t::random_per_epoch,
        "train waves must default to random_per_epoch SOURCE_ORDER");
  check(implicit_train.compatible_protocol_ids.size() == 2 &&
            implicit_train.compatible_protocol_ids[0] == "cwu_01v" &&
            implicit_train.compatible_protocol_ids[1] == "cwu_02v",
        "COMPATIBLE_PROTOCOLS should parse as ordered protocol ids");
  check(wave_settings::wave_supports_protocol(implicit_train, "cwu_02v"),
        "parsed compatible protocols should include cwu_02v");
  check(!wave_settings::wave_supports_protocol(implicit_train, "cwu_03v"),
        "parsed compatible protocols should reject unknown protocol");
  check(!implicit_train.source_order_policy_explicit,
        "omitted train SOURCE_ORDER must remain implicit");
  check(std::string(wave_settings::source_order_warning_token(implicit_train))
            .empty(),
        "implicit train random SOURCE_ORDER must not warn");

  const auto explicit_train = wave_settings::decode_wave_settings_from_dsl(
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = train_sequential;\n"
      "  TARGET = wikimyei.representation.encoding.vicreg;\n"
      "  MODE = train;\n"
      "  SOURCE_RANGE = all;\n"
      "  SOURCE_ORDER = sequential;\n"
      "};\n");
  check(explicit_train.source_order_policy ==
            wave_settings::wave_source_order_policy_t::sequential,
        "explicit sequential train SOURCE_ORDER must be preserved");
  check(explicit_train.source_order_policy_explicit,
        "explicit train SOURCE_ORDER must be marked explicit");
  check(std::string(wave_settings::source_order_warning_token(
            explicit_train)) == "train_wave_explicit_sequential_source_order",
        "explicit sequential train SOURCE_ORDER must warn");
}

void test_execute_expected_wave_binding() {
  const auto dir = make_tmp_dir("expected_wave_execute");
  const auto runtime_root = dir / "runtime";
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  std::filesystem::create_directories(runtime_root);

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = test_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = anchor_index;\n"
                        "  ANCHOR_INDEX_BEGIN = 1;\n"
                        "  ANCHOR_INDEX_END = 3;\n"
                        "  INPUT_MDN_CHECKPOINT = /tmp/mdn.pt;\n"
                        "  INPUT_REPRESENTATION_CHECKPOINT = /tmp/rep.pt;\n"
                        "};\n");

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
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load runtime policy: " + error);

  const std::string config_hash = digest_file_for_handoff(
      config_path, "kikijyeba.runtime.handoff.base_config_file.v1");
  const std::string policy_hash = digest_file_for_handoff(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  const std::string good_handoff_digest = "test";
  const std::string good_handoff_id = "runtime_handoff_" + good_handoff_digest;
  const std::string stale_handoff_digest = "stale_hash";
  const std::string stale_handoff_id =
      "runtime_handoff_" + stale_handoff_digest;
  const std::string symbolic_handoff_digest = "symbolic";
  const std::string symbolic_handoff_id =
      "runtime_handoff_" + symbolic_handoff_digest;

  const std::string good_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", good_args, &ctx,
                                        &result, &error),
        "expected-wave-bound execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "expected-wave-bound execute returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "expected-wave-bound execute should run when fields match");

  const std::string handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      good_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      good_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() + "\",\"hash\":\"" + config_hash +
      "\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_hashes\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"hash\":\"" + policy_hash +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[]}}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", handoff_args, &ctx,
                                        &result, &error),
        "runtime_handoff execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "runtime_handoff execute returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "runtime_handoff execute should run when fields match");
  require_contains(result, "\"--runtime-handoff-id\"",
                   "runtime_handoff execute should pass handoff id to runtime");
  require_contains(result, "\"" + good_handoff_id + "\"",
                   "runtime_handoff execute should pass concrete handoff id");
  require_contains(result, "\"--runtime-handoff-digest\"",
                   "runtime_handoff execute should pass handoff digest to "
                   "runtime");
  require_contains(result, "\"" + good_handoff_digest + "\"",
                   "runtime_handoff execute should pass concrete handoff "
                   "digest");

  const std::string stale_hash_handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      stale_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      stale_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() +
      "\",\"hash\":\"stale_config_hash\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_hashes\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"hash\":\"" + policy_hash +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[]}}";
  result.clear();
  error.clear();
  const bool stale_hash_ok = hero_runtime::execute_tool_json(
      "hero.runtime.run", stale_hash_handoff_args, &ctx, &result, &error);
  check(!stale_hash_ok,
        "runtime_handoff stale base config hash should fail before execution: "
        "result=" +
            result + " error=" + error);
  check(error.find("base_config path/hash") != std::string::npos,
        "runtime_handoff stale hash should report base_config hash failure");

  const std::string unresolved_handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      symbolic_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      symbolic_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() + "\",\"hash\":\"" + config_hash +
      "\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"latest_satisfying:"
      "channel_mdn_train_core_ready\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"latest_satisfying:"
      "channel_mdn_train_core_ready\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_hashes\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"hash\":\"" + policy_hash +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[\"PLAN_INPUT_MDN_CHECKPOINT=latest_satisfying:"
      "channel_mdn_train_core_ready\"]}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", unresolved_handoff_args, &ctx, &result, &error),
        "runtime_handoff unresolved selector should fail before execution");
  check(error.find("E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS") != std::string::npos,
        "runtime_handoff unresolved selector should report unresolved symbols");

  const std::string bad_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"train|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", bad_args, &ctx,
                                         &result, &error),
        "expected-wave mismatch should fail before execution");
  check(error.find("E_RUNTIME_EXPECTED_WAVE_MISMATCH") != std::string::npos,
        "expected-wave mismatch should report a specific error");

  const std::string bad_checkpoint_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/other.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", bad_checkpoint_args, &ctx, &result, &error),
        "expected-wave checkpoint mismatch should fail before execution");
  check(error.find("model_state_inputs differs") != std::string::npos,
        "checkpoint mismatch should report model_state_inputs failure");

  std::filesystem::remove_all(dir);
}

void test_execute_wave_overlay() {
  const auto dir = make_tmp_dir("execute_wave_overlay");
  const auto runtime_root = dir / "runtime";
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  std::filesystem::create_directories(runtime_root);

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = profile_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = all;\n"
                        "};\n");
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
                              "dev_nuke_backup_enabled:bool = false\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load runtime policy: " + error);

  const std::string args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\","
      "\"wave_overlay\":{\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\",\"anchor_index_end\":\"3\"},"
      "\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\"}}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", args, &ctx, &result,
                                        &error),
        "wave-overlay execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "wave-overlay execute returned error: " + result);
  require_contains(result, "\"--source-range\"",
                   "wave overlay should be passed to runtime process");
  require_contains(result, "\"anchor_index\"",
                   "wave overlay should pass anchor_index policy");
  require_contains(result, "\"--anchor-index-begin\"",
                   "wave overlay should pass anchor begin flag");
  require_contains(result, "\"1\"", "wave overlay should pass anchor begin");
  require_contains(result, "\"--anchor-index-end\"",
                   "wave overlay should pass anchor end flag");
  require_contains(result, "\"3\"", "wave overlay should pass anchor end");

  const std::string invalid_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\","
      "\"wave_overlay\":{\"source_range\":\"all\","
      "\"anchor_index_begin\":\"1\",\"anchor_index_end\":\"3\"}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", invalid_args, &ctx,
                                         &result, &error),
        "invalid wave overlay should fail");
  check(error.find("E_RUNTIME_WAVE_OVERLAY_INVALID") != std::string::npos,
        "invalid wave overlay reports overlay error");

  std::filesystem::remove_all(dir);
}

void test_replay_operator_tool() {
  const auto dir = make_tmp_dir("replay_tool");
  const auto runtime_root = dir / "runtime";
  const auto job_dir = runtime_root / "jobs" / "completed_job";
  const auto replay_artifact_dir =
      job_dir / "artifacts" / "kikijyeba.environment.replay.v1";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  const auto fake_exec = dir / "fake_cuwacunu_exec.sh";
  std::filesystem::create_directories(replay_artifact_dir);

  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() + "\n");
  write_text(job_dir / "job.manifest",
             "job_id=completed_job\n"
             "config_path=" +
                 config_path.string() +
                 "\n"
                 "target_component_family_id=wikimyei.inference.expected_"
                 "value.mdn\n");
  write_text(job_dir / "job.state", "status=completed\n");
  write_text(replay_artifact_dir / "runtime_replay_batches.index",
             "schema=kikijyeba.environment.replay.runtime_batch_index.v1\n"
             "entry_count=1\n");
  const std::string hero_replay_report_text =
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=hero_replay\n"
      "completed_count=2\n";
  const std::string hero_replay_report_digest =
      replay_report_digest_for_text(hero_replay_report_text);
  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=hero_replay\n"
             "entry_0_policy_count=2\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n"
             "entry_0_report_path=hero_replay.report\n"
             "entry_0_report_digest=" +
                 hero_replay_report_digest + "\n");
  write_text(replay_artifact_dir / "hero_replay.report",
             hero_replay_report_text);
  write_text(fake_exec,
             "#!/bin/sh\n"
             "printf 'replay_experiment_id=hero_replay\\n'\n"
             "printf 'replay_completed_count=2\\n'\n"
             "printf 'replay_report_path=%s\\n' \"" +
                 (replay_artifact_dir / "hero_replay.report").string() +
                 "\"\n");
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
                              "dev_nuke_backup_enabled:bool = false\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load replay runtime policy: " + error);

  const std::string dry_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"base_reserve_node_id\":\"USDT\","
      "\"risky_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_replay\","
      "\"max_steps\":8,"
      "\"include_equal_weight\":true,"
      "\"allow_synthetic_direct_edges\":true,"
      "\"linear_transaction_cost_rate\":0.001,"
      "\"execution_profile_digest\":\"profile_digest_fixture\","
      "\"policy_set_digest\":\"policy_set_digest_fixture\"}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", dry_args, &ctx,
                                        &result, &error),
        "replay dry-run failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "replay dry-run returned error: " + result);
  require_contains(result, "\"dry_run\":true",
                   "replay dry-run should not execute process");
  require_contains(result, "\"--replay-from-job-dir\"",
                   "replay should target job-dir replay mode");
  require_contains(result, "\"--replay-base-reserve-node\"",
                   "replay should pass base reserve override");
  require_contains(result, "\"--replay-risky-nodes\"",
                   "replay should pass risky-node override");
  require_contains(result, "\"--replay-max-steps\"",
                   "replay should pass max steps");
  require_contains(result, "\"--replay-include-equal-weight\"",
                   "replay should pass baseline policy flag");
  require_contains(result, "\"--replay-allow-synthetic-direct-edges\"",
                   "replay should pass synthetic-edge replay flag");
  require_contains(result, "\"--replay-linear-transaction-cost-rate\"",
                   "replay should pass transaction-cost replay flag");
  require_contains(result, "\"--replay-execution-profile-digest\"",
                   "replay should pass execution profile identity digest");
  require_contains(result, "\"profile_digest_fixture\"",
                   "replay should bind execution profile digest value");
  require_contains(result, "\"--replay-policy-set-digest\"",
                   "replay should pass policy set identity digest");
  require_contains(result, "\"policy_set_digest_fixture\"",
                   "replay should bind policy set digest value");

  const std::string run_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"execute\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"base_reserve_node_id\":\"USDT\","
      "\"risky_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_replay\","
      "\"max_steps\":8}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", run_args, &ctx,
                                        &result, &error),
        "replay execution failed with allow_execute=false: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "replay execution returned error: " + result);
  require_contains(result, "\"ok\":true", "replay execution should succeed");
  require_contains(result, "\"replay_completed_count\":\"2\"",
                   "replay should expose replay stdout fields");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_index\"}",
            &ctx, &result, &error),
        "read replay experiment index failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "read replay experiment index returned error: " + result);
  require_contains(result, "runtime_replay_experiments.index",
                   "read_artifact should resolve replay experiment index");
  require_contains(result, "\"entry_0_experiment_id\":\"hero_replay\"",
                   "read_artifact should parse replay experiment index fields");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "read replay experiment report returned error: " + result);
  require_contains(result, "hero_replay.report",
                   "read_artifact should resolve latest replay report");
  require_contains(result, "\"completed_count\":\"2\"",
                   "read_artifact should parse replay report fields");
  require_contains(result, "\"replay_report_integrity\"",
                   "read_artifact should expose replay report integrity");
  require_contains(result, "\"report_digest_bound\":true",
                   "read_artifact should expose declared digest binding");
  require_contains(result, "\"report_digest_match\":true",
                   "read_artifact should verify matching report digest");
  require_contains(result, hero_replay_report_digest,
                   "read_artifact should expose replay report digest");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.inspect",
                                        "{\"subject\":\"job\",\"job_dir\":\"" +
                                            job_dir.string() +
                                            "\",\"include_text\":false}",
                                        &ctx, &result, &error),
        "get_job replay artifact summary failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "get_job replay artifact summary returned error: " + result);
  require_contains(result, "\"replay_experiment_index\"",
                   "get_job should summarize replay experiment index");
  require_contains(result, "\"replay_experiment_report\"",
                   "get_job should summarize replay experiment report");
  require_contains(result, "\"replay_experiment_report_integrity\"",
                   "get_job should summarize replay report integrity");
  require_contains(result, "\"report_digest_match\":true",
                   "get_job should expose matching replay report digest");

  const std::string escaped_replay_report_text =
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=escaped_replay\n"
      "completed_count=999\n";
  const std::string escaped_replay_report_digest =
      replay_report_digest_for_text(escaped_replay_report_text);
  write_text(job_dir / "artifacts" / "outside_replay.report",
             escaped_replay_report_text);
  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=escaped_replay\n"
             "entry_0_policy_count=1\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=1\n"
             "entry_0_completed_count=1\n"
             "entry_0_report_path=../outside_replay.report\n"
             "entry_0_report_digest=" +
                 escaped_replay_report_digest + "\n");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report with unsafe index path failed: " +
            error);
  check(!hero_runtime::tool_result_is_error(result),
        "unsafe replay report index should not crash artifact read: " + result);
  check(result.find("completed_count=999") == std::string::npos &&
            result.find("outside_replay.report") == std::string::npos,
        "Runtime Hero must not follow parent-directory replay report paths "
        "from runtime_replay_experiments.index");
  require_contains(result, "\"report_path_rejected\":true",
                   "unsafe replay report path should be surfaced");
  require_contains(result, "\"report_digest_bound\":true",
                   "unsafe replay report digest binding should remain visible");

  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=hero_replay\n"
             "entry_0_policy_count=2\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n"
             "entry_0_report_path=hero_replay.report\n"
             "entry_0_report_digest=mutated_digest\n");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report with mismatched digest failed: " +
            error);
  check(!hero_runtime::tool_result_is_error(result),
        "mismatched replay report digest should remain readable: " + result);
  require_contains(result, "\"completed_count\":\"2\"",
                   "digest mismatch should not hide replay report content");
  require_contains(result, "\"declared_report_digest\":\"mutated_digest\"",
                   "read_artifact should expose mismatched declared digest");
  require_contains(result, "\"report_digest_bound\":true",
                   "read_artifact should expose mismatched digest binding");
  require_contains(result, "\"report_digest_match\":false",
                   "read_artifact should expose replay digest mismatch");

  write_text(job_dir / "job.state", "status=failed\n");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", dry_args, &ctx,
                                         &result, &error),
        "replay should reject non-completed jobs");
  require_contains(error, "E_RUNTIME_REPLAY_JOB_NOT_COMPLETED",
                   "replay should report non-completed job error");

  std::filesystem::remove_all(dir);
}

void test_canonical_mdn_previews_channel_job() {
  const auto result =
      run_wave_preview("wikimyei.inference.expected_value.mdn", "run|debug");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default protocol id should be cwu_01v");
  require_contains(result, "\"compatible_protocols\":[]",
                   "missing COMPATIBLE_PROTOCOLS is represented as empty");
  require_contains(result, "\"wave_protocol_compatible\":true",
                   "missing COMPATIBLE_PROTOCOLS remains backward compatible");
  require_contains(result,
                   "\"active_representation_family\":\"wikimyei."
                   "representation.encoding.vicreg\"",
                   "default active representation should be VICReg");
  require_contains(result,
                   "\"target_component_family_id\":\"wikimyei.inference."
                   "expected_value.mdn\"",
                   "canonical MDN target field should use component family id");
  require_contains(result, "\"job_kind\":\"channel_inference_mdn\"",
                   "canonical MDN target previews channel job");
  require_contains(result, "\"train_target\":false",
                   "run mode is non-mutating");
  require_contains(result, "\"source_order\":\"sequential\"",
                   "run waves default to sequential source order");
  require_contains(result, "\"source_order_explicit\":false",
                   "omitted run SOURCE_ORDER is reported as implicit");
  require_contains(result, "\"source_order_warning_level\":\"none\"",
                   "run waves do not warn about sequential source order");
}

void test_cwu02_mdn_preview_uses_mtf_representation_chain() {
  const auto result =
      run_wave_preview("wikimyei.inference.expected_value.mdn", "run|debug", "",
                       cwu02_mtf_protocol_text());
  require_contains(result, "\"protocol_id\":\"cwu_02v\"",
                   "cwu_02v protocol id should be surfaced");
  require_contains(result,
                   "\"active_representation_family\":\"wikimyei."
                   "representation.encoding.mtf_jepa_mae_vicreg\"",
                   "cwu_02v active representation should be MTF");
  require_contains(
      result, "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen",
      "cwu_02v MDN preview should route through frozen MTF representation");
}

void test_canonical_vicreg_previews_channel_job() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.vicreg", "train|debug");
  require_contains(result, "\"job_kind\":\"channel_representation_vicreg\"",
                   "canonical VICReg target previews channel job");
  require_contains(result, "\"train_target\":true",
                   "train mode mutates target");
  require_contains(result, "\"source_order\":\"random_per_epoch\"",
                   "train waves default to RandomSampler source order");
  require_contains(result, "\"source_order_explicit\":false",
                   "omitted train SOURCE_ORDER is reported as implicit");
  require_contains(result, "\"source_order_warning_level\":\"none\"",
                   "implicit train source order should not warn");
}

void test_canonical_mtf_previews_separate_representation_job() {
  const auto result =
      run_wave_preview("wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
                       "train|debug", "", cwu02_mtf_protocol_text());
  require_contains(result, "\"protocol_id\":\"cwu_02v\"",
                   "MTF representation preview should use cwu_02v protocol");
  require_contains(result, "\"protocol_target_compatible\":true",
                   "MTF representation target is compatible with cwu_02v");
  require_contains(
      result, "\"job_kind\":\"channel_representation_mtf_jepa_mae_vicreg\"",
      "canonical MTF target previews separate representation job");
  require_contains(result,
                   "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train",
                   "MTF preview reports MTF representation execution chain");
  require_contains(result, "\"train_target\":true",
                   "train mode mutates MTF target");
}

void test_mismatched_protocol_representation_preview_warns() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg", "train|debug");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default preview protocol remains cwu_01v");
  require_contains(result, "\"protocol_target_compatible\":false",
                   "MTF target should warn under cwu_01v");
  require_contains(result,
                   "\"protocol_target_warning\":\"wave_target_not_docked_by_"
                   "active_protocol\"",
                   "MTF target under cwu_01v reports protocol warning");
}

void test_wave_profile_protocol_preview_warns() {
  const auto result = run_wave_preview("wikimyei.inference.expected_value.mdn",
                                       "run|debug", "", "", "cwu_02v");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default preview protocol remains cwu_01v");
  require_contains(result, "\"compatible_protocols\":[\"cwu_02v\"]",
                   "preview reports declared compatible protocols");
  require_contains(result, "\"wave_protocol_compatible\":false",
                   "active cwu_01v should not match cwu_02v-only profile");
  require_contains(result,
                   "\"wave_protocol_warning\":\"active_protocol_not_declared_by"
                   "_wave_profile\"",
                   "profile protocol mismatch reports warning");
}

void test_train_wave_explicit_sequential_source_order_warns() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.vicreg", "train|debug", "sequential");
  require_contains(result, "\"source_order\":\"sequential\"",
                   "explicit sequential train SOURCE_ORDER is preserved");
  require_contains(result, "\"source_order_explicit\":true",
                   "explicit train SOURCE_ORDER is reported");
  require_contains(result, "\"source_order_warning_level\":\"warning\"",
                   "explicit sequential train SOURCE_ORDER warns loudly");
  require_contains(result,
                   "\"source_order_warnings\":\"train_wave_explicit_"
                   "sequential_source_order\"",
                   "explicit sequential train SOURCE_ORDER warning token");
}

void test_source_key_wave_preview_reports_key_bounds() {
  const auto dir = make_tmp_dir("source_key_wave");
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto config_path = dir / ".config";

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = source_key_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = source_key;\n"
                        "  SOURCE_KEY_BEGIN = 1002;\n"
                        "  SOURCE_KEY_END = 1004;\n"
                        "};\n");
  write_text(config_path, "[KIKIJYEBA]\n"
                          "kikijyeba_settings_wave_dsl_path = " +
                              wave_path.string() + "\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave source-key preview failed: " +
                error);
  check(
      !hero_runtime::tool_result_is_error(result),
      "hero.runtime.inspect subject=wave source-key preview returned error: " +
          result);
  require_contains(result, "\"source_range\":\"source_key\"",
                   "source-key preview reports source range");
  require_contains(result, "\"source_key_begin\":\"1002\"",
                   "source-key preview reports begin");
  require_contains(result, "\"source_key_end\":\"1004\"",
                   "source-key preview reports end");
  std::filesystem::remove_all(dir);
}

void test_mdn_wave_preview_reads_jkimyei_model_state_inputs() {
  const auto dir = make_tmp_dir("mdn_jkimyei_inputs_preview");
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto jkimyei_path =
      dir / "wikimyei.inference.expected_value.mdn.jkimyei";
  const auto config_path = dir / ".config";

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = mdn_train;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = train|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = all;\n"
                        "};\n");
  write_text(jkimyei_path, "TRAINING {\n"
                           "  TRAINING_ID = mdn_train;\n"
                           "  TASK = mdn_expected_value_inference;\n"
                           "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
                           "  LEARNING_RATE = 0.001;\n"
                           "  MAX_STEPS = 250;\n"
                           "  BATCH_SIZE = 64;\n"
                           "  INPUT_REPRESENTATION_CHECKPOINT = /tmp/rep.pt;\n"
                           "  INPUT_MDN_CHECKPOINT = /tmp/mdn.pt;\n"
                           "};\n");
  write_text(config_path, "[KIKIJYEBA]\n"
                          "kikijyeba_settings_wave_dsl_path = " +
                              wave_path.string() +
                              "\n"
                              "[JKIMYEI]\n"
                              "wikimyei_inference_expected_value_mdn_"
                              "jkimyei_path = " +
                              jkimyei_path.string() + "\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave jkimyei-input preview failed: " +
                error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.inspect subject=wave jkimyei-input preview returned "
        "error: " +
            result);
  require_contains(
      result, "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"",
      "MDN wave preview should expose jkimyei representation input");
  require_contains(result, "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\"",
                   "MDN wave preview should expose jkimyei MDN input");
  std::filesystem::remove_all(dir);
}

void test_policy_training_causal_schedule_contract() {
  namespace schedule_contract = cuwacunu::kikijyeba::runtime;
  schedule_contract::causal_policy_training_schedule_t schedule{};
  schedule.schedule_id = "causal_policy_training_schedule_1";
  schedule.cursor_key_kind = "numeric_anchor_index";
  schedule.training_range_digest = "range_train_digest";
  schedule.validation_range_digest = "range_validation_digest";
  schedule.test_range_digest = "range_test_digest";

  const auto make_artifact =
      [](std::string digest,
         std::string kind) -> schedule_contract::artifact_fit_ledger_t {
    schedule_contract::artifact_fit_ledger_t artifact{};
    artifact.artifact_digest = std::move(digest);
    artifact.artifact_kind = std::move(kind);
    artifact.artifact_schema_id = "artifact_schema_v1";
    artifact.producer_job_id = "producer_job";
    artifact.fit_cutoff_key = "100";
    artifact.fit_input_cutoff_key = "90";
    artifact.fit_target_cutoff_key = "100";
    artifact.normalization_cutoff_key = "100";
    artifact.calibration_cutoff_key = "100";
    artifact.covariance_fit_cutoff_key = "100";
    artifact.replay_buffer_cutoff_key = "100";
    artifact.target_label_available_from_key = "120";
    artifact.reward_available_from_key = "120";
    artifact.trajectory_usable_from_key = "120";
    artifact.usable_from_key = "120";
    artifact.embargo_policy_id = "target_horizon_embargo.v1";
    artifact.training_block_id = "B0";
    artifact.snapshot_generation = "0";
    return artifact;
  };
  schedule.artifact_fits.push_back(
      make_artifact("rep_snapshot_0", "representation"));
  schedule.artifact_fits.push_back(make_artifact("mdn_snapshot_0", "mdn"));
  schedule.artifact_fits.push_back(
      make_artifact("observer_snapshot_0", "observer_belief"));
  schedule.artifact_fits.push_back(
      make_artifact("policy_snapshot_0", "policy"));
  schedule.artifact_fits.push_back(
      make_artifact("normalization_snapshot_0", "normalization"));
  schedule.artifact_fits.push_back(
      make_artifact("calibration_snapshot_0", "calibration"));
  schedule.artifact_fits.push_back(
      make_artifact("covariance_snapshot_0", "covariance_coupler"));
  schedule.artifact_fits.push_back(
      make_artifact("replay_buffer_snapshot_0", "replay_buffer"));
  schedule.artifact_fits.push_back(
      make_artifact("reward_baseline_snapshot_0", "reward_baseline"));

  schedule_contract::snapshot_family_t family{};
  family.snapshot_family_digest = "snapshot_family_0";
  family.representation_snapshot_digest = "rep_snapshot_0";
  family.mdn_snapshot_digest = "mdn_snapshot_0";
  family.observer_belief_snapshot_digest = "observer_snapshot_0";
  family.policy_snapshot_digest = "policy_snapshot_0";
  family.normalization_snapshot_digest = "normalization_snapshot_0";
  family.calibration_snapshot_digest = "calibration_snapshot_0";
  family.covariance_coupler_snapshot_digest = "covariance_snapshot_0";
  family.replay_buffer_snapshot_digest = "replay_buffer_snapshot_0";
  family.reward_baseline_snapshot_digest = "reward_baseline_snapshot_0";
  schedule.snapshot_families.push_back(family);

  schedule_contract::causal_training_block_t block{};
  block.block_id = "B1";
  block.block_cursor_begin = "200";
  block.block_cursor_end = "300";
  block.block_digest = "block_digest_1";
  block.observation_count = 64;
  block.artifact_use.block_id = block.block_id;
  block.artifact_use.block_cursor_begin = block.block_cursor_begin;
  block.artifact_use.block_cursor_end = block.block_cursor_end;
  block.artifact_use.snapshot_family_digest_used =
      family.snapshot_family_digest;
  block.artifact_use.representation_snapshot_digest_used =
      family.representation_snapshot_digest;
  block.artifact_use.mdn_snapshot_digest_used = family.mdn_snapshot_digest;
  block.artifact_use.observer_belief_snapshot_digest_used =
      family.observer_belief_snapshot_digest;
  block.artifact_use.policy_snapshot_digest_used =
      family.policy_snapshot_digest;
  block.artifact_use.normalization_snapshot_digest_used =
      family.normalization_snapshot_digest;
  block.artifact_use.calibration_snapshot_digest_used =
      family.calibration_snapshot_digest;
  block.artifact_use.covariance_coupler_snapshot_digest_used =
      family.covariance_coupler_snapshot_digest;
  block.artifact_use.replay_buffer_snapshot_digest_used =
      family.replay_buffer_snapshot_digest;
  block.artifact_use.reward_baseline_snapshot_digest_used =
      family.reward_baseline_snapshot_digest;
  block.artifact_use.cajtucu_execution_profile_digest =
      "execution_profile_digest_v1";
  block.artifact_use.reward_contract_digest = "reward_contract_digest_v1";
  block.artifact_use.no_future_snapshot_use = true;
  schedule.blocks.push_back(block);

  auto issues =
      schedule_contract::validate_causal_policy_training_schedule(schedule);
  check(issues.empty(), "causal walk-forward schedule should validate");
  check(!schedule_contract::policy_training_causal_schedule_digest(schedule)
             .empty(),
        "causal walk-forward schedule has a stable digest");
  check(
      schedule_contract::causal_policy_training_schedule_no_future_snapshot_use(
          schedule),
      "no-future-snapshot proof is derived from fit/use ledgers");

  auto declared_false = schedule;
  declared_false.blocks.front().artifact_use.no_future_snapshot_use = false;
  issues = schedule_contract::validate_causal_policy_training_schedule(
      declared_false);
  check(issues.empty(),
        "schedule validity derives no-future-snapshot use from ledgers rather "
        "than trusting the declaration flag");

  auto same_block = schedule;
  same_block.artifact_fits.front().training_block_id = "B1";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(same_block);
  check(has_issue_containing(issues, "same_block_snapshot_use"),
        "schedule rejects snapshots trained on the same block they serve");

  auto future_use = schedule;
  future_use.artifact_fits.front().usable_from_key = "250";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(future_use);
  check(has_issue_containing(issues, "future_snapshot_use"),
        "schedule rejects snapshots unavailable at block start");

  for (const auto &pair :
       {std::pair<std::size_t, std::string>{4, "normalization"},
        std::pair<std::size_t, std::string>{5, "calibration"},
        std::pair<std::size_t, std::string>{6, "covariance_coupler"},
        std::pair<std::size_t, std::string>{7, "replay_buffer"},
        std::pair<std::size_t, std::string>{8, "reward_baseline"}}) {
    auto future_support = schedule;
    future_support.artifact_fits[pair.first].usable_from_key = "250";
    issues = schedule_contract::validate_causal_policy_training_schedule(
        future_support);
    check(has_issue_containing(issues, "future_snapshot_use:" + pair.second),
          "schedule rejects future " + pair.second + " snapshot use");
  }

  auto late_target_label = schedule;
  late_target_label.artifact_fits.front().target_label_available_from_key =
      "130";
  issues = schedule_contract::validate_causal_policy_training_schedule(
      late_target_label);
  check(
      has_issue_containing(issues, "target_label_available_after_usable_from"),
      "schedule rejects labels unavailable by artifact usable_from_key");

  auto late_reward = schedule;
  late_reward.artifact_fits.front().reward_available_from_key = "130";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(late_reward);
  check(has_issue_containing(issues, "reward_available_after_usable_from"),
        "schedule rejects rewards unavailable by artifact usable_from_key");

  auto late_trajectory = schedule;
  late_trajectory.artifact_fits.front().trajectory_usable_from_key = "130";
  issues = schedule_contract::validate_causal_policy_training_schedule(
      late_trajectory);
  check(
      has_issue_containing(issues, "trajectory_available_after_usable_from"),
      "schedule rejects trajectories unavailable by artifact usable_from_key");

  auto opaque_keys = schedule;
  opaque_keys.cursor_key_kind = "opaque_unsortable";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(opaque_keys);
  check(has_issue_containing(issues, "opaque_or_unsupported_cursor_key_kind"),
        "schedule rejects opaque cursor keys without an ordering map");

  auto offline = schedule;
  offline.training_schedule_mode =
      schedule_contract::k_policy_training_schedule_mode_offline_research;
  issues = schedule_contract::validate_causal_policy_training_schedule(offline);
  check(has_issue_containing(
            issues, "offline_full_window_research_not_readiness_eligible"),
        "full-window research cannot satisfy readiness");
}

std::string valid_policy_training_args(std::string requested_mode = "plan") {
  return "{\"operation\":\"policy_training\",\"requested_mode\":\"" +
         requested_mode +
         "\",\"policy_id\":\"wikimyei.policy.rl.ppo_portfolio.v0\","
         "\"policy_kind\":\"ppo\","
         "\"policy_architecture_digest\":\"arch_digest_v0\","
         "\"training_config_digest\":\"train_config_digest_v0\","
         "\"training_range_digest\":\"range_train_digest\","
         "\"validation_range_digest\":\"range_validation_digest\","
         "\"test_range_digest\":\"range_test_digest\","
         "\"environment_contract_id\":\"kikijyeba.environment.replay.v1\","
         "\"observation_schema_digest\":\"observation_schema_digest_v1\","
         "\"action_schema_digest\":\"action_schema_digest_v1\","
         "\"reward_contract_digest\":\"reward_contract_digest_v1\","
         "\"execution_profile_digest\":\"execution_profile_digest_v1\","
         "\"training_schedule_mode\":\"causal_walk_forward_training.v1\","
         "\"causal_schedule_schema_id\":\"kikijyeba.runtime.policy_training_"
         "causal_schedule.v1\","
         "\"causal_schedule_digest\":\"causal_schedule_digest_v1\","
         "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\","
         "\"causal_schedule_no_future_snapshot_use_source\":\"derived_from_"
         "artifact_fit_use_ledgers\","
         "\"normalization_fit_range_digest\":\"range_train_digest\","
         "\"replay_buffer_source_range_digest\":\"range_train_digest\","
         "\"early_stopping_policy_digest\":\"early_stop_digest_v1\","
         "\"hyperparameter_selection_policy_digest\":\"selector_digest_v1\","
         "\"selector_split\":\"validation\","
         "\"parent_replay_environment_fact_digest\":\"replay_env_fact_digest\","
         "\"max_episodes\":4,"
         "\"max_steps\":64,"
         "\"max_parallel_jobs\":1,"
         "\"max_wall_clock_seconds\":120,"
         "\"causal_schedule_readiness_eligible\":true,"
         "\"causal_schedule_no_future_snapshot_use\":true,"
         "\"offline_full_window_research_allowed\":false,"
         "\"final_refit_uses_validation\":false,"
         "\"validation_no_longer_proof\":false,"
         "\"sealed_test_required\":false,"
         "\"live_execution_allowed\":false}";
}

void test_policy_training_contract_operation_is_plan_only() {
  hero_runtime::runtime_context_t ctx{};
  std::string result;
  std::string error;
  check(hero_runtime::execute_tool_json("hero.runtime.run",
                                        valid_policy_training_args("plan"),
                                        &ctx, &result, &error),
        "policy-training plan failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "policy-training plan returned error: " + result);
  require_contains(result,
                   "\"schema_version\":\"kikijyeba.runtime.policy_training_"
                   "job_contract_packet.v1\"",
                   "policy-training plan reports contract packet schema");
  require_contains(result, "\"contract_digest\":",
                   "policy-training plan reports contract digest");
  require_contains(result, "\"policy_training_execution_supported\":false",
                   "policy-training plan remains contract-only");
  require_contains(result,
                   "\"normalization_fit_range_digest\":\"range_train_digest\"",
                   "policy-training plan binds normalization fit range");
  require_contains(result,
                   "\"training_schedule_mode\":\"causal_walk_forward_"
                   "training.v1\"",
                   "policy-training plan binds causal schedule mode");
  require_contains(result,
                   "\"causal_schedule_digest\":\"causal_schedule_digest_v1\"",
                   "policy-training plan binds causal schedule digest");
  require_contains(
      result, "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\"",
      "policy-training plan binds causal cursor key ordering");
  require_contains(result,
                   "\"causal_schedule_no_future_snapshot_use_source\":"
                   "\"derived_from_artifact_fit_use_ledgers\"",
                   "policy-training plan binds derived no-future proof source");
  require_contains(result, "\"runtime_trains_policy\":false",
                   "policy-training plan denies training authority");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run",
                                        valid_policy_training_args("dry_run"),
                                        &ctx, &result, &error),
        "policy-training dry_run failed: " + error);
  require_contains(result, "\"requested_mode\":\"dry_run\"",
                   "policy-training dry_run reports requested mode");

  std::string leaking_args = valid_policy_training_args("plan");
  const std::string validation_digest_field =
      "\"validation_range_digest\":\"range_validation_digest\"";
  const auto validation_pos = leaking_args.find(validation_digest_field);
  check(validation_pos != std::string::npos,
        "test fixture should contain validation range digest");
  leaking_args.replace(validation_pos, validation_digest_field.size(),
                       "\"validation_range_digest\":\"range_train_digest\"");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", leaking_args, &ctx,
                                         &result, &error),
        "policy-training contract should reject train/validation overlap");
  require_contains(error, "training_validation_range_overlap",
                   "policy-training overlap refusal should name the issue");

  std::string scheduleless_args = valid_policy_training_args("plan");
  const std::string schedule_digest_field =
      "\"causal_schedule_digest\":\"causal_schedule_digest_v1\",";
  const auto schedule_digest_pos =
      scheduleless_args.find(schedule_digest_field);
  check(schedule_digest_pos != std::string::npos,
        "test fixture should contain causal schedule digest");
  scheduleless_args.erase(schedule_digest_pos, schedule_digest_field.size());
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", scheduleless_args,
                                         &ctx, &result, &error),
        "policy-training contract should reject missing causal schedule");
  require_contains(error, "missing required field: causal_schedule_digest",
                   "missing causal schedule refusal should be stable");

  std::string missing_source_args = valid_policy_training_args("plan");
  const std::string schedule_source_field =
      "\"causal_schedule_no_future_snapshot_use_source\":\"derived_from_"
      "artifact_fit_use_ledgers\",";
  const auto schedule_source_pos =
      missing_source_args.find(schedule_source_field);
  check(schedule_source_pos != std::string::npos,
        "test fixture should contain causal schedule no-future source");
  missing_source_args.erase(schedule_source_pos, schedule_source_field.size());
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", missing_source_args, &ctx, &result, &error),
        "policy-training contract should reject missing causal schedule "
        "no-future source");
  require_contains(
      error,
      "missing required field: causal_schedule_no_future_snapshot_use_source",
      "missing causal schedule no-future source refusal should be stable");

  std::string asserted_source_args = valid_policy_training_args("plan");
  const auto asserted_source_pos =
      asserted_source_args.find(schedule_source_field);
  check(asserted_source_pos != std::string::npos,
        "test fixture should contain causal schedule no-future source");
  asserted_source_args.replace(
      asserted_source_pos, schedule_source_field.size(),
      "\"causal_schedule_no_future_snapshot_use_source\":\"asserted_by_"
      "caller\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", asserted_source_args, &ctx, &result, &error),
        "policy-training contract should reject caller-asserted no-future "
        "source");
  require_contains(error, "unsupported_no_future_snapshot_use_source",
                   "caller-asserted no-future source refusal should be stable");

  std::string opaque_cursor_args = valid_policy_training_args("plan");
  const std::string cursor_kind_field =
      "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\",";
  const auto cursor_kind_pos = opaque_cursor_args.find(cursor_kind_field);
  check(cursor_kind_pos != std::string::npos,
        "test fixture should contain causal schedule cursor kind");
  opaque_cursor_args.replace(
      cursor_kind_pos, cursor_kind_field.size(),
      "\"causal_schedule_cursor_key_kind\":\"opaque_unsortable\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", opaque_cursor_args,
                                         &ctx, &result, &error),
        "policy-training contract should reject opaque cursor key ordering");
  require_contains(error,
                   "opaque_or_unsupported_causal_schedule_cursor_key_kind",
                   "opaque cursor key refusal should be stable");

  std::string offline_args = valid_policy_training_args("plan");
  const std::string causal_mode =
      "\"training_schedule_mode\":\"causal_walk_forward_training.v1\"";
  const auto causal_mode_pos = offline_args.find(causal_mode);
  check(causal_mode_pos != std::string::npos,
        "test fixture should contain causal schedule mode");
  offline_args.replace(causal_mode_pos, causal_mode.size(),
                       "\"training_schedule_mode\":\"offline_full_window_"
                       "research\"");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", offline_args, &ctx,
                                         &result, &error),
        "policy-training contract should reject full-window research as "
        "readiness evidence");
  require_contains(error, "offline_full_window_research_not_readiness_eligible",
                   "full-window research refusal should name readiness leak");

  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run",
                                         valid_policy_training_args("execute"),
                                         &ctx, &result, &error),
        "policy-training execute should be refused");
  require_contains(error, "E_RUNTIME_POLICY_TRAINING_EXECUTION_NOT_IMPLEMENTED",
                   "policy-training execute refusal should be stable");
}

} // namespace

int main() {
  try {
    test_canonical_mdn_previews_channel_job();
    test_cwu02_mdn_preview_uses_mtf_representation_chain();
    test_canonical_vicreg_previews_channel_job();
    test_canonical_mtf_previews_separate_representation_job();
    test_mismatched_protocol_representation_preview_warns();
    test_wave_profile_protocol_preview_warns();
    test_train_wave_explicit_sequential_source_order_warns();
    test_wave_settings_train_defaults_to_random_source_order();
    test_source_key_wave_preview_reports_key_bounds();
    test_mdn_wave_preview_reads_jkimyei_model_state_inputs();
    test_policy_training_causal_schedule_contract();
    test_policy_training_contract_operation_is_plan_only();
    test_execute_expected_wave_binding();
    test_execute_wave_overlay();
    test_replay_operator_tool();
    std::cout << "hero runtime wave preview tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
