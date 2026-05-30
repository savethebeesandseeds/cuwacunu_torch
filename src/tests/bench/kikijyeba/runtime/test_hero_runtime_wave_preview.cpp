#include "hero/runtime_hero/hero_runtime_tools.h"
#include "kikijyeba/marshal/digest.h"
#include "kikijyeba/settings/wave.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

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
      "hero.runtime.wave", "{\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.wave failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.wave returned error: " + result);
  std::filesystem::remove_all(dir);
  return result;
}

void require_contains(const std::string &text, std::string_view needle,
                      const std::string &message) {
  check(text.find(needle) != std::string::npos,
        message + " missing needle: " + std::string(needle) + "\n" + text);
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
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"marshal_expected_wave\":{"
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
  check(hero_runtime::execute_tool_json("hero.runtime.execute", good_args, &ctx,
                                        &result, &error),
        "expected-wave-bound execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "expected-wave-bound execute returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "expected-wave-bound execute should run when fields match");

  const std::string handoff_args =
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"confirm_execute\":false,\"runtime_handoff\":{"
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
  check(hero_runtime::execute_tool_json("hero.runtime.execute", handoff_args,
                                        &ctx, &result, &error),
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
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"confirm_execute\":false,\"runtime_handoff\":{"
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
      "hero.runtime.execute", stale_hash_handoff_args, &ctx, &result, &error);
  check(!stale_hash_ok,
        "runtime_handoff stale base config hash should fail before execution: "
        "result=" +
            result + " error=" + error);
  check(error.find("base_config path/hash") != std::string::npos,
        "runtime_handoff stale hash should report base_config hash failure");

  const std::string unresolved_handoff_args =
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"confirm_execute\":false,\"runtime_handoff\":{"
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
  check(!hero_runtime::execute_tool_json("hero.runtime.execute",
                                         unresolved_handoff_args, &ctx, &result,
                                         &error),
        "runtime_handoff unresolved selector should fail before execution");
  check(error.find("E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS") != std::string::npos,
        "runtime_handoff unresolved selector should report unresolved symbols");

  const std::string bad_args =
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"marshal_expected_wave\":{"
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
  check(!hero_runtime::execute_tool_json("hero.runtime.execute", bad_args, &ctx,
                                         &result, &error),
        "expected-wave mismatch should fail before execution");
  check(error.find("E_RUNTIME_EXPECTED_WAVE_MISMATCH") != std::string::npos,
        "expected-wave mismatch should report a specific error");

  const std::string bad_checkpoint_args =
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,\"marshal_expected_wave\":{"
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
            "hero.runtime.execute", bad_checkpoint_args, &ctx, &result, &error),
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
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,"
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
  check(hero_runtime::execute_tool_json("hero.runtime.execute", args, &ctx,
                                        &result, &error),
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
      "{\"config_path\":\"" + config_path.string() +
      "\",\"dry_run\":true,"
      "\"wave_overlay\":{\"source_range\":\"all\","
      "\"anchor_index_begin\":\"1\",\"anchor_index_end\":\"3\"}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.execute", invalid_args,
                                         &ctx, &result, &error),
        "invalid wave overlay should fail");
  check(error.find("E_RUNTIME_WAVE_OVERLAY_INVALID") != std::string::npos,
        "invalid wave overlay reports overlay error");

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
      "hero.runtime.wave", "{\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.wave source-key preview failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.wave source-key preview returned error: " + result);
  require_contains(result, "\"source_range\":\"source_key\"",
                   "source-key preview reports source range");
  require_contains(result, "\"source_key_begin\":\"1002\"",
                   "source-key preview reports begin");
  require_contains(result, "\"source_key_end\":\"1004\"",
                   "source-key preview reports end");
  std::filesystem::remove_all(dir);
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
    test_execute_expected_wave_binding();
    test_execute_wave_overlay();
    std::cout << "hero runtime wave preview tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
