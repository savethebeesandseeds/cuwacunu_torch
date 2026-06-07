#include "hero/lattice_hero/lattice/target/lattice_target_evaluator.h"
#include "tests/bench/kikijyeba/test_support/lattice_forecast_artifact_fixture.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace exposure = cuwacunu::hero::lattice::exposure;
namespace lattice_fixture = cuwacunu::tests::kikijyeba::lattice_fixture;
namespace target = cuwacunu::hero::lattice::target;
namespace fs = std::filesystem;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_decode_error(const std::string &dsl, const std::string &needle,
                         const std::string &message) {
  try {
    (void)target::decode_lattice_targets_from_dsl(dsl);
  } catch (const std::runtime_error &ex) {
    check(std::string(ex.what()).find(needle) != std::string::npos,
          message + " (unexpected error: " + ex.what() + ")");
    return;
  }
  throw std::runtime_error(message + " (decode unexpectedly succeeded)");
}

std::string artifact_readiness_target_dsl(const std::string &target_extra,
                                          const std::string &post_blocks = "") {
  return std::string(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_artifact_training_knob;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
)DSL") + target_extra +
         R"DSL(
};
)DSL" + post_blocks;
}

bool has_issue_containing(const std::vector<std::string> &issues,
                          const std::string &needle) {
  return std::any_of(issues.begin(), issues.end(), [&](const auto &issue) {
    return issue.find(needle) != std::string::npos;
  });
}

bool has_reason_containing(const std::vector<std::string> &reasons,
                           const std::string &needle) {
  return std::any_of(reasons.begin(), reasons.end(), [&](const auto &reason) {
    return reason.find(needle) != std::string::npos;
  });
}

bool has_deficit_key(
    const std::vector<target::lattice_target_evaluation_t::proof_deficit_t>
        &deficits,
    const std::string &key) {
  return std::any_of(deficits.begin(), deficits.end(),
                     [&](const auto &deficit) { return deficit.key == key; });
}

bool deficit_has_related_fact_integrity_issue_code(
    const std::vector<target::lattice_target_evaluation_t::proof_deficit_t>
        &deficits,
    const std::string &key, const std::string &issue_code) {
  return std::any_of(
      deficits.begin(), deficits.end(), [&](const auto &deficit) {
        return deficit.key == key &&
               std::find(deficit.related_fact_integrity_issue_codes.begin(),
                         deficit.related_fact_integrity_issue_codes.end(),
                         issue_code) !=
                   deficit.related_fact_integrity_issue_codes.end();
      });
}

std::string
describe_evaluation(const target::lattice_target_evaluation_t &eval) {
  std::ostringstream out;
  out << " status=" << target::lattice_target_status_name(eval.status)
      << " reasons=";
  for (const auto &reason : eval.reasons) {
    out << "[" << reason << "]";
  }
  out << " deficits=";
  for (const auto &deficit : eval.deficits) {
    out << "[" << deficit.key << "]";
  }
  out << " fact_integrity_declared="
      << eval.fact_integrity_summary.relation_declared_count
      << " fact_integrity_bound="
      << eval.fact_integrity_summary.relation_bound_count
      << " fact_integrity_unresolved="
      << eval.fact_integrity_summary.unresolved_relation_count
      << " fact_integrity_identity_mismatch="
      << eval.fact_integrity_summary.identity_mismatch_count;
  return out.str();
}

void check_artifact_proof_no_decision_authority(
    const target::lattice_target_proof_certificate_t::artifact_proof_t &proof,
    const std::string &label) {
  check(proof.artifact_evidence && proof.deterministic_artifact &&
            proof.visibility_only && proof.authority_clean &&
            proof.proof_template_bound && !proof.proof_template_claim.empty() &&
            proof.fact_identity_contract_bound &&
            proof.fact_identity_envelope_complete &&
            proof.fact_identity_contract_schema ==
                target::k_lattice_fact_identity_contract_schema_v1 &&
            proof.fact_identity_contract_id ==
                target::k_lattice_fact_identity_contract_id_v1 &&
            proof.row_index_interval_authority &&
            proof.source_key_window_audit_only && proof.lineage_bound &&
            proof.passed && proof.issues.empty(),
        label + " should be a clean visibility-only artifact proof bound to "
                "the catalog identity contract");
  check(!proof.readiness_authority && !proof.quality_authority &&
            !proof.performance_authority && !proof.checkpoint_selector &&
            !proof.coverage_authority && !proof.leakage_authority &&
            !proof.contract_identity_authority && !proof.allocation_authority &&
            !proof.execution_authority && !proof.market_readiness_authority &&
            !proof.deployment_authority && !proof.policy_gate &&
            !proof.target_dependency_authority &&
            !proof.runtime_wave_authority && !proof.marshal_reachability &&
            !proof.checkpoint_source_authority &&
            !proof.plan_checkpoint_input_authority &&
            !proof.model_state_mutation &&
            !proof.raw_potential_tradable_return && !proof.replay_executor &&
            !proof.fact_identity_target_kind_authority &&
            !proof.fact_identity_runtime_wave_authority &&
            !proof.fact_identity_marshal_reachability &&
            !proof.fact_identity_policy_gate_authority,
        label + " should carry explicit false decision-authority flags");
  check(!proof.fact_schema.empty() && !proof.fact_type.empty() &&
            !proof.fact_digest.empty() &&
            !proof.parent_exposure_fact_digest.empty() &&
            !proof.job_id.empty() && !proof.wave_id.empty() &&
            !proof.protocol_id.empty() && !proof.contract_fingerprint.empty() &&
            !proof.component.empty() &&
            !proof.component_assembly_fingerprint.empty() &&
            !proof.graph_order_fingerprint.empty() &&
            !proof.source_cursor_token.empty() &&
            !proof.split_policy_fingerprint.empty() &&
            !proof.split_name.empty() && !proof.anchor_range.empty() &&
            !proof.completed_anchor_range.empty(),
        label + " should carry the common fact identity envelope");
}

void check_warning_envelope(
    const target::lattice_target_evaluation_t::warning_result_t &warning,
    const std::string &target_id, const std::string &warning_family,
    const std::string &fact_family, const std::string &label) {
  check(warning.target_id == target_id && warning.source == "lattice" &&
            warning.component == "lattice_target" && !warning.blocking &&
            warning.warning_family == warning_family &&
            warning.readiness_effect == "non_blocking_warning_only" &&
            warning.fact_family == fact_family &&
            !warning.evidence_digest.empty() &&
            !warning.machine_reason_code.empty() &&
            !warning.human_explanation.empty() &&
            !warning.suggested_inspection_panel.empty() &&
            warning.target_ids_observed_against.size() == 1 &&
            warning.target_ids_observed_against.front() == target_id,
        label + " should expose a typed non-blocking Lattice warning "
                "envelope");
}

void write_text(const fs::path &path, const std::string &text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write fixture: " + path.string());
  }
  out << text;
}

std::string read_text(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to read fixture: " + path.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string read_active_lattice_targets_dsl() {
  const std::vector<fs::path> candidates{
      fs::current_path() / "src/config/hero.lattice.targets.dsl",
      fs::current_path() / "../../../../config/hero.lattice.targets.dsl",
      "/cuwacunu/src/config/hero.lattice.targets.dsl",
  };
  for (const auto &candidate : candidates) {
    if (fs::exists(candidate)) {
      return read_text(candidate);
    }
  }
  throw std::runtime_error("failed to locate active lattice targets DSL");
}

std::string mtf_report_text(const std::string &checkpoint_digest,
                            bool include_schema = true,
                            const std::string &reported_digest_override = {}) {
  std::ostringstream out;
  if (include_schema) {
    out << "report_schema_id="
           "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1\n"
           "report_schema_version=1\n"
           "report_writer_id=jkimyei.training.representation."
           "mtf_jepa_mae_vicreg_graph_first_launcher\n"
           "report_writer_version=1\n";
  }
  out << "optimizer_steps=3\n"
         "mean_loss=0.30\n"
         "representation_architecture=mtf_jepa_mae_vicreg.v1\n"
         "representation_contract=standalone.mtf_jepa_mae_vicreg.v1\n"
         "primary_value_shape=[B_flat,De]\n"
         "sequence_value_shape=[B_flat,Ntok,De]\n"
         "channel_value_shape=[B_flat,C,De]\n"
         "channel_axis_policy=optional_channel_output\n"
         "temporal_reduction=mask_aware_token_pool\n"
         "input_nodelift_shape=[B,C,Hx,N,F]\n"
         "mtf_training_shape=[B_flat,C,Hx,F]\n"
         "flattening_contract=anchor_node_flatten.v1\n"
         "recommended_graph_restore_shape=[B,N,C,De]\n"
         "graph_restore_available=false\n"
         "graph_restore_reason="
         "runtime_reports_flattened_anchor_node_samples_only\n"
         "reshape_lossless=true\n"
         "anchor_batch_count=4\n"
         "node_count=2\n"
         "flattened_sample_count=8\n"
         "channel_count=3\n"
         "history_length=16\n"
         "input_feature_width=5\n"
         "run_data_kind=market\n"
         "readiness_scope=market_pretraining\n"
         "synthetic_training_passed=false\n"
         "market_readiness_claimed=false\n"
         "finite_parameter_check=true\n"
         "gradients_finite=true\n"
         "sample_valid_count=8\n"
         "sample_valid_fraction=1.0\n"
         "channel_valid_count=24\n"
         "channel_valid_fraction=1.0\n"
         "valid_latent_rows=24\n"
         "total_valid_projection_rows=24\n"
         "tf_pair_count=16\n"
         "tf_pair_valid_count=16\n"
         "vicreg_global_valid_rows=8\n"
         "vicreg_channel_valid_rows=24\n"
         "context_starved_sample_count=0\n"
         "reduced_targets_for_context_count=0\n"
         "min_context_satisfied_count=8\n"
         "target_ema_distance=0.01\n"
         "latent_std=0.25\n"
         "latent_norm=1.5\n"
         "loss_jepa_mean=0.20\n"
         "loss_mae_time_mean=0.03\n"
         "loss_mae_frequency_mean=0.04\n"
         "loss_tf_align_mean=0.02\n"
         "loss_vicreg_global_mean=0.01\n"
         "loss_vicreg_channel_mean=0.01\n"
         "representation_embedding_dim=32\n"
         "representation_effective_rank=16\n"
         "representation_effective_rank_fraction=0.50\n"
         "representation_min_dimension_variance=0.01\n"
         "representation_max_dimension_variance=1.0\n"
         "representation_condition_number=100\n"
         "representation_isotropy_score=0.25\n"
         "checkpoint_written=true\n"
         "checkpoint_path=mtf.pt\n"
         "checkpoint_path_reported=mtf.pt\n"
         "checkpoint_digest_verified=true\n"
         "checkpoint_file_exists=true\n"
         "checkpoint_bytes=14\n"
      << "checkpoint_digest_reported="
      << (reported_digest_override.empty() ? checkpoint_digest
                                           : reported_digest_override)
      << "\n";
  return out.str();
}

void write_mtf_job_fixture(const fs::path &runtime_root,
                           bool include_schema = true,
                           const std::string &reported_digest_override = {},
                           const std::string &protocol_id = "cwu_02v") {
  const auto job_dir = runtime_root / "mtf_ready_job";
  std::ostringstream manifest;
  manifest << "job_id=mtf_ready_job\n"
              "job_kind=channel_representation_mtf_jepa_mae_vicreg\n";
  if (!protocol_id.empty()) {
    manifest << "protocol_id=" << protocol_id << "\n";
  }
  manifest << "target_component_family_id="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
              "wave_id=wave_mtf\n"
              "wave_action=train\n"
              "mutated_components="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
              "graph_order_fingerprint=graph_1\n"
              "source_range_policy=anchor_index\n"
              "resolved_anchor_index_begin=0\n"
              "resolved_anchor_index_end=10\n"
              "accepted_anchor_count=10\n"
              "source_cursor_token=cursor_1\n"
              "protocol_contract_fingerprint=contract_1\n"
              "mtf_jepa_mae_vicreg_assembly_fingerprint=mtf_1\n";
  write_text(job_dir / "job.manifest", manifest.str());
  write_text(job_dir / "job.state", "status=completed\n"
                                    "optimizer_steps=3\n"
                                    "last_loss=0.35\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=mtf.pt\n");
  write_text(job_dir / "mtf.pt", "mtf checkpoint");
  const auto digest = exposure::file_content_digest(job_dir / "mtf.pt");
  write_text(job_dir / "channel_representation.report",
             mtf_report_text(digest, include_schema, reported_digest_override));
  const auto fact = exposure::make_exposure_fact_from_job_dir(job_dir);
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(job_dir), fact);
  exposure::write_checkpoint_fact_sidecar(
      exposure::checkpoint_fact_path_for_job_dir(job_dir),
      exposure::make_checkpoint_fact_from_exposure_fact(fact));
}

void write_mtf_job_fixture_with_loss(const fs::path &runtime_root,
                                     const std::string &job_id,
                                     const std::string &checkpoint_text,
                                     double last_loss,
                                     fs::file_time_type marker_time) {
  const auto job_dir = runtime_root / job_id;
  std::ostringstream manifest;
  manifest << "job_id=" << job_id << "\n"
           << "job_kind=channel_representation_mtf_jepa_mae_vicreg\n"
           << "protocol_id=cwu_02v\n"
           << "target_component_family_id="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
           << "wave_id=wave_" << job_id << "\n"
           << "wave_action=train\n"
           << "mutated_components="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
           << "graph_order_fingerprint=graph_1\n"
           << "source_range_policy=anchor_index\n"
           << "resolved_anchor_index_begin=0\n"
           << "resolved_anchor_index_end=10\n"
           << "accepted_anchor_count=10\n"
           << "source_cursor_token=cursor_1\n"
           << "protocol_contract_fingerprint=contract_1\n"
           << "mtf_jepa_mae_vicreg_assembly_fingerprint=mtf_1\n";
  write_text(job_dir / "job.manifest", manifest.str());

  std::ostringstream state;
  state << "status=completed\n"
        << "optimizer_steps=3\n"
        << "last_loss=" << last_loss << "\n"
        << "checkpoint_written=true\n"
        << "checkpoint_path=mtf.pt\n";
  write_text(job_dir / "job.state", state.str());
  write_text(job_dir / "mtf.pt", checkpoint_text);
  const auto digest = exposure::file_content_digest(job_dir / "mtf.pt");
  write_text(job_dir / "channel_representation.report",
             mtf_report_text(digest));
  const auto fact = exposure::make_exposure_fact_from_job_dir(job_dir);
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(job_dir), fact);
  exposure::write_checkpoint_fact_sidecar(
      exposure::checkpoint_fact_path_for_job_dir(job_dir),
      exposure::make_checkpoint_fact_from_exposure_fact(fact));
  fs::last_write_time(job_dir / "job.state", marker_time);
}

std::vector<target::lattice_target_spec_t> mtf_eval_specs() {
  return target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mtf_ready;
  TARGET_KIND = mtf_representation_ready;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = mtf_ready;
  WARNING_ID = mtf_effective_rank_low_visibility_only;
  KIND = representation_health;
  USE = observed_input;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = representation_effective_rank_fraction;
  BELOW = 0.75;
};
)DSL");
}

std::vector<target::lattice_target_spec_t>
mtf_latest_satisfying_metric_blind_specs() {
  return target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mtf_latest_source_ready;
  TARGET_KIND = mtf_representation_ready;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = mtf_latest_consumer_ready;
  TARGET_KIND = mtf_representation_ready;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  CHECKPOINT_SOURCE = latest_satisfying:mtf_latest_source_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};
)DSL");
}

std::vector<target::lattice_target_spec_t> source_analytics_warn_specs() {
  return target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mtf_source_analytics_warn_ready;
  TARGET_KIND = mtf_representation_ready;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = mtf_source_analytics_warn_ready;
  WARNING_ID = source_missingness_high_visibility_only;
  KIND = source_analytics;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = missingness_fraction;
  ABOVE = 0.10;
};
)DSL");
}

std::vector<target::lattice_target_spec_t> mtf_severe_runtime_warn_specs() {
  return target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mtf_severe_runtime_warn_ready;
  TARGET_KIND = mtf_representation_ready;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = mtf_severe_runtime_warn_ready;
  WARNING_ID = checkpoint_written_severe_visibility_only;
  KIND = runtime_health;
  USE = observed_input;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = checkpoint_written;
  ABOVE = 0.5;
};
)DSL");
}

std::vector<target::lattice_target_spec_t> mtf_unscoped_eval_specs() {
  return target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mtf_unscoped_ready;
  TARGET_KIND = mtf_representation_ready;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MAX_WAVES = 1;
};
)DSL");
}

target::lattice_target_evaluator_options_t
mtf_eval_options(const fs::path &runtime_root) {
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = runtime_root;
  options.active_identity.protocol_id = "cwu_02v";
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_1";
  options.active_identity.mtf_jepa_mae_vicreg_assembly_fingerprint = "mtf_1";
  return options;
}

exposure::lattice_exposure_ledger_t artifact_readiness_ledger(
    bool observer_authority_drift = false,
    bool forecast_authority_drift = false, bool replay_authority_drift = false,
    bool baseline_transform_digest_mismatch = false,
    bool forecast_baseline_digest_mismatch = false,
    bool forecast_selection_signal_digest_mismatch = false,
    bool observer_forecast_lineage_mismatch = false,
    bool allocation_observer_digest_mismatch = false,
    bool allocation_forecast_artifact_mismatch = false,
    bool allocation_reserve_source_mismatch = false,
    bool allocation_reserve_base_policy_mismatch = false,
    bool allocation_reserve_graph_unbound = false,
    bool allocation_missing_reason_contract = false,
    bool replay_contract_mismatch = false,
    bool replay_incomplete_attempts = false,
    bool replay_missing_projection_metrics = false,
    bool forecast_missing_horizon_support = false,
    bool replay_missing_time_law_steps = false,
    bool replay_future_observation_violation = false,
    bool replay_mixed_future_keys = false,
    bool replay_missing_projection_step_evidence = false,
    bool replay_expected_step_mismatch = false,
    bool replay_action_time_policy_mismatch = false,
    bool replay_source_order_policy_mismatch = false,
    bool replay_parallelism_mismatch = false,
    bool replay_world_mode_mismatch = false,
    bool replay_schema_mismatch = false,
    bool replay_runtime_run_id_missing = false,
    bool replay_report_digest_mismatch = false,
    bool transform_missing_support_surface = false,
    bool baseline_missing_support = false,
    bool forecast_missing_target_transform_binding = false,
    bool forecast_missing_baseline_binding = false,
    bool forecast_missing_evaluated_checkpoint_lineage = false,
    bool forecast_model_state_mutation = false,
    bool forecast_negative_skill = false,
    bool policy_training_future_snapshot_use = false,
    bool replay_missing_policy_summary = false,
    bool replay_failed_attempts = false,
    bool policy_training_component_is_policy = false) {
  exposure::lattice_exposure_fact_t parent{};
  parent.fact_type = "exposure";
  parent.contract_fingerprint = "contract_1";
  parent.protocol_id = "cwu_02v";
  parent.graph_order_fingerprint = "graph_1";
  parent.source_cursor_token = "cursor_1";
  parent.cursor_domain = "ujcamei.graph_anchor";
  parent.split_policy_fingerprint = "split_policy_1";
  parent.component_assembly_fingerprint = "mdn_1";
  parent.target_component_family_id = "wikimyei.inference.expected_value.mdn";
  parent.job_id = "artifact_job";
  parent.wave_id = "artifact_wave";
  parent.wave_action = "evaluate";
  parent.job_status = "completed";
  parent.split_name = "train_core";
  parent.split_role = exposure::exposure_split_role_t::train;
  parent.anchor_range = {.begin = 0, .end = 10};
  parent.completed_anchor_range = parent.anchor_range;

  exposure::lattice_exposure_ledger_t ledger{};
  ledger.add(parent, /*derive_source_receipts=*/false,
             /*derive_selection_signals=*/false);
  const auto parent_digest = exposure::exposure_fact_digest(parent);
  auto selection_signal =
      exposure::make_selection_signal_fact_from_exposure_fact(parent);
  ledger.add_selection_signal(selection_signal);
  const auto selection_signal_digest =
      exposure::selection_signal_fact_digest(selection_signal);

  exposure::lattice_target_transform_fact_t transform{};
  transform.parent_exposure_fact_digest = parent_digest;
  transform.contract_fingerprint = parent.contract_fingerprint;
  transform.protocol_id = parent.protocol_id;
  transform.graph_order_fingerprint = parent.graph_order_fingerprint;
  transform.source_cursor_token = parent.source_cursor_token;
  transform.split_policy_fingerprint = parent.split_policy_fingerprint;
  transform.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  transform.target_component_family_id = parent.target_component_family_id;
  transform.job_id = parent.job_id;
  transform.wave_id = parent.wave_id;
  transform.job_status = parent.job_status;
  transform.wave_action = parent.wave_action;
  transform.split_name = parent.split_name;
  transform.split_role = parent.split_role;
  transform.anchor_range = parent.anchor_range;
  transform.completed_anchor_range = parent.completed_anchor_range;
  transform.target_feature_ids = {"ev_close", "ev_return"};
  transform.horizon = 3;
  transform.target_mode = "log_return";
  transform.normalization_contract = "zscore.train_core.v1";
  transform.inverse_transform_contract = "exp_return.v1";
  transform.units = "log_return";
  transform.target_mask_policy_digest = "mask_policy_1";
  transform.support_surface_identity = "BTC_ETH:h3";
  transform.support_surface_digest = "support_surface_1";
  if (transform_missing_support_surface) {
    transform.support_surface_identity.clear();
    transform.support_surface_digest.clear();
  }
  ledger.add_target_transform(transform);
  const auto transform_digest =
      exposure::target_transform_fact_digest(transform);

  exposure::lattice_forecast_baseline_fact_t baseline{};
  baseline.parent_exposure_fact_digest = parent_digest;
  baseline.contract_fingerprint = parent.contract_fingerprint;
  baseline.protocol_id = parent.protocol_id;
  baseline.graph_order_fingerprint = parent.graph_order_fingerprint;
  baseline.source_cursor_token = parent.source_cursor_token;
  baseline.split_policy_fingerprint = parent.split_policy_fingerprint;
  baseline.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  baseline.target_component_family_id = parent.target_component_family_id;
  baseline.job_id = parent.job_id;
  baseline.wave_id = parent.wave_id;
  baseline.job_status = parent.job_status;
  baseline.wave_action = parent.wave_action;
  baseline.split_name = parent.split_name;
  baseline.split_role = parent.split_role;
  baseline.anchor_range = parent.anchor_range;
  baseline.completed_anchor_range = parent.completed_anchor_range;
  baseline.target_feature_ids = transform.target_feature_ids;
  baseline.horizon = transform.horizon;
  baseline.baseline_kind = "previous_value";
  baseline.baseline_parameters = "lag=1";
  baseline.target_transform_fact_digest = baseline_transform_digest_mismatch
                                              ? "missing_transform_digest"
                                              : transform_digest;
  baseline.support_count = 42;
  baseline.valid_count = 40;
  if (baseline_missing_support) {
    baseline.support_count = 0;
    baseline.valid_count = 0;
  }
  baseline.missing_count = 2;
  baseline.baseline_mean_nll = 1.50;
  baseline.baseline_ev_mae = 0.20;
  baseline.baseline_ev_rmse = 0.30;
  baseline.baseline_signed_error = -0.01;
  baseline.baseline_directional_accuracy = 0.55;
  baseline.metric_status = "computed";
  ledger.add_forecast_baseline(baseline);
  const auto baseline_digest =
      exposure::forecast_baseline_fact_digest(baseline);

  exposure::lattice_forecast_eval_fact_t forecast{};
  forecast.parent_exposure_fact_digest = parent_digest;
  forecast.contract_fingerprint = parent.contract_fingerprint;
  forecast.protocol_id = parent.protocol_id;
  forecast.graph_order_fingerprint = parent.graph_order_fingerprint;
  forecast.source_cursor_token = parent.source_cursor_token;
  forecast.split_policy_fingerprint = parent.split_policy_fingerprint;
  forecast.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  forecast.target_component_family_id = parent.target_component_family_id;
  forecast.job_id = parent.job_id;
  forecast.wave_id = parent.wave_id;
  forecast.job_status = parent.job_status;
  forecast.wave_action = parent.wave_action;
  forecast.split_name = parent.split_name;
  forecast.split_role = parent.split_role;
  forecast.anchor_range = parent.anchor_range;
  forecast.completed_anchor_range = parent.completed_anchor_range;
  forecast.target_feature_ids = transform.target_feature_ids;
  forecast.horizon = transform.horizon;
  forecast.support_count = 42;
  forecast.valid_count = 40;
  forecast.missing_count = 2;
  forecast.weakest_support_rows = 8;
  forecast.mean_nll = 1.10;
  forecast.mean_nll_per_horizon = {1.10};
  if (!forecast_missing_horizon_support) {
    forecast.valid_target_count_per_horizon = {40};
  }
  forecast.ev_mae = 0.16;
  forecast.ev_rmse = 0.24;
  if (forecast_negative_skill) {
    forecast.mean_nll = 1.80;
    forecast.mean_nll_per_horizon = {1.80};
    forecast.ev_mae = 0.28;
    forecast.ev_rmse = 0.36;
  }
  forecast.signed_error = -0.02;
  forecast.directional_accuracy = 0.62;
  forecast.calibration_coverage = 0.91;
  forecast.pit_summary = "uniform-ish";
  forecast.sigma_scale_sanity = "ok";
  forecast.support_by_node = "BTC:20,ETH:20";
  forecast.support_by_channel = "close:40";
  forecast.support_by_target_feature = "ev_close:40,ev_return:40";
  if (!forecast_missing_horizon_support) {
    forecast.support_by_horizon = "h3:40";
  }
  forecast.forecast_artifact_digest = "forecast_artifact_1";
  if (!forecast_missing_evaluated_checkpoint_lineage) {
    forecast.evaluated_representation_checkpoint_digest = "rep_checkpoint_1";
    forecast.evaluated_mdn_checkpoint_digest = "mdn_checkpoint_1";
  }
  if (!forecast_missing_target_transform_binding) {
    forecast.target_transform_fact_digest = transform_digest;
  }
  if (!forecast_missing_baseline_binding) {
    forecast.baseline_fact_digests =
        forecast_baseline_digest_mismatch
            ? std::vector<std::string>{"missing_baseline_digest"}
            : std::vector<std::string>{baseline_digest};
  }
  forecast.selection_signal_fact_digests =
      forecast_selection_signal_digest_mismatch
          ? std::vector<std::string>{"missing_selection_signal_digest"}
          : std::vector<std::string>{selection_signal_digest};
  forecast.quality_authority = forecast_authority_drift;
  forecast.model_state_mutation = forecast_model_state_mutation;
  ledger.add_forecast_eval(forecast);
  const auto forecast_digest = exposure::forecast_eval_fact_digest(forecast);

  exposure::lattice_observer_belief_fact_t observer{};
  observer.parent_exposure_fact_digest = parent_digest;
  observer.contract_fingerprint = parent.contract_fingerprint;
  observer.protocol_id = parent.protocol_id;
  observer.graph_order_fingerprint = parent.graph_order_fingerprint;
  observer.source_cursor_token = parent.source_cursor_token;
  observer.split_policy_fingerprint = parent.split_policy_fingerprint;
  observer.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  observer.target_component_family_id = parent.target_component_family_id;
  observer.job_id = parent.job_id;
  observer.wave_id = parent.wave_id;
  observer.job_status = parent.job_status;
  observer.wave_action = parent.wave_action;
  observer.split_name = parent.split_name;
  observer.split_role = parent.split_role;
  observer.anchor_range = parent.anchor_range;
  observer.completed_anchor_range = parent.completed_anchor_range;
  observer.belief_kind = "raw_nodelift_potential";
  observer.channel_consensus = "BTC:0.62,ETH:0.58";
  observer.potential_surface_diagnostics = "stable_surface";
  observer.nodelift_return_projection = "potential_only";
  observer.covariance_coupling = "diagonal_scenario_bank";
  observer.scenario_bank_digest = "scenario_bank_1";
  observer.nodelift_residual_quality = "ok";
  observer.projection_validation_scores = "rmse:0.24";
  observer.confidence = 0.64;
  observer.data_quality = 0.88;
  observer.liquidity = 0.72;
  observer.forecast_artifact_digest = "forecast_artifact_1";
  observer.forecast_artifact_lineage = observer_forecast_lineage_mismatch
                                           ? "missing_forecast_eval_digest"
                                           : forecast_digest;
  observer.feature_semantics_fingerprint = "feature_semantics_1";
  observer.dock_binding_fingerprint = "dock_binding_1";
  observer.raw_potential_tradable_return = observer_authority_drift;
  ledger.add_observer_belief(observer);
  const auto observer_digest = exposure::observer_belief_fact_digest(observer);

  exposure::lattice_allocation_engine_fact_t allocation{};
  allocation.parent_exposure_fact_digest = parent_digest;
  allocation.contract_fingerprint = parent.contract_fingerprint;
  allocation.protocol_id = parent.protocol_id;
  allocation.graph_order_fingerprint = parent.graph_order_fingerprint;
  allocation.source_cursor_token = parent.source_cursor_token;
  allocation.split_policy_fingerprint = parent.split_policy_fingerprint;
  allocation.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  allocation.target_component_family_id = parent.target_component_family_id;
  allocation.job_id = parent.job_id;
  allocation.wave_id = parent.wave_id;
  allocation.job_status = parent.job_status;
  allocation.wave_action = parent.wave_action;
  allocation.split_name = parent.split_name;
  allocation.split_role = parent.split_role;
  allocation.anchor_range = parent.anchor_range;
  allocation.completed_anchor_range = parent.completed_anchor_range;
  allocation.target_node_weights = "BTC:0.40,ETH:0.25";
  allocation.accounting_numeraire_node_id = "USD_CASH";
  allocation.accounting_numeraire_node_source =
      allocation_reserve_source_mismatch ? "allocator_output" : "base_policy";
  allocation.base_policy_accounting_numeraire_node_id =
      allocation_reserve_base_policy_mismatch ? "EUR_CASH" : "USD_CASH";
  allocation.accounting_numeraire_node_graph_bound =
      !allocation_reserve_graph_unbound;
  allocation.numeraire_weight = 0.35;
  allocation.turnover = 0.12;
  allocation.objective_terms = "growth:0.70,cvar:0.20,cost:0.10";
  allocation.cvar_loss = 0.08;
  allocation.transaction_cost_estimate = 0.003;
  allocation.constraint_diagnostics = "all_constraints_satisfied";
  allocation.cap_diagnostics = "per_node_caps_ok";
  allocation.scenario_growth_floor_status = "met";
  allocation.fallback_reasons =
      allocation_missing_reason_contract ? "" : "none";
  allocation.derisk_reasons = allocation_missing_reason_contract ? "" : "none";
  allocation.observer_belief_fact_digest =
      allocation_observer_digest_mismatch ? "missing_observer_belief_digest"
                                          : observer_digest;
  allocation.forecast_artifact_digest = allocation_forecast_artifact_mismatch
                                            ? "different_forecast_artifact"
                                            : observer.forecast_artifact_digest;
  allocation.base_policy_digest = "base_policy_1";
  ledger.add_allocation_engine(allocation);
  const auto allocation_digest =
      exposure::allocation_engine_fact_digest(allocation);

  exposure::lattice_replay_environment_fact_t replay{};
  replay.parent_exposure_fact_digest = parent_digest;
  replay.contract_fingerprint = parent.contract_fingerprint;
  replay.protocol_id = parent.protocol_id;
  replay.graph_order_fingerprint = parent.graph_order_fingerprint;
  replay.source_cursor_token = parent.source_cursor_token;
  replay.split_policy_fingerprint = parent.split_policy_fingerprint;
  replay.component_assembly_fingerprint = parent.component_assembly_fingerprint;
  replay.target_component_family_id = parent.target_component_family_id;
  replay.job_id = parent.job_id;
  replay.wave_id = parent.wave_id;
  replay.job_status = parent.job_status;
  replay.wave_action = parent.wave_action;
  replay.split_name = parent.split_name;
  replay.split_role = parent.split_role;
  replay.anchor_range = parent.anchor_range;
  replay.completed_anchor_range = parent.completed_anchor_range;
  replay.batch_index_path =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_replay/runtime_replay_batches.index";
  replay.experiment_index_path =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_replay/runtime_replay_experiments.index";
  replay.experiment_report_path =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_replay/replay_validation.report";
  replay.runtime_replay_batch_index_schema =
      replay_schema_mismatch
          ? "wrong.runtime.batch.schema"
          : "kikijyeba.environment.replay.runtime_batch_index.v1";
  replay.runtime_replay_experiment_index_schema =
      replay_schema_mismatch
          ? "wrong.runtime.experiment.schema"
          : "kikijyeba.environment.replay.runtime_experiment_index.v1";
  replay.replay_experiment_report_schema =
      replay_schema_mismatch
          ? "wrong.replay.report.schema"
          : exposure::replay_environment_experiment_report_schema_id();
  replay.experiment_report_digest = "replay_report_digest_1";
  replay.experiment_index_report_digest = replay_report_digest_mismatch
                                              ? "mutated_replay_report_digest"
                                              : replay.experiment_report_digest;
  replay.experiment_id = "replay_validation";
  replay.runtime_run_id = replay_runtime_run_id_missing ? "" : "runtime_run_1";
  replay.environment_run_id = "env_run_1";
  replay.execution_profile_digest = "execution_profile_digest_1";
  replay.policy_set_digest = "policy_set_digest_1";
  replay.replay_environment_version = "kikijyeba.environment.replay.v1";
  replay.replay_environment_component_assembly_id = "replay_environment_v1";
  replay.replay_environment_world_mode =
      replay_world_mode_mismatch ? "paper_live" : "historical_replay";
  replay.replay_environment_api_contract = "rl_compatible_reset_step";
  replay.replay_environment_spawn_model = "episode_parallel_step_sequential";
  replay.replay_environment_range_source =
      replay_contract_mismatch ? "external_window"
                               : "ujcamei_component_stream_cursor";
  replay.replay_environment_source_range_policy = "anchor_index_or_source_key";
  replay.replay_environment_source_order_policy =
      (replay_contract_mismatch || replay_source_order_policy_mismatch)
          ? "random"
          : "sequential";
  replay.replay_environment_range_resolution =
      "runtime_resolved_cursor_identity";
  replay.replay_environment_observation_time_law = "time_t_only";
  replay.replay_environment_realization_reveal = "after_action_execution";
  replay.replay_environment_realization_key_policy = "shared_key_per_frame";
  replay.replay_environment_action_kind = "target_node_weights";
  replay.replay_environment_action_time_policy =
      (replay_contract_mismatch || replay_action_time_policy_mismatch)
          ? "decision_timestamp_unbound"
          : "decision_timestamp_after_knowledge_before_realization";
  replay.replay_environment_graph_node_universe_policy =
      "episode_spec_graph_node_ids";
  replay.replay_environment_reward_policy =
      "post_execution_ledger_log_growth_drawdown_cost_turnover_invalid";
  replay.replay_environment_projection_validation =
      "projected_log_return_vs_realized_asset_numeraire_return";
  replay.replay_environment_policy_surface = "policy_adapter";
  replay.replay_environment_action_policy_identity =
      "policy_adapter_must_match_action";
  replay.replay_environment_initial_policy_kind =
      "deterministic_allocator_or_baseline";
  replay.replay_environment_experiment_task_identity =
      "bundle_policy_task_indices";
  replay.replay_environment_experiment_run_identity =
      "single_runtime_environment_run";
  replay.replay_environment_step_artifact_identity =
      "episode_run_policy_cursor";
  replay.replay_environment_experiment_report_count_policy =
      "counts_match_evidence";
  replay.replay_environment_artifact_schema = "cajtucu_ready_replay_artifacts";
  replay.replay_environment_lattice_fact_family = "replay_environment";
  replay.replay_environment_lattice_target =
      "replay_environment_artifact_ready";
  replay.replay_environment_require_resolved_cursor = true;
  replay.replay_environment_require_no_future_leakage = true;
  replay.replay_environment_require_projection_validation = true;
  replay.replay_environment_default_max_parallel_jobs =
      replay_parallelism_mismatch ? 0 : 1;
  replay.experiment_requested_max_parallel_jobs =
      replay_parallelism_mismatch ? -1 : 1;
  replay.experiment_resolved_parallelism = replay_parallelism_mismatch ? 0 : 1;
  replay.batch_entry_count = 1;
  replay.experiment_entry_count = 1;
  replay.replay_bundle_count = 2;
  replay.policy_count = 2;
  replay.policy_summary_count = replay_missing_policy_summary ? 0 : 2;
  replay.attempted_count = 2;
  replay.completed_count =
      (replay_incomplete_attempts || replay_failed_attempts) ? 1 : 2;
  replay.failed_count = replay_failed_attempts ? 1 : 0;
  replay.episode_count = 2;
  replay.episode_requested_range_bound_count = 2;
  replay.episode_cursor_bound_count = 2;
  replay.episode_anchor_interval_bound_count = 2;
  replay.episode_anchor_keys_bound_count = 2;
  replay.time_law_expected_step_count = replay_expected_step_mismatch ? 5 : 4;
  if (!replay_missing_time_law_steps) {
    replay.time_law_observation_step_count = 4;
    replay.time_law_action_step_count = 4;
    replay.time_law_execution_step_count = 4;
    replay.time_law_realization_after_action_count = 4;
  }
  replay.time_law_future_observation_violation_count =
      replay_future_observation_violation ? 1 : 0;
  replay.mixed_future_realization_key_count = replay_mixed_future_keys ? 1 : 0;
  if (!replay_missing_projection_step_evidence) {
    replay.projection_validation_step_count = 4;
  }
  replay.cajtucu_valid_trace_count = 4;
  replay.cajtucu_invalid_trace_count = 0;
  replay.cajtucu_missing_direct_pair_count = 0;
  replay.cajtucu_nontradable_edge_reject_count = 0;
  replay.cajtucu_below_min_notional_reject_count = 0;
  replay.cajtucu_above_max_notional_reject_count = 0;
  replay.cajtucu_insufficient_sell_units_reject_count = 0;
  replay.cajtucu_insufficient_units_reject_count = 0;
  replay.cajtucu_invalid_sell_price_count = 0;
  replay.cajtucu_large_equity_mismatch_count = 0;
  replay.cajtucu_synthetic_market_step_count = 0;
  replay.requested_order_count = 4;
  replay.executed_order_count = 4;
  replay.rejected_order_count = 0;
  replay.partial_order_count = 0;
  replay.mean_total_reward = 0.12;
  replay.mean_total_log_growth = 0.03;
  replay.mean_final_equity_numeraire = 103.0;
  replay.mean_max_drawdown = 0.08;
  replay.mean_total_turnover = 0.40;
  replay.mean_total_transaction_cost_numeraire = 0.015;
  replay.requested_notional_numeraire = 10.0;
  replay.executed_notional_numeraire = 9.8;
  replay.rejected_notional_numeraire = 0.0;
  replay.fill_ratio = 0.98;
  replay.fee_cost_numeraire = 0.006;
  replay.spread_cost_numeraire = 0.005;
  replay.slippage_cost_numeraire = 0.004;
  replay.mean_target_weight_error_l1 = 0.02;
  replay.mean_target_weight_error_linf = 0.01;
  if (!replay_missing_projection_metrics) {
    replay.mean_projection_mae = 0.012;
    replay.mean_projection_signed_bias = -0.002;
    replay.mean_projection_directional_accuracy = 0.75;
    replay.mean_projection_interval_coverage = 0.80;
  }
  replay.replay_executor = replay_authority_drift;
  ledger.add_replay_environment(replay);
  const auto replay_digest = exposure::replay_environment_fact_digest(replay);

  exposure::lattice_policy_training_fact_t policy_training{};
  policy_training.parent_exposure_fact_digest = parent_digest;
  policy_training.contract_fingerprint = parent.contract_fingerprint;
  policy_training.protocol_id = parent.protocol_id;
  policy_training.graph_order_fingerprint = parent.graph_order_fingerprint;
  policy_training.source_cursor_token = parent.source_cursor_token;
  policy_training.split_policy_fingerprint = parent.split_policy_fingerprint;
  policy_training.component_assembly_fingerprint =
      parent.component_assembly_fingerprint;
  policy_training.target_component_family_id =
      policy_training_component_is_policy ? "wikimyei.policy.trainable"
                                          : parent.target_component_family_id;
  policy_training.job_id = parent.job_id;
  policy_training.wave_id = parent.wave_id;
  policy_training.job_status = parent.job_status;
  policy_training.wave_action = parent.wave_action;
  policy_training.split_name = parent.split_name;
  policy_training.split_role = parent.split_role;
  policy_training.anchor_range = parent.anchor_range;
  policy_training.completed_anchor_range = parent.completed_anchor_range;
  policy_training.policy_id = "wikimyei.policy.rl.ppo_portfolio.v0";
  policy_training.policy_kind = "ppo_policy_adapter";
  policy_training.policy_architecture_digest = "policy_architecture_1";
  policy_training.training_config_digest = "policy_training_config_1";
  policy_training.training_range_digest = "train_range_digest_1";
  policy_training.validation_range_digest = "validation_range_digest_1";
  policy_training.test_range_digest = "test_range_digest_1";
  policy_training.environment_contract_id = "kikijyeba.environment.replay.v1";
  policy_training.observation_schema_digest =
      "kikijyeba.environment.policy_input.v1";
  policy_training.action_schema_digest =
      "kikijyeba.environment.action.target_node_weights.v1";
  policy_training.reward_contract_digest =
      "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
      "drawdown.v1";
  policy_training.policy_input_schema_id =
      "kikijyeba.environment.policy_input.v1";
  policy_training.action_adapter_id = "target_node_weights_simplex.v1";
  policy_training.reward_contract_id =
      "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
      "drawdown.v1";
  policy_training.execution_profile_digest = replay.execution_profile_digest;
  policy_training.training_schedule_mode = "causal_walk_forward_training.v1";
  policy_training.causal_schedule_schema_id =
      "kikijyeba.runtime.policy_training_causal_schedule.v1";
  policy_training.causal_schedule_digest = "causal_schedule_digest_1";
  policy_training.causal_schedule_cursor_key_kind = "numeric_anchor_index";
  policy_training.causal_schedule_no_future_snapshot_use_source =
      "derived_from_artifact_fit_use_ledgers";
  policy_training.normalization_fit_range_digest =
      policy_training.training_range_digest;
  policy_training.replay_buffer_source_range_digest =
      policy_training.training_range_digest;
  policy_training.early_stopping_policy_digest = "early_stopping_validation_1";
  policy_training.hyperparameter_selection_policy_digest =
      "hyperparameter_selection_validation_1";
  policy_training.selector_split = "validation";
  policy_training.selector_policy_digest = "selector_policy_1";
  policy_training.parent_checkpoint_digest = "sdu_seed_checkpoint_1";
  policy_training.checkpoint_digest = "ppo_checkpoint_1";
  policy_training.parent_forecast_eval_fact_digest = forecast_digest;
  policy_training.parent_observer_belief_fact_digest = observer_digest;
  policy_training.parent_allocation_engine_fact_digest = allocation_digest;
  policy_training.parent_replay_environment_fact_digest = replay_digest;
  policy_training.random_seed = 0;
  policy_training.random_seed_bound = true;
  policy_training.training_range_disjoint_validation = true;
  policy_training.training_range_disjoint_test = true;
  policy_training.validation_range_disjoint_test = true;
  policy_training.normalization_fit_training_only = true;
  policy_training.replay_buffer_training_only = true;
  policy_training.reward_baseline_training_only = true;
  policy_training.early_stopping_uses_validation_only = true;
  policy_training.hyperparameter_selection_uses_validation_only = true;
  policy_training.test_sealed_until_final_report = true;
  policy_training.test_first_access_after_selection = true;
  policy_training.runtime_job_kind_bound = true;
  policy_training.policy_checkpoint_written = true;
  policy_training.causal_schedule_readiness_eligible = true;
  policy_training.causal_schedule_no_future_snapshot_use =
      !policy_training_future_snapshot_use;
  policy_training.offline_full_window_research = false;
  ledger.add_policy_training(policy_training);

  return ledger;
}

exposure::lattice_exposure_ledger_t artifact_readiness_ledger_with_proof_fault(
    bool transform_missing_support_surface = false,
    bool baseline_missing_support = false,
    bool forecast_missing_target_transform_binding = false,
    bool forecast_missing_baseline_binding = false,
    bool forecast_missing_evaluated_checkpoint_lineage = false,
    bool forecast_model_state_mutation = false) {
  return artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/false,
      /*replay_contract_mismatch=*/false,
      /*replay_incomplete_attempts=*/false,
      /*replay_missing_projection_metrics=*/false,
      /*forecast_missing_horizon_support=*/false,
      /*replay_missing_time_law_steps=*/false,
      /*replay_future_observation_violation=*/false,
      /*replay_mixed_future_keys=*/false,
      /*replay_missing_projection_step_evidence=*/false,
      /*replay_expected_step_mismatch=*/false,
      /*replay_action_time_policy_mismatch=*/false,
      /*replay_source_order_policy_mismatch=*/false,
      /*replay_parallelism_mismatch=*/false,
      /*replay_world_mode_mismatch=*/false,
      /*replay_schema_mismatch=*/false,
      /*replay_runtime_run_id_missing=*/false,
      /*replay_report_digest_mismatch=*/false,
      transform_missing_support_surface, baseline_missing_support,
      forecast_missing_target_transform_binding,
      forecast_missing_baseline_binding,
      forecast_missing_evaluated_checkpoint_lineage,
      forecast_model_state_mutation);
}

target::lattice_target_evaluator_options_t
artifact_eval_options(const exposure::lattice_exposure_ledger_t &ledger) {
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_artifact_readiness";
  options.exposure_ledger = &ledger;
  options.auto_build_exposure_ledger = false;
  options.active_identity.protocol_id = "cwu_02v";
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_1";
  options.active_identity.mdn_assembly_fingerprint = "mdn_1";
  return options;
}

target::lattice_target_evaluator_options_t
artifact_runtime_eval_options(const fs::path &runtime_root) {
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = runtime_root;
  options.auto_build_exposure_ledger = true;
  options.active_identity.protocol_id = "cwu_02v";
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_1";
  options.active_identity.mdn_assembly_fingerprint = "mdn_1";
  return options;
}

} // namespace

int main() {
  const auto specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = vicreg_ready;
  TARGET_KIND = vicreg_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = mtf_representation_ready;
  TARGET_KIND = mtf_representation_ready;
  COMPONENT = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_ready;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = vicreg_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.01;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};
)DSL");

  check(specs.size() == 3, "active lattice target DSL decodes three targets");
  check(specs[0].kind == target::lattice_target_kind_t::vicreg_ready,
        "VICReg target kind decodes");
  check(specs[1].kind ==
            target::lattice_target_kind_t::mtf_representation_ready,
        "MTF representation target kind decodes");
  check(specs[2].kind == target::lattice_target_kind_t::channel_mdn_ready,
        "channel MDN target kind decodes");
  check(target::lattice_target_kind_name(specs[0].kind) ==
            std::string("vicreg_ready"),
        "VICReg target kind has canonical name");
  check(target::lattice_target_kind_name(specs[1].kind) ==
            std::string("mtf_representation_ready"),
        "MTF representation target kind has canonical name");
  check(target::lattice_target_kind_name(specs[2].kind) ==
            std::string("channel_mdn_ready"),
        "channel MDN target kind has canonical name");
  check(target::default_component_for_target_kind(specs[0].kind) ==
            "wikimyei.representation.encoding.vicreg",
        "VICReg target kind maps to representation component family");
  check(target::default_component_for_target_kind(specs[1].kind) ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "MTF target kind maps to MTF representation component family");
  check(target::default_component_for_target_kind(specs[2].kind) ==
            "wikimyei.inference.expected_value.mdn",
        "channel MDN target kind maps to MDN component family");
  const auto digest_policy_summary =
      target::lattice_proof_certificate_digest_policy_summary();
  check(
      digest_policy_summary.summary_self_check_passed &&
          digest_policy_summary.policy_count == 12 &&
          digest_policy_summary.digest_contributing_count == 9 &&
          digest_policy_summary.status_authority_count == 8 &&
          std::find(digest_policy_summary.digest_contributing_surfaces.begin(),
                    digest_policy_summary.digest_contributing_surfaces.end(),
                    "artifact_proofs") !=
              digest_policy_summary.digest_contributing_surfaces.end(),
      "proof certificate digest policy includes artifact proofs as a hashed "
      "status surface");

  const auto active_specs = target::decode_lattice_targets_from_dsl(
      read_active_lattice_targets_dsl());
  const auto active_policy_gates = target::decode_lattice_policy_gates_from_dsl(
      read_active_lattice_targets_dsl());
  check(
      active_policy_gates.size() == 2 &&
          std::all_of(active_policy_gates.begin(), active_policy_gates.end(),
                      [](const auto &gate) { return !gate.enabled; }) &&
          std::any_of(
              active_policy_gates.begin(), active_policy_gates.end(),
              [](const auto &gate) {
                return gate.policy_id ==
                           "forecast_quality_acceptance_reserved" &&
                       gate.target_id == "forecast_eval_artifact_ready" &&
                       gate.policy_kind == "forecast_quality_acceptance" &&
                       gate.metric == "skill_vs_baseline" &&
                       gate.metric_definition ==
                           "skill_vs_baseline_same_transform_horizon_"
                           "support" &&
                       gate.baseline_definition ==
                           "same_protocol_split_transform_horizon_"
                           "baseline_fact" &&
                       gate.uncertainty_model ==
                           "disabled_until_support_and_calibration_"
                           "policy_exists" &&
                       gate.negative_tests ==
                           "required_selection_leakage_and_negative_"
                           "skill_tests" &&
                       gate.calibration_requirements ==
                           "pit_interval_and_sigma_scale_visibility_"
                           "required" &&
                       gate.holdout_declaration ==
                           "validation_holdout_out_of_sample" &&
                       gate.threshold_selection_audit ==
                           "disabled_no_threshold_tuning_audit" &&
                       target::lattice_policy_gate_input_contract_complete(
                           gate) &&
                       gate.status == "disabled_reserved" &&
                       !gate.policy_fingerprint.empty();
              }) &&
          std::any_of(
              active_policy_gates.begin(), active_policy_gates.end(),
              [](const auto &gate) {
                return gate.policy_id == "allocation_acceptance_reserved" &&
                       gate.target_id == "allocation_artifact_ready" &&
                       gate.policy_kind == "allocation_acceptance" &&
                       gate.metric == "risk_adjusted_growth_after_cost" &&
                       gate.metric_definition ==
                           "replay_or_policy_declared_risk_adjusted_"
                           "growth_after_cost" &&
                       gate.baseline_definition ==
                           "base_policy_accounting_numeraire_graph_node_"
                           "reference" &&
                       gate.uncertainty_model ==
                           "disabled_until_replay_uncertainty_policy_"
                           "exists" &&
                       gate.negative_tests ==
                           "required_cost_cvar_fallback_and_derisk_"
                           "negative_tests" &&
                       gate.calibration_requirements ==
                           "observer_confidence_data_quality_"
                           "liquidity_required" &&
                       gate.holdout_declaration ==
                           "validation_holdout_out_of_sample" &&
                       gate.threshold_selection_audit ==
                           "disabled_no_threshold_tuning_audit" &&
                       target::lattice_policy_gate_input_contract_complete(
                           gate) &&
                       gate.status == "disabled_reserved" &&
                       !gate.policy_fingerprint.empty();
              }),
      "active target catalog may declare disabled future policy gates without "
      "making them target proofs");
  const auto find_active_spec = [&](const std::string &target_id)
      -> const target::lattice_target_spec_t & {
    const auto it = std::find_if(
        active_specs.begin(), active_specs.end(),
        [&](const auto &spec) { return spec.target_id == target_id; });
    check(it != active_specs.end(),
          "active lattice target DSL declares " + target_id);
    return *it;
  };
  const auto &active_transform =
      find_active_spec("target_transform_contract_ready");
  const auto &active_baseline =
      find_active_spec("forecast_baseline_artifact_ready");
  const auto &active_forecast =
      find_active_spec("forecast_eval_artifact_ready");
  const auto &active_observer =
      find_active_spec("observer_belief_artifact_ready");
  const auto &active_allocation = find_active_spec("allocation_artifact_ready");
  const auto &active_replay =
      find_active_spec("replay_environment_artifact_ready");
  const auto &active_policy_training =
      find_active_spec("policy_training_artifact_ready");
  check(active_transform.target_class == "artifact_readiness" &&
            active_transform.subject_fact_family == "target_transform" &&
            active_transform.proof_kind == "target_transform_contract_bound" &&
            active_transform.protocol_id == "cwu_02v" &&
            active_transform.train_split == "validation_holdout" &&
            active_transform.checkpoint_source == "none" &&
            active_transform.max_waves == 0,
        "active target catalog exposes target-transform contract readiness as "
        "a non-dispatchable artifact proof");
  check(active_baseline.target_class == "artifact_readiness" &&
            active_baseline.subject_fact_family == "forecast_baseline" &&
            active_baseline.proof_kind == "forecast_baseline_artifact_bound" &&
            active_baseline.warning_specs.size() == 3 &&
            std::any_of(active_baseline.warning_specs.begin(),
                        active_baseline.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_baseline" &&
                                 w.metric == "valid_count";
                        }) &&
            std::any_of(active_baseline.warning_specs.begin(),
                        active_baseline.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_baseline" &&
                                 w.metric == "baseline_kind_count";
                        }) &&
            std::any_of(active_baseline.warning_specs.begin(),
                        active_baseline.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_baseline" &&
                                 w.metric == "computed_metric_fact_count";
                        }) &&
            active_forecast.target_class == "artifact_readiness" &&
            active_forecast.subject_fact_family == "forecast_eval" &&
            active_forecast.proof_kind == "forecast_eval_artifact_bound" &&
            active_forecast.warning_specs.size() == 3 &&
            std::any_of(active_forecast.warning_specs.begin(),
                        active_forecast.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_eval" &&
                                 w.metric == "calibration_coverage";
                        }) &&
            std::any_of(active_forecast.warning_specs.begin(),
                        active_forecast.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_eval" &&
                                 w.metric == "skill_vs_baseline";
                        }) &&
            std::any_of(active_forecast.warning_specs.begin(),
                        active_forecast.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "forecast_eval" &&
                                 w.metric == "weakest_horizon_support_rows";
                        }) &&
            active_observer.target_class == "artifact_readiness" &&
            active_observer.subject_fact_family == "observer_belief" &&
            active_observer.proof_kind == "observer_belief_artifact_bound" &&
            active_observer.warning_specs.size() == 3 &&
            std::any_of(active_observer.warning_specs.begin(),
                        active_observer.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "observer_belief" &&
                                 w.metric == "confidence";
                        }) &&
            std::any_of(active_observer.warning_specs.begin(),
                        active_observer.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "observer_belief" &&
                                 w.metric == "data_quality";
                        }) &&
            std::any_of(active_observer.warning_specs.begin(),
                        active_observer.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "observer_belief" &&
                                 w.metric == "liquidity";
                        }) &&
            active_allocation.target_class == "artifact_readiness" &&
            active_allocation.subject_fact_family == "allocation_engine" &&
            active_allocation.proof_kind == "allocation_artifact_bound" &&
            active_allocation.warning_specs.size() == 5 &&
            std::any_of(active_allocation.warning_specs.begin(),
                        active_allocation.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "allocation_engine" &&
                                 w.metric == "turnover";
                        }) &&
            std::any_of(active_allocation.warning_specs.begin(),
                        active_allocation.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "allocation_engine" &&
                                 w.metric == "cap_diagnostics_bound";
                        }) &&
            std::any_of(active_allocation.warning_specs.begin(),
                        active_allocation.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "allocation_engine" &&
                                 w.metric == "scenario_growth_floor_attention";
                        }) &&
            std::any_of(active_allocation.warning_specs.begin(),
                        active_allocation.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "allocation_engine" &&
                                 w.metric == "fallback_active";
                        }) &&
            std::any_of(active_allocation.warning_specs.begin(),
                        active_allocation.warning_specs.end(),
                        [](const auto &w) {
                          return w.kind == "allocation_engine" &&
                                 w.metric == "derisk_active";
                        }) &&
            active_replay.target_class == "artifact_readiness" &&
            active_replay.subject_fact_family == "replay_environment" &&
            active_replay.proof_kind == "replay_environment_artifact_bound" &&
            active_replay.protocol_id == "cwu_02v" &&
            active_replay.train_split == "validation_holdout" &&
            active_replay.checkpoint_source == "none" &&
            active_replay.plan_mode == "none" && active_replay.max_waves == 0 &&
            active_replay.warning_specs.empty() &&
            active_policy_training.target_class == "artifact_readiness" &&
            active_policy_training.subject_fact_family == "policy_training" &&
            active_policy_training.proof_kind ==
                "policy_training_artifact_bound" &&
            active_policy_training.protocol_id == "cwu_02v" &&
            active_policy_training.component == "wikimyei.policy.trainable" &&
            active_policy_training.train_split == "validation_holdout" &&
            active_policy_training.checkpoint_source == "none" &&
            active_policy_training.plan_mode == "none" &&
            active_policy_training.max_waves == 0 &&
            active_policy_training.warning_specs.empty(),
        "active target catalog exposes roadmap evidence families through "
        "artifact proofs, not TARGET_KIND expansion");
  const auto &artifact_proof_templates =
      target::lattice_artifact_readiness_proof_templates();
  check(artifact_proof_templates.size() == 7 &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "forecast_eval") != nullptr &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "forecast_eval")
                    ->proof_kind == "forecast_eval_artifact_bound" &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "replay_environment") != nullptr &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "replay_environment")
                    ->proof_kind == "replay_environment_artifact_bound" &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "policy_training") != nullptr &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "policy_training")
                    ->proof_kind == "policy_training_artifact_bound" &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "source_analytics") == nullptr &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "selection_signal") == nullptr &&
            target::artifact_readiness_proof_template_for_subject_fact_family(
                "representation_support") == nullptr,
        "artifact-readiness proof templates explicitly enumerate proofable "
        "fact families and leave source analytics, selection signals, "
        "and representation support as non-promotable catalog evidence");

  const auto artifact_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = observer_belief_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = observer_belief;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_WARN {
  TARGET_ID = observer_belief_artifact_ready;
  WARNING_ID = observer_belief_confidence_low_visibility_only;
  KIND = observer_belief;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = confidence;
  BELOW = 0.70;
};

LATTICE_TARGET {
  TARGET_ID = target_transform_contract_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = target_transform;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = forecast_baseline_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_baseline;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_WARN {
  TARGET_ID = forecast_baseline_artifact_ready;
  WARNING_ID = forecast_baseline_valid_support_low_visibility_only;
  KIND = forecast_baseline;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = valid_count;
  BELOW = 50;
};

LATTICE_WARN {
  TARGET_ID = forecast_baseline_artifact_ready;
  WARNING_ID = forecast_baseline_kind_coverage_low_visibility_only;
  KIND = forecast_baseline;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = baseline_kind_count;
  BELOW = 4;
};

LATTICE_WARN {
  TARGET_ID = forecast_baseline_artifact_ready;
  WARNING_ID = forecast_baseline_metric_coverage_low_visibility_only;
  KIND = forecast_baseline;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = computed_metric_fact_count;
  BELOW = 1;
};

LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_WARN {
  TARGET_ID = forecast_eval_artifact_ready;
  WARNING_ID = forecast_eval_calibration_coverage_low_visibility_only;
  KIND = forecast_eval;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = calibration_coverage;
  BELOW = 0.95;
};

LATTICE_WARN {
  TARGET_ID = forecast_eval_artifact_ready;
  WARNING_ID = forecast_eval_skill_vs_baseline_negative_visibility_only;
  KIND = forecast_eval;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = skill_vs_baseline;
  BELOW = 0.0;
};

LATTICE_WARN {
  TARGET_ID = forecast_eval_artifact_ready;
  WARNING_ID = forecast_eval_horizon_support_low_visibility_only;
  KIND = forecast_eval;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = weakest_horizon_support_rows;
  BELOW = 10;
};

LATTICE_TARGET {
  TARGET_ID = allocation_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  PROOF_KIND = allocation_artifact_bound;
  SUBJECT_FACT_FAMILY = allocation_engine;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = replay_environment_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = replay_environment;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_TARGET {
  TARGET_ID = policy_training_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  PROOF_KIND = policy_training_artifact_bound;
  SUBJECT_FACT_FAMILY = policy_training;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};

LATTICE_WARN {
  TARGET_ID = allocation_artifact_ready;
  WARNING_ID = allocation_turnover_high_visibility_only;
  KIND = allocation_engine;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  SCOPE = target_component_family_id;
  METRIC = turnover;
  ABOVE = 0.10;
};
)DSL");

  check(artifact_specs.size() == 7,
        "artifact readiness targets decode without TARGET_KIND expansion");
  check(artifact_specs[0].target_class == "artifact_readiness" &&
            artifact_specs[0].subject_fact_family == "observer_belief" &&
            artifact_specs[0].proof_kind == "observer_belief_artifact_bound" &&
            artifact_specs[0].kind ==
                target::lattice_target_kind_t::not_applicable &&
            !artifact_specs[0].target_kind_applicable &&
            target::lattice_target_effective_kind_name(artifact_specs[0]) ==
                "not_applicable" &&
            target::lattice_target_effective_kind_selector(artifact_specs[0]) ==
                "none" &&
            artifact_specs[0].checkpoint_source == "none" &&
            artifact_specs[0].max_waves == 0 &&
            artifact_specs[0].warning_specs.size() == 1 &&
            artifact_specs[0].warning_specs.front().kind == "observer_belief" &&
            !artifact_specs[0].require_checkpoint_exists &&
            !artifact_specs[0].require_finite_loss,
        "observer artifact readiness defaults to fact proof, no checkpoint, "
        "and no wave dispatch");
  check(
      artifact_specs[1].target_class == "artifact_readiness" &&
          artifact_specs[1].subject_fact_family == "target_transform" &&
          artifact_specs[1].proof_kind == "target_transform_contract_bound" &&
          artifact_specs[1].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[1].target_kind_applicable &&
          artifact_specs[1].checkpoint_source == "none" &&
          artifact_specs[1].plan_mode == "none" &&
          artifact_specs[1].max_waves == 0 &&
          artifact_specs[2].subject_fact_family == "forecast_baseline" &&
          artifact_specs[2].proof_kind == "forecast_baseline_artifact_bound" &&
          artifact_specs[2].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[2].target_kind_applicable &&
          artifact_specs[2].checkpoint_source == "none" &&
          artifact_specs[2].plan_mode == "none" &&
          artifact_specs[2].max_waves == 0 &&
          artifact_specs[2].warning_specs.size() == 3 &&
          std::all_of(
              artifact_specs[2].warning_specs.begin(),
              artifact_specs[2].warning_specs.end(),
              [](const auto &w) { return w.kind == "forecast_baseline"; }) &&
          artifact_specs[3].subject_fact_family == "forecast_eval" &&
          artifact_specs[3].proof_kind == "forecast_eval_artifact_bound" &&
          artifact_specs[3].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[3].target_kind_applicable &&
          artifact_specs[3].checkpoint_source == "none" &&
          artifact_specs[3].plan_mode == "none" &&
          artifact_specs[3].max_waves == 0 &&
          artifact_specs[3].warning_specs.size() == 3 &&
          std::all_of(
              artifact_specs[3].warning_specs.begin(),
              artifact_specs[3].warning_specs.end(),
              [](const auto &w) { return w.kind == "forecast_eval"; }) &&
          artifact_specs[4].target_class == "artifact_readiness" &&
          artifact_specs[4].subject_fact_family == "allocation_engine" &&
          artifact_specs[4].proof_kind == "allocation_artifact_bound" &&
          artifact_specs[4].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[4].target_kind_applicable &&
          artifact_specs[4].checkpoint_source == "none" &&
          artifact_specs[4].plan_mode == "none" &&
          artifact_specs[4].max_waves == 0 &&
          artifact_specs[4].warning_specs.size() == 1 &&
          artifact_specs[4].warning_specs.front().kind == "allocation_engine" &&
          artifact_specs[5].target_class == "artifact_readiness" &&
          artifact_specs[5].subject_fact_family == "replay_environment" &&
          artifact_specs[5].proof_kind == "replay_environment_artifact_bound" &&
          artifact_specs[5].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[5].target_kind_applicable &&
          artifact_specs[5].checkpoint_source == "none" &&
          artifact_specs[5].plan_mode == "none" &&
          artifact_specs[5].max_waves == 0 &&
          artifact_specs[6].target_class == "artifact_readiness" &&
          artifact_specs[6].subject_fact_family == "policy_training" &&
          artifact_specs[6].proof_kind == "policy_training_artifact_bound" &&
          artifact_specs[6].kind ==
              target::lattice_target_kind_t::not_applicable &&
          !artifact_specs[6].target_kind_applicable &&
          artifact_specs[6].checkpoint_source == "none" &&
          artifact_specs[6].plan_mode == "none" &&
          artifact_specs[6].max_waves == 0,
      "allocation artifact readiness keeps proof kind separate from target "
      "kind");

  const auto artifact_canonical =
      target::canonical_lattice_target_spec_text(artifact_specs[0]);
  check(artifact_canonical.find("target_kind_applicable=false\n") !=
                std::string::npos &&
            artifact_canonical.find("kind=not_applicable\n") !=
                std::string::npos &&
            artifact_canonical.find("target_kind_effective=none\n") !=
                std::string::npos &&
            artifact_canonical.find("kind=channel_mdn_ready\n") ==
                std::string::npos,
        "artifact target canonical identity must not carry the legacy "
        "channel_mdn_ready default kind");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_not_applicable_kind;
  TARGET_KIND = not_applicable;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "TARGET_KIND not_applicable is internal to "
                      "artifact_readiness",
                      "not_applicable remains an internal sentinel, not a "
                      "declared target kind");

  const auto replay_artifact_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = replay_environment_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = replay_environment;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};
)DSL");
  check(replay_artifact_specs.size() == 1 &&
            replay_artifact_specs.front().proof_kind ==
                "replay_environment_artifact_bound" &&
            replay_artifact_specs.front().checkpoint_source == "none" &&
            replay_artifact_specs.front().max_waves == 0,
        "replay environment is now an explicit non-dispatchable artifact "
        "readiness proof template");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_artifact_target_kind;
  TARGET_CLASS = artifact_readiness;
  TARGET_KIND = channel_mdn_ready;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "artifact_readiness cannot declare TARGET_KIND",
                      "artifact readiness must not carry a trainable target "
                      "kind");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_source_analytics_target;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = source_analytics;
  PROOF_KIND = source_analytics_artifact_bound;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "has no artifact proof template",
                      "fact families must not become artifact-readiness "
                      "targets without an explicit proof template");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_selection_signal_target;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = selection_signal;
  PROOF_KIND = selection_signal_artifact_bound;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "has no artifact proof template",
                      "selection signals are leakage-visibility facts and must "
                      "not become artifact-readiness targets");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_representation_support_target;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = representation_support;
  PROOF_KIND = representation_support_artifact_bound;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "has no artifact proof template",
                      "representation support is visibility evidence and must "
                      "not become an artifact-readiness target");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mismatched_forecast_eval_proof;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  PROOF_KIND = forecast_quality_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "PROOF_KIND forecast_quality_ready does not match "
                      "SUBJECT_FACT_FAMILY forecast_eval",
                      "artifact-readiness proof kind must match its explicit "
                      "fact-family proof template");

  target::lattice_target_spec_t manual_bad_artifact_template{};
  manual_bad_artifact_template.target_id = "manual_source_analytics_target";
  manual_bad_artifact_template.target_class = "artifact_readiness";
  manual_bad_artifact_template.target_kind_applicable = false;
  manual_bad_artifact_template.subject_fact_family = "source_analytics";
  manual_bad_artifact_template.proof_kind = "source_analytics_artifact_bound";
  manual_bad_artifact_template.checkpoint_source = "none";
  manual_bad_artifact_template.plan_mode = "none";
  manual_bad_artifact_template.max_waves = 0;
  manual_bad_artifact_template.require_contract_match = false;
  manual_bad_artifact_template.require_checkpoint_exists = false;
  manual_bad_artifact_template.require_finite_loss = false;
  manual_bad_artifact_template.require_component_match = false;
  target::lattice_target_evaluator_t manual_bad_artifact_evaluator(
      {manual_bad_artifact_template});
  const auto manual_bad_artifact_eval =
      manual_bad_artifact_evaluator.evaluate("manual_source_analytics_target");
  check(manual_bad_artifact_eval.status ==
                target::lattice_target_status_t::blocked &&
            !manual_bad_artifact_eval.plan_ready &&
            manual_bad_artifact_eval.suggested_wave.empty() &&
            has_reason_containing(manual_bad_artifact_eval.reasons,
                                  "has no artifact proof template"),
        "manual artifact-readiness specs that bypass DSL still cannot promote "
        "warning-only fact families into target proofs");

  target::lattice_target_spec_t manual_missing_kind{};
  manual_missing_kind.target_id = "manual_missing_target_kind";
  target::lattice_target_evaluator_t manual_missing_kind_evaluator(
      {manual_missing_kind});
  const auto manual_missing_kind_eval =
      manual_missing_kind_evaluator.evaluate("manual_missing_target_kind");
  check(manual_missing_kind_eval.status ==
                target::lattice_target_status_t::blocked &&
            manual_missing_kind_eval.kind ==
                target::lattice_target_kind_t::not_applicable &&
            !target::lattice_target_kind_applicable(manual_missing_kind_eval) &&
            target::lattice_target_effective_kind_name(
                manual_missing_kind_eval) == "not_applicable" &&
            has_reason_containing(manual_missing_kind_eval.reasons,
                                  "concrete TARGET_KIND"),
        "manual non-artifact specs that bypass DSL must not inherit a "
        "training-readiness target kind");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_artifact_wave;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  WAVE_MODE = run|debug;
};
)DSL",
                      "artifact_readiness targets must use WAVE_MODE=none",
                      "artifact readiness must not declare Runtime wave mode");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_artifact_attempts;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  PLAN_MAX_ATTEMPTS = 1;
};
)DSL",
                      "artifact_readiness targets must use "
                      "PLAN_MAX_ATTEMPTS=0",
                      "artifact readiness must not declare dispatch attempts");

  expect_decode_error(
      artifact_readiness_target_dsl("  REQUIRE_CHECKPOINT_EXISTS = true;\n"),
      "artifact_readiness targets cannot require checkpoint existence",
      "artifact readiness must not inherit checkpoint existence readiness");

  expect_decode_error(
      artifact_readiness_target_dsl("  REQUIRE_FINITE_LOSS = true;\n"),
      "artifact_readiness targets cannot require finite loss",
      "artifact readiness must not inherit finite-loss readiness");

  expect_decode_error(
      artifact_readiness_target_dsl("  MIN_OPTIMIZER_STEPS = 1;\n"),
      "artifact_readiness targets cannot require optimizer steps",
      "artifact readiness must not inherit training effort thresholds");

  expect_decode_error(
      artifact_readiness_target_dsl("  MIN_VALID_TARGET_FRACTION = 0.5;\n"),
      "artifact_readiness targets cannot require valid-target readiness "
      "thresholds",
      "artifact readiness must not inherit valid-target readiness thresholds");

  expect_decode_error(
      artifact_readiness_target_dsl("  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;\n"),
      "artifact_readiness targets cannot require MDN node head counts",
      "artifact readiness must not inherit MDN node-head readiness");

  expect_decode_error(
      artifact_readiness_target_dsl("  MIN_OBSERVED_INPUT_COVERAGE = 0.8;\n"),
      "artifact_readiness targets cannot require exposure coverage thresholds",
      "artifact readiness must not inherit exposure coverage thresholds");

  expect_decode_error(
      artifact_readiness_target_dsl("  UPSTREAM_TARGET_ID = "
                                    "channel_mdn_train_core_ready;\n"),
      "artifact_readiness targets cannot declare UPSTREAM_TARGET_ID or "
      "LATTICE_DEPENDS",
      "artifact readiness must not inherit target dependency scheduling");

  expect_decode_error(
      artifact_readiness_target_dsl("",
                                    R"DSL(
LATTICE_REQUIRES {
  TARGET_ID = accidental_artifact_training_knob;
  KIND = artifact;
  ARTIFACT = output_checkpoint;
  EXISTS = true;
};
)DSL"),
      "artifact_readiness targets cannot require checkpoint existence",
      "artifact readiness must reject lowered checkpoint requirements");

  expect_decode_error(
      artifact_readiness_target_dsl("",
                                    R"DSL(
LATTICE_REQUIRES {
  TARGET_ID = accidental_artifact_training_knob;
  KIND = effort;
  COUNTER = optimizer_steps;
  OP = ge;
  VALUE = 1;
};
)DSL"),
      "artifact_readiness targets cannot require optimizer steps",
      "artifact readiness must reject lowered training effort requirements");

  expect_decode_error(
      artifact_readiness_target_dsl("",
                                    R"DSL(
LATTICE_REQUIRES {
  TARGET_ID = accidental_artifact_training_knob;
  KIND = exposure_coverage;
  USE = observed_input;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.8;
};
)DSL"),
      "artifact_readiness targets cannot require exposure coverage thresholds",
      "artifact readiness must reject lowered exposure coverage requirements");

  expect_decode_error(
      artifact_readiness_target_dsl("",
                                    R"DSL(
LATTICE_DEPENDS {
  TARGET_ID = accidental_artifact_training_knob;
  UPSTREAM_TARGET_ID = channel_mdn_train_core_ready;
  BINDING = target;
};
)DSL"),
      "artifact_readiness targets cannot declare UPSTREAM_TARGET_ID or "
      "LATTICE_DEPENDS",
      "artifact readiness must reject lowered target dependencies");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = channel_mdn_ready;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:forecast_eval_artifact_ready;
};
)DSL",
                      "latest_satisfying cannot reference "
                      "TARGET_CLASS=artifact_readiness",
                      "evaluated checkpoint selection must not point at "
                      "artifact-readiness facts");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_from_artifact;
  TARGET_KIND = channel_mdn_ready;
  CHECKPOINT_SOURCE = latest_satisfying:forecast_eval_artifact_ready;
};
)DSL",
                      "latest_satisfying cannot reference "
                      "TARGET_CLASS=artifact_readiness",
                      "checkpoint source selection must not point at "
                      "artifact-readiness facts");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = channel_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = channel_mdn_ready;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_ready;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_ready;
  TARGET_KIND = channel_mdn_ready;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:channel_mdn_validation_eval_ready;
};
)DSL",
                      "latest_satisfying cannot reference "
                      "TARGET_CLASS=evaluation_readiness",
                      "plan checkpoint hints must not point at evaluation "
                      "targets as if they produced checkpoints");

  const auto artifact_ledger = artifact_readiness_ledger();
  check(!artifact_ledger.replay_environment_facts().empty() &&
            artifact_ledger.replay_environment_facts()
                    .front()
                    .replay_environment_source_order_policy == "sequential",
        "replay fixture should bind sequential source order policy, got=" +
            (artifact_ledger.replay_environment_facts().empty()
                 ? std::string("<none>")
                 : artifact_ledger.replay_environment_facts()
                       .front()
                       .replay_environment_source_order_policy));
  target::lattice_target_evaluator_t artifact_evaluator(
      artifact_specs, artifact_eval_options(artifact_ledger));
  const auto observer_eval =
      artifact_evaluator.evaluate("observer_belief_artifact_ready");
  check(observer_eval.status == target::lattice_target_status_t::satisfied &&
            observer_eval.target_class == "artifact_readiness" &&
            !observer_eval.target_kind_applicable &&
            target::lattice_target_effective_kind_name(observer_eval) ==
                "not_applicable" &&
            target::lattice_target_effective_kind_selector(observer_eval) ==
                "none" &&
            observer_eval.proof_kind == "observer_belief_artifact_bound" &&
            observer_eval.subject_fact_family == "observer_belief" &&
            !observer_eval.plan_ready && observer_eval.suggested_wave.empty() &&
            observer_eval.proof_certificate.artifacts.size() == 1 &&
            observer_eval.proof_certificate.artifacts.front().fact_family ==
                "observer_belief" &&
            observer_eval.proof_certificate.artifacts.front().passed &&
            observer_eval.proof_certificate_check.passed &&
            observer_eval.warning_results.size() == 1 &&
            observer_eval.warning_results.front().kind == "observer_belief" &&
            observer_eval.warning_results.front().status == "warning" &&
            observer_eval.warning_results.front().threshold_triggered &&
            observer_eval.warning_results.front().threshold_relation ==
                "below_threshold" &&
            observer_eval.warning_results.front().measured_value == 0.64 &&
            observer_eval.warning_results.front().threshold == 0.70 &&
            observer_eval.warning_results.front().unit == "fraction" &&
            observer_eval.warning_results.front().evidence_basis ==
                "observer_belief_facts" &&
            observer_eval.warning_results.front().use == "observer_belief" &&
            observer_eval.warning_results.front().effect == "none" &&
            observer_eval.warning_results.front().diagnostic_sample_count ==
                1 &&
            observer_eval.warning_summary.triggered_warning_count == 1 &&
            observer_eval.warning_summary.blocking_warning_count == 0 &&
            observer_eval.warning_summary.non_blocking_warning_count == 1 &&
            observer_eval.warning_summary.all_warnings_non_blocking,
        "observer artifact readiness proves a clean observer_belief fact "
        "without training-job readiness semantics");
  check_warning_envelope(observer_eval.warning_results.front(),
                         "observer_belief_artifact_ready",
                         "observer_belief_consistency", "observer_belief",
                         "observer artifact warning");
  check_artifact_proof_no_decision_authority(
      observer_eval.proof_certificate.artifacts.front(),
      "observer_belief_artifact_ready");
  const auto transform_eval =
      artifact_evaluator.evaluate("target_transform_contract_ready");
  check(transform_eval.status == target::lattice_target_status_t::satisfied &&
            transform_eval.proof_kind == "target_transform_contract_bound" &&
            !transform_eval.target_kind_applicable &&
            transform_eval.subject_fact_family == "target_transform" &&
            !transform_eval.plan_ready &&
            transform_eval.suggested_wave.empty() &&
            transform_eval.proof_certificate.artifacts.size() == 1 &&
            transform_eval.proof_certificate.artifacts.front().fact_family ==
                "target_transform" &&
            transform_eval.proof_certificate.artifacts.front().passed &&
            transform_eval.proof_certificate_check.passed,
        "target transform artifact readiness proves transform contract "
        "identity and support-surface lineage without quality authority");
  check_artifact_proof_no_decision_authority(
      transform_eval.proof_certificate.artifacts.front(),
      "target_transform_contract_ready");
  const auto baseline_eval =
      artifact_evaluator.evaluate("forecast_baseline_artifact_ready");
  const auto baseline_support_warning = std::find_if(
      baseline_eval.warning_results.begin(),
      baseline_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_baseline_valid_support_low_visibility_only";
      });
  const auto baseline_kind_warning = std::find_if(
      baseline_eval.warning_results.begin(),
      baseline_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_baseline_kind_coverage_low_visibility_only";
      });
  const auto baseline_metric_warning = std::find_if(
      baseline_eval.warning_results.begin(),
      baseline_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_baseline_metric_coverage_low_visibility_only";
      });
  check(baseline_eval.status == target::lattice_target_status_t::satisfied &&
            baseline_eval.proof_kind == "forecast_baseline_artifact_bound" &&
            !baseline_eval.target_kind_applicable &&
            baseline_eval.subject_fact_family == "forecast_baseline" &&
            !baseline_eval.plan_ready && baseline_eval.suggested_wave.empty() &&
            baseline_eval.proof_certificate.artifacts.size() == 1 &&
            baseline_eval.proof_certificate.artifacts.front().fact_family ==
                "forecast_baseline" &&
            baseline_eval.proof_certificate.artifacts.front().passed &&
            baseline_eval.proof_certificate_check.passed &&
            baseline_eval.warning_results.size() == 3 &&
            baseline_support_warning != baseline_eval.warning_results.end() &&
            baseline_support_warning->kind == "forecast_baseline" &&
            baseline_support_warning->status == "warning" &&
            baseline_support_warning->threshold_triggered &&
            baseline_support_warning->threshold_relation == "below_threshold" &&
            baseline_support_warning->measured_value == 40.0 &&
            baseline_support_warning->threshold == 50.0 &&
            baseline_support_warning->unit == "count" &&
            baseline_support_warning->evidence_basis ==
                "forecast_baseline_facts" &&
            baseline_support_warning->use == "evaluation_baseline" &&
            baseline_support_warning->effect == "none" &&
            baseline_support_warning->diagnostic_sample_count == 40 &&
            baseline_kind_warning != baseline_eval.warning_results.end() &&
            baseline_kind_warning->kind == "forecast_baseline" &&
            baseline_kind_warning->status == "warning" &&
            baseline_kind_warning->threshold_triggered &&
            baseline_kind_warning->threshold_relation == "below_threshold" &&
            baseline_kind_warning->measured_value == 1.0 &&
            baseline_kind_warning->threshold == 4.0 &&
            baseline_kind_warning->unit == "count" &&
            baseline_kind_warning->diagnostic_metric_family ==
                "baseline_kind_coverage" &&
            baseline_kind_warning->evidence_basis ==
                "forecast_baseline_facts" &&
            baseline_kind_warning->use == "evaluation_baseline" &&
            baseline_kind_warning->effect == "none" &&
            baseline_kind_warning->diagnostic_sample_count == 40 &&
            baseline_metric_warning != baseline_eval.warning_results.end() &&
            baseline_metric_warning->kind == "forecast_baseline" &&
            baseline_metric_warning->status == "clear" &&
            !baseline_metric_warning->threshold_triggered &&
            baseline_metric_warning->measured_value == 1.0 &&
            baseline_metric_warning->threshold == 1.0 &&
            baseline_metric_warning->unit == "count" &&
            baseline_metric_warning->diagnostic_metric_family ==
                "baseline_metric_coverage" &&
            baseline_metric_warning->evidence_basis ==
                "forecast_baseline_facts" &&
            baseline_metric_warning->diagnostic_sample_count == 40 &&
            baseline_eval.warning_summary.triggered_warning_count == 2,
        "forecast baseline artifact readiness proves baseline evidence "
        "lineage against an in-ledger target transform digest");
  check_artifact_proof_no_decision_authority(
      baseline_eval.proof_certificate.artifacts.front(),
      "forecast_baseline_artifact_ready");
  check_warning_envelope(*baseline_support_warning,
                         "forecast_baseline_artifact_ready", "forecast_support",
                         "forecast_baseline",
                         "forecast baseline support warning");
  check_warning_envelope(*baseline_kind_warning,
                         "forecast_baseline_artifact_ready",
                         "forecast_baseline_comparison", "forecast_baseline",
                         "forecast baseline kind warning");
  check_warning_envelope(*baseline_metric_warning,
                         "forecast_baseline_artifact_ready",
                         "forecast_baseline_comparison", "forecast_baseline",
                         "forecast baseline metric warning");
  const auto forecast_eval =
      artifact_evaluator.evaluate("forecast_eval_artifact_ready");
  const auto forecast_calibration_warning = std::find_if(
      forecast_eval.warning_results.begin(),
      forecast_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_calibration_coverage_low_visibility_only";
      });
  const auto forecast_skill_warning = std::find_if(
      forecast_eval.warning_results.begin(),
      forecast_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_skill_vs_baseline_negative_visibility_only";
      });
  const auto forecast_horizon_support_warning = std::find_if(
      forecast_eval.warning_results.begin(),
      forecast_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_horizon_support_low_visibility_only";
      });
  check(
      forecast_eval.status == target::lattice_target_status_t::satisfied &&
          forecast_eval.proof_kind == "forecast_eval_artifact_bound" &&
          !forecast_eval.target_kind_applicable &&
          forecast_eval.subject_fact_family == "forecast_eval" &&
          !forecast_eval.plan_ready && forecast_eval.suggested_wave.empty() &&
          forecast_eval.fact_integrity_summary_available &&
          forecast_eval.fact_integrity_summary.inspected_family_count == 1 &&
          forecast_eval.fact_integrity_summary.relation_declared_count == 3 &&
          forecast_eval.fact_integrity_summary.relation_bound_count == 3 &&
          forecast_eval.fact_integrity_summary.relation_integrity_clean &&
          forecast_eval.proof_certificate.artifacts.size() == 1 &&
          forecast_eval.proof_certificate.artifacts.front().fact_family ==
              "forecast_eval" &&
          forecast_eval.proof_certificate.artifacts.front().passed &&
          forecast_eval.proof_certificate_check.passed &&
          forecast_eval.warning_results.size() == 3 &&
          forecast_calibration_warning != forecast_eval.warning_results.end() &&
          forecast_calibration_warning->kind == "forecast_eval" &&
          forecast_calibration_warning->status == "warning" &&
          forecast_calibration_warning->threshold_triggered &&
          forecast_calibration_warning->threshold_relation ==
              "below_threshold" &&
          forecast_calibration_warning->measured_value > 0.90 &&
          forecast_calibration_warning->measured_value < 0.92 &&
          forecast_calibration_warning->threshold == 0.95 &&
          forecast_calibration_warning->unit == "fraction" &&
          forecast_calibration_warning->evidence_basis ==
              "forecast_eval_facts" &&
          forecast_calibration_warning->use == "evaluation_metric" &&
          forecast_calibration_warning->effect == "none" &&
          forecast_calibration_warning->diagnostic_sample_count == 40 &&
          forecast_skill_warning != forecast_eval.warning_results.end() &&
          forecast_skill_warning->kind == "forecast_eval" &&
          forecast_skill_warning->status == "clear" &&
          !forecast_skill_warning->threshold_triggered &&
          forecast_skill_warning->threshold_relation == "not_below_threshold" &&
          forecast_skill_warning->measured_value > 0.19 &&
          forecast_skill_warning->measured_value < 0.21 &&
          forecast_skill_warning->threshold == 0.0 &&
          forecast_skill_warning->unit == "relative_skill" &&
          forecast_skill_warning->diagnostic_metric_family ==
              "baseline_comparison" &&
          forecast_skill_warning->diagnostic_sample_count == 40 &&
          forecast_horizon_support_warning !=
              forecast_eval.warning_results.end() &&
          forecast_horizon_support_warning->kind == "forecast_eval" &&
          forecast_horizon_support_warning->status == "clear" &&
          !forecast_horizon_support_warning->threshold_triggered &&
          forecast_horizon_support_warning->threshold_relation ==
              "not_below_threshold" &&
          forecast_horizon_support_warning->measured_value == 40.0 &&
          forecast_horizon_support_warning->threshold == 10.0 &&
          forecast_horizon_support_warning->unit == "count" &&
          forecast_horizon_support_warning->diagnostic_metric_family ==
              "support_surface" &&
          forecast_eval.warning_summary.triggered_warning_count == 1,
      "forecast eval artifact readiness proves checkpoint/transform/baseline "
      "lineage and selection-signal audit without forecast quality gating");
  check_artifact_proof_no_decision_authority(
      forecast_eval.proof_certificate.artifacts.front(),
      "forecast_eval_artifact_ready");
  check_warning_envelope(*forecast_calibration_warning,
                         "forecast_eval_artifact_ready", "forecast_calibration",
                         "forecast_eval", "forecast calibration warning");
  check_warning_envelope(*forecast_skill_warning,
                         "forecast_eval_artifact_ready",
                         "forecast_baseline_comparison", "forecast_eval",
                         "forecast skill warning");
  check_warning_envelope(*forecast_horizon_support_warning,
                         "forecast_eval_artifact_ready", "forecast_support",
                         "forecast_eval", "forecast horizon support warning");
  auto tampered_forecast_certificate = forecast_eval.proof_certificate;
  tampered_forecast_certificate.artifacts.front().proof_kind =
      "forecast_quality_ready";
  tampered_forecast_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          tampered_forecast_certificate);
  const auto tampered_forecast_certificate_check =
      target::verify_lattice_target_proof_certificate(
          tampered_forecast_certificate);
  check(!tampered_forecast_certificate_check.passed &&
            has_issue_containing(tampered_forecast_certificate_check.issues,
                                 "proof_kind does not match artifact proof "
                                 "template") &&
            has_issue_containing(tampered_forecast_certificate_check.issues,
                                 "proof_template_bound mismatch"),
        "artifact proof certificates bind proof_kind to the explicit "
        "fact-family proof template");

  const auto scanned_artifact_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_scanned_artifacts";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      scanned_artifact_root,
      lattice_fixture::anchor_range_forecast_artifact_options(
          /*forecast_baseline_digest_mismatch=*/false));
  target::lattice_target_evaluator_t scanned_artifact_evaluator(
      artifact_specs, artifact_runtime_eval_options(scanned_artifact_root));
  const auto scanned_forecast_eval =
      scanned_artifact_evaluator.evaluate("forecast_eval_artifact_ready");
  const auto scanned_skill_warning = std::find_if(
      scanned_forecast_eval.warning_results.begin(),
      scanned_forecast_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_skill_vs_baseline_negative_visibility_only";
      });
  const auto scanned_horizon_warning = std::find_if(
      scanned_forecast_eval.warning_results.begin(),
      scanned_forecast_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_horizon_support_low_visibility_only";
      });
  check(scanned_forecast_eval.status ==
                target::lattice_target_status_t::satisfied &&
            scanned_forecast_eval.proof_kind ==
                "forecast_eval_artifact_bound" &&
            scanned_forecast_eval.fact_integrity_summary_available &&
            scanned_forecast_eval.fact_integrity_summary
                    .relation_declared_count == 3 &&
            scanned_forecast_eval.fact_integrity_summary.relation_bound_count ==
                3 &&
            scanned_forecast_eval.fact_integrity_summary
                .relation_integrity_clean &&
            scanned_forecast_eval.proof_certificate.artifacts.size() == 1 &&
            scanned_forecast_eval.proof_certificate.artifacts.front().passed &&
            scanned_forecast_eval.proof_certificate_check.passed &&
            scanned_forecast_eval.warning_results.size() == 3 &&
            scanned_skill_warning !=
                scanned_forecast_eval.warning_results.end() &&
            scanned_skill_warning->kind == "forecast_eval" &&
            scanned_skill_warning->status == "clear" &&
            scanned_skill_warning->measured_value > 0.19 &&
            scanned_skill_warning->measured_value < 0.21 &&
            scanned_skill_warning->evidence_basis == "forecast_eval_facts" &&
            scanned_horizon_warning !=
                scanned_forecast_eval.warning_results.end() &&
            scanned_horizon_warning->status == "clear" &&
            scanned_horizon_warning->measured_value == 40.0 &&
            !scanned_forecast_eval.plan_ready &&
            scanned_forecast_eval.suggested_wave.empty(),
        "forecast eval artifact readiness should pass from scanned sidecar "
        "facts with bound transform, baseline, and selection-signal lineage" +
            describe_evaluation(scanned_forecast_eval));
  check_artifact_proof_no_decision_authority(
      scanned_forecast_eval.proof_certificate.artifacts.front(),
      "scanned forecast_eval_artifact_ready");

  const auto negative_skill_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/false,
      /*replay_contract_mismatch=*/false,
      /*replay_incomplete_attempts=*/false,
      /*replay_missing_projection_metrics=*/false,
      /*forecast_missing_horizon_support=*/false,
      /*replay_missing_time_law_steps=*/false,
      /*replay_future_observation_violation=*/false,
      /*replay_mixed_future_keys=*/false,
      /*replay_missing_projection_step_evidence=*/false,
      /*replay_expected_step_mismatch=*/false,
      /*replay_action_time_policy_mismatch=*/false,
      /*replay_source_order_policy_mismatch=*/false,
      /*replay_parallelism_mismatch=*/false,
      /*replay_world_mode_mismatch=*/false,
      /*replay_schema_mismatch=*/false,
      /*replay_runtime_run_id_missing=*/false,
      /*replay_report_digest_mismatch=*/false,
      /*transform_missing_support_surface=*/false,
      /*baseline_missing_support=*/false,
      /*forecast_missing_target_transform_binding=*/false,
      /*forecast_missing_baseline_binding=*/false,
      /*forecast_missing_evaluated_checkpoint_lineage=*/false,
      /*forecast_model_state_mutation=*/false,
      /*forecast_negative_skill=*/true);
  target::lattice_target_evaluator_t negative_skill_evaluator(
      artifact_specs, artifact_eval_options(negative_skill_ledger));
  const auto negative_skill_eval =
      negative_skill_evaluator.evaluate("forecast_eval_artifact_ready");
  const auto negative_skill_warning = std::find_if(
      negative_skill_eval.warning_results.begin(),
      negative_skill_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "forecast_eval_skill_vs_baseline_negative_visibility_only";
      });
  check(negative_skill_eval.status ==
                target::lattice_target_status_t::satisfied &&
            !negative_skill_eval.plan_ready &&
            negative_skill_eval.suggested_wave.empty() &&
            negative_skill_eval.proof_certificate.artifacts.size() == 1 &&
            negative_skill_eval.proof_certificate.artifacts.front().passed &&
            negative_skill_eval.proof_certificate_check.passed &&
            negative_skill_warning !=
                negative_skill_eval.warning_results.end() &&
            negative_skill_warning->status == "warning" &&
            negative_skill_warning->threshold_triggered &&
            negative_skill_warning->threshold_relation == "below_threshold" &&
            negative_skill_warning->measured_value < 0.0 &&
            negative_skill_eval.warning_summary.triggered_warning_count == 2 &&
            negative_skill_eval.warning_summary.blocking_warning_count == 0 &&
            negative_skill_eval.warning_summary.all_warnings_non_blocking,
        "forecast eval artifact readiness must pass with negative "
        "skill-vs-baseline visibility warnings and no dispatch advice");

  auto scanned_identity_drift_options =
      artifact_runtime_eval_options(scanned_artifact_root);
  scanned_identity_drift_options.active_identity.graph_order_fingerprint =
      "graph_other";
  target::lattice_target_evaluator_t scanned_identity_drift_evaluator(
      artifact_specs, scanned_identity_drift_options);
  const auto scanned_identity_drift_eval =
      scanned_identity_drift_evaluator.evaluate("forecast_eval_artifact_ready");
  check(scanned_identity_drift_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(scanned_identity_drift_eval.reasons,
                                  "matched the target identity") &&
            scanned_identity_drift_eval.evidence.matching_job_count == 1 &&
            scanned_identity_drift_eval.evidence.matching_train_attempt_count ==
                0 &&
            has_deficit_key(scanned_identity_drift_eval.deficits,
                            "artifact:forecast_eval_identity") &&
            scanned_identity_drift_eval.proof_certificate.artifacts.empty(),
        "forecast eval artifact readiness should reject scanned facts whose "
        "identity does not match the active graph/cursor identity");

  const auto allocation_eval =
      artifact_evaluator.evaluate("allocation_artifact_ready");
  check(
      allocation_eval.status == target::lattice_target_status_t::satisfied &&
          allocation_eval.target_class == "artifact_readiness" &&
          !allocation_eval.target_kind_applicable &&
          allocation_eval.proof_kind == "allocation_artifact_bound" &&
          allocation_eval.subject_fact_family == "allocation_engine" &&
          !allocation_eval.plan_ready &&
          allocation_eval.suggested_wave.empty() &&
          allocation_eval.proof_certificate.artifacts.size() == 1 &&
          allocation_eval.proof_certificate.artifacts.front().fact_family ==
              "allocation_engine" &&
          allocation_eval.proof_certificate.artifacts.front().passed &&
          allocation_eval.proof_certificate_check.passed &&
          allocation_eval.warning_results.size() == 1 &&
          allocation_eval.warning_results.front().kind == "allocation_engine" &&
          allocation_eval.warning_results.front().status == "warning" &&
          allocation_eval.warning_results.front().threshold_triggered &&
          allocation_eval.warning_results.front().threshold_relation ==
              "above_threshold" &&
          allocation_eval.warning_results.front().measured_value == 0.12 &&
          allocation_eval.warning_results.front().threshold == 0.10 &&
          allocation_eval.warning_results.front().unit == "fraction" &&
          allocation_eval.warning_results.front().evidence_basis ==
              "allocation_engine_facts" &&
          allocation_eval.warning_results.front().use == "allocation_engine" &&
          allocation_eval.warning_results.front().effect == "none" &&
          allocation_eval.warning_results.front().diagnostic_sample_count ==
              1 &&
          allocation_eval.warning_summary.triggered_warning_count == 1,
      "allocation artifact readiness proves deterministic allocation output "
      "lineage without becoming allocation authority");
  check(allocation_eval.warning_summary.blocking_warning_count == 0 &&
            allocation_eval.warning_summary.all_warnings_non_blocking &&
            allocation_eval.deficits.empty(),
        "allocation turnover warnings must not become hidden policy gates or "
        "target deficits");
  check_artifact_proof_no_decision_authority(
      allocation_eval.proof_certificate.artifacts.front(),
      "allocation_artifact_ready");
  check_warning_envelope(allocation_eval.warning_results.front(),
                         "allocation_artifact_ready",
                         "allocation_engine_diagnostics", "allocation_engine",
                         "allocation turnover warning");

  const auto replay_eval =
      artifact_evaluator.evaluate("replay_environment_artifact_ready");
  check(replay_eval.status == target::lattice_target_status_t::satisfied &&
            replay_eval.target_class == "artifact_readiness" &&
            !replay_eval.target_kind_applicable &&
            replay_eval.proof_kind == "replay_environment_artifact_bound" &&
            replay_eval.subject_fact_family == "replay_environment" &&
            !replay_eval.plan_ready && replay_eval.suggested_wave.empty() &&
            replay_eval.proof_certificate.artifacts.size() == 1 &&
            replay_eval.proof_certificate.artifacts.front().fact_family ==
                "replay_environment" &&
            replay_eval.proof_certificate.artifacts.front().passed &&
            replay_eval.proof_certificate.artifacts.front().lineage_bound &&
            replay_eval.proof_certificate.artifacts.front()
                .proof_template_bound &&
            replay_eval.proof_certificate_check.passed &&
            replay_eval.warning_results.empty() && replay_eval.deficits.empty(),
        "replay environment artifact readiness proves cost-aware replay "
        "report lineage, time-law evidence, Cajtucu traces, and profile "
        "identity without dispatch authority");
  check_artifact_proof_no_decision_authority(
      replay_eval.proof_certificate.artifacts.front(),
      "replay_environment_artifact_ready");

  const auto missing_replay_policy_summary_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/false,
      /*replay_contract_mismatch=*/false,
      /*replay_incomplete_attempts=*/false,
      /*replay_missing_projection_metrics=*/false,
      /*forecast_missing_horizon_support=*/false,
      /*replay_missing_time_law_steps=*/false,
      /*replay_future_observation_violation=*/false,
      /*replay_mixed_future_keys=*/false,
      /*replay_missing_projection_step_evidence=*/false,
      /*replay_expected_step_mismatch=*/false,
      /*replay_action_time_policy_mismatch=*/false,
      /*replay_source_order_policy_mismatch=*/false,
      /*replay_parallelism_mismatch=*/false,
      /*replay_world_mode_mismatch=*/false,
      /*replay_schema_mismatch=*/false,
      /*replay_runtime_run_id_missing=*/false,
      /*replay_report_digest_mismatch=*/false,
      /*transform_missing_support_surface=*/false,
      /*baseline_missing_support=*/false,
      /*forecast_missing_target_transform_binding=*/false,
      /*forecast_missing_baseline_binding=*/false,
      /*forecast_missing_evaluated_checkpoint_lineage=*/false,
      /*forecast_model_state_mutation=*/false,
      /*forecast_negative_skill=*/false,
      /*policy_training_future_snapshot_use=*/false,
      /*replay_missing_policy_summary=*/true);
  target::lattice_target_evaluator_t missing_replay_policy_summary_evaluator(
      artifact_specs,
      artifact_eval_options(missing_replay_policy_summary_ledger));
  const auto missing_replay_policy_summary_eval =
      missing_replay_policy_summary_evaluator.evaluate(
          "replay_environment_artifact_ready");
  check(missing_replay_policy_summary_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(missing_replay_policy_summary_eval.reasons,
                                  "missing_replay_policy_summary_evidence") &&
            missing_replay_policy_summary_eval.proof_certificate.artifacts
                    .size() == 1 &&
            !missing_replay_policy_summary_eval.proof_certificate.artifacts
                 .front()
                 .passed &&
            !missing_replay_policy_summary_eval.proof_certificate.artifacts
                 .front()
                 .lineage_bound,
        "replay environment artifact readiness rejects reports without "
        "policy-summary evidence");

  const auto failed_replay_attempt_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/false,
      /*replay_contract_mismatch=*/false,
      /*replay_incomplete_attempts=*/false,
      /*replay_missing_projection_metrics=*/false,
      /*forecast_missing_horizon_support=*/false,
      /*replay_missing_time_law_steps=*/false,
      /*replay_future_observation_violation=*/false,
      /*replay_mixed_future_keys=*/false,
      /*replay_missing_projection_step_evidence=*/false,
      /*replay_expected_step_mismatch=*/false,
      /*replay_action_time_policy_mismatch=*/false,
      /*replay_source_order_policy_mismatch=*/false,
      /*replay_parallelism_mismatch=*/false,
      /*replay_world_mode_mismatch=*/false,
      /*replay_schema_mismatch=*/false,
      /*replay_runtime_run_id_missing=*/false,
      /*replay_report_digest_mismatch=*/false,
      /*transform_missing_support_surface=*/false,
      /*baseline_missing_support=*/false,
      /*forecast_missing_target_transform_binding=*/false,
      /*forecast_missing_baseline_binding=*/false,
      /*forecast_missing_evaluated_checkpoint_lineage=*/false,
      /*forecast_model_state_mutation=*/false,
      /*forecast_negative_skill=*/false,
      /*policy_training_future_snapshot_use=*/false,
      /*replay_missing_policy_summary=*/false,
      /*replay_failed_attempts=*/true);
  target::lattice_target_evaluator_t failed_replay_attempt_evaluator(
      artifact_specs, artifact_eval_options(failed_replay_attempt_ledger));
  const auto failed_replay_attempt_eval =
      failed_replay_attempt_evaluator.evaluate(
          "replay_environment_artifact_ready");
  check(failed_replay_attempt_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(failed_replay_attempt_eval.reasons,
                                  "replay_environment_failed_attempts") &&
            failed_replay_attempt_eval.proof_certificate.artifacts.size() ==
                1 &&
            !failed_replay_attempt_eval.proof_certificate.artifacts.front()
                 .passed &&
            !failed_replay_attempt_eval.proof_certificate.artifacts.front()
                 .lineage_bound,
        "replay environment artifact readiness rejects reports with failed "
        "rollout attempts");

  const auto policy_training_eval =
      artifact_evaluator.evaluate("policy_training_artifact_ready");
  check(
      policy_training_eval.status ==
              target::lattice_target_status_t::satisfied &&
          policy_training_eval.target_class == "artifact_readiness" &&
          !policy_training_eval.target_kind_applicable &&
          policy_training_eval.proof_kind == "policy_training_artifact_bound" &&
          policy_training_eval.subject_fact_family == "policy_training" &&
          !policy_training_eval.plan_ready &&
          policy_training_eval.suggested_wave.empty() &&
          policy_training_eval.fact_integrity_summary_available &&
          policy_training_eval.fact_integrity_summary.inspected_family_count ==
              1 &&
          policy_training_eval.fact_integrity_summary.relation_declared_count ==
              4 &&
          policy_training_eval.fact_integrity_summary.relation_bound_count ==
              4 &&
          policy_training_eval.fact_integrity_summary
              .relation_integrity_clean &&
          policy_training_eval.proof_certificate.artifacts.size() == 1 &&
          policy_training_eval.proof_certificate.artifacts.front()
                  .fact_family == "policy_training" &&
          policy_training_eval.proof_certificate.artifacts.front().passed &&
          policy_training_eval.proof_certificate.artifacts.front()
              .lineage_bound &&
          policy_training_eval.proof_certificate.artifacts.front()
              .proof_template_bound &&
          policy_training_eval.proof_certificate_check.passed &&
          policy_training_eval.warning_results.empty() &&
          policy_training_eval.deficits.empty(),
      "policy training artifact readiness proves checkpoint lineage, "
      "range separation, training-only normalizers/replay buffers, selector "
      "policy, sealed test access, environment/action/reward/execution "
      "profile identity, and parent replay evidence without training or "
      "selection authority");
  check_artifact_proof_no_decision_authority(
      policy_training_eval.proof_certificate.artifacts.front(),
      "policy_training_artifact_ready");

  const auto cross_component_policy_training_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = policy_training_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  PROOF_KIND = policy_training_artifact_bound;
  SUBJECT_FACT_FAMILY = policy_training;
  SUBJECT_COMPONENT = wikimyei.policy.trainable;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};
)DSL");
  const auto cross_component_policy_training_ledger =
      artifact_readiness_ledger(
          /*observer_authority_drift=*/false,
          /*forecast_authority_drift=*/false,
          /*replay_authority_drift=*/false,
          /*baseline_transform_digest_mismatch=*/false,
          /*forecast_baseline_digest_mismatch=*/false,
          /*forecast_selection_signal_digest_mismatch=*/false,
          /*observer_forecast_lineage_mismatch=*/false,
          /*allocation_observer_digest_mismatch=*/false,
          /*allocation_forecast_artifact_mismatch=*/false,
          /*allocation_reserve_source_mismatch=*/false,
          /*allocation_reserve_base_policy_mismatch=*/false,
          /*allocation_reserve_graph_unbound=*/false,
          /*allocation_missing_reason_contract=*/false,
          /*replay_contract_mismatch=*/false,
          /*replay_incomplete_attempts=*/false,
          /*replay_missing_projection_metrics=*/false,
          /*forecast_missing_horizon_support=*/false,
          /*replay_missing_time_law_steps=*/false,
          /*replay_future_observation_violation=*/false,
          /*replay_mixed_future_keys=*/false,
          /*replay_missing_projection_step_evidence=*/false,
          /*replay_expected_step_mismatch=*/false,
          /*replay_action_time_policy_mismatch=*/false,
          /*replay_source_order_policy_mismatch=*/false,
          /*replay_parallelism_mismatch=*/false,
          /*replay_world_mode_mismatch=*/false,
          /*replay_schema_mismatch=*/false,
          /*replay_runtime_run_id_missing=*/false,
          /*replay_report_digest_mismatch=*/false,
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/false,
          /*forecast_missing_target_transform_binding=*/false,
          /*forecast_missing_baseline_binding=*/false,
          /*forecast_missing_evaluated_checkpoint_lineage=*/false,
          /*forecast_model_state_mutation=*/false,
          /*forecast_negative_skill=*/false,
          /*policy_training_future_snapshot_use=*/false,
          /*replay_missing_policy_summary=*/false,
          /*replay_failed_attempts=*/false,
          /*policy_training_component_is_policy=*/true);
  target::lattice_target_evaluator_t cross_component_policy_training_evaluator(
      cross_component_policy_training_specs,
      artifact_eval_options(cross_component_policy_training_ledger));
  const auto cross_component_policy_training_eval =
      cross_component_policy_training_evaluator.evaluate(
          "policy_training_artifact_ready");
  check(cross_component_policy_training_eval.status ==
                target::lattice_target_status_t::satisfied &&
            cross_component_policy_training_eval.proof_certificate.artifacts
                    .size() == 1 &&
            cross_component_policy_training_eval.proof_certificate.artifacts
                .front()
                .lineage_bound,
        "policy-training artifact readiness resolves MDN/replay parent "
        "lineage while the subject policy fact is scoped to "
        "wikimyei.policy.trainable: " +
            describe_evaluation(cross_component_policy_training_eval));

  const auto bad_artifact_ledger =
      artifact_readiness_ledger(/*observer_authority_drift=*/true);
  target::lattice_target_evaluator_t bad_artifact_evaluator(
      artifact_specs, artifact_eval_options(bad_artifact_ledger));
  const auto bad_observer_eval =
      bad_artifact_evaluator.evaluate("observer_belief_artifact_ready");
  check(bad_observer_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(
                bad_observer_eval.reasons,
                "raw_nodelift_potential_cannot_be_tradable_return"),
        "observer artifact readiness rejects raw NodeLift potential when it "
        "claims tradable-return authority");
  check(bad_observer_eval.proof_certificate.artifacts.size() == 1 &&
            !bad_observer_eval.proof_certificate.artifacts.front().passed &&
            !bad_observer_eval.proof_certificate.artifacts.front()
                 .authority_clean &&
            bad_observer_eval.proof_certificate.artifacts.front()
                .raw_potential_tradable_return &&
            has_issue_containing(
                bad_observer_eval.proof_certificate.artifacts.front().issues,
                "raw_nodelift_potential_cannot_be_tradable_return") &&
            !bad_observer_eval.proof_certificate_check.passed,
        "failed observer artifact readiness exposes the failed proof row and "
        "raw-potential authority drift");
  check(bad_observer_eval.plan_basis.primary_deficit_key ==
                "artifact:observer_belief_authority" &&
            has_deficit_key(bad_observer_eval.deficits,
                            "artifact:observer_belief_authority") &&
            has_deficit_key(bad_observer_eval.deficits,
                            "artifact:observer_belief_issue_raw_nodelift_"
                            "potential_cannot_be_tradable_return") &&
            !has_deficit_key(bad_observer_eval.deficits,
                             "certificate:proof_certificate_check"),
        "failed observer artifact readiness should plan from structured "
        "artifact authority deficits, not generic certificate failure");
  const auto bad_forecast_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/true);
  target::lattice_target_evaluator_t bad_forecast_evaluator(
      artifact_specs, artifact_eval_options(bad_forecast_ledger));
  const auto bad_forecast_eval =
      bad_forecast_evaluator.evaluate("forecast_eval_artifact_ready");
  check(bad_forecast_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(
                bad_forecast_eval.reasons,
                "forecast_eval_must_remain_artifact_evidence_only"),
        "forecast eval artifact readiness rejects quality-authority drift");
  check(bad_forecast_eval.proof_certificate.artifacts.size() == 1 &&
            !bad_forecast_eval.proof_certificate.artifacts.front().passed &&
            !bad_forecast_eval.proof_certificate.artifacts.front()
                 .authority_clean &&
            bad_forecast_eval.proof_certificate.artifacts.front()
                .quality_authority &&
            has_issue_containing(
                bad_forecast_eval.proof_certificate.artifacts.front().issues,
                "forecast_eval_must_remain_artifact_evidence_only") &&
            !bad_forecast_eval.proof_certificate_check.passed,
        "failed forecast artifact readiness exposes the failed proof row and "
        "quality-authority drift");
  check(bad_forecast_eval.plan_basis.primary_deficit_key ==
                "artifact:forecast_eval_authority" &&
            has_deficit_key(bad_forecast_eval.deficits,
                            "artifact:forecast_eval_authority") &&
            has_deficit_key(
                bad_forecast_eval.deficits,
                "artifact:forecast_eval_issue_forecast_eval_must_remain_"
                "artifact_evidence_only") &&
            !has_deficit_key(bad_forecast_eval.deficits,
                             "certificate:proof_certificate_check"),
        "failed forecast artifact readiness should plan from structured "
        "artifact authority deficits, not generic certificate failure");
  const auto bad_forecast_horizon_support_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/false,
      /*replay_contract_mismatch=*/false,
      /*replay_incomplete_attempts=*/false,
      /*replay_missing_projection_metrics=*/false,
      /*forecast_missing_horizon_support=*/true);
  target::lattice_target_evaluator_t bad_forecast_horizon_support_evaluator(
      artifact_specs,
      artifact_eval_options(bad_forecast_horizon_support_ledger));
  const auto bad_forecast_horizon_support_eval =
      bad_forecast_horizon_support_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(
      bad_forecast_horizon_support_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(bad_forecast_horizon_support_eval.reasons,
                                "missing_horizon_support_surface") &&
          bad_forecast_horizon_support_eval.proof_certificate.artifacts
                  .size() == 1 &&
          !bad_forecast_horizon_support_eval.proof_certificate.artifacts.front()
               .passed &&
          !bad_forecast_horizon_support_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              bad_forecast_horizon_support_eval.proof_certificate.artifacts
                  .front()
                  .issues,
              "missing_horizon_support_surface") &&
          has_deficit_key(
              bad_forecast_horizon_support_eval.deficits,
              "artifact:forecast_eval_issue_missing_horizon_support_surface") &&
          bad_forecast_horizon_support_eval.plan_basis.primary_deficit_key ==
              "artifact:forecast_eval_lineage" &&
          !bad_forecast_horizon_support_eval.plan_ready &&
          bad_forecast_horizon_support_eval.suggested_wave.empty(),
      "forecast eval artifact readiness rejects missing horizon support "
      "surfaces as lineage evidence, without dispatching a wave");

  const auto missing_transform_support_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/true);
  target::lattice_target_evaluator_t missing_transform_support_evaluator(
      artifact_specs, artifact_eval_options(missing_transform_support_ledger));
  const auto missing_transform_support_eval =
      missing_transform_support_evaluator.evaluate(
          "target_transform_contract_ready");
  check(
      missing_transform_support_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(missing_transform_support_eval.reasons,
                                "missing_support_surface_identity") &&
          has_reason_containing(missing_transform_support_eval.reasons,
                                "missing_support_surface_digest") &&
          missing_transform_support_eval.proof_certificate.artifacts.size() ==
              1 &&
          !missing_transform_support_eval.proof_certificate.artifacts.front()
               .passed &&
          !missing_transform_support_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              missing_transform_support_eval.proof_certificate.artifacts.front()
                  .issues,
              "missing_support_surface_identity") &&
          !missing_transform_support_eval.plan_ready &&
          missing_transform_support_eval.suggested_wave.empty(),
      "target-transform artifact readiness rejects missing support-surface "
      "identity without dispatching a wave");

  const auto missing_baseline_support_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/true);
  target::lattice_target_evaluator_t missing_baseline_support_evaluator(
      artifact_specs, artifact_eval_options(missing_baseline_support_ledger));
  const auto missing_baseline_support_eval =
      missing_baseline_support_evaluator.evaluate(
          "forecast_baseline_artifact_ready");
  check(
      missing_baseline_support_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(missing_baseline_support_eval.reasons,
                                "missing_support_count") &&
          missing_baseline_support_eval.proof_certificate.artifacts.size() ==
              1 &&
          !missing_baseline_support_eval.proof_certificate.artifacts.front()
               .passed &&
          !missing_baseline_support_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              missing_baseline_support_eval.proof_certificate.artifacts.front()
                  .issues,
              "missing_support_count") &&
          !missing_baseline_support_eval.plan_ready &&
          missing_baseline_support_eval.suggested_wave.empty(),
      "forecast-baseline artifact readiness rejects facts without support "
      "counts as incomplete evidence");

  const auto missing_forecast_transform_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/false,
          /*forecast_missing_target_transform_binding=*/true);
  target::lattice_target_evaluator_t missing_forecast_transform_evaluator(
      artifact_specs, artifact_eval_options(missing_forecast_transform_ledger));
  const auto missing_forecast_transform_eval =
      missing_forecast_transform_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(
      missing_forecast_transform_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(missing_forecast_transform_eval.reasons,
                                "missing_target_transform_fact_digest") &&
          missing_forecast_transform_eval.proof_certificate.artifacts.size() ==
              1 &&
          !missing_forecast_transform_eval.proof_certificate.artifacts.front()
               .passed &&
          !missing_forecast_transform_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              missing_forecast_transform_eval.proof_certificate.artifacts
                  .front()
                  .issues,
              "missing_target_transform_fact_digest"),
      "forecast-eval artifact readiness rejects missing target-transform "
      "bindings");

  const auto missing_forecast_baseline_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/false,
          /*forecast_missing_target_transform_binding=*/false,
          /*forecast_missing_baseline_binding=*/true);
  target::lattice_target_evaluator_t missing_forecast_baseline_evaluator(
      artifact_specs, artifact_eval_options(missing_forecast_baseline_ledger));
  const auto missing_forecast_baseline_eval =
      missing_forecast_baseline_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(
      missing_forecast_baseline_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(missing_forecast_baseline_eval.reasons,
                                "missing_baseline_fact_digests") &&
          missing_forecast_baseline_eval.proof_certificate.artifacts.size() ==
              1 &&
          !missing_forecast_baseline_eval.proof_certificate.artifacts.front()
               .passed &&
          !missing_forecast_baseline_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              missing_forecast_baseline_eval.proof_certificate.artifacts.front()
                  .issues,
              "missing_baseline_fact_digests"),
      "forecast-eval artifact readiness rejects missing baseline evidence "
      "bindings");

  const auto missing_forecast_checkpoint_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/false,
          /*forecast_missing_target_transform_binding=*/false,
          /*forecast_missing_baseline_binding=*/false,
          /*forecast_missing_evaluated_checkpoint_lineage=*/true);
  target::lattice_target_evaluator_t missing_forecast_checkpoint_evaluator(
      artifact_specs,
      artifact_eval_options(missing_forecast_checkpoint_ledger));
  const auto missing_forecast_checkpoint_eval =
      missing_forecast_checkpoint_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(
      missing_forecast_checkpoint_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(
              missing_forecast_checkpoint_eval.reasons,
              "missing_evaluated_representation_checkpoint_digest") &&
          has_reason_containing(missing_forecast_checkpoint_eval.reasons,
                                "missing_evaluated_mdn_checkpoint_digest") &&
          missing_forecast_checkpoint_eval.proof_certificate.artifacts.size() ==
              1 &&
          !missing_forecast_checkpoint_eval.proof_certificate.artifacts.front()
               .passed &&
          !missing_forecast_checkpoint_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              missing_forecast_checkpoint_eval.proof_certificate.artifacts
                  .front()
                  .issues,
              "missing_evaluated_representation_checkpoint_digest"),
      "forecast-eval artifact readiness rejects missing evaluated checkpoint "
      "lineage without selecting a checkpoint");

  const auto forecast_mutation_ledger =
      artifact_readiness_ledger_with_proof_fault(
          /*transform_missing_support_surface=*/false,
          /*baseline_missing_support=*/false,
          /*forecast_missing_target_transform_binding=*/false,
          /*forecast_missing_baseline_binding=*/false,
          /*forecast_missing_evaluated_checkpoint_lineage=*/false,
          /*forecast_model_state_mutation=*/true);
  target::lattice_target_evaluator_t forecast_mutation_evaluator(
      artifact_specs, artifact_eval_options(forecast_mutation_ledger));
  const auto forecast_mutation_eval =
      forecast_mutation_evaluator.evaluate("forecast_eval_artifact_ready");
  check(
      forecast_mutation_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(
              forecast_mutation_eval.reasons,
              "forecast_eval_must_remain_artifact_evidence_only") &&
          forecast_mutation_eval.proof_certificate.artifacts.size() == 1 &&
          !forecast_mutation_eval.proof_certificate.artifacts.front().passed &&
          forecast_mutation_eval.proof_certificate.artifacts.front()
              .model_state_mutation &&
          !forecast_mutation_eval.proof_certificate.artifacts.front()
               .deterministic_artifact &&
          !forecast_mutation_eval.proof_certificate.artifacts.front()
               .authority_clean &&
          has_issue_containing(
              forecast_mutation_eval.proof_certificate.artifacts.front().issues,
              "forecast_eval_must_remain_artifact_evidence_only"),
      "forecast-eval artifact readiness rejects model-state mutation as "
      "artifact authority drift");
  const auto bad_baseline_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/true);
  target::lattice_target_evaluator_t bad_baseline_lineage_evaluator(
      artifact_specs, artifact_eval_options(bad_baseline_lineage_ledger));
  const auto bad_baseline_lineage_eval =
      bad_baseline_lineage_evaluator.evaluate(
          "forecast_baseline_artifact_ready");
  check(bad_baseline_lineage_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(bad_baseline_lineage_eval.reasons,
                                  "target_transform_fact_digest_not_found") &&
            bad_baseline_lineage_eval.fact_integrity_summary_available &&
            bad_baseline_lineage_eval.fact_integrity_summary
                    .relation_declared_count == 1 &&
            bad_baseline_lineage_eval.fact_integrity_summary
                    .unresolved_relation_count == 1 &&
            !bad_baseline_lineage_eval.fact_integrity_summary
                 .relation_integrity_clean &&
            has_deficit_key(
                bad_baseline_lineage_eval.deficits,
                "fact_integrity:forecast_baseline_unresolved_relation") &&
            deficit_has_related_fact_integrity_issue_code(
                bad_baseline_lineage_eval.deficits,
                "artifact:forecast_baseline_issue_target_transform_fact_"
                "digest_not_found",
                "artifact_job:target_transform_fact_digest_not_found") &&
            bad_baseline_lineage_eval.proof_certificate.artifacts.size() == 1 &&
            !bad_baseline_lineage_eval.proof_certificate.artifacts.front()
                 .lineage_bound,
        "forecast baseline artifact readiness rejects target-transform digests "
        "that are absent for the target identity");

  const auto bad_forecast_baseline_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/true);
  target::lattice_target_evaluator_t bad_forecast_baseline_lineage_evaluator(
      artifact_specs,
      artifact_eval_options(bad_forecast_baseline_lineage_ledger));
  const auto bad_forecast_baseline_lineage_eval =
      bad_forecast_baseline_lineage_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(
      bad_forecast_baseline_lineage_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(bad_forecast_baseline_lineage_eval.reasons,
                                "baseline_fact_digest_not_found") &&
          bad_forecast_baseline_lineage_eval.fact_integrity_summary_available &&
          bad_forecast_baseline_lineage_eval.fact_integrity_summary
                  .relation_declared_count == 3 &&
          bad_forecast_baseline_lineage_eval.fact_integrity_summary
                  .relation_bound_count == 2 &&
          bad_forecast_baseline_lineage_eval.fact_integrity_summary
                  .unresolved_relation_count == 1 &&
          !bad_forecast_baseline_lineage_eval.fact_integrity_summary
               .relation_integrity_clean &&
          has_deficit_key(bad_forecast_baseline_lineage_eval.deficits,
                          "fact_integrity:forecast_eval_unresolved_relation") &&
          deficit_has_related_fact_integrity_issue_code(
              bad_forecast_baseline_lineage_eval.deficits,
              "artifact:forecast_eval_issue_baseline_fact_digest_not_found",
              "artifact_job:baseline_fact_digest_not_found") &&
          bad_forecast_baseline_lineage_eval.proof_certificate.artifacts
                  .size() == 1 &&
          !bad_forecast_baseline_lineage_eval.proof_certificate.artifacts
               .front()
               .lineage_bound,
      "forecast eval artifact readiness rejects baseline digests that are "
      "absent for the target identity");

  const auto scanned_bad_baseline_root =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_scanned_bad_baseline";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      scanned_bad_baseline_root,
      lattice_fixture::anchor_range_forecast_artifact_options(
          /*forecast_baseline_digest_mismatch=*/true));
  target::lattice_target_evaluator_t scanned_bad_baseline_evaluator(
      artifact_specs, artifact_runtime_eval_options(scanned_bad_baseline_root));
  const auto scanned_bad_baseline_eval =
      scanned_bad_baseline_evaluator.evaluate("forecast_eval_artifact_ready");
  check(
      scanned_bad_baseline_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(scanned_bad_baseline_eval.reasons,
                                "baseline_fact_digest_not_found") &&
          scanned_bad_baseline_eval.fact_integrity_summary_available &&
          scanned_bad_baseline_eval.fact_integrity_summary
                  .relation_declared_count == 3 &&
          scanned_bad_baseline_eval.fact_integrity_summary
                  .relation_bound_count == 2 &&
          scanned_bad_baseline_eval.fact_integrity_summary
                  .unresolved_relation_count == 1 &&
          !scanned_bad_baseline_eval.fact_integrity_summary
               .relation_integrity_clean &&
          has_deficit_key(scanned_bad_baseline_eval.deficits,
                          "fact_integrity:forecast_eval_unresolved_relation") &&
          deficit_has_related_fact_integrity_issue_code(
              scanned_bad_baseline_eval.deficits,
              "artifact:forecast_eval_issue_baseline_fact_digest_not_found",
              "artifact_job:baseline_fact_digest_not_found") &&
          scanned_bad_baseline_eval.proof_certificate.artifacts.size() == 1 &&
          !scanned_bad_baseline_eval.proof_certificate.artifacts.front()
               .lineage_bound,
      "forecast eval artifact readiness should reject scanned sidecar facts "
      "whose declared baseline digest cannot bind under the target identity");

  const auto bad_forecast_selection_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/true);
  target::lattice_target_evaluator_t bad_forecast_selection_lineage_evaluator(
      artifact_specs,
      artifact_eval_options(bad_forecast_selection_lineage_ledger));
  const auto bad_forecast_selection_lineage_eval =
      bad_forecast_selection_lineage_evaluator.evaluate(
          "forecast_eval_artifact_ready");
  check(bad_forecast_selection_lineage_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(bad_forecast_selection_lineage_eval.reasons,
                                  "selection_signal_fact_digest_not_found") &&
            deficit_has_related_fact_integrity_issue_code(
                bad_forecast_selection_lineage_eval.deficits,
                "artifact:forecast_eval_issue_selection_signal_fact_digest_not_"
                "found",
                "artifact_job:selection_signal_fact_digest_not_found") &&
            bad_forecast_selection_lineage_eval.proof_certificate.artifacts
                    .size() == 1 &&
            !bad_forecast_selection_lineage_eval.proof_certificate.artifacts
                 .front()
                 .lineage_bound,
        "forecast eval artifact readiness rejects selection-signal digests "
        "that are absent for the target identity");

  const auto bad_observer_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/true);
  target::lattice_target_evaluator_t bad_observer_lineage_evaluator(
      artifact_specs, artifact_eval_options(bad_observer_lineage_ledger));
  const auto bad_observer_lineage_eval =
      bad_observer_lineage_evaluator.evaluate("observer_belief_artifact_ready");
  check(bad_observer_lineage_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(bad_observer_lineage_eval.reasons,
                                  "forecast_artifact_lineage_not_found") &&
            bad_observer_lineage_eval.fact_integrity_summary_available &&
            bad_observer_lineage_eval.fact_integrity_summary
                    .relation_declared_count == 1 &&
            bad_observer_lineage_eval.fact_integrity_summary
                    .unresolved_relation_count == 1 &&
            !bad_observer_lineage_eval.fact_integrity_summary
                 .relation_integrity_clean &&
            has_deficit_key(
                bad_observer_lineage_eval.deficits,
                "fact_integrity:observer_belief_unresolved_relation") &&
            deficit_has_related_fact_integrity_issue_code(
                bad_observer_lineage_eval.deficits,
                "artifact:observer_belief_issue_forecast_artifact_lineage_not_"
                "found",
                "artifact_job:forecast_artifact_lineage_not_found") &&
            bad_observer_lineage_eval.proof_certificate.artifacts.size() == 1 &&
            !bad_observer_lineage_eval.proof_certificate.artifacts.front()
                 .lineage_bound,
        "observer belief artifact readiness rejects forecast lineage digests "
        "that are absent for the target identity");

  const auto bad_allocation_observer_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/true);
  target::lattice_target_evaluator_t bad_allocation_observer_lineage_evaluator(
      artifact_specs,
      artifact_eval_options(bad_allocation_observer_lineage_ledger));
  const auto bad_allocation_observer_lineage_eval =
      bad_allocation_observer_lineage_evaluator.evaluate(
          "allocation_artifact_ready");
  check(bad_allocation_observer_lineage_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(bad_allocation_observer_lineage_eval.reasons,
                                  "observer_belief_fact_digest_not_found") &&
            bad_allocation_observer_lineage_eval
                .fact_integrity_summary_available &&
            bad_allocation_observer_lineage_eval.fact_integrity_summary
                    .relation_declared_count == 2 &&
            bad_allocation_observer_lineage_eval.fact_integrity_summary
                    .unresolved_relation_count == 1 &&
            !bad_allocation_observer_lineage_eval.fact_integrity_summary
                 .relation_integrity_clean &&
            has_deficit_key(
                bad_allocation_observer_lineage_eval.deficits,
                "fact_integrity:allocation_engine_unresolved_relation") &&
            deficit_has_related_fact_integrity_issue_code(
                bad_allocation_observer_lineage_eval.deficits,
                "artifact:allocation_engine_issue_observer_belief_fact_digest_"
                "not_found",
                "artifact_job:observer_belief_fact_digest_not_found") &&
            bad_allocation_observer_lineage_eval.proof_certificate.artifacts
                    .size() == 1 &&
            !bad_allocation_observer_lineage_eval.proof_certificate.artifacts
                 .front()
                 .lineage_bound,
        "allocation artifact readiness rejects observer-belief digests that "
        "are absent for the target identity");

  const auto bad_allocation_forecast_lineage_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/true);
  target::lattice_target_evaluator_t bad_allocation_forecast_lineage_evaluator(
      artifact_specs,
      artifact_eval_options(bad_allocation_forecast_lineage_ledger));
  const auto bad_allocation_forecast_lineage_eval =
      bad_allocation_forecast_lineage_evaluator.evaluate(
          "allocation_artifact_ready");
  check(
      bad_allocation_forecast_lineage_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(bad_allocation_forecast_lineage_eval.reasons,
                                "observer_forecast_artifact_digest_mismatch") &&
          has_reason_containing(bad_allocation_forecast_lineage_eval.reasons,
                                "forecast_artifact_digest_not_found") &&
          deficit_has_related_fact_integrity_issue_code(
              bad_allocation_forecast_lineage_eval.deficits,
              "artifact:allocation_engine_issue_observer_forecast_artifact_"
              "digest_mismatch",
              "artifact_job:observer_forecast_artifact_digest_mismatch") &&
          deficit_has_related_fact_integrity_issue_code(
              bad_allocation_forecast_lineage_eval.deficits,
              "artifact:allocation_engine_issue_forecast_artifact_digest_not_"
              "found",
              "artifact_job:forecast_artifact_digest_not_found") &&
          bad_allocation_forecast_lineage_eval.proof_certificate.artifacts
                  .size() == 1 &&
          !bad_allocation_forecast_lineage_eval.proof_certificate.artifacts
               .front()
               .lineage_bound,
      "allocation artifact readiness rejects forecast-artifact digests that "
      "do not match the observer lineage and forecast-eval evidence");

  const auto bad_allocation_reserve_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/true,
      /*allocation_reserve_base_policy_mismatch=*/true,
      /*allocation_reserve_graph_unbound=*/true);
  target::lattice_target_evaluator_t bad_allocation_reserve_evaluator(
      artifact_specs, artifact_eval_options(bad_allocation_reserve_ledger));
  const auto bad_allocation_reserve_eval =
      bad_allocation_reserve_evaluator.evaluate("allocation_artifact_ready");
  check(
      bad_allocation_reserve_eval.status !=
              target::lattice_target_status_t::satisfied &&
          has_reason_containing(
              bad_allocation_reserve_eval.reasons,
              "accounting_numeraire_node_source_not_base_policy") &&
          has_reason_containing(
              bad_allocation_reserve_eval.reasons,
              "accounting_numeraire_node_base_policy_mismatch") &&
          has_reason_containing(bad_allocation_reserve_eval.reasons,
                                "accounting_numeraire_node_not_graph_bound") &&
          bad_allocation_reserve_eval.proof_certificate.artifacts.size() == 1 &&
          !bad_allocation_reserve_eval.proof_certificate.artifacts.front()
               .lineage_bound &&
          has_issue_containing(
              bad_allocation_reserve_eval.proof_certificate.artifacts.front()
                  .issues,
              "accounting_numeraire_node_source_not_base_policy") &&
          has_issue_containing(
              bad_allocation_reserve_eval.proof_certificate.artifacts.front()
                  .issues,
              "accounting_numeraire_node_base_policy_mismatch") &&
          has_issue_containing(
              bad_allocation_reserve_eval.proof_certificate.artifacts.front()
                  .issues,
              "accounting_numeraire_node_not_graph_bound"),
      "allocation artifact readiness requires the accounting numeraire node to "
      "be "
      "graph-bound BasePolicy evidence");

  const auto bad_allocation_reason_contract_ledger = artifact_readiness_ledger(
      /*observer_authority_drift=*/false,
      /*forecast_authority_drift=*/false,
      /*replay_authority_drift=*/false,
      /*baseline_transform_digest_mismatch=*/false,
      /*forecast_baseline_digest_mismatch=*/false,
      /*forecast_selection_signal_digest_mismatch=*/false,
      /*observer_forecast_lineage_mismatch=*/false,
      /*allocation_observer_digest_mismatch=*/false,
      /*allocation_forecast_artifact_mismatch=*/false,
      /*allocation_reserve_source_mismatch=*/false,
      /*allocation_reserve_base_policy_mismatch=*/false,
      /*allocation_reserve_graph_unbound=*/false,
      /*allocation_missing_reason_contract=*/true);
  target::lattice_target_evaluator_t bad_allocation_reason_contract_evaluator(
      artifact_specs,
      artifact_eval_options(bad_allocation_reason_contract_ledger));
  const auto bad_allocation_reason_contract_eval =
      bad_allocation_reason_contract_evaluator.evaluate(
          "allocation_artifact_ready");
  check(bad_allocation_reason_contract_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(bad_allocation_reason_contract_eval.reasons,
                                  "missing_fallback_reason_contract") &&
            has_reason_containing(bad_allocation_reason_contract_eval.reasons,
                                  "missing_derisk_reason_contract") &&
            bad_allocation_reason_contract_eval.proof_certificate.artifacts
                    .size() == 1 &&
            !bad_allocation_reason_contract_eval.proof_certificate.artifacts
                 .front()
                 .lineage_bound,
        "allocation artifact readiness requires explicit fallback and de-risk "
        "reason contracts, even when the allocation did not fall back");

  target::lattice_target_spec_t manual_artifact{};
  manual_artifact.target_id = "manual_forecast_eval_artifact_ready";
  manual_artifact.target_class = "artifact_readiness";
  manual_artifact.target_kind_applicable = false;
  manual_artifact.proof_kind = "forecast_eval_artifact_bound";
  manual_artifact.subject_fact_family = "forecast_eval";
  manual_artifact.component = "wikimyei.inference.expected_value.mdn";
  manual_artifact.checkpoint_source = "none";
  manual_artifact.source_range = "anchor_index";
  manual_artifact.anchor_index_begin = 0;
  manual_artifact.anchor_index_end = 10;
  manual_artifact.plan_mode = "none";
  manual_artifact.max_waves = 0;
  manual_artifact.require_component_match = false;
  manual_artifact.require_checkpoint_exists = false;
  manual_artifact.require_finite_loss = false;
  target::lattice_target_spec_t manual_artifact_with_dependency =
      manual_artifact;
  manual_artifact_with_dependency.target_id =
      "manual_forecast_eval_artifact_with_dependency";
  manual_artifact_with_dependency.upstream_target_id =
      "manual_channel_mdn_training_ready";
  target::lattice_target_spec_t manual_upstream{};
  manual_upstream.target_id = "manual_channel_mdn_training_ready";
  manual_upstream.kind = target::lattice_target_kind_t::channel_mdn_ready;
  manual_upstream.component = target::default_component_for_target_kind(
      target::lattice_target_kind_t::channel_mdn_ready);
  manual_upstream.require_contract_match = false;
  manual_upstream.require_component_match = false;
  manual_upstream.require_checkpoint_exists = false;
  manual_upstream.require_finite_loss = false;
  manual_upstream.plan_mode = "train|debug";
  manual_upstream.max_waves = 1;
  target::lattice_target_evaluator_t manual_artifact_dependency_evaluator(
      {manual_artifact_with_dependency, manual_upstream});
  const auto manual_artifact_dependency_eval =
      manual_artifact_dependency_evaluator.evaluate(
          "manual_forecast_eval_artifact_with_dependency");
  check(manual_artifact_dependency_eval.status ==
                target::lattice_target_status_t::blocked &&
            !manual_artifact_dependency_eval.plan_ready &&
            manual_artifact_dependency_eval.suggested_wave.empty() &&
            has_reason_containing(
                manual_artifact_dependency_eval.reasons,
                "artifact_readiness targets cannot declare UPSTREAM_TARGET_ID "
                "or LATTICE_DEPENDS"),
        "manual artifact-readiness specs that bypass DSL still cannot forward "
        "upstream target dependency plans");

  target::lattice_target_spec_t manual_consumer{};
  manual_consumer.target_id = "manual_channel_mdn_validation_eval";
  manual_consumer.target_class = "evaluation_readiness";
  manual_consumer.kind = target::lattice_target_kind_t::channel_mdn_ready;
  manual_consumer.component = target::default_component_for_target_kind(
      target::lattice_target_kind_t::channel_mdn_ready);
  manual_consumer.evaluated_checkpoint_source =
      "latest_satisfying:manual_forecast_eval_artifact_ready";
  manual_consumer.require_contract_match = false;
  manual_consumer.require_component_match = false;
  manual_consumer.require_checkpoint_exists = false;
  target::lattice_target_evaluator_t manual_artifact_reference_evaluator(
      {manual_artifact, manual_consumer});
  const auto manual_artifact_reference_eval =
      manual_artifact_reference_evaluator.evaluate(
          "manual_channel_mdn_validation_eval");
  check(manual_artifact_reference_eval.status ==
                target::lattice_target_status_t::blocked &&
            !manual_artifact_reference_eval.plan_ready &&
            manual_artifact_reference_eval.suggested_wave.empty() &&
            has_reason_containing(
                manual_artifact_reference_eval.reasons,
                "EVALUATED_CHECKPOINT_SOURCE latest_satisfying cannot "
                "reference TARGET_CLASS=artifact_readiness"),
        "evaluator rejects manual specs that bypass DSL and point "
        "latest_satisfying at artifact readiness");
  check(
      !manual_artifact_reference_eval.proof_certificate.dependencies.empty() &&
          manual_artifact_reference_eval.proof_certificate.dependencies.front()
                  .kind == "evaluated_checkpoint_source" &&
          manual_artifact_reference_eval.proof_certificate.dependencies.front()
                  .status == "blocked",
      "manual artifact latest_satisfying rejection is represented as a "
      "blocked dependency proof");

  exposure::lattice_exposure_fact_t channel_representation_health{};
  channel_representation_health.representation_health_available = true;
  channel_representation_health.finite_parameter_check = true;
  channel_representation_health.total_valid_projection_rows = 144000;
  channel_representation_health.mean_adapter_valid_channel_time_fraction = 0.4;
  channel_representation_health.augmented_valid_feature_count = 320000;
  channel_representation_health.mean_augmented_valid_feature_fraction = 0.4;
  channel_representation_health.mean_augmented_feature_retention_fraction = 1.0;
  channel_representation_health.representation_embedding_dim = 32;
  channel_representation_health.representation_effective_rank = 21.0;
  channel_representation_health.representation_effective_rank_fraction = 0.65;
  channel_representation_health.representation_min_dimension_variance = 0.1;
  channel_representation_health.representation_max_dimension_variance = 10.0;
  channel_representation_health.representation_condition_number = 100.0;
  channel_representation_health.representation_isotropy_score = 0.01;
  check(!target::lattice_target_eval_detail::
             strict_channel_representation_health_mismatch(
                 channel_representation_health)
                 .has_value(),
        "channel VICReg health does not require MTF report schema fields");

  exposure::lattice_exposure_fact_t mtf_representation_health{};
  mtf_representation_health.representation_architecture =
      "mtf_jepa_mae_vicreg.v1";
  mtf_representation_health.representation_contract =
      "standalone.mtf_jepa_mae_vicreg.v1";
  mtf_representation_health.representation_value_shape = "[B_flat,De]";
  mtf_representation_health.channel_axis_policy = "optional_channel_output";
  mtf_representation_health.temporal_reduction = "mask_aware_token_pool";
  mtf_representation_health.input_nodelift_shape = "[B,C,Hx,N,F]";
  mtf_representation_health.mtf_training_shape = "[B_flat,C,Hx,F]";
  mtf_representation_health.flattening_contract = "anchor_node_flatten.v1";
  mtf_representation_health.recommended_graph_restore_shape = "[B,N,C,De]";
  mtf_representation_health.reshape_lossless = true;
  mtf_representation_health.representation_health_available = true;
  mtf_representation_health.finite_parameter_check = true;
  mtf_representation_health.gradients_finite = true;
  mtf_representation_health.sample_valid_count = 1200;
  mtf_representation_health.sample_valid_fraction = 0.75;
  mtf_representation_health.channel_valid_count = 3600;
  mtf_representation_health.channel_valid_fraction = 0.75;
  mtf_representation_health.valid_latent_rows = 1200;
  mtf_representation_health.total_valid_projection_rows = 1200;
  mtf_representation_health.representation_embedding_dim = 32;
  mtf_representation_health.latent_std = 0.25;
  mtf_representation_health.latent_norm = 4.0;
  check(!target::lattice_target_eval_detail::
             strict_channel_semantic_contract_mismatch(
                 mtf_representation_health,
                 target::lattice_target_kind_t::mtf_representation_ready)
                 .has_value(),
        "MTF representation semantic contract is accepted for MTF readiness");
  check(
      !target::lattice_target_eval_detail::
           strict_mtf_representation_health_mismatch(mtf_representation_health)
               .has_value(),
      "MTF representation health does not require VICReg augmentation "
      "counters");

  exposure::lattice_exposure_fact_t mtf_backed_mdn{};
  mtf_backed_mdn.input_representation_assembly_id = "mtf_jepa_mae_vicreg_v1";
  mtf_backed_mdn.context_mode = "channel_context_strict";
  mtf_backed_mdn.context_contract =
      "graph_order.channel_node_representation.v1";
  mtf_backed_mdn.context_value_shape = "[B,N,C,De]";
  mtf_backed_mdn.output_contract =
      "graph_order.channel_node_future_distribution.v1";
  check(
      !target::lattice_target_eval_detail::
           strict_channel_semantic_contract_mismatch(
               mtf_backed_mdn, target::lattice_target_kind_t::channel_mdn_ready)
               .has_value(),
      "channel MDN semantic contract accepts MTF representation assembly "
      "when graph-order context contract is preserved");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = accidental_policy_gate;
  TARGET_CLASS = policy_gate;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "reserved for future explicit policy gates",
                      "policy gate target classes stay disabled");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = market_ready;
  TARGET_CLASS = market_readiness;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "market/deployment readiness must remain outside "
                      "Lattice",
                      "market readiness is not a proof-core target class");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = deployment_ready;
  TARGET_CLASS = deployment_readiness;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "market/deployment readiness must remain outside "
                      "Lattice",
                      "deployment readiness is not a proof-core target class");

  const std::string disabled_policy_gate_target_dsl = R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = forecast_quality_acceptance_reserved;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};

LATTICE_POLICY_GATE {
  POLICY_ID = allocation_acceptance_reserved;
  POLICY_KIND = allocation_acceptance;
  TARGET_ID = allocation_artifact_ready;
  METRIC = risk_adjusted_growth_after_cost;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = base_policy;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = allocation_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = allocation_engine;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL";
  const std::string no_policy_gate_target_dsl = R"DSL(
LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};

LATTICE_TARGET {
  TARGET_ID = allocation_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = allocation_engine;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL";
  const auto disabled_policy_gates =
      target::decode_lattice_policy_gates_from_dsl(
          disabled_policy_gate_target_dsl);
  const auto disabled_policy_specs =
      target::decode_lattice_targets_from_dsl(disabled_policy_gate_target_dsl);
  const auto no_policy_specs =
      target::decode_lattice_targets_from_dsl(no_policy_gate_target_dsl);
  const auto disabled_policy_gate_targets =
      target::decode_lattice_compiled_targets_from_dsl(
          disabled_policy_gate_target_dsl);
  const auto same_target_without_policy_gate =
      target::decode_lattice_compiled_targets_from_dsl(
          no_policy_gate_target_dsl);
  const auto target_fingerprint = [](const auto &targets,
                                     const std::string &target_id) {
    const auto it =
        std::find_if(targets.begin(), targets.end(), [&](const auto &target) {
          return target.lowered_v0.target_id == target_id;
        });
    return it == targets.end() ? std::string{} : it->target_spec_fingerprint;
  };
  check(disabled_policy_gates.size() == 2 &&
            std::all_of(disabled_policy_gates.begin(),
                        disabled_policy_gates.end(),
                        [](const auto &gate) { return !gate.enabled; }) &&
            std::any_of(
                disabled_policy_gates.begin(), disabled_policy_gates.end(),
                [](const auto &gate) {
                  return gate.policy_id ==
                             "forecast_quality_acceptance_reserved" &&
                         gate.policy_kind == "forecast_quality_acceptance" &&
                         gate.target_id == "forecast_eval_artifact_ready" &&
                         gate.metric == "skill_vs_naive" &&
                         gate.metric_definition ==
                             "reserved_policy_metric_definition" &&
                         gate.baseline == "naive_previous_value" &&
                         gate.baseline_definition ==
                             "reserved_policy_baseline_definition" &&
                         gate.threshold == 0.0 &&
                         gate.uncertainty_policy == "disabled" &&
                         gate.uncertainty_model == "disabled_policy_model" &&
                         gate.support_minimum == 0.0 &&
                         gate.selector_split == "validation_holdout" &&
                         gate.anti_leakage_policy == "required" &&
                         gate.tie_policy == "reject_ties" &&
                         gate.negative_tests == "reserved_negative_tests" &&
                         gate.calibration_requirements ==
                             "reserved_calibration_requirements" &&
                         gate.holdout_declaration ==
                             "reserved_holdout_declaration" &&
                         gate.threshold_selection_audit ==
                             "reserved_threshold_selection_audit" &&
                         target::lattice_policy_gate_input_contract_complete(
                             gate) &&
                         gate.status == "disabled_reserved" &&
                         !gate.policy_fingerprint.empty();
                }) &&
            std::any_of(
                disabled_policy_gates.begin(), disabled_policy_gates.end(),
                [](const auto &gate) {
                  return gate.policy_id == "allocation_acceptance_reserved" &&
                         gate.policy_kind == "allocation_acceptance" &&
                         gate.target_id == "allocation_artifact_ready" &&
                         gate.metric == "risk_adjusted_growth_after_cost" &&
                         gate.metric_definition ==
                             "reserved_policy_metric_definition" &&
                         gate.baseline == "base_policy" &&
                         gate.baseline_definition ==
                             "reserved_policy_baseline_definition" &&
                         gate.uncertainty_model == "disabled_policy_model" &&
                         target::lattice_policy_gate_input_contract_complete(
                             gate) &&
                         gate.status == "disabled_reserved" &&
                         !gate.policy_fingerprint.empty();
                }) &&
            disabled_policy_gate_targets.size() == 2 &&
            same_target_without_policy_gate.size() == 2 &&
            !target_fingerprint(disabled_policy_gate_targets,
                                "forecast_eval_artifact_ready")
                 .empty() &&
            target_fingerprint(disabled_policy_gate_targets,
                               "forecast_eval_artifact_ready") ==
                target_fingerprint(same_target_without_policy_gate,
                                   "forecast_eval_artifact_ready") &&
            target_fingerprint(disabled_policy_gate_targets,
                               "allocation_artifact_ready") ==
                target_fingerprint(same_target_without_policy_gate,
                                   "allocation_artifact_ready"),
        "disabled policy gates are first-class reserved declarations but do "
        "not change target fingerprints or status");

  target::lattice_target_evaluator_t disabled_policy_evaluator(
      disabled_policy_specs, artifact_eval_options(artifact_ledger));
  target::lattice_target_evaluator_t no_policy_evaluator(
      no_policy_specs, artifact_eval_options(artifact_ledger));
  const auto disabled_policy_forecast_eval =
      disabled_policy_evaluator.evaluate("forecast_eval_artifact_ready");
  const auto no_policy_forecast_eval =
      no_policy_evaluator.evaluate("forecast_eval_artifact_ready");
  const auto disabled_policy_allocation_eval =
      disabled_policy_evaluator.evaluate("allocation_artifact_ready");
  const auto no_policy_allocation_eval =
      no_policy_evaluator.evaluate("allocation_artifact_ready");
  check(
      disabled_policy_forecast_eval.status == no_policy_forecast_eval.status &&
          disabled_policy_forecast_eval.status ==
              target::lattice_target_status_t::satisfied &&
          disabled_policy_forecast_eval.proof_certificate.certificate_digest ==
              no_policy_forecast_eval.proof_certificate.certificate_digest &&
          disabled_policy_forecast_eval.warning_results.empty() &&
          disabled_policy_forecast_eval.deficits.empty() &&
          !disabled_policy_forecast_eval.plan_ready &&
          disabled_policy_forecast_eval.suggested_wave.empty() &&
          disabled_policy_allocation_eval.status ==
              no_policy_allocation_eval.status &&
          disabled_policy_allocation_eval.status ==
              target::lattice_target_status_t::satisfied &&
          disabled_policy_allocation_eval.proof_certificate
                  .certificate_digest ==
              no_policy_allocation_eval.proof_certificate.certificate_digest &&
          disabled_policy_allocation_eval.warning_results.empty() &&
          disabled_policy_allocation_eval.deficits.empty() &&
          !disabled_policy_allocation_eval.plan_ready &&
          disabled_policy_allocation_eval.suggested_wave.empty(),
      "disabled policy reservations must be evaluation-inert: no status, "
      "certificate, warning, deficit, plan, or dispatch effect");

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = forecast_quality_acceptance_reserved;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = true;
};
)DSL",
                      "LATTICE_POLICY_GATE is reserved and disabled",
                      "enabled policy gate syntax is visible but cannot affect "
                      "target status");

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = forecast_quality_acceptance_reserved;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
  POLICY_FINGERPRINT = forged_policy_fingerprint;
};
)DSL",
                      "POLICY_FINGERPRINT mismatch",
                      "reserved policy gates must not accept forged policy "
                      "fingerprints");

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = old_forecast_quality_gate;
  POLICY_KIND = forecast_quality;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "POLICY_KIND is unsupported",
                      "reserved policy gates should reject old generic policy "
                      "kinds");

  for (const auto &future_policy_kind :
       {"performance_acceptance", "market_readiness", "deployment_readiness"}) {
    expect_decode_error(std::string(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = reserved_future_decision_surface;
  POLICY_KIND = )DSL") + future_policy_kind +
                            R"DSL(;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};
)DSL",
                        "fail-closed future decision surfaces",
                        "future policy gate kind " +
                            std::string(future_policy_kind) +
                            " should remain fail-closed");
  }

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = unbound_forecast_quality_gate;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = missing_forecast_eval_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = forecast_eval_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = forecast_eval;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "references unknown TARGET_ID",
                      "reserved policy gates must bind to a declared target");

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = readiness_policy_gate;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = channel_mdn_train_core_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_ready;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "must reference TARGET_CLASS=artifact_readiness",
                      "reserved policy gates must not attach to runtime "
                      "readiness targets");

  expect_decode_error(R"DSL(
LATTICE_POLICY_GATE {
  POLICY_ID = forecast_gate_on_allocation_artifact;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = allocation_artifact_ready;
  METRIC = skill_vs_naive;
  METRIC_DEFINITION = reserved_policy_metric_definition;
  BASELINE = naive_previous_value;
  BASELINE_DEFINITION = reserved_policy_baseline_definition;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_policy_model;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = reserved_negative_tests;
  CALIBRATION_REQUIREMENTS = reserved_calibration_requirements;
  HOLDOUT_DECLARATION = reserved_holdout_declaration;
  THRESHOLD_SELECTION_AUDIT = reserved_threshold_selection_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = allocation_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = allocation_engine;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "must reference SUBJECT_FACT_FAMILY=forecast_eval",
                      "reserved forecast quality gates must bind to forecast "
                      "evaluation artifact proofs");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = removed_node_target;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "was removed from the active vocabulary",
                      "old node MDN target kind points at active replacement");

  const auto warning_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = channel_mdn_runtime_health;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_runtime_health;
  WARNING_ID = high_pre_clip_grad_norm;
  KIND = runtime_health;
  USE = target_supervision;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = grad_norm_max_pre_clip;
  ABOVE = 1000.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_runtime_health;
  WARNING_ID = finite_parameter_check_failed;
  KIND = runtime_health;
  USE = target_supervision;
  SCOPE = target_component_family_id;
  EFFECT = mutated_component;
  METRIC = finite_parameter_check;
  BELOW = 1.0;
};
)DSL");

  check(warning_specs.size() == 1, "runtime_health warning target decodes");
  check(warning_specs.front().warning_specs.size() == 2,
        "runtime_health warning clauses attach to target");
  check(warning_specs.front().warning_specs.front().kind == "runtime_health",
        "runtime_health kind is preserved");
  check(warning_specs.front().warning_specs.front().metric ==
            "grad_norm_max_pre_clip",
        "runtime_health metric is preserved");

  const auto mtf_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_mtf_evidence";
  fs::remove_all(mtf_root);
  write_mtf_job_fixture(mtf_root);
  target::lattice_target_evaluator_t mtf_evaluator(mtf_eval_specs(),
                                                   mtf_eval_options(mtf_root));
  const auto mtf_eval = mtf_evaluator.evaluate("mtf_ready");
  check(mtf_eval.status == target::lattice_target_status_t::satisfied,
        "MTF readiness is satisfied by schema-bound digest-verified facts");
  check(!mtf_eval.warning_results.empty(),
        "MTF geometry warning remains visibility-only");
  check(mtf_eval.evidence.checkpoint_digest_verified,
        "MTF evaluator verifies checkpoint digest against artifact bytes");
  check(mtf_eval.evidence.protocol_id == "cwu_02v",
        "MTF evidence preserves protocol id");

  target::lattice_target_evaluator_t mtf_severe_warning_evaluator(
      mtf_severe_runtime_warn_specs(), mtf_eval_options(mtf_root));
  const auto mtf_severe_warning_eval =
      mtf_severe_warning_evaluator.evaluate("mtf_severe_runtime_warn_ready");
  const auto severe_runtime_warning = std::find_if(
      mtf_severe_warning_eval.warning_results.begin(),
      mtf_severe_warning_eval.warning_results.end(), [](const auto &warning) {
        return warning.warning_id ==
               "checkpoint_written_severe_visibility_only";
      });
  check(mtf_severe_warning_eval.status ==
            target::lattice_target_status_t::satisfied,
        "a severe runtime-health warning must not block an otherwise "
        "satisfied readiness target");
  check(severe_runtime_warning != mtf_severe_warning_eval.warning_results.end(),
        "severe runtime-health warning should be emitted");
  check(severe_runtime_warning->status == "warning" &&
            severe_runtime_warning->severity == "severe" &&
            severe_runtime_warning->threshold_triggered &&
            severe_runtime_warning->threshold_relation == "above_threshold" &&
            severe_runtime_warning->warning_family == "runtime_health" &&
            severe_runtime_warning->source == "lattice" &&
            severe_runtime_warning->component == "lattice_target" &&
            !severe_runtime_warning->blocking &&
            severe_runtime_warning->readiness_effect ==
                "non_blocking_warning_only" &&
            !severe_runtime_warning->evidence_digest.empty() &&
            severe_runtime_warning->suggested_inspection_panel ==
                "hero.runtime.health",
        "severe runtime-health warning should remain a typed, non-blocking "
        "warning envelope");
  check(!mtf_severe_warning_eval.plan_ready &&
            mtf_severe_warning_eval.suggested_wave.empty() &&
            mtf_severe_warning_eval.deficits.empty() &&
            mtf_severe_warning_eval.warning_summary.warning_result_count >= 1 &&
            mtf_severe_warning_eval.warning_summary.blocking_warning_count ==
                0 &&
            mtf_severe_warning_eval.warning_summary.all_warnings_non_blocking,
        "severe warnings should not change plan readiness or suggested wave "
        "dispatch");

  const auto source_analytics_warn_root =
      fs::temp_directory_path() / "cuwacunu_lattice_source_analytics_warn";
  fs::remove_all(source_analytics_warn_root);
  write_mtf_job_fixture(source_analytics_warn_root);
  write_text(source_analytics_warn_root / "mtf_ready_job" /
                 "lattice.source_analytics.fact",
             "schema=kikijyeba.lattice.source_analytics.v1\n"
             "entropy=1.25\n"
             "entropy_rate=0.50\n"
             "information_density=0.75\n"
             "compression_ratio=2.50\n"
             "power_spectrum_entropy=0.33\n"
             "sample_validity_fraction=0.72\n"
             "missingness_fraction=0.28\n"
             "duplicate_sample_count=3\n"
             "source_receipt_fact_count=1\n"
             "source_health_level=warn\n"
             "visibility_only=true\n"
             "readiness_authority=false\n"
             "coverage_authority=false\n"
             "leakage_authority=false\n"
             "contract_identity_authority=false\n");
  target::lattice_target_evaluator_t source_analytics_warn_evaluator(
      source_analytics_warn_specs(),
      mtf_eval_options(source_analytics_warn_root));
  const auto source_analytics_warn_eval =
      source_analytics_warn_evaluator.evaluate(
          "mtf_source_analytics_warn_ready");
  const auto source_analytics_warning = std::find_if(
      source_analytics_warn_eval.warning_results.begin(),
      source_analytics_warn_eval.warning_results.end(), [](const auto &warn) {
        return warn.kind == "source_analytics" &&
               warn.warning_id == "source_missingness_high_visibility_only";
      });
  check(source_analytics_warn_eval.status ==
            target::lattice_target_status_t::satisfied,
        "source analytics warnings do not block an otherwise satisfied "
        "readiness target");
  check(source_analytics_warning !=
            source_analytics_warn_eval.warning_results.end(),
        "source analytics warning result is emitted from catalog facts");
  check(source_analytics_warning->status == "warning" &&
            source_analytics_warning->threshold_triggered &&
            source_analytics_warning->threshold_relation == "above_threshold" &&
            source_analytics_warning->evidence_basis ==
                "source_analytics_facts" &&
            source_analytics_warning->measurement_available &&
            std::abs(source_analytics_warning->measured_value - 0.28) < 1e-12 &&
            source_analytics_warning->exposure_summary.fact_count == 1 &&
            source_analytics_warning->diagnostic_sample_count == 1 &&
            source_analytics_warning->use == "source_observation" &&
            source_analytics_warning->effect == "none",
        "source analytics warning is thresholded as warning-only source "
        "evidence, not as coverage or readiness authority");
  check_warning_envelope(*source_analytics_warning,
                         "mtf_source_analytics_warn_ready", "source_health",
                         "source_analytics", "source analytics warning");
  check(
      source_analytics_warn_eval.warning_summary.blocking_warning_count == 0 &&
          source_analytics_warn_eval.warning_summary
                  .non_blocking_warning_count ==
              source_analytics_warn_eval.warning_summary.warning_result_count &&
          source_analytics_warn_eval.deficits.empty() &&
          source_analytics_warn_eval.warning_summary.all_warnings_non_blocking,
      "source analytics warnings should remain typed non-blocking "
      "visibility");

  const auto latest_root =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_latest_satisfying_metric_blind";
  fs::remove_all(latest_root);
  const auto latest_base_time = fs::file_time_type::clock::now();
  write_mtf_job_fixture_with_loss(latest_root, "older_better_metric_job",
                                  "older better checkpoint",
                                  /*last_loss=*/0.01, latest_base_time);
  write_mtf_job_fixture_with_loss(
      latest_root, "newer_worse_metric_job", "newer worse checkpoint",
      /*last_loss=*/99.0, latest_base_time + std::chrono::seconds(10));
  target::lattice_target_evaluator_t latest_satisfying_evaluator(
      mtf_latest_satisfying_metric_blind_specs(),
      mtf_eval_options(latest_root));
  const auto latest_source_eval =
      latest_satisfying_evaluator.evaluate("mtf_latest_source_ready");
  const auto latest_consumer_eval =
      latest_satisfying_evaluator.evaluate("mtf_latest_consumer_ready");
  check(latest_source_eval.status ==
                target::lattice_target_status_t::satisfied &&
            latest_source_eval.evidence.job_dir.filename() ==
                "newer_worse_metric_job" &&
            latest_source_eval.evidence.loss == 99.0 &&
            fs::exists(latest_root / "older_better_metric_job" / "mtf.pt"),
        "readiness evaluation should select the newest satisfied MTF job even "
        "when an older satisfied job has a better metric");
  check(
      latest_consumer_eval.status ==
              target::lattice_target_status_t::satisfied &&
          latest_consumer_eval.evidence.job_dir.filename() ==
              "newer_worse_metric_job" &&
          latest_consumer_eval.evidence.loss == 99.0 &&
          !latest_consumer_eval.proof_certificate.dependencies.empty() &&
          latest_consumer_eval.proof_certificate.dependencies.front().kind ==
              "checkpoint_source" &&
          latest_consumer_eval.proof_certificate.dependencies.front().status ==
              "satisfied" &&
          latest_consumer_eval.proof_certificate.dependencies.front()
                  .source_target_id == "mtf_latest_source_ready" &&
          latest_consumer_eval.proof_certificate.dependencies.front()
              .mdn_checkpoint_match &&
          latest_consumer_eval.proof_certificate.dependencies.front()
                  .expected_mdn_checkpoint.filename() == "mtf.pt" &&
          latest_consumer_eval.suggested_wave.empty(),
      "latest_satisfying must remain readiness-recency selection, not a "
      "quality or best-metric selector");

  const auto catalog_seed_ledger = artifact_readiness_ledger();
  exposure::lattice_exposure_ledger_t catalog_only_ledger{};
  catalog_only_ledger.add_target_transform(
      catalog_seed_ledger.target_transform_facts().front());
  catalog_only_ledger.add_forecast_baseline(
      catalog_seed_ledger.forecast_baseline_facts().front());
  catalog_only_ledger.add_forecast_eval(
      catalog_seed_ledger.forecast_eval_facts().front());
  catalog_only_ledger.add_observer_belief(
      catalog_seed_ledger.observer_belief_facts().front());
  catalog_only_ledger.add_allocation_engine(
      catalog_seed_ledger.allocation_engine_facts().front());
  const auto catalog_only_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_catalog_only";
  fs::remove_all(catalog_only_root);
  fs::create_directories(catalog_only_root);
  auto catalog_only_options = mtf_eval_options(catalog_only_root);
  catalog_only_options.exposure_ledger = &catalog_only_ledger;
  catalog_only_options.auto_build_exposure_ledger = false;
  target::lattice_target_evaluator_t catalog_only_evaluator(
      mtf_eval_specs(), catalog_only_options);
  const auto catalog_only_eval = catalog_only_evaluator.evaluate("mtf_ready");
  check(catalog_only_eval.status ==
                target::lattice_target_status_t::missing_report &&
            catalog_only_eval.evidence.matching_job_count == 0 &&
            has_reason_containing(catalog_only_eval.reasons,
                                  "no matching job manifest/report found"),
        "catalog artifact facts alone must not satisfy trainable TARGET_KIND "
        "readiness or stand in for runtime job evidence");

  auto wrong_protocol_options = mtf_eval_options(mtf_root);
  wrong_protocol_options.active_identity.protocol_id = "cwu_01v";
  target::lattice_target_evaluator_t wrong_protocol_evaluator(
      mtf_eval_specs(), wrong_protocol_options);
  const auto wrong_protocol_eval =
      wrong_protocol_evaluator.evaluate("mtf_ready");
  check(wrong_protocol_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(wrong_protocol_eval.reasons,
                                  "target protocol id does not match active "
                                  "protocol id"),
        "MTF readiness is protocol-scoped and cannot satisfy cwu_01v");

  const auto missing_protocol_root =
      fs::temp_directory_path() /
      "cuwacunu_lattice_target_mtf_missing_protocol";
  fs::remove_all(missing_protocol_root);
  write_mtf_job_fixture(missing_protocol_root, /*include_schema=*/true, {}, {});
  target::lattice_target_evaluator_t missing_protocol_evaluator(
      mtf_unscoped_eval_specs(), mtf_eval_options(missing_protocol_root));
  const auto missing_protocol_eval =
      missing_protocol_evaluator.evaluate("mtf_unscoped_ready");
  check(missing_protocol_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(missing_protocol_eval.reasons,
                                  "candidate protocol id does not match "
                                  "active protocol id"),
        "readiness evidence without protocol identity fails closed even for "
        "compatibility unscoped targets");

  const auto missing_schema_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_mtf_missing_schema";
  fs::remove_all(missing_schema_root);
  write_mtf_job_fixture(missing_schema_root, /*include_schema=*/false);
  target::lattice_target_evaluator_t missing_schema_evaluator(
      mtf_eval_specs(), mtf_eval_options(missing_schema_root));
  const auto missing_schema_eval =
      missing_schema_evaluator.evaluate("mtf_ready");
  check(missing_schema_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(missing_schema_eval.reasons,
                                  "MTF report schema mismatch"),
        "MTF readiness fails closed when report schema identity is missing");

  const auto wrong_digest_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_mtf_wrong_digest";
  fs::remove_all(wrong_digest_root);
  write_mtf_job_fixture(wrong_digest_root, /*include_schema=*/true,
                        "wrong_digest");
  target::lattice_target_evaluator_t wrong_digest_evaluator(
      mtf_eval_specs(), mtf_eval_options(wrong_digest_root));
  const auto wrong_digest_eval = wrong_digest_evaluator.evaluate("mtf_ready");
  check(wrong_digest_eval.status !=
                target::lattice_target_status_t::satisfied &&
            has_reason_containing(wrong_digest_eval.reasons,
                                  "checkpoint digest verification failed"),
        "MTF readiness fails closed on wrong reported checkpoint digest");
  fs::remove_all(mtf_root);
  fs::remove_all(missing_schema_root);
  fs::remove_all(wrong_digest_root);

  target::lattice_target_proof_certificate_t proof{};
  proof.target_id = "handoff_identity_target";
  proof.target_spec_fingerprint = "target_spec";
  proof.closure.checked = true;
  proof.closure.checkpoint_path = "/tmp/handoff_identity_target.pt";
  proof.closure.fact_count = 1;
  proof.closure.resolution_authority = "unresolved_lineage";
  proof.closure.unresolved_input_checkpoints.push_back("/tmp/missing.pt");
  proof.closure.fact_digests.push_back("fact_digest");
  target::lattice_target_proof_certificate_t::closure_proof_t::causal_exposure_t
      causal{};
  causal.fact_digest = "fact_digest";
  causal.cursor_domain = "ujcamei.graph_anchor";
  causal.job_id = "job";
  causal.wave_id = "wave";
  causal.wave_action = "train";
  causal.job_status = "completed";
  causal.runtime_handoff_id = "runtime_handoff_wrong";
  causal.runtime_handoff_digest = "digest";
  causal.target_component_family_id = "wikimyei.inference.expected_value.mdn";
  causal.component_assembly_fingerprint = "component";
  causal.uses.push_back("target_supervision");
  causal.mutated_component = true;
  causal.anchor_range = {.begin = 0, .end = 1};
  causal.completed_anchor_range = causal.anchor_range;
  causal.target_footprint = causal.anchor_range;
  proof.closure.causal_exposures.push_back(causal);
  const auto bad_handoff_check =
      target::verify_lattice_target_proof_certificate(proof);
  check(has_issue_containing(bad_handoff_check.issues,
                             "runtime handoff id/digest mismatch"),
        "proof certificates reject mismatched Runtime handoff identity");

  proof.closure.causal_exposures.front().runtime_handoff_id =
      "runtime_handoff_digest";
  proof.certificate_digest =
      target::lattice_target_proof_certificate_digest(proof);
  const auto good_handoff_check =
      target::verify_lattice_target_proof_certificate(proof);
  check(!has_issue_containing(good_handoff_check.issues,
                              "runtime handoff id/digest mismatch"),
        "proof certificates accept matching Runtime handoff identity");
  check(!has_issue_containing(good_handoff_check.issues,
                              "incomplete runtime handoff binding"),
        "proof certificates accept complete Runtime handoff identity");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_runtime_health_metric;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};

LATTICE_WARN {
  TARGET_ID = bad_runtime_health_metric;
  WARNING_ID = bad_metric;
  KIND = runtime_health;
  METRIC = not_a_metric;
  ABOVE = 1.0;
};
)DSL",
                      "runtime_health METRIC is unsupported",
                      "unsupported runtime_health metric is rejected");

  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_runtime_health_direction;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
};

LATTICE_WARN {
  TARGET_ID = bad_runtime_health_direction;
  WARNING_ID = bad_direction;
  KIND = runtime_health;
  METRIC = finite_parameter_check;
  ABOVE = 1.0;
  BELOW = 1.0;
};
)DSL",
                      "runtime_health requires exactly one",
                      "runtime_health requires one threshold direction");

  return 0;
}
