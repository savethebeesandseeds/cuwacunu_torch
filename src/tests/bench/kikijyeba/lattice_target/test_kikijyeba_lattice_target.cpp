#include "kikijyeba/lattice/target/lattice_target_evaluator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
namespace target = cuwacunu::kikijyeba::lattice::target;
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

void write_text(const fs::path &path, const std::string &text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write fixture: " + path.string());
  }
  out << text;
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

  auto wrong_protocol_options = mtf_eval_options(mtf_root);
  wrong_protocol_options.active_identity.protocol_id = "cwu_01v";
  target::lattice_target_evaluator_t wrong_protocol_evaluator(
      mtf_eval_specs(), wrong_protocol_options);
  const auto wrong_protocol_eval =
      wrong_protocol_evaluator.evaluate("mtf_ready");
  check(wrong_protocol_eval.status != target::lattice_target_status_t::satisfied &&
            has_reason_containing(wrong_protocol_eval.reasons,
                                  "target protocol id does not match active "
                                  "protocol id"),
        "MTF readiness is protocol-scoped and cannot satisfy cwu_01v");

  const auto missing_protocol_root =
      fs::temp_directory_path() / "cuwacunu_lattice_target_mtf_missing_protocol";
  fs::remove_all(missing_protocol_root);
  write_mtf_job_fixture(missing_protocol_root, /*include_schema=*/true, {},
                        {});
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
        "legacy unscoped targets");

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
