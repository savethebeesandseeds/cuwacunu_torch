#include "kikijyeba/marshal/batch_preview.h"
#include "kikijyeba/marshal/codex_assist.h"
#include "kikijyeba/marshal/dispatch_adapter.h"
#include "kikijyeba/marshal/dispatch_advice.h"
#include "kikijyeba/marshal/dispatch_operation.h"
#include "kikijyeba/marshal/dispatch_receipt.h"
#include "kikijyeba/marshal/execution_gate.h"
#include "kikijyeba/marshal/runtime_hero_handoff.h"
#include "kikijyeba/marshal/status.h"
#include "kikijyeba/marshal/tool_handler.h"
#include "kikijyeba/marshal/tool_schema.h"
#include "tests/bench/kikijyeba/test_support/lattice_forecast_artifact_fixture.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace marshal = cuwacunu::kikijyeba::marshal;
namespace lattice_fixture = cuwacunu::tests::kikijyeba::lattice_fixture;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_marshal_" + label + "_" +
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

std::string shell_quote(const std::string &value) {
  std::string out = "'";
  for (const char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out += "'";
  return out;
}

std::string read_command_stdout(const std::string &command) {
  std::array<char, 4096> buffer{};
  std::string out;
  FILE *pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen failed for command: " + command);
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    out.append(buffer.data());
  }
  const int status = pclose(pipe);
  if (status != 0) {
    throw std::runtime_error("command failed: " + command + "\n" + out);
  }
  return out;
}

std::filesystem::path
first_existing(const std::vector<std::filesystem::path> &candidates) {
  for (const auto &candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  std::ostringstream msg;
  msg << "missing expected file:";
  for (const auto &candidate : candidates) {
    msg << " " << candidate.string();
  }
  throw std::runtime_error(msg.str());
}

std::filesystem::path hero_lattice_binary_path() {
  return first_existing({"/cuwacunu/.build/hero/hero_lattice.mcp",
                         "/cuwacunu/.build/hero/hero_lattice_mcp"});
}

marshal::marshal_active_identity_t active_identity() {
  marshal::marshal_active_identity_t identity{};
  identity.protocol_contract_fingerprint = "contract_1";
  identity.graph_order_fingerprint = "graph_1";
  identity.target_spec_fingerprint = "target_spec_1";
  identity.split_policy_fingerprint = "split_policy_1";
  return identity;
}

marshal::marshal_dispatch_validation_context_t context() {
  marshal::marshal_dispatch_validation_context_t out{};
  out.active_config_path = "/cuwacunu/src/config/.config";
  out.active_runtime_root = "/cuwacunu/.runtime/cuwacunu_exec";
  out.active_identity = active_identity();
  out.supported_wave_targets.insert("wikimyei.inference.expected_value.mdn");
  out.allowed_model_state_roots.push_back("/cuwacunu/.runtime/cuwacunu_exec");
  out.freshness_check_timestamp_utc = "2026-05-23T00:10:00Z";
  out.max_advice_age_seconds = 3600;
  return out;
}

marshal::marshal_dispatch_advice_t valid_advice() {
  marshal::marshal_dispatch_advice_t advice{};
  advice.config_path = "/cuwacunu/src/config/.config";
  advice.runtime_root = "/cuwacunu/.runtime/cuwacunu_exec";
  advice.target_id = "channel_mdn_validation_eval_ready";
  advice.target_status = "blocked";
  advice.active_identity = active_identity();
  advice.plan_basis.present = true;
  advice.plan_basis.available = true;
  advice.plan_basis.source_range = "anchor_index";
  advice.plan_basis.target_anchor_index_begin = 1800;
  advice.plan_basis.target_anchor_index_end = 2050;
  advice.plan_basis.primary_deficit_key = "coverage:evaluation_metric";
  advice.plan_basis.primary_deficit_message =
      "validation evaluation coverage missing";
  advice.plan_basis.deficit_keys = {"coverage:evaluation_metric"};
  advice.plan_basis.deficit_priority_classes = {"coverage"};
  advice.suggested_wave.target = "wikimyei.inference.expected_value.mdn";
  advice.suggested_wave.mode = "run|debug";
  advice.suggested_wave.source_range = "anchor_index";
  advice.suggested_wave.anchor_index_begin = 1800;
  advice.suggested_wave.anchor_index_end = 2050;
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt";
  advice.suggested_wave.plan_inputs["PLAN_INPUT_REPRESENTATION_CHECKPOINT"] =
      "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt";
  advice.max_waves = 1;
  advice.recommendation_attempt_count = 0;
  advice.required_plan_inputs = {"PLAN_INPUT_MDN_CHECKPOINT",
                                 "PLAN_INPUT_REPRESENTATION_CHECKPOINT"};
  advice.source_lattice_tool = "hero.lattice.evaluate_target";
  advice.source_lattice_timestamp = "2026-05-23T00:00:00Z";
  advice.plan_basis_digest = marshal::plan_basis_digest(advice.plan_basis);
  advice.suggested_wave_digest =
      marshal::suggested_wave_digest(advice.suggested_wave);
  advice.plan_input_digest =
      marshal::plan_input_digest(advice.suggested_wave.plan_inputs);
  return advice;
}

marshal::marshal_dispatch_request_t
valid_request(const marshal::marshal_dispatch_advice_t &advice) {
  marshal::marshal_dispatch_request_t request{};
  request.requested_mode = marshal::marshal_dispatch_mode_t::dry_run;
  request.target_id = advice.target_id;
  request.config_path = advice.config_path;
  request.runtime_root = advice.runtime_root;
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  return request;
}

marshal::marshal_runtime_policy_snapshot_t valid_policy() {
  marshal::marshal_runtime_policy_snapshot_t policy{};
  policy.runtime_hero_available = true;
  policy.runtime_exec_exists = true;
  policy.runtime_exec_executable = true;
  policy.default_dry_run = true;
  policy.allow_execute = false;
  policy.allow_train_execute = false;
  policy.runtime_root = "/cuwacunu/.runtime/cuwacunu_exec";
  return policy;
}

marshal::marshal_runtime_wave_snapshot_t
valid_active_wave(const marshal::marshal_dispatch_advice_t &advice) {
  marshal::marshal_runtime_wave_snapshot_t wave{};
  wave.available = true;
  wave.wave_id = "cwu_01v_channel_validation_eval_mdn_1800_2050";
  wave.target_component_family_id = advice.suggested_wave.target;
  wave.mode = advice.suggested_wave.mode;
  wave.source_range = advice.suggested_wave.source_range;
  wave.source_order = "sequential";
  wave.anchor_index_begin = advice.suggested_wave.anchor_index_begin;
  wave.anchor_index_end = advice.suggested_wave.anchor_index_end;
  wave.job_kind = "channel_inference_mdn";
  wave.train_target = false;
  wave.model_state_inputs = advice.suggested_wave.plan_inputs;
  return wave;
}

marshal::marshal_prior_dry_run_evidence_t prior_dry_run_evidence(
    const marshal::marshal_dispatch_advice_t &advice,
    const marshal::marshal_dispatch_request_t &execute_request,
    const marshal::marshal_dispatch_validation_context_t &gate_context,
    const marshal::marshal_runtime_policy_snapshot_t &policy,
    const marshal::marshal_runtime_wave_snapshot_t &wave) {
  auto dry_run_request = execute_request;
  dry_run_request.requested_mode = marshal::marshal_dispatch_mode_t::dry_run;
  dry_run_request.operator_confirmation_token.clear();
  const auto dry_run_decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, dry_run_request, gate_context, policy, wave);
  marshal::marshal_dry_run_dispatch_response_t dry_run_response{};
  dry_run_response.accepted = dry_run_decision.accepted;
  dry_run_response.advice_digest = marshal::dispatch_advice_digest(advice);
  dry_run_response.request_digest =
      marshal::dispatch_request_digest(dry_run_request);
  dry_run_response.decision = dry_run_decision;
  dry_run_response.runtime_request_digest =
      marshal::runtime_dry_run_request_digest(dry_run_decision.runtime_request);
  dry_run_response.runtime_handoff.ok = dry_run_decision.accepted;
  dry_run_response.runtime_handoff.tool_call_ok = dry_run_decision.accepted;
  dry_run_response.runtime_handoff.runtime_dry_run_ok =
      dry_run_decision.accepted;
  dry_run_response.runtime_handoff.tool_result_json =
      dry_run_decision.accepted ? "{\"ok\":true}" : "{\"ok\":false}";
  dry_run_response.runtime_dry_run_output_included = dry_run_decision.accepted;
  dry_run_response.response_digest =
      marshal::dry_run_dispatch_response_digest(dry_run_response);
  return marshal::make_prior_dry_run_evidence(advice, dry_run_request,
                                              dry_run_response, policy, wave);
}

std::string string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << marshal::detail::json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

std::string string_map_json(const std::map<std::string, std::string> &values) {
  std::ostringstream out;
  out << "{";
  std::size_t i = 0;
  for (const auto &[key, value] : values) {
    if (i++ != 0) {
      out << ",";
    }
    out << marshal::detail::json_quote(key) << ":"
        << marshal::detail::json_quote(value);
  }
  out << "}";
  return out.str();
}

std::string optional_size_json(const std::optional<std::size_t> &value) {
  if (!value.has_value()) {
    return "null";
  }
  return std::to_string(*value);
}

std::string active_identity_json(const marshal::marshal_active_identity_t &id) {
  std::ostringstream out;
  out << "{\"protocol_contract_fingerprint\":"
      << marshal::detail::json_quote(id.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << marshal::detail::json_quote(id.graph_order_fingerprint)
      << ",\"target_spec_fingerprint\":"
      << marshal::detail::json_quote(id.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << marshal::detail::json_quote(id.split_policy_fingerprint) << "}";
  return out.str();
}

std::string plan_basis_json(const marshal::marshal_plan_basis_t &basis) {
  std::ostringstream out;
  out << "{\"present\":" << (basis.present ? "true" : "false")
      << ",\"available\":" << (basis.available ? "true" : "false")
      << ",\"source_range\":" << marshal::detail::json_quote(basis.source_range)
      << ",\"target_anchor_index_begin\":"
      << optional_size_json(basis.target_anchor_index_begin)
      << ",\"target_anchor_index_end\":"
      << optional_size_json(basis.target_anchor_index_end)
      << ",\"primary_deficit_key\":"
      << marshal::detail::json_quote(basis.primary_deficit_key)
      << ",\"primary_deficit_message\":"
      << marshal::detail::json_quote(basis.primary_deficit_message)
      << ",\"deficit_keys\":" << string_array_json(basis.deficit_keys)
      << ",\"deficit_priority_classes\":"
      << string_array_json(basis.deficit_priority_classes) << "}";
  return out.str();
}

std::string suggested_wave_json(const marshal::marshal_suggested_wave_t &wave) {
  std::ostringstream out;
  out << "{\"target\":" << marshal::detail::json_quote(wave.target)
      << ",\"mode\":" << marshal::detail::json_quote(wave.mode)
      << ",\"source_range\":" << marshal::detail::json_quote(wave.source_range)
      << ",\"anchor_index_begin\":"
      << optional_size_json(wave.anchor_index_begin)
      << ",\"anchor_index_end\":" << optional_size_json(wave.anchor_index_end)
      << ",\"plan_inputs\":" << string_map_json(wave.plan_inputs) << "}";
  return out.str();
}

std::string advice_json(const marshal::marshal_dispatch_advice_t &advice) {
  std::ostringstream out;
  out << "{\"schema_version\":"
      << marshal::detail::json_quote(advice.schema_version)
      << ",\"config_path\":" << marshal::detail::json_quote(advice.config_path)
      << ",\"runtime_root\":"
      << marshal::detail::json_quote(advice.runtime_root)
      << ",\"target_id\":" << marshal::detail::json_quote(advice.target_id)
      << ",\"target_status\":"
      << marshal::detail::json_quote(advice.target_status)
      << ",\"active_identity\":" << active_identity_json(advice.active_identity)
      << ",\"plan_basis\":" << plan_basis_json(advice.plan_basis)
      << ",\"plan_basis_digest\":"
      << marshal::detail::json_quote(advice.plan_basis_digest)
      << ",\"suggested_wave\":" << suggested_wave_json(advice.suggested_wave)
      << ",\"suggested_wave_digest\":"
      << marshal::detail::json_quote(advice.suggested_wave_digest)
      << ",\"plan_input_digest\":"
      << marshal::detail::json_quote(advice.plan_input_digest)
      << ",\"max_waves\":" << advice.max_waves
      << ",\"recommendation_attempt_count\":"
      << advice.recommendation_attempt_count << ",\"required_plan_inputs\":"
      << string_array_json(advice.required_plan_inputs)
      << ",\"source_lattice_tool\":"
      << marshal::detail::json_quote(advice.source_lattice_tool)
      << ",\"source_lattice_timestamp\":"
      << marshal::detail::json_quote(advice.source_lattice_timestamp)
      << ",\"advice_receipt_digest\":"
      << marshal::detail::json_quote(advice.advice_receipt_digest)
      << ",\"advice_signature_key_id\":"
      << marshal::detail::json_quote(advice.advice_signature_key_id)
      << ",\"advice_signature\":"
      << marshal::detail::json_quote(advice.advice_signature) << "}";
  return out.str();
}

std::string request_json(const marshal::marshal_dispatch_request_t &request) {
  std::ostringstream out;
  out << "{\"schema_version\":"
      << marshal::detail::json_quote(request.schema_version)
      << ",\"requested_mode\":"
      << marshal::detail::json_quote(marshal::to_string(request.requested_mode))
      << ",\"target_id\":" << marshal::detail::json_quote(request.target_id)
      << ",\"config_path\":" << marshal::detail::json_quote(request.config_path)
      << ",\"runtime_root\":"
      << marshal::detail::json_quote(request.runtime_root)
      << ",\"advice_digest\":"
      << marshal::detail::json_quote(request.advice_digest)
      << ",\"operator_confirmation_token\":"
      << marshal::detail::json_quote(request.operator_confirmation_token)
      << ",\"requested_overrides\":"
      << string_map_json(request.requested_overrides) << "}";
  return out.str();
}

std::string
context_json(const marshal::marshal_dispatch_validation_context_t &ctx) {
  std::vector<std::string> supported_targets(ctx.supported_wave_targets.begin(),
                                             ctx.supported_wave_targets.end());
  std::vector<std::string> supported_ranges(ctx.supported_source_ranges.begin(),
                                            ctx.supported_source_ranges.end());
  std::vector<std::string> allowed_tools(ctx.allowed_lattice_tools.begin(),
                                         ctx.allowed_lattice_tools.end());
  std::ostringstream out;
  out << "{\"active_config_path\":"
      << marshal::detail::json_quote(ctx.active_config_path)
      << ",\"active_runtime_root\":"
      << marshal::detail::json_quote(ctx.active_runtime_root)
      << ",\"active_identity\":" << active_identity_json(ctx.active_identity)
      << ",\"supported_wave_targets\":" << string_array_json(supported_targets)
      << ",\"supported_source_ranges\":" << string_array_json(supported_ranges)
      << ",\"allowed_lattice_tools\":" << string_array_json(allowed_tools)
      << ",\"allowed_model_state_roots\":"
      << string_array_json(ctx.allowed_model_state_roots)
      << ",\"freshness_check_timestamp_utc\":"
      << marshal::detail::json_quote(ctx.freshness_check_timestamp_utc)
      << ",\"max_advice_age_seconds\":" << ctx.max_advice_age_seconds
      << ",\"allowed_clock_skew_seconds\":" << ctx.allowed_clock_skew_seconds
      << ",\"require_signed_lattice_advice\":"
      << (ctx.require_signed_lattice_advice ? "true" : "false")
      << ",\"trusted_lattice_advice_key_id\":"
      << marshal::detail::json_quote(ctx.trusted_lattice_advice_key_id)
      << ",\"trusted_lattice_advice_verification_key\":"
      << marshal::detail::json_quote(
             ctx.trusted_lattice_advice_verification_key)
      << ",\"allow_execute\":" << (ctx.allow_execute ? "true" : "false")
      << ",\"allow_execute_mode\":"
      << (ctx.allow_execute_mode ? "true" : "false") << "}";
  return out.str();
}

std::string
policy_json(const marshal::marshal_runtime_policy_snapshot_t &policy) {
  std::ostringstream out;
  out << "{\"runtime_hero_available\":"
      << (policy.runtime_hero_available ? "true" : "false")
      << ",\"runtime_exec_exists\":"
      << (policy.runtime_exec_exists ? "true" : "false")
      << ",\"runtime_exec_executable\":"
      << (policy.runtime_exec_executable ? "true" : "false")
      << ",\"default_dry_run\":" << (policy.default_dry_run ? "true" : "false")
      << ",\"allow_execute\":" << (policy.allow_execute ? "true" : "false")
      << ",\"allow_train_execute\":"
      << (policy.allow_train_execute ? "true" : "false") << ",\"runtime_root\":"
      << marshal::detail::json_quote(policy.runtime_root) << "}";
  return out.str();
}

std::string wave_json(const marshal::marshal_runtime_wave_snapshot_t &wave) {
  std::ostringstream out;
  out << "{\"available\":" << (wave.available ? "true" : "false")
      << ",\"wave_id\":" << marshal::detail::json_quote(wave.wave_id)
      << ",\"target_component_family_id\":"
      << marshal::detail::json_quote(wave.target_component_family_id)
      << ",\"mode\":" << marshal::detail::json_quote(wave.mode)
      << ",\"source_range\":" << marshal::detail::json_quote(wave.source_range)
      << ",\"source_order\":" << marshal::detail::json_quote(wave.source_order)
      << ",\"anchor_index_begin\":"
      << optional_size_json(wave.anchor_index_begin)
      << ",\"anchor_index_end\":" << optional_size_json(wave.anchor_index_end)
      << ",\"job_kind\":" << marshal::detail::json_quote(wave.job_kind)
      << ",\"train_target\":" << (wave.train_target ? "true" : "false")
      << ",\"model_state_inputs\":" << string_map_json(wave.model_state_inputs)
      << "}";
  return out.str();
}

std::string prior_json(const marshal::marshal_prior_dry_run_evidence_t &prior) {
  std::ostringstream out;
  out << "{\"present\":" << (prior.present ? "true" : "false")
      << ",\"accepted\":" << (prior.accepted ? "true" : "false")
      << ",\"advice_digest\":"
      << marshal::detail::json_quote(prior.advice_digest)
      << ",\"dispatch_request_identity_digest\":"
      << marshal::detail::json_quote(prior.dispatch_request_identity_digest)
      << ",\"runtime_preview_request_digest\":"
      << marshal::detail::json_quote(prior.runtime_preview_request_digest)
      << ",\"suggested_wave_digest\":"
      << marshal::detail::json_quote(prior.suggested_wave_digest)
      << ",\"plan_input_digest\":"
      << marshal::detail::json_quote(prior.plan_input_digest)
      << ",\"runtime_policy_digest\":"
      << marshal::detail::json_quote(prior.runtime_policy_digest)
      << ",\"runtime_wave_digest\":"
      << marshal::detail::json_quote(prior.runtime_wave_digest)
      << ",\"runtime_response_digest\":"
      << marshal::detail::json_quote(prior.runtime_response_digest)
      << ",\"dry_run_response_digest\":"
      << marshal::detail::json_quote(prior.dry_run_response_digest)
      << ",\"evidence_digest\":"
      << marshal::detail::json_quote(prior.evidence_digest) << "}";
  return out.str();
}

std::string receipt_json(const marshal::marshal_dispatch_receipt_t &receipt) {
  std::ostringstream out;
  out << "{\"schema_version\":"
      << marshal::detail::json_quote(receipt.schema_version)
      << ",\"retention_class\":"
      << marshal::detail::json_quote(receipt.retention_class)
      << ",\"receipt_id\":" << marshal::detail::json_quote(receipt.receipt_id)
      << ",\"receipt_kind\":"
      << marshal::detail::json_quote(receipt.receipt_kind)
      << ",\"timestamp\":" << marshal::detail::json_quote(receipt.timestamp)
      << ",\"target_id\":" << marshal::detail::json_quote(receipt.target_id)
      << ",\"lattice_proof_context\":"
      << active_identity_json(receipt.lattice_proof_context)
      << ",\"plan_basis_digest\":"
      << marshal::detail::json_quote(receipt.plan_basis_digest)
      << ",\"suggested_wave_digest\":"
      << marshal::detail::json_quote(receipt.suggested_wave_digest)
      << ",\"advice_digest\":"
      << marshal::detail::json_quote(receipt.advice_digest)
      << ",\"request_digest\":"
      << marshal::detail::json_quote(receipt.request_digest)
      << ",\"decision_digest\":"
      << marshal::detail::json_quote(receipt.decision_digest)
      << ",\"runtime_request_digest\":"
      << marshal::detail::json_quote(receipt.runtime_request_digest)
      << ",\"runtime_preview_request_digest\":"
      << marshal::detail::json_quote(receipt.runtime_preview_request_digest)
      << ",\"runtime_execution_request_digest\":"
      << marshal::detail::json_quote(receipt.runtime_execution_request_digest)
      << ",\"runtime_handoff_arguments_digest\":"
      << marshal::detail::json_quote(receipt.runtime_handoff_arguments_digest)
      << ",\"runtime_response_digest\":"
      << marshal::detail::json_quote(receipt.runtime_response_digest)
      << ",\"policy_decision\":"
      << marshal::detail::json_quote(receipt.policy_decision)
      << ",\"confirmation_status\":"
      << marshal::detail::json_quote(receipt.confirmation_status)
      << ",\"refusal_reasons\":" << string_array_json(receipt.refusal_reasons)
      << ",\"compacted_fields\":" << string_array_json(receipt.compacted_fields)
      << ",\"runtime_tool_result_json\":"
      << marshal::detail::json_quote(receipt.runtime_tool_result_json)
      << ",\"post_run_verification\":"
      << marshal::detail::json_quote(receipt.post_run_verification)
      << ",\"non_authority_statement\":"
      << marshal::detail::json_quote(receipt.non_authority_statement) << "}";
  return out.str();
}

void test_valid_advice() {
  const auto advice = valid_advice();
  const auto request = valid_request(advice);
  const auto result =
      marshal::validate_dispatch_advice(advice, request, context());

  check(result.dispatchable, "valid dispatch advice should be dispatchable");
  check(result.refusal_reasons.empty(),
        "valid dispatch advice should not carry refusal reasons");
  check(result.advice_digest == marshal::dispatch_advice_digest(advice),
        "validation should expose the canonical advice digest");
  check(marshal::marshal_digest_is_strong_hex(result.advice_digest),
        "advice digest should use a 256-bit hex digest");
  check(result.request_digest == marshal::dispatch_request_digest(request),
        "validation should expose the canonical request digest");
  check(result.non_authority_statement.find("target satisfaction remains") !=
            std::string::npos,
        "validation should carry the non-authority statement");

  auto reordered = advice;
  reordered.required_plan_inputs = {"PLAN_INPUT_REPRESENTATION_CHECKPOINT",
                                    "PLAN_INPUT_MDN_CHECKPOINT"};
  check(marshal::dispatch_advice_digest(advice) ==
            marshal::dispatch_advice_digest(reordered),
        "required input ordering should not change the advice digest");
}

void test_stale_contract_status_remains_dispatchable() {
  auto advice = valid_advice();
  advice.target_status = "stale_contract";
  advice.plan_basis.primary_deficit_key = "proof_context:contract_fingerprint";
  advice.plan_basis.primary_deficit_message =
      "evidence contract fingerprint does not match the active contract "
      "fingerprint";
  advice.plan_basis.deficit_keys = {"proof_context:contract_fingerprint"};
  advice.plan_basis.deficit_priority_classes = {"proof_context"};
  advice.plan_basis_digest = marshal::plan_basis_digest(advice.plan_basis);
  const auto result = marshal::validate_dispatch_advice(
      advice, valid_request(advice), context());

  check(result.dispatchable,
        "stale_contract should remain dispatchable when Lattice supplies a "
        "current concrete trainable plan");
  check(!result.has(
            marshal::marshal_refusal_reason_t::target_status_not_dispatchable),
        "stale_contract should not be reported as a non-dispatchable target "
        "status");
}

void test_missing_plan_basis() {
  auto advice = valid_advice();
  advice.plan_basis.present = false;
  advice.plan_basis.available = false;
  advice.plan_basis_digest.clear();
  const auto result = marshal::validate_dispatch_advice(
      advice, valid_request(advice), context());

  check(!result.dispatchable, "missing plan_basis must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::missing_plan_basis),
        "missing plan_basis refusal should be reported");
}

void test_missing_suggested_wave() {
  auto advice = valid_advice();
  advice.suggested_wave.target.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  const auto result = marshal::validate_dispatch_advice(
      advice, valid_request(advice), context());

  check(!result.dispatchable, "missing suggested_wave must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::missing_suggested_wave),
        "missing suggested_wave refusal should be reported");
}

void test_stale_identity() {
  auto stale_target = valid_advice();
  stale_target.active_identity.target_spec_fingerprint = "old_target_spec";
  auto result = marshal::validate_dispatch_advice(
      stale_target, valid_request(stale_target), context());
  check(!result.dispatchable, "stale target spec must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::stale_target_spec),
        "stale target spec refusal should be reported");

  auto stale_protocol = valid_advice();
  stale_protocol.active_identity.protocol_contract_fingerprint = "old_contract";
  result = marshal::validate_dispatch_advice(
      stale_protocol, valid_request(stale_protocol), context());
  check(!result.dispatchable, "stale active identity must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::stale_active_identity),
        "stale active identity refusal should be reported");
}

void test_forbidden_mode() {
  const auto advice = valid_advice();
  auto request = valid_request(advice);
  request.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  const auto result =
      marshal::validate_dispatch_advice(advice, request, context());

  check(!result.dispatchable, "execute mode must fail in M1 by default");
  check(result.has(marshal::marshal_refusal_reason_t::forbidden_dispatch_mode),
        "forbidden dispatch mode refusal should be reported");

  auto permissive_context = context();
  permissive_context.allow_execute = true;
  const auto still_forbidden =
      marshal::validate_dispatch_advice(advice, request, permissive_context);
  check(!still_forbidden.dispatchable,
        "M1 must reject execute even when future policy context permits it");
  check(still_forbidden.has(
            marshal::marshal_refusal_reason_t::forbidden_dispatch_mode),
        "M1 execution refusal should not be bypassed by context");
}

void test_missing_checkpoint_input() {
  auto advice = valid_advice();
  advice.suggested_wave.plan_inputs.erase("PLAN_INPUT_MDN_CHECKPOINT");
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                                  context());
  check(!result.dispatchable, "missing checkpoint input must fail closed");
  check(
      result.has(marshal::marshal_refusal_reason_t::missing_model_state_input),
      "missing model-state input refusal should be reported");

  advice = valid_advice();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "latest_satisfying:channel_mdn_train_core_no_test_leakage";
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "symbolic checkpoint input must fail closed");
  check(
      result.has(marshal::marshal_refusal_reason_t::missing_model_state_input),
      "unresolved symbolic input should be treated as missing model-state");

  advice = valid_advice();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "relative/checkpoint.pt";
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "relative checkpoint input must fail closed");
  check(
      result.has(marshal::marshal_refusal_reason_t::missing_model_state_input),
      "relative checkpoint input should be treated as missing model-state");

  advice = valid_advice();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "/tmp/outside_runtime/checkpoint.pt";
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable,
        "checkpoint input outside allowed roots must fail closed");
  check(
      result.has(marshal::marshal_refusal_reason_t::missing_model_state_input),
      "outside-root checkpoint input should be rejected as model-state");
}

void test_malformed_range_and_attempt_budget() {
  auto advice = valid_advice();
  advice.suggested_wave.anchor_index_begin = 2050;
  advice.suggested_wave.anchor_index_end = 1800;
  advice.suggested_wave_digest.clear();
  auto result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                                  context());
  check(!result.dispatchable, "malformed wave range must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::malformed_wave_range),
        "malformed range refusal should be reported");

  advice = valid_advice();
  advice.recommendation_attempt_count = 1;
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "exhausted max_waves must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::max_waves_exhausted),
        "max_waves exhausted refusal should be reported");
}

void test_digest_and_root_guards() {
  auto advice = valid_advice();
  auto request = valid_request(advice);
  request.advice_digest = "bad_digest";
  auto result = marshal::validate_dispatch_advice(advice, request, context());
  check(!result.dispatchable, "bad advice digest must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::advice_digest_mismatch),
        "advice digest mismatch should be reported");

  request = valid_request(advice);
  request.runtime_root = "/tmp/other_runtime";
  result = marshal::validate_dispatch_advice(advice, request, context());
  check(!result.dispatchable, "runtime root mismatch must fail closed");
  check(result.has(
            marshal::marshal_refusal_reason_t::advice_runtime_root_mismatch),
        "runtime root mismatch should be reported");

  advice = valid_advice();
  advice.plan_basis_digest = "bad_nested_digest";
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "bad nested digest must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::advice_digest_mismatch),
        "nested digest mismatch should be reported");

  const auto root = make_tmp_dir("symlink_escape");
  const auto allowed_root = root / "allowed";
  const auto outside_root = root / "outside";
  std::filesystem::create_directories(allowed_root);
  std::filesystem::create_directories(outside_root);
  write_text(outside_root / "mdn.pt", "outside");
  std::filesystem::create_directory_symlink(outside_root,
                                            allowed_root / "escape");
  advice = valid_advice();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      (allowed_root / "escape" / "mdn.pt").string();
  advice.suggested_wave.plan_inputs["PLAN_INPUT_REPRESENTATION_CHECKPOINT"] =
      (allowed_root / "rep.pt").string();
  write_text(allowed_root / "rep.pt", "inside");
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto symlink_ctx = context();
  symlink_ctx.allowed_model_state_roots = {allowed_root.string()};
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             symlink_ctx);
  check(!result.dispatchable,
        "checkpoint input symlink escape must fail closed");
  check(
      result.has(marshal::marshal_refusal_reason_t::missing_model_state_input),
      "symlink escape should be reported as model-state input refusal");
  std::filesystem::remove_all(root);
}

void test_lattice_advice_provenance_and_freshness() {
  auto advice = valid_advice();
  advice.source_lattice_tool = "hero.runtime.execute";
  auto result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                                  context());
  check(!result.dispatchable, "wrong advice provenance tool must fail");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "wrong Lattice tool should be reported as unproven advice");

  advice = valid_advice();
  advice.source_lattice_timestamp = "not-a-timestamp";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "invalid advice timestamp must fail");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "invalid timestamp should be reported as unproven advice");

  advice = valid_advice();
  advice.source_lattice_timestamp = "2026-05-22T00:00:00Z";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "stale advice timestamp must fail");
  check(result.has(marshal::marshal_refusal_reason_t::stale_lattice_advice),
        "stale timestamp should be reported");

  advice = valid_advice();
  advice.source_lattice_timestamp = "2026-05-23T02:00:00Z";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "future advice timestamp must fail");
  check(result.has(marshal::marshal_refusal_reason_t::stale_lattice_advice),
        "future timestamp should be reported");
}

void test_signed_lattice_advice_receipt_verification() {
  auto ctx = context();
  ctx.require_signed_lattice_advice = true;
  ctx.trusted_lattice_advice_key_id = "local-test-key";
  ctx.trusted_lattice_advice_verification_key = "local-test-verifier";

  auto unsigned_advice = valid_advice();
  auto result = marshal::validate_dispatch_advice(
      unsigned_advice, valid_request(unsigned_advice), ctx);
  check(!result.dispatchable, "required signed advice must fail when absent");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "missing advice signature should be reported as unproven");

  auto signed_advice = valid_advice();
  signed_advice.advice_receipt_digest =
      marshal::marshal_digest_for_text("test.lattice_advice_receipt", "ok");
  signed_advice.advice_signature_key_id = ctx.trusted_lattice_advice_key_id;
  signed_advice.advice_signature = marshal::sign_lattice_advice_receipt(
      signed_advice, ctx.trusted_lattice_advice_key_id,
      ctx.trusted_lattice_advice_verification_key);
  result = marshal::validate_dispatch_advice(signed_advice,
                                             valid_request(signed_advice), ctx);
  check(result.dispatchable,
        "valid signed Lattice advice receipt should be accepted");

  auto tampered = signed_advice;
  tampered.advice_receipt_digest = marshal::marshal_digest_for_text(
      "test.lattice_advice_receipt", "tampered");
  result =
      marshal::validate_dispatch_advice(tampered, valid_request(tampered), ctx);
  check(!result.dispatchable, "tampered signed advice receipt must fail");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "tampered advice signature should be reported as unproven");

  tampered = signed_advice;
  tampered.advice_signature_key_id = "other-key";
  tampered.advice_signature = marshal::sign_lattice_advice_receipt(
      tampered, "other-key", ctx.trusted_lattice_advice_verification_key);
  result =
      marshal::validate_dispatch_advice(tampered, valid_request(tampered), ctx);
  check(!result.dispatchable, "untrusted advice signature key must fail");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "untrusted key should be reported as unproven advice");
}

void test_status_and_unsupported_surface() {
  auto advice = valid_advice();
  advice.target_status = "satisfied";
  auto result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                                  context());
  check(!result.dispatchable, "satisfied target must not be dispatchable");
  check(result.has(marshal::marshal_refusal_reason_t::target_already_satisfied),
        "target_already_satisfied refusal should be reported");

  advice = valid_advice();
  advice.suggested_wave.target = "unknown.component";
  advice.suggested_wave_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "unsupported target must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::unsupported_wave_target),
        "unsupported wave target refusal should be reported");

  advice = valid_advice();
  advice.suggested_wave.source_range = "edge_key";
  advice.suggested_wave_digest.clear();
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "unsupported source range must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::unsupported_source_range),
        "unsupported source range refusal should be reported");
}

void test_source_key_dispatch_advice_validation() {
  auto advice = valid_advice();
  advice.plan_basis.source_range = "source_key";
  advice.plan_basis.target_anchor_index_begin = std::nullopt;
  advice.plan_basis.target_anchor_index_end = std::nullopt;
  advice.plan_basis.target_source_key_begin = 1002;
  advice.plan_basis.target_source_key_end = 1004;
  advice.suggested_wave.source_range = "source_key";
  advice.suggested_wave.anchor_index_begin = std::nullopt;
  advice.suggested_wave.anchor_index_end = std::nullopt;
  advice.suggested_wave.source_key_begin = 1002;
  advice.suggested_wave.source_key_end = 1004;
  advice.plan_basis_digest = marshal::plan_basis_digest(advice.plan_basis);
  advice.suggested_wave_digest =
      marshal::suggested_wave_digest(advice.suggested_wave);
  advice.plan_input_digest =
      marshal::plan_input_digest(advice.suggested_wave.plan_inputs);

  auto result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                                  context());
  check(result.dispatchable, "source_key advice should validate");

  advice.suggested_wave.source_key_end = 1002;
  advice.suggested_wave_digest =
      marshal::suggested_wave_digest(advice.suggested_wave);
  result = marshal::validate_dispatch_advice(advice, valid_request(advice),
                                             context());
  check(!result.dispatchable, "empty source_key range must fail closed");
  check(result.has(marshal::marshal_refusal_reason_t::malformed_wave_range),
        "malformed source_key range should be reported");
}

void test_m2_dry_run_preview_success() {
  const auto advice = valid_advice();
  auto request = valid_request(advice);
  request.lattice_certificate_refs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "resolver_receipt_mdn";
  const auto decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, request, context(), valid_policy(), valid_active_wave(advice));

  check(decision.accepted, "valid M2 dry-run preview should be accepted");
  check(decision.runtime_handoff_available,
        "valid M2 dry-run preview should be handoff-available");
  check(decision.runtime_request.dry_run,
        "M2 runtime request must force dry_run=true");
  check(!decision.runtime_request.confirm_execute,
        "M2 runtime request must force confirm_execute=false");
  check(decision.runtime_request.wave_target == advice.suggested_wave.target,
        "runtime request target should derive from suggested_wave target");
  check(decision.runtime_request.model_state_inputs ==
            advice.suggested_wave.plan_inputs,
        "runtime request model-state inputs should derive from advice");
  const auto handoff_args =
      marshal::runtime_hero_execute_args_json(decision.runtime_request, 30);
  check(handoff_args.find("\"wave_overlay\":") != std::string::npos,
        "Runtime handoff should include the Lattice range wave_overlay");
  check(handoff_args.find("\"anchor_index_begin\":\"1800\"") !=
            std::string::npos,
        "Runtime handoff should overlay the advised anchor begin");
  check(handoff_args.find("\"anchor_index_end\":\"2050\"") != std::string::npos,
        "Runtime handoff should overlay the advised anchor end");
  check(handoff_args.find("\"runtime_handoff\":") != std::string::npos,
        "Runtime handoff should include the canonical runtime_handoff object");
  check(handoff_args.find("\"handoff_schema_version\":\"1\"") !=
            std::string::npos,
        "runtime_handoff should declare schema version 1");
  check(handoff_args.find("\"handoff_digest\":") != std::string::npos,
        "runtime_handoff should expose its canonical digest");
  check(handoff_args.find(
            "\"target_id\":\"channel_mdn_validation_eval_ready\"") !=
            std::string::npos,
        "runtime_handoff should bind the lattice target id");
  check(handoff_args.find("\"base_config\":") != std::string::npos,
        "runtime_handoff should include base config identity");
  check(handoff_args.find("\"runtime_policy\":") != std::string::npos,
        "runtime_handoff should include runtime policy identity");
  check(
      handoff_args.find("\"unresolved_symbols\":[]") != std::string::npos,
      "resolved runtime handoff should expose an empty unresolved-symbol list");
  check(handoff_args.find("\"lattice_certificate_refs\":{"
                          "\"PLAN_INPUT_MDN_CHECKPOINT\":"
                          "\"resolver_receipt_mdn\"}") != std::string::npos,
        "runtime_handoff should carry Lattice resolver certificate refs");
  check(handoff_args.find("\"target_component_family_id\":") !=
            std::string::npos,
        "Runtime handoff should use canonical target_component_family_id");
  check(handoff_args.find("\"mode\":\"run|debug\"") != std::string::npos,
        "Runtime handoff should use canonical mode");
  check(handoff_args.find("\"source_order\":\"sequential\"") !=
            std::string::npos,
        "Runtime handoff should bind Runtime Hero source_order");
  check(handoff_args.find("\"wave_target\":") == std::string::npos &&
            handoff_args.find("\"wave_mode\":") == std::string::npos,
        "Runtime handoff should not emit removed wave aliases");
  check(!marshal::runtime_dry_run_request_digest(decision.runtime_request)
             .empty(),
        "runtime dry-run request digest should be available");
  check(decision.non_authority_statement.find("audit metadata only") !=
            std::string::npos,
        "M2 decision should carry the non-authority statement");
}

void test_runtime_handoff_accepts_reusable_all_range_overlay() {
  marshal::marshal_runtime_dry_run_request_t request{};
  request.target_id = "cwu_02v_representation_train_core_ready";
  request.config_path = "/cuwacunu/src/config/operator/train_core.config";
  request.wave_target = "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  request.wave_mode = "train|debug";
  request.source_range = "anchor_index";
  request.source_order = "random_per_epoch";
  request.anchor_index_begin = 0;
  request.anchor_index_end = 1600;

  const std::string reusable_wave =
      "{\"structuredContent\":{\"readable\":true,"
      "\"target_component_family_id\":"
      "\"wikimyei.representation.encoding.mtf_jepa_mae_vicreg\","
      "\"mode\":\"train|debug\","
      "\"source_range\":\"all\","
      "\"source_order\":\"random_per_epoch\","
      "\"model_state_inputs\":{}}}";
  check(marshal::detail::runtime_wave_json_matches_request(reusable_wave,
                                                           request),
        "Runtime handoff preflight should accept reusable all-range profiles "
        "when the request carries a concrete overlay range");

  request.anchor_index_end = std::nullopt;
  check(!marshal::detail::runtime_wave_json_matches_request(reusable_wave,
                                                            request),
        "Runtime handoff preflight must reject all-range profiles without a "
        "concrete overlay range");
}

void test_m2_runtime_rejection() {
  const auto advice = valid_advice();
  auto policy = valid_policy();
  policy.runtime_exec_exists = false;
  const auto decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), policy,
      valid_active_wave(advice));

  check(!decision.accepted, "Runtime Hero rejection must block M2 preview");
  check(decision.has(marshal::marshal_refusal_reason_t::runtime_policy_refused),
        "runtime policy refusal should be reported");
}

void test_m2_mode_and_range_mismatch() {
  const auto advice = valid_advice();
  auto wave = valid_active_wave(advice);
  wave.mode = "train|debug";
  auto decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);

  check(!decision.accepted, "Runtime wave mode mismatch must block preview");
  check(decision.has(marshal::marshal_refusal_reason_t::runtime_wave_mismatch),
        "runtime wave mode mismatch should be reported");

  wave = valid_active_wave(advice);
  wave.anchor_index_begin = 0;
  decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);
  check(!decision.accepted, "Runtime wave range mismatch must block preview");
  check(decision.has(marshal::marshal_refusal_reason_t::runtime_wave_mismatch),
        "runtime wave range mismatch should be reported");

  wave = valid_active_wave(advice);
  wave.source_range = "all";
  wave.anchor_index_begin = std::nullopt;
  wave.anchor_index_end = std::nullopt;
  decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);
  check(decision.accepted,
        "Runtime reusable all-range profile should accept advised overlay");
  check(!decision.has(marshal::marshal_refusal_reason_t::runtime_wave_mismatch),
        "overlay-compatible reusable profile should not report range mismatch");
}

void test_m2_checkpoint_mismatch_and_unavailable_handoff() {
  const auto advice = valid_advice();
  auto wave = valid_active_wave(advice);
  wave.model_state_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "/cuwacunu/.runtime/cuwacunu_exec/jobs/other/checkpoint.pt";
  auto decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);

  check(!decision.accepted, "checkpoint input mismatch must block preview");
  check(decision.has(
            marshal::marshal_refusal_reason_t::checkpoint_input_mismatch),
        "checkpoint mismatch should be reported");

  wave = valid_active_wave(advice);
  wave.model_state_inputs.erase("PLAN_INPUT_MDN_CHECKPOINT");
  decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);
  check(!decision.accepted,
        "missing Runtime wave checkpoint input must block preview");
  check(
      decision.has(
          marshal::marshal_refusal_reason_t::runtime_checkpoint_input_missing),
      "missing Runtime checkpoint input should be reported");

  wave = valid_active_wave(advice);
  wave.available = false;
  decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, valid_request(advice), context(), valid_policy(), wave);
  check(!decision.accepted, "unavailable Runtime wave must block preview");
  check(decision.has(
            marshal::marshal_refusal_reason_t::runtime_handoff_unavailable),
        "runtime handoff unavailable should be reported");
}

void test_m2_runtime_hero_live_dry_run_handoff() {
  const auto root = make_tmp_dir("runtime_hero_handoff");
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

  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  auto ctx = context();
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();

  const auto decision = marshal::build_runtime_dry_run_dispatch_preview(
      advice, request, ctx, policy, valid_active_wave(advice));
  check(decision.accepted,
        "accepted M2 decision should be available before live handoff");

  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;
  const auto handoff = marshal::call_runtime_hero_dry_run(decision, options);

  check(handoff.attempted, "Runtime Hero handoff should be attempted");
  check(handoff.tool_call_ok, "Runtime Hero tool call should succeed");
  check(handoff.runtime_dry_run_ok,
        "Runtime Hero dry-run structured result should be ok");
  check(handoff.ok, "Runtime Hero dry-run handoff should succeed");
  check(!handoff.tool_result_error,
        "Runtime Hero dry-run handoff should not return an error result");
  check(handoff.arguments_json.find("\"dry_run\":true") != std::string::npos,
        "Runtime Hero handoff must force dry_run=true");
  check(handoff.arguments_json.find("\"confirm_execute\":false") !=
            std::string::npos,
        "Runtime Hero handoff must force confirm_execute=false");
  check(handoff.tool_result_json.find("\"isError\":false") != std::string::npos,
        "Runtime Hero tool result should be non-error");
  check(handoff.tool_result_json.find("\"exit_code\":0") != std::string::npos,
        "Runtime Hero dry-run process should report exit_code=0");
  check(handoff.non_authority_statement.find("audit metadata only") !=
            std::string::npos,
        "Runtime Hero handoff result should keep Marshal non-authority");

  std::filesystem::remove_all(root);
}

void test_runtime_handoff_structured_result_parsing() {
  const std::string nested_ok =
      R"({"content":[{"type":"text","text":"nested {\"ok\":true}"}],"structuredContent":{"ok":false,"nested":{"ok":true}},"isError":false})";
  check(!marshal::detail::mcp_structured_content_bool_is_true(nested_ok, "ok"),
        "Runtime handoff should read structuredContent.ok, not nested ok");
  check(!marshal::detail::mcp_tool_result_is_error(nested_ok),
        "top-level isError=false should not be treated as error");

  const std::string nested_error =
      R"({"content":[{"type":"text","text":"nested {\"isError\":true}"}],"structuredContent":{"ok":true},"isError":false})";
  check(!marshal::detail::mcp_tool_result_is_error(nested_error),
        "Runtime handoff should read top-level isError only");

  const std::string top_error =
      R"({"content":[],"structuredContent":{"ok":true},"isError":true})";
  check(marshal::detail::mcp_tool_result_is_error(top_error),
        "top-level isError=true should be treated as error");
}

void test_m2_public_dry_run_dispatch_operation() {
  const auto root = make_tmp_dir("public_dry_run_dispatch");
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  auto ctx = context();
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;

  const auto response = marshal::run_marshal_dry_run_dispatch(
      advice, request, ctx, policy, valid_active_wave(advice), options);

  check(response.accepted, "public M2 dispatch operation should be accepted");
  check(response.runtime_dry_run_output_included,
        "public M2 response should include Runtime Hero dry-run output");
  check(response.runtime_handoff.ok,
        "public M2 response should include a successful Runtime handoff");
  check(!response.target_satisfaction_claimed,
        "public M2 response must not claim target satisfaction");
  check(!response.response_digest.empty(),
        "public M2 response should expose a stable digest");
  check(response.runtime_handoff.tool_result_json.find("\"isError\":false") !=
            std::string::npos,
        "public M2 response should include Runtime Hero non-error result");

  auto unproven_advice = advice;
  unproven_advice.source_lattice_tool.clear();
  auto unproven_request = valid_request(unproven_advice);
  unproven_request.config_path = config_path.string();
  unproven_request.runtime_root = runtime_root.string();
  unproven_request.advice_digest =
      marshal::dispatch_advice_digest(unproven_advice);
  const auto unproven_response = marshal::run_marshal_dry_run_dispatch(
      unproven_advice, unproven_request, ctx, policy,
      valid_active_wave(unproven_advice), options);
  check(!unproven_response.accepted,
        "unproven Lattice advice must not dispatch");
  check(unproven_response.decision.has(
            marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "unproven Lattice advice refusal should be reported");
  check(!unproven_response.runtime_handoff.attempted,
        "unproven advice should not call Runtime Hero");

  std::filesystem::remove_all(root);
}

void test_m3_execution_gate_refusals() {
  auto advice = valid_advice();
  auto request = valid_request(advice);
  auto gate_context = context();
  auto policy = valid_policy();
  policy.allow_execute = true;
  const auto wave = valid_active_wave(advice);

  marshal::marshal_execution_gate_input_t input{};
  input.advice = advice;
  input.request = request;
  input.validation_context = gate_context;
  input.policy = policy;
  input.active_wave = wave;
  input.prior_dry_run =
      prior_dry_run_evidence(advice, request, gate_context, policy, wave);

  auto result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject non-execute requests");
  check(result.has(marshal::marshal_refusal_reason_t::forbidden_dispatch_mode),
        "M3 non-execute refusal should be reported");

  request.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  input.request = request;
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject missing confirmation");
  check(result.has(marshal::marshal_refusal_reason_t::missing_confirmation),
        "M3 missing confirmation should be reported");

  request.operator_confirmation_token = "wrong_confirmation";
  input.request = request;
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject wrong confirmation");
  check(result.has(marshal::marshal_refusal_reason_t::confirmation_mismatch),
        "M3 confirmation mismatch should be reported");

  request.operator_confirmation_token =
      marshal::confirmation_token(advice, request);
  input.request = request;
  input.policy.allow_execute = false;
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject Runtime policy refusal");
  check(result.has(marshal::marshal_refusal_reason_t::runtime_policy_refused),
        "M3 Runtime policy refusal should be reported");

  input.policy = policy;
  input.prior_dry_run = {};
  result = marshal::validate_execution_gate(input);
  check(!result.accepted,
        "M3 should require structured prior dry-run evidence");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_missing),
        "M3 missing dry-run evidence should be reported");

  input.prior_dry_run =
      prior_dry_run_evidence(advice, request, gate_context, policy, wave);
  input.prior_dry_run.runtime_preview_request_digest = "wrong_digest";
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject mismatched prior dry-run evidence");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_mismatch),
        "M3 dry-run mismatch should be reported");

  input.prior_dry_run =
      prior_dry_run_evidence(advice, request, gate_context, policy, wave);
  input.prior_dry_run.dry_run_response_digest =
      marshal::marshal_digest_for_text("test.placeholder", "not the dry-run");
  result = marshal::validate_execution_gate(input);
  check(!result.accepted,
        "M3 should reject arbitrary dry-run response digest evidence");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_mismatch),
        "M3 arbitrary dry-run digest mismatch should be reported");

  auto other_advice = advice;
  other_advice.target_id = "other_target";
  other_advice.plan_basis_digest.clear();
  other_advice.suggested_wave_digest.clear();
  other_advice.plan_input_digest.clear();
  input.prior_dry_run =
      prior_dry_run_evidence(other_advice, request, gate_context, policy, wave);
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject prior dry-run from other advice");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_mismatch),
        "M3 other-advice dry-run mismatch should be reported");

  auto checkpoint_advice = advice;
  checkpoint_advice.suggested_wave.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
      "/cuwacunu/.runtime/cuwacunu_exec/jobs/other/checkpoint.pt";
  checkpoint_advice.plan_basis_digest.clear();
  checkpoint_advice.suggested_wave_digest.clear();
  checkpoint_advice.plan_input_digest.clear();
  auto checkpoint_wave = valid_active_wave(checkpoint_advice);
  auto checkpoint_request = request;
  checkpoint_request.advice_digest =
      marshal::dispatch_advice_digest(checkpoint_advice);
  input.prior_dry_run =
      prior_dry_run_evidence(checkpoint_advice, checkpoint_request,
                             gate_context, policy, checkpoint_wave);
  result = marshal::validate_execution_gate(input);
  check(!result.accepted,
        "M3 should reject prior dry-run from different checkpoint input");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_mismatch),
        "M3 checkpoint dry-run mismatch should be reported");

  input.prior_dry_run =
      prior_dry_run_evidence(advice, request, gate_context, policy, wave);
  input.prior_dry_run.accepted = false;
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject refused prior dry-run evidence");
  check(result.has(marshal::marshal_refusal_reason_t::dry_run_receipt_mismatch),
        "M3 refused dry-run evidence should be reported");

  auto unproven_advice = advice;
  unproven_advice.source_lattice_tool.clear();
  auto unproven_request = request;
  unproven_request.advice_digest =
      marshal::dispatch_advice_digest(unproven_advice);
  unproven_request.operator_confirmation_token =
      marshal::confirmation_token(unproven_advice, unproven_request);
  input.advice = unproven_advice;
  input.request = unproven_request;
  input.prior_dry_run =
      prior_dry_run_evidence(unproven_advice, unproven_request, gate_context,
                             policy, valid_active_wave(unproven_advice));
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject unproven Lattice advice");
  check(result.has(marshal::marshal_refusal_reason_t::unproven_lattice_advice),
        "M3 unproven advice should be reported");

  input.advice = advice;
  input.request = request;
  input.prior_dry_run =
      prior_dry_run_evidence(advice, request, gate_context, policy, wave);
  input.active_wave.train_target = true;
  input.policy.allow_train_execute = false;
  result = marshal::validate_execution_gate(input);
  check(!result.accepted, "M3 should reject train execution without policy");
  check(result.has(marshal::marshal_refusal_reason_t::runtime_policy_refused),
        "M3 train policy refusal should be reported");
}

void test_m3_accepted_execution_handoff() {
  const auto root = make_tmp_dir("execution_handoff");
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
                              "allow_execute:bool = true\n"
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  request.operator_confirmation_token =
      marshal::confirmation_token(advice, request);

  auto gate_context = context();
  gate_context.active_config_path = config_path.string();
  gate_context.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  policy.allow_execute = true;

  marshal::marshal_execution_gate_input_t input{};
  input.advice = advice;
  input.request = request;
  input.validation_context = gate_context;
  input.policy = policy;
  input.active_wave = valid_active_wave(advice);
  input.prior_dry_run = prior_dry_run_evidence(
      advice, request, gate_context, policy, valid_active_wave(advice));

  const auto gate = marshal::validate_execution_gate(input);
  check(gate.accepted, "M3 execution gate should accept exact confirmation");

  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;
  const auto handoff = marshal::call_runtime_hero_execution(gate, options);
  check(handoff.attempted, "M3 execution handoff should be attempted");
  check(handoff.decoded_wave_checked,
        "M3 execution handoff should verify Runtime Hero decoded wave");
  check(handoff.decoded_wave_matches_request,
        "M3 execution handoff decoded wave should match request");
  check(handoff.ok, "M3 accepted execution handoff should succeed");
  check(marshal::marshal_digest_is_strong_hex(handoff.arguments_digest),
        "M3 execution handoff should record a strong argument digest");
  check(handoff.arguments_json.find("\"dry_run\":false") != std::string::npos,
        "M3 execution handoff must set dry_run=false");
  check(handoff.arguments_json.find("\"confirm_execute\":true") !=
            std::string::npos,
        "M3 execution handoff must set confirm_execute=true");
  check(handoff.tool_result_json.find("\"isError\":false") != std::string::npos,
        "M3 execution handoff should return non-error Runtime Hero output");

  const auto receipt = marshal::make_execution_dispatch_receipt(
      advice, gate, handoff, "2026-05-23T00:20:00Z");
  check(!receipt.runtime_preview_request_digest.empty(),
        "execution receipt should record preview request digest");
  check(!receipt.runtime_execution_request_digest.empty(),
        "execution receipt should record execution request digest");
  check(!receipt.runtime_handoff_arguments_digest.empty(),
        "execution receipt should record handoff argument digest");
  check(receipt.runtime_preview_request_digest !=
            receipt.runtime_execution_request_digest,
        "execution receipt should distinguish preview and execution request "
        "digests");

  std::filesystem::remove_all(root);
}

void test_m3_execution_handoff_rechecks_runtime_wave() {
  const auto root = make_tmp_dir("execution_handoff_wave_recheck");
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
                              "allow_execute:bool = true\n"
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  request.operator_confirmation_token =
      marshal::confirmation_token(advice, request);

  auto gate_context = context();
  gate_context.active_config_path = config_path.string();
  gate_context.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  policy.allow_execute = true;
  const auto active_wave = valid_active_wave(advice);

  marshal::marshal_execution_gate_input_t input{};
  input.advice = advice;
  input.request = request;
  input.validation_context = gate_context;
  input.policy = policy;
  input.active_wave = active_wave;
  input.prior_dry_run = prior_dry_run_evidence(advice, request, gate_context,
                                               policy, active_wave);
  const auto gate = marshal::validate_execution_gate(input);
  check(gate.accepted, "pre-change execution gate should accept");

  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/other/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;
  const auto handoff = marshal::call_runtime_hero_execution(gate, options);
  check(handoff.attempted,
        "execution handoff should attempt decoded-wave preflight");
  check(handoff.decoded_wave_checked,
        "execution handoff should check decoded wave after gate");
  check(!handoff.decoded_wave_matches_request,
        "changed Runtime wave must fail decoded-wave binding");
  check(!handoff.tool_call_ok,
        "changed Runtime wave must not call Runtime Hero execute");
  check(!handoff.ok, "changed Runtime wave must refuse execution handoff");

  std::filesystem::remove_all(root);
}

void test_m4_dispatch_receipt_replay_audit() {
  const auto root = make_tmp_dir("dispatch_receipt");
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  auto ctx = context();
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;

  const auto response = marshal::run_marshal_dry_run_dispatch(
      advice, request, ctx, policy, valid_active_wave(advice), options);
  check(response.accepted, "dry-run response should be available for receipt");

  const auto receipt = marshal::make_dry_run_dispatch_receipt(
      advice, response, "2026-05-23T00:00:00Z");
  check(!receipt.receipt_id.empty(), "receipt should have an id");
  check(marshal::marshal_digest_is_strong_hex(receipt.receipt_id),
        "receipt id should use a 256-bit hex digest");
  check(receipt.target_id == advice.target_id, "receipt should record target");
  check(receipt.runtime_preview_request_digest ==
            response.runtime_request_digest,
        "dry-run receipt should record preview request digest");
  check(!receipt.runtime_handoff_arguments_digest.empty(),
        "dry-run receipt should record handoff argument digest");
  check(receipt.policy_decision == "accepted",
        "receipt should record accepted policy decision");
  check(receipt.confirmation_status == "not_required",
        "dry-run receipt should record no confirmation requirement");
  check(receipt.non_authority_statement.find("target satisfaction remains") !=
            std::string::npos,
        "receipt should state non-authority");
  check(receipt.post_run_verification.find("Lattice Hero") != std::string::npos,
        "receipt should direct post-run verification to Lattice Hero");

  const auto audit =
      marshal::replay_dispatch_receipt(receipt, advice.active_identity);
  check(audit.accepted, "fresh receipt replay audit should pass");
  check(audit.non_authoritative, "receipt replay should remain non-authority");
  check(audit.explanation.find("target satisfaction requires Lattice Hero") !=
            std::string::npos,
        "receipt replay should explain target proof boundary");

  auto stale_identity = advice.active_identity;
  stale_identity.target_spec_fingerprint = "new_target_spec";
  const auto stale_audit =
      marshal::replay_dispatch_receipt(receipt, stale_identity);
  check(!stale_audit.accepted && stale_audit.stale,
        "stale receipt replay should fail closed");

  auto bad_receipt = receipt;
  bad_receipt.non_authority_statement.clear();
  const auto bad_audit =
      marshal::replay_dispatch_receipt(bad_receipt, advice.active_identity);
  check(!bad_audit.accepted,
        "receipt missing non-authority statement should fail replay audit");

  auto tampered_receipt = receipt;
  tampered_receipt.policy_decision = "refused";
  const auto tampered_audit = marshal::replay_dispatch_receipt(
      tampered_receipt, advice.active_identity);
  check(!tampered_audit.accepted,
        "tampered receipt content should fail replay audit");

  auto missing_digest_receipt = receipt;
  missing_digest_receipt.runtime_request_digest.clear();
  missing_digest_receipt.runtime_preview_request_digest.clear();
  marshal::finalize_receipt_id(&missing_digest_receipt);
  const auto missing_digest_audit = marshal::replay_dispatch_receipt(
      missing_digest_receipt, advice.active_identity);
  check(!missing_digest_audit.accepted,
        "receipt missing runtime request digest should fail replay audit");

  const auto receipt_path = root / ".marshal_dispatch" / "receipt.lls";
  marshal::write_dispatch_receipt(receipt_path, receipt);
  check(std::filesystem::exists(receipt_path),
        "receipt write should create an audit artifact");

  const auto compact_receipt = marshal::compact_dispatch_receipt(receipt);
  check(compact_receipt.retention_class == "compact",
        "compact receipt should record compact retention class");
  check(compact_receipt.runtime_tool_result_json.empty(),
        "compact receipt should remove bulky runtime output");
  check(!compact_receipt.compacted_fields.empty(),
        "compact receipt should report compacted fields");
  check(compact_receipt.receipt_id != receipt.receipt_id,
        "compact receipt should have an id for compacted content");
  check(compact_receipt.advice_digest == receipt.advice_digest,
        "compact receipt should retain advice digest");
  check(compact_receipt.runtime_response_digest ==
            receipt.runtime_response_digest,
        "compact receipt should retain runtime response digest");
  const auto compact_audit =
      marshal::replay_dispatch_receipt(compact_receipt, advice.active_identity);
  check(compact_audit.accepted,
        "compact receipt replay should remain auditable");

  auto tampered_compact = compact_receipt;
  tampered_compact.refusal_reasons.push_back("tampered");
  const auto tampered_compact_audit = marshal::replay_dispatch_receipt(
      tampered_compact, advice.active_identity);
  check(!tampered_compact_audit.accepted,
        "tampered compact receipt should fail replay audit");

  auto tombstone_receipt = compact_receipt;
  tombstone_receipt.retention_class = "tombstone";
  tombstone_receipt.runtime_tool_result_json.clear();
  tombstone_receipt.compacted_fields.push_back("compact_payload");
  marshal::finalize_receipt_id(&tombstone_receipt);
  const auto tombstone_audit = marshal::replay_dispatch_receipt(
      tombstone_receipt, advice.active_identity);
  check(tombstone_audit.accepted,
        "tombstone receipt replay should have explicit semantics");

  std::filesystem::remove_all(root);
}

void test_m6_operator_status_surface() {
  auto advice = valid_advice();
  auto response = marshal::marshal_dry_run_dispatch_response_t{};
  response.accepted = true;
  response.advice_digest = marshal::dispatch_advice_digest(advice);
  response.request_digest =
      marshal::dispatch_request_digest(valid_request(advice));
  response.runtime_request_digest = marshal::runtime_dry_run_request_digest(
      marshal::build_runtime_dry_run_dispatch_preview(
          advice, valid_request(advice), context(), valid_policy(),
          valid_active_wave(advice))
          .runtime_request);
  response.runtime_handoff.tool_result_json = "{\"ok\":true}";
  const auto receipt = marshal::make_dry_run_dispatch_receipt(
      advice, response, "2026-05-23T00:00:00Z");

  auto failed_receipt = receipt;
  failed_receipt.receipt_kind = "execute";
  failed_receipt.policy_decision = "refused";
  failed_receipt.refusal_reasons = {"runtime_policy_refused"};
  const auto status = marshal::make_marshal_status(
      "/tmp/marshal_status_receipts", {receipt, failed_receipt}, valid_policy(),
      true);

  check(status.recent_receipt_count == 2,
        "status should count recent receipts");
  check(status.last_accepted_dry_run_receipt_id == receipt.receipt_id,
        "status should expose last accepted dry-run receipt");
  check(status.last_refusal_reason == "runtime_policy_refused",
        "status should expose latest refusal reason");
  check(status.lattice_advice_surface_available,
        "status should report lattice advice surface availability");
  check(marshal::canonical_marshal_status_text(status).find(
            "non_authority_statement") != std::string::npos,
        "status should carry a non-authority statement");
}

void test_m7_batch_preview_independence() {
  const auto root = make_tmp_dir("batch_preview");
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  auto ctx = context();
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;

  auto bad_advice = advice;
  bad_advice.target_id = "bad_target";
  bad_advice.suggested_wave.anchor_index_begin = 0;
  auto bad_request = valid_request(bad_advice);
  bad_request.config_path = config_path.string();
  bad_request.runtime_root = runtime_root.string();
  bad_request.advice_digest = marshal::dispatch_advice_digest(bad_advice);

  const std::vector<marshal::marshal_batch_preview_item_t> items{
      marshal::marshal_batch_preview_item_t{"ok", advice, request,
                                            valid_active_wave(advice)},
      marshal::marshal_batch_preview_item_t{"bad", bad_advice, bad_request,
                                            valid_active_wave(advice)}};
  const auto preview = marshal::run_batch_preview(items, ctx, policy, options);

  check(marshal::batch_preview_has_independent_results(preview, 2),
        "batch preview should return one result per item");
  check(preview.responses[0].accepted,
        "first batch item should be independently accepted");
  check(!preview.responses[1].accepted,
        "second batch item should be independently refused");
  check(preview.items[0].item_id == "ok",
        "batch preview should preserve item ids");
  check(preview.items[0].advice_digest ==
            marshal::dispatch_advice_digest(advice),
        "batch preview should expose per-item advice digest");
  check(preview.items[1].request_digest ==
            marshal::dispatch_request_digest(bad_request),
        "batch preview should expose per-item request digest");
  check(preview.items[0].validation_context_digest ==
            marshal::dispatch_validation_context_digest(ctx),
        "batch preview should expose validation context digest");
  check(preview.items[0].runtime_policy_digest ==
            marshal::runtime_policy_snapshot_digest(policy),
        "batch preview should expose runtime policy digest");
  check(preview.items[0].runtime_wave_digest ==
            marshal::runtime_wave_snapshot_digest(valid_active_wave(advice)),
        "batch preview should expose per-item runtime wave digest");
  check(preview.dry_run_only, "batch preview must remain dry-run only");

  std::filesystem::remove_all(root);
}

void test_m5_codex_assist_uses_deterministic_primitives() {
  const auto root = make_tmp_dir("codex_assist");
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = cwu_01v_channel_validation_eval_mdn_1800_2050;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/mdn/checkpoint.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/cuwacunu/.runtime/cuwacunu_exec/jobs/vicreg/checkpoint.pt;\n"
             "};\n");

  auto advice = valid_advice();
  advice.config_path = config_path.string();
  advice.runtime_root = runtime_root.string();
  advice.source_lattice_tool = "hero.lattice.target_deficit";
  advice.plan_basis_digest.clear();
  advice.suggested_wave_digest.clear();
  advice.plan_input_digest.clear();
  auto request = valid_request(advice);
  request.config_path = config_path.string();
  request.runtime_root = runtime_root.string();
  request.advice_digest = marshal::dispatch_advice_digest(advice);
  auto ctx = context();
  ctx.active_config_path = config_path.string();
  ctx.active_runtime_root = runtime_root.string();
  auto policy = valid_policy();
  policy.runtime_root = runtime_root.string();
  marshal::marshal_runtime_hero_handoff_options_t options{};
  options.global_config_path = config_path;
  options.policy_path = policy_path;
  options.timeout_seconds = 5;

  const auto response = marshal::codex_assisted_dry_run_explanation(
      "Explain this dispatch to the operator.", {"dry_run", "cancel"}, advice,
      request, ctx, policy, valid_active_wave(advice), options);
  check(response.accepted,
        "Codex-assisted dry-run should use deterministic path");
  check(response.deterministic_primitive == "run_marshal_dry_run_dispatch",
        "Codex-assisted dry-run should name deterministic primitive");
  check(response.prompt_text.find("Explain") != std::string::npos,
        "free-form text should remain prompt/explanation text");
  check(!response.dry_run_response.target_satisfaction_claimed,
        "Codex-assisted dry-run must not claim target satisfaction");

  marshal::marshal_execution_gate_input_t gate_input{};
  gate_input.advice = advice;
  gate_input.request = request;
  gate_input.validation_context = ctx;
  gate_input.policy = policy;
  gate_input.active_wave = valid_active_wave(advice);
  const auto confirmation =
      marshal::codex_assisted_execution_confirmation_prompt(
          "Ask the operator to confirm.", {"confirm", "cancel"}, gate_input);
  check(!confirmation.accepted,
        "Codex-assisted confirmation cannot bypass M3 gate");
  check(confirmation.deterministic_primitive == "validate_execution_gate",
        "Codex-assisted confirmation should use M3 primitive");

  std::filesystem::remove_all(root);
}

void test_m9_marshal_tool_schema_compatibility() {
  const auto issues = marshal::validate_marshal_tool_schemas();
  check(issues.empty(), "Marshal tool schemas should be MCP-compatible");
  check(marshal::direct_cli_and_mcp_expose_same_primitives(),
        "Marshal direct CLI and MCP surfaces should expose same primitives");
  const auto names = marshal::marshal_tool_names();
  check(!names.empty(), "Marshal tool catalog should not be empty");
  check(names.size() == 3, "Marshal tool catalog should expose exactly three "
                           "operator-facing tools");
  check(std::find(names.begin(), names.end(), "hero.marshal.status") !=
            names.end(),
        "Marshal tool catalog should expose status");
  check(std::find(names.begin(), names.end(), "hero.marshal.prepare") !=
            names.end(),
        "Marshal tool catalog should expose prepare");
  check(std::find(names.begin(), names.end(), "hero.marshal.inspect") !=
            names.end(),
        "Marshal tool catalog should expose inspect");
  const auto tools_list_json = marshal::build_marshal_tools_list_result_json();
  check(
      tools_list_json.find("\"name\":\"hero.marshal.inspect\"") !=
              std::string::npos &&
          tools_list_json.find("\"include_preview\":{\"type\":\"boolean\"}") !=
              std::string::npos &&
          tools_list_json.find("\"digest\":{\"type\":\"string\"}") !=
              std::string::npos &&
          tools_list_json.find("\"fact_digest\":{\"type\":\"string\"}") !=
              std::string::npos &&
          tools_list_json.find("\"digest_prefix\":{\"type\":\"string\"}") !=
              std::string::npos &&
          tools_list_json.find("\"fact_index\":{\"type\":\"integer\"}") !=
              std::string::npos &&
          tools_list_json.find("\"index\":{\"type\":\"integer\"}") !=
              std::string::npos,
      "inspect MCP schema should advertise fact-preview "
      "selectors accepted by the handler");
  for (const auto &hidden_name :
       {"hero.marshal.summarize_training_state", "hero.marshal.compare_runs",
        "hero.marshal.lookup_target_advice",
        "hero.marshal.prepare_target_dispatch", "hero.marshal.validate_advice",
        "hero.marshal.dry_run_dispatch", "hero.marshal.execution_gate",
        "hero.marshal.replay_receipt", "hero.marshal.batch_preview",
        "hero.marshal.evaluate_run", "hero.marshal.inspect_run",
        "hero.marshal.reach_lattice_target", "hero.marshal.evaluate",
        "hero.marshal.inspect_evidence_panel"}) {
    check(std::find(names.begin(), names.end(), hidden_name) == names.end(),
          std::string("Marshal tool catalog should not expose ") + hidden_name);
  }
  std::string result;
  std::string error;
  check(!marshal::execute_marshal_tool_json("hero.marshal.evaluate_run", "{}",
                                            &result, &error),
        "old evaluate_run tool name should not remain callable");
  check(error.find("unknown tool") != std::string::npos,
        "old evaluate_run name should fail as an unknown tool");
  for (const auto &retired_name :
       {"hero.marshal.reach_lattice_target", "hero.marshal.evaluate",
        "hero.marshal.inspect_evidence_panel"}) {
    result.clear();
    error.clear();
    check(!marshal::execute_marshal_tool_json(retired_name, "{}", &result,
                                              &error),
          std::string("retired public tool should not remain callable: ") +
              retired_name);
    check(error.find("unknown tool") != std::string::npos,
          std::string("retired public tool should fail as unknown: ") +
              retired_name);
  }
  check(!marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                            R"({"subject":"codex_review"})",
                                            &result, &error),
        "inspect should reject unsupported subjects");
  check(error.find("subject must be run, target, protocol, spawn, component, "
                   "or facts") != std::string::npos,
        "unsupported inspect subject should fail explicitly");
  check(!marshal::execute_marshal_tool_json("hero.marshal.inspect", "{}",
                                            &result, &error),
        "inspect should require an explicit subject");
  check(error.find("missing required field: subject") != std::string::npos,
        "missing inspect subject should fail explicitly");

  const auto malformed =
      cuwacunu::hero::mcp_schema_compat::validate_tool_input_schema(
          "hero.marshal.bad", R"({"type":"object")");
  check(!malformed.empty(), "malformed schema should be rejected");
  const auto missing_type =
      cuwacunu::hero::mcp_schema_compat::validate_tool_input_schema(
          "hero.marshal.bad", R"({"properties":{}})");
  check(!missing_type.empty(), "missing type schema should be rejected");
  const auto forbidden =
      cuwacunu::hero::mcp_schema_compat::validate_tool_input_schema(
          "hero.marshal.bad", R"({"type":"object","oneOf":[]})");
  check(!forbidden.empty(), "top-level oneOf should be rejected");
}

std::string g_fake_lattice_tool_name;
std::string g_fake_lattice_arguments_json;
int g_fake_lattice_resolve_count = 0;
bool g_fake_lattice_resolve_concrete = false;
std::string g_fake_lattice_resolve_fault;
bool g_fake_lattice_prepare_warning = false;
std::string g_fake_lattice_prepare_warning_kind;
int g_fake_lattice_evaluate_target_count = 0;
int g_fake_lattice_scan_facts_count = 0;
int g_fake_lattice_fact_summary_count = 0;
int g_fake_lattice_fact_lineage_count = 0;
int g_fake_lattice_fact_preview_count = 0;
bool g_fake_lattice_policy_fingerprint_mismatch = false;
bool g_fake_lattice_artifact_warning = false;

std::string fake_policy_gate_reservation_json_fragment() {
  const bool mismatch = g_fake_lattice_policy_fingerprint_mismatch;
  std::ostringstream out;
  out << R"("policy_gate_reservation_summary":{"schema":"kikijyeba.lattice.policy_gate_reservation.summary.v1","reservation_count":1,"target_match_count":1,"enabled_policy_gate_count":0,"disabled_policy_gate_count":1,)"
      << R"("policy_fingerprint_verified_count":)" << (mismatch ? 0 : 1)
      << R"(,"policy_fingerprint_mismatch_count":)" << (mismatch ? 1 : 0)
      << R"(,"policy_input_contract_complete_count":1,"missing_policy_input_contract_count":0,"required_policy_inputs":["POLICY_ID","POLICY_KIND","TARGET_ID","METRIC","METRIC_DEFINITION","BASELINE","BASELINE_DEFINITION","THRESHOLD","UNCERTAINTY_POLICY","UNCERTAINTY_MODEL","SUPPORT_MINIMUM","SELECTOR_SPLIT","ANTI_LEAKAGE_POLICY","TIE_POLICY","NEGATIVE_TESTS","CALIBRATION_REQUIREMENTS","HOLDOUT_DECLARATION","THRESHOLD_SELECTION_AUDIT","ENABLED"],"policy_kind_count":1,"policy_kinds":["forecast_quality_acceptance"],"all_reservations_disabled":true,"all_policy_fingerprints_verified":)"
      << (mismatch ? "false" : "true")
      << R"(,"all_policy_input_contracts_complete":true,"target_status_authority_count":0,"target_spec_fingerprint_authority_count":0,"proof_certificate_authority_count":0,"decision_policy_authority_count":0,"runtime_execution_authority_count":0,"allocation_authority_count":0,"market_readiness_authority_count":0,"deployment_authority_count":0,"enabled_policy_gates_fail_closed":true,"status_effect":"disabled_reservation_only_no_target_status_effect"},"policy_gate_reservations":[{"policy_id":"forecast_quality_acceptance_reserved","policy_kind":"forecast_quality_acceptance","target_id":"forecast_eval_artifact_ready","metric":"skill_vs_naive","metric_definition":"reserved_metric_definition","baseline":"naive","baseline_definition":"reserved_baseline_definition","threshold":0.0,"uncertainty_policy":"reserved","uncertainty_model":"reserved_model","support_minimum":0.0,"selector_split":"validation_holdout","anti_leakage_policy":"reserved","tie_policy":"reserved","negative_tests":"reserved_negative_tests","calibration_requirements":"reserved_calibration_requirements","holdout_declaration":"reserved_holdout_declaration","threshold_selection_audit":"reserved_threshold_selection_audit","enabled":false,"status":"reserved_disabled","policy_fingerprint":"pg_fake","policy_fingerprint_verified":)"
      << (mismatch ? "false" : "true") << R"(,"policy_fingerprint_mismatch":)"
      << (mismatch ? "true" : "false")
      << R"(,"policy_input_contract_schema":"kikijyeba.lattice.policy_gate.input_contract.v1","required_policy_inputs":["POLICY_ID","POLICY_KIND","TARGET_ID","METRIC","METRIC_DEFINITION","BASELINE","BASELINE_DEFINITION","THRESHOLD","UNCERTAINTY_POLICY","UNCERTAINTY_MODEL","SUPPORT_MINIMUM","SELECTOR_SPLIT","ANTI_LEAKAGE_POLICY","TIE_POLICY","NEGATIVE_TESTS","CALIBRATION_REQUIREMENTS","HOLDOUT_DECLARATION","THRESHOLD_SELECTION_AUDIT","ENABLED"],"missing_policy_inputs":[],"policy_input_contract_complete":true,"decision_policy_authority":false,"disabled_reason":"policy_gate_layer_reserved","target_status_authority":false,"target_spec_fingerprint_authority":false,"proof_certificate_authority":false,"runtime_execution_authority":false,"allocation_authority":false,"market_readiness_authority":false,"deployment_authority":false,"fields":[]}],)";
  return out.str();
}

void inject_fake_policy_gate_reservations(std::string *payload) {
  const std::string needle = R"("subject_fact_family":"forecast_eval",)";
  const auto pos = payload->find(needle);
  if (pos == std::string::npos) {
    return;
  }
  payload->insert(pos + needle.size(),
                  fake_policy_gate_reservation_json_fragment());
}

std::string fake_artifact_boundary_denial_json_fragment() {
  return R"("target_dependency_authority":false,"runtime_wave_authority":false,"marshal_reachability":false,"checkpoint_source_authority":false,"plan_checkpoint_input_authority":false,)";
}

void inject_fake_artifact_boundary_denials(std::string *payload) {
  const std::string needle = R"("quality_authority":true,)";
  const auto fragment = fake_artifact_boundary_denial_json_fragment();
  std::size_t pos = 0;
  while ((pos = payload->find(needle, pos)) != std::string::npos) {
    const auto insert_at = pos + needle.size();
    payload->insert(insert_at, fragment);
    pos = insert_at + fragment.size();
  }
}

bool real_lattice_hero_callback(const std::string &tool_name,
                                const std::string &arguments_json,
                                std::string *out_tool_result_json,
                                std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (tool_name == "hero.lattice.evaluate_target") {
    ++g_fake_lattice_evaluate_target_count;
  } else if (tool_name == "hero.lattice.scan_facts") {
    ++g_fake_lattice_scan_facts_count;
  } else if (tool_name == "hero.lattice.fact_summary") {
    ++g_fake_lattice_fact_summary_count;
  } else if (tool_name == "hero.lattice.fact_lineage") {
    ++g_fake_lattice_fact_lineage_count;
  } else if (tool_name == "hero.lattice.fact_preview") {
    ++g_fake_lattice_fact_preview_count;
  }
  if (out_error_message) {
    out_error_message->clear();
  }
  try {
    const auto binary = hero_lattice_binary_path();
    const std::string command =
        shell_quote(binary.string()) + " --global-config " +
        shell_quote("/cuwacunu/src/config/.config") + " --tool " +
        shell_quote(tool_name) + " --args-json " + shell_quote(arguments_json);
    if (out_tool_result_json) {
      *out_tool_result_json = read_command_stdout(command);
    } else {
      (void)read_command_stdout(command);
    }
    return true;
  } catch (const std::exception &ex) {
    if (out_error_message) {
      *out_error_message = ex.what();
    }
    return false;
  }
}

bool fake_lattice_target_deficit_callback(const std::string &tool_name,
                                          const std::string &arguments_json,
                                          std::string *out_tool_result_json,
                                          std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.target_deficit") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.target_deficit executed"}],"structuredContent":{"config_path":"/tmp/marshal_lookup/.config","runtime_root":"/tmp/marshal_lookup/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"target_id":"lookup_target","status":"metric_failed","split_policy_fingerprint":"sp","plan_ready":true,"plan_basis":{"available":true,"reason":"suggested wave addresses proof deficits","primary_deficit_key":"coverage:missing","primary_deficit_message":"coverage missing","primary_deficit_priority_class":"coverage","deficit_keys":["coverage:missing"],"deficit_priority_classes":["coverage"]},"suggested_wave":{"target":"wikimyei.inference.expected_value.mdn","mode":"run|debug","source_range":"anchor_index","anchor_index_begin":10,"anchor_index_end":20,"input_mdn_checkpoint":"/tmp/marshal_lookup/runtime/mdn.pt","input_representation_checkpoint":"/tmp/marshal_lookup/runtime/rep.pt","text":"TARGET=wikimyei.inference.expected_value.mdn\n"},"proof_certificate":{"target_id":"lookup_target","target_spec_fingerprint":"ts","split_policy_fingerprint":"sp"}},"isError":false})";
  }
  return true;
}

bool fake_lattice_satisfied_without_certificate_callback(
    const std::string &tool_name, const std::string &arguments_json,
    std::string *out_tool_result_json, std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.target_deficit") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.target_deficit executed"}],"structuredContent":{"target_id":"satisfied_without_certificate","status":"satisfied","plan_ready":false,"suggested_wave":null,"proof_certificate":null,"active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"}},"isError":false})";
  }
  return true;
}

bool fake_lattice_artifact_evidence_callback(const std::string &tool_name,
                                             const std::string &arguments_json,
                                             std::string *out_tool_result_json,
                                             std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name == "hero.lattice.target_deficit") {
    if (out_tool_result_json) {
      *out_tool_result_json =
          R"({"content":[{"type":"text","text":"hero.lattice.target_deficit executed"}],"structuredContent":{"config_path":"/tmp/marshal_artifact/.config","runtime_root":"/tmp/marshal_artifact/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"target_id":"forecast_eval_artifact_ready","status":"metric_failed","target_class":"artifact_readiness","kind":"not_applicable","target_kind_applicable":false,"target_kind_effective":"none","proof_kind":"forecast_eval_artifact_bound","subject_fact_family":"forecast_eval","component":"","split_policy_fingerprint":"sp","plan_ready":false,"plan_basis":null,"suggested_wave":null,"proof_certificate":{"target_id":"forecast_eval_artifact_ready","target_spec_fingerprint":"ts","split_policy_fingerprint":"sp","artifacts":[{"proof_kind":"forecast_eval_artifact_bound","proof_template_bound":true,"proof_template_claim":"forecast evaluation artifact existence, checkpoint lineage, target-transform binding, baseline binding, selection-signal audit binding, and support counts","fact_family":"forecast_eval","fact_digest":"forecast_eval_fact","passed":false,"issues":["missing_baseline_fact"]}]}},"isError":false})";
      if (g_fake_lattice_artifact_warning) {
        const std::string tail = R"(},"isError":false})";
        const auto pos = out_tool_result_json->rfind(tail);
        check(pos != std::string::npos,
              "artifact warning fixture should find structuredContent tail");
        out_tool_result_json->insert(
            pos,
            R"(,"warnings":[{"warning_id":"artifact_warning","severity":"critical","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"artifact_warning_digest","source":"lattice"}])");
      }
      inject_fake_artifact_boundary_denials(out_tool_result_json);
      inject_fake_policy_gate_reservations(out_tool_result_json);
    }
    return true;
  }
  if (tool_name == "hero.lattice.evaluate_target") {
    ++g_fake_lattice_evaluate_target_count;
    if (out_tool_result_json) {
      *out_tool_result_json =
          R"({"content":[{"type":"text","text":"hero.lattice.evaluate_target executed"}],"structuredContent":{"config_path":"/tmp/marshal_artifact/.config","runtime_root":"/tmp/marshal_artifact/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"evaluation":{"target_id":"forecast_eval_artifact_ready","status":"blocked","target_class":"artifact_readiness","proof_kind":"forecast_eval_artifact_bound","subject_fact_family":"forecast_eval","kind":"not_applicable","target_kind_applicable":false,"target_kind_effective":"none","component":"","plan_ready":false,"suggested_wave":null,"deficits":[{"kind":"artifact","dimension":"forecast_eval_authority","key":"artifact:forecast_eval_authority","status":"forbidden","message":"artifact proof contains forbidden authority flags: quality_authority"},{"kind":"artifact","dimension":"forecast_eval_lineage","key":"artifact:forecast_eval_lineage","status":"missing","message":"artifact proof lineage is unbound"},{"kind":"artifact","dimension":"forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","key":"artifact:forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","status":"forbidden","message":"artifact proof issue: forecast_eval_must_remain_artifact_evidence_only"},{"kind":"artifact","dimension":"forecast_eval_issue_baseline_fact_digest_not_found","key":"artifact:forecast_eval_issue_baseline_fact_digest_not_found","status":"missing","message":"artifact proof issue: baseline_fact_digest_not_found","related_fact_integrity_issue_codes":["mdn:baseline_fact_digest_not_found"]}],"plan_basis":{"available":false,"reason":"artifact_readiness target is non-dispatchable; inspect evidence catalog for proof deficits: artifact:forecast_eval_authority","primary_deficit_key":"artifact:forecast_eval_authority","primary_deficit_message":"artifact proof contains forbidden authority flags: quality_authority","primary_deficit_priority_class":"artifact","deficit_keys":["artifact:forecast_eval_authority","artifact:forecast_eval_lineage","artifact:forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","artifact:forecast_eval_issue_baseline_fact_digest_not_found"],"deficit_priority_classes":["artifact"],"suggested_action":"inspect"},"proof_certificate":{"target_id":"forecast_eval_artifact_ready","artifacts":[{"proof_kind":"forecast_eval_artifact_bound","proof_template_bound":true,"proof_template_claim":"forecast evaluation artifact existence, checkpoint lineage, target-transform binding, baseline binding, selection-signal audit binding, and support counts","fact_family":"forecast_eval","fact_digest":"forecast_eval_fact","identity_match":true,"artifact_evidence":true,"deterministic_artifact":true,"visibility_only":true,"authority_clean":false,"quality_authority":true,"lineage_bound":false,"passed":false,"issues":["forecast_eval_must_remain_artifact_evidence_only","baseline_fact_digest_not_found"]}]}}},"isError":false})";
      inject_fake_artifact_boundary_denials(out_tool_result_json);
      inject_fake_policy_gate_reservations(out_tool_result_json);
    }
    return true;
  }
  if (tool_name == "hero.lattice.scan_facts") {
    ++g_fake_lattice_scan_facts_count;
    if (out_tool_result_json) {
      *out_tool_result_json =
          R"({"content":[{"type":"text","text":"hero.lattice.scan_facts executed"}],"structuredContent":{"schema":"kikijyeba.lattice.fact_catalog_scan.v1","runtime_root":"/tmp/marshal_artifact/runtime","read_only":true,"target_proof":false,"dispatchable":false,"runtime_executor":false,"returned_family_count":1,"fact_integrity_summary":{"schema":"kikijyeba.lattice.fact_integrity_summary.v1","inspected_family_count":1,"reported_family_count":1,"relation_declared_count":3,"relation_bound_count":0,"unresolved_relation_count":3,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":3,"relation_integrity_clean":false,"read_only":true,"target_proof":false,"dispatchable":false,"runtime_executor":false,"families_with_unresolved_relation":["forecast_eval"],"families_with_identity_mismatch":[],"families_with_digest_mismatch":[],"integrity_flags":["unresolved_relation"],"issue_codes":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"],"families":[{"schema":"kikijyeba.lattice.fact_integrity_family_summary.v1","family":"forecast_eval","summary_schema":"kikijyeba.lattice.forecast_eval_summary.v1","relation_declared_count":3,"relation_bound_count":0,"unresolved_relation_count":3,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":3,"relation_integrity_clean":false,"issues":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"]}]},"families":[{"catalog_summary":{"family":"forecast_eval","fact_count":1,"artifact_readiness_proofable":true,"artifact_readiness_proof_kind":"forecast_eval_artifact_bound","artifact_readiness_proof_claim":"forecast evaluation artifact existence, checkpoint lineage, target-transform binding, baseline binding, selection-signal audit binding, and support counts","artifact_readiness_target_promotion_allowed":true,"artifact_readiness_target_promotion_blocked":false,"artifact_readiness_target_promotion_blocked_reason":null,"artifact_readiness_warning_summary_only":false},"payload_summary":{"schema":"kikijyeba.lattice.forecast_eval_summary.v1","exposure_fact_count":1,"forecast_eval_fact_count":1,"parent_exposure_fact_count":1,"target_transform_declared_count":1,"target_transform_bound_count":0,"unresolved_target_transform_count":1,"target_transform_identity_mismatch_count":0,"baseline_declared_count":1,"baseline_bound_count":0,"unresolved_baseline_count":1,"baseline_identity_mismatch_count":0,"selection_signal_declared_count":1,"selection_signal_audit_count":0,"unresolved_selection_signal_count":1,"selection_signal_identity_mismatch_count":0,"warning_count":3,"artifact_evidence":true,"visibility_only":true,"readiness_authority":false,"quality_authority":false,"performance_authority":false,"checkpoint_selector":false,"coverage_authority":false,"leakage_authority":false,"contract_identity_authority":false,"issues":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"]},"facts":[]}],"warnings":[]},"isError":false})";
    }
    return true;
  }
  if (tool_name == "hero.lattice.fact_summary") {
    ++g_fake_lattice_fact_summary_count;
    if (out_tool_result_json) {
      if (arguments_json.find("selection_signal") != std::string::npos) {
        *out_tool_result_json =
            R"({"content":[{"type":"text","text":"hero.lattice.fact_summary executed"}],"structuredContent":{"schema":"kikijyeba.lattice.fact_summary.v1","runtime_root":"/tmp/marshal_artifact/runtime","read_only":true,"target_proof":false,"returned_family_count":1,"fact_integrity_summary":{"schema":"kikijyeba.lattice.fact_integrity_summary.v1","inspected_family_count":1,"reported_family_count":1,"relation_declared_count":0,"relation_bound_count":0,"unresolved_relation_count":0,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":1,"relation_integrity_clean":true,"read_only":true,"target_proof":false,"dispatchable":false,"runtime_executor":false,"families_with_unresolved_relation":[],"families_with_identity_mismatch":[],"families_with_digest_mismatch":[],"integrity_flags":[],"issue_codes":["selection_signal:visibility_only"],"families":[{"schema":"kikijyeba.lattice.fact_integrity_family_summary.v1","family":"selection_signal","summary_schema":"kikijyeba.lattice.selection_signal_summary.v1","relation_declared_count":0,"relation_bound_count":0,"unresolved_relation_count":0,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":1,"relation_integrity_clean":true,"issues":["selection_signal:visibility_only"]}]},"families":[{"catalog_summary":{"family":"selection_signal","fact_count":1,"artifact_readiness_proofable":false,"artifact_readiness_proof_kind":null,"artifact_readiness_proof_claim":null,"artifact_readiness_target_promotion_allowed":false,"artifact_readiness_target_promotion_blocked":true,"artifact_readiness_target_promotion_blocked_reason":"leakage_visibility_only","artifact_readiness_warning_summary_only":false},"payload_summary":{"schema":"kikijyeba.lattice.selection_signal_summary.v1","selection_signal_fact_count":1,"warning_count":1,"artifact_evidence":true,"visibility_only":true,"selection_authority":false,"checkpoint_selector":false,"coverage_authority":false,"leakage_authority":false,"readiness_authority":false,"quality_authority":false,"performance_authority":false,"issues":["selection_signal:visibility_only"]}}],"warnings":[]},"isError":false})";
      } else if (arguments_json.find("replay_environment") !=
                 std::string::npos) {
        *out_tool_result_json =
            R"({"content":[{"type":"text","text":"hero.lattice.fact_summary executed"}],"structuredContent":{"schema":"kikijyeba.lattice.fact_summary.v1","runtime_root":"/tmp/marshal_artifact/runtime","read_only":true,"target_proof":false,"returned_family_count":1,"fact_integrity_summary":{"schema":"kikijyeba.lattice.fact_integrity_summary.v1","inspected_family_count":1,"reported_family_count":0,"relation_declared_count":0,"relation_bound_count":0,"unresolved_relation_count":0,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":0,"relation_integrity_clean":true,"read_only":true,"target_proof":false,"dispatchable":false,"runtime_executor":false,"families_with_unresolved_relation":[],"families_with_identity_mismatch":[],"families_with_digest_mismatch":[],"integrity_flags":[],"issue_codes":[],"families":[]},"families":[{"catalog_summary":{"family":"replay_environment","fact_count":1,"artifact_readiness_proofable":false,"artifact_readiness_proof_kind":null,"artifact_readiness_proof_claim":null,"artifact_readiness_target_promotion_allowed":false,"artifact_readiness_target_promotion_blocked":true,"artifact_readiness_target_promotion_blocked_reason":"no_artifact_proof_template","artifact_readiness_warning_summary_only":false},"payload_summary":{"schema":"kikijyeba.lattice.replay_environment_summary.v1","exposure_fact_count":1,"replay_environment_fact_count":1,"parent_exposure_fact_count":1,"batch_index_bound_count":1,"experiment_index_bound_count":1,"experiment_report_bound_count":1,"experiment_id_bound_count":1,"environment_run_id_bound_count":1,"replay_contract_version_bound_count":1,"replay_contract_component_bound_count":1,"replay_contract_policy_surface_bound_count":1,"replay_contract_time_law_bound_count":1,"replay_contract_guard_bound_count":1,"episode_requested_range_bound_count_total":2,"episode_cursor_bound_count_total":2,"episode_anchor_interval_bound_count_total":2,"episode_anchor_keys_bound_count_total":2,"missing_batch_index_count":0,"missing_experiment_index_count":0,"missing_experiment_report_count":0,"missing_episode_requested_range_count":0,"missing_episode_cursor_evidence_count":0,"missing_episode_anchor_interval_count":0,"missing_episode_anchor_keys_count":0,"artifact_evidence_count":1,"warning_count":0,"batch_entry_count_total":1,"experiment_entry_count_total":1,"replay_bundle_count_total":1,"policy_count_total":1,"attempted_count_total":1,"completed_count_total":1,"artifact_evidence":true,"visibility_only":true,"replay_executor":false,"allocation_authority":false,"execution_authority":false,"readiness_authority":false,"quality_authority":false,"performance_authority":false,"market_readiness_authority":false,"deployment_authority":false,"checkpoint_selector":false,"coverage_authority":false,"leakage_authority":false,"contract_identity_authority":false,"issues":[]}}],"warnings":[]},"isError":false})";
      } else {
        *out_tool_result_json =
            R"({"content":[{"type":"text","text":"hero.lattice.fact_summary executed"}],"structuredContent":{"schema":"kikijyeba.lattice.fact_summary.v1","runtime_root":"/tmp/marshal_artifact/runtime","read_only":true,"target_proof":false,"returned_family_count":1,"fact_integrity_summary":{"schema":"kikijyeba.lattice.fact_integrity_summary.v1","inspected_family_count":1,"reported_family_count":1,"relation_declared_count":3,"relation_bound_count":0,"unresolved_relation_count":3,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":3,"relation_integrity_clean":false,"read_only":true,"target_proof":false,"dispatchable":false,"runtime_executor":false,"families_with_unresolved_relation":["forecast_eval"],"families_with_identity_mismatch":[],"families_with_digest_mismatch":[],"integrity_flags":["unresolved_relation"],"issue_codes":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"],"families":[{"schema":"kikijyeba.lattice.fact_integrity_family_summary.v1","family":"forecast_eval","summary_schema":"kikijyeba.lattice.forecast_eval_summary.v1","relation_declared_count":3,"relation_bound_count":0,"unresolved_relation_count":3,"identity_mismatch_count":0,"digest_mismatch_count":0,"warning_count":3,"relation_integrity_clean":false,"issues":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"]}]},"families":[{"catalog_summary":{"family":"forecast_eval","fact_count":1,"artifact_readiness_proofable":true,"artifact_readiness_proof_kind":"forecast_eval_artifact_bound","artifact_readiness_proof_claim":"forecast evaluation artifact existence, checkpoint lineage, target-transform binding, baseline binding, selection-signal audit binding, and support counts","artifact_readiness_target_promotion_allowed":true,"artifact_readiness_target_promotion_blocked":false,"artifact_readiness_target_promotion_blocked_reason":null,"artifact_readiness_warning_summary_only":false},"payload_summary":{"schema":"kikijyeba.lattice.forecast_eval_summary.v1","exposure_fact_count":1,"forecast_eval_fact_count":1,"parent_exposure_fact_count":1,"target_transform_declared_count":1,"target_transform_bound_count":0,"unresolved_target_transform_count":1,"target_transform_identity_mismatch_count":0,"baseline_declared_count":1,"baseline_bound_count":0,"unresolved_baseline_count":1,"baseline_identity_mismatch_count":0,"selection_signal_declared_count":1,"selection_signal_audit_count":0,"unresolved_selection_signal_count":1,"selection_signal_identity_mismatch_count":0,"warning_count":3,"artifact_evidence":true,"visibility_only":true,"readiness_authority":false,"quality_authority":false,"performance_authority":false,"checkpoint_selector":false,"coverage_authority":false,"leakage_authority":false,"contract_identity_authority":false,"issues":["mdn:target_transform_fact_digest_not_found","mdn:baseline_fact_digest_not_found","mdn:selection_signal_fact_digest_not_found"]}}],"warnings":[]},"isError":false})";
      }
    }
    return true;
  }
  if (tool_name == "hero.lattice.fact_lineage") {
    ++g_fake_lattice_fact_lineage_count;
    std::string relation = "forecast_eval";
    if (arguments_json.find("replay_environment") != std::string::npos) {
      relation = "replay_environment";
    } else if (arguments_json.find("selection_signal") != std::string::npos) {
      relation = "selection_signal";
    }
    if (out_tool_result_json) {
      std::ostringstream structured;
      structured
          << "{\"schema\":\"kikijyeba.lattice.fact_lineage.v1\""
          << ",\"runtime_root\":\"/tmp/marshal_artifact/runtime\""
          << ",\"read_only\":true"
          << ",\"target_proof\":false"
          << ",\"dispatchable\":false"
          << ",\"runtime_executor\":false"
          << ",\"fact_families_are_not_target_kinds\":true"
          << ",\"lineage_rows_are_audit_only\":true"
          << ",\"cache_rows_used_for_target_satisfaction\":false"
          << ",\"selected_family_count\":1"
          << ",\"selected_relation_count\":1"
          << ",\"selected_relations\":["
          << marshal::detail::json_quote(relation) << "]"
          << ",\"lineage_row_set_digest\":"
          << marshal::detail::json_quote("fake_" + relation + "_lineage")
          << ",\"relation_counts\":[{\"relation\":"
          << marshal::detail::json_quote(relation) << ",\"count\":1}]"
          << ",\"matching_row_count\":1"
          << ",\"returned_row_count\":1"
          << ",\"truncated\":false"
          << ",\"warnings\":[]"
          << ",\"lineage_rows\":[{\"relation\":"
          << marshal::detail::json_quote(relation)
          << ",\"key\":\"parent_digest|artifact_job|artifact\","
          << "\"digest\":" << marshal::detail::json_quote(relation + "_fact")
          << "}]"
          << ",\"fact_integrity_summary\":{\"schema\":\"kikijyeba.lattice."
             "fact_integrity_summary.v1\","
          << "\"inspected_family_count\":1,"
          << "\"reported_family_count\":1,"
          << "\"relation_declared_count\":0,"
          << "\"relation_bound_count\":0,"
          << "\"unresolved_relation_count\":0,"
          << "\"identity_mismatch_count\":0,"
          << "\"digest_mismatch_count\":0,"
          << "\"warning_count\":0,"
          << "\"relation_integrity_clean\":true,"
          << "\"read_only\":true,"
          << "\"target_proof\":false,"
          << "\"dispatchable\":false,"
          << "\"runtime_executor\":false,"
          << "\"families_with_unresolved_relation\":[],"
          << "\"families_with_identity_mismatch\":[],"
          << "\"families_with_digest_mismatch\":[],"
          << "\"integrity_flags\":[],"
          << "\"issue_codes\":[],"
          << "\"families\":[]}}";
      *out_tool_result_json =
          "{\"content\":[{\"type\":\"text\",\"text\":\"hero.lattice."
          "fact_lineage executed\"}],\"structuredContent\":" +
          structured.str() + ",\"isError\":false}";
    }
    return true;
  }
  if (tool_name == "hero.lattice.fact_preview") {
    ++g_fake_lattice_fact_preview_count;
    std::string relation = "forecast_eval";
    if (arguments_json.find("replay_environment") != std::string::npos) {
      relation = "replay_environment";
    } else if (arguments_json.find("selection_signal") != std::string::npos) {
      relation = "selection_signal";
    }
    if (out_tool_result_json) {
      std::ostringstream structured;
      structured
          << "{\"schema\":\"kikijyeba.lattice.fact_preview.v1\""
          << ",\"runtime_root\":\"/tmp/marshal_artifact/runtime\""
          << ",\"family\":" << marshal::detail::json_quote(relation)
          << ",\"relation\":" << marshal::detail::json_quote(relation)
          << ",\"read_only\":true"
          << ",\"target_proof\":false"
          << ",\"dispatchable\":false"
          << ",\"runtime_executor\":false"
          << ",\"fact_families_are_not_target_kinds\":true"
          << ",\"writes_evidence\":false"
          << ",\"preview_rows_are_audit_only\":true"
          << ",\"facts_used_for_target_satisfaction\":false"
          << ",\"cache_rows_used_for_target_satisfaction\":false"
          << ",\"checkpoint_selected\":false"
          << ",\"model_selector\":false"
          << ",\"digest_filter\":\"\""
          << ",\"digest_prefix_filter\":\"\""
          << ",\"fact_index_filter\":0"
          << ",\"family_fact_count\":1"
          << ",\"matching_fact_count\":1"
          << ",\"returned_fact_count\":1"
          << ",\"truncated\":false"
          << ",\"lineage_row_set_digest\":"
          << marshal::detail::json_quote("fake_" + relation + "_preview")
          << ",\"relation_counts\":[{\"relation\":"
          << marshal::detail::json_quote(relation) << ",\"count\":1}]"
          << ",\"matching_lineage_row_count\":1"
          << ",\"returned_lineage_row_count\":1"
          << ",\"lineage_rows\":[{\"relation\":"
          << marshal::detail::json_quote(relation)
          << ",\"key\":\"parent_digest|artifact_job|artifact\","
          << "\"digest\":" << marshal::detail::json_quote(relation + "_fact")
          << "}]"
          << ",\"facts\":[{\"fact_index\":0,\"digest\":"
          << marshal::detail::json_quote(relation + "_fact")
          << ",\"identity_envelope\":{\"schema\":\"kikijyeba.lattice."
             "fact_identity_envelope.v1\","
          << "\"fact_identity_contract_schema\":\"kikijyeba.lattice."
             "fact_identity_contract.v1\","
          << "\"fact_family\":" << marshal::detail::json_quote(relation)
          << ",\"fact_digest\":"
          << marshal::detail::json_quote(relation + "_fact")
          << ",\"protocol_id\":\"cwu_02v\","
          << "\"source_cursor_token\":\"cursor_1\","
          << "\"parent_exposure_fact_digests\":[\"parent_digest\"],"
          << "\"support_count\":42,"
          << "\"valid_count\":40,"
          << "\"row_index_interval_authority\":true,"
          << "\"source_key_window_audit_only\":true,"
          << "\"target_proof\":false,"
          << "\"dispatchable\":false,"
          << "\"facts_used_for_target_satisfaction\":false}"
          << ",\"fact\":{\"schema\":\"kikijyeba.lattice." << relation
          << ".v1\",\"forecast_artifact_digest\":\"forecast_artifact_1\"}}]"
          << ",\"fact_integrity_summary\":{\"schema\":\"kikijyeba.lattice."
             "fact_integrity_summary.v1\","
          << "\"inspected_family_count\":1,"
          << "\"reported_family_count\":1,"
          << "\"relation_declared_count\":0,"
          << "\"relation_bound_count\":0,"
          << "\"unresolved_relation_count\":0,"
          << "\"identity_mismatch_count\":0,"
          << "\"digest_mismatch_count\":0,"
          << "\"warning_count\":0,"
          << "\"relation_integrity_clean\":true,"
          << "\"read_only\":true,"
          << "\"target_proof\":false,"
          << "\"dispatchable\":false,"
          << "\"runtime_executor\":false,"
          << "\"families_with_unresolved_relation\":[],"
          << "\"families_with_identity_mismatch\":[],"
          << "\"families_with_digest_mismatch\":[],"
          << "\"integrity_flags\":[],"
          << "\"issue_codes\":[],"
          << "\"families\":[]}}";
      *out_tool_result_json =
          "{\"content\":[{\"type\":\"text\",\"text\":\"hero.lattice."
          "fact_preview executed\"}],\"structuredContent\":" +
          structured.str() + ",\"isError\":false}";
    }
    return true;
  }
  if (out_error_message) {
    *out_error_message = "unexpected lattice tool";
  }
  return false;
}

bool fake_lattice_malformed_target_deficit_callback(
    const std::string &tool_name, const std::string &arguments_json,
    std::string *out_tool_result_json, std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.target_deficit") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.target_deficit executed"}],"structuredContent":[],"isError":false})";
  }
  return true;
}

bool fake_lattice_operational_report_callback(const std::string &tool_name,
                                              const std::string &arguments_json,
                                              std::string *out_tool_result_json,
                                              std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.evaluate_targets") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.evaluate_targets executed"}],"structuredContent":{"read_only":true,"evaluations":[{"target_id":"vicreg_train_core_ready","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}},{"target_id":"channel_mdn_train_core_ready","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}},{"target_id":"channel_mdn_train_core_no_validation_leakage","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}},{"target_id":"channel_mdn_train_core_no_test_leakage","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}},{"target_id":"channel_mdn_validation_eval_ready","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}}]},"isError":false})";
  }
  return true;
}

bool fake_lattice_operational_report_blocker_callback(
    const std::string &tool_name, const std::string &arguments_json,
    std::string *out_tool_result_json, std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.evaluate_targets") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.evaluate_targets executed"}],"structuredContent":{"read_only":true,"evaluations":[{"target_id":"vicreg_train_core_ready","status":"satisfied","proof_certificate_check":{"passed":true,"issues":[]}},{"target_id":"channel_mdn_train_core_ready","status":"satisfied","proof_certificate_check":{"passed":false,"issues":["closure_unresolved"]}},{"target_id":"channel_mdn_validation_eval_ready","status":"metric_failed","proof_certificate_check":{"passed":true,"issues":[]},"plan_basis":{"deficit_keys":["coverage:evaluation_metric"],"primary_deficit_key":"coverage:evaluation_metric"},"warnings":[{"warning_id":"validation_eval_missing","severity":"watch","source":"lattice"}]},{"target_id":"forecast_eval_artifact_ready","status":"blocked","target_class":"artifact_readiness","kind":"not_applicable","target_kind_applicable":false,"target_kind_effective":"none","proof_kind":"forecast_eval_artifact_bound","subject_fact_family":"forecast_eval","proof_certificate_check":{"passed":false,"issues":["artifact[0] quality authority present","artifact[0] authority drift present","artifact[0] lineage unbound"]},"deficits":[{"kind":"artifact","dimension":"forecast_eval_authority","key":"artifact:forecast_eval_authority","status":"forbidden"},{"kind":"artifact","dimension":"forecast_eval_lineage","key":"artifact:forecast_eval_lineage","status":"missing"},{"kind":"artifact","dimension":"forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","key":"artifact:forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","status":"forbidden"},{"kind":"artifact","dimension":"forecast_eval_issue_baseline_fact_digest_not_found","key":"artifact:forecast_eval_issue_baseline_fact_digest_not_found","status":"missing","related_fact_integrity_issue_codes":["mdn:baseline_fact_digest_not_found"]}],"plan_basis":{"primary_deficit_key":"artifact:forecast_eval_authority","deficit_keys":["artifact:forecast_eval_authority","artifact:forecast_eval_lineage","artifact:forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only","artifact:forecast_eval_issue_baseline_fact_digest_not_found"]},"proof_certificate":{"artifacts":[{"proof_kind":"forecast_eval_artifact_bound","proof_template_bound":true,"proof_template_claim":"forecast evaluation artifact existence, checkpoint lineage, target-transform binding, baseline binding, selection-signal audit binding, and support counts","fact_family":"forecast_eval","fact_digest":"forecast_eval_fact","identity_match":true,"artifact_evidence":true,"deterministic_artifact":true,"visibility_only":true,"authority_clean":false,"quality_authority":true,"lineage_bound":false,"passed":false,"issues":["forecast_eval_must_remain_artifact_evidence_only","baseline_fact_digest_not_found"]}]}}]},"isError":false})";
    inject_fake_artifact_boundary_denials(out_tool_result_json);
    inject_fake_policy_gate_reservations(out_tool_result_json);
  }
  return true;
}

bool fake_lattice_prepare_callback(const std::string &tool_name,
                                   const std::string &arguments_json,
                                   std::string *out_tool_result_json,
                                   std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name == "hero.lattice.target_deficit") {
    if (out_tool_result_json) {
      std::ostringstream json;
      json
          << R"({"content":[{"type":"text","text":"hero.lattice.target_deficit executed"}],"structuredContent":{"config_path":"/tmp/marshal_prepare/.config","runtime_root":"/tmp/marshal_prepare/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"target_id":"channel_mdn_validation_eval_ready","status":"metric_failed","split_policy_fingerprint":"sp","plan_ready":true,"plan_basis":{"available":true,"reason":"suggested wave addresses proof deficits","primary_deficit_key":"coverage:evaluation_metric","primary_deficit_message":"validation evaluation coverage missing","primary_deficit_priority_class":"coverage","deficit_keys":["coverage:evaluation_metric"],"deficit_priority_classes":["coverage"]},"suggested_wave":{"target":"wikimyei.inference.expected_value.mdn","mode":"run|debug","source_range":"anchor_index","anchor_index_begin":1800,"anchor_index_end":2050,"input_mdn_checkpoint":"latest_satisfying:channel_mdn_train_core_no_test_leakage","input_representation_checkpoint":"latest_satisfying:vicreg_train_core_ready","text":"TARGET=wikimyei.inference.expected_value.mdn\n"},"proof_certificate":{"target_id":"channel_mdn_validation_eval_ready","target_spec_fingerprint":"ts","split_policy_fingerprint":"sp"})";
      if (g_fake_lattice_prepare_warning) {
        if (g_fake_lattice_prepare_warning_kind == "blocking") {
          json
              << R"(,"warnings":[{"warning_id":"test_lattice_blocking_warning","severity":"info","component":"lattice_target","blocking":true,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"}])";
        } else if (g_fake_lattice_prepare_warning_kind == "unknown_severity") {
          json
              << R"(,"warnings":[{"warning_id":"test_lattice_unknown_severity","severity":"mystery","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"}])";
        } else if (g_fake_lattice_prepare_warning_kind == "missing_severity") {
          json
              << R"(,"warnings":[{"warning_id":"test_lattice_missing_severity","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"}])";
        } else if (g_fake_lattice_prepare_warning_kind == "mixed") {
          json
              << R"(,"warnings":[{"warning_id":"test_lattice_watch_warning","severity":"watch","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"},{"warning_id":"test_lattice_critical_warning","severity":"critical","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"}])";
        } else {
          json
              << R"(,"warnings":[{"warning_id":"test_lattice_warning","severity":"watch","component":"lattice_target","blocking":false,"scope":"target_deficit","evidence_digest":"test_evidence_digest","source":"lattice"}])";
        }
      }
      json << R"(},"isError":false})";
      *out_tool_result_json = json.str();
    }
    return true;
  }
  if (tool_name == "hero.lattice.latest_satisfying_checkpoint") {
    ++g_fake_lattice_resolve_count;
    const bool is_representation =
        arguments_json.find("vicreg_train_core_ready") != std::string::npos;
    const std::string source_target =
        is_representation ? "vicreg_train_core_ready"
                          : "channel_mdn_train_core_no_test_leakage";
    const std::string path = is_representation
                                 ? "/tmp/marshal_prepare/runtime/vicreg.pt"
                                 : "/tmp/marshal_prepare/runtime/mdn.pt";
    const bool resolver_ok = g_fake_lattice_resolve_fault != "resolver_not_ok";
    const bool read_only =
        g_fake_lattice_resolve_fault != "authority_violation";
    const bool runtime_executor =
        g_fake_lattice_resolve_fault == "authority_violation";
    const bool writes_evidence =
        g_fake_lattice_resolve_fault == "authority_violation";
    const bool model_selector =
        g_fake_lattice_resolve_fault == "authority_violation";
    const bool closure_checked =
        g_fake_lattice_resolve_fault != "closure_not_checked";
    const bool closure_complete =
        g_fake_lattice_resolve_fault != "closure_incomplete";
    const bool checkpoint_path_present =
        g_fake_lattice_resolve_concrete &&
        g_fake_lattice_resolve_fault != "checkpoint_path_missing";
    const bool proof_check_passed =
        g_fake_lattice_resolve_fault != "proof_check_failed";
    const bool receipt_present =
        g_fake_lattice_resolve_fault != "receipt_missing";
    const std::string resolver_symbolic_hint =
        g_fake_lattice_resolve_fault == "symbolic_hint_mismatch"
            ? "latest_satisfying:wrong_target"
            : "latest_satisfying:" + source_target;
    const std::string resolver_source_target =
        g_fake_lattice_resolve_fault == "source_target_mismatch"
            ? "wrong_target"
            : source_target;
    const std::string resolver_target_status =
        g_fake_lattice_resolve_fault == "target_status_not_satisfied"
            ? "metric_failed"
            : "satisfied";
    std::ostringstream json;
    json << "{\"content\":[{\"type\":\"text\",\"text\":"
         << marshal::detail::json_quote(
                "hero.lattice.latest_satisfying_checkpoint executed")
         << "}],\"structuredContent\":{\"ok\":"
         << (resolver_ok ? "true" : "false")
         << ",\"read_only\":" << (read_only ? "true" : "false")
         << ",\"runtime_executor\":" << (runtime_executor ? "true" : "false")
         << ",\"writes_evidence\":" << (writes_evidence ? "true" : "false")
         << ",\"model_selector\":" << (model_selector ? "true" : "false")
         << ",\"selection_basis\":\"latest_satisfying\""
         << ",\"symbolic_hint\":"
         << marshal::detail::json_quote(resolver_symbolic_hint)
         << ",\"source_target_id\":"
         << marshal::detail::json_quote(resolver_source_target)
         << ",\"target_status\":"
         << marshal::detail::json_quote(resolver_target_status)
         << ",\"resolved\":"
         << (g_fake_lattice_resolve_concrete ? "true" : "false")
         << ",\"resolution_status\":"
         << marshal::detail::json_quote(g_fake_lattice_resolve_concrete
                                            ? "resolved"
                                            : "target_not_satisfied")
         << ",\"concrete_path\":";
    if (g_fake_lattice_resolve_concrete) {
      json << marshal::detail::json_quote(path);
    } else {
      json << "null";
    }
    json << ",\"checkpoint_closure_checked\":"
         << (closure_checked ? "true" : "false")
         << ",\"checkpoint_closure_complete\":"
         << (closure_complete ? "true" : "false")
         << ",\"checkpoint_path_present\":"
         << (checkpoint_path_present ? "true" : "false")
         << ",\"proof_certificate_check_passed\":"
         << (proof_check_passed ? "true" : "false")
         << ",\"resolver_receipt_digest\":";
    if (receipt_present) {
      json << marshal::detail::json_quote(
          marshal::marshal_digest_for_text("test.resolver", path));
    } else {
      json << "\"\"";
    }
    json << "},\"isError\":false}";
    if (out_tool_result_json) {
      *out_tool_result_json = json.str();
    }
    return true;
  }
  if (out_error_message) {
    *out_error_message = "unexpected lattice tool";
  }
  return false;
}

std::string prepare_context_json() {
  return "{\"active_config_path\":\"/tmp/marshal_prepare/.config\","
         "\"active_runtime_root\":\"/tmp/marshal_prepare/runtime\","
         "\"active_identity\":{\"protocol_contract_fingerprint\":\"pc\","
         "\"graph_order_fingerprint\":\"go\","
         "\"target_spec_fingerprint\":\"ts\","
         "\"split_policy_fingerprint\":\"sp\"},"
         "\"supported_wave_targets\":[\"wikimyei.inference.expected_value."
         "mdn\"],"
         "\"supported_source_ranges\":[\"anchor_index\"],"
         "\"allowed_model_state_roots\":[\"/tmp/marshal_prepare/runtime\"],"
         "\"freshness_check_timestamp_utc\":\"2026-05-24T00:00:00Z\"}";
}

std::string prepare_policy_json(bool allow_execute = false,
                                bool allow_train = false) {
  marshal::marshal_runtime_policy_snapshot_t policy{};
  policy.runtime_hero_available = true;
  policy.runtime_exec_exists = true;
  policy.runtime_exec_executable = true;
  policy.default_dry_run = true;
  policy.allow_execute = allow_execute;
  policy.allow_train_execute = allow_train;
  policy.runtime_root = "/tmp/marshal_prepare/runtime";
  return policy_json(policy);
}

std::string prepare_wave_json(bool include_checkpoint_inputs) {
  marshal::marshal_runtime_wave_snapshot_t wave{};
  wave.available = true;
  wave.wave_id = "prepare_validation_eval";
  wave.target_component_family_id = "wikimyei.inference.expected_value.mdn";
  wave.mode = "run|debug";
  wave.source_range = "anchor_index";
  wave.anchor_index_begin = 1800;
  wave.anchor_index_end = 2050;
  wave.job_kind = "channel_inference_mdn";
  wave.train_target = false;
  if (include_checkpoint_inputs) {
    wave.model_state_inputs["PLAN_INPUT_MDN_CHECKPOINT"] =
        "/tmp/marshal_prepare/runtime/mdn.pt";
    wave.model_state_inputs["PLAN_INPUT_REPRESENTATION_CHECKPOINT"] =
        "/tmp/marshal_prepare/runtime/vicreg.pt";
  }
  return wave_json(wave);
}

std::string prepare_args_json(bool include_checkpoint_inputs,
                              bool include_machine_payload) {
  return "{\"target_id\":\"channel_mdn_validation_eval_ready\","
         "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
         "\"drive_mode\":\"one_step\","
         "\"requested_mode\":\"dry_run\","
         "\"materialize_plan_inputs\":true,"
         "\"include_runtime_dry_run\":false,"
         "\"include_machine_payload\":" +
         std::string(include_machine_payload ? "true" : "false") +
         ",\"context\":" + prepare_context_json() +
         ",\"runtime_policy\":" + prepare_policy_json() +
         ",\"runtime_wave\":" + prepare_wave_json(include_checkpoint_inputs) +
         "}";
}

std::filesystem::path
write_prepare_runtime_files(bool allow_execute = false,
                            bool allow_train = false,
                            std::filesystem::path runtime_exec_path = {}) {
  const auto root = std::filesystem::path("/tmp/marshal_prepare");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  const auto policy_path = root / "hero.runtime.dsl";
  const auto wave_path = root / "wave.dsl";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(runtime_root);
  if (runtime_exec_path.empty()) {
    runtime_exec_path = "/bin/true";
  }
  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "[KIKIJYEBA]\n"
                              "kikijyeba_settings_wave_dsl_path = " +
                              wave_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = " +
                              runtime_exec_path.string() +
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
                              "allow_execute:bool = " +
                              std::string(allow_execute ? "true" : "false") +
                              "\n"
                              "allow_train_execute:bool = " +
                              std::string(allow_train ? "true" : "false") +
                              "\n"
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
  write_text(wave_path,
             "KIKIJYEBA_WAVE {\n"
             "  WAVE_ID = prepare_validation_eval;\n"
             "  TARGET = wikimyei.inference.expected_value.mdn;\n"
             "  MODE = run|debug;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = anchor_index;\n"
             "  ANCHOR_INDEX_BEGIN = 1800;\n"
             "  ANCHOR_INDEX_END = 2050;\n"
             "  INPUT_MDN_CHECKPOINT = /tmp/marshal_prepare/runtime/mdn.pt;\n"
             "  INPUT_REPRESENTATION_CHECKPOINT = "
             "/tmp/marshal_prepare/runtime/vicreg.pt;\n"
             "};\n");
  return root;
}

std::filesystem::path write_terminal_fact_runtime_exec() {
  const auto script = std::filesystem::path("/tmp/marshal_prepare") /
                      "runtime_exec_with_terminal_fact.sh";
  const auto job_dir = std::filesystem::path("/tmp/marshal_prepare/runtime") /
                       "terminal_completed_job";
  write_text(script,
             "#!/bin/sh\n"
             "set -eu\n"
             "job_dir='" +
                 job_dir.string() +
                 "'\n"
                 "mkdir -p \"$job_dir\"\n"
                 "cat > \"$job_dir/job.manifest\" <<'EOF'\n"
                 "job_id=terminal_completed_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "target_component_family_id=wikimyei.inference.expected_value."
                 "mdn\n"
                 "runtime_handoff_id=terminal_handoff\n"
                 "runtime_handoff_digest=terminal_handoff_digest\n"
                 "EOF\n"
                 "cat > \"$job_dir/job.state\" <<'EOF'\n"
                 "job_id=terminal_completed_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "status=completed\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "optimizer_steps=0\n"
                 "checkpoint_written=false\n"
                 "EOF\n"
                 "cat > \"$job_dir/runtime.result.fact\" <<'EOF'\n"
                 "fact_type=runtime.result.fact\n"
                 "job_id=terminal_completed_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "status=completed\n"
                 "optimizer_steps=0\n"
                 "checkpoint_written=false\n"
                 "runtime_handoff_id=terminal_handoff\n"
                 "runtime_handoff_digest=terminal_handoff_digest\n"
                 "fact_digest=test_terminal_fact_digest\n"
                 "EOF\n"
                 "echo manifest_path=$job_dir/job.manifest\n");
  std::filesystem::permissions(script,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace);
  return script;
}

std::filesystem::path write_reconciled_terminal_fact_runtime_exec() {
  const auto script = std::filesystem::path("/tmp/marshal_prepare") /
                      "runtime_exec_with_reconciled_terminal_fact.sh";
  const auto job_dir = std::filesystem::path("/tmp/marshal_prepare/runtime") /
                       "terminal_reconciled_job";
  write_text(script,
             "#!/bin/sh\n"
             "set -eu\n"
             "handoff_id=\n"
             "handoff_digest=\n"
             "target_driver_run_id=\n"
             "while [ \"$#\" -gt 0 ]; do\n"
             "  case \"$1\" in\n"
             "    --runtime-handoff-id) shift; handoff_id=\"$1\" ;;\n"
             "    --runtime-handoff-digest) shift; handoff_digest=\"$1\" ;;\n"
             "    --marshal-target-driver-run-id) shift; "
             "target_driver_run_id=\"$1\" ;;\n"
             "  esac\n"
             "  shift || true\n"
             "done\n"
             "job_dir='" +
                 job_dir.string() +
                 "'\n"
                 "mkdir -p \"$job_dir\"\n"
                 "cat > \"$job_dir/job.manifest\" <<EOF\n"
                 "job_id=terminal_reconciled_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "target_component_family_id=wikimyei.inference.expected_value."
                 "mdn\n"
                 "runtime_handoff_id=$handoff_id\n"
                 "runtime_handoff_digest=$handoff_digest\n"
                 "marshal_target_driver_run_id=$target_driver_run_id\n"
                 "EOF\n"
                 "cat > \"$job_dir/job.state\" <<'EOF'\n"
                 "job_id=terminal_reconciled_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "status=completed\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "optimizer_steps=0\n"
                 "checkpoint_written=false\n"
                 "EOF\n"
                 "cat > \"$job_dir/runtime.result.fact\" <<EOF\n"
                 "fact_type=runtime.result.fact\n"
                 "job_id=terminal_reconciled_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "status=completed\n"
                 "optimizer_steps=0\n"
                 "checkpoint_written=false\n"
                 "runtime_handoff_id=$handoff_id\n"
                 "runtime_handoff_digest=$handoff_digest\n"
                 "marshal_target_driver_run_id=$target_driver_run_id\n"
                 "fact_digest=test_reconciled_terminal_fact_digest\n"
                 "EOF\n");
  std::filesystem::permissions(script,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace);
  return script;
}

std::filesystem::path write_state_only_runtime_exec() {
  const auto script = std::filesystem::path("/tmp/marshal_prepare") /
                      "runtime_exec_with_state_only.sh";
  const auto job_dir = std::filesystem::path("/tmp/marshal_prepare/runtime") /
                       "terminal_state_only_job";
  write_text(script,
             "#!/bin/sh\n"
             "set -eu\n"
             "job_dir='" +
                 job_dir.string() +
                 "'\n"
                 "mkdir -p \"$job_dir\"\n"
                 "cat > \"$job_dir/job.manifest\" <<'EOF'\n"
                 "job_id=terminal_state_only_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "target_component_family_id=wikimyei.inference.expected_value."
                 "mdn\n"
                 "runtime_handoff_id=state_only_handoff\n"
                 "runtime_handoff_digest=state_only_handoff_digest\n"
                 "EOF\n"
                 "cat > \"$job_dir/job.state\" <<'EOF'\n"
                 "job_id=terminal_state_only_job\n"
                 "job_kind=channel_inference_mdn\n"
                 "status=completed\n"
                 "wave_id=prepare_validation_eval\n"
                 "wave_action=run\n"
                 "optimizer_steps=0\n"
                 "checkpoint_written=false\n"
                 "EOF\n"
                 "echo manifest_path=$job_dir/job.manifest\n");
  std::filesystem::permissions(script,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace);
  return script;
}

std::filesystem::path
write_checkpoint_io_runtime_exec(bool write_checkpoint_io_fact) {
  const auto script =
      std::filesystem::path("/tmp/marshal_prepare") /
      (write_checkpoint_io_fact ? "runtime_exec_with_checkpoint_io_fact.sh"
                                : "runtime_exec_missing_checkpoint_io_fact.sh");
  const auto job_id = write_checkpoint_io_fact
                          ? std::string("terminal_checkpoint_io_job")
                          : std::string("terminal_missing_checkpoint_io_job");
  const auto job_dir =
      std::filesystem::path("/tmp/marshal_prepare/runtime") / job_id;
  std::string text;
  text += "#!/bin/sh\n";
  text += "set -eu\n";
  text += "handoff_id=\n";
  text += "handoff_digest=\n";
  text += "target_driver_run_id=\n";
  text += "while [ \"$#\" -gt 0 ]; do\n";
  text += "  case \"$1\" in\n";
  text += "    --runtime-handoff-id) shift; handoff_id=\"$1\" ;;\n";
  text += "    --runtime-handoff-digest) shift; handoff_digest=\"$1\" ;;\n";
  text += "    --marshal-target-driver-run-id) shift; "
          "target_driver_run_id=\"$1\" ;;\n";
  text += "  esac\n";
  text += "  shift || true\n";
  text += "done\n";
  text += "job_dir='" + job_dir.string() + "'\n";
  text += "mkdir -p \"$job_dir\"\n";
  text += "cat > \"$job_dir/job.manifest\" <<EOF\n";
  text += "job_id=" + job_id + "\n";
  text += "job_kind=channel_inference_mdn\n";
  text += "wave_id=prepare_validation_eval\n";
  text += "wave_action=run\n";
  text += "target_component_family_id=wikimyei.inference.expected_value.mdn\n";
  text += "runtime_handoff_id=$handoff_id\n";
  text += "runtime_handoff_digest=$handoff_digest\n";
  text += "marshal_target_driver_run_id=$target_driver_run_id\n";
  text += "EOF\n";
  text += "cat > \"$job_dir/job.state\" <<'EOF'\n";
  text += "job_id=" + job_id + "\n";
  text += "job_kind=channel_inference_mdn\n";
  text += "status=completed\n";
  text += "wave_id=prepare_validation_eval\n";
  text += "wave_action=run\n";
  text += "optimizer_steps=3\n";
  text += "checkpoint_written=true\n";
  text += "checkpoint_path=/tmp/marshal_prepare/runtime/mdn_out.pt\n";
  text += "EOF\n";
  text += "cat > \"$job_dir/runtime.result.fact\" <<EOF\n";
  text += "fact_type=runtime.result.fact\n";
  text += "job_id=" + job_id + "\n";
  text += "job_kind=channel_inference_mdn\n";
  text += "status=completed\n";
  text += "optimizer_steps=3\n";
  text += "checkpoint_written=true\n";
  text += "checkpoint_path=/tmp/marshal_prepare/runtime/mdn_out.pt\n";
  text += "runtime_handoff_id=$handoff_id\n";
  text += "runtime_handoff_digest=$handoff_digest\n";
  text += "marshal_target_driver_run_id=$target_driver_run_id\n";
  text += "fact_digest=test_checkpoint_io_terminal_fact_digest\n";
  text += "EOF\n";
  if (write_checkpoint_io_fact) {
    text += "cat > \"$job_dir/runtime.checkpoint_io.fact\" <<EOF\n";
    text += "fact_type=runtime.checkpoint_io.fact\n";
    text += "job_id=" + job_id + "\n";
    text += "job_kind=channel_inference_mdn\n";
    text += "status=completed\n";
    text += "checkpoint_written=true\n";
    text += "checkpoint_path=/tmp/marshal_prepare/runtime/mdn_out.pt\n";
    text += "runtime_handoff_id=$handoff_id\n";
    text += "runtime_handoff_digest=$handoff_digest\n";
    text += "marshal_target_driver_run_id=$target_driver_run_id\n";
    text += "fact_digest=test_checkpoint_io_fact_digest\n";
    text += "EOF\n";
  }
  text += "echo manifest_path=$job_dir/job.manifest\n";
  write_text(script, text);
  std::filesystem::permissions(script,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace);
  return script;
}

void test_target_reach_lattice_target_unresolved_model_state() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = false;
  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(false, false),
                                           &result, &error),
        "prepare should produce an operator packet");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "unresolved model-state inputs should block dispatch preparation");
  check(result.find("\"blocker_bucket\":\"unresolved_model_state\"") !=
            std::string::npos,
        "unresolved latest_satisfying hints should be bucketed clearly");
  check(result.find("\"next_action\":\"resolve_model_state\"") !=
            std::string::npos,
        "operator packet should name model-state resolution as next action");
  check(result.find("\"shape_match\":true") != std::string::npos,
        "Runtime target/mode/range shape should still be visible as matched");
  check(result.find("\"checkpoint_inputs_match\":\"pending\"") !=
            std::string::npos,
        "symbolic checkpoint inputs should report pending checkpoint match");
  check(result.find("\"runtime_dry_run\":{\"requested\":false,"
                    "\"attempted\":false") != std::string::npos,
        "prepare must not dry-run by default");
  check(g_fake_lattice_resolve_count == 2,
        "prepare should ask Lattice to resolve both hints");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_target_reach_lattice_target_resolved_ready_for_dry_run() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, true),
                                           &result, &error),
        "prepare should materialize resolved hints");
  check(
      result.find("\"dispatch_state\":\"ready_for_dry_run\"") !=
          std::string::npos,
      "resolved inputs and matching Runtime wave should be ready for dry-run");
  check(result.find("\"blocker_bucket\":\"none\"") != std::string::npos,
        "resolved prepare packet should not carry a blocker bucket");
  check(result.find("\"next_action\":\"dry_run\"") != std::string::npos,
        "ready prepare packet should point to the explicit dry-run action");
  check(result.find("\"checkpoint_inputs_match\":true") != std::string::npos,
        "concrete Runtime checkpoint inputs should match materialized advice");
  check(result.find("/tmp/marshal_prepare/runtime/mdn.pt") != std::string::npos,
        "operator packet should expose the resolved MDN checkpoint path");
  check(result.find("\"runtime_dry_run\":{\"requested\":false,"
                    "\"attempted\":false") != std::string::npos,
        "resolved preparation should still stop before dry-run by default");
  check(result.find("\"machine_payload\"") != std::string::npos,
        "machine payload should be available when explicitly requested");
  check(g_fake_lattice_resolve_count == 2,
        "resolved prepare should still delegate both resolutions to Lattice");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_reach_lattice_target_wraps_target_dispatch() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, false),
                                           &result, &error),
        "prepare should produce the target-first operator packet");
  check(result.find("\"tool\":\"hero.marshal.prepare\"") != std::string::npos,
        "prepare should identify the high-level Marshal tool");
  check(result.find("\"operator_summary\":{") != std::string::npos &&
            result.find("\"stop_reason\":{") != std::string::npos &&
            result.find("\"wave_panel\":{") != std::string::npos &&
            result.find("\"runtime_panel\":{") != std::string::npos &&
            result.find("\"lattice_panel\":{") != std::string::npos &&
            result.find("\"audit_panel\":{") != std::string::npos,
        "prepare should expose the M19 compact operator panels");
  check(result.find("\"dispatch_state\":\"ready_for_dry_run\"") !=
            std::string::npos,
        "prepare should reuse the target-dispatch readiness logic");
  check(result.find("\"next_command\":{\"tool\":\"hero.marshal.prepare\"") !=
            std::string::npos,
        "prepare should keep follow-up commands on the high-level surface");
  check(result.find("\"checkpoint_inputs_match\":true") != std::string::npos,
        "prepare should still check Runtime checkpoint inputs");
  check(g_fake_lattice_resolve_count == 2,
        "prepare should still delegate checkpoint resolution to "
        "the existing read-only callback");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_target_reach_lattice_target_rejects_untrusted_resolution() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault = "proof_check_failed";
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, false),
                                           &result, &error),
        "prepare should keep responding to untrusted resolver "
        "output");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "untrusted resolver proof should block dispatch preparation");
  check(result.find("\"blocker_bucket\":\"unresolved_model_state\"") !=
            std::string::npos,
        "untrusted resolver proof should remain a model-state blocker");
  check(result.find("\"status\":\"proof_certificate_check_failed\"") !=
            std::string::npos,
        "operator packet should expose the resolver trust failure");
  check(result.find("\"checkpoint_inputs_match\":\"pending\"") !=
            std::string::npos,
        "untrusted resolver output must not materialize checkpoint advice");

  g_fake_lattice_resolve_fault = "authority_violation";
  g_fake_lattice_resolve_count = 0;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, false),
                                           &result, &error),
        "prepare should report resolver authority violations");
  check(result.find("\"status\":\"resolver_authority_violation\"") !=
            std::string::npos,
        "Marshal should reject resolver output that is not read-only");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "authority-violating resolver output must not reach dry-run readiness");

  g_fake_lattice_resolve_fault = "source_target_mismatch";
  g_fake_lattice_resolve_count = 0;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, false),
                                           &result, &error),
        "prepare should reject resolver source-target drift");
  check(result.find("\"status\":\"resolver_source_target_mismatch\"") !=
            std::string::npos,
        "Marshal should require Lattice to resolve the same requested target");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "source-target resolver drift must block dispatch preparation");

  g_fake_lattice_resolve_fault = "target_status_not_satisfied";
  g_fake_lattice_resolve_count = 0;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare",
                                           prepare_args_json(true, false),
                                           &result, &error),
        "prepare should reject non-satisfying source targets");
  check(result.find("\"status\":\"target_not_satisfied\"") != std::string::npos,
        "Marshal should require latest_satisfying sources to be satisfied");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "non-satisfying source-target resolution must not reach dry-run "
        "readiness");

  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m15_budgeted_reach_lattice_target_dry_run_driver() {
  const auto root = write_prepare_runtime_files();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"include_runtime_dry_run\":false,"
      "\"include_machine_payload\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":false,"
      "\"allow_train_execute\":false,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":true,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() + ",\"runtime_policy\":" + prepare_policy_json() +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "budgeted prepare should execute bounded dry-run driver");
  check(result.find("\"drive_mode\":\"budgeted\"") != std::string::npos,
        "budgeted driver should report drive_mode");
  check(result.find("\"driver_terminal_state\":\"blocked_max_waves\"") !=
            std::string::npos,
        "budgeted dry-run with one wave should stop on finite max_waves");
  check(result.find("\"runtime_dry_run\":{\"requested\":true,"
                    "\"attempted\":true,\"accepted\":true") !=
            std::string::npos,
        "budgeted dry-run should perform exactly one Runtime dry-run handoff");
  check(result.find("\"runtime_handoff_attempt_count\":1") != std::string::npos,
        "target-driver ledger should count Runtime handoff attempts");
  check(result.find("\"execution_attempt_count\":0") != std::string::npos,
        "dry-run driver must not execute Runtime state mutation");
  check(result.find("\"driver_policy_digest\":") != std::string::npos,
        "target-driver ledger should record driver_policy digest");
  check(result.find("\"iteration_digest\":") != std::string::npos,
        "target-driver ledger should record per-iteration digest");
  check(result.find("\"non_authority_statement\":") != std::string::npos,
        "target-driver ledger should keep Marshal non-authority boundary");
  check(g_fake_lattice_resolve_count == 2,
        "budgeted dry-run should resolve both checkpoint hints once");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m16_5_budgeted_reach_lattice_target_requires_explicit_policy() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string missing_policy_args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\"}";
  std::string result;
  std::string error;
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", missing_policy_args, &result, &error),
        "budgeted prepare should reject missing driver_policy");
  check(error.find("requires explicit driver_policy") != std::string::npos,
        "missing driver_policy error should be explicit");

  const std::string missing_max_waves_args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"no_progress_window\":1}}";
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", missing_max_waves_args, &result, &error),
        "budgeted prepare should reject missing max_waves");
  check(error.find("requires driver_policy.max_waves") != std::string::npos,
        "missing max_waves error should be explicit");

  const std::string missing_wall_clock_args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"max_waves\":1,\"no_progress_window\":1}}";
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", missing_wall_clock_args, &result, &error),
        "budgeted prepare should reject missing max_wall_clock_seconds");
  check(error.find("requires driver_policy.max_wall_clock_seconds") !=
            std::string::npos,
        "missing max_wall_clock_seconds error should be explicit");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m15_budgeted_reach_lattice_target_no_progress_window() {
  const auto root = write_prepare_runtime_files();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"include_machine_payload\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":3,"
      "\"max_wall_clock_seconds\":5,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() + ",\"runtime_policy\":" + prepare_policy_json() +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "budgeted dry-run driver should enforce no-progress window");
  check(result.find("\"driver_terminal_state\":\"blocked_no_progress\"") !=
            std::string::npos,
        "repeated target deficit should stop on no-progress");
  check(result.find("\"runtime_handoff_attempt_count\":1") != std::string::npos,
        "no-progress should stop before repeating Runtime dry-run handoff");
  check(result.find("\"iteration_count\":2") != std::string::npos,
        "no-progress ledger should record the repeated Lattice check");
  check(g_fake_lattice_resolve_count == 4,
        "two Lattice deficit checks should materialize two checkpoint hints "
        "each before no-progress stop");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m16_5_execute_requires_runtime_terminal_evidence() {
  const auto root = write_prepare_runtime_files(/*allow_execute=*/true);
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"include_machine_payload\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "execute target driver should return an operator packet");
  check(result.find("\"driver_terminal_state\":"
                    "\"blocked_runtime_completion_missing\"") !=
            std::string::npos,
        "execute with require_runtime_job_completion should block when Runtime "
        "emits no durable terminal evidence");
  check(result.find("\"runtime_job_completion_observed\":false") !=
            std::string::npos,
        "handoff.ok alone must not count as Runtime job completion evidence");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m16_5_execute_binds_runtime_terminal_evidence() {
  const auto script_path = std::filesystem::path("/tmp/marshal_prepare") /
                           "runtime_exec_with_reconciled_terminal_fact.sh";
  const auto root =
      write_prepare_runtime_files(/*allow_execute=*/true,
                                  /*allow_train=*/false, script_path);
  (void)write_reconciled_terminal_fact_runtime_exec();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "execute target driver should bind terminal Runtime evidence");
  check(result.find("\"runtime_job_completion_observed\":true") !=
            std::string::npos,
        "durable terminal job evidence should satisfy completion observation");
  check(result.find("\"runtime_job_id\":\"terminal_reconciled_job\"") !=
            std::string::npos,
        "target-driver iteration should bind Runtime job id");
  check(result.find("\"runtime_terminal_fact_digest\":\"") != std::string::npos,
        "target-driver iteration should bind terminal fact digest");
  check(result.find("\"runtime_job_manifest_digest\":\"") != std::string::npos,
        "target-driver iteration should bind job manifest digest");
  check(result.find("\"runtime_handoff_digest\":\"") != std::string::npos,
        "target-driver iteration should bind Runtime handoff digest from "
        "the Marshal-generated handoff");
  check(result.find("\"last_runtime_job_manifest_digest\":\"") !=
            std::string::npos,
        "target-driver ledger should retain the last job manifest digest for "
        "resume");
  check(result.find("\"last_runtime_handoff_digest\":\"") != std::string::npos,
        "target-driver ledger should retain the last handoff digest for "
        "resume");
  check(result.find("\"last_safe_point\":\"runtime_terminal_evidence\"") !=
            std::string::npos,
        "target-driver ledger should report the last verified Runtime evidence "
        "point");
  check(
      result.find("\"next_safe_recheck\":\"ask_lattice_to_recheck_target\"") !=
          std::string::npos,
      "target-driver ledger should name the next safe proof recheck");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m16_5_execute_rejects_terminal_evidence_handoff_mismatch() {
  const auto script_path = std::filesystem::path("/tmp/marshal_prepare") /
                           "runtime_exec_with_terminal_fact.sh";
  const auto root =
      write_prepare_runtime_files(/*allow_execute=*/true,
                                  /*allow_train=*/false, script_path);
  (void)write_terminal_fact_runtime_exec();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "handoff-mismatched Runtime evidence should produce a bounded packet");
  check(result.find("\"driver_terminal_state\":"
                    "\"blocked_runtime_completion_missing\"") !=
            std::string::npos,
        "terminal evidence with the wrong handoff identity must not satisfy "
        "completion observation");
  check(result.find("\"runtime_job_completion_observed\":false") !=
            std::string::npos,
        "handoff-mismatched terminal evidence must be treated as unobserved");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m16_5_execute_reconciles_terminal_evidence_by_handoff() {
  const auto root = std::filesystem::path("/tmp/marshal_reconcile");
  const auto runtime_root = root / "runtime";
  const auto job_dir = runtime_root / "terminal_reconciled_job";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(job_dir);
  write_text(job_dir / "job.manifest",
             "job_id=terminal_reconciled_job\n"
             "job_kind=channel_inference_mdn\n"
             "runtime_handoff_id=runtime_handoff_reconcile_test\n"
             "runtime_handoff_digest=reconcile_digest\n");
  write_text(job_dir / "job.state", "job_id=terminal_reconciled_job\n"
                                    "status=completed\n"
                                    "checkpoint_written=false\n");
  write_text(job_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "job_id=terminal_reconciled_job\n"
             "status=completed\n"
             "checkpoint_written=false\n"
             "runtime_handoff_id=runtime_handoff_reconcile_test\n"
             "runtime_handoff_digest=reconcile_digest\n");

  marshal::marshal_runtime_hero_handoff_result_t handoff{};
  handoff.ok = true;
  handoff.arguments_json = "{\"runtime_handoff\":{\"handoff_id\":"
                           "\"runtime_handoff_reconcile_test\","
                           "\"handoff_digest\":\"reconcile_digest\"}}";
  handoff.tool_result_json = "{\"structuredContent\":{\"ok\":true}}";

  const auto evidence =
      marshal::tool_detail::reconcile_runtime_terminal_evidence_after_handoff(
          handoff, runtime_root);
  check(evidence.observed,
        "durable terminal evidence matched by handoff identity should satisfy "
        "completion observation");
  check(
      evidence.job_id == "terminal_reconciled_job",
      "reconciled terminal evidence should bind the recovered Runtime job id");
  check(evidence.runtime_handoff_id == "runtime_handoff_reconcile_test" &&
            evidence.runtime_handoff_digest == "reconcile_digest",
        "reconciled terminal evidence should preserve the matched handoff "
        "identity");
  std::filesystem::remove_all(root);
}

void test_m16_5_reconcile_rejects_wrong_handoff_identity() {
  const auto root = std::filesystem::path("/tmp/marshal_reconcile_wrong");
  const auto runtime_root = root / "runtime";
  const auto job_dir = runtime_root / "wrong_handoff_job";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(job_dir);
  write_text(job_dir / "job.manifest",
             "job_id=wrong_handoff_job\n"
             "job_kind=channel_inference_mdn\n"
             "runtime_handoff_id=runtime_handoff_wrong\n"
             "runtime_handoff_digest=wrong_digest\n");
  write_text(job_dir / "job.state", "job_id=wrong_handoff_job\n"
                                    "status=completed\n"
                                    "checkpoint_written=false\n");
  write_text(job_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "job_id=wrong_handoff_job\n"
             "status=completed\n"
             "checkpoint_written=false\n"
             "runtime_handoff_id=runtime_handoff_wrong\n"
             "runtime_handoff_digest=wrong_digest\n");

  marshal::marshal_runtime_hero_handoff_result_t handoff{};
  handoff.ok = true;
  handoff.arguments_json = "{\"runtime_handoff\":{\"handoff_id\":"
                           "\"runtime_handoff_expected\","
                           "\"handoff_digest\":\"expected_digest\"}}";
  handoff.tool_result_json =
      "{\"structuredContent\":{\"ok\":true,\"artifacts\":{\"job_dir\":\"" +
      job_dir.string() + "\",\"manifest\":{\"path\":\"" +
      (job_dir / "job.manifest").string() + "\"}}}}";

  const auto evidence =
      marshal::tool_detail::reconcile_runtime_terminal_evidence_after_handoff(
          handoff, runtime_root);
  check(!evidence.observed,
        "complete terminal evidence with the wrong handoff id/digest must not "
        "count as observed");
  check(evidence.runtime_handoff_id == "runtime_handoff_wrong" &&
            evidence.runtime_handoff_digest == "wrong_digest",
        "mismatched evidence should remain inspectable for diagnostics");
  std::filesystem::remove_all(root);
}

void test_m16_5_reconcile_requires_expected_handoff_id_and_digest() {
  const auto root = std::filesystem::path("/tmp/marshal_reconcile_partial");
  const auto runtime_root = root / "runtime";
  const auto job_dir = runtime_root / "partial_handoff_job";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(job_dir);
  write_text(job_dir / "job.manifest",
             "job_id=partial_handoff_job\n"
             "job_kind=channel_inference_mdn\n"
             "runtime_handoff_id=runtime_handoff_partial\n"
             "runtime_handoff_digest=partial_digest\n");
  write_text(job_dir / "job.state", "job_id=partial_handoff_job\n"
                                    "status=completed\n"
                                    "checkpoint_written=false\n");
  write_text(job_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "job_id=partial_handoff_job\n"
             "status=completed\n"
             "checkpoint_written=false\n"
             "runtime_handoff_id=runtime_handoff_partial\n"
             "runtime_handoff_digest=partial_digest\n");

  marshal::marshal_runtime_hero_handoff_result_t missing_digest{};
  missing_digest.ok = true;
  missing_digest.arguments_json =
      "{\"runtime_handoff\":{\"handoff_id\":\"runtime_handoff_partial\"}}";
  missing_digest.tool_result_json = "{\"structuredContent\":{\"ok\":true}}";
  const auto evidence =
      marshal::tool_detail::reconcile_runtime_terminal_evidence_after_handoff(
          missing_digest, runtime_root);
  check(!evidence.observed,
        "fallback reconciliation must require both expected handoff id and "
        "digest");
  std::filesystem::remove_all(root);
}

void test_m20_execute_requires_runtime_result_fact_not_state_only() {
  const auto script_path = std::filesystem::path("/tmp/marshal_prepare") /
                           "runtime_exec_with_state_only.sh";
  const auto root =
      write_prepare_runtime_files(/*allow_execute=*/true,
                                  /*allow_train=*/false, script_path);
  (void)write_state_only_runtime_exec();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "execute target driver should return an operator packet for state-only "
        "Runtime output");
  check(result.find("\"driver_terminal_state\":"
                    "\"blocked_runtime_completion_missing\"") !=
            std::string::npos,
        "job.state alone must not satisfy require_runtime_job_completion");
  check(result.find("runtime_result_fact_missing") != std::string::npos,
        "state-only Runtime output should name missing runtime.result.fact");
  check(result.find("\"runtime_job_completion_observed\":false") !=
            std::string::npos,
        "completion observation must stay false without runtime.result.fact");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m20_execute_requires_checkpoint_io_fact_when_checkpoint_io_occurs() {
  const auto script_path = std::filesystem::path("/tmp/marshal_prepare") /
                           "runtime_exec_missing_checkpoint_io_fact.sh";
  const auto root =
      write_prepare_runtime_files(/*allow_execute=*/true,
                                  /*allow_train=*/false, script_path);
  (void)write_checkpoint_io_runtime_exec(/*write_checkpoint_io_fact=*/false);
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "execute target driver should report missing checkpoint I/O fact");
  check(result.find("\"driver_terminal_state\":"
                    "\"blocked_runtime_checkpoint_io_missing\"") !=
            std::string::npos,
        "checkpoint writes require runtime.checkpoint_io.fact");
  check(result.find("runtime_checkpoint_io_fact_missing") != std::string::npos,
        "missing checkpoint I/O fact should be audit-visible");
  check(result.find("\"runtime_checkpoint_io_fact_digest\":\"\"") !=
            std::string::npos,
        "missing checkpoint I/O fact should leave digest empty");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m20_execute_binds_checkpoint_io_fact() {
  const auto script_path = std::filesystem::path("/tmp/marshal_prepare") /
                           "runtime_exec_with_checkpoint_io_fact.sh";
  const auto root =
      write_prepare_runtime_files(/*allow_execute=*/true,
                                  /*allow_train=*/false, script_path);
  (void)write_checkpoint_io_runtime_exec(/*write_checkpoint_io_fact=*/true);
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"allow_execute\":true,"
      "\"require_runtime_job_completion\":true,"
      "\"require_post_wave_lattice_satisfied_check\":false,"
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() +
      ",\"runtime_policy\":" + prepare_policy_json(/*allow_execute=*/true) +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "execute target driver should accept checkpoint I/O evidence");
  check(result.find("\"runtime_job_completion_observed\":true") !=
            std::string::npos,
        "runtime.result.fact plus checkpoint I/O fact should satisfy "
        "completion observation");
  check(result.find("\"runtime_checkpoint_io_fact_digest\":\"") !=
            std::string::npos,
        "target-driver iteration should bind checkpoint I/O fact digest");
  check(result.find("\"runtime_checkpoint_io_fact_digest\":\"\"") ==
            std::string::npos,
        "checkpoint I/O fact digest should not be empty when fact exists");
  check(result.find("\"last_runtime_checkpoint_io_fact_digest\":\"") !=
            std::string::npos,
        "target-driver ledger should retain checkpoint I/O fact digest");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m15_budgeted_reach_lattice_target_stops_on_lattice_warning() {
  const auto root = write_prepare_runtime_files();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = true;
  g_fake_lattice_prepare_warning_kind.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":3,"
      "\"max_wall_clock_seconds\":5,"
      "\"stop_on_lattice_warning\":true,"
      "\"stop_on_warning_severity\":\"watch\","
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() + ",\"runtime_policy\":" + prepare_policy_json() +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "budgeted driver should enforce Lattice warning stop policy");
  check(result.find("\"driver_terminal_state\":\"blocked_lattice_warning\"") !=
            std::string::npos,
        "Lattice warning should stop the target driver");
  check(result.find("lattice_warning_severity") != std::string::npos,
        "typed warning severity stop should be audit-visible in refusal "
        "reasons");
  check(result.find("lattice_warning_id:test_lattice_warning") !=
            std::string::npos,
        "typed warning stop should bind the warning id");
  check(result.find("\"runtime_handoff_attempt_count\":0") != std::string::npos,
        "Lattice warning stop should happen before Runtime handoff");

  g_fake_lattice_prepare_warning = false;
  g_fake_lattice_prepare_warning_kind.clear();
  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m21_typed_warning_policy_decision() {
  const std::string mixed =
      R"({"warnings":[{"warning_id":"low","severity":"watch","component":"target","blocking":false,"scope":"deficit","evidence_digest":"e1","source":"lattice"},{"warning_id":"high","severity":"critical","component":"target","blocking":false,"scope":"deficit","evidence_digest":"e2","source":"lattice"}]})";
  const auto mixed_decision =
      marshal::tool_detail::warning_stop_decision(mixed, "lattice", "watch");
  check(mixed_decision.stop,
        "mixed typed warnings should trigger when highest severity is above "
        "threshold");
  check(mixed_decision.warning.warning_id == "high",
        "mixed typed warnings should choose the highest applicable severity");
  check(mixed_decision.reason_code == "lattice_warning_severity",
        "severity-based stop should carry a typed reason code");

  const std::string below_threshold =
      R"({"warnings":[{"warning_id":"watch_only","severity":"watch","component":"target","blocking":false,"scope":"deficit","evidence_digest":"e1","source":"lattice"}]})";
  const auto below_decision = marshal::tool_detail::warning_stop_decision(
      below_threshold, "lattice", "critical");
  check(!below_decision.stop,
        "non-blocking warning below threshold must remain visible but not stop "
        "the driver");

  const std::string missing_severity =
      R"({"warnings":[{"warning_id":"missing","component":"target","blocking":false,"scope":"deficit","evidence_digest":"e1","source":"lattice"}]})";
  const auto missing_decision = marshal::tool_detail::warning_stop_decision(
      missing_severity, "lattice", "watch");
  check(missing_decision.stop && missing_decision.malformed,
        "missing warning severity should be handled deterministically");
  check(missing_decision.reason_code == "lattice_warning_missing_severity",
        "missing severity should have an explicit reason code");

  const std::string unknown_severity =
      R"({"warnings":[{"warning_id":"unknown","severity":"strange","component":"target","blocking":false,"scope":"deficit","evidence_digest":"e1","source":"lattice"}]})";
  const auto unknown_decision = marshal::tool_detail::warning_stop_decision(
      unknown_severity, "lattice", "watch");
  check(unknown_decision.stop && unknown_decision.malformed,
        "unknown warning severity should fail closed under stop policy");
  check(unknown_decision.reason_code == "lattice_warning_unknown_severity",
        "unknown severity should have an explicit reason code");

  const std::string blocking =
      R"({"warnings":[{"warning_id":"blocking","severity":"info","component":"target","blocking":true,"scope":"deficit","evidence_digest":"e1","source":"lattice"}]})";
  const auto blocking_decision = marshal::tool_detail::warning_stop_decision(
      blocking, "lattice", "critical");
  check(blocking_decision.stop,
        "blocking warnings should stop even below the severity threshold");
  check(blocking_decision.reason_code == "lattice_warning_blocking",
        "blocking warning stop should carry explicit owner/reason");
}

void test_m21_warning_only_visibility_does_not_stop_below_threshold() {
  const auto root = write_prepare_runtime_files();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = true;
  g_fake_lattice_prepare_warning_kind.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":1,"
      "\"max_wall_clock_seconds\":5,"
      "\"stop_on_lattice_warning\":true,"
      "\"stop_on_warning_severity\":\"critical\","
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() + ",\"runtime_policy\":" + prepare_policy_json() +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "non-blocking warning below threshold should still produce a bounded "
        "driver packet");
  check(result.find("\"driver_terminal_state\":\"blocked_lattice_warning\"") ==
            std::string::npos,
        "warning-only visibility below threshold must not become a driver "
        "failure");
  check(result.find("\"runtime_handoff_attempt_count\":1") != std::string::npos,
        "driver should continue to Runtime dry-run when warning is below "
        "threshold");

  g_fake_lattice_prepare_warning = false;
  g_fake_lattice_prepare_warning_kind.clear();
  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_m21_blocking_lattice_warning_stops_with_owner_reason() {
  const auto root = write_prepare_runtime_files();
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = true;
  g_fake_lattice_prepare_warning_kind = "blocking";
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"materialize_plan_inputs\":true,"
      "\"timeout_seconds\":5,"
      "\"driver_policy\":{\"max_waves\":3,"
      "\"max_wall_clock_seconds\":5,"
      "\"stop_on_lattice_warning\":true,"
      "\"stop_on_warning_severity\":\"critical\","
      "\"no_progress_window\":1},"
      "\"context\":" +
      prepare_context_json() + ",\"runtime_policy\":" + prepare_policy_json() +
      ",\"runtime_wave\":" + prepare_wave_json(true) + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "blocking typed Lattice warning should produce a bounded driver "
        "packet");
  check(result.find("\"driver_terminal_state\":\"blocked_lattice_warning\"") !=
            std::string::npos,
        "blocking typed warning should stop the driver");
  check(result.find("lattice_warning_blocking") != std::string::npos,
        "blocking typed warning should carry explicit owner/reason");
  check(result.find("lattice_warning_id:test_lattice_blocking_warning") !=
            std::string::npos,
        "blocking typed warning should bind warning id");
  check(result.find("\"runtime_handoff_attempt_count\":0") != std::string::npos,
        "blocking Lattice warning should stop before Runtime handoff");

  g_fake_lattice_prepare_warning = false;
  g_fake_lattice_prepare_warning_kind.clear();
  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

marshal::marshal_target_driver_ledger_t make_m22_target_driver_ledger() {
  marshal::marshal_target_driver_policy_t policy{};
  policy.max_waves = 2;
  policy.max_wall_clock_seconds = 5;
  policy.allow_execute = true;
  policy.require_runtime_job_completion = true;

  marshal::marshal_target_driver_iteration_t iteration{};
  iteration.iteration_index = 0;
  iteration.active_identity.protocol_contract_fingerprint = "pc";
  iteration.active_identity.graph_order_fingerprint = "go";
  iteration.active_identity.target_spec_fingerprint = "ts";
  iteration.active_identity.split_policy_fingerprint = "sp";
  iteration.target_status = "metric_failed";
  iteration.dispatch_state = "ready_for_dry_run";
  iteration.blocker_bucket = "none";
  iteration.next_action = "dry_run";
  iteration.terminal_state = "continued";
  iteration.terminal_reason = "Runtime execution completed";
  iteration.target_deficit_digest =
      marshal::marshal_digest_for_text("test.deficit", "deficit");
  iteration.suggested_wave_digest =
      marshal::marshal_digest_for_text("test.wave", "wave");
  iteration.plan_input_digest =
      marshal::marshal_digest_for_text("test.plan_inputs", "inputs");
  iteration.advice_digest =
      marshal::marshal_digest_for_text("test.advice", "advice");
  iteration.request_digest =
      marshal::marshal_digest_for_text("test.request", "request");
  iteration.runtime_policy_digest = "runtime_policy_digest_v1";
  iteration.runtime_wave_digest =
      marshal::marshal_digest_for_text("test.runtime_wave", "wave");
  iteration.runtime_preview_request_digest =
      marshal::marshal_digest_for_text("test.preview", "preview");
  iteration.runtime_execution_request_digest =
      marshal::marshal_digest_for_text("test.execute", "execute");
  iteration.runtime_handoff_arguments_digest =
      marshal::marshal_digest_for_text("test.handoff_args", "args");
  iteration.runtime_handoff_id = "handoff_id";
  iteration.runtime_handoff_digest = "handoff_digest";
  iteration.runtime_response_digest =
      marshal::marshal_digest_for_text("test.runtime_response", "response");
  iteration.dry_run_response_digest =
      marshal::marshal_digest_for_text("test.dry_run", "dry_run");
  iteration.runtime_job_id = "runtime_job";
  iteration.runtime_job_state_digest = "job_state_digest";
  iteration.runtime_job_manifest_digest = "job_manifest_digest";
  iteration.runtime_terminal_fact_digest = "runtime_result_fact_digest";
  iteration.runtime_checkpoint_io_fact_digest = "checkpoint_io_fact_digest";
  iteration.runtime_terminal_status = "completed";
  iteration.post_run_lattice_evaluation_digest =
      marshal::marshal_digest_for_text("test.post_lattice", "satisfied");
  iteration.progress_signature =
      marshal::marshal_digest_for_text("test.progress", "progress");
  iteration.dry_run_attempted = true;
  iteration.dry_run_accepted = true;
  iteration.execution_attempted = true;
  iteration.execution_accepted = true;
  iteration.runtime_job_completion_observed = true;
  iteration.lattice_target_satisfied_after_wave = true;

  marshal::marshal_target_driver_ledger_t ledger{};
  ledger.target_id = "channel_mdn_validation_eval_ready";
  ledger.active_identity = iteration.active_identity;
  ledger.drive_mode = marshal::marshal_target_drive_mode_t::budgeted;
  ledger.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  ledger.driver_policy = policy;
  ledger.driver_policy_digest = marshal::target_driver_policy_digest(policy);
  ledger.ledger_created_at_utc = "2026-05-24T00:00:00Z";
  ledger.ledger_nonce = "test_target_driver_nonce";
  ledger.iteration_count = 1;
  ledger.runtime_handoff_attempt_count = 1;
  ledger.execution_attempt_count = 1;
  ledger.last_target_deficit_digest = iteration.target_deficit_digest;
  ledger.last_suggested_wave_digest = iteration.suggested_wave_digest;
  ledger.last_progress_signature = iteration.progress_signature;
  ledger.last_runtime_job_id = iteration.runtime_job_id;
  ledger.last_runtime_job_state_digest = iteration.runtime_job_state_digest;
  ledger.last_runtime_job_manifest_digest =
      iteration.runtime_job_manifest_digest;
  ledger.last_runtime_terminal_fact_digest =
      iteration.runtime_terminal_fact_digest;
  ledger.last_runtime_checkpoint_io_fact_digest =
      iteration.runtime_checkpoint_io_fact_digest;
  ledger.last_runtime_handoff_id = iteration.runtime_handoff_id;
  ledger.last_runtime_handoff_digest = iteration.runtime_handoff_digest;
  ledger.last_runtime_policy_digest = iteration.runtime_policy_digest;
  ledger.terminal_state = "reached";
  ledger.terminal_reason = "post-wave Lattice evaluation reported target "
                           "satisfied";
  ledger.iterations.push_back(iteration);
  marshal::finalize_target_driver_run_id(&ledger);
  return ledger;
}

marshal::tool_detail::target_driver_replay_context_t
make_m22_replay_context(const marshal::marshal_target_driver_ledger_t &ledger) {
  marshal::tool_detail::target_driver_replay_context_t context{};
  context.target_id = ledger.target_id;
  context.drive_mode = marshal::to_string(ledger.drive_mode);
  context.requested_mode = marshal::to_string(ledger.requested_mode);
  context.driver_policy_digest = ledger.driver_policy_digest;
  context.active_identity = ledger.active_identity;
  context.runtime_policy_digest = ledger.last_runtime_policy_digest;
  return context;
}

void test_m22_target_driver_replay_audit_full_and_stale_identity() {
  const auto ledger = make_m22_target_driver_ledger();
  const auto ledger_json =
      marshal::tool_detail::target_driver_ledger_json(ledger);
  const auto context = make_m22_replay_context(ledger);

  const auto audit =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, context);
  check(audit.accepted, "full target-driver ledger replay should pass");
  check(!audit.compact, "full target-driver ledger should not be compact");
  check(audit.non_authoritative,
        "target-driver replay should remain non-authoritative");
  check(audit.explanation.find("does not prove target satisfaction") !=
            std::string::npos,
        "target-driver replay should state the proof boundary");

  auto stale = context;
  stale.active_identity.protocol_contract_fingerprint = "new_pc";
  const auto stale_protocol =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, stale);
  check(!stale_protocol.accepted && stale_protocol.stale,
        "protocol contract changes should reject old driver ledgers");

  stale = context;
  stale.active_identity.graph_order_fingerprint = "new_go";
  const auto stale_graph =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, stale);
  check(!stale_graph.accepted && stale_graph.stale,
        "graph order changes should reject old driver ledgers");

  stale = context;
  stale.active_identity.target_spec_fingerprint = "new_ts";
  const auto stale_target =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, stale);
  check(!stale_target.accepted && stale_target.stale,
        "target DSL changes should reject old driver ledgers");

  stale = context;
  stale.active_identity.split_policy_fingerprint = "new_sp";
  const auto stale_split =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, stale);
  check(!stale_split.accepted && stale_split.stale,
        "split DSL changes should reject old driver ledgers");

  stale = context;
  stale.runtime_policy_digest = "new_runtime_policy";
  const auto stale_policy =
      marshal::tool_detail::replay_target_driver_ledger(ledger_json, stale);
  check(!stale_policy.accepted && stale_policy.stale,
        "Runtime policy changes should reject old driver ledgers");
}

void test_m22_target_driver_run_id_is_stable_and_not_ledger_digest() {
  auto ledger = make_m22_target_driver_ledger();
  const auto run_id = ledger.target_driver_run_id;
  const auto ledger_digest = marshal::target_driver_ledger_digest(ledger);
  marshal::finalize_target_driver_run_id(&ledger);
  check(ledger.target_driver_run_id == run_id,
        "target-driver run id finalization must be idempotent");
  check(run_id.find("marshal_target_driver_run_") == 0,
        "target-driver run id should use the run identity namespace");
  check(ledger.target_driver_run_key.find("marshal_target_driver_key_") == 0,
        "target-driver run key should use the deterministic grouping "
        "namespace");
  check(!ledger.ledger_created_at_utc.empty() && !ledger.ledger_nonce.empty(),
        "target-driver ledger should record creation timestamp and nonce");
  check(run_id != "marshal_target_driver_" + ledger_digest,
        "target-driver run id must not be the self-referential final ledger "
        "digest");
  check(marshal::target_driver_ledger_digest(ledger) == ledger_digest,
        "final ledger digest should remain stable after idempotent run-id "
        "finalization");
}

void test_m22_target_driver_run_key_groups_but_run_id_is_invocation_unique() {
  auto first = make_m22_target_driver_ledger();
  auto second = first;
  second.target_driver_run_id.clear();
  second.ledger_nonce = "second_invocation_nonce";
  marshal::finalize_target_driver_run_id(&second);

  check(first.target_driver_run_key == second.target_driver_run_key,
        "same target, identity, mode, and driver policy should share the "
        "target-driver run key");
  check(first.target_driver_run_id != second.target_driver_run_id,
        "different ledger nonce should produce a distinct target-driver run "
        "id");
}

void test_m22_target_driver_replay_audit_tamper_and_compact() {
  const auto ledger = make_m22_target_driver_ledger();
  const auto ledger_json =
      marshal::tool_detail::target_driver_ledger_json(ledger);
  const auto context = make_m22_replay_context(ledger);

  auto tampered = ledger_json;
  const std::string old_job = "\"last_runtime_job_id\":\"runtime_job\"";
  const auto old_job_pos = tampered.find(old_job);
  check(old_job_pos != std::string::npos,
        "test ledger should contain last Runtime job id");
  tampered.replace(old_job_pos, old_job.size(),
                   "\"last_runtime_job_id\":\"tampered_job\"");
  const auto tampered_audit =
      marshal::tool_detail::replay_target_driver_ledger(tampered, context);
  check(!tampered_audit.accepted,
        "tampered full target-driver ledger should fail replay audit");

  const auto ledger_digest = marshal::target_driver_ledger_digest(ledger);
  const std::string compact =
      "{\"schema_version\":\"kikijyeba.marshal.target_driver_ledger.v1\","
      "\"target_driver_run_id\":\"" +
      ledger.target_driver_run_id + "\",\"target_id\":\"" + ledger.target_id +
      "\",\"active_identity\":{\"protocol_contract_fingerprint\":\"pc\","
      "\"graph_order_fingerprint\":\"go\","
      "\"target_spec_fingerprint\":\"ts\","
      "\"split_policy_fingerprint\":\"sp\"},"
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"execute\","
      "\"driver_policy_digest\":\"" +
      ledger.driver_policy_digest +
      "\",\"iteration_count\":1,"
      "\"runtime_handoff_attempt_count\":1,"
      "\"execution_attempt_count\":1,"
      "\"last_runtime_job_id\":\"runtime_job\","
      "\"last_runtime_job_manifest_digest\":\"job_manifest_digest\","
      "\"last_runtime_terminal_fact_digest\":\"runtime_result_fact_digest\","
      "\"last_runtime_handoff_digest\":\"handoff_digest\","
      "\"last_runtime_policy_digest\":\"runtime_policy_digest_v1\","
      "\"terminal_state\":\"reached\","
      "\"ledger_digest\":\"" +
      ledger_digest +
      "\",\"compacted_fields\":[\"iterations\"],"
      "\"non_authority_statement\":\"marshal dispatch advice and receipts are "
      "audit metadata only; target satisfaction remains a lattice proof over "
      "runtime evidence\"}";
  const auto compact_audit =
      marshal::tool_detail::replay_target_driver_ledger(compact, context);
  check(compact_audit.accepted && compact_audit.compact,
        "compact target-driver ledger should remain replay-auditable");
  check(compact_audit.explanation.find("removed iteration bodies") !=
            std::string::npos,
        "compact target-driver replay should explain what was removed");

  auto compact_missing_note = compact;
  const std::string old_note = "\"compacted_fields\":[\"iterations\"]";
  const auto note_pos = compact_missing_note.find(old_note);
  check(note_pos != std::string::npos,
        "compact test ledger should contain compaction note");
  compact_missing_note.replace(note_pos, old_note.size(),
                               "\"compacted_fields\":[]");
  const auto missing_note_audit =
      marshal::tool_detail::replay_target_driver_ledger(compact_missing_note,
                                                        context);
  check(!missing_note_audit.accepted,
        "compact target-driver ledger without compaction note should fail");
}

void test_m16_target_driver_resume_guards_policy_and_identity() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  marshal::marshal_target_driver_policy_t policy{};
  policy.max_waves = 2;
  policy.max_wall_clock_seconds = 5;
  policy.no_progress_window = 1;
  const auto policy_digest = marshal::target_driver_policy_digest(policy);
  const std::string resume =
      "{\"schema_version\":\"kikijyeba.marshal.target_driver_ledger.v1\","
      "\"target_driver_run_id\":\"previous_driver\","
      "\"target_id\":\"different_target\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"driver_policy_digest\":" +
      marshal::detail::json_quote(policy_digest) +
      ",\"iteration_count\":1,"
      "\"runtime_handoff_attempt_count\":1,"
      "\"execution_attempt_count\":0,"
      "\"last_target_deficit_digest\":\"old_deficit\","
      "\"last_suggested_wave_digest\":\"old_wave\","
      "\"last_progress_signature\":\"old_progress\","
      "\"terminal_state\":\"\"}";
  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"max_waves\":2,\"max_wall_clock_seconds\":5,"
      "\"no_progress_window\":1},"
      "\"resume_ledger\":" +
      resume + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "stale resume ledger should produce a bounded Marshal packet");
  check(result.find("\"driver_terminal_state\":\"blocked_stale_resume\"") !=
            std::string::npos,
        "resume with different target should fail closed");
  check(result.find("\"resumed_from_run_id\":\"previous_driver\"") !=
            std::string::npos,
        "resume ledger should preserve the previous run id for audit");
  check(g_fake_lattice_resolve_count == 0,
        "stale resume must block before new Lattice resolution or Runtime "
        "handoff");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m16_target_driver_resume_does_not_repeat_reached_run() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  marshal::marshal_target_driver_policy_t policy{};
  policy.max_waves = 2;
  policy.max_wall_clock_seconds = 5;
  policy.no_progress_window = 1;
  const auto policy_digest = marshal::target_driver_policy_digest(policy);
  const std::string resume =
      "{\"schema_version\":\"kikijyeba.marshal.target_driver_ledger.v1\","
      "\"target_driver_run_id\":\"previous_driver\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"driver_policy_digest\":" +
      marshal::detail::json_quote(policy_digest) +
      ",\"iteration_count\":1,"
      "\"runtime_handoff_attempt_count\":1,"
      "\"execution_attempt_count\":0,"
      "\"last_target_deficit_digest\":\"old_deficit\","
      "\"last_suggested_wave_digest\":\"old_wave\","
      "\"last_progress_signature\":\"old_progress\","
      "\"ledger_digest\":\"previous_ledger_digest\","
      "\"terminal_state\":\"reached\"}";
  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"max_waves\":2,\"max_wall_clock_seconds\":5,"
      "\"no_progress_window\":1},"
      "\"resume_ledger\":" +
      resume + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "reached resume ledger should return a bounded Marshal packet");
  check(result.find("\"driver_terminal_state\":\"reached\"") !=
            std::string::npos,
        "reached resume should preserve reached terminal state");
  check(result.find("already records the target as reached") !=
            std::string::npos,
        "reached resume should explain why no new handoff occurred");
  check(g_fake_lattice_resolve_count == 0,
        "reached resume must not call Lattice or repeat Runtime handoff");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m16_5_target_driver_resume_requires_ledger_digest() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  marshal::marshal_target_driver_policy_t policy{};
  policy.max_waves = 2;
  policy.max_wall_clock_seconds = 5;
  policy.no_progress_window = 1;
  const auto policy_digest = marshal::target_driver_policy_digest(policy);
  const std::string resume =
      "{\"schema_version\":\"kikijyeba.marshal.target_driver_ledger.v1\","
      "\"target_driver_run_id\":\"previous_driver\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"driver_policy_digest\":" +
      marshal::detail::json_quote(policy_digest) +
      ",\"iteration_count\":1,"
      "\"runtime_handoff_attempt_count\":1,"
      "\"execution_attempt_count\":0,"
      "\"last_target_deficit_digest\":\"old_deficit\","
      "\"last_suggested_wave_digest\":\"old_wave\","
      "\"last_progress_signature\":\"old_progress\","
      "\"terminal_state\":\"\"}";
  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"max_waves\":2,\"max_wall_clock_seconds\":5,"
      "\"no_progress_window\":1},"
      "\"resume_ledger\":" +
      resume + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "resume ledger without ledger_digest should fail closed");
  check(result.find("\"driver_terminal_state\":\"blocked_stale_resume\"") !=
            std::string::npos,
        "missing ledger_digest should be treated as stale resume evidence");
  check(result.find("missing ledger_digest") != std::string::npos,
        "missing ledger_digest blocker should be explicit");
  check(g_fake_lattice_resolve_count == 0,
        "missing ledger_digest must block before Lattice or Runtime calls");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m16_5_target_driver_resume_requires_terminal_identity_after_execute() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  g_fake_lattice_prepare_warning = false;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  marshal::marshal_target_driver_policy_t policy{};
  policy.max_waves = 2;
  policy.max_wall_clock_seconds = 5;
  policy.no_progress_window = 1;
  const auto policy_digest = marshal::target_driver_policy_digest(policy);
  const std::string resume =
      "{\"schema_version\":\"kikijyeba.marshal.target_driver_ledger.v1\","
      "\"target_driver_run_id\":\"previous_driver\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"driver_policy_digest\":" +
      marshal::detail::json_quote(policy_digest) +
      ",\"iteration_count\":1,"
      "\"runtime_handoff_attempt_count\":1,"
      "\"execution_attempt_count\":1,"
      "\"last_target_deficit_digest\":\"old_deficit\","
      "\"last_suggested_wave_digest\":\"old_wave\","
      "\"last_progress_signature\":\"old_progress\","
      "\"ledger_digest\":\"previous_ledger_digest\","
      "\"terminal_state\":\"\"}";
  const std::string args =
      "{\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"driver_policy\":{\"max_waves\":2,\"max_wall_clock_seconds\":5,"
      "\"no_progress_window\":1},"
      "\"resume_ledger\":" +
      resume + "}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", args,
                                           &result, &error),
        "resume ledger with execution history should require terminal Runtime "
        "identity");
  check(result.find("\"driver_terminal_state\":\"blocked_stale_resume\"") !=
            std::string::npos,
        "execution history without terminal evidence identity should fail "
        "closed");
  check(result.find("terminal evidence, job manifest, or handoff identity") !=
            std::string::npos,
        "resume terminal evidence blocker should be explicit");
  check(g_fake_lattice_resolve_count == 0,
        "stale execution resume must block before Lattice or Runtime calls");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m9_marshal_tool_handlers_validate_arguments() {
  auto advice = valid_advice();
  auto request = valid_request(advice);
  const auto ctx = context();

  std::string result;
  std::string error;
  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_target_deficit_callback);
  const std::string lookup_args =
      "{\"target_id\":\"lookup_target\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
      "\"drive_mode\":\"one_step\","
      "\"requested_mode\":\"dry_run\","
      "\"context\":{\"active_config_path\":\"/tmp/marshal_lookup/.config\","
      "\"active_runtime_root\":\"/tmp/marshal_lookup/runtime\","
      "\"active_identity\":{\"protocol_contract_fingerprint\":\"pc\","
      "\"graph_order_fingerprint\":\"go\","
      "\"target_spec_fingerprint\":\"ts\","
      "\"split_policy_fingerprint\":\"sp\"},"
      "\"supported_wave_targets\":[\"wikimyei.inference.expected_value.mdn\"],"
      "\"supported_source_ranges\":[\"anchor_index\"],"
      "\"allowed_model_state_roots\":[\"/tmp/marshal_lookup/runtime\"],"
      "\"freshness_check_timestamp_utc\":\"2026-05-24T00:00:00Z\"}}";
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", lookup_args,
                                           &result, &error),
        "prepare should ask Lattice and materialize advice");
  check(g_fake_lattice_tool_name == "hero.lattice.target_deficit",
        "prepare should call Lattice target_deficit");
  check(g_fake_lattice_arguments_json.find("\"target_id\":\"lookup_target\"") !=
            std::string::npos,
        "prepare should pass target_id to Lattice");
  check(result.find("\"tool\":\"hero.marshal.prepare\"") != std::string::npos,
        "prepare should return the public operator packet");
  check(
      result.find("\"target_status\":\"metric_failed\"") != std::string::npos,
      "metric_failed Lattice targets should remain dispatchable when planned");
  check(result.find("PLAN_INPUT_MDN_CHECKPOINT") != std::string::npos &&
            result.find("PLAN_INPUT_REPRESENTATION_CHECKPOINT") !=
                std::string::npos,
        "prepare should map Lattice checkpoint hints to PLAN_INPUT "
        "keys");

  const std::string free_text_lookup_args =
      lookup_args.substr(0, lookup_args.size() - 1) +
      ",\"target_text\":\"dispatch whatever target seems useful\"}";
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", free_text_lookup_args, &result, &error),
        "prepare must reject free-form target text");
  check(error.find("unknown field: target_text") != std::string::npos,
        "prepare should expose unknown-field rejection for "
        "target_text");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_satisfied_without_certificate_callback);
  result.clear();
  error.clear();
  const std::string already_satisfied_args =
      "{\"target_id\":\"satisfied_without_certificate\","
      "\"drive_mode\":\"one_step\","
      "\"requested_mode\":\"dry_run\","
      "\"include_runtime_dry_run\":true,"
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\"}";
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", already_satisfied_args, &result, &error),
        "prepare should stop cleanly when Lattice returns a "
        "satisfied target with suggested_wave=null");
  check(result.find("\"dispatch_state\":\"already_satisfied\"") !=
            std::string::npos,
        "already satisfied target should report already_satisfied dispatch "
        "state");
  check(result.find("\"driver_terminal_state\":\"reached\"") !=
            std::string::npos,
        "already satisfied target should terminate as reached from Lattice");
  check(result.find("Lattice plan result missing suggested_wave") ==
            std::string::npos,
        "already satisfied target must not require a suggested wave");
  check(result.find("not_applicable_no_suggested_wave") != std::string::npos,
        "already satisfied target should not compare Runtime wave shape "
        "against an absent suggested wave");
  check(result.find("\"differing_fields\":[]") != std::string::npos,
        "already satisfied target should not emit irrelevant Runtime wave "
        "diffs");
  check(result.find("runtime_wave_mismatch") == std::string::npos,
        "already satisfied target should not carry irrelevant Runtime wave "
        "mismatch refusal noise");
  marshal::set_marshal_lattice_tool_callback(nullptr);

  const auto wave = valid_active_wave(advice);
  marshal::marshal_dry_run_dispatch_response_t response{};
  response.accepted = true;
  response.advice_digest = marshal::dispatch_advice_digest(advice);
  response.request_digest = marshal::dispatch_request_digest(request);
  response.runtime_request_digest = marshal::runtime_dry_run_request_digest(
      marshal::build_runtime_dry_run_dispatch_preview(advice, request, ctx,
                                                      valid_policy(), wave)
          .runtime_request);
  response.runtime_handoff.arguments_digest =
      marshal::marshal_digest_for_text("test.arguments", "{}");
  response.runtime_handoff.tool_result_json = "{\"ok\":true}";
  response.response_digest =
      marshal::dry_run_dispatch_response_digest(response);
  const auto receipt = marshal::make_dry_run_dispatch_receipt(
      advice, response, "2026-05-23T00:00:00Z");
  const std::string status_args =
      "{\"receipt_root\":\"/tmp/marshal_status\",\"receipts\":[" +
      receipt_json(receipt) +
      "],\"runtime_policy\":" + policy_json(valid_policy()) +
      ",\"lattice_advice_surface_available\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.status", status_args,
                                           &result, &error),
        "status handler should execute deterministic primitive");
  check(result.find("\"recent_receipt_count\":1") != std::string::npos,
        "status handler should expose receipt count");
  check(result.find("\"schema\":\"kikijyeba.marshal.status.v1\"") !=
                std::string::npos &&
            result.find("\"target_proof\":false") != std::string::npos &&
            result.find("\"fact_families_are_not_target_kinds\":true") !=
                std::string::npos &&
            result.find("\"allocation_decision\":false") != std::string::npos &&
            result.find("\"market_readiness_decision\":false") !=
                std::string::npos,
        "status handler should declare the same read-only non-proof boundary "
        "as other Marshal summaries");

  for (const auto &removed_tool :
       {"hero.marshal.lookup_target_advice",
        "hero.marshal.prepare_target_dispatch", "hero.marshal.validate_advice",
        "hero.marshal.dry_run_dispatch", "hero.marshal.execution_gate",
        "hero.marshal.replay_receipt", "hero.marshal.batch_preview",
        "hero.marshal.compare_runs", "hero.marshal.summarize_training_state"}) {
    check(!marshal::execute_marshal_tool_json(removed_tool, "{}", &result,
                                              &error),
          std::string("removed Marshal tool should not be callable: ") +
              removed_tool);
    check(error.find("unknown tool: ") != std::string::npos,
          "removed Marshal tools should fail as unknown tools");
  }
}

void test_artifact_evidence_panel_and_reach_boundary() {
  g_fake_lattice_evaluate_target_count = 0;
  g_fake_lattice_scan_facts_count = 0;
  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  g_fake_lattice_resolve_count = 0;
  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_artifact_evidence_callback);

  std::string result;
  std::string error;
  const std::string panel_args =
      "{\"subject\":\"facts\","
      "\"target_id\":\"forecast_eval_artifact_ready\","
      "\"fact_family\":\"forecast_eval\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_facts\":true,"
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", panel_args,
                                           &result, &error),
        "inspect should expose a read-only target/fact packet");
  check(g_fake_lattice_evaluate_target_count == 1,
        "evidence panel should call Lattice evaluate_target for target proof");
  check(g_fake_lattice_scan_facts_count == 1,
        "evidence panel should call Lattice scan_facts when facts are "
        "requested");
  check(g_fake_lattice_fact_lineage_count == 1,
        "evidence panel should call Lattice fact_lineage for fact-family "
        "lineage");
  check(g_fake_lattice_arguments_json.find("\"family\":\"forecast_eval\"") !=
            std::string::npos,
        "evidence panel should pass fact_family to Lattice as family");
  check(result.find("\"tool\":\"hero.marshal.inspect\"") != std::string::npos,
        "evidence panel should identify the public tool");
  check(result.find("\"read_only\":true") != std::string::npos &&
            result.find("\"dispatchable\":false") != std::string::npos &&
            result.find("\"runtime_executor\":false") != std::string::npos,
        "evidence panel must be read-only and non-dispatchable");
  check(result.find("\"target_proof\":false") != std::string::npos &&
            result.find("\"fact_families_are_not_target_kinds\":true") !=
                std::string::npos &&
            result.find("\"allocation_decision\":false") != std::string::npos &&
            result.find("\"market_readiness_decision\":false") !=
                std::string::npos &&
            result.find("\"deployment_decision\":false") != std::string::npos &&
            result.find("\"marshal_proof_authority\":false") !=
                std::string::npos,
        "evidence panel must declare fact families as non-targets and avoid "
        "proof, allocation, market, or deployment authority");
  check(result.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "evidence panel must not claim target satisfaction for Marshal");
  check(result.find("\"target_class\":\"artifact_readiness\"") !=
                std::string::npos &&
            result.find("\"kind\":\"not_applicable\"") != std::string::npos &&
            result.find("\"target_kind_applicable\":false") !=
                std::string::npos &&
            result.find("\"target_kind_effective\":\"none\"") !=
                std::string::npos &&
            result.find("\"proof_kind\":\"forecast_eval_artifact_bound\"") !=
                std::string::npos &&
            result.find("\"artifact_proof_count\":1") != std::string::npos,
        "evidence panel should summarize artifact proof identity and target "
        "kind non-applicability");
  check(result.find("\"artifact_proof_template_bound_count\":1") !=
                std::string::npos &&
            result.find("\"artifact_proof_template_unbound_count\":0") !=
                std::string::npos &&
            result.find("\"artifact_proof_kinds\":[\"forecast_eval_artifact_"
                        "bound\"]") != std::string::npos &&
            result.find("forecast evaluation artifact existence, checkpoint "
                        "lineage") != std::string::npos,
        "evidence panel should relay explicit artifact proof-template binding");
  check(
      result.find("\"artifact_failed_proof_count\":1") != std::string::npos &&
          result.find("\"artifact_proof_template_bound_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_proof_template_unbound_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_lineage_unbound_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_authority_drift_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_integrity_flags\":[\"lineage_unbound\"]") !=
              std::string::npos &&
          result.find("\"artifact_authority_flags\":[\"quality_authority\"]") !=
              std::string::npos &&
          result.find("\"artifact_issue_codes\":[\"forecast_eval_must_"
                      "remain_artifact_evidence_only\","
                      "\"baseline_fact_digest_not_found\"]") !=
              std::string::npos &&
          result.find("\"artifact_related_fact_integrity_issue_codes\":["
                      "\"mdn:baseline_fact_digest_not_found\"]") !=
              std::string::npos &&
          result.find("\"artifact_deficit_keys\":[\"artifact:forecast_eval_"
                      "authority\",\"artifact:forecast_eval_lineage\","
                      "\"artifact:forecast_eval_issue_forecast_eval_must_"
                      "remain_artifact_evidence_only\","
                      "\"artifact:forecast_eval_issue_baseline_fact_digest_not_"
                      "found\"]") != std::string::npos &&
          result.find("\"primary_deficit_key\":\"artifact:forecast_eval_"
                      "authority\"") != std::string::npos,
      "evidence panel should preserve failed artifact proof authority "
      "details from Lattice");
  check(
      result.find("\"artifact_authority_denial_flags\":[\"target_dependency_"
                  "authority\",\"runtime_wave_authority\","
                  "\"marshal_reachability\",\"checkpoint_source_authority\","
                  "\"plan_checkpoint_input_authority\"]") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability\":false") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority\":false") !=
              std::string::npos,
      "evidence panel should relay explicit artifact boundary denials without "
      "turning them into Marshal authority");
  check(
      result.find("\"policy_gate_reservation_count\":1") != std::string::npos &&
          result.find("\"enabled_policy_gate_count\":0") != std::string::npos &&
          result.find("\"disabled_policy_gate_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_verified_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":0") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_input_contract_complete_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_missing_input_contract_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_all_input_contracts_complete\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_decision_policy_authority_count\":0") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_mismatch\":false") !=
              std::string::npos &&
          result.find("\"policy_input_contract_complete\":true") !=
              std::string::npos &&
          result.find("\"decision_policy_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_certificate_authority_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_decision_policy_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_reservation_summary\":{") !=
              std::string::npos &&
          result.find(
              "\"policy_id\":\"forecast_quality_acceptance_reserved\"") !=
              std::string::npos,
      "evidence panel should relay disabled policy-gate reservations without "
      "granting Marshal proof, status, dispatch, or fingerprint authority");

  g_fake_lattice_policy_fingerprint_mismatch = true;
  g_fake_lattice_evaluate_target_count = 0;
  g_fake_lattice_scan_facts_count = 0;
  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", panel_args,
                                           &result, &error),
        "inspect should surface a Lattice policy-fingerprint "
        "mismatch as read-only audit context");
  check(g_fake_lattice_evaluate_target_count == 1,
        "fingerprint mismatch evidence panel should still ask Lattice for the "
        "target proof");
  check(
      result.find("\"policy_gate_policy_fingerprint_verified_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":1") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":false") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_verified\":false") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_mismatch\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_allocation_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_market_readiness_authority\":false") !=
              std::string::npos,
      "policy-fingerprint mismatch should be visible but must not grant "
      "Marshal "
      "proof, dispatch, allocation, or market authority");
  g_fake_lattice_policy_fingerprint_mismatch = false;

  check(result.find("\"source_tool\":\"hero.lattice.scan_facts\"") !=
            std::string::npos,
        "fact panel should report the Lattice fact scan source");
  check(result.find("\"lineage_panel\":{\"source_tool\":\"hero.lattice."
                    "fact_lineage\"") != std::string::npos &&
            result.find("\"lineage_rows_are_audit_only\":true") !=
                std::string::npos &&
            result.find("\"cache_rows_used_for_target_satisfaction\":false") !=
                std::string::npos &&
            result.find("\"selected_relations\":[\"forecast_eval\"]") !=
                std::string::npos &&
            result.find("\"lineage_rows\":[{\"relation\":\"forecast_eval\"") !=
                std::string::npos,
        "fact panel should relay Lattice fact_lineage as an audit-only lineage "
        "panel");
  check(result.find("\"fact_integrity_summary\":{") != std::string::npos &&
            result.find("\"relation_declared_count\":3") != std::string::npos &&
            result.find("\"relation_bound_count\":0") != std::string::npos &&
            result.find("\"unresolved_relation_count\":3") !=
                std::string::npos &&
            result.find("\"identity_mismatch_count\":0") != std::string::npos &&
            result.find("\"families_with_unresolved_relation\":["
                        "\"forecast_eval\"]") != std::string::npos &&
            result.find("\"integrity_flags\":[\"unresolved_relation\"]") !=
                std::string::npos &&
            result.find("\"mdn:baseline_fact_digest_not_found\"") !=
                std::string::npos,
        "fact panel should summarize unresolved fact-family lineage separately "
        "from target proof deficits");
  check(
      result.find("\"fact_catalog_family_count\":1") != std::string::npos &&
          result.find("\"artifact_readiness_proofable_family_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_readiness_promotion_blocked_family_count\":"
                      "0") != std::string::npos &&
          result.find("\"fact_catalog_families\":[\"forecast_eval\"]") !=
              std::string::npos &&
          result.find("\"artifact_readiness_proofable_families\":["
                      "\"forecast_eval\"]") != std::string::npos &&
          result.find("\"artifact_readiness_proof_kinds\":[\"forecast_eval_"
                      "artifact_bound\"]") != std::string::npos &&
          result.find("\"artifact_readiness_promotion_blocked_reasons\":[]") !=
              std::string::npos &&
          result.find("forecast evaluation artifact existence, checkpoint "
                      "lineage") != std::string::npos,
      "fact panel should relay catalog proof-template and promotion-boundary "
      "metadata");

  g_fake_lattice_evaluate_target_count = 0;
  g_fake_lattice_scan_facts_count = 0;
  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  result.clear();
  error.clear();
  const std::string family_only_panel_args =
      "{\"subject\":\"facts\","
      "\"fact_family\":\"replay_environment\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", family_only_panel_args, &result, &error),
        "inspect should support fact-family-only replay "
        "evidence inspection");
  check(g_fake_lattice_evaluate_target_count == 0,
        "fact-family-only evidence inspection must not ask Lattice to prove a "
        "target");
  check(g_fake_lattice_scan_facts_count == 0 &&
            g_fake_lattice_fact_summary_count == 1 &&
            g_fake_lattice_fact_lineage_count == 1,
        "fact-family-only evidence inspection should use summary mode unless "
        "facts are requested and still relay lineage");
  check(g_fake_lattice_arguments_json.find(
            "\"family\":\"replay_environment\"") != std::string::npos,
        "fact-family-only evidence inspection should pass replay_environment "
        "to Lattice");
  check(result.find("\"target_panel\":null") != std::string::npos &&
            result.find("\"fact_family\":\"replay_environment\"") !=
                std::string::npos &&
            result.find("\"replay_environment_summary\":{") !=
                std::string::npos &&
            result.find("\"replay_contract_version_bound_count\":1") !=
                std::string::npos &&
            result.find("\"replay_contract_component_bound_count\":1") !=
                std::string::npos &&
            result.find("\"replay_contract_policy_surface_bound_count\":1") !=
                std::string::npos &&
            result.find("\"replay_contract_time_law_bound_count\":1") !=
                std::string::npos &&
            result.find("\"replay_contract_guard_bound_count\":1") !=
                std::string::npos &&
            result.find("\"episode_requested_range_bound_count_total\":2") !=
                std::string::npos &&
            result.find("\"episode_cursor_bound_count_total\":2") !=
                std::string::npos &&
            result.find("\"missing_episode_requested_range_count\":0") !=
                std::string::npos &&
            result.find("\"missing_episode_cursor_evidence_count\":0") !=
                std::string::npos &&
            result.find("\"artifact_readiness_proofable_families\":[]") !=
                std::string::npos &&
            result.find("\"artifact_readiness_proof_kinds\":[]") !=
                std::string::npos &&
            result.find("\"artifact_readiness_promotion_blocked_reasons\":["
                        "\"no_artifact_proof_template\"]") !=
                std::string::npos &&
            result.find("\"dispatchable\":false") != std::string::npos &&
            result.find("\"runtime_executor\":false") != std::string::npos,
        "fact-family-only replay evidence panel must remain read-only and "
        "surface replay contract counters");

  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  result.clear();
  error.clear();
  const std::string family_only_forecast_eval_args =
      "{\"subject\":\"facts\","
      "\"fact_family\":\"forecast_eval\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           family_only_forecast_eval_args,
                                           &result, &error),
        "inspect should expose forecast_eval fact summaries "
        "without target proof");
  check(g_fake_lattice_evaluate_target_count == 0 &&
            g_fake_lattice_fact_summary_count == 1 &&
            g_fake_lattice_fact_lineage_count == 1,
        "forecast_eval fact-family-only evidence inspection should remain "
        "summary-only plus lineage audit");
  check(result.find("\"target_panel\":null") != std::string::npos &&
            result.find("\"source_tool\":\"hero.lattice.fact_summary\"") !=
                std::string::npos &&
            result.find("\"lineage_panel\":{\"source_tool\":\"hero.lattice."
                        "fact_lineage\"") != std::string::npos &&
            result.find("\"selected_relations\":[\"forecast_eval\"]") !=
                std::string::npos &&
            result.find("\"fact_integrity_summary\":{") != std::string::npos &&
            result.find("\"relation_declared_count\":3") != std::string::npos &&
            result.find("\"unresolved_relation_count\":3") !=
                std::string::npos &&
            result.find("\"families_with_unresolved_relation\":["
                        "\"forecast_eval\"]") != std::string::npos &&
            result.find("\"artifact_readiness_proofable_families\":["
                        "\"forecast_eval\"]") != std::string::npos &&
            result.find("\"artifact_readiness_proof_kinds\":[\"forecast_eval_"
                        "artifact_bound\"]") != std::string::npos,
        "forecast_eval fact-family-only evidence panel should surface "
        "unresolved transform/baseline/selection lineage");

  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  g_fake_lattice_fact_preview_count = 0;
  result.clear();
  error.clear();
  const std::string family_preview_args =
      "{\"subject\":\"facts\","
      "\"fact_family\":\"forecast_eval\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_preview\":true,\"fact_index\":0,"
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", family_preview_args, &result, &error),
        "inspect should expose fact_preview when explicitly "
        "requested");
  check(g_fake_lattice_evaluate_target_count == 0 &&
            g_fake_lattice_fact_summary_count == 1 &&
            g_fake_lattice_fact_lineage_count == 1 &&
            g_fake_lattice_fact_preview_count == 1,
        "fact preview evidence inspection should remain summary plus lineage "
        "plus preview, without target proof");
  check(
      result.find("\"preview_panel\":{\"source_tool\":\"hero.lattice."
                  "fact_preview\"") != std::string::npos &&
          result.find("\"preview_rows_are_audit_only\":true") !=
              std::string::npos &&
          result.find("\"facts_used_for_target_satisfaction\":false") !=
              std::string::npos &&
          result.find("\"cache_rows_used_for_target_satisfaction\":false") !=
              std::string::npos &&
          result.find("\"fact_index_filter\":0") != std::string::npos &&
          result.find("\"matching_fact_count\":1") != std::string::npos &&
          result.find("\"returned_fact_count\":1") != std::string::npos &&
          result.find("\"identity_envelope\":{\"schema\":\"kikijyeba.lattice."
                      "fact_identity_envelope.v1\"") != std::string::npos &&
          result.find("\"fact_identity_contract_schema\":\"kikijyeba.lattice."
                      "fact_identity_contract.v1\"") != std::string::npos &&
          result.find("\"fact_family\":\"forecast_eval\"") !=
              std::string::npos &&
          result.find("\"parent_exposure_fact_digests\":[\"parent_digest\"]") !=
              std::string::npos &&
          result.find("\"support_count\":42") != std::string::npos &&
          result.find("\"facts_used_for_target_satisfaction\":false") !=
              std::string::npos &&
          result.find("\"forecast_artifact_digest\":\"forecast_artifact_1\"") !=
              std::string::npos &&
          result.find("\"preview_lattice_args\":\"{\\\"runtime_root\\\":"
                      "\\\"/tmp/marshal_artifact/runtime\\\",\\\"family\\\":"
                      "\\\"forecast_eval\\\",\\\"fact_index\\\":0}\"") !=
              std::string::npos,
      "fact preview panel should relay concrete facts without authority");

  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  result.clear();
  error.clear();
  const std::string family_only_selection_signal_args =
      "{\"subject\":\"facts\","
      "\"fact_family\":\"selection_signal\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           family_only_selection_signal_args,
                                           &result, &error),
        "inspect should expose selection_signal fact summaries "
        "without target proof");
  check(g_fake_lattice_evaluate_target_count == 0 &&
            g_fake_lattice_fact_summary_count == 1 &&
            g_fake_lattice_fact_lineage_count == 1,
        "selection_signal fact-family-only evidence inspection should remain "
        "summary-only plus lineage audit");
  check(result.find("\"target_panel\":null") != std::string::npos &&
            result.find("\"fact_family\":\"selection_signal\"") !=
                std::string::npos &&
            result.find("\"artifact_readiness_proofable_family_count\":0") !=
                std::string::npos &&
            result.find("\"artifact_readiness_promotion_blocked_family_count\":"
                        "1") != std::string::npos &&
            result.find("\"artifact_readiness_promotion_blocked_families\":["
                        "\"selection_signal\"]") != std::string::npos &&
            result.find("\"artifact_readiness_promotion_blocked_reasons\":["
                        "\"leakage_visibility_only\"]") != std::string::npos &&
            result.find("\"dispatchable\":false") != std::string::npos,
        "selection_signal fact panel should relay the leakage-only promotion "
        "boundary");

  result.clear();
  error.clear();
  g_fake_lattice_resolve_count = 0;
  const std::string reach_args =
      "{\"target_id\":\"forecast_eval_artifact_ready\","
      "\"drive_mode\":\"one_step\","
      "\"requested_mode\":\"dry_run\","
      "\"include_runtime_dry_run\":true,"
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", reach_args,
                                           &result, &error),
        "prepare should return an operator packet for artifact "
        "targets");
  check(result.find("Lattice plan result missing suggested_wave") ==
            std::string::npos,
        "artifact targets should not fail as malformed missing-wave plans");
  check(result.find("\"blocker_bucket\":\"non_dispatchable_artifact_"
                    "readiness\"") != std::string::npos,
        "prepare should explicitly mark artifact readiness "
        "targets non-dispatchable");
  check(result.find("\"kind\":\"not_applicable\"") != std::string::npos &&
            result.find("\"target_kind_applicable\":false") !=
                std::string::npos &&
            result.find("\"target_kind_effective\":\"none\"") !=
                std::string::npos,
        "artifact readiness reach should relay Lattice target-kind "
        "non-applicability");
  check(result.find("\"next_action\":\"inspect\"") != std::string::npos,
        "artifact readiness reach should route to evidence inspection");
  check(result.find("\"tool\":\"hero.marshal.inspect\"") != std::string::npos,
        "artifact readiness reach should suggest the evidence-panel tool");
  check(result.find("\"runtime_dry_run\":{\"requested\":true,"
                    "\"attempted\":false") != std::string::npos,
        "artifact readiness reach must not attempt Runtime dry-run");
  check(g_fake_lattice_resolve_count == 0,
        "artifact readiness reach must not materialize latest_satisfying plan "
        "inputs");
  check(result.find("\"dispatch_validation_applied\":false") !=
            std::string::npos,
        "artifact readiness reach must skip Runtime dispatch validation");
  check(result.find("\"target_proof\":false") != std::string::npos &&
            result.find("\"target_satisfaction_claimed_by_marshal\":false") !=
                std::string::npos &&
            result.find("\"fact_families_are_not_target_kinds\":true") !=
                std::string::npos &&
            result.find("\"marshal_proof_authority\":false") !=
                std::string::npos &&
            result.find("\"allocation_decision\":false") != std::string::npos &&
            result.find("\"market_readiness_decision\":false") !=
                std::string::npos,
        "artifact readiness reach should remain a non-proof, non-decision "
        "operator packet even when it prepares dispatch context");
  check(
      result.find("\"policy_gate_reservation_count\":1") != std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_verified_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":0") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos &&
          result.find(
              "\"policy_id\":\"forecast_quality_acceptance_reserved\"") !=
              std::string::npos,
      "artifact readiness reach should carry policy-gate reservations and "
      "fingerprint audit only as read-only Lattice context");
  check(result.find("\"validation\":{\"dispatchable\":false,"
                    "\"refusal_reasons\":[]}") != std::string::npos,
        "artifact readiness machine payload should not carry dispatch "
        "validation refusals");
  check(
      result.find("\"decision\":{\"accepted\":false,"
                  "\"runtime_handoff_available\":false,"
                  "\"refusal_reasons\":[],\"runtime_request_digest\":\"\"}") !=
          std::string::npos,
      "artifact readiness machine payload must not derive a Runtime preview "
      "request");
  check(result.find("missing_suggested_wave") == std::string::npos,
        "artifact readiness reach must not report missing suggested_wave as a "
        "dispatch refusal");
  check(g_fake_lattice_resolve_count == 0,
        "artifact readiness reach must not resolve model-state checkpoints");

  result.clear();
  error.clear();
  const std::string evaluate_artifact_args =
      "{\"subject\":\"target\","
      "\"target_id\":\"forecast_eval_artifact_ready\","
      "\"runtime_root\":\"/tmp/marshal_artifact/runtime\","
      "\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", evaluate_artifact_args, &result, &error),
        "evaluate subject=target should report artifact-readiness targets: " +
            error);
  check(g_fake_lattice_tool_name == "hero.lattice.target_deficit",
        "evaluate subject=target should use Lattice target_deficit for "
        "artifact targets");
  check(result.find("\"target_class\":\"artifact_readiness\"") !=
                std::string::npos &&
            result.find("\"kind\":\"not_applicable\"") != std::string::npos &&
            result.find("\"target_kind_applicable\":false") !=
                std::string::npos &&
            result.find("\"target_kind_effective\":\"none\"") !=
                std::string::npos &&
            result.find("\"proof_kind\":\"forecast_eval_artifact_bound\"") !=
                std::string::npos &&
            result.find("\"subject_fact_family\":\"forecast_eval\"") !=
                std::string::npos,
        "target evaluation should preserve artifact proof identity and target "
        "kind non-applicability");
  check(result.find("\"next_safe_action\":\"inspect\"") != std::string::npos,
        "artifact target evaluation should route to evidence inspection");
  check(result.find("\"next_safe_action\":\"prepare\"") == std::string::npos,
        "artifact target evaluation must not suggest prepare");
  check(result.find("\"suggested_wave\":null") != std::string::npos,
        "artifact target evaluation should preserve the absent suggested wave");
  check(result.find("\"target_proof\":false") != std::string::npos &&
            result.find("\"target_satisfaction_claimed_by_marshal\":false") !=
                std::string::npos &&
            result.find("\"fact_families_are_not_target_kinds\":true") !=
                std::string::npos &&
            result.find("\"marshal_proof_authority\":false") !=
                std::string::npos &&
            result.find("\"allocation_decision\":false") != std::string::npos &&
            result.find("\"market_readiness_decision\":false") !=
                std::string::npos,
        "target evaluation should relay Lattice proof context without making "
        "Marshal a proof or decision authority");
  check(
      result.find("\"policy_gate_reservation_count\":1") != std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_verified_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":0") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_reservation_summary\":{") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos,
      "target evaluation should relay policy-gate reservations without "
      "turning them into Marshal authority");

  result.clear();
  error.clear();
  const std::string fact_family_reach_args =
      "{\"target_id\":\"forecast_eval_artifact_ready\","
      "\"fact_family\":\"forecast_eval\","
      "\"drive_mode\":\"one_step\"}";
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.prepare", fact_family_reach_args, &result, &error),
        "prepare must reject fact_family arguments instead of "
        "treating fact evidence as reachable work");
  check(error.find("unknown field: fact_family") != std::string::npos,
        "prepare should expose unknown-field rejection for "
        "fact_family");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_artifact_reach_warning_policy_remains_inspection_only() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_artifact_warning = true;
  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_artifact_evidence_callback);

  std::string result;
  std::string error;
  const std::string reach_args =
      "{\"target_id\":\"forecast_eval_artifact_ready\","
      "\"drive_mode\":\"budgeted\","
      "\"requested_mode\":\"dry_run\","
      "\"include_runtime_dry_run\":true,"
      "\"include_machine_payload\":true,"
      "\"driver_policy\":{\"max_waves\":3,"
      "\"max_wall_clock_seconds\":5,"
      "\"stop_on_lattice_warning\":true,"
      "\"stop_on_warning_severity\":\"watch\","
      "\"no_progress_window\":1}}";
  check(marshal::execute_marshal_tool_json("hero.marshal.prepare", reach_args,
                                           &result, &error),
        "budgeted artifact reach should produce an inspection packet even when "
        "Lattice warnings are present: " +
            error);
  check(result.find("\"driver_terminal_state\":\"blocked_non_dispatchable_"
                    "artifact_readiness\"") != std::string::npos &&
            result.find("\"blocker_bucket\":\"non_dispatchable_artifact_"
                        "readiness\"") != std::string::npos,
        "artifact readiness should remain the terminal bucket before warning "
        "stop policy is considered");
  check(result.find("\"driver_terminal_state\":\"blocked_lattice_warning\"") ==
                std::string::npos &&
            result.find("lattice_warning_severity") == std::string::npos &&
            result.find("lattice_warning_id:artifact_warning") ==
                std::string::npos,
        "artifact warnings must not reframe the non-dispatchable evidence "
        "target as a warning-gated runtime target");
  check(result.find("\"next_safe_action\":\"inspect\"") != std::string::npos &&
            result.find("\"next_action\":\"inspect\"") != std::string::npos &&
            result.find("\"tool\":\"hero.marshal.inspect\"") !=
                std::string::npos,
        "artifact warning packets should still route operators to evidence "
        "inspection");
  check(result.find("\"runtime_dry_run\":{\"requested\":true,"
                    "\"attempted\":false") != std::string::npos &&
            result.find("\"runtime_handoff_attempt_count\":0") !=
                std::string::npos,
        "artifact warning packets must not hand off to Runtime");
  check(result.find("\"dispatch_validation_applied\":false") !=
            std::string::npos,
        "artifact warning packets must still bypass dispatch validation");
  check(g_fake_lattice_resolve_count == 0,
        "artifact warning packets must not resolve model-state checkpoints");

  g_fake_lattice_artifact_warning = false;
  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_artifact_evidence_panel_with_real_lattice_response() {
  const auto root = make_tmp_dir("real_lattice_artifact_panel");
  const auto runtime_root = root / "runtime";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      runtime_root, /*forecast_baseline_digest_mismatch=*/true);

  g_fake_lattice_evaluate_target_count = 0;
  g_fake_lattice_scan_facts_count = 0;
  g_fake_lattice_fact_summary_count = 0;
  g_fake_lattice_fact_lineage_count = 0;
  g_fake_lattice_tool_name.clear();
  g_fake_lattice_arguments_json.clear();
  marshal::set_marshal_lattice_tool_callback(real_lattice_hero_callback);

  std::string result;
  std::string error;
  const std::string panel_args =
      "{\"subject\":\"facts\","
      "\"target_id\":\"forecast_eval_artifact_ready\","
      "\"fact_family\":\"forecast_eval\","
      "\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":\"/cuwacunu/src/config/.config\","
      "\"protocol_contract_fingerprint\":\"contract_1\","
      "\"graph_order_fingerprint\":\"graph_1\","
      "\"source_cursor_token\":\"cursor_1\","
      "\"mdn_assembly_fingerprint\":\"mdn_1\","
      "\"include_facts\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", panel_args,
                                           &result, &error),
        "inspect should relay a real Lattice artifact response: " + error);
  check(g_fake_lattice_evaluate_target_count == 1 &&
            g_fake_lattice_scan_facts_count == 1 &&
            g_fake_lattice_fact_lineage_count == 1,
        "real Lattice panel should call evaluate_target, scan_facts, and "
        "fact_lineage");
  check(g_fake_lattice_tool_name == "hero.lattice.fact_lineage",
        "real Lattice panel should finish by inspecting requested fact "
        "lineage");
  check(result.find("\"tool\":\"hero.marshal.inspect\"") != std::string::npos,
        "real Lattice panel should identify the Marshal tool");
  check(result.find("\"read_only\":true") != std::string::npos &&
            result.find("\"dispatchable\":false") != std::string::npos &&
            result.find("\"runtime_executor\":false") != std::string::npos,
        "real Lattice panel must remain read-only and non-dispatchable");
  check(result.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "real Lattice panel must not claim target satisfaction for Marshal");
  check(result.find("\"target_panel\":{\"source_tool\":\"hero.lattice."
                    "evaluate_target\"") != std::string::npos &&
            result.find("\"status\":\"blocked\"") != std::string::npos &&
            result.find("\"target_class\":\"artifact_readiness\"") !=
                std::string::npos &&
            result.find("\"kind\":\"not_applicable\"") != std::string::npos &&
            result.find("\"target_kind_applicable\":false") !=
                std::string::npos &&
            result.find("\"target_kind_effective\":\"none\"") !=
                std::string::npos &&
            result.find("\"proof_kind\":\"forecast_eval_artifact_bound\"") !=
                std::string::npos &&
            result.find("\"subject_fact_family\":\"forecast_eval\"") !=
                std::string::npos,
        "real Lattice target panel should preserve artifact proof identity and "
        "target-kind non-applicability");
  check(
      result.find("\"artifact_proof_count\":1") != std::string::npos &&
          result.find("\"artifact_proof_template_bound_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_proof_template_unbound_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_failed_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_lineage_unbound_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_authority_drift_proof_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_integrity_flags\":[\"lineage_unbound\"]") !=
              std::string::npos &&
          result.find("\"artifact_authority_flags\":[]") != std::string::npos &&
          result.find("\"artifact_fact_preview_hint_count\":1") !=
              std::string::npos &&
          result.find(
              "\"artifact_fact_preview_families\":[\"forecast_eval\"]") !=
              std::string::npos &&
          result.find("\"artifact_fact_preview_tools\":[\"hero.lattice."
                      "fact_preview\"]") != std::string::npos &&
          result.find("\"artifact_fact_preview_marshal_tools\":[\"hero."
                      "marshal.inspect\"]") != std::string::npos &&
          result.find("\"artifact_issue_codes\":[\"baseline_fact_digest_not_"
                      "found\"]") != std::string::npos &&
          result.find("\"artifact_related_fact_integrity_issue_codes\":["
                      "\"artifact_job:baseline_fact_digest_not_found\"]") !=
              std::string::npos,
      "real Lattice target panel should relay artifact proof failure details");
  check(
      result.find("\"artifact_authority_denial_flags\":[\"target_dependency_"
                  "authority\",\"runtime_wave_authority\","
                  "\"marshal_reachability\",\"checkpoint_source_authority\","
                  "\"plan_checkpoint_input_authority\"]") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability\":false") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority\":false") !=
              std::string::npos,
      "real Lattice target panel should preserve explicit non-dispatch and "
      "non-checkpoint authority denials");
  check(result.find("forecast evaluation artifact existence, checkpoint "
                    "lineage") != std::string::npos,
        "real Lattice target panel should relay artifact proof-template claim");
  check(
      result.find("\"policy_gate_reservation_summary\":{") !=
              std::string::npos &&
          result.find("\"policy_gate_reservations\":[") != std::string::npos &&
          result.find("\"enabled_policy_gates_fail_closed\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_verified_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":0") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_mismatch\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos,
      "real Lattice target panel should relay disabled policy-gate "
      "reservations and fingerprint audit without Marshal authority");
  check(result.find("\"artifact_deficit_keys\":[\"artifact:forecast_eval_"
                    "lineage\",\"artifact:forecast_eval_issue_baseline_fact_"
                    "digest_not_found\"") != std::string::npos,
        "real Lattice target panel should relay artifact deficit keys");
  check(result.find("\"fact_panel\":{\"source_tool\":\"hero.lattice."
                    "scan_facts\"") != std::string::npos &&
            result.find("\"fact_family\":\"forecast_eval\"") !=
                std::string::npos &&
            result.find("\"relation_declared_count\":3") != std::string::npos &&
            result.find("\"relation_bound_count\":2") != std::string::npos &&
            result.find("\"unresolved_relation_count\":1") !=
                std::string::npos &&
            result.find("\"relation_integrity_clean\":false") !=
                std::string::npos &&
            result.find("\"integrity_flags\":[\"unresolved_relation\"]") !=
                std::string::npos &&
            result.find("\"artifact_job:baseline_fact_digest_not_found\"") !=
                std::string::npos,
        "real Lattice fact panel should relay scanner-backed fact integrity");
  check(result.find("\"lineage_panel\":{\"source_tool\":\"hero.lattice."
                    "fact_lineage\"") != std::string::npos &&
            result.find("\"lineage_rows_are_audit_only\":true") !=
                std::string::npos &&
            result.find("\"cache_rows_used_for_target_satisfaction\":false") !=
                std::string::npos &&
            result.find("\"selected_relations\":[\"forecast_eval\"]") !=
                std::string::npos &&
            result.find("\"matching_row_count\":1") != std::string::npos &&
            result.find("\"lineage_rows\":[{\"relation\":\"forecast_eval\"") !=
                std::string::npos,
        "real Lattice fact panel should relay fact_lineage rows without target "
        "authority");
  check(result.find("\"artifact_readiness_proofable_families\":["
                    "\"forecast_eval\"]") != std::string::npos &&
            result.find("\"artifact_readiness_proof_kinds\":[\"forecast_eval_"
                        "artifact_bound\"]") != std::string::npos &&
            result.find("\"artifact_readiness_promotion_blocked_family_count\":"
                        "0") != std::string::npos,
        "real Lattice fact panel should relay catalog proof-template metadata");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_operational_report_summarizes_training_state() {
  const auto root = make_tmp_dir("operational_report");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  const auto mdn_jkimyei_path = root / "mdn.jkimyei";
  std::filesystem::create_directories(runtime_root);

  write_text(config_path,
             "[JKIMYEI]\n"
             "wikimyei_inference_expected_value_mdn_jkimyei_path = " +
                 mdn_jkimyei_path.string() + "\n");
  write_text(mdn_jkimyei_path, "TRAINING {\n"
                               "  GRAD_CLIP_NORM = 5.0;\n"
                               "};\n");

  const auto vicreg_dir =
      runtime_root /
      "real_train_core_vicreg_0_1600.train.channel_representation_vicreg";
  const auto mdn_dir =
      runtime_root /
      "real_train_core_channel_mdn_0_1600.train.channel_inference_mdn";
  const auto eval_dir =
      runtime_root /
      "real_validation_eval_channel_mdn_1800_2050.run.channel_inference_mdn";
  std::filesystem::create_directories(vicreg_dir);
  std::filesystem::create_directories(mdn_dir);
  std::filesystem::create_directories(eval_dir);

  const auto vicreg_report = vicreg_dir / "channel_representation.report";
  write_text(vicreg_dir / "job.manifest",
             "job_id=real_train_core_vicreg_0_1600.train.channel_"
             "representation_vicreg\n"
             "job_kind=channel_representation_vicreg\n"
             "component_family_id=wikimyei.representation.encoding.vicreg\n"
             "component_spawn_registry_id=test_spawn_registry\n"
             "component_spawn_id=vic\n"
             "component_spawn_label=wikimyei.representation.encoding."
             "vicreg#vic\n"
             "component_spawn_fingerprint=vicreg_spawn_fp\n"
             "wave_id=real_train_core_vicreg_0_1600\n"
             "target_component_family_id=wikimyei.representation.encoding."
             "vicreg\n"
             "wave_action=train\n"
             "wave_mode=train|debug\n"
             "source_range_policy=anchor_index\n"
             "source_order_policy=random\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=1600\n"
             "accepted_anchor_count=1600\n");
  write_text(
      vicreg_dir / "job.state",
      "job_id=real_train_core_vicreg_0_1600.train.channel_"
      "representation_vicreg\n"
      "job_kind=channel_representation_vicreg\n"
      "status=completed\n"
      "wave_id=real_train_core_vicreg_0_1600\n"
      "target_component_family_id=wikimyei.representation.encoding."
      "vicreg\n"
      "wave_action=train\n"
      "resolved_anchor_index_begin=0\n"
      "resolved_anchor_index_end=1600\n"
      "optimizer_steps=250\n"
      "last_loss=11.5173\n"
      "checkpoint_written=true\n"
      "checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "report_path=" +
          vicreg_report.string() + "\n");
  write_text(vicreg_report, "optimizer_steps=250\n"
                            "last_loss=11.5173\n"
                            "mean_loss=13.8741\n"
                            "representation_effective_rank_fraction=0.684131\n"
                            "representation_min_dimension_variance=0.135824\n"
                            "checkpoint_written=true\n");
  write_text(vicreg_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "optimizer_steps=250\n"
             "final_loss=11.5173\n"
             "mean_loss=13.8741\n"
             "representation_effective_rank_fraction=0.684131\n"
             "checkpoint_written=true\n");
  write_text(
      vicreg_dir / "runtime.checkpoint_io.fact",
      "fact_type=runtime.checkpoint_io.fact\n"
      "checkpoint_written=true\n"
      "checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n");
  write_text(vicreg_dir / "runtime.health_measurement.fact",
             "fact_type=runtime.health_measurement.fact\n"
             "representation_effective_rank_fraction=0.684131\n"
             "representation_min_dimension_variance=0.135824\n");

  const auto mdn_report = mdn_dir / "channel_inference.report";
  write_text(
      mdn_dir / "job.manifest",
      "job_id=real_train_core_channel_mdn_0_1600.train.channel_"
      "inference_mdn\n"
      "job_kind=channel_inference_mdn\n"
      "component_family_id=wikimyei.inference.expected_value.mdn\n"
      "component_spawn_registry_id=test_spawn_registry\n"
      "component_spawn_id=mdn\n"
      "component_spawn_label=wikimyei.inference.expected_value."
      "mdn#mdn\n"
      "component_spawn_fingerprint=mdn_spawn_fp\n"
      "wave_id=real_train_core_channel_mdn_0_1600\n"
      "target_component_family_id=wikimyei.inference.expected_value."
      "mdn\n"
      "wave_action=train\n"
      "wave_mode=train|debug\n"
      "source_range_policy=anchor_index\n"
      "source_order_policy=random\n"
      "resolved_anchor_index_begin=0\n"
      "resolved_anchor_index_end=1600\n"
      "accepted_anchor_count=1600\n"
      "input_representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n");
  write_text(
      mdn_dir / "job.state",
      "job_id=real_train_core_channel_mdn_0_1600.train.channel_"
      "inference_mdn\n"
      "job_kind=channel_inference_mdn\n"
      "status=completed\n"
      "wave_id=real_train_core_channel_mdn_0_1600\n"
      "target_component_family_id=wikimyei.inference.expected_value."
      "mdn\n"
      "wave_action=train\n"
      "resolved_anchor_index_begin=0\n"
      "resolved_anchor_index_end=1600\n"
      "optimizer_steps=250\n"
      "last_loss=2.10814\n"
      "checkpoint_written=true\n"
      "checkpoint_path=" +
          (mdn_dir / "channel_inference.report.channel_mdn.pt").string() +
          "\n"
          "report_path=" +
          mdn_report.string() + "\n");
  write_text(
      mdn_report,
      "config_path=" + config_path.string() +
          "\n"
          "optimizer_steps=250\n"
          "last_loss=2.10814\n"
          "mean_loss=30.9495\n"
          "mean_valid_target_fraction=0.830741\n"
          "mean_sigma_mean=3.40765\n"
          "mean_mixture_entropy=0.723886\n"
          "mean_mixture_usage=0.168799,0.398562,0.432639\n"
          "nonfinite_output_count=0\n"
          "max_grad_norm=35257.9\n"
          "representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "representation_checkpoint_loaded=true\n"
          "checkpoint_written=true\n");
  write_text(mdn_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "optimizer_steps=250\n"
             "final_loss=2.10814\n"
             "mean_loss=30.9495\n"
             "valid_target_fraction=0.910001\n"
             "sigma_mean=3.40765\n"
             "mixture_entropy=0.723886\n"
             "mixture_usage=0.168799,0.398562,0.432639\n"
             "nonfinite_output_count=0\n"
             "grad_norm_max_pre_clip=35257.9\n"
             "grad_clip_norm=5.0\n"
             "checkpoint_written=true\n");
  write_text(
      mdn_dir / "runtime.checkpoint_io.fact",
      "fact_type=runtime.checkpoint_io.fact\n"
      "checkpoint_written=true\n"
      "checkpoint_path=" +
          (mdn_dir / "channel_inference.report.channel_mdn.pt").string() +
          "\n"
          "representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "representation_checkpoint_loaded=true\n");
  write_text(mdn_dir / "runtime.health_measurement.fact",
             "fact_type=runtime.health_measurement.fact\n"
             "nonfinite_output_count=0\n"
             "grad_norm_max_pre_clip=35257.9\n"
             "grad_clip_norm=5.0\n"
             "sigma_mean=3.40765\n"
             "mixture_entropy=0.723886\n");

  const auto eval_report = eval_dir / "channel_inference.report";
  write_text(
      eval_dir / "job.manifest",
      "job_id=real_validation_eval_channel_mdn_1800_2050.run.channel_"
      "inference_mdn\n"
      "job_kind=channel_inference_mdn\n"
      "component_family_id=wikimyei.inference.expected_value.mdn\n"
      "component_spawn_registry_id=test_spawn_registry\n"
      "component_spawn_id=mdn\n"
      "component_spawn_label=wikimyei.inference.expected_value."
      "mdn#mdn\n"
      "component_spawn_fingerprint=mdn_spawn_fp\n"
      "wave_id=real_validation_eval_channel_mdn_1800_2050\n"
      "target_component_family_id=wikimyei.inference.expected_value."
      "mdn\n"
      "wave_action=run\n"
      "wave_mode=run|debug\n"
      "source_range_policy=anchor_index\n"
      "source_order_policy=sequential\n"
      "resolved_anchor_index_begin=1800\n"
      "resolved_anchor_index_end=2050\n"
      "accepted_anchor_count=250\n"
      "input_representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "input_mdn_checkpoint_path=" +
          (mdn_dir / "channel_inference.report.channel_mdn.pt").string() +
          "\n"
          "runtime_handoff_id=validation_eval_handoff\n"
          "runtime_handoff_digest=validation_eval_handoff_digest\n");
  write_text(eval_dir / "job.state",
             "job_id=real_validation_eval_channel_mdn_1800_2050.run.channel_"
             "inference_mdn\n"
             "job_kind=channel_inference_mdn\n"
             "status=completed\n"
             "wave_id=real_validation_eval_channel_mdn_1800_2050\n"
             "target_component_family_id=wikimyei.inference.expected_value."
             "mdn\n"
             "wave_action=run\n"
             "resolved_anchor_index_begin=1800\n"
             "resolved_anchor_index_end=2050\n"
             "optimizer_steps=0\n"
             "last_loss=2.23523\n"
             "checkpoint_written=false\n"
             "checkpoint_path=\n"
             "report_path=" +
                 eval_report.string() + "\n");
  write_text(
      eval_report,
      "config_path=" + config_path.string() +
          "\n"
          "optimizer_steps=0\n"
          "last_loss=2.23523\n"
          "mean_loss=2.24177\n"
          "mean_valid_target_fraction=0.779562\n"
          "mean_sigma_mean=2.58429\n"
          "mean_mixture_entropy=0.601403\n"
          "mean_mixture_usage=0.173429,0.504673,0.321898\n"
          "nonfinite_output_count=0\n"
          "representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "representation_checkpoint_loaded=true\n"
          "mdn_checkpoint_path=" +
          (mdn_dir / "channel_inference.report.channel_mdn.pt").string() +
          "\n"
          "mdn_checkpoint_loaded=true\n"
          "checkpoint_written=false\n");
  write_text(eval_dir / "runtime.result.fact",
             "fact_type=runtime.result.fact\n"
             "optimizer_steps=0\n"
             "final_loss=2.23523\n"
             "mean_loss=2.24177\n"
             "valid_target_fraction=0.779562\n"
             "sigma_mean=2.58429\n"
             "mixture_entropy=0.601403\n"
             "mixture_usage=0.173429,0.504673,0.321898\n"
             "nonfinite_output_count=0\n"
             "checkpoint_written=false\n");
  write_text(
      eval_dir / "runtime.checkpoint_io.fact",
      "fact_type=runtime.checkpoint_io.fact\n"
      "checkpoint_written=false\n"
      "representation_checkpoint_path=" +
          (vicreg_dir / "channel_representation.report.vicreg.pt").string() +
          "\n"
          "representation_checkpoint_loaded=true\n"
          "mdn_checkpoint_path=" +
          (mdn_dir / "channel_inference.report.channel_mdn.pt").string() +
          "\n"
          "mdn_checkpoint_loaded=true\n");
  write_text(eval_dir / "runtime.health_measurement.fact",
             "fact_type=runtime.health_measurement.fact\n"
             "nonfinite_output_count=0\n"
             "sigma_mean=2.58429\n"
             "mixture_entropy=0.601403\n");
  const auto replay_dir =
      eval_dir / "artifacts" / "kikijyeba.environment.replay.v1";
  write_text(replay_dir / "runtime_replay_batches.index",
             "schema=kikijyeba.environment.replay.runtime_batch_index.v1\n"
             "entry_count=1\n"
             "entry_0_wave_pulse_index=0\n"
             "entry_0_begin_anchor_index=1800\n"
             "entry_0_end_anchor_index=2050\n"
             "entry_0_anchor_count=250\n"
             "entry_0_batch_cursor_token=cursor_eval_replay\n"
             "entry_0_artifact_path_index_path=pulses/pulse_000000/"
             "runtime_replay_artifacts.index\n");
  write_text(replay_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=validation_eval_replay\n"
             "entry_0_environment_run_id=env_run_validation_eval_replay\n"
             "entry_0_policy_count=2\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n"
             "entry_0_report_path=validation_eval_replay.report\n");
  write_text(replay_dir / "validation_eval_replay.report",
             "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
             "artifact.v1\n"
             "experiment_id=validation_eval_replay\n"
             "replay_bundle_count=1\n"
             "attempted_count=2\n"
             "completed_count=2\n"
             "mean_total_reward=0.015\n"
             "mean_total_log_growth=0.018\n"
             "mean_final_equity_base=1.018\n"
             "policy_summary_count=2\n"
             "episode_count=2\n"
             "episode_0_requested_anchor_index_begin=10\n"
             "episode_0_requested_anchor_index_end=11\n"
             "episode_0_requested_source_key_begin=1000\n"
             "episode_0_requested_source_key_end=1001\n"
             "episode_0_accepted_cursor_kind=graph_anchor\n"
             "episode_0_accepted_cursor_scope=episode\n"
             "episode_0_accepted_batch_cursor_token=cursor_0\n"
             "episode_0_accepted_anchor_index_begin=10\n"
             "episode_0_accepted_anchor_index_end=11\n"
             "episode_0_accepted_anchor_keys=1000\n"
             "episode_1_requested_anchor_index_begin=20\n"
             "episode_1_requested_anchor_index_end=21\n"
             "episode_1_requested_source_key_begin=2000\n"
             "episode_1_requested_source_key_end=2001\n"
             "episode_1_accepted_cursor_kind=graph_anchor\n"
             "episode_1_accepted_cursor_scope=episode\n"
             "episode_1_accepted_batch_cursor_token=cursor_1\n"
             "episode_1_accepted_anchor_index_begin=20\n"
             "episode_1_accepted_anchor_index_end=21\n"
             "episode_1_accepted_anchor_keys=2000\n");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_operational_report_callback);
  const std::string args =
      "{\"subject\":\"run\",\"mode\":\"training_state\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"include_machine_payload\":true" + "}";
  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", args,
                                           &result, &error),
        "evaluate subject=run training_state mode should produce a report");
  check(g_fake_lattice_tool_name == "hero.lattice.evaluate_targets",
        "operational report should quote Lattice target statuses");
  check(result.find("\"read_only\":true") != std::string::npos,
        "operational report should be read-only");
  check(result.find("\"runtime_executor\":false") != std::string::npos,
        "operational report must not be a Runtime executor");
  check(result.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "operational report must not claim target satisfaction");
  check(result.find("\"target_proof\":false") != std::string::npos &&
            result.find("\"dispatchable\":false") != std::string::npos &&
            result.find("\"fact_families_are_not_target_kinds\":true") !=
                std::string::npos &&
            result.find("\"allocation_decision\":false") != std::string::npos &&
            result.find("\"market_readiness_decision\":false") !=
                std::string::npos &&
            result.find("\"deployment_decision\":false") != std::string::npos,
        "operational report should declare read-only non-proof and "
        "non-decision boundaries");
  check(result.find("\"execution_status\":\"completed_chain\"") !=
            std::string::npos,
        "operational report should detect the completed train/eval chain");
  check(result.find("\"proof_status\":\"clean\"") != std::string::npos,
        "satisfied Lattice statuses should render proof_status clean");
  check(result.find("\"validation_eval_mutated_model_state\":false") !=
            std::string::npos,
        "validation eval with zero optimizer steps should be non-mutating");
  check(result.find("\"operator_summary\":{") != std::string::npos,
        "operational report should expose the compact operator summary");
  check(result.find("\"stop_reason\":{") != std::string::npos &&
            result.find("\"runtime_panel\":{") != std::string::npos &&
            result.find("\"lattice_panel\":{") != std::string::npos &&
            result.find("\"audit_panel\":{") != std::string::npos,
        "operational report should expose the M19 compact panels");
  check(result.find("\"job_chain\":[") != std::string::npos,
        "operational report should expose compact chain rows by default");
  check(result.find("\"chain_summary\":[") != std::string::npos,
        "operational report should expose explicit chain summary rows");
  check(result.find("\"spawn_id\":\"mdn\"") != std::string::npos &&
            result.find("\"spawn_fingerprint\":\"mdn_spawn_fp\"") !=
                std::string::npos,
        "chain summary should expose component spawn identity");
  check(result.find("\"source_range\":{\"policy\":\"anchor_index\"") !=
            std::string::npos,
        "chain summary should expose source range policy");
  check(result.find("\"source_order\":\"sequential\"") != std::string::npos,
        "chain summary should expose Runtime source order");
  check(result.find("\"accepted_anchor_count\":250") != std::string::npos,
        "chain summary should expose accepted anchor count");
  check(
      result.find("\"handoff_identity\":{\"available\":true,"
                  "\"handoff_id\":\"validation_eval_handoff\"") !=
          std::string::npos,
      "chain summary should expose handoff identity when Runtime recorded it");
  check(result.find("\"runtime_replay_evidence_job_count\":1") !=
            std::string::npos,
        "operator report should count jobs with replay evidence");
  check(result.find("\"runtime_replay_experiment_report_available\":true") !=
            std::string::npos,
        "current state should surface replay report availability");
  check(
      result.find("\"replay_evidence\":{\"available\":true") !=
              std::string::npos &&
          result.find("\"latest_experiment_id\":\"validation_eval_replay\"") !=
              std::string::npos &&
          result.find("\"completed_count\":2") != std::string::npos &&
          result.find("\"report_mean_total_log_growth\":0.018") !=
              std::string::npos,
      "chain summary should expose replay experiment evidence");
  check(result.find("inspect_runtime_replay_evidence") != std::string::npos,
        "operator report should offer replay evidence inspection");
  check(result.find("\"runtime_result_fact\":true") != std::string::npos &&
            result.find("\"runtime_checkpoint_io_fact\":true") !=
                std::string::npos &&
            result.find("\"runtime_health_measurement_fact\":true") !=
                std::string::npos,
        "chain summary should expose terminal Runtime fact availability");
  check(result.find("\"machine_payload\":{") != std::string::npos,
        "include_machine_payload should expose full audit details");
  check(result.find("\"channel_mdn_validation_eval_ready\":{\"status\":"
                    "\"satisfied\"") != std::string::npos,
        "target statuses should include validation eval readiness");
  check(result.find("\"role\":\"vicreg_train\"") != std::string::npos &&
            result.find("\"role\":\"mdn_train\"") != std::string::npos &&
            result.find("\"role\":\"mdn_validation_eval\"") !=
                std::string::npos,
        "operator report should include all three chain jobs");
  check(result.find("\"optimizer_steps\":250") != std::string::npos,
        "operator report should expose train optimizer steps");
  check(result.find("\"optimizer_steps\":0") != std::string::npos,
        "operator report should expose eval optimizer steps");
  check(result.find("\"mean_valid_target_fraction\":0.910001") !=
            std::string::npos,
        "operator report should prefer Runtime terminal result facts over raw "
        "reports");
  check(result.find("\"id\":\"high_pre_clip_grad_norm\"") != std::string::npos,
        "operator report should flag the MDN grad norm watch item");
  check(result.find("\"clip_norm\":5.0") != std::string::npos,
        "operator report should include the training clip norm");
  check(result.find("inspect_runtime_terminal_result_facts") !=
            std::string::npos,
        "operator report should switch to terminal fact inspection once facts "
        "exist");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_operational_report_quotes_target_blockers() {
  const auto root = make_tmp_dir("operational_report_target_blockers");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  std::filesystem::create_directories(runtime_root);
  write_text(config_path, "[HERO]\n");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_operational_report_blocker_callback);
  const std::string args =
      "{\"subject\":\"run\",\"mode\":\"training_state\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"target_ids\":[\"vicreg_train_core_ready\","
      "\"channel_mdn_train_core_ready\","
      "\"channel_mdn_validation_eval_ready\","
      "\"forecast_eval_artifact_ready\"]}";

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", args,
                                           &result, &error),
        "evaluate subject=run should quote Lattice target blockers");
  check(g_fake_lattice_tool_name == "hero.lattice.evaluate_targets",
        "target blocker view should come from Lattice target evaluation");
  check(result.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "target blocker view must not become Marshal proof authority");
  check(result.find("\"target_blockers\":[") != std::string::npos,
        "operational report should include the M17 target-blocker panel");
  check(result.find("\"operator_summary\":{") != std::string::npos,
        "default evaluate output should include an operator summary");
  check(result.find("\"job_chain\":[") != std::string::npos,
        "default evaluate output should include compact job-chain rows");
  check(result.find("\"latest_jobs\"") == std::string::npos,
        "default evaluate output should keep full job rows out of the "
        "operator packet");
  check(result.find("\"machine_payload\"") == std::string::npos,
        "default evaluate output should not include the full audit payload");
  check(result.find("\"target_id\":\"channel_mdn_validation_eval_ready\","
                    "\"status\":\"metric_failed\"") != std::string::npos,
        "target blocker panel should include the unsatisfied target");
  check(result.find("\"blockers\":[\"target_not_satisfied\","
                    "\"deficit:coverage:evaluation_metric\","
                    "\"warning:validation_eval_missing\"]") !=
            std::string::npos,
        "target blocker panel should quote Lattice deficits and warnings");
  check(result.find("\"next_safe_action\":\"prepare\"") != std::string::npos,
        "unsatisfied target with deficits should point back to the bounded "
        "target driver");
  check(result.find("\"target_id\":\"forecast_eval_artifact_ready\","
                    "\"status\":\"blocked\","
                    "\"target_class\":\"artifact_readiness\"") !=
            std::string::npos,
        "target blocker panel should preserve artifact-readiness identity");
  check(result.find("\"kind\":\"not_applicable\"") != std::string::npos &&
            result.find("\"target_kind_applicable\":false") !=
                std::string::npos &&
            result.find("\"target_kind_effective\":\"none\"") !=
                std::string::npos,
        "target blocker panel should preserve artifact target-kind "
        "non-applicability");
  check(result.find("\"blockers\":[\"target_not_satisfied\","
                    "\"artifact_readiness_evidence_panel\","
                    "\"artifact_lineage_unbound\","
                    "\"artifact_authority_drift\","
                    "\"artifact_issue:forecast_eval_must_remain_artifact_"
                    "evidence_only\","
                    "\"artifact_issue:baseline_fact_digest_not_found\","
                    "\"fact_integrity_issue:mdn:baseline_fact_digest_not_"
                    "found\","
                    "\"deficit:artifact:forecast_eval_authority\","
                    "\"deficit:artifact:forecast_eval_lineage\","
                    "\"deficit:artifact:forecast_eval_issue_forecast_eval_"
                    "must_remain_artifact_evidence_only\","
                    "\"deficit:artifact:forecast_eval_issue_baseline_fact_"
                    "digest_not_found\"]") != std::string::npos,
        "artifact blocker panel should preserve artifact proof details");
  check(result.find("\"target_id\":\"forecast_eval_artifact_ready\","
                    "\"status\":\"blocked\"") == std::string::npos ||
            result.find("\"proof_certificate_check_failed\","
                        "\"artifact_readiness_evidence_panel\"") ==
                std::string::npos,
        "artifact proof details should suppress the generic certificate "
        "blocker");
  check(
      result.find("\"artifact_failed_proof_count\":1") != std::string::npos &&
          result.find("\"artifact_lineage_unbound_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_authority_drift_proof_count\":1") !=
              std::string::npos &&
          result.find("\"artifact_fact_families\":[\"forecast_eval\"]") !=
              std::string::npos &&
          result.find("\"artifact_proof_kinds\":[\"forecast_eval_artifact_"
                      "bound\"]") != std::string::npos &&
          result.find("forecast evaluation artifact existence, checkpoint "
                      "lineage") != std::string::npos &&
          result.find("\"artifact_integrity_flags\":[\"lineage_unbound\"]") !=
              std::string::npos &&
          result.find("\"artifact_authority_flags\":[\"quality_authority\"]") !=
              std::string::npos &&
          result.find("\"artifact_related_fact_integrity_issue_codes\":["
                      "\"mdn:baseline_fact_digest_not_found\"]") !=
              std::string::npos &&
          result.find("\"artifact_deficit_keys\":[\"artifact:forecast_eval_"
                      "authority\",\"artifact:forecast_eval_lineage\","
                      "\"artifact:forecast_eval_issue_forecast_eval_must_"
                      "remain_artifact_evidence_only\","
                      "\"artifact:forecast_eval_issue_baseline_fact_digest_not_"
                      "found\"]") != std::string::npos &&
          result.find("\"primary_deficit_key\":\"artifact:forecast_eval_"
                      "authority\"") != std::string::npos,
      "artifact blocker panel should expose failed artifact proof summary");
  check(
      result.find("\"artifact_authority_denial_flags\":[\"target_dependency_"
                  "authority\",\"runtime_wave_authority\","
                  "\"marshal_reachability\",\"checkpoint_source_authority\","
                  "\"plan_checkpoint_input_authority\"]") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority_count\":0") !=
              std::string::npos &&
          result.find("\"artifact_target_dependency_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_runtime_wave_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_marshal_reachability\":false") !=
              std::string::npos &&
          result.find("\"artifact_checkpoint_source_authority\":false") !=
              std::string::npos &&
          result.find("\"artifact_plan_checkpoint_input_authority\":false") !=
              std::string::npos,
      "artifact blocker panel should carry Lattice boundary denials through "
      "the run-level summary");
  check(
      result.find("\"policy_gate_reservation_count\":1") != std::string::npos &&
          result.find("\"enabled_policy_gate_count\":0") != std::string::npos &&
          result.find("\"disabled_policy_gate_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_verified_count\":1") !=
              std::string::npos &&
          result.find("\"policy_gate_policy_fingerprint_mismatch_count\":0") !=
              std::string::npos &&
          result.find(
              "\"policy_gate_all_policy_fingerprints_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_fingerprint_verified\":true") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_certificate_authority_count\":0") !=
              std::string::npos &&
          result.find("\"policy_gate_dispatch_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_target_status_authority\":false") !=
              std::string::npos &&
          result.find("\"policy_gate_proof_authority\":false") !=
              std::string::npos &&
          result.find(
              "\"policy_id\":\"forecast_quality_acceptance_reserved\"") !=
              std::string::npos,
      "run-level target blockers should preserve disabled policy-gate "
      "reservations and fingerprint audit without Marshal status, proof, or "
      "dispatch authority");
  check(result.find("\"next_safe_action\":\"inspect\"") != std::string::npos,
        "artifact target blockers should route to the evidence panel");
  check(result.find("\"next_safe_actions\":[\"inspect\"") != std::string::npos,
        "run-level next safe actions should prioritize artifact evidence "
        "inspection");
  check(result.find("\"proof_certificate_issues\":[\"closure_unresolved\"]") !=
            std::string::npos,
        "target blocker panel should quote proof-certificate issues");
  check(result.find("\"next_safe_action\":\"inspect_lattice_certificate_"
                    "failure\"") != std::string::npos,
        "proof check failures should be routed to certificate inspection");

  marshal::set_marshal_lattice_tool_callback(nullptr);
  std::filesystem::remove_all(root);
}

void test_evaluate_deterministic_subjects() {
  const auto root = make_tmp_dir("evaluate_deterministic_subjects");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  const auto vicreg_dir =
      runtime_root / "eval_subject_vicreg.train.channel_representation_vicreg";
  const auto mdn_dir =
      runtime_root / "eval_subject_mdn.train.channel_inference_mdn";
  std::filesystem::create_directories(vicreg_dir);
  std::filesystem::create_directories(mdn_dir);
  write_text(config_path, "[HERO]\n");
  write_text(
      vicreg_dir / "job.manifest",
      "job_id=eval_subject_vicreg.train.channel_representation_vicreg\n");
  write_text(vicreg_dir / "job.state",
             "job_id=eval_subject_vicreg.train.channel_representation_vicreg\n"
             "job_kind=channel_representation_vicreg\n"
             "status=completed\n"
             "component_family_id=wikimyei.representation.encoding.vicreg\n"
             "component_spawn_registry_id=test_registry\n"
             "component_spawn_id=vic\n"
             "component_spawn_label=vicreg_main\n"
             "component_spawn_fingerprint=vic_fp\n"
             "target_component_family_id=wikimyei.representation.encoding."
             "vicreg\n"
             "wave_action=train\n"
             "wave_mode=train|debug\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=1600\n"
             "accepted_anchor_count=1600\n"
             "checkpoint_written=true\n"
             "checkpoint_path=/tmp/vicreg.pt\n"
             "protocol_contract_fingerprint=pc\n"
             "graph_order_fingerprint=go\n"
             "source_cursor_token=cursor\n"
             "vicreg_assembly_fingerprint=vic\n"
             "mdn_assembly_fingerprint=mdn\n");
  write_text(mdn_dir / "job.manifest",
             "job_id=eval_subject_mdn.train.channel_inference_mdn\n");
  write_text(mdn_dir / "job.state",
             "job_id=eval_subject_mdn.train.channel_inference_mdn\n"
             "job_kind=channel_inference_mdn\n"
             "status=completed\n"
             "component_family_id=wikimyei.inference.expected_value.mdn\n"
             "component_spawn_registry_id=test_registry\n"
             "component_spawn_id=mdn\n"
             "component_spawn_label=mdn_main\n"
             "component_spawn_fingerprint=mdn_fp\n"
             "target_component_family_id=wikimyei.inference.expected_value."
             "mdn\n"
             "wave_action=train\n"
             "wave_mode=train|debug\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=1600\n"
             "accepted_anchor_count=1600\n"
             "checkpoint_written=true\n"
             "checkpoint_path=/tmp/mdn.pt\n"
             "protocol_contract_fingerprint=pc\n"
             "graph_order_fingerprint=go\n"
             "source_cursor_token=cursor\n"
             "vicreg_assembly_fingerprint=vic\n"
             "mdn_assembly_fingerprint=mdn\n");
  write_text(mdn_dir / "runtime.result.fact", "fact_type=runtime.result.fact\n"
                                              "status=completed\n"
                                              "optimizer_steps=10\n"
                                              "checkpoint_written=true\n"
                                              "final_loss=2.5\n");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_target_deficit_callback);
  std::string result;
  std::string error;
  const std::string target_args =
      "{\"subject\":\"target\",\"target_id\":\"lookup_target\","
      "\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"include_machine_payload\":true}";
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", target_args,
                                           &result, &error),
        "evaluate subject=target should query Lattice target deficit");
  check(g_fake_lattice_tool_name == "hero.lattice.target_deficit",
        "target subject should use Lattice target_deficit");
  check(result.find("\"subject\":\"target\"") != std::string::npos &&
            result.find("\"target_panel\":{") != std::string::npos,
        "target subject should return a target panel");
  check(result.find("\"target_status\":\"metric_failed\"") != std::string::npos,
        "target subject should expose Lattice target status");
  check(
      result.find("\"target_status_source\":\"hero.lattice.target_deficit\"") !=
          std::string::npos,
      "target subject should mark the Lattice status source");
  check(result.find("\"proof_authority\":\"lattice\"") != std::string::npos,
        "target subject should name Lattice as proof authority");
  check(result.find("\"certificate_status\":\"available\"") !=
            std::string::npos,
        "target subject should report certificate availability from Lattice "
        "payload");
  check(result.find("\"proof_certificate_present\":true") != std::string::npos,
        "target subject should expose proof certificate presence");
  check(result.find("\"next_safe_action\":\"prepare\"") != std::string::npos,
        "target subject should point blocked planned targets at the driver");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_satisfied_without_certificate_callback);
  const std::string satisfied_no_certificate_args =
      "{\"subject\":\"target\",\"target_id\":\"satisfied_without_certificate\","
      "\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      "}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           satisfied_no_certificate_args,
                                           &result, &error),
        "satisfied target without certificate metadata should still report");
  check(result.find("\"target_status\":\"satisfied\"") != std::string::npos,
        "satisfied target status should be quoted from Lattice");
  check(result.find("\"certificate_status\":\"unavailable\"") !=
            std::string::npos,
        "satisfied target without proof material should not claim a "
        "certificate");
  check(result.find("\"next_safe_action\":\"inspect_lattice_status\"") !=
            std::string::npos,
        "satisfied target without certificate should route to status "
        "inspection, not certificate inspection");
  check(result.find("\"next_safe_action\":\"inspect_lattice_certificate\"") ==
            std::string::npos,
        "Marshal should not imply certificate inspection without Lattice "
        "certificate material");

  marshal::set_marshal_lattice_tool_callback(
      fake_lattice_malformed_target_deficit_callback);
  result.clear();
  error.clear();
  check(!marshal::execute_marshal_tool_json("hero.marshal.inspect", target_args,
                                            &result, &error),
        "malformed Lattice target_deficit payload should fail closed");
  check(error.find("expected JSON object") != std::string::npos,
        "malformed target_deficit failure should explain JSON object shape");
  marshal::set_marshal_lattice_tool_callback(nullptr);

  const std::string protocol_args =
      "{\"subject\":\"protocol\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"protocol_contract_fingerprint\":\"pc\","
      "\"graph_order_fingerprint\":\"go\","
      "\"source_cursor_token\":\"cursor\","
      "\"vicreg_assembly_fingerprint\":\"vic\","
      "\"mdn_assembly_fingerprint\":\"mdn\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           protocol_args, &result, &error),
        "evaluate subject=protocol should inspect identity evidence");
  check(result.find("\"subject\":\"protocol\"") != std::string::npos &&
            result.find("\"issue_count\":0") != std::string::npos,
        "protocol subject should report matched identity evidence");
  check(result.find("\"status\":\"matched\"") != std::string::npos,
        "protocol subject should mark expected observed fields as matched");
  check(result.find("\"expectation_policy\":\"observed_ok_report\"") !=
            std::string::npos,
        "protocol subject should default to report semantics");
  check(result.find("\"identity_verified\":false") != std::string::npos,
        "report-mode protocol output should not claim strict identity "
        "verification");
  check(result.find("\"evidence_scope\":\"runtime_jobs_only\"") !=
            std::string::npos,
        "protocol subject should declare its Runtime-only evidence scope");
  check(result.find("\"machine_payload\"") == std::string::npos,
        "protocol subject should omit machine payload by default");

  const std::string protocol_strict_args =
      "{\"subject\":\"protocol\",\"identity_mode\":\"strict\","
      "\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"protocol_contract_fingerprint\":\"pc\","
      "\"graph_order_fingerprint\":\"go\","
      "\"source_cursor_token\":\"cursor\","
      "\"vicreg_assembly_fingerprint\":\"vic\","
      "\"mdn_assembly_fingerprint\":\"mdn\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", protocol_strict_args, &result, &error),
        "protocol strict mode should accept complete matching expected "
        "identity");
  check(result.find("\"expectation_policy\":\"strict_expected_identity\"") !=
            std::string::npos,
        "strict protocol subject should declare strict expectation policy");
  check(result.find("\"identity_verified\":true") != std::string::npos,
        "strict protocol subject should verify complete matching identity");

  const std::string protocol_observed_only_args =
      "{\"subject\":\"protocol\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      "}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           protocol_observed_only_args, &result,
                                           &error),
        "protocol report mode should allow observed-only identity evidence");
  check(result.find("\"issue_count\":0") != std::string::npos,
        "observed-only report mode should not invent a mismatch");
  check(result.find("\"status\":\"observed\"") != std::string::npos,
        "observed-only report mode should distinguish observed from matched");
  check(result.find("\"identity_verified\":false") != std::string::npos,
        "observed-only protocol report must not claim strict verification");

  result.clear();
  error.clear();
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.inspect",
            "{\"subject\":\"protocol\",\"identity_mode\":\"strict\","
            "\"runtime_root\":" +
                marshal::detail::json_quote(runtime_root.string()) + "}",
            &result, &error),
        "strict protocol mode should require explicit expected identity");
  check(error.find("identity_mode=strict requires expected identity fields") !=
            std::string::npos,
        "strict protocol missing expectations should fail clearly");

  const std::string protocol_mismatch_args =
      "{\"subject\":\"protocol\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"protocol_contract_fingerprint\":\"wrong\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", protocol_mismatch_args, &result, &error),
        "protocol mismatch should return a report with issues");
  check(result.find("identity_mismatch:protocol_contract_fingerprint") !=
            std::string::npos,
        "protocol mismatch should be surfaced as an identity issue");

  const std::string spawn_args =
      "{\"subject\":\"spawn\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"spawn_id\":\"mdn\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", spawn_args,
                                           &result, &error),
        "evaluate subject=spawn should inspect spawn evidence");
  check(result.find("\"subject\":\"spawn\"") != std::string::npos &&
            result.find("\"match_count\":1") != std::string::npos &&
            result.find("\"spawn_fingerprints\":[\"mdn_fp\"]") !=
                std::string::npos,
        "spawn subject should match the requested spawn");
  check(result.find("\"evidence_scope\":\"runtime_jobs_only\"") !=
                std::string::npos &&
            result.find("\"proof_authority\":\"none\"") != std::string::npos &&
            result.find("\"marshal_role\":\"evidence_grouping\"") !=
                std::string::npos,
        "spawn subject should declare non-proof Runtime grouping semantics");

  const std::string spawn_fingerprint_args =
      "{\"subject\":\"spawn\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"component_spawn_fingerprint\":\"mdn_fp\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", spawn_fingerprint_args, &result, &error),
        "spawn subject should support fingerprint-only lookup");
  check(result.find("\"query_match_count\":1") != std::string::npos,
        "spawn fingerprint lookup should report query match count");

  const std::string missing_spawn_args =
      "{\"subject\":\"spawn\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"spawn_id\":\"missing_spawn\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           missing_spawn_args, &result, &error),
        "spawn no-match should produce a non-authoritative observation packet");
  check(result.find("\"query_match_count\":0") != std::string::npos &&
            result.find("missing_spawn_evidence") != std::string::npos,
        "spawn no-match should be explicit without becoming proof");

  const std::string component_args =
      "{\"subject\":\"component\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"component_family_id\":\"wikimyei.inference.expected_value.mdn\"}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           component_args, &result, &error),
        "evaluate subject=component should inspect component evidence");
  check(result.find("\"subject\":\"component\"") != std::string::npos &&
            result.find("\"component_family_id\":\"wikimyei.inference."
                        "expected_value.mdn\"") != std::string::npos &&
            result.find("\"spawn_ids\":[\"mdn\"]") != std::string::npos,
        "component subject should group matching component jobs by spawn");
  check(result.find("\"evidence_scope\":\"runtime_jobs_only\"") !=
                std::string::npos &&
            result.find("\"proof_authority\":\"none\"") != std::string::npos &&
            result.find("\"marshal_role\":\"evidence_grouping\"") !=
                std::string::npos,
        "component subject should declare non-proof Runtime grouping "
        "semantics");

  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", R"({"subject":"target"})", &result, &error),
        "target subject should require target_id");
  check(error.find("subject=target requires target_id") != std::string::npos,
        "missing target_id should fail explicitly");
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.inspect", R"({"subject":"spawn"})", &result, &error),
        "spawn subject should require spawn identity");
  check(error.find("subject=spawn requires") != std::string::npos,
        "missing spawn identity should fail explicitly");
  check(!marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                            R"({"subject":"component"})",
                                            &result, &error),
        "component subject should require component identity");
  check(error.find("subject=component requires") != std::string::npos,
        "missing component identity should fail explicitly");
  check(
      !marshal::execute_marshal_tool_json(
          "hero.marshal.inspect",
          R"({"subject":"target","target_id":"lookup_target","baseline_job_id":"x"})",
          &result, &error),
      "target subject should reject run-only fields");
  check(error.find("subject=target unknown field: baseline_job_id") !=
            std::string::npos,
        "target subject run-only field rejection should be explicit");
  check(!marshal::execute_marshal_tool_json(
            "hero.marshal.inspect",
            R"({"subject":"protocol","spawn_id":"mdn"})", &result, &error),
        "protocol subject should reject spawn-only fields");
  check(error.find("subject=protocol unknown field: spawn_id") !=
            std::string::npos,
        "protocol subject spawn-only field rejection should be explicit");
  check(
      !marshal::execute_marshal_tool_json(
          "hero.marshal.inspect",
          R"({"subject":"spawn","target_id":"lookup_target","spawn_id":"mdn"})",
          &result, &error),
      "spawn subject should reject target-only fields");
  check(error.find("subject=spawn unknown field: target_id") !=
            std::string::npos,
        "spawn subject target-only field rejection should be explicit");
  check(
      !marshal::execute_marshal_tool_json(
          "hero.marshal.inspect",
          R"({"subject":"component","component_family_id":"x","protocol_contract_fingerprint":"pc"})",
          &result, &error),
      "component subject should reject protocol-only fields");
  check(error.find("subject=component unknown field: "
                   "protocol_contract_fingerprint") != std::string::npos,
        "component subject protocol-only field rejection should be explicit");

  std::filesystem::remove_all(root);
}

void test_compare_runs_reports_descriptive_deltas() {
  const auto root = make_tmp_dir("compare_runs");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  std::filesystem::create_directories(runtime_root);
  write_text(config_path, "[HERO]\n");

  const auto baseline_dir =
      runtime_root / "baseline_validation.run.channel_inference_mdn";
  const auto candidate_dir =
      runtime_root / "candidate_validation.run.channel_inference_mdn";
  const auto baseline_report = baseline_dir / "channel_inference.report";
  const auto candidate_report = candidate_dir / "channel_inference.report";
  const auto rep_checkpoint = runtime_root / "rep.pt";
  const auto baseline_mdn_checkpoint = runtime_root / "baseline_mdn.pt";
  const auto candidate_mdn_checkpoint = runtime_root / "candidate_mdn.pt";

  auto write_run = [&](const std::filesystem::path &job_dir,
                       const std::filesystem::path &report_path,
                       const std::string &job_id, const std::string &loss,
                       const std::string &mean_loss,
                       const std::string &valid_fraction,
                       const std::string &grad_norm,
                       const std::string &finite_parameter_check,
                       const std::string &config_bundle_id,
                       const std::string &config_receipt_id,
                       const std::string &mdn_checkpoint) {
    write_text(job_dir / "job.manifest",
               "job_id=" + job_id +
                   "\n"
                   "job_kind=channel_inference_mdn\n"
                   "config_path=" +
                   config_path.string() +
                   "\n"
                   "config_bundle_id=" +
                   config_bundle_id +
                   "\n"
                   "config_receipt_id=" +
                   config_receipt_id +
                   "\n"
                   "protocol_contract_fingerprint=contract_compare\n"
                   "graph_order_fingerprint=graph_compare\n"
                   "source_cursor_token=cursor_compare\n"
                   "wave_id=validation_eval_mdn\n"
                   "target_component_family_id=wikimyei.inference.expected_"
                   "value.mdn\n"
                   "wave_action=run\n"
                   "wave_mode=run|debug\n"
                   "source_range_policy=anchor_index\n"
                   "source_order_policy=sequential\n"
                   "resolved_anchor_index_begin=1800\n"
                   "resolved_anchor_index_end=2050\n"
                   "accepted_anchor_count=250\n");
    write_text(job_dir / "job.state",
               "job_id=" + job_id +
                   "\n"
                   "job_kind=channel_inference_mdn\n"
                   "status=completed\n"
                   "wave_id=validation_eval_mdn\n"
                   "target_component_family_id=wikimyei.inference.expected_"
                   "value.mdn\n"
                   "wave_action=run\n"
                   "resolved_anchor_index_begin=1800\n"
                   "resolved_anchor_index_end=2050\n"
                   "optimizer_steps=0\n"
                   "last_loss=" +
                   loss +
                   "\n"
                   "checkpoint_written=false\n"
                   "checkpoint_path=\n"
                   "report_path=" +
                   report_path.string() + "\n");
    write_text(report_path,
               "optimizer_steps=0\n"
               "last_loss=" +
                   loss +
                   "\n"
                   "mean_loss=" +
                   mean_loss +
                   "\n"
                   "mean_valid_target_fraction=" +
                   valid_fraction +
                   "\n"
                   "total_valid_target_count=7000\n"
                   "mean_sigma_mean=2.5\n"
                   "mean_sigma_mean_valid=2.4\n"
                   "min_sigma_min=0.01\n"
                   "max_sigma_max=6.0\n"
                   "mean_mixture_entropy=0.60\n"
                   "mean_mixture_usage=0.2,0.5,0.3\n"
                   "mean_nll_per_channel=2.1,2.4\n"
                   "mean_nll_per_target_feature=1.8,2.7,2.2\n"
                   "mean_nll_per_channel_target_feature=1.8,2.0,2.4,2.5,2."
                   "6,2.7\n"
                   "valid_target_count_per_channel=3500,3500\n"
                   "valid_target_count_per_target_feature=2400,2300,2300\n"
                   "valid_target_count_per_channel_target_feature=1200,1150,"
                   "1150,1200,1150,1150\n"
                   "nonfinite_output_count=0\n"
                   "finite_parameter_check=" +
                   finite_parameter_check +
                   "\n"
                   "max_grad_norm=" +
                   grad_norm +
                   "\n"
                   "representation_checkpoint_path=" +
                   rep_checkpoint.string() +
                   "\n"
                   "representation_checkpoint_loaded=true\n"
                   "mdn_checkpoint_path=" +
                   mdn_checkpoint +
                   "\n"
                   "mdn_checkpoint_loaded=true\n"
                   "checkpoint_written=false\n");
    write_text(job_dir / "runtime.result.fact",
               "fact_type=runtime.result.fact\n"
               "optimizer_steps=0\n"
               "final_loss=" +
                   loss +
                   "\n"
                   "mean_loss=" +
                   mean_loss +
                   "\n"
                   "valid_target_fraction=" +
                   valid_fraction +
                   "\n"
                   "total_valid_target_count=7000\n"
                   "sigma_mean=2.5\n"
                   "sigma_mean_valid=2.4\n"
                   "mixture_entropy=0.60\n"
                   "mixture_usage=0.2,0.5,0.3\n"
                   "mean_nll_per_channel=2.1,2.4\n"
                   "mean_nll_per_target_feature=1.8,2.7,2.2\n"
                   "mean_nll_per_channel_target_feature=1.8,2.0,2.4,2.5,2."
                   "6,2.7\n"
                   "valid_target_count_per_channel=3500,3500\n"
                   "valid_target_count_per_target_feature=2400,2300,2300\n"
                   "valid_target_count_per_channel_target_feature=1200,1150,"
                   "1150,1200,1150,1150\n"
                   "nonfinite_output_count=0\n"
                   "finite_parameter_check=" +
                   finite_parameter_check +
                   "\n"
                   "grad_norm_max_pre_clip=" +
                   grad_norm +
                   "\n"
                   "grad_clip_norm=5.0\n"
                   "checkpoint_written=false\n");
    write_text(job_dir / "runtime.checkpoint_io.fact",
               "fact_type=runtime.checkpoint_io.fact\n"
               "checkpoint_written=false\n"
               "representation_checkpoint_path=" +
                   rep_checkpoint.string() +
                   "\n"
                   "representation_checkpoint_loaded=true\n"
                   "mdn_checkpoint_path=" +
                   mdn_checkpoint +
                   "\n"
                   "mdn_checkpoint_loaded=true\n");
    write_text(job_dir / "runtime.health_measurement.fact",
               "fact_type=runtime.health_measurement.fact\n"
               "nonfinite_output_count=0\n"
               "finite_parameter_check=" +
                   finite_parameter_check +
                   "\n"
                   "grad_norm_max_pre_clip=" +
                   grad_norm +
                   "\n"
                   "grad_clip_norm=5.0\n"
                   "sigma_mean=2.5\n"
                   "mixture_entropy=0.60\n");
  };

  write_run(baseline_dir, baseline_report,
            "baseline_validation.run.channel_inference_mdn", "2.50", "2.60",
            "0.70", "800.0", "", "bundle_baseline", "receipt_baseline",
            baseline_mdn_checkpoint.string());
  write_run(candidate_dir, candidate_report,
            "candidate_validation.run.channel_inference_mdn", "2.10", "2.20",
            "0.80", "50.0", "1", "bundle_candidate", "receipt_candidate",
            candidate_mdn_checkpoint.string());

  const std::string args =
      "{\"subject\":\"run\",\"mode\":\"compare\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"baseline_job_id\":\"baseline_validation.run.channel_inference_mdn\""
      ",\"candidate_job_id\":\"candidate_validation.run.channel_inference_mdn\""
      ",\"include_machine_payload\":true}";
  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect", args,
                                           &result, &error),
        "evaluate subject=run compare mode should produce a read-only "
        "comparison");
  check(result.find("\"read_only\":true") != std::string::npos,
        "compare_runs should be read-only");
  check(result.find("\"target_satisfaction_claimed\":false") !=
            std::string::npos,
        "compare_runs must not claim target satisfaction");
  check(result.find("\"model_selector\":false") != std::string::npos &&
            result.find("\"checkpoint_selected\":false") != std::string::npos,
        "compare_runs must not select models or checkpoints");
  check(result.find("\"comparable\":true") != std::string::npos,
        "matching validation jobs should be comparable");
  check(result.find("\"operator_summary\":{") != std::string::npos,
        "compare output should include compact operator summary");
  check(result.find("\"stop_reason\":{") != std::string::npos &&
            result.find("\"runtime_panel\":{") != std::string::npos &&
            result.find("\"lattice_panel\":{") != std::string::npos &&
            result.find("\"audit_panel\":{") != std::string::npos,
        "compare output should expose the M19 compact panels");
  check(result.find("\"run_pair\":{") != std::string::npos,
        "compare output should include compact run pair by default");
  check(result.find("\"config_identity_deltas\":{") != std::string::npos,
        "compare_runs should expose config identity deltas");
  check(result.find("\"config_bundle_id\":{\"baseline\":\"bundle_baseline\","
                    "\"candidate\":\"bundle_candidate\",\"changed\":true}") !=
            std::string::npos,
        "compare_runs should report changed config bundle identity");
  check(result.find("\"config_receipt_id\":{\"baseline\":\"receipt_baseline\","
                    "\"candidate\":\"receipt_candidate\",\"changed\":true}") !=
            std::string::npos,
        "compare_runs should report changed config receipt identity");
  check(result.find("\"wave_range_deltas\":{") != std::string::npos,
        "compare_runs should expose wave/range deltas");
  check(result.find("\"source_range_policy\":{\"baseline\":\"anchor_index\","
                    "\"candidate\":\"anchor_index\",\"changed\":false}") !=
            std::string::npos,
        "compare_runs should report unchanged source-range policy");
  check(result.find("\"anchor_index_begin\":{\"baseline\":1800,\"candidate\":"
                    "1800,\"delta\":0}") != std::string::npos,
        "compare_runs should report comparable anchor begin");
  check(result.find("\"mean_loss\":{\"baseline\":2.60,\"candidate\":2.20") !=
            std::string::npos,
        "compare_runs should expose mean loss deltas");
  check(result.find("\"per_channel\":[2.1,2.4]") != std::string::npos,
        "compare_runs should expose per-channel NLL visibility");
  check(result.find("\"per_target_feature\":[1.8,2.7,2.2]") !=
            std::string::npos,
        "compare_runs should expose per-target-feature NLL visibility");
  check(result.find("\"finite_parameter_check\":true") != std::string::npos,
        "compare_runs should expose finite parameter health");
  check(result.find("\"finite_parameter_check\":null") != std::string::npos,
        "compare_runs should not turn missing finite parameter evidence into "
        "false");
  check(result.find("\"mdn_checkpoint_changed\":true") != std::string::npos,
        "compare_runs should report checkpoint lineage deltas");
  check(result.find("\"cleared\":[\"high_pre_clip_grad_norm\"]") !=
            std::string::npos,
        "compare_runs should report warning deltas");
  check(result.find("Compare does not choose. Compare explains.") !=
            std::string::npos,
        "compare_runs should preserve the descriptive invariant");

  const std::string evaluate_args =
      "{\"subject\":\"run\",\"mode\":\"compare\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"baseline_job_id\":\"baseline_validation.run.channel_inference_mdn\""
      ",\"candidate_job_id\":\"candidate_validation.run.channel_inference_mdn\""
      ",\"include_machine_payload\":false}";
  result.clear();
  error.clear();
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           evaluate_args, &result, &error),
        "evaluate subject=run mode=compare should wrap descriptive run "
        "comparison");
  check(result.find("\"tool\":\"hero.marshal.inspect\"") != std::string::npos,
        "evaluate should identify the high-level Marshal report tool");
  check(result.find("\"subject\":\"run\"") != std::string::npos,
        "evaluate should identify the run subject");
  check(result.find("\"mode\":\"compare\"") != std::string::npos,
        "evaluate compare mode should be explicit");
  check(
      result.find("\"result\":{\"ok\":true,\"report_kind\":\"run_compare\"") !=
          std::string::npos,
      "evaluate subject=run compare mode should preserve the comparison "
      "payload");
  check(result.find("\"operator_summary\":{") != std::string::npos,
        "default compare output should include compact operator summary");
  check(result.find("\"run_pair\":{") != std::string::npos,
        "default compare output should include compact run pair");
  check(result.find("\"baseline_run\"") == std::string::npos,
        "default compare output should keep full baseline job row out of the "
        "operator packet");
  check(result.find("\"performance_visibility\"") == std::string::npos,
        "default compare output should keep full performance panels behind the "
        "machine payload");
  check(result.find("\"machine_payload\"") == std::string::npos,
        "default compare output should not include full audit payload");
  check(result.find("Compare does not choose. Compare explains.") !=
            std::string::npos,
        "evaluate subject=run compare mode should preserve the descriptive "
        "invariant");

  std::filesystem::remove_all(root);
}

void test_operational_report_missing_eval_steps_is_not_mutation_evidence() {
  const auto root = make_tmp_dir("operational_report_missing_steps");
  const auto runtime_root = root / "runtime";
  const auto config_path = root / ".config";
  const auto eval_dir =
      runtime_root /
      "legacy_validation_eval_channel_mdn_1800_2050.run.channel_inference_mdn";
  std::filesystem::create_directories(eval_dir);
  write_text(config_path, "[HERO]\n");
  write_text(eval_dir / "job.manifest",
             "job_id=legacy_validation_eval_channel_mdn_1800_2050.run."
             "channel_inference_mdn\n");
  write_text(eval_dir / "job.state",
             "job_id=legacy_validation_eval_channel_mdn_1800_2050.run."
             "channel_inference_mdn\n"
             "job_kind=channel_inference_mdn\n"
             "status=completed\n"
             "target_component_family_id=wikimyei.inference.expected_value."
             "mdn\n"
             "wave_action=run\n"
             "resolved_anchor_index_begin=1800\n"
             "resolved_anchor_index_end=2050\n"
             "checkpoint_written=false\n"
             "checkpoint_path=\n");
  write_text(eval_dir / "runtime.result.fact", "fact_type=runtime.result.fact\n"
                                               "checkpoint_written=false\n"
                                               "final_loss=2.0\n");
  write_text(eval_dir / "runtime.checkpoint_io.fact",
             "fact_type=runtime.checkpoint_io.fact\n"
             "checkpoint_written=false\n");

  marshal::marshal_operational_report_options_t options{};
  options.runtime_root = runtime_root;
  options.config_path = config_path;
  options.job_ids = {
      "legacy_validation_eval_channel_mdn_1800_2050.run.channel_inference_mdn"};
  const auto report = marshal::build_marshal_operational_report_json(options);
  check(report.find("\"validation_eval_mutated_model_state\":false") !=
            std::string::npos,
        "missing eval optimizer-step evidence must not be treated as mutation");

  const std::string evaluate_args =
      "{\"subject\":\"run\",\"mode\":\"single_job\",\"runtime_root\":" +
      marshal::detail::json_quote(runtime_root.string()) +
      ",\"config_path\":" + marshal::detail::json_quote(config_path.string()) +
      ",\"job_id\":\"legacy_validation_eval_channel_mdn_1800_2050.run."
      "channel_inference_mdn\"}";
  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json("hero.marshal.inspect",
                                           evaluate_args, &result, &error),
        "evaluate subject=run mode=single_job should wrap the operational "
        "report");
  check(result.find("\"tool\":\"hero.marshal.inspect\"") != std::string::npos,
        "evaluate single-job mode should identify the high-level tool");
  check(result.find("\"subject\":\"run\"") != std::string::npos,
        "evaluate should identify the run subject");
  check(result.find("\"mode\":\"single_job\"") != std::string::npos,
        "evaluate single-job mode should be explicit");
  check(result.find("\"validation_eval_mutated_model_state\":false") !=
            std::string::npos,
        "evaluate single-job mode should preserve mutation semantics");

  write_text(eval_dir / "runtime.result.fact", "fact_type=runtime.result.fact\n"
                                               "checkpoint_written=false\n"
                                               "model_state_mutated=true\n"
                                               "final_loss=2.0\n");
  const auto mutated_report =
      marshal::build_marshal_operational_report_json(options);
  check(mutated_report.find("\"validation_eval_mutated_model_state\":true") !=
            std::string::npos,
        "explicit Runtime model_state_mutated evidence must be surfaced");

  std::filesystem::remove_all(root);
}

} // namespace

int main() {
  test_valid_advice();
  test_stale_contract_status_remains_dispatchable();
  test_missing_plan_basis();
  test_missing_suggested_wave();
  test_stale_identity();
  test_forbidden_mode();
  test_missing_checkpoint_input();
  test_malformed_range_and_attempt_budget();
  test_digest_and_root_guards();
  test_lattice_advice_provenance_and_freshness();
  test_signed_lattice_advice_receipt_verification();
  test_status_and_unsupported_surface();
  test_source_key_dispatch_advice_validation();
  test_m2_dry_run_preview_success();
  test_runtime_handoff_accepts_reusable_all_range_overlay();
  test_m2_runtime_rejection();
  test_m2_mode_and_range_mismatch();
  test_m2_checkpoint_mismatch_and_unavailable_handoff();
  test_m2_runtime_hero_live_dry_run_handoff();
  test_runtime_handoff_structured_result_parsing();
  test_m2_public_dry_run_dispatch_operation();
  test_m3_execution_gate_refusals();
  test_m3_accepted_execution_handoff();
  test_m3_execution_handoff_rechecks_runtime_wave();
  test_m4_dispatch_receipt_replay_audit();
  test_m6_operator_status_surface();
  test_m7_batch_preview_independence();
  test_m5_codex_assist_uses_deterministic_primitives();
  test_m9_marshal_tool_schema_compatibility();
  test_target_reach_lattice_target_unresolved_model_state();
  test_target_reach_lattice_target_resolved_ready_for_dry_run();
  test_reach_lattice_target_wraps_target_dispatch();
  test_target_reach_lattice_target_rejects_untrusted_resolution();
  test_m15_budgeted_reach_lattice_target_dry_run_driver();
  test_m16_5_budgeted_reach_lattice_target_requires_explicit_policy();
  test_m15_budgeted_reach_lattice_target_no_progress_window();
  test_m16_5_execute_requires_runtime_terminal_evidence();
  test_m16_5_execute_binds_runtime_terminal_evidence();
  test_m16_5_execute_rejects_terminal_evidence_handoff_mismatch();
  test_m16_5_execute_reconciles_terminal_evidence_by_handoff();
  test_m16_5_reconcile_rejects_wrong_handoff_identity();
  test_m16_5_reconcile_requires_expected_handoff_id_and_digest();
  test_m20_execute_requires_runtime_result_fact_not_state_only();
  test_m20_execute_requires_checkpoint_io_fact_when_checkpoint_io_occurs();
  test_m20_execute_binds_checkpoint_io_fact();
  test_m15_budgeted_reach_lattice_target_stops_on_lattice_warning();
  test_m21_typed_warning_policy_decision();
  test_m21_warning_only_visibility_does_not_stop_below_threshold();
  test_m21_blocking_lattice_warning_stops_with_owner_reason();
  test_m22_target_driver_replay_audit_full_and_stale_identity();
  test_m22_target_driver_run_id_is_stable_and_not_ledger_digest();
  test_m22_target_driver_run_key_groups_but_run_id_is_invocation_unique();
  test_m22_target_driver_replay_audit_tamper_and_compact();
  test_m16_target_driver_resume_guards_policy_and_identity();
  test_m16_target_driver_resume_does_not_repeat_reached_run();
  test_m16_5_target_driver_resume_requires_ledger_digest();
  test_m16_5_target_driver_resume_requires_terminal_identity_after_execute();
  test_m9_marshal_tool_handlers_validate_arguments();
  test_artifact_evidence_panel_and_reach_boundary();
  test_artifact_reach_warning_policy_remains_inspection_only();
  test_artifact_evidence_panel_with_real_lattice_response();
  test_operational_report_summarizes_training_state();
  test_operational_report_quotes_target_blockers();
  test_evaluate_deterministic_subjects();
  test_compare_runs_reports_descriptive_deltas();
  test_operational_report_missing_eval_steps_is_not_mutation_evidence();
  return 0;
}
