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
  const auto request = valid_request(advice);
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
  check(handoff_args.find("\"target_component_family_id\":") !=
            std::string::npos,
        "Runtime handoff should use canonical target_component_family_id");
  check(handoff_args.find("\"mode\":\"run|debug\"") != std::string::npos,
        "Runtime handoff should use canonical mode");
  check(handoff_args.find("\"wave_target\":") != std::string::npos,
        "Runtime handoff should retain wave_target compatibility alias");
  check(!marshal::runtime_dry_run_request_digest(decision.runtime_request)
             .empty(),
        "runtime dry-run request digest should be available");
  check(decision.non_authority_statement.find("audit metadata only") !=
            std::string::npos,
        "M2 decision should carry the non-authority statement");
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  advice.source_lattice_tool = "hero.lattice.plan_target";
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
  check(std::find(names.begin(), names.end(),
                  "hero.marshal.dry_run_dispatch") != names.end(),
        "Marshal tool catalog should expose dry_run_dispatch");
  check(std::find(names.begin(), names.end(),
                  "hero.marshal.prepare_target_dispatch") != names.end(),
        "Marshal tool catalog should expose prepare_target_dispatch");

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

bool fake_lattice_plan_target_callback(const std::string &tool_name,
                                       const std::string &arguments_json,
                                       std::string *out_tool_result_json,
                                       std::string *out_error_message) {
  g_fake_lattice_tool_name = tool_name;
  g_fake_lattice_arguments_json = arguments_json;
  if (out_error_message) {
    out_error_message->clear();
  }
  if (tool_name != "hero.lattice.plan_target") {
    if (out_error_message) {
      *out_error_message = "unexpected lattice tool";
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json =
        R"({"content":[{"type":"text","text":"hero.lattice.plan_target executed"}],"structuredContent":{"config_path":"/tmp/marshal_lookup/.config","runtime_root":"/tmp/marshal_lookup/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"target_id":"lookup_target","status":"metric_failed","split_policy_fingerprint":"sp","plan_ready":true,"plan_basis":{"available":true,"reason":"suggested wave addresses proof deficits","primary_deficit_key":"coverage:missing","primary_deficit_message":"coverage missing","primary_deficit_priority_class":"coverage","deficit_keys":["coverage:missing"],"deficit_priority_classes":["coverage"]},"suggested_wave":{"target":"wikimyei.inference.expected_value.mdn","mode":"run|debug","source_range":"anchor_index","anchor_index_begin":10,"anchor_index_end":20,"input_mdn_checkpoint":"/tmp/marshal_lookup/runtime/mdn.pt","input_representation_checkpoint":"/tmp/marshal_lookup/runtime/rep.pt","text":"TARGET=wikimyei.inference.expected_value.mdn\n"},"proof_certificate":{"target_id":"lookup_target","target_spec_fingerprint":"ts","split_policy_fingerprint":"sp"}},"isError":false})";
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
  if (tool_name == "hero.lattice.plan_target") {
    if (out_tool_result_json) {
      *out_tool_result_json =
          R"({"content":[{"type":"text","text":"hero.lattice.plan_target executed"}],"structuredContent":{"config_path":"/tmp/marshal_prepare/.config","runtime_root":"/tmp/marshal_prepare/runtime","active_identity":{"protocol_contract_fingerprint":"pc","graph_order_fingerprint":"go","source_cursor_token":"cursor","vicreg_assembly_fingerprint":"vic","mdn_assembly_fingerprint":"mdn"},"target_id":"channel_mdn_validation_eval_ready","status":"metric_failed","split_policy_fingerprint":"sp","plan_ready":true,"plan_basis":{"available":true,"reason":"suggested wave addresses proof deficits","primary_deficit_key":"coverage:evaluation_metric","primary_deficit_message":"validation evaluation coverage missing","primary_deficit_priority_class":"coverage","deficit_keys":["coverage:evaluation_metric"],"deficit_priority_classes":["coverage"]},"suggested_wave":{"target":"wikimyei.inference.expected_value.mdn","mode":"run|debug","source_range":"anchor_index","anchor_index_begin":1800,"anchor_index_end":2050,"input_mdn_checkpoint":"latest_satisfying:channel_mdn_train_core_no_test_leakage","input_representation_checkpoint":"latest_satisfying:vicreg_train_core_ready","text":"TARGET=wikimyei.inference.expected_value.mdn\n"},"proof_certificate":{"target_id":"channel_mdn_validation_eval_ready","target_spec_fingerprint":"ts","split_policy_fingerprint":"sp"}},"isError":false})";
    }
    return true;
  }
  if (tool_name == "hero.lattice.resolve_latest_satisfying") {
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
                "hero.lattice.resolve_latest_satisfying executed")
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

std::string prepare_policy_json() {
  marshal::marshal_runtime_policy_snapshot_t policy{};
  policy.runtime_hero_available = true;
  policy.runtime_exec_exists = true;
  policy.runtime_exec_executable = true;
  policy.default_dry_run = true;
  policy.allow_execute = false;
  policy.allow_train_execute = false;
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

void test_target_prepare_dispatch_unresolved_model_state() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = false;
  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(false, false), &result, &error),
        "prepare_target_dispatch should produce an operator packet");
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
        "prepare_target_dispatch must not dry-run by default");
  check(g_fake_lattice_resolve_count == 2,
        "prepare_target_dispatch should ask Lattice to resolve both hints");

  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_target_prepare_dispatch_resolved_ready_for_dry_run() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(true, true), &result, &error),
        "prepare_target_dispatch should materialize resolved hints");
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

void test_target_prepare_dispatch_rejects_untrusted_resolution() {
  g_fake_lattice_resolve_count = 0;
  g_fake_lattice_resolve_concrete = true;
  g_fake_lattice_resolve_fault = "proof_check_failed";
  marshal::set_marshal_lattice_tool_callback(fake_lattice_prepare_callback);

  std::string result;
  std::string error;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(true, false), &result, &error),
        "prepare_target_dispatch should keep responding to untrusted resolver "
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
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(true, false), &result, &error),
        "prepare_target_dispatch should report resolver authority violations");
  check(result.find("\"status\":\"resolver_authority_violation\"") !=
            std::string::npos,
        "Marshal should reject resolver output that is not read-only");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "authority-violating resolver output must not reach dry-run readiness");

  g_fake_lattice_resolve_fault = "source_target_mismatch";
  g_fake_lattice_resolve_count = 0;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(true, false), &result, &error),
        "prepare_target_dispatch should reject resolver source-target drift");
  check(result.find("\"status\":\"resolver_source_target_mismatch\"") !=
            std::string::npos,
        "Marshal should require Lattice to resolve the same requested target");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "source-target resolver drift must block dispatch preparation");

  g_fake_lattice_resolve_fault = "target_status_not_satisfied";
  g_fake_lattice_resolve_count = 0;
  check(marshal::execute_marshal_tool_json(
            "hero.marshal.prepare_target_dispatch",
            prepare_args_json(true, false), &result, &error),
        "prepare_target_dispatch should reject non-satisfying source targets");
  check(result.find("\"status\":\"target_not_satisfied\"") != std::string::npos,
        "Marshal should require latest_satisfying sources to be satisfied");
  check(result.find("\"dispatch_state\":\"blocked\"") != std::string::npos,
        "non-satisfying source-target resolution must not reach dry-run "
        "readiness");

  g_fake_lattice_resolve_fault.clear();
  marshal::set_marshal_lattice_tool_callback(nullptr);
}

void test_m9_marshal_tool_handlers_validate_arguments() {
  auto advice = valid_advice();
  auto request = valid_request(advice);
  const auto ctx = context();
  auto policy = valid_policy();
  policy.runtime_exec_exists = false;
  const auto wave = valid_active_wave(advice);

  std::string result;
  std::string error;
  marshal::set_marshal_lattice_tool_callback(fake_lattice_plan_target_callback);
  const std::string lookup_args =
      "{\"target_id\":\"lookup_target\","
      "\"source_lattice_timestamp\":\"2026-05-24T00:00:00Z\","
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
  check(marshal::execute_marshal_tool_json("hero.marshal.lookup_target_advice",
                                           lookup_args, &result, &error),
        "lookup_target_advice should ask Lattice and materialize advice");
  check(g_fake_lattice_tool_name == "hero.lattice.plan_target",
        "lookup wrapper should call Lattice plan_target");
  check(g_fake_lattice_arguments_json.find("\"target_id\":\"lookup_target\"") !=
            std::string::npos,
        "lookup wrapper should pass target_id to Lattice");
  check(result.find("\"dispatchable\":true") != std::string::npos,
        "lookup wrapper should validate materialized explicit advice");
  check(
      result.find("\"target_status\":\"metric_failed\"") != std::string::npos,
      "metric_failed Lattice targets should remain dispatchable when planned");
  check(
      result.find("PLAN_INPUT_MDN_CHECKPOINT") != std::string::npos &&
          result.find("PLAN_INPUT_REPRESENTATION_CHECKPOINT") !=
              std::string::npos,
      "lookup wrapper should map Lattice checkpoint hints to PLAN_INPUT keys");

  const std::string free_text_lookup_args =
      lookup_args.substr(0, lookup_args.size() - 1) +
      ",\"target_text\":\"dispatch whatever target seems useful\"}";
  check(!marshal::execute_marshal_tool_json("hero.marshal.lookup_target_advice",
                                            free_text_lookup_args, &result,
                                            &error),
        "lookup wrapper must reject free-form target text");
  check(error.find("unknown field: target_text") != std::string::npos,
        "lookup wrapper should expose unknown-field rejection for target_text");
  marshal::set_marshal_lattice_tool_callback(nullptr);

  const std::string validate_args = "{\"advice\":" + advice_json(advice) +
                                    ",\"request\":" + request_json(request) +
                                    ",\"context\":" + context_json(ctx) + "}";
  check(marshal::execute_marshal_tool_json("hero.marshal.validate_advice",
                                           validate_args, &result, &error),
        "validate_advice handler should execute");
  check(result.find("\"isError\":false") != std::string::npos,
        "validate_advice handler should return non-error result");
  check(result.find("\"dispatchable\":true") != std::string::npos,
        "validate_advice handler should materialize advice and request");

  const std::string missing_request_args =
      "{\"advice\":" + advice_json(advice) + "}";
  check(!marshal::execute_marshal_tool_json("hero.marshal.validate_advice",
                                            missing_request_args, &result,
                                            &error),
        "validate_advice handler should reject missing required request");
  check(error.find("missing required field: request") != std::string::npos,
        "missing required field should be reported by actual handler");

  const std::string unknown_field_args =
      validate_args.substr(0, validate_args.size() - 1) +
      ",\"unexpected\":true}";
  check(!marshal::execute_marshal_tool_json("hero.marshal.validate_advice",
                                            unknown_field_args, &result,
                                            &error),
        "validate_advice handler should reject unknown top-level fields");
  check(error.find("unknown field: unexpected") != std::string::npos,
        "unknown field should be reported by actual handler");

  const std::string dry_run_args =
      "{\"advice\":" + advice_json(advice) +
      ",\"request\":" + request_json(request) +
      ",\"context\":" + context_json(ctx) +
      ",\"runtime_policy\":" + policy_json(policy) +
      ",\"runtime_wave\":" + wave_json(wave) + "}";
  check(marshal::execute_marshal_tool_json("hero.marshal.dry_run_dispatch",
                                           dry_run_args, &result, &error),
        "dry_run_dispatch handler should execute deterministic primitive");
  check(result.find("\"isError\":false") != std::string::npos,
        "dry_run_dispatch refusal should be a non-error tool result");

  auto execute_request = request;
  execute_request.requested_mode = marshal::marshal_dispatch_mode_t::execute;
  execute_request.operator_confirmation_token =
      marshal::confirmation_token(advice, execute_request);
  policy.runtime_exec_exists = true;
  policy.allow_execute = true;
  const auto prior =
      prior_dry_run_evidence(advice, execute_request, ctx, policy, wave);
  const std::string gate_args =
      "{\"advice\":" + advice_json(advice) +
      ",\"request\":" + request_json(execute_request) +
      ",\"context\":" + context_json(ctx) +
      ",\"prior_dry_run\":" + prior_json(prior) +
      ",\"runtime_policy\":" + policy_json(policy) +
      ",\"runtime_wave\":" + wave_json(wave) + "}";
  check(marshal::execute_marshal_tool_json("hero.marshal.execution_gate",
                                           gate_args, &result, &error),
        "execution_gate handler should execute deterministic primitive");
  check(result.find("\"accepted\":true") != std::string::npos,
        "execution_gate handler should accept verified prior dry-run evidence");

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
  const std::string replay_args =
      "{\"receipt\":" + receipt_json(receipt) +
      ",\"active_identity\":" + active_identity_json(advice.active_identity) +
      "}";
  check(marshal::execute_marshal_tool_json("hero.marshal.replay_receipt",
                                           replay_args, &result, &error),
        "replay_receipt handler should execute deterministic primitive");
  check(result.find("\"accepted\":true") != std::string::npos,
        "replay_receipt handler should accept intact receipt");

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

  const std::string batch_args =
      "{\"items\":[{\"item_id\":\"one\",\"advice\":" + advice_json(advice) +
      ",\"request\":" + request_json(request) +
      ",\"runtime_wave\":" + wave_json(wave) +
      "}],\"context\":" + context_json(ctx) +
      ",\"runtime_policy\":" + policy_json(policy) + "}";
  check(marshal::execute_marshal_tool_json("hero.marshal.batch_preview",
                                           batch_args, &result, &error),
        "batch_preview handler should execute deterministic primitive");
  check(result.find("\"item_count\":1") != std::string::npos,
        "batch_preview handler should expose item count");
}

} // namespace

int main() {
  test_valid_advice();
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
  test_target_prepare_dispatch_unresolved_model_state();
  test_target_prepare_dispatch_resolved_ready_for_dry_run();
  test_target_prepare_dispatch_rejects_untrusted_resolution();
  test_m9_marshal_tool_handlers_validate_arguments();
  return 0;
}
