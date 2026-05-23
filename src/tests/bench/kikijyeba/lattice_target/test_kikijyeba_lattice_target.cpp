#include "kikijyeba/lattice/target/lattice_target_evaluator.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace target = cuwacunu::kikijyeba::lattice::target;
namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
namespace split = cuwacunu::kikijyeba::lattice::split;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string join_strings(const std::vector<std::string> &values,
                         const std::string &separator) {
  std::string out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out += separator;
    }
    out += values[i];
  }
  return out;
}

void expect_plan_basis_projects_deficits(
    const target::lattice_target_evaluation_t &evaluation,
    const std::string &message) {
  std::vector<std::string> expected_keys;
  std::vector<std::string> expected_messages;
  std::vector<std::string> expected_priority_classes;
  std::string expected_primary_key;
  std::string expected_primary_message;
  std::int64_t expected_primary_priority = 1000;
  std::string expected_primary_priority_class = "fallback";
  for (const auto &deficit : evaluation.deficits) {
    if (!deficit.plan_relevant) {
      continue;
    }
    check(!deficit.key.empty(), message + " (deficit key is empty)");
    check(!deficit.priority_class.empty(),
          message + " (deficit priority_class is empty)");
    if (std::find(expected_priority_classes.begin(),
                  expected_priority_classes.end(),
                  deficit.priority_class) == expected_priority_classes.end()) {
      expected_priority_classes.push_back(deficit.priority_class);
    }
    if (expected_primary_key.empty() ||
        deficit.priority < expected_primary_priority) {
      expected_primary_key = deficit.key;
      expected_primary_message = deficit.message;
      expected_primary_priority = deficit.priority;
      expected_primary_priority_class = deficit.priority_class;
    }
    if (std::find(expected_keys.begin(), expected_keys.end(), deficit.key) ==
        expected_keys.end()) {
      expected_keys.push_back(deficit.key);
    }
    if (!deficit.message.empty() &&
        std::find(expected_messages.begin(), expected_messages.end(),
                  deficit.message) == expected_messages.end()) {
      expected_messages.push_back(deficit.message);
    }
  }
  check(evaluation.plan_basis.deficit_keys == expected_keys,
        message + " (plan_basis deficit_keys are not a deficit projection)");
  check(evaluation.plan_basis.deficit_messages == expected_messages,
        message +
            " (plan_basis deficit_messages are not a deficit projection)");
  check(evaluation.plan_basis.deficit_priority_classes ==
            expected_priority_classes,
        message + " (plan_basis deficit_priority_classes are not a deficit "
                  "projection)");
  check(evaluation.plan_basis.primary_deficit_key == expected_primary_key,
        message + " (plan_basis primary_deficit_key is not priority-derived)");
  check(evaluation.plan_basis.primary_deficit_message ==
            expected_primary_message,
        message +
            " (plan_basis primary_deficit_message is not priority-derived)");
  check(evaluation.plan_basis.primary_deficit_priority ==
            expected_primary_priority,
        message +
            " (plan_basis primary_deficit_priority is not priority-derived)");
  check(evaluation.plan_basis.primary_deficit_priority_class ==
            expected_primary_priority_class,
        message + " (plan_basis primary_deficit_priority_class is not "
                  "priority-derived)");
}

void expect_validation_model_state_deficit(
    const target::lattice_target_evaluation_t &evaluation,
    const std::string &message) {
  check(evaluation.proof_certificate_check.passed &&
            evaluation.proof_certificate_check.issues.empty(),
        message + " (proof certificate should be self-consistent)");
  check(
      std::any_of(
          evaluation.reasons.begin(), evaluation.reasons.end(),
          [](const auto &reason) {
            return reason ==
                   target::k_lattice_non_mutating_validation_model_state_reason;
          }) &&
          std::any_of(evaluation.deficits.begin(), evaluation.deficits.end(),
                      [](const auto &deficit) {
                        return deficit.key == "model_state:mutated_component" &&
                               deficit.kind == "model_state" &&
                               deficit.status == "forbidden" &&
                               deficit.priority == 25 &&
                               deficit.priority_class == "model_state";
                      }) &&
          evaluation.plan_basis.primary_deficit_key ==
              "model_state:mutated_component",
      message);
  expect_plan_basis_projects_deficits(evaluation,
                                      message + " should project deficits");
}

void expect_validation_output_checkpoint_deficit(
    const target::lattice_target_evaluation_t &evaluation,
    const std::string &message) {
  check(evaluation.proof_certificate_check.passed &&
            evaluation.proof_certificate_check.issues.empty(),
        message + " (proof certificate should be self-consistent)");
  check(std::any_of(
            evaluation.reasons.begin(), evaluation.reasons.end(),
            [](const auto &reason) {
              return reason ==
                     target::k_lattice_read_only_validation_model_state_reason;
            }) &&
            std::any_of(evaluation.deficits.begin(), evaluation.deficits.end(),
                        [](const auto &deficit) {
                          return deficit.key ==
                                     "model_state:output_checkpoint" &&
                                 deficit.kind == "model_state" &&
                                 deficit.dimension == "output_checkpoint" &&
                                 deficit.status == "forbidden" &&
                                 deficit.priority == 25 &&
                                 deficit.priority_class == "model_state";
                        }) &&
            evaluation.plan_basis.primary_deficit_key ==
                "model_state:output_checkpoint",
        message);
  expect_plan_basis_projects_deficits(evaluation,
                                      message + " should project deficits");
}

void expect_validation_input_checkpoint_deficit(
    const target::lattice_target_evaluation_t &evaluation,
    const std::string &message) {
  check(evaluation.proof_certificate_check.passed &&
            evaluation.proof_certificate_check.issues.empty(),
        message + " (proof certificate should be self-consistent)");
  check(
      std::any_of(
          evaluation.reasons.begin(), evaluation.reasons.end(),
          [](const auto &reason) {
            return reason ==
                   target::k_lattice_exact_validation_model_state_input_reason;
          }) &&
          std::any_of(evaluation.deficits.begin(), evaluation.deficits.end(),
                      [](const auto &deficit) {
                        return deficit.key == "model_state:input_checkpoint" &&
                               deficit.kind == "model_state" &&
                               deficit.status == "mismatch" &&
                               deficit.priority == 25 &&
                               deficit.priority_class == "model_state";
                      }) &&
          evaluation.plan_basis.primary_deficit_key ==
              "model_state:input_checkpoint",
      message);
  expect_plan_basis_projects_deficits(evaluation,
                                      message + " should project deficits");
}

void expect_warnings_project_warning_results(
    const target::lattice_target_evaluation_t &evaluation,
    const std::string &message) {
  std::vector<std::string> expected;
  std::int64_t triggered_count = 0;
  std::int64_t unavailable_count = 0;
  std::int64_t clear_measured_count = 0;
  const auto count_relation = [&](const char *relation) {
    return static_cast<std::int64_t>(std::count_if(
        evaluation.warning_results.begin(), evaluation.warning_results.end(),
        [&](const auto &warning) {
          return warning.threshold_relation == relation;
        }));
  };
  for (const auto &warning : evaluation.warning_results) {
    check((warning.status == "warning") == warning.threshold_triggered,
          message + " (warning status and threshold_triggered disagree for " +
              warning.warning_id + ")");
    if (warning.threshold_triggered) {
      expected.push_back(warning.message);
      ++triggered_count;
    }
    if (!warning.measurement_available) {
      ++unavailable_count;
    }
    if (!warning.threshold_triggered && warning.measurement_available) {
      ++clear_measured_count;
    }
  }
  check(evaluation.warnings == expected,
        message + " (warnings are not the warning_results projection)");
  const auto relation_count =
      evaluation.warning_summary.above_threshold_count +
      evaluation.warning_summary.not_above_threshold_count +
      evaluation.warning_summary.below_threshold_count +
      evaluation.warning_summary.not_below_threshold_count +
      evaluation.warning_summary.unavailable_relation_count +
      evaluation.warning_summary.no_threshold_relation_count +
      evaluation.warning_summary.unknown_direction_relation_count;
  check(evaluation.warning_summary.warning_result_count ==
                static_cast<std::int64_t>(evaluation.warning_results.size()) &&
            evaluation.warning_summary.triggered_warning_count ==
                triggered_count &&
            evaluation.warning_summary.unavailable_warning_count ==
                unavailable_count &&
            evaluation.warning_summary.clear_measured_warning_count ==
                clear_measured_count &&
            evaluation.warning_summary.warning_count == triggered_count,
        message + " (warning_summary does not project warning_results)");
  check(
      evaluation.warning_summary.above_threshold_count ==
              count_relation("above_threshold") &&
          evaluation.warning_summary.not_above_threshold_count ==
              count_relation("not_above_threshold") &&
          evaluation.warning_summary.below_threshold_count ==
              count_relation("below_threshold") &&
          evaluation.warning_summary.not_below_threshold_count ==
              count_relation("not_below_threshold") &&
          evaluation.warning_summary.unavailable_relation_count ==
              count_relation("unavailable") &&
          evaluation.warning_summary.no_threshold_relation_count ==
              count_relation("no_threshold") &&
          evaluation.warning_summary.unknown_direction_relation_count ==
              count_relation("unknown_direction") &&
          relation_count == evaluation.warning_summary.warning_result_count,
      message +
          " (warning_summary relation counts do not project warning_results)");
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

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_lattice_target_" + label + "_" +
                    std::to_string(static_cast<long long>(::getpid())));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

std::filesystem::path lattice_acceptance_fixture_root() {
  const std::vector<std::filesystem::path> candidates = {
      std::filesystem::current_path() /
          "../../../fixtures/kikijyeba/lattice_acceptance_v0",
      std::filesystem::current_path() /
          "src/tests/fixtures/kikijyeba/lattice_acceptance_v0",
      "/cuwacunu/src/tests/fixtures/kikijyeba/lattice_acceptance_v0",
  };
  for (const auto &candidate : candidates) {
    std::error_code ec;
    const auto normalized = std::filesystem::weakly_canonical(candidate, ec);
    const auto probe = ec ? candidate : normalized;
    if (std::filesystem::is_directory(probe / "good") &&
        std::filesystem::is_directory(probe / "broken_lineage")) {
      return probe;
    }
  }
  throw std::runtime_error("lattice acceptance fixture root not found");
}

std::filesystem::path lattice_wrong_mdn_validation_fixture_root() {
  const std::vector<std::filesystem::path> candidates = {
      std::filesystem::current_path() /
          "../../../fixtures/kikijyeba/lattice_validation_eval_wrong_mdn",
      std::filesystem::current_path() /
          "src/tests/fixtures/kikijyeba/lattice_validation_eval_wrong_mdn",
      "/cuwacunu/src/tests/fixtures/kikijyeba/"
      "lattice_validation_eval_wrong_mdn",
  };
  for (const auto &candidate : candidates) {
    std::error_code ec;
    const auto normalized = std::filesystem::weakly_canonical(candidate, ec);
    const auto probe = ec ? candidate : normalized;
    if (std::filesystem::is_directory(probe / "runtime_root") &&
        std::filesystem::is_regular_file(probe / "targets.dsl") &&
        std::filesystem::is_regular_file(probe / "splits.dsl")) {
      return probe;
    }
  }
  throw std::runtime_error("lattice wrong-MDN validation fixture root not "
                           "found");
}

std::filesystem::path production_config_path() {
  const std::vector<std::filesystem::path> candidates = {
      std::filesystem::current_path() / "../../../../config/.config",
      std::filesystem::current_path() / "src/config/.config",
      "/cuwacunu/src/config/.config",
  };
  for (const auto &candidate : candidates) {
    std::error_code ec;
    const auto normalized = std::filesystem::weakly_canonical(candidate, ec);
    const auto probe = ec ? candidate : normalized;
    if (std::filesystem::is_regular_file(probe)) {
      return probe;
    }
  }
  throw std::runtime_error("production src/config/.config not found");
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open output file: " + path.string());
  out << text;
}

void attach_source_key_window(exposure::lattice_exposure_fact_t &fact) {
  const auto key_for_row = [](std::int64_t row) {
    return std::to_string(row * 10);
  };
  fact.source_key_footprint_precision = "graph_anchor_key_window_v1";
  if (!fact.anchor_range.empty()) {
    fact.first_anchor_key = key_for_row(fact.anchor_range.begin);
    fact.last_anchor_key = key_for_row(fact.anchor_range.end - 1);
  }
  if (!fact.observed_footprint.empty()) {
    fact.observed_source_key_begin = key_for_row(fact.observed_footprint.begin);
    fact.observed_source_key_end = key_for_row(fact.observed_footprint.end);
  }
  if (!fact.target_footprint.empty()) {
    fact.target_source_key_begin = key_for_row(fact.target_footprint.begin);
    fact.target_source_key_end = key_for_row(fact.target_footprint.end);
  }
}

std::string target_dsl() {
  return R"DSL(
LATTICE_TARGET {
  TARGET_ID = vicreg_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 2;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = vicreg_representation_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL";
}

void write_representation_job(const std::filesystem::path &root,
                              const std::filesystem::path &checkpoint) {
  const auto dir = root / "rep_ready";
  write_text(dir / "job.manifest",
             "job_id=rep_ready\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "graph_order_fingerprint=graph_1\n"
             "source_cursor_token=cursor_ranged\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=2\n"
                                            "last_loss=0.25\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=") +
                                    checkpoint.string() + "\n");
  write_text(dir / "representation.report",
             std::string("optimizer_steps=2\n"
                         "mean_loss=0.20\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "checkpoint");
}

void write_ranged_representation_job(const std::filesystem::path &root,
                                     const std::filesystem::path &checkpoint,
                                     std::int64_t begin, std::int64_t end) {
  const auto dir = root / "rep_ranged";
  write_text(dir / "job.manifest",
             "job_id=rep_ranged\n"
             "wave_id=wave_ranged\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=2\n"
                                            "last_loss=0.25\n"
                                            "wave_streamed_anchor_count=") +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint.string() + "\n");
  write_text(dir / "representation.report",
             std::string("optimizer_steps=2\n"
                         "mean_loss=0.20\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "checkpoint");
}

exposure::lattice_exposure_fact_t
make_rep_exposure_fact(const std::filesystem::path &checkpoint,
                       std::int64_t begin, std::int64_t end,
                       std::string graph_order_fingerprint = "graph_1",
                       std::string source_cursor_token = "cursor_ranged") {
  exposure::lattice_exposure_fact_t fact{};
  fact.contract_fingerprint = "contract_1";
  fact.graph_order_fingerprint = std::move(graph_order_fingerprint);
  fact.component_assembly_fingerprint = "vicreg_1";
  fact.target_component = "wikimyei.representation.encoding.vicreg";
  fact.representation_architecture = "global_fused_temporal_node_encoder.v1";
  fact.representation_contract = "graph_order.node_representation.v1";
  fact.representation_value_shape = "[B,N,D_e] or [B,N,Hx,D_e]";
  fact.channel_axis_policy = "fused_before_primary_output";
  fact.source_cursor_token = std::move(source_cursor_token);
  fact.job_id = "rep_ranged";
  fact.wave_id = "wave_ranged";
  fact.job_status = "completed";
  fact.wave_action = "train";
  fact.anchor_range = exposure::anchor_interval_t{.begin = begin, .end = end};
  fact.coverage_precision = "contiguous_completed_range_v1";
  fact.completed_anchor_range = fact.anchor_range;
  fact.observed_footprint = fact.anchor_range;
  fact.target_footprint = fact.anchor_range;
  fact.source_input_length = 1;
  fact.source_future_length = 0;
  fact.use.observed_input = true;
  fact.use.mutated_component = true;
  fact.optimizer_steps = 2;
  fact.output_checkpoint = checkpoint;
  fact.finite_loss = true;
  fact.mean_loss = 0.20;
  fact.representation_health_available = true;
  fact.mean_invariance_loss = 0.10;
  fact.mean_variance_loss = 0.30;
  fact.mean_covariance_loss = 0.05;
  fact.last_grad_norm = 1.50;
  fact.max_grad_norm = 2.50;
  fact.total_valid_projection_rows = 120;
  fact.mean_adapter_valid_channel_time_fraction = 0.75;
  fact.augmented_valid_feature_count = 240;
  fact.mean_augmented_valid_feature_fraction = 0.70;
  fact.mean_augmented_feature_retention_fraction = 0.93;
  fact.finite_parameter_check = true;
  fact.representation_embedding_dim = 16;
  fact.representation_effective_rank = 8.0;
  fact.representation_effective_rank_fraction = 0.50;
  fact.representation_min_dimension_variance = 0.002;
  fact.representation_max_dimension_variance = 1.25;
  fact.representation_condition_number = 625.0;
  fact.representation_isotropy_score = 0.42;
  attach_source_key_window(fact);
  return fact;
}

exposure::lattice_exposure_fact_t
make_mdn_exposure_fact(const std::filesystem::path &checkpoint,
                       const std::filesystem::path &representation_checkpoint,
                       std::int64_t begin, std::int64_t end,
                       std::string graph_order_fingerprint = "graph_1",
                       std::string source_cursor_token = "cursor_ranged") {
  exposure::lattice_exposure_fact_t fact{};
  fact.contract_fingerprint = "contract_1";
  fact.graph_order_fingerprint = std::move(graph_order_fingerprint);
  fact.component_assembly_fingerprint = "mdn_1";
  fact.target_component = "wikimyei.inference.expected_value.mdn";
  fact.input_representation_id = "node_vicreg_v1";
  fact.context_mode = "global_node_context";
  fact.context_contract = "graph_order.node_representation.v1";
  fact.context_value_shape = "[B_node,D_e]";
  fact.output_contract = "graph_order.node_expected_value.v1";
  fact.source_cursor_token = std::move(source_cursor_token);
  fact.job_id = "mdn_ranged";
  fact.wave_id = "wave_mdn_ranged";
  fact.job_status = "completed";
  fact.wave_action = "train";
  fact.anchor_range = exposure::anchor_interval_t{.begin = begin, .end = end};
  fact.coverage_precision = "contiguous_completed_range_v1";
  fact.completed_anchor_range = fact.anchor_range;
  fact.observed_footprint =
      exposure::anchor_interval_t{.begin = begin - 1, .end = end};
  fact.target_footprint =
      exposure::anchor_interval_t{.begin = begin + 1, .end = end + 1};
  fact.source_input_length = 2;
  fact.source_future_length = 1;
  fact.use.observed_input = true;
  fact.use.target_supervision = true;
  fact.use.mutated_component = true;
  fact.optimizer_steps = 2;
  fact.valid_target_count = 25;
  fact.valid_target_fraction = 0.25;
  fact.output_checkpoint = checkpoint;
  fact.input_checkpoints.push_back(representation_checkpoint);
  fact.finite_loss = true;
  fact.mean_loss = 0.40;
  fact.mean_nll = 0.40;
  attach_source_key_window(fact);
  return fact;
}

void write_ranged_channel_representation_job(
    const std::filesystem::path &root, const std::filesystem::path &checkpoint,
    std::int64_t begin, std::int64_t end) {
  const auto dir = root / "channel_rep_ranged";
  write_text(dir / "job.manifest",
             "job_id=channel_rep_ranged\n"
             "wave_id=wave_channel_rep_ranged\n"
             "job_kind=channel_representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=2\n"
                                            "last_loss=0.18\n"
                                            "wave_streamed_anchor_count=") +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint.string() + "\n");
  write_text(dir / "channel_representation.report",
             std::string("optimizer_steps=2\n"
                         "mean_loss=0.18\n"
                         "mean_invariance_loss=0.08\n"
                         "mean_variance_loss=0.11\n"
                         "mean_covariance_loss=0.04\n"
                         "total_valid_projection_rows=120\n"
                         "mean_adapter_valid_channel_time_fraction=0.80\n"
                         "augmented_valid_feature_count=240\n"
                         "mean_augmented_valid_feature_fraction=0.72\n"
                         "mean_augmented_feature_retention_fraction=0.90\n"
                         "finite_parameter_check=true\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "channel representation checkpoint");
}

void write_ranged_channel_mdn_job(
    const std::filesystem::path &root, const std::filesystem::path &checkpoint,
    const std::filesystem::path &representation_checkpoint, std::int64_t begin,
    std::int64_t end) {
  const auto dir = root / "channel_mdn_ranged";
  write_text(dir / "job.manifest",
             "job_id=channel_mdn_ranged\n"
             "wave_id=wave_channel_mdn_ranged\n"
             "job_kind=channel_inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.inference.expected_value.mdn\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=2\n"
                                            "last_loss=0.32\n"
                                            "wave_streamed_anchor_count=") +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint.string() + "\n");
  write_text(dir / "channel_inference.report",
             std::string("optimizer_steps=2\n"
                         "mean_loss=0.32\n"
                         "input_representation_id=vicreg_v1\n"
                         "context_mode=channel_context_strict\n"
                         "context_contract="
                         "graph_order.channel_node_representation.v1\n"
                         "context_value_shape=[B_node,C,De]\n"
                         "output_contract="
                         "graph_order.channel_node_future_distribution.v1\n"
                         "output_value_shape=log_pi:[B_node,C,Hf,K];mu_sigma:"
                         "[B_node,C,Hf,K,Df]\n"
                         "representation_checkpoint_loaded=true\n"
                         "allow_untrained_representation=false\n"
                         "representation_checkpoint_path=") +
                 representation_checkpoint.string() +
                 "\n"
                 "total_valid_target_count=25\n"
                 "mean_valid_target_fraction=0.25\n"
                 "mean_sigma_mean=0.50\n"
                 "min_sigma_min=0.10\n"
                 "max_sigma_max=1.20\n"
                 "mean_mixture_entropy=0.60\n"
                 "mean_nll_per_channel=0.31,0.33\n"
                 "mean_nll_per_horizon=0.32\n"
                 "mean_mixture_usage=0.55,0.45\n"
                 "nonfinite_output_count=0\n"
                 "finite_parameter_check=true\n"
                 "checkpoint_written=true\n"
                 "checkpoint_path=" +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "channel mdn checkpoint");
  write_text(representation_checkpoint, "channel representation checkpoint");
}

void write_ranged_channel_mdn_eval_job(
    const std::filesystem::path &root,
    const std::filesystem::path &mdn_checkpoint,
    const std::filesystem::path &representation_checkpoint, std::int64_t begin,
    std::int64_t end) {
  const auto dir = root / "channel_mdn_validation_eval";
  write_text(dir / "job.manifest",
             "job_id=channel_mdn_validation_eval\n"
             "wave_id=wave_channel_mdn_validation_eval\n"
             "job_kind=channel_inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_action=run\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=0\n"
                                            "last_loss=0.35\n"
                                            "wave_streamed_anchor_count=") +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=false\n"
                                    "checkpoint_path=\n");
  write_text(dir / "channel_inference.report",
             std::string("optimizer_steps=0\n"
                         "mean_loss=0.35\n"
                         "input_representation_id=vicreg_v1\n"
                         "representation_checkpoint_loaded=true\n"
                         "mdn_checkpoint_loaded=true\n"
                         "allow_untrained_representation=false\n"
                         "representation_checkpoint_path=") +
                 representation_checkpoint.string() +
                 "\n"
                 "mdn_checkpoint_path=" +
                 mdn_checkpoint.string() +
                 "\n"
                 "context_mode=channel_context_strict\n"
                 "context_contract=graph_order.channel_node_representation.v1\n"
                 "context_value_shape=[B_node,C,De]\n"
                 "output_contract="
                 "graph_order.channel_node_future_distribution.v1\n"
                 "output_value_shape=log_pi:[B_node,C,Hf,K];mu_sigma:[B_node,C,"
                 "Hf,K,Df]\n"
                 "total_valid_target_count=8\n"
                 "mean_valid_target_fraction=0.25\n"
                 "mean_sigma_mean=0.50\n"
                 "min_sigma_min=0.10\n"
                 "max_sigma_max=1.20\n"
                 "mean_mixture_entropy=0.60\n"
                 "mean_nll_per_channel=0.34,0.36\n"
                 "mean_nll_per_horizon=0.35\n"
                 "mean_mixture_usage=0.50,0.50\n"
                 "nonfinite_output_count=0\n"
                 "finite_parameter_check=true\n"
                 "checkpoint_written=false\n"
                 "checkpoint_path=\n");
}

exposure::lattice_exposure_fact_t make_channel_rep_exposure_fact(
    const std::filesystem::path &checkpoint, std::int64_t begin,
    std::int64_t end, std::string graph_order_fingerprint = "graph_1",
    std::string source_cursor_token = "cursor_ranged") {
  auto fact = make_rep_exposure_fact(checkpoint, begin, end,
                                     std::move(graph_order_fingerprint),
                                     std::move(source_cursor_token));
  fact.target_component = "wikimyei.representation.encoding.vicreg";
  fact.representation_architecture = "channel_preserving_local_node_encoder.v1";
  fact.representation_contract = "graph_order.channel_node_representation.v1";
  fact.representation_value_shape = "[B,N,C,De]";
  fact.representation_sequence_value_shape = "[M,C,Hx,De]";
  fact.channel_axis_policy = "preserved_primary_output";
  fact.temporal_reduction = "mask_aware_anchor_weighted";
  fact.job_id = "channel_rep_ranged";
  fact.wave_id = "wave_channel_rep_ranged";
  return fact;
}

exposure::lattice_exposure_fact_t make_channel_mdn_exposure_fact(
    const std::filesystem::path &checkpoint,
    const std::filesystem::path &representation_checkpoint, std::int64_t begin,
    std::int64_t end, std::string graph_order_fingerprint = "graph_1",
    std::string source_cursor_token = "cursor_ranged") {
  auto fact = make_mdn_exposure_fact(
      checkpoint, representation_checkpoint, begin, end,
      std::move(graph_order_fingerprint), std::move(source_cursor_token));
  fact.target_component = "wikimyei.inference.expected_value.mdn";
  fact.input_representation_id = "vicreg_v1";
  fact.context_mode = "channel_context_strict";
  fact.context_contract = "graph_order.channel_node_representation.v1";
  fact.context_value_shape = "[B_node,C,De]";
  fact.output_contract = "graph_order.channel_node_future_distribution.v1";
  fact.output_value_shape =
      "log_pi:[B_node,C,Hf,K];mu_sigma:[B_node,C,Hf,K,Df]";
  fact.job_id = "channel_mdn_ranged";
  fact.wave_id = "wave_channel_mdn_ranged";
  fact.mean_loss = 0.32;
  fact.mean_nll = 0.32;
  fact.inference_health_available = true;
  fact.mean_sigma_mean = 0.50;
  fact.min_sigma_min = 0.10;
  fact.max_sigma_max = 1.20;
  fact.mean_mixture_entropy = 0.60;
  fact.mean_nll_per_channel = {0.31, 0.33};
  fact.mean_nll_per_horizon = {0.32};
  fact.mean_mixture_usage = {0.55, 0.45};
  fact.nonfinite_output_count = 0;
  fact.finite_parameter_check = true;
  return fact;
}

void write_ranged_mdn_job(
    const std::filesystem::path &root, const std::filesystem::path &checkpoint,
    const std::filesystem::path &representation_checkpoint, std::int64_t begin,
    std::int64_t end) {
  const auto dir = root / "mdn_ranged";
  write_text(dir / "job.manifest",
             "job_id=mdn_ranged\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.inference.expected_value.mdn\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=2\n"
                                            "last_loss=0.40\n"
                                            "wave_streamed_anchor_count=") +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint.string() + "\n");
  write_text(dir / "inference.report",
             std::string("optimizer_steps=2\n"
                         "mean_loss=0.40\n"
                         "representation_checkpoint_loaded=true\n"
                         "allow_untrained_representation=false\n"
                         "representation_checkpoint_path=") +
                 representation_checkpoint.string() +
                 "\n"
                 "mean_valid_node_target_fraction=0.25\n"
                 "last_active_node_head_count=3\n"
                 "last_trained_node_head_count=3\n"
                 "last_evaluated_node_head_count=3\n"
                 "checkpoint_written=true\n"
                 "checkpoint_path=" +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "checkpoint");
  write_text(representation_checkpoint, "representation checkpoint");
}

void write_ranged_mdn_eval_job(
    const std::filesystem::path &root,
    const std::filesystem::path &mdn_checkpoint,
    const std::filesystem::path &representation_checkpoint, std::int64_t begin,
    std::int64_t end, bool load_mdn_checkpoint = true,
    bool load_representation_checkpoint = true,
    bool write_output_checkpoint = false) {
  const auto dir = root / "mdn_validation_eval";
  const auto output_checkpoint = root / "mdn_validation_eval_output.pt";
  write_text(dir / "job.manifest",
             "job_id=mdn_validation_eval\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_action=run\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "graph_order_fingerprint=graph_1\n"
                 "source_cursor_token=cursor_ranged\n"
                 "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state",
             std::string("status=completed\n"
                         "optimizer_steps=0\n"
                         "last_loss=0.45\n"
                         "wave_streamed_anchor_count=") +
                 std::to_string(end - begin) +
                 "\n"
                 "wave_pulses_completed=1\n"
                 "wave_pulses_skipped=0\n"
                 "skipped_batches=0\n"
                 "checkpoint_written=" +
                 (write_output_checkpoint ? "true" : "false") +
                 "\n"
                 "checkpoint_path=" +
                 (write_output_checkpoint ? output_checkpoint.string() : "") +
                 "\n");
  if (write_output_checkpoint) {
    write_text(output_checkpoint,
               "unexpected validation evaluation checkpoint");
  }
  const std::string report_text =
      std::string("optimizer_steps=0\n"
                  "mean_loss=0.45\n"
                  "representation_checkpoint_loaded=") +
      (load_representation_checkpoint ? "true" : "false") +
      "\n"
      "mdn_checkpoint_loaded=" +
      (load_mdn_checkpoint ? "true" : "false") +
      "\n"
      "mdn_checkpoint_path=" +
      (load_mdn_checkpoint ? mdn_checkpoint.string() : "") +
      "\n"
      "allow_untrained_representation=false\n"
      "representation_checkpoint_path=" +
      representation_checkpoint.string() +
      "\n"
      "total_valid_target_count=6\n"
      "mean_valid_node_target_fraction=0.25\n"
      "last_active_node_head_count=3\n"
      "last_evaluated_node_head_count=3\n"
      "target_coords=0,1\n"
      "node_ids=BTC,ETH,SOL\n"
      "routed_row_count_by_node=10,10,10\n"
      "active_row_count_by_node=10,0,0\n"
      "trained_row_count_by_node=0,0,0\n"
      "evaluated_row_count_by_node=10,0,0\n"
      "valid_target_count_by_node=6,0,0\n"
      "mean_nll_by_node=0.45,nan,nan\n"
      "checkpoint_written=" +
      (write_output_checkpoint ? "true" : "false") + "\n";
  write_text(dir / "inference.report", report_text);
}

void write_mdn_job(const std::filesystem::path &root,
                   const std::filesystem::path &checkpoint,
                   const std::filesystem::path &representation_checkpoint) {
  const auto dir = root / "mdn_ready";
  write_text(dir / "job.manifest",
             "job_id=mdn_ready\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=1\n"
                                            "last_loss=0.5\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=") +
                                    checkpoint.string() + "\n");
  const auto report_text = std::string("optimizer_steps=1\n"
                                       "mean_loss=0.4\n"
                                       "representation_checkpoint_loaded=true\n"
                                       "allow_untrained_representation=false\n"
                                       "representation_checkpoint_path=") +
                           representation_checkpoint.string() +
                           "\n"
                           "mean_valid_node_target_fraction=0.25\n"
                           "last_active_node_head_count=3\n"
                           "last_evaluated_node_head_count=3\n"
                           "checkpoint_written=true\n"
                           "checkpoint_path=" +
                           checkpoint.string() + "\n";
  write_text(dir / "inference.report", report_text);
  write_text(checkpoint, "checkpoint");
  write_text(representation_checkpoint, "representation checkpoint");
}

void write_mdn_without_representation_checkpoint(
    const std::filesystem::path &root,
    const std::filesystem::path &checkpoint) {
  const auto dir = root / "mdn_no_representation";
  write_text(dir / "job.manifest",
             "job_id=mdn_no_representation\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", std::string("status=completed\n"
                                            "optimizer_steps=1\n"
                                            "last_loss=0.5\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=") +
                                    checkpoint.string() + "\n");
  write_text(dir / "inference.report",
             std::string("optimizer_steps=1\n"
                         "mean_loss=0.4\n"
                         "representation_checkpoint_loaded=false\n"
                         "allow_untrained_representation=true\n"
                         "mean_valid_node_target_fraction=0.25\n"
                         "last_active_node_head_count=3\n"
                         "last_evaluated_node_head_count=3\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 checkpoint.string() + "\n");
  write_text(checkpoint, "checkpoint");
}

void write_bad_representation_job(const std::filesystem::path &root,
                                  const std::string &label,
                                  const std::string &status = "completed") {
  const auto dir = root / label;
  write_text(dir / "job.manifest",
             "job_id=" + label +
                 "\n"
                 "job_kind=representation_vicreg\n"
                 "target_component=wikimyei.representation.encoding.vicreg\n"
                 "wave_action=train\n"
                 "source_range_policy=all\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=" + status +
                                    "\n"
                                    "optimizer_steps=1\n"
                                    "last_loss=0.25\n"
                                    "checkpoint_written=false\n");
  write_text(dir / "representation.report", "optimizer_steps=1\n"
                                            "mean_loss=0.20\n"
                                            "checkpoint_written=false\n");
}

std::string single_rep_target_dsl(std::int64_t max_waves) {
  return std::string(R"DSL(
LATTICE_TARGET {
  TARGET_ID = vicreg_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  PLAN_MODE = train|debug;
  MAX_WAVES = )DSL") +
         std::to_string(max_waves) + R"DSL(;
};
)DSL";
}

std::string split_policy_dsl() {
  return R"DSL(
LATTICE_SPLIT_POLICY {
  POLICY_ID = unit_holdout;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 200;
  ANCHOR_INDEX_END = 210;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};

LATTICE_SPLIT {
  SPLIT_ID = test_holdout;
  ROLE = test;
  ANCHOR_INDEX_BEGIN = 220;
  ANCHOR_INDEX_END = 230;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
)DSL";
}

std::string acceptance_split_policy_dsl() {
  return R"DSL(
LATTICE_SPLIT_POLICY {
  POLICY_ID = acceptance_holdout;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = acceptance_smoke;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 29;
  ANCHOR_INDEX_END = 31;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
  ALLOW_USES = evaluation_metric;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
)DSL";
}

std::string acceptance_target_dsl() {
  return R"DSL(
LATTICE_TARGET {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = acceptance_smoke;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  FORBID_SPLIT = validation_holdout;
  FORBID_EXPOSURE_USES = observed_input|target_supervision|selection_signal;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_acceptance_smoke_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = acceptance_smoke;
  UPSTREAM_TARGET_ID = vicreg_acceptance_smoke_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_TRAINED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  FORBID_SPLIT = validation_holdout;
  FORBID_EXPOSURE_USES = observed_input|target_supervision|selection_signal;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = node_mdn_acceptance_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:node_mdn_acceptance_smoke_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL";
}

target::lattice_target_evaluator_options_t
acceptance_options_for_runtime_root(const std::filesystem::path &root) {
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = root;
  options.split_policy = split::decode_lattice_split_policy_from_dsl(
      acceptance_split_policy_dsl());
  options.active_identity.protocol_contract_fingerprint = "contract_acceptance";
  options.active_identity.graph_order_fingerprint = "graph_acceptance";
  options.active_identity.source_cursor_token = "cursor_acceptance";
  options.active_identity.vicreg_assembly_fingerprint = "vicreg_acceptance";
  options.active_identity.mdn_assembly_fingerprint = "mdn_acceptance";
  return options;
}

void run_acceptance_fixture_regression() {
  const auto fixture_root = lattice_acceptance_fixture_root();
  const auto specs =
      target::decode_lattice_targets_from_dsl(acceptance_target_dsl());

  const auto good_root = fixture_root / "good";
  target::lattice_target_evaluator_t good_eval(
      specs, acceptance_options_for_runtime_root(good_root));
  const auto good_vicreg = good_eval.evaluate("vicreg_acceptance_smoke_ready");
  check(good_vicreg.status == target::lattice_target_status_t::satisfied,
        "acceptance fixture VICReg target should be satisfied");
  const auto good_mdn = good_eval.evaluate("node_mdn_acceptance_smoke_ready");
  check(good_mdn.status == target::lattice_target_status_t::satisfied,
        "acceptance fixture node MDN target should be satisfied");
  const auto good_no_validation_leakage =
      good_eval.evaluate("node_mdn_acceptance_no_validation_leakage");
  check(good_no_validation_leakage.status ==
            target::lattice_target_status_t::satisfied,
        "acceptance fixture node MDN validation leakage guard should be "
        "satisfied");

  const auto good_scan =
      exposure::scan_exposure_ledger_from_runtime_root(good_root);
  check(good_scan.ledger.facts().size() == 2,
        "acceptance fixture should expose VICReg and MDN facts");
  check(good_scan.ledger.checkpoint_facts().size() == 2,
        "acceptance fixture should expose VICReg and MDN checkpoint facts");
  const auto good_closure = good_scan.ledger.checkpoint_closure_result(
      good_root / "mdn_acceptance" / "inference.report.mdn.pt");
  check(good_closure.complete(),
        "good MDN acceptance checkpoint closure should be complete");
  check(good_closure.facts.size() == 2,
        "good MDN acceptance closure should contain MDN and VICReg facts");

  const auto broken_root = fixture_root / "broken_lineage";
  target::lattice_target_evaluator_t broken_eval(
      specs, acceptance_options_for_runtime_root(broken_root));
  const auto broken_vicreg =
      broken_eval.evaluate("vicreg_acceptance_smoke_ready");
  check(broken_vicreg.status == target::lattice_target_status_t::satisfied,
        "broken fixture keeps upstream VICReg target satisfied");
  const auto broken_mdn =
      broken_eval.evaluate("node_mdn_acceptance_smoke_ready");
  check(broken_mdn.status == target::lattice_target_status_t::exposure_failed,
        "broken lineage fixture should fail closed");
  const auto broken_no_validation_leakage =
      broken_eval.evaluate("node_mdn_acceptance_no_validation_leakage");
  check(broken_no_validation_leakage.status ==
            target::lattice_target_status_t::blocked,
        "validation leakage guard should block when its CHECKPOINT_SOURCE "
        "target is not satisfied");

  const auto broken_scan =
      exposure::scan_exposure_ledger_from_runtime_root(broken_root);
  const auto broken_closure = broken_scan.ledger.checkpoint_closure_result(
      broken_root / "mdn_acceptance" / "inference.report.mdn.pt");
  check(!broken_closure.complete(),
        "broken MDN acceptance checkpoint closure should be incomplete");
  check(!broken_closure.unresolved_input_checkpoints.empty(),
        "broken MDN acceptance closure should report unresolved input "
        "checkpoint lineage");

  const auto wrong_mdn_fixture = lattice_wrong_mdn_validation_fixture_root();
  const auto wrong_mdn_runtime_root = wrong_mdn_fixture / "runtime_root";
  const auto wrong_mdn_specs =
      target::load_lattice_targets_from_config(wrong_mdn_fixture / ".config");
  target::lattice_target_evaluator_options_t wrong_mdn_options{};
  wrong_mdn_options.runtime_root = wrong_mdn_runtime_root;
  wrong_mdn_options.split_policy =
      *target::load_lattice_split_policy_from_config_if_available(
          wrong_mdn_fixture / ".config");
  wrong_mdn_options.active_identity.protocol_contract_fingerprint =
      "contract_fixture";
  wrong_mdn_options.active_identity.graph_order_fingerprint = "graph_fixture";
  wrong_mdn_options.active_identity.source_cursor_token = "cursor_fixture";
  wrong_mdn_options.active_identity.vicreg_assembly_fingerprint =
      "vicreg_fixture";
  wrong_mdn_options.active_identity.mdn_assembly_fingerprint = "mdn_fixture";
  target::lattice_target_evaluator_t wrong_mdn_eval(wrong_mdn_specs,
                                                    wrong_mdn_options);
  const auto wrong_mdn_train =
      wrong_mdn_eval.evaluate("fixture_node_mdn_train_core_ready");
  check(wrong_mdn_train.status == target::lattice_target_status_t::satisfied,
        "wrong-MDN fixture should keep train-core MDN target satisfied");
  const auto wrong_mdn_guard =
      wrong_mdn_eval.evaluate("fixture_node_mdn_no_validation_leakage");
  check(wrong_mdn_guard.status == target::lattice_target_status_t::satisfied,
        "wrong-MDN fixture should keep the no-validation-leakage guard "
        "satisfied");
  const auto wrong_mdn_validation =
      wrong_mdn_eval.evaluate("fixture_node_mdn_validation_eval_ready");
  check(wrong_mdn_validation.status ==
            target::lattice_target_status_t::exposure_failed,
        "wrong-MDN validation fixture should fail validation evaluation");
  check(std::any_of(wrong_mdn_validation.deficits.begin(),
                    wrong_mdn_validation.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "dependency:mdn_checkpoint" &&
                             deficit.kind == "dependency" &&
                             deficit.dimension == "mdn_checkpoint" &&
                             deficit.status == "mismatch";
                    }),
        "wrong-MDN validation fixture should expose an MDN checkpoint mismatch "
        "deficit");
  check(!wrong_mdn_validation.proof_certificate_check.passed &&
            std::any_of(
                wrong_mdn_validation.proof_certificate_check.issues.begin(),
                wrong_mdn_validation.proof_certificate_check.issues.end(),
                [](const auto &issue) {
                  return issue.find("checkpoint mismatch") != std::string::npos;
                }),
        "wrong-MDN validation fixture should expose a certificate-check "
        "mismatch issue for the inconsistent checkpoint-source binding");
}

void run_channel_target_kind_regression() {
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_channel_mdn_component;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
};
)DSL",
                      "COMPONENT does not match TARGET_KIND",
                      "channel MDN target kind must not accept representation "
                      "component");

  const auto specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = vicreg_train_core_ready;
  TARGET_KIND = vicreg_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_ready;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 300;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 310;
  FORBID_EXPOSURE_USES = observed_input|target_supervision|selection_signal;
  FORBID_EXPOSURE_REQUIRES_MUTATED_COMPONENT = true;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_no_test_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_no_validation_leakage;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 400;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 410;
  FORBID_EXPOSURE_USES = observed_input|target_supervision|selection_signal;
  FORBID_EXPOSURE_REQUIRES_MUTATED_COMPONENT = true;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 300;
  ANCHOR_INDEX_END = 310;
  UPSTREAM_TARGET_ID = channel_mdn_train_core_no_test_leakage;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_VALID_TARGET_FRACTION = 0.05;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_no_test_leakage;
  MIN_EVALUATION_METRIC_COVERAGE = 1.0;
  PLAN_MODE = run|debug;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  SCOPE = target_component;
  EFFECT = mutated_component;
  METRIC = mean_nll;
  ABOVE = 0.30;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_train_core_ready;
  PLAN_ID = train_channel_mdn_train_core;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  PLAN_ID = run_channel_mdn_validation_eval;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = run|debug;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:channel_mdn_train_core_no_test_leakage;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 1;
};
)DSL");
  check(specs.size() == 5,
        "expected channel train, guard, and validation targets");
  check(target::lattice_target_kind_name(specs[0].kind) ==
                std::string("vicreg_ready") &&
            target::lattice_target_kind_name(specs[1].kind) ==
                std::string("channel_mdn_ready"),
        "channel target kinds should round-trip through parser");

  const auto root = make_tmp_dir("channel_lattice_targets");
  const auto channel_rep_checkpoint = root / "channel_rep.pt";
  const auto channel_mdn_checkpoint = root / "channel_mdn.pt";
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = root;
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_ranged";
  options.active_identity.vicreg_assembly_fingerprint = "vicreg_1";
  options.active_identity.mdn_assembly_fingerprint = "mdn_1";

  target::lattice_target_evaluator_t empty_eval(specs, options);
  const auto missing_channel_rep =
      empty_eval.evaluate("vicreg_train_core_ready");
  check(missing_channel_rep.status ==
                target::lattice_target_status_t::missing_report &&
            missing_channel_rep.plan_ready &&
            missing_channel_rep.suggested_wave.target ==
                "wikimyei.representation.encoding.vicreg",
        "missing channel VICReg target should plan the channel representation "
        "wave");
  const auto blocked_channel_mdn =
      empty_eval.evaluate("channel_mdn_train_core_ready");
  check(blocked_channel_mdn.status ==
                target::lattice_target_status_t::blocked &&
            blocked_channel_mdn.suggested_wave.target ==
                "wikimyei.representation.encoding.vicreg",
        "missing channel MDN upstream should forward the channel VICReg plan");
  const auto blocked_channel_guard =
      empty_eval.evaluate("channel_mdn_train_core_no_validation_leakage");
  check(blocked_channel_guard.status ==
            target::lattice_target_status_t::blocked,
        "channel MDN leakage guard should block until its CHECKPOINT_SOURCE "
        "target is satisfied");
  const auto blocked_channel_test_guard =
      empty_eval.evaluate("channel_mdn_train_core_no_test_leakage");
  check(blocked_channel_test_guard.status ==
            target::lattice_target_status_t::blocked,
        "channel MDN test leakage guard should block until the validation "
        "leakage guard is satisfied");

  write_ranged_channel_representation_job(root, channel_rep_checkpoint, 100,
                                          200);
  exposure::lattice_exposure_ledger_t rep_only_ledger{};
  const auto channel_rep_fact =
      make_channel_rep_exposure_fact(channel_rep_checkpoint, 100, 200);
  rep_only_ledger.add(channel_rep_fact);
  rep_only_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(channel_rep_fact));
  options.exposure_ledger = &rep_only_ledger;
  target::lattice_target_evaluator_t rep_only_eval(specs, options);
  const auto channel_rep_ready =
      rep_only_eval.evaluate("vicreg_train_core_ready");
  check(channel_rep_ready.status == target::lattice_target_status_t::satisfied,
        "channel VICReg readiness should accept channel representation jobs "
        "and reports");
  exposure::lattice_exposure_ledger_t bad_channel_rep_contract_ledger{};
  auto bad_channel_rep_contract_fact = channel_rep_fact;
  bad_channel_rep_contract_fact.representation_contract =
      "graph_order.node_representation.v1";
  bad_channel_rep_contract_ledger.add(bad_channel_rep_contract_fact);
  bad_channel_rep_contract_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_channel_rep_contract_fact));
  target::lattice_target_evaluator_options_t bad_channel_rep_contract_options =
      options;
  bad_channel_rep_contract_options.exposure_ledger =
      &bad_channel_rep_contract_ledger;
  target::lattice_target_evaluator_t bad_channel_rep_contract_eval(
      specs, bad_channel_rep_contract_options);
  const auto bad_channel_rep_contract_ready =
      bad_channel_rep_contract_eval.evaluate("vicreg_train_core_ready");
  check(bad_channel_rep_contract_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(bad_channel_rep_contract_ready.reasons.begin(),
                        bad_channel_rep_contract_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("semantic contract mismatch") !=
                                     std::string::npos &&
                                 reason.find("representation_contract") !=
                                     std::string::npos;
                        }),
        "channel VICReg readiness rejects fused representation contracts");

  exposure::lattice_exposure_ledger_t bad_channel_rep_health_ledger{};
  auto bad_channel_rep_health_fact = channel_rep_fact;
  bad_channel_rep_health_fact.representation_health_available = false;
  bad_channel_rep_health_ledger.add(bad_channel_rep_health_fact);
  bad_channel_rep_health_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_channel_rep_health_fact));
  target::lattice_target_evaluator_options_t bad_channel_rep_health_options =
      options;
  bad_channel_rep_health_options.exposure_ledger =
      &bad_channel_rep_health_ledger;
  target::lattice_target_evaluator_t bad_channel_rep_health_eval(
      specs, bad_channel_rep_health_options);
  const auto bad_channel_rep_health_ready =
      bad_channel_rep_health_eval.evaluate("vicreg_train_core_ready");
  check(
      bad_channel_rep_health_ready.status ==
              target::lattice_target_status_t::exposure_failed &&
          std::any_of(bad_channel_rep_health_ready.reasons.begin(),
                      bad_channel_rep_health_ready.reasons.end(),
                      [](const auto &reason) {
                        return reason.find("representation health mismatch") !=
                                   std::string::npos &&
                               reason.find("representation_health_available") !=
                                   std::string::npos;
                      }),
      "channel VICReg readiness rejects channel representations without "
      "health diagnostics");

  exposure::lattice_exposure_ledger_t bad_channel_augmented_health_ledger{};
  auto bad_channel_augmented_health_fact = channel_rep_fact;
  bad_channel_augmented_health_fact.augmented_valid_feature_count = 0;
  bad_channel_augmented_health_fact.mean_augmented_valid_feature_fraction = 0.0;
  bad_channel_augmented_health_fact.mean_augmented_feature_retention_fraction =
      0.0;
  bad_channel_augmented_health_ledger.add(bad_channel_augmented_health_fact);
  bad_channel_augmented_health_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_channel_augmented_health_fact));
  target::lattice_target_evaluator_options_t
      bad_channel_augmented_health_options = options;
  bad_channel_augmented_health_options.exposure_ledger =
      &bad_channel_augmented_health_ledger;
  target::lattice_target_evaluator_t bad_channel_augmented_health_eval(
      specs, bad_channel_augmented_health_options);
  const auto bad_channel_augmented_health_ready =
      bad_channel_augmented_health_eval.evaluate("vicreg_train_core_ready");
  check(bad_channel_augmented_health_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(bad_channel_augmented_health_ready.reasons.begin(),
                        bad_channel_augmented_health_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find(
                                     "representation health mismatch") !=
                                     std::string::npos &&
                                 reason.find("augmented_valid_feature_count") !=
                                     std::string::npos;
                        }),
        "channel VICReg readiness rejects representations with no retained "
        "augmented view evidence");

  const auto missing_channel_mdn =
      rep_only_eval.evaluate("channel_mdn_train_core_ready");
  check(
      missing_channel_mdn.status ==
              target::lattice_target_status_t::missing_report &&
          missing_channel_mdn.plan_ready &&
          missing_channel_mdn.suggested_wave.target ==
              "wikimyei.inference.expected_value.mdn" &&
          missing_channel_mdn.suggested_wave.input_representation_checkpoint ==
              "latest_satisfying:vicreg_train_core_ready",
      "satisfied channel upstream should let channel MDN plan its own wave "
      "with the channel VICReg checkpoint input");

  write_ranged_channel_mdn_job(root, channel_mdn_checkpoint,
                               channel_rep_checkpoint, 100, 200);
  exposure::lattice_exposure_ledger_t full_ledger = rep_only_ledger;
  const auto channel_mdn_fact = make_channel_mdn_exposure_fact(
      channel_mdn_checkpoint, channel_rep_checkpoint, 100, 200);
  full_ledger.add(channel_mdn_fact);
  full_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact));
  exposure::lattice_exposure_ledger_t bad_channel_mdn_contract_ledger =
      rep_only_ledger;
  auto bad_channel_mdn_contract_fact = channel_mdn_fact;
  bad_channel_mdn_contract_fact.context_contract =
      "graph_order.node_representation.v1";
  bad_channel_mdn_contract_ledger.add(bad_channel_mdn_contract_fact);
  bad_channel_mdn_contract_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_channel_mdn_contract_fact));
  target::lattice_target_evaluator_options_t bad_channel_mdn_contract_options =
      options;
  bad_channel_mdn_contract_options.exposure_ledger =
      &bad_channel_mdn_contract_ledger;
  target::lattice_target_evaluator_t bad_channel_mdn_contract_eval(
      specs, bad_channel_mdn_contract_options);
  const auto bad_channel_mdn_contract_ready =
      bad_channel_mdn_contract_eval.evaluate("channel_mdn_train_core_ready");
  check(bad_channel_mdn_contract_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(bad_channel_mdn_contract_ready.reasons.begin(),
                        bad_channel_mdn_contract_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("semantic contract mismatch") !=
                                     std::string::npos &&
                                 reason.find("context_contract") !=
                                     std::string::npos;
                        }),
        "channel MDN readiness rejects non-channel context contracts");

  exposure::lattice_exposure_ledger_t bad_channel_mdn_health_ledger =
      rep_only_ledger;
  auto bad_channel_mdn_health_fact = channel_mdn_fact;
  bad_channel_mdn_health_fact.inference_health_available = false;
  bad_channel_mdn_health_ledger.add(bad_channel_mdn_health_fact);
  bad_channel_mdn_health_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_channel_mdn_health_fact));
  target::lattice_target_evaluator_options_t bad_channel_mdn_health_options =
      options;
  bad_channel_mdn_health_options.exposure_ledger =
      &bad_channel_mdn_health_ledger;
  target::lattice_target_evaluator_t bad_channel_mdn_health_eval(
      specs, bad_channel_mdn_health_options);
  const auto bad_channel_mdn_health_ready =
      bad_channel_mdn_health_eval.evaluate("channel_mdn_train_core_ready");
  check(bad_channel_mdn_health_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(bad_channel_mdn_health_ready.reasons.begin(),
                        bad_channel_mdn_health_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("channel MDN health mismatch") !=
                                     std::string::npos &&
                                 reason.find("inference_health_available") !=
                                     std::string::npos;
                        }),
        "channel MDN readiness rejects channel inference reports without "
        "distribution health diagnostics");

  exposure::lattice_exposure_ledger_t nonfinite_channel_mdn_ledger =
      rep_only_ledger;
  auto nonfinite_channel_mdn_fact = channel_mdn_fact;
  nonfinite_channel_mdn_fact.nonfinite_output_count = 2;
  nonfinite_channel_mdn_ledger.add(nonfinite_channel_mdn_fact);
  nonfinite_channel_mdn_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          nonfinite_channel_mdn_fact));
  target::lattice_target_evaluator_options_t nonfinite_channel_mdn_options =
      options;
  nonfinite_channel_mdn_options.exposure_ledger = &nonfinite_channel_mdn_ledger;
  target::lattice_target_evaluator_t nonfinite_channel_mdn_eval(
      specs, nonfinite_channel_mdn_options);
  const auto nonfinite_channel_mdn_ready =
      nonfinite_channel_mdn_eval.evaluate("channel_mdn_train_core_ready");
  check(nonfinite_channel_mdn_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(nonfinite_channel_mdn_ready.reasons.begin(),
                        nonfinite_channel_mdn_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("channel MDN health mismatch") !=
                                     std::string::npos &&
                                 reason.find("nonfinite_output_count") !=
                                     std::string::npos;
                        }),
        "channel MDN readiness rejects reports with non-finite distribution "
        "outputs");

  options.exposure_ledger = &full_ledger;
  target::lattice_target_evaluator_t full_eval(specs, options);
  const auto channel_mdn_ready =
      full_eval.evaluate("channel_mdn_train_core_ready");
  check(channel_mdn_ready.status == target::lattice_target_status_t::satisfied,
        "channel MDN readiness should consume channel inference reports and "
        "channel representation producer lineage");
  check(channel_mdn_ready.evidence.report_path.filename() ==
            std::filesystem::path("channel_inference.report"),
        "channel MDN evidence should use the channel inference report");
  check(channel_mdn_ready.node_support_summaries.empty(),
        "channel MDN readiness should not synthesize old node-head support");
  check(std::any_of(channel_mdn_ready.warning_results.begin(),
                    channel_mdn_ready.warning_results.end(),
                    [](const auto &warning) {
                      return warning.warning_id ==
                                 "high_channel_mdn_mean_nll" &&
                             warning.status == "warning" &&
                             warning.measurement_available;
                    }),
        "channel MDN readiness should evaluate channel MDN distribution "
        "warnings");
  const auto channel_mdn_proof_text =
      target::canonical_lattice_target_proof_certificate_text(
          channel_mdn_ready.proof_certificate);
  check(channel_mdn_proof_text.find("context_contract="
                                    "graph_order.channel_node_representation."
                                    "v1") != std::string::npos,
        "channel MDN proof certificate exposes strict-channel checkpoint "
        "context contract");
  check(channel_mdn_proof_text.find("representation_contract="
                                    "graph_order.channel_node_representation."
                                    "v1") != std::string::npos,
        "channel MDN proof certificate exposes channel representation "
        "producer contract");
  const auto channel_mdn_guard =
      full_eval.evaluate("channel_mdn_train_core_no_validation_leakage");
  check(channel_mdn_guard.status == target::lattice_target_status_t::satisfied,
        "channel MDN leakage guard should accept clean channel checkpoint "
        "lineage");
  const auto channel_mdn_test_guard =
      full_eval.evaluate("channel_mdn_train_core_no_test_leakage");
  check(channel_mdn_test_guard.status ==
            target::lattice_target_status_t::satisfied,
        "channel MDN test leakage guard should accept clean strict-channel "
        "checkpoint lineage after the validation guard");
  const auto missing_channel_validation_eval =
      full_eval.evaluate("channel_mdn_validation_eval_ready");
  check(
      missing_channel_validation_eval.status ==
              target::lattice_target_status_t::missing_report &&
          missing_channel_validation_eval.plan_ready &&
          missing_channel_validation_eval.suggested_wave.target ==
              "wikimyei.inference.expected_value.mdn" &&
          missing_channel_validation_eval.suggested_wave.mode == "run|debug" &&
          missing_channel_validation_eval.suggested_wave.input_mdn_checkpoint ==
              "latest_satisfying:"
              "channel_mdn_train_core_no_test_leakage" &&
          missing_channel_validation_eval.suggested_wave
                  .input_representation_checkpoint ==
              "latest_satisfying:vicreg_train_core_ready",
      "satisfied strict-channel training should plan run-mode validation "
      "evaluation with exact channel MDN and representation checkpoint inputs");

  write_ranged_channel_mdn_eval_job(root, channel_mdn_checkpoint,
                                    channel_rep_checkpoint, 300, 310);
  exposure::lattice_exposure_ledger_t validation_eval_ledger = full_ledger;
  const auto channel_validation_eval_fact =
      exposure::make_exposure_fact_from_job_dir(root /
                                                "channel_mdn_validation_eval");
  validation_eval_ledger.add(channel_validation_eval_fact);
  target::lattice_target_evaluator_options_t validation_eval_options = options;
  validation_eval_options.exposure_ledger = &validation_eval_ledger;
  target::lattice_target_evaluator_t validation_eval(specs,
                                                     validation_eval_options);
  const auto channel_validation_eval_ready =
      validation_eval.evaluate("channel_mdn_validation_eval_ready");
  check(channel_validation_eval_ready.status ==
            target::lattice_target_status_t::satisfied,
        "strict-channel validation evaluation should satisfy with a run-mode "
        "channel MDN report and no new checkpoint");
  check(channel_validation_eval_ready.evidence.report_path.filename() ==
            std::filesystem::path("channel_inference.report"),
        "strict-channel validation evaluation should consume channel inference "
        "reports");
  check(channel_validation_eval_ready.evidence.optimizer_steps == 0 &&
            !channel_validation_eval_ready.evidence.checkpoint_written,
        "strict-channel validation evaluation evidence should be read-only");
  check(channel_validation_eval_ready.exposure_summaries.size() == 1 &&
            channel_validation_eval_ready.exposure_summaries.front().use ==
                exposure::exposure_use_t::evaluation_metric &&
            channel_validation_eval_ready.exposure_summaries.front()
                    .loaded_anchor_events == 10,
        "strict-channel validation evaluation should report evaluation_metric "
        "coverage over the validation range");
  const auto channel_eval_dependency = std::find_if(
      channel_validation_eval_ready.proof_certificate.dependencies.begin(),
      channel_validation_eval_ready.proof_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  check(channel_eval_dependency != channel_validation_eval_ready
                                       .proof_certificate.dependencies.end() &&
            channel_eval_dependency->mdn_checkpoint_match &&
            channel_eval_dependency->representation_checkpoint_match,
        "strict-channel validation proof should bind exactly to the evaluated "
        "channel MDN and representation checkpoints");
  check(
      channel_validation_eval_ready.proof_certificate_check.passed &&
          channel_validation_eval_ready.proof_certificate_check.issues.empty(),
      "strict-channel validation proof certificate should be self-consistent");

  exposure::lattice_exposure_ledger_t leaky_channel_ledger{};
  leaky_channel_ledger.add(channel_rep_fact);
  leaky_channel_ledger.add(channel_mdn_fact);
  auto leaky_channel_mdn_fact = make_channel_mdn_exposure_fact(
      channel_mdn_checkpoint, channel_rep_checkpoint, 303, 306);
  leaky_channel_ledger.add(leaky_channel_mdn_fact);
  target::lattice_target_evaluator_options_t leaky_channel_options = options;
  leaky_channel_options.exposure_ledger = &leaky_channel_ledger;
  target::lattice_target_evaluator_t leaky_channel_eval(specs,
                                                        leaky_channel_options);
  const auto leaky_channel_guard = leaky_channel_eval.evaluate(
      "channel_mdn_train_core_no_validation_leakage");
  check(leaky_channel_guard.status ==
            target::lattice_target_status_t::exposure_failed,
        "channel MDN leakage guard should fail when the selected checkpoint "
        "closure touches validation evidence");

  const auto standalone_mdn_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = standalone_channel_mdn_ready;
  TARGET_KIND = channel_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  exposure::lattice_exposure_ledger_t bad_loaded_rep_contract_ledger{};
  auto bad_loaded_rep_contract_fact = channel_rep_fact;
  bad_loaded_rep_contract_fact.channel_axis_policy =
      "fused_before_primary_output";
  bad_loaded_rep_contract_ledger.add(bad_loaded_rep_contract_fact);
  bad_loaded_rep_contract_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_loaded_rep_contract_fact));
  bad_loaded_rep_contract_ledger.add(channel_mdn_fact);
  bad_loaded_rep_contract_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact));
  target::lattice_target_evaluator_options_t bad_loaded_rep_contract_options =
      options;
  bad_loaded_rep_contract_options.exposure_ledger =
      &bad_loaded_rep_contract_ledger;
  target::lattice_target_evaluator_t bad_loaded_rep_contract_eval(
      standalone_mdn_specs, bad_loaded_rep_contract_options);
  const auto bad_loaded_rep_contract_ready =
      bad_loaded_rep_contract_eval.evaluate("standalone_channel_mdn_ready");
  check(bad_loaded_rep_contract_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(bad_loaded_rep_contract_ready.reasons.begin(),
                        bad_loaded_rep_contract_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("loaded representation "
                                             "checkpoint semantic contract "
                                             "mismatch") != std::string::npos &&
                                 reason.find("channel_axis_policy") !=
                                     std::string::npos;
                        }),
        "channel MDN readiness rejects loaded representation checkpoints whose "
        "producer erased the channel axis");

  exposure::lattice_exposure_ledger_t bad_loaded_rep_health_ledger{};
  auto bad_loaded_rep_health_fact = channel_rep_fact;
  bad_loaded_rep_health_fact.representation_effective_rank_fraction = 0.0;
  bad_loaded_rep_health_ledger.add(bad_loaded_rep_health_fact);
  bad_loaded_rep_health_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(
          bad_loaded_rep_health_fact));
  bad_loaded_rep_health_ledger.add(channel_mdn_fact);
  bad_loaded_rep_health_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact));
  target::lattice_target_evaluator_options_t bad_loaded_rep_health_options =
      options;
  bad_loaded_rep_health_options.exposure_ledger = &bad_loaded_rep_health_ledger;
  target::lattice_target_evaluator_t bad_loaded_rep_health_eval(
      standalone_mdn_specs, bad_loaded_rep_health_options);
  const auto bad_loaded_rep_health_ready =
      bad_loaded_rep_health_eval.evaluate("standalone_channel_mdn_ready");
  check(bad_loaded_rep_health_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(
                bad_loaded_rep_health_ready.reasons.begin(),
                bad_loaded_rep_health_ready.reasons.end(),
                [](const auto &reason) {
                  return reason.find("loaded representation checkpoint "
                                     "health mismatch") != std::string::npos &&
                         reason.find(
                             "representation_effective_rank_fraction") !=
                             std::string::npos;
                }),
        "channel MDN readiness rejects loaded representation checkpoints whose "
        "health diagnostics show collapsed channel evidence");

  exposure::lattice_exposure_ledger_t wrong_rep_ledger{};
  const auto old_rep_fact =
      make_rep_exposure_fact(channel_rep_checkpoint, 100, 200);
  wrong_rep_ledger.add(old_rep_fact);
  wrong_rep_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(old_rep_fact));
  wrong_rep_ledger.add(channel_mdn_fact);
  wrong_rep_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact));
  options.exposure_ledger = &wrong_rep_ledger;
  target::lattice_target_evaluator_t wrong_rep_eval(standalone_mdn_specs,
                                                    options);
  const auto wrong_rep_ready =
      wrong_rep_eval.evaluate("standalone_channel_mdn_ready");
  check(wrong_rep_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            std::any_of(wrong_rep_ready.reasons.begin(),
                        wrong_rep_ready.reasons.end(),
                        [](const auto &reason) {
                          return reason.find("loaded representation checkpoint "
                                             "semantic contract mismatch") !=
                                     std::string::npos &&
                                 reason.find("representation_architecture") !=
                                     std::string::npos;
                        }),
        "channel MDN readiness must reject old fused VICReg producer lineage "
        "(status=" +
            std::string(
                target::lattice_target_status_name(wrong_rep_ready.status)) +
            ", reasons=" + join_strings(wrong_rep_ready.reasons, "; ") + ")");

  std::filesystem::remove_all(root);
}

} // namespace

int main() {
  const auto specs = target::decode_lattice_targets_from_dsl(target_dsl());
  check(specs.size() == 2, "expected two lattice targets");
  check(target::validate_lattice_target_specs(specs).ok(),
        "baseline target specs should have no warnings");
  const auto duplicate_component_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = rep_ready_a;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
};

LATTICE_TARGET {
  TARGET_ID = rep_ready_b;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
};
)DSL");
  check(!target::validate_lattice_target_specs(duplicate_component_specs).ok(),
        "duplicate target selector should produce a warning");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = duplicate_target_id;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
};

LATTICE_TARGET {
  TARGET_ID = duplicate_target_id;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
};
)DSL",
                      "duplicate TARGET_ID",
                      "duplicate target identifiers should fail fast");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = premature_performance_gate;
  TARGET_CLASS = validation_performance;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "reserved for future explicit validation performance",
                      "validation performance target syntax should fail "
                      "closed until uncertainty and selector policy are "
                      "implemented");
  expect_decode_error(R"DSL(
LATTICE_PROFILE {
  PROFILE_ID = duplicate_profile_id;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
};

LATTICE_PROFILE {
  PROFILE_ID = duplicate_profile_id;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "duplicate PROFILE_ID",
                      "duplicate profile identifiers should fail fast");
  expect_decode_error(R"DSL(
LATTICE_GUARD {
  GUARD_ID = duplicate_guard_id;
  KIND = exposure_overlap;
  SPLIT = validation_holdout;
  USES = observed_input;
};

LATTICE_GUARD {
  GUARD_ID = duplicate_guard_id;
  KIND = exposure_overlap;
  SPLIT = test_holdout;
  USES = target_supervision;
};
)DSL",
                      "duplicate GUARD_ID",
                      "duplicate guard identifiers should fail fast");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = unknown_profile_target;
  USE_PROFILE = missing_profile;
  OVER_SPLIT = train_core;
};
)DSL",
                      "unknown USE_PROFILE",
                      "targets should fail closed when a referenced profile is "
                      "missing");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = unknown_guard_target;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  GUARD = missing_guard;
};
)DSL",
                      "unknown GUARD",
                      "targets should fail closed when a referenced guard is "
                      "missing");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_missing_anchor_region;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
};
)DSL",
                      "SOURCE_RANGE=anchor_index requires either OVER_SPLIT or "
                      "ANCHOR_INDEX_BEGIN/END",
                      "graph-anchor targets should identify an evidence "
                      "region");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_region_order;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 10;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "ANCHOR_INDEX_END must be greater than "
                      "ANCHOR_INDEX_BEGIN",
                      "explicit anchor regions should have positive length");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_split_and_anchor_region;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
};
)DSL",
                      "OVER_SPLIT and ANCHOR_INDEX_BEGIN/END are mutually "
                      "exclusive",
                      "target evidence regions should not mix split and "
                      "explicit-anchor coordinates");
  const auto ranged_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 2;
  ANCHOR_INDEX_END = 5;
};
)DSL");
  check(ranged_specs.front().anchor_index_begin.has_value() &&
            *ranged_specs.front().anchor_index_begin == 2,
        "ranged target should decode ANCHOR_INDEX_BEGIN");
  check(ranged_specs.front().anchor_index_end.has_value() &&
            *ranged_specs.front().anchor_index_end == 5,
        "ranged target should decode ANCHOR_INDEX_END");
  const auto explicit_warning_range_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_warning_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
};

LATTICE_WARN {
  TARGET_ID = ranged_warning_ready;
  KIND = anchor_domain_health;
  ANCHOR_INDEX_BEGIN = 20;
  ANCHOR_INDEX_END = 30;
  ACCEPTED_FRACTION_BELOW = 0.8;
};

LATTICE_WARN {
  TARGET_ID = ranged_warning_ready;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 30;
  ANCHOR_INDEX_END = 40;
  METRIC = weakest_valid_target_count;
  BELOW = 1;
};

LATTICE_WARN {
  TARGET_ID = ranged_warning_ready;
  KIND = mdn_distribution_calibration;
  ANCHOR_INDEX_BEGIN = 40;
  ANCHOR_INDEX_END = 50;
  USE = target_supervision;
  EFFECT = mutated_component;
  METRIC = mean_nll;
  ABOVE = 1.0;
};
)DSL");
  check(explicit_warning_range_specs.front().warning_specs.size() == 3 &&
            explicit_warning_range_specs.front()
                    .warning_specs[0]
                    .anchor_index_begin == 20 &&
            explicit_warning_range_specs.front()
                    .warning_specs[0]
                    .anchor_index_end == 30 &&
            explicit_warning_range_specs.front()
                    .warning_specs[1]
                    .anchor_index_begin == 30 &&
            explicit_warning_range_specs.front()
                    .warning_specs[1]
                    .anchor_index_end == 40 &&
            explicit_warning_range_specs.front()
                    .warning_specs[2]
                    .anchor_index_begin == 40 &&
            explicit_warning_range_specs.front()
                    .warning_specs[2]
                    .anchor_index_end == 50,
        "non-load warning kinds should decode explicit anchor ranges");
  const auto profiled_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_GUARD {
  GUARD_ID = no_validation_training_leakage;
  KIND = exposure_overlap;
  SPLIT = validation_holdout;
  USES = observed_input|target_supervision|selection_signal;
  SCOPE = checkpoint_closure;
  EFFECT = mutated_component;
  COORDINATE = source_row_footprint;
};

LATTICE_PROFILE {
  PROFILE_ID = vicreg_training_readiness;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  GUARD = no_validation_training_leakage;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = profiled_representation_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_REQUIRES {
  TARGET_ID = profiled_representation_ready;
  REQUIREMENT_ID = observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  MIN_COVERAGE = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = profiled_representation_ready;
  WAVE_TARGET = wikimyei.representation.encoding.vicreg;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_MAX_ATTEMPTS = 3;
};
)DSL");
  check(profiled_specs.size() == 1, "profiled target should decode");
  check(profiled_specs.front().use_profile == "vicreg_training_readiness",
        "profile id should be retained for inspection");
  check(profiled_specs.front().guard_id == "no_validation_training_leakage",
        "guard id should be retained for inspection");
  check(profiled_specs.front().train_split == "train_core",
        "OVER_SPLIT alias should set train_split");
  check(profiled_specs.front().forbid_split == "validation_holdout",
        "GUARD should lower to FORBID_SPLIT");
  check(profiled_specs.front().forbid_exposure_uses.size() == 3,
        "GUARD should lower exposure uses");
  check(profiled_specs.front().plan_mode == "train|debug",
        "WAVE_MODE alias should set plan mode");
  check(profiled_specs.front().max_waves == 3,
        "PLAN_MAX_ATTEMPTS alias should set max_waves");
  check(profiled_specs.front().min_observed_input_coverage == 0.95,
        "LATTICE_REQUIRES exposure_coverage should lower to observed input "
        "coverage");

  const auto clause_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = clause_node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_DEPENDS {
  TARGET_ID = clause_node_mdn_ready;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = checkpoint_output;
  KIND = artifact;
  ARTIFACT = output_checkpoint;
  EXISTS = true;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = finite_nll;
  KIND = metric;
  METRIC = mean_nll;
  OP = finite;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = optimizer_effort;
  KIND = effort;
  COUNTER = optimizer_steps;
  OP = ge;
  VALUE = 2;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = target_density;
  KIND = valid_target_fraction;
  OP = ge;
  VALUE = 0.25;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = active_heads;
  KIND = node_head_count;
  HEAD = active;
  OP = ge;
  VALUE = 3;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = trained_heads;
  KIND = node_head_count;
  HEAD = trained;
  OP = ge;
  VALUE = 2;
};

LATTICE_REQUIRES {
  TARGET_ID = clause_node_mdn_ready;
  REQUIREMENT_ID = target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  CURSOR_EPOCHS = 0.8;
};

LATTICE_FORBIDS {
  TARGET_ID = clause_node_mdn_ready;
  FORBID_ID = no_validation_training_leakage;
  KIND = exposure_overlap;
  SPLIT = validation_holdout;
  USES = observed_input|target_supervision|selection_signal;
  SCOPE = checkpoint_closure;
  EFFECT = mutated_component;
  COORDINATE = source_row_footprint;
};

LATTICE_PLAN {
  TARGET_ID = clause_node_mdn_ready;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:clean_node_mdn_no_validation_leakage;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready;
  PLAN_MAX_ATTEMPTS = 4;
};
)DSL");
  check(clause_specs.size() == 1, "clause target should decode");
  const auto &clause_spec = clause_specs.front();
  check(clause_spec.upstream_target_id == "vicreg_train_core_ready",
        "LATTICE_DEPENDS should lower upstream target");
  check(clause_spec.require_checkpoint_exists &&
            clause_spec.require_finite_loss,
        "artifact and metric clauses should lower readiness requirements");
  check(clause_spec.min_optimizer_steps == 2,
        "effort clause should lower optimizer steps");
  check(clause_spec.min_valid_target_fraction == 0.25,
        "valid target fraction clause should lower threshold");
  check(clause_spec.require_active_node_head_count == 3,
        "node head count clause should lower active heads");
  check(clause_spec.require_trained_node_head_count == 2,
        "node head count clause should lower trained heads");
  check(clause_spec.min_target_supervision_coverage == 0.8,
        "exposure coverage clause should lower target supervision coverage");
  check(clause_spec.forbid_split == "validation_holdout",
        "LATTICE_FORBIDS should lower forbidden split");
  check(clause_spec.forbid_exposure_uses.size() == 3,
        "LATTICE_FORBIDS should lower forbidden uses");
  check(clause_spec.plan_mode == "train|debug" && clause_spec.max_waves == 4,
        "LATTICE_PLAN should lower wave mode and max attempts");
  check(clause_spec.plan_input_mdn_checkpoint ==
                "latest_satisfying:clean_node_mdn_no_validation_leakage" &&
            clause_spec.plan_input_representation_checkpoint ==
                "latest_satisfying:vicreg_train_core_ready",
        "LATTICE_PLAN should lower model-state input recommendations");

  const auto compiled_clause_targets =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = compiled_node_mdn_ready;
  TARGET_CLASS = readiness;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_DEPENDS {
  TARGET_ID = compiled_node_mdn_ready;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
  BINDING = loaded_representation_checkpoint;
  REQUIRE_EXACT_LOADED_CHECKPOINT = true;
};

LATTICE_REQUIRES {
  TARGET_ID = compiled_node_mdn_ready;
  REQUIREMENT_ID = target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  MIN_COVERAGE = 0.8;
};

LATTICE_PLAN {
  TARGET_ID = compiled_node_mdn_ready;
  PLAN_ID = train_mdn_train_core;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
  WAVE_MODE = train|debug;
  WAVE_RANGE = split:train_core;
  PLAN_MAX_ATTEMPTS = 4;
};

LATTICE_WARN {
  TARGET_ID = compiled_node_mdn_ready;
  WARNING_ID = high_target_supervision_load;
  KIND = exposure_load;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};
)DSL");
  check(compiled_clause_targets.size() == 1,
        "compiled target decoder should return one target");
  const auto &compiled_clause = compiled_clause_targets.front();
  check(compiled_clause.lowered_v0.target_id == "compiled_node_mdn_ready",
        "compiled target should preserve lowered target");
  check(compiled_clause.lowered_v0.target_class == "readiness",
        "compiled target should preserve target class");
  check(compiled_clause.dependencies.size() == 1 &&
            compiled_clause.dependencies.front().binding ==
                "loaded_representation_checkpoint",
        "compiled target should preserve dependency clauses");
  check(compiled_clause.requirements.size() == 1 &&
            compiled_clause.requirements.front().clause_id ==
                "target_supervision_train_core_coverage",
        "compiled target should preserve requirement clauses");
  check(compiled_clause.plans.size() == 1 &&
            compiled_clause.plans.front().clause_id == "train_mdn_train_core",
        "compiled target should preserve plan clauses");
  check(compiled_clause.warning_clauses.size() == 1 &&
            compiled_clause.warning_clauses.front().clause_id ==
                "high_target_supervision_load",
        "compiled target should preserve warning clauses");
  check(compiled_clause.lowered_v0.warning_specs.size() == 1 &&
            compiled_clause.lowered_v0.warning_specs.front()
                    .cursor_epochs_above == 3.0,
        "LATTICE_WARN should lower to warning specs");
  check(!compiled_clause.target_spec_fingerprint.empty(),
        "compiled target should have a deterministic fingerprint");
  check(compiled_clause.lowered_v0.target_spec_fingerprint ==
            compiled_clause.target_spec_fingerprint,
        "lowered target carries compiled fingerprint into evaluator input");

  const auto compiled_clause_changed =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = compiled_node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_REQUIRES {
  TARGET_ID = compiled_node_mdn_ready;
  REQUIREMENT_ID = target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  COORDINATE = graph_anchor_coverage;
  MIN_COVERAGE = 0.9;
};

LATTICE_WARN {
  TARGET_ID = compiled_node_mdn_ready;
  WARNING_ID = high_target_supervision_load;
  KIND = exposure_load;
  USE = target_supervision;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};
)DSL");
  check(compiled_clause.target_spec_fingerprint !=
            compiled_clause_changed.front().target_spec_fingerprint,
        "compiled target fingerprint should change when a clause changes");
  const auto compiled_model_state_inputs_a =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = runtime_model_state_input_ready;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:first_clean_mdn_source;
};

LATTICE_PLAN {
  TARGET_ID = runtime_model_state_input_ready;
  PLAN_ID = validation_eval_runtime_inputs;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:first_clean_rep_source;
};
)DSL");
  const auto compiled_model_state_inputs_b =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = runtime_model_state_input_ready;
  TARGET_KIND = node_mdn_ready;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:second_clean_mdn_source;
};

LATTICE_PLAN {
  TARGET_ID = runtime_model_state_input_ready;
  PLAN_ID = validation_eval_runtime_inputs;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:second_clean_rep_source;
};
)DSL");
  check(compiled_model_state_inputs_a.size() == 1 &&
            compiled_model_state_inputs_b.size() == 1,
        "runtime model-state input fixture should compile one target");
  check(compiled_model_state_inputs_a.front()
                    .lowered_v0.plan_input_mdn_checkpoint !=
                compiled_model_state_inputs_b.front()
                    .lowered_v0.plan_input_mdn_checkpoint &&
            compiled_model_state_inputs_a.front()
                    .lowered_v0.plan_input_representation_checkpoint !=
                compiled_model_state_inputs_b.front()
                    .lowered_v0.plan_input_representation_checkpoint,
        "runtime model-state input hints should stay available to planning");
  check(compiled_model_state_inputs_a.front().target_spec_fingerprint ==
            compiled_model_state_inputs_b.front().target_spec_fingerprint,
        "runtime model-state input hints should not alter target identity");
  const auto compiled_plan_advice_a =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = advisory_plan_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  const auto compiled_plan_advice_b =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = advisory_plan_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = advisory_plan_ready;
  PLAN_ID = alternate_advisory_plan;
  WAVE_MODE = run|debug;
  PLAN_MAX_ATTEMPTS = 0;
};
)DSL");
  check(compiled_plan_advice_a.front().lowered_v0.plan_mode !=
                compiled_plan_advice_b.front().lowered_v0.plan_mode &&
            compiled_plan_advice_a.front().lowered_v0.max_waves !=
                compiled_plan_advice_b.front().lowered_v0.max_waves,
        "advisory planning knobs should still lower into planning state");
  check(compiled_plan_advice_a.front().target_spec_fingerprint ==
            compiled_plan_advice_b.front().target_spec_fingerprint,
        "advisory planning knobs should not alter target identity");
  const auto compiled_warning_policy_a =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = warning_policy_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = warning_policy_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 2.0;
};
)DSL");
  const auto compiled_warning_policy_b =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = warning_policy_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = warning_policy_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 5.0;
};
)DSL");
  const auto compiled_warning_policy_c =
      target::decode_lattice_compiled_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = warning_policy_ready;
  TARGET_KIND = representation_ready;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};
)DSL");
  check(
      compiled_warning_policy_a.size() == 1 &&
          compiled_warning_policy_b.size() == 1 &&
          compiled_warning_policy_c.size() == 1 &&
          compiled_warning_policy_a.front().lowered_v0.warning_specs.size() ==
              1 &&
          compiled_warning_policy_b.front().lowered_v0.warning_specs.size() ==
              1 &&
          compiled_warning_policy_c.front().lowered_v0.warning_specs.empty() &&
          compiled_warning_policy_a.front().warning_clauses.size() == 1 &&
          compiled_warning_policy_b.front().warning_clauses.size() == 1 &&
          compiled_warning_policy_c.front().warning_clauses.empty(),
      "warning policy fixtures should preserve visible warning clauses outside "
      "proof identity");
  check(compiled_warning_policy_a.front()
                    .lowered_v0.warning_specs.front()
                    .cursor_epochs_above !=
                compiled_warning_policy_b.front()
                    .lowered_v0.warning_specs.front()
                    .cursor_epochs_above &&
            std::find_if(compiled_warning_policy_a.front()
                             .warning_clauses.front()
                             .fields.begin(),
                         compiled_warning_policy_a.front()
                             .warning_clauses.front()
                             .fields.end(),
                         [](const auto &field) {
                           return field.key == "CURSOR_EPOCHS_ABOVE" &&
                                  field.value == "2.0";
                         }) != compiled_warning_policy_a.front()
                                   .warning_clauses.front()
                                   .fields.end() &&
            std::find_if(compiled_warning_policy_b.front()
                             .warning_clauses.front()
                             .fields.begin(),
                         compiled_warning_policy_b.front()
                             .warning_clauses.front()
                             .fields.end(),
                         [](const auto &field) {
                           return field.key == "CURSOR_EPOCHS_ABOVE" &&
                                  field.value == "5.0";
                         }) != compiled_warning_policy_b.front()
                                   .warning_clauses.front()
                                   .fields.end(),
        "warning policy should stay available to explanation and evaluation");
  check(compiled_warning_policy_a.front().target_spec_fingerprint ==
            compiled_warning_policy_b.front().target_spec_fingerprint,
        "non-blocking warning threshold changes should not alter target "
        "identity");
  check(
      compiled_warning_policy_a.front().target_spec_fingerprint ==
          compiled_warning_policy_c.front().target_spec_fingerprint,
      "adding or removing non-blocking warning clauses should not alter target "
      "identity");

  const auto mixed_alias_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mixed_alias_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SUBJECT_COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  PLAN_MODE = train|debug;
  WAVE_MODE = train|debug;
  MAX_WAVES = 3;
  PLAN_MAX_ATTEMPTS = 3;
};
)DSL");
  check(!target::validate_lattice_target_specs(mixed_alias_specs).ok(),
        "mixing old and new alias names should produce validation warnings");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_fraction_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  MIN_VALID_TARGET_FRACTION = 1.2;
};
)DSL",
                      "MIN_VALID_TARGET_FRACTION must be a fraction",
                      "flat fraction thresholds should be unit-checked");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_cursor_epoch_requirement;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_REQUIRES {
  TARGET_ID = bad_cursor_epoch_requirement;
  KIND = exposure_coverage;
  USE = observed_input;
  SPLIT = train_core;
  EFFECT = mutated_component;
  CURSOR_EPOCHS = 1.2;
};
)DSL",
                      "exposure_coverage CURSOR_EPOCHS must be a fraction",
                      "coverage cursor-epoch requirements should be "
                      "unit-checked as fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_requires_kind;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_REQUIRES {
  TARGET_ID = bad_requires_kind;
  KIND = imaginary_requirement;
};
)DSL",
                      "unsupported LATTICE_REQUIRES KIND",
                      "unknown requirement clause kinds should fail fast");
  expect_decode_error(R"DSL(
LATTICE_GUARD {
  GUARD_ID = bad_guard_kind;
  KIND = imaginary_overlap;
  SPLIT = validation_holdout;
  USES = observed_input;
};

LATTICE_TARGET {
  TARGET_ID = bad_guard_kind_target;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  GUARD = bad_guard_kind;
};
)DSL",
                      "LATTICE_GUARD KIND must be exposure_overlap",
                      "unknown guard clause kinds should fail fast");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_gini_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_gini_warning;
  KIND = node_support_balance;
  METRIC = valid_target_count_gini;
  ABOVE = 1.2;
};
)DSL",
                      "ABOVE must be a fraction",
                      "Gini warning thresholds should be unit-checked as "
                      "fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_entropy_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_entropy_warning;
  KIND = node_support_balance;
  METRIC = valid_target_count_normalized_entropy;
  BELOW = 1.2;
};
)DSL",
                      "BELOW must be a fraction",
                      "normalized entropy warning thresholds should be "
                      "unit-checked as fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_count_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_count_warning;
  KIND = node_support_floor;
  METRIC = weakest_valid_target_count;
  BELOW = 1.5;
};
)DSL",
                      "BELOW must be an integral count threshold",
                      "node count warning thresholds should be integral "
                      "counts");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_fraction_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_fraction_warning;
  KIND = node_support_floor;
  METRIC = min_valid_target_fraction;
  BELOW = 1.2;
};
)DSL",
                      "BELOW must be a fraction",
                      "node-support fraction warning thresholds should be "
                      "unit-checked as fractions");
  expect_decode_error(
      R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_wilson_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_wilson_warning;
  KIND = node_support_floor;
  METRIC = valid_target_wilson_lower_95;
  BELOW = 1.2;
};
)DSL",
      "BELOW must be a fraction",
      "node-support Wilson lower-bound warning thresholds should "
      "be unit-checked as fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_boolean_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_boolean_warning;
  KIND = representation_health;
  METRIC = finite_parameter_check;
  BELOW = 2;
};
)DSL",
                      "BELOW must be a fraction",
                      "boolean representation-health thresholds should be "
                      "unit-checked");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_count_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_count_warning;
  KIND = representation_health;
  METRIC = total_valid_projection_rows;
  ABOVE = 12.5;
};
)DSL",
                      "ABOVE must be an integral count threshold",
                      "count-valued representation-health thresholds should be "
                      "unit-checked as integral counts");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_missing_threshold_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_missing_threshold_warning;
  KIND = representation_health;
  METRIC = mean_variance_loss;
};
)DSL",
                      "representation_health requires exactly one of ABOVE or "
                      "BELOW",
                      "representation-health warnings should require exactly "
                      "one threshold direction");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_double_threshold_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_double_threshold_warning;
  KIND = representation_health;
  METRIC = mean_variance_loss;
  ABOVE = 0.25;
  BELOW = 0.10;
};
)DSL",
                      "representation_health requires exactly one of ABOVE or "
                      "BELOW",
                      "representation-health warnings should reject competing "
                      "threshold directions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_unknown_metric_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_unknown_metric_warning;
  KIND = representation_health;
  METRIC = imaginary_geometry_score;
  ABOVE = 0.25;
};
)DSL",
                      "representation_health METRIC is unsupported",
                      "representation-health warnings should reject unknown "
                      "metric names");
  expect_decode_error(
      R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_condition_number_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_condition_number_warning;
  KIND = representation_health;
  METRIC = representation_condition_number;
  ABOVE = 0.5;
};
)DSL",
      "ABOVE must be a condition_number value >= 1",
      "representation condition-number warning thresholds should "
      "respect the mathematical lower bound");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_condition_number_direction;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_condition_number_direction;
  KIND = representation_health;
  METRIC = representation_condition_number;
  BELOW = 100;
};
)DSL",
                      "representation_condition_number uses ABOVE",
                      "representation condition-number warnings should reject "
                      "inverted threshold direction");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_effective_rank_direction;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_representation_effective_rank_direction;
  KIND = representation_health;
  METRIC = representation_effective_rank_fraction;
  ABOVE = 0.5;
};
)DSL",
                      "representation_effective_rank_fraction uses BELOW",
                      "representation effective-rank warnings should reject "
                      "inverted threshold direction");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_fraction_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_anchor_fraction_warning;
  KIND = anchor_domain_health;
  ACCEPTED_FRACTION_BELOW = 1.2;
};
)DSL",
                      "ACCEPTED_FRACTION_BELOW must be between 0 and 1",
                      "anchor-domain accepted-fraction thresholds should be "
                      "unit-checked as fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_warning_half_range;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_anchor_warning_half_range;
  KIND = anchor_domain_health;
  ANCHOR_INDEX_BEGIN = 20;
  ACCEPTED_FRACTION_BELOW = 0.8;
};
)DSL",
                      "explicit range requires both ANCHOR_INDEX_BEGIN and "
                      "ANCHOR_INDEX_END",
                      "all warning kinds should reject half-specified "
                      "explicit anchor ranges");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_node_warning_range_order;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_node_warning_range_order;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 30;
  ANCHOR_INDEX_END = 20;
  METRIC = weakest_valid_target_count;
  BELOW = 1;
};
)DSL",
                      "ANCHOR_INDEX_END must be greater than BEGIN",
                      "all warning kinds should reject non-positive explicit "
                      "anchor ranges");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_failed_fetch_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_anchor_failed_fetch_warning;
  KIND = anchor_domain_health;
  SKIPPED_FAILED_FETCH_PROBE_ABOVE = -1;
};
)DSL",
                      "SKIPPED_FAILED_FETCH_PROBE_ABOVE must be non-negative",
                      "anchor-domain failed-fetch thresholds should be "
                      "unit-checked as non-negative counts");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_domain_empty_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_anchor_domain_empty_warning;
  KIND = anchor_domain_health;
};
)DSL",
                      "anchor_domain_health requires ACCEPTED_FRACTION_BELOW "
                      "or SKIPPED_FAILED_FETCH_PROBE_ABOVE",
                      "anchor-domain health warnings should require at least "
                      "one measurable threshold");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_anchor_domain_double_warning;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_anchor_domain_double_warning;
  KIND = anchor_domain_health;
  ACCEPTED_FRACTION_BELOW = 0.8;
  SKIPPED_FAILED_FETCH_PROBE_ABOVE = 0;
};
)DSL",
                      "exactly one",
                      "anchor-domain health warnings should reject competing "
                      "metric thresholds in one clause");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_forbids_kind;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_FORBIDS {
  TARGET_ID = bad_forbids_kind;
  KIND = imaginary_overlap;
  SPLIT = validation_holdout;
  USES = observed_input;
};
)DSL",
                      "LATTICE_FORBIDS KIND must be exposure_overlap",
                      "unknown forbid clause kinds should fail fast");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_warning_kind;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_warning_kind;
  KIND = imaginary_warning;
  ABOVE = 1.0;
};
)DSL",
                      "unsupported LATTICE_WARN KIND",
                      "unknown warning clause kinds should fail fast");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_mdn_calibration_fraction_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_mdn_calibration_fraction_warning;
  KIND = mdn_distribution_calibration;
  METRIC = pit_ks_statistic;
  ABOVE = 1.2;
};
)DSL",
                      "ABOVE must be a fraction",
                      "MDN calibration fraction warning thresholds should be "
                      "unit-checked as fractions");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_mdn_calibration_direction_warning;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_mdn_calibration_direction_warning;
  KIND = mdn_distribution_calibration;
  METRIC = mean_nll;
  BELOW = 0.5;
};
)DSL",
                      "mdn_distribution_calibration uses ABOVE",
                      "MDN calibration warnings should reject inverted "
                      "threshold direction");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_mdn_calibration_unknown_metric;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_WARN {
  TARGET_ID = bad_mdn_calibration_unknown_metric;
  KIND = mdn_distribution_calibration;
  METRIC = imaginary_distribution_score;
  ABOVE = 0.5;
};
)DSL",
                      "mdn_distribution_calibration METRIC is unsupported",
                      "MDN calibration warnings should reject unknown metrics");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_plan_wave_target;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = bad_plan_wave_target;
  WAVE_TARGET = wikimyei.inference.expected_value.mdn;
};
)DSL",
                      "LATTICE_PLAN WAVE_TARGET must match",
                      "plan wave targets should remain attached to the "
                      "subject component");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_plan_wave_range;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = bad_plan_wave_range;
  WAVE_RANGE = train_core;
};
)DSL",
                      "LATTICE_PLAN WAVE_RANGE v0 must be split:<id>",
                      "plan wave ranges should use explicit split references");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_plan_attempt_budget;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = bad_plan_attempt_budget;
  PLAN_MAX_ATTEMPTS = -1;
};
)DSL",
                      "LATTICE_PLAN PLAN_MAX_ATTEMPTS must be non-negative",
                      "plan attempt budgets should be non-negative");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = known_plan_target;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = unknown_plan_target;
  PLAN_MODE = train|debug;
};
)DSL",
                      "LATTICE_PLAN references unknown TARGET_ID",
                      "extended plan clauses should not bind to unknown "
                      "targets");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_plan_input_conflict;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:first_checkpoint_source;
};

LATTICE_PLAN {
  TARGET_ID = bad_plan_input_conflict;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:second_checkpoint_source;
};
)DSL",
                      "conflicting PLAN_INPUT_MDN_CHECKPOINT values",
                      "model-state plan input hints should be internally "
                      "consistent");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_representation_plan_input_conflict;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:first_rep_source;
};

LATTICE_PLAN {
  TARGET_ID = bad_representation_plan_input_conflict;
  PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:second_rep_source;
};
)DSL",
                      "conflicting PLAN_INPUT_REPRESENTATION_CHECKPOINT "
                      "values",
                      "representation model-state plan input hints should be "
                      "internally consistent");
  expect_decode_error(R"DSL(
LATTICE_TARGET {
  TARGET_ID = bad_plan_split_conflict;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = train_core;
};

LATTICE_PLAN {
  TARGET_ID = bad_plan_split_conflict;
  WAVE_RANGE = split:validation_holdout;
};
)DSL",
                      "conflicting OVER_SPLIT values",
                      "plan wave ranges should agree with the target split");
  const auto split_policy =
      split::decode_lattice_split_policy_from_dsl(split_policy_dsl());
  const auto split_policy_fingerprint =
      split::lattice_split_policy_fingerprint(split_policy);
  check(split_policy.find_split("train_core") != nullptr,
        "split policy should decode train_core");
  check(split_policy.find_split("validation_holdout") != nullptr &&
            split_policy.find_split("validation_holdout")
                    ->protect_from_uses.size() == 3,
        "split policy should decode default holdout protection uses");
  check(!split_policy_fingerprint.empty(),
        "split policy fingerprint should be present");
  const auto split_backed_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = split_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  check(split_backed_specs.front().train_split == "train_core",
        "target should decode TRAIN_SPLIT");
  check(split_backed_specs.front().forbid_split == "validation_holdout",
        "target should decode PROTECT_SPLIT as forbidden split");
  check(split_backed_specs.front().forbid_exposure_uses.empty() &&
            split_backed_specs.front().forbid_exposure_uses_from_split_policy,
        "PROTECT_SPLIT should defer forbidden uses to the split policy");

  const auto root = make_tmp_dir("main");
  const auto targets_path = root / "targets.dsl";
  const auto targets_bnf_path = root / "targets.dsl.bnf";
  const auto splits_path = root / "splits.dsl";
  const auto splits_bnf_path = root / "splits.dsl.bnf";
  const auto config_path = root / ".config";
  write_text(targets_path, target_dsl());
  write_text(targets_bnf_path, "<instruction> ::= {<target_block>} ;\n");
  write_text(splits_path, split_policy_dsl());
  write_text(splits_bnf_path, "<instruction> ::= <policy_block> ;\n");
  write_text(config_path,
             std::string("kikijyeba_lattice_splits_dsl_bnf_path = ") +
                 splits_bnf_path.string() +
                 "\n"
                 "kikijyeba_lattice_splits_dsl_path = " +
                 splits_path.string() +
                 "\n"
                 "kikijyeba_lattice_targets_dsl_bnf_path = " +
                 targets_bnf_path.string() +
                 "\n"
                 "kikijyeba_lattice_targets_dsl_path = " +
                 targets_path.string() + "\n");
  const auto loaded_specs =
      target::load_lattice_targets_from_config(config_path);
  check(loaded_specs.size() == specs.size(),
        "load_lattice_targets_from_config should load target file");
  const auto loaded_split_policy =
      target::load_lattice_split_policy_from_config_if_available(config_path);
  check(loaded_split_policy.has_value() &&
            loaded_split_policy->find_split("validation_holdout") != nullptr,
        "load_lattice_split_policy_from_config_if_available should load split "
        "file");

  const auto production_config = production_config_path();
  const auto production_specs =
      target::load_lattice_targets_from_config(production_config);
  const auto production_validation =
      target::validate_lattice_target_specs(production_specs);
  check(production_validation.ok(),
        "production kikijyeba.lattice.targets.dsl should validate cleanly");
  const auto production_split_policy =
      target::load_lattice_split_policy_from_config_if_available(
          production_config);
  check(production_split_policy.has_value() &&
            production_split_policy->find_split("train_core") != nullptr &&
            production_split_policy->find_split("validation_holdout") !=
                nullptr &&
            production_split_policy->find_split("test_holdout") != nullptr,
        "production config should expose train, validation, and test lattice "
        "splits");
  const auto find_production_target = [&](const std::string &target_id)
      -> const target::lattice_target_spec_t * {
    const auto it = std::find_if(
        production_specs.begin(), production_specs.end(),
        [&](const auto &spec) { return spec.target_id == target_id; });
    return it == production_specs.end() ? nullptr : &*it;
  };
  const auto *production_channel_train =
      find_production_target("channel_mdn_train_core_ready");
  const auto *production_channel_no_validation =
      find_production_target("channel_mdn_train_core_no_validation_leakage");
  const auto *production_channel_no_test =
      find_production_target("channel_mdn_train_core_no_test_leakage");
  const auto *production_channel_eval =
      find_production_target("channel_mdn_validation_eval_ready");
  check(production_channel_train != nullptr &&
            production_channel_train->kind ==
                target::lattice_target_kind_t::channel_mdn_ready &&
            production_channel_train->component ==
                "wikimyei.inference.expected_value.mdn" &&
            production_channel_train->upstream_target_id ==
                "vicreg_train_core_ready" &&
            production_channel_train->train_split == "train_core" &&
            production_channel_train->plan_input_representation_checkpoint ==
                "latest_satisfying:vicreg_train_core_ready",
        "production channel MDN train target should consume channel VICReg "
        "checkpoint input over train_core");
  check(production_channel_no_validation != nullptr &&
            production_channel_no_validation->checkpoint_source ==
                "latest_satisfying:channel_mdn_train_core_ready" &&
            production_channel_no_validation->forbid_split ==
                "validation_holdout" &&
            production_channel_no_validation->max_waves == 0,
        "production channel MDN validation leakage guard should protect the "
        "validation holdout");
  check(production_channel_no_test != nullptr &&
            production_channel_no_test->checkpoint_source ==
                "latest_satisfying:"
                "channel_mdn_train_core_no_validation_leakage" &&
            production_channel_no_test->forbid_split == "test_holdout" &&
            production_channel_no_test->max_waves == 0,
        "production channel MDN test leakage guard should sit after the "
        "validation leakage guard");
  check(production_channel_eval != nullptr &&
            production_channel_eval->kind ==
                target::lattice_target_kind_t::channel_mdn_ready &&
            production_channel_eval->target_class == "evaluation_readiness" &&
            production_channel_eval->train_split == "validation_holdout" &&
            production_channel_eval->upstream_target_id ==
                "channel_mdn_train_core_no_test_leakage" &&
            production_channel_eval->evaluated_checkpoint_source ==
                "latest_satisfying:channel_mdn_train_core_no_test_leakage" &&
            production_channel_eval->plan_mode == "run|debug" &&
            production_channel_eval->plan_input_mdn_checkpoint ==
                "latest_satisfying:channel_mdn_train_core_no_test_leakage" &&
            production_channel_eval->plan_input_representation_checkpoint ==
                "latest_satisfying:vicreg_train_core_ready" &&
            production_channel_eval->max_waves == 1 &&
            std::abs(production_channel_eval->min_evaluation_metric_coverage -
                     0.95) < 1e-12,
        "production channel MDN validation evaluation target should be a "
        "run-mode strict-channel eval over validation_holdout with exact model "
        "state inputs");

  run_acceptance_fixture_regression();
  run_channel_target_kind_regression();

  const auto priority_vocabulary =
      target::lattice_deficit_priority_vocabulary();
  const auto deficit_vector_planning_summary =
      target::lattice_deficit_vector_planning_summary();
  const auto model_state_priority_entry = std::find_if(
      priority_vocabulary.begin(), priority_vocabulary.end(),
      [](const auto &entry) { return entry.priority_class == "model_state"; });
  const auto certificate_priority_entry = std::find_if(
      priority_vocabulary.begin(), priority_vocabulary.end(),
      [](const auto &entry) { return entry.priority_class == "certificate"; });
  const auto metric_priority_entry = std::find_if(
      priority_vocabulary.begin(), priority_vocabulary.end(),
      [](const auto &entry) { return entry.priority_class == "metric"; });
  const auto artifact_priority_entry = std::find_if(
      priority_vocabulary.begin(), priority_vocabulary.end(),
      [](const auto &entry) { return entry.priority_class == "artifact"; });
  check(priority_vocabulary.size() == 12 &&
            priority_vocabulary.front().priority_class == "identity" &&
            priority_vocabulary.front().priority == 10 &&
            certificate_priority_entry != priority_vocabulary.end() &&
            certificate_priority_entry->priority == 15 &&
            model_state_priority_entry != priority_vocabulary.end() &&
            model_state_priority_entry->priority == 25 &&
            artifact_priority_entry != priority_vocabulary.end() &&
            artifact_priority_entry->priority == 35 &&
            metric_priority_entry != priority_vocabulary.end() &&
            metric_priority_entry->priority == 70 &&
            priority_vocabulary.back().priority_class == "fallback" &&
            priority_vocabulary.back().priority == 1000,
        "deficit priority vocabulary should expose the deterministic planning "
        "order");
  check(
      deficit_vector_planning_summary.schema ==
              "kikijyeba.lattice.deficit_vector_planning.summary.v1" &&
          deficit_vector_planning_summary.deficit_priority_class_count == 12 &&
          deficit_vector_planning_summary.plan_advice_policy_count == 5 &&
          deficit_vector_planning_summary.plan_advice_only_count == 4 &&
          deficit_vector_planning_summary.evidence_authority_count == 0 &&
          deficit_vector_planning_summary.scheduler_authority_count == 1 &&
          deficit_vector_planning_summary.lattice_executor_count == 0 &&
          deficit_vector_planning_summary.affects_contract_identity_count ==
              0 &&
          deficit_vector_planning_summary.attempt_budget_guard_count == 1 &&
          deficit_vector_planning_summary.duplicate_priority_count == 0 &&
          deficit_vector_planning_summary.empty_policy_field_count == 0 &&
          deficit_vector_planning_summary.lower_priority_is_primary &&
          deficit_vector_planning_summary.priority_order_strictly_increasing &&
          deficit_vector_planning_summary.fallback_priority_is_terminal &&
          deficit_vector_planning_summary
              .checkpoint_binding_outranks_dependency &&
          deficit_vector_planning_summary.identity_outranks_certificate &&
          deficit_vector_planning_summary.leakage_outranks_coverage &&
          deficit_vector_planning_summary.metric_outranks_status &&
          deficit_vector_planning_summary.plan_basis_advice_present &&
          deficit_vector_planning_summary.suggested_wave_advice_present &&
          deficit_vector_planning_summary.plan_input_model_state_hint_present &&
          deficit_vector_planning_summary.attempt_budget_guard_present &&
          deficit_vector_planning_summary
              .runtime_hero_executor_boundary_present &&
          deficit_vector_planning_summary
              .plan_surfaces_are_advice_not_evidence &&
          deficit_vector_planning_summary.runtime_hero_is_only_executor &&
          deficit_vector_planning_summary
              .plan_advice_does_not_affect_contract_identity &&
          deficit_vector_planning_summary.all_policies_have_boundary_text &&
          deficit_vector_planning_summary.summary_self_check_passed &&
          deficit_vector_planning_summary.summary_issue_count == 0 &&
          deficit_vector_planning_summary.summary_issues.empty() &&
          std::find(deficit_vector_planning_summary.priority_classes.begin(),
                    deficit_vector_planning_summary.priority_classes.end(),
                    "checkpoint_binding") !=
              deficit_vector_planning_summary.priority_classes.end() &&
          std::find(
              deficit_vector_planning_summary.plan_advice_surfaces.begin(),
              deficit_vector_planning_summary.plan_advice_surfaces.end(),
              "suggested_wave") !=
              deficit_vector_planning_summary.plan_advice_surfaces.end() &&
          std::find(
              deficit_vector_planning_summary.plan_advice_surfaces.begin(),
              deficit_vector_planning_summary.plan_advice_surfaces.end(),
              "hero.runtime.execute") !=
              deficit_vector_planning_summary.plan_advice_surfaces.end(),
      "deficit vector planning summary should prove deterministic deficit "
      "priority, advisory planning, and Runtime Hero executor boundary");
  const auto [checkpoint_priority, checkpoint_class] =
      target::lattice_deficit_priority_for("dependency", "mdn_checkpoint");
  check(checkpoint_priority == 20 && checkpoint_class == "checkpoint_binding",
        "checkpoint-binding dependency deficits should outrank generic "
        "dependency deficits");
  const auto [certificate_priority, certificate_class] =
      target::lattice_deficit_priority_for("certificate",
                                           "proof_certificate_check");
  check(certificate_priority == 15 && certificate_class == "certificate",
        "certificate self-check deficits should expose their own planning "
        "priority class");
  const auto [model_state_priority, model_state_class] =
      target::lattice_deficit_priority_for("model_state", "output_checkpoint");
  check(model_state_priority == 25 && model_state_class == "model_state",
        "model-state deficits should expose their own planning priority class");
  const auto [model_state_input_priority, model_state_input_class] =
      target::lattice_deficit_priority_for("model_state", "input_checkpoint");
  check(model_state_input_priority == 25 &&
            model_state_input_class == "model_state",
        "model-state input deficits should share the model-state planning "
        "priority class");
  const auto [metric_priority, metric_class] =
      target::lattice_deficit_priority_for("metric", "optimizer_steps");
  check(metric_priority == 70 && metric_class == "metric",
        "metric deficits should expose their own planning priority class");
  const auto [artifact_priority, artifact_class] =
      target::lattice_deficit_priority_for("artifact", "checkpoint");
  check(artifact_priority == 35 && artifact_class == "artifact",
        "artifact deficits should expose their own planning priority class");
  const auto warning_relation_vocabulary =
      target::lattice_warning_threshold_relation_vocabulary();
  check(warning_relation_vocabulary.size() == 7 &&
            warning_relation_vocabulary.front().relation == "above_threshold" &&
            warning_relation_vocabulary.front().measurement_available &&
            warning_relation_vocabulary.front().triggered &&
            warning_relation_vocabulary.back().relation ==
                "unknown_direction" &&
            !warning_relation_vocabulary.back().measurement_available &&
            !warning_relation_vocabulary.back().triggered,
        "warning threshold relation vocabulary should expose trigger and "
        "availability semantics");
  const auto warning_anchor_scope_policy =
      target::lattice_warning_anchor_scope_policy_vocabulary();
  const auto anchor_fallback_policy =
      std::find_if(warning_anchor_scope_policy.begin(),
                   warning_anchor_scope_policy.end(), [](const auto &entry) {
                     return entry.policy == "anchor_visibility_fallback";
                   });
  const auto node_matrix_policy =
      std::find_if(warning_anchor_scope_policy.begin(),
                   warning_anchor_scope_policy.end(), [](const auto &entry) {
                     return entry.policy == "node_support_warning_matrix";
                   });
  check(
      warning_anchor_scope_policy.size() == 3 &&
          warning_anchor_scope_policy.front().policy ==
              "resolved_warning_anchor_range" &&
          warning_anchor_scope_policy.front().surface.find(
              "warning_results[].warning_anchor_range") != std::string::npos &&
          warning_anchor_scope_policy.front().surface.find(
              "explain_target.warning_scope_previews[]."
              "resolved_warning_anchor_range") != std::string::npos &&
          warning_anchor_scope_policy.front().visibility_only &&
          !warning_anchor_scope_policy.front().can_satisfy_readiness &&
          !warning_anchor_scope_policy.front()
               .contributes_to_certificate_digest &&
          anchor_fallback_policy != warning_anchor_scope_policy.end() &&
          anchor_fallback_policy->readiness_effect.find("cannot contribute") !=
              std::string::npos &&
          node_matrix_policy != warning_anchor_scope_policy.end() &&
          node_matrix_policy->readiness_effect.find("MDN-head support") !=
              std::string::npos,
      "warning anchor-scope policy should expose visibility-only interval "
      "and fallback semantics");
  const auto evidence_order_relation_vocabulary =
      target::lattice_evidence_order_relation_vocabulary();
  check(evidence_order_relation_vocabulary.size() == 6 &&
            evidence_order_relation_vocabulary.front() ==
                target::lattice_evidence_order_relation_t::unavailable &&
            std::find(
                evidence_order_relation_vocabulary.begin(),
                evidence_order_relation_vocabulary.end(),
                target::lattice_evidence_order_relation_t::selector_leaked) !=
                evidence_order_relation_vocabulary.end() &&
            evidence_order_relation_vocabulary.back() ==
                target::lattice_evidence_order_relation_t::incomparable,
        "evidence order relation vocabulary should expose Pareto comparison "
        "states");
  const auto evidence_dimension_vocabulary =
      target::lattice_evidence_order_dimension_vocabulary();
  const auto find_dimension = [&](const std::string &dimension) {
    return std::find_if(
        evidence_dimension_vocabulary.begin(),
        evidence_dimension_vocabulary.end(),
        [&](const auto &entry) { return entry.dimension == dimension; });
  };
  const auto triggered_warning_dimension =
      find_dimension("triggered_warning_count");
  const auto warning_count_dimension = find_dimension("warning_count");
  const auto source_key_visibility_dimension =
      find_dimension("source_key_audit_count");
  const auto selector_leakage_dimension =
      find_dimension("selection_signal_leakage_found");
  const auto node_support_cv_dimension =
      find_dimension("max_node_support_coefficient_of_variation");
  const auto node_support_gini_dimension =
      find_dimension("max_node_support_gini");
  const auto unresolved_lineage_dimension =
      find_dimension("unresolved_lineage_count");
  check(
      evidence_dimension_vocabulary.size() == 26 &&
          triggered_warning_dimension != evidence_dimension_vocabulary.end() &&
          triggered_warning_dimension->polarity == "lower_is_stronger" &&
          triggered_warning_dimension->comparison_dimension &&
          warning_count_dimension != evidence_dimension_vocabulary.end() &&
          warning_count_dimension->polarity ==
              "alias_for_triggered_warning_count" &&
          !warning_count_dimension->comparison_dimension &&
          source_key_visibility_dimension !=
              evidence_dimension_vocabulary.end() &&
          source_key_visibility_dimension->polarity == "visibility_only" &&
          !source_key_visibility_dimension->comparison_dimension &&
          selector_leakage_dimension != evidence_dimension_vocabulary.end() &&
          selector_leakage_dimension->polarity ==
              "false_required_for_participation" &&
          selector_leakage_dimension->comparison_dimension &&
          node_support_cv_dimension != evidence_dimension_vocabulary.end() &&
          node_support_cv_dimension->polarity == "lower_is_stronger" &&
          node_support_cv_dimension->comparison_dimension &&
          node_support_gini_dimension != evidence_dimension_vocabulary.end() &&
          node_support_gini_dimension->polarity == "lower_is_stronger" &&
          node_support_gini_dimension->comparison_dimension &&
          unresolved_lineage_dimension != evidence_dimension_vocabulary.end() &&
          unresolved_lineage_dimension->polarity == "lower_is_stronger" &&
          unresolved_lineage_dimension->comparison_dimension,
      "evidence order dimension vocabulary should expose dimension polarity "
      "and comparison membership");
  const auto exposure_algebra_vocabulary =
      exposure::lattice_exposure_measure_algebra_vocabulary();
  const auto exposure_algebra_summary =
      exposure::lattice_exposure_measure_algebra_summary();
  check(
      exposure_algebra_vocabulary.size() == 2 &&
          exposure_algebra_vocabulary[0].measure == "unique_coverage" &&
          exposure_algebra_vocabulary[0].algebra ==
              exposure::k_unique_coverage_algebra &&
          exposure_algebra_vocabulary[0].operation == "interval_union" &&
          exposure_algebra_vocabulary[0].unit ==
              exposure::k_unique_coverage_unit &&
          exposure_algebra_vocabulary[0].idempotent &&
          !exposure_algebra_vocabulary[0].additive &&
          exposure_algebra_vocabulary[1].measure == "exposure_load" &&
          exposure_algebra_vocabulary[1].algebra == exposure::k_load_algebra &&
          exposure_algebra_vocabulary[1].operation == "interval_measure_sum" &&
          exposure_algebra_vocabulary[1].unit == exposure::k_load_unit &&
          !exposure_algebra_vocabulary[1].idempotent &&
          exposure_algebra_vocabulary[1].additive,
      "exposure algebra vocabulary should distinguish idempotent coverage "
      "from additive load");
  check(exposure_algebra_summary.schema ==
                "kikijyeba.lattice.exposure_measure_algebra.summary.v1" &&
            exposure_algebra_summary.measure_count == 2 &&
            exposure_algebra_summary.idempotent_measure_count == 1 &&
            exposure_algebra_summary.additive_measure_count == 1 &&
            exposure_algebra_summary.coverage_measure_count == 1 &&
            exposure_algebra_summary.load_measure_count == 1 &&
            exposure_algebra_summary.dual_idempotent_additive_count == 0 &&
            exposure_algebra_summary.empty_field_count == 0 &&
            exposure_algebra_summary.unique_coverage_is_idempotent_union &&
            exposure_algebra_summary.exposure_load_is_additive_measure &&
            exposure_algebra_summary.coverage_not_additive &&
            exposure_algebra_summary.load_not_idempotent &&
            exposure_algebra_summary.measure_units_declared &&
            exposure_algebra_summary.coverage_and_load_units_distinct &&
            exposure_algebra_summary.summary_self_check_passed &&
            exposure_algebra_summary.summary_issue_count == 0 &&
            exposure_algebra_summary.summary_issues.empty(),
        "exposure algebra summary should self-check coverage/load algebra "
        "partition and units");
  const auto source_key_policy_vocabulary =
      exposure::lattice_source_key_coordinate_policy_vocabulary();
  const auto find_source_key_policy = [&](const std::string &policy) {
    return std::find_if(
        source_key_policy_vocabulary.begin(),
        source_key_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto row_index_policy =
      find_source_key_policy("row_index_interval_authority");
  const auto source_key_window_policy =
      find_source_key_policy("source_key_window_audit");
  const auto order_preserving_policy =
      find_source_key_policy("order_preserving_map_check");
  const auto affine_policy =
      find_source_key_policy("affine_step_consistency_check");
  check(source_key_policy_vocabulary.size() == 4 &&
            row_index_policy != source_key_policy_vocabulary.end() &&
            row_index_policy->coverage_authority &&
            row_index_policy->leakage_authority &&
            !row_index_policy->audit_only &&
            source_key_window_policy != source_key_policy_vocabulary.end() &&
            !source_key_window_policy->coverage_authority &&
            !source_key_window_policy->leakage_authority &&
            source_key_window_policy->audit_only &&
            order_preserving_policy != source_key_policy_vocabulary.end() &&
            std::find(order_preserving_policy->audit_fields.begin(),
                      order_preserving_policy->audit_fields.end(),
                      "order_preserving") !=
                order_preserving_policy->audit_fields.end() &&
            affine_policy != source_key_policy_vocabulary.end() &&
            std::find(affine_policy->audit_fields.begin(),
                      affine_policy->audit_fields.end(),
                      "affine_consistent") != affine_policy->audit_fields.end(),
        "source-key coordinate policy vocabulary should keep row-index "
        "intervals authoritative and source-key maps audit-only");
  const auto source_key_policy_summary =
      exposure::lattice_source_key_coordinate_policy_summary();
  check(source_key_policy_summary.schema ==
                "kikijyeba.lattice.source_key_coordinate_policy.summary.v1" &&
            source_key_policy_summary.policy_count == 4 &&
            source_key_policy_summary.coverage_authority_count == 1 &&
            source_key_policy_summary.leakage_authority_count == 1 &&
            source_key_policy_summary.audit_only_count == 3 &&
            source_key_policy_summary.row_index_coordinate_count == 1 &&
            source_key_policy_summary.source_key_coordinate_count == 1 &&
            source_key_policy_summary.row_to_source_key_map_count == 2 &&
            source_key_policy_summary.empty_field_count == 0 &&
            source_key_policy_summary.row_index_authority_present &&
            source_key_policy_summary.source_key_window_audit_present &&
            source_key_policy_summary.order_preserving_map_check_present &&
            source_key_policy_summary.affine_step_consistency_check_present &&
            source_key_policy_summary
                .row_index_is_coverage_and_leakage_authority &&
            source_key_policy_summary.source_key_rows_are_audit_only &&
            source_key_policy_summary
                .audit_rows_have_no_coverage_or_leakage_authority &&
            source_key_policy_summary.order_preserving_fields_declared &&
            source_key_policy_summary.affine_fields_declared &&
            source_key_policy_summary.all_policies_have_claim_text &&
            source_key_policy_summary.summary_self_check_passed &&
            source_key_policy_summary.summary_issue_count == 0 &&
            source_key_policy_summary.summary_issues.empty() &&
            std::find(source_key_policy_summary.policy_names.begin(),
                      source_key_policy_summary.policy_names.end(),
                      "row_index_interval_authority") !=
                source_key_policy_summary.policy_names.end(),
        "source-key coordinate policy summary should self-check row-index "
        "authority and audit-only source-key map rows");
  const auto source_receipt_policy_vocabulary =
      exposure::lattice_source_receipt_policy_vocabulary();
  const auto find_source_receipt_policy = [&](const std::string &policy) {
    return std::find_if(
        source_receipt_policy_vocabulary.begin(),
        source_receipt_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto compact_receipt_policy =
      find_source_receipt_policy("compact_source_file_receipts");
  const auto row_index_overlap_policy =
      find_source_receipt_policy("row_index_overlap_authority");
  const auto structured_receipt_policy =
      find_source_receipt_policy("structured_source_receipt_fact");
  const auto receipt_identity_policy =
      find_source_receipt_policy("receipt_identity_boundary");
  check(
      source_receipt_policy_vocabulary.size() == 4 &&
          compact_receipt_policy != source_receipt_policy_vocabulary.end() &&
          compact_receipt_policy->audit_only &&
          !compact_receipt_policy->coverage_authority &&
          !compact_receipt_policy->leakage_authority &&
          !compact_receipt_policy->contract_identity_authority &&
          row_index_overlap_policy != source_receipt_policy_vocabulary.end() &&
          row_index_overlap_policy->coverage_authority &&
          row_index_overlap_policy->leakage_authority &&
          !row_index_overlap_policy->audit_only &&
          structured_receipt_policy != source_receipt_policy_vocabulary.end() &&
          structured_receipt_policy->structured_fact_available &&
          structured_receipt_policy->audit_only &&
          receipt_identity_policy != source_receipt_policy_vocabulary.end() &&
          !receipt_identity_policy->contract_identity_authority,
      "source receipt policy vocabulary should keep compact receipts as "
      "audit metadata outside coverage, leakage, and contract identity");
  const auto source_receipt_policy_summary =
      exposure::lattice_source_receipt_policy_summary();
  check(source_receipt_policy_summary.schema ==
                "kikijyeba.lattice.source_receipt_policy.summary.v1" &&
            source_receipt_policy_summary.policy_count == 4 &&
            source_receipt_policy_summary.coverage_authority_count == 1 &&
            source_receipt_policy_summary.leakage_authority_count == 1 &&
            source_receipt_policy_summary.contract_identity_authority_count ==
                0 &&
            source_receipt_policy_summary.audit_only_count == 3 &&
            source_receipt_policy_summary.structured_fact_available_count ==
                1 &&
            source_receipt_policy_summary.future_policy_count == 0 &&
            source_receipt_policy_summary.empty_field_count == 0 &&
            source_receipt_policy_summary.compact_receipts_audit_only &&
            source_receipt_policy_summary.row_index_overlap_authority_present &&
            source_receipt_policy_summary.structured_receipt_available &&
            source_receipt_policy_summary.receipt_identity_non_contract &&
            source_receipt_policy_summary
                .only_row_index_has_coverage_leakage_authority &&
            source_receipt_policy_summary.no_contract_identity_authority &&
            source_receipt_policy_summary.structured_receipts_audit_only &&
            source_receipt_policy_summary
                .audit_rows_have_no_coverage_or_leakage_authority &&
            source_receipt_policy_summary.compact_receipt_fields_declared &&
            source_receipt_policy_summary.structured_receipt_fields_declared &&
            source_receipt_policy_summary.all_policies_have_claim_text &&
            source_receipt_policy_summary.summary_self_check_passed &&
            source_receipt_policy_summary.summary_issue_count == 0 &&
            source_receipt_policy_summary.summary_issues.empty() &&
            std::find(source_receipt_policy_summary.policy_names.begin(),
                      source_receipt_policy_summary.policy_names.end(),
                      "receipt_identity_boundary") !=
                source_receipt_policy_summary.policy_names.end(),
        "source receipt policy summary should self-check audit-only receipts, "
        "row-index overlap authority, and non-contract identity scope");
  const auto valid_target_uncertainty_vocabulary =
      exposure::lattice_valid_target_uncertainty_vocabulary();
  check(
      valid_target_uncertainty_vocabulary.size() == 3 &&
          valid_target_uncertainty_vocabulary.front().scope ==
              "exposure_load_summary.valid_target_support" &&
          valid_target_uncertainty_vocabulary.front().method ==
              "wilson_score_interval_95" &&
          valid_target_uncertainty_vocabulary.front().estimator ==
              "binomial_fraction" &&
          std::abs(
              valid_target_uncertainty_vocabulary.front().confidence_level -
              0.95) < 1e-12 &&
          valid_target_uncertainty_vocabulary.front().success_count_field ==
              "valid_target_success_count_for_uncertainty" &&
          valid_target_uncertainty_vocabulary.front().opportunity_count_field ==
              "valid_target_opportunity_count_for_uncertainty" &&
          valid_target_uncertainty_vocabulary.front().lower_bound_field ==
              "valid_target_wilson_lower_95" &&
          valid_target_uncertainty_vocabulary.back().scope ==
              "node_support_summary.weakest_node_valid_target_support" &&
          valid_target_uncertainty_vocabulary.back().success_count_field ==
              "weakest_valid_target_count" &&
          valid_target_uncertainty_vocabulary.back().opportunity_count_field ==
              "weakest_valid_target_denominator" &&
          valid_target_uncertainty_vocabulary.back().no_trials_policy ==
              "non_claiming_nan_interval",
      "valid-target uncertainty vocabulary should expose Wilson support "
      "fields and no-trials policy");
  const auto valid_target_uncertainty_summary =
      exposure::lattice_valid_target_uncertainty_summary();
  check(
      valid_target_uncertainty_summary.schema ==
              "kikijyeba.lattice.valid_target_uncertainty.summary.v1" &&
          valid_target_uncertainty_summary.scope_count == 3 &&
          valid_target_uncertainty_summary.wilson_method_count == 3 &&
          valid_target_uncertainty_summary.binomial_estimator_count == 3 &&
          valid_target_uncertainty_summary.confidence_95_count == 3 &&
          valid_target_uncertainty_summary.no_trials_non_claiming_count == 3 &&
          valid_target_uncertainty_summary.exposure_load_scope_count == 1 &&
          valid_target_uncertainty_summary.node_support_scope_count == 2 &&
          valid_target_uncertainty_summary.weakest_node_scope_count == 1 &&
          valid_target_uncertainty_summary.empty_field_count == 0 &&
          valid_target_uncertainty_summary
              .exposure_load_support_scope_present &&
          valid_target_uncertainty_summary
              .node_aggregate_support_scope_present &&
          valid_target_uncertainty_summary.weakest_node_support_scope_present &&
          valid_target_uncertainty_summary.all_scopes_use_wilson_95 &&
          valid_target_uncertainty_summary.all_estimators_binomial_fraction &&
          valid_target_uncertainty_summary.all_no_trials_non_claiming &&
          valid_target_uncertainty_summary.all_count_fields_declared &&
          valid_target_uncertainty_summary.all_fraction_fields_declared &&
          valid_target_uncertainty_summary.all_interval_bounds_declared &&
          valid_target_uncertainty_summary
              .support_visibility_not_performance_gate &&
          valid_target_uncertainty_summary.summary_self_check_passed &&
          valid_target_uncertainty_summary.summary_issue_count == 0 &&
          valid_target_uncertainty_summary.summary_issues.empty() &&
          std::find(valid_target_uncertainty_summary.scope_names.begin(),
                    valid_target_uncertainty_summary.scope_names.end(),
                    "node_support_summary.aggregate_valid_target_support") !=
              valid_target_uncertainty_summary.scope_names.end(),
      "valid-target uncertainty summary should self-check Wilson support "
      "scope bindings and non-claiming no-trials policy");
  const auto leakage_rule_vocabulary =
      target::lattice_leakage_rule_vocabulary();
  const auto leakage_rule_summary = target::lattice_leakage_rule_summary();
  check(leakage_rule_vocabulary.size() == 1 &&
            leakage_rule_vocabulary.front().rule ==
                target::k_lattice_leakage_rule &&
            leakage_rule_vocabulary.front().protected_set_algebra ==
                "temporal_morphological_dilation" &&
            leakage_rule_vocabulary.front().dilation_kernel ==
                "[-dilation_left,+dilation_right]" &&
            leakage_rule_vocabulary.front().predicate ==
                "closure_forbidden_use_intersection_empty" &&
            leakage_rule_vocabulary.front().witness_basis ==
                "labeled_causal_exposure_reachability" &&
            leakage_rule_vocabulary.front().requires_closure_complete &&
            leakage_rule_vocabulary.front().protected_range_contains_base,
        "leakage rule vocabulary should expose protected-set dilation "
        "semantics");
  check(
      leakage_rule_summary.schema ==
              "kikijyeba.lattice.leakage_rule.summary.v1" &&
          leakage_rule_summary.rule_count == 1 &&
          leakage_rule_summary.temporal_dilation_rule_count == 1 &&
          leakage_rule_summary.closure_complete_required_count == 1 &&
          leakage_rule_summary.protected_contains_base_count == 1 &&
          leakage_rule_summary.empty_field_count == 0 &&
          leakage_rule_summary.single_v1_rule &&
          leakage_rule_summary
              .protected_split_dilation_intersection_rule_present &&
          leakage_rule_summary.predicate_is_forbidden_use_empty_intersection &&
          leakage_rule_summary.witness_basis_is_labeled_causal_reachability &&
          leakage_rule_summary.closure_completeness_required &&
          leakage_rule_summary.protected_range_contains_base &&
          leakage_rule_summary.all_rules_have_policy_text &&
          leakage_rule_summary.summary_self_check_passed &&
          leakage_rule_summary.summary_issue_count == 0 &&
          leakage_rule_summary.summary_issues.empty() &&
          std::find(leakage_rule_summary.rule_names.begin(),
                    leakage_rule_summary.rule_names.end(),
                    target::k_lattice_leakage_rule) !=
              leakage_rule_summary.rule_names.end(),
      "leakage rule summary should self-check protected split dilation, "
      "complete closure, and causal overlap witnesses");
  const auto causal_edge_vocabulary = target::lattice_causal_edge_vocabulary();
  const auto causal_edge_summary = target::lattice_causal_edge_summary();
  const auto find_causal_edge = [&](const std::string &edge) {
    return std::find_if(causal_edge_vocabulary.begin(),
                        causal_edge_vocabulary.end(),
                        [&](const auto &entry) { return entry.edge == edge; });
  };
  const auto loaded_checkpoint_edge = find_causal_edge("loaded_checkpoint");
  const auto created_checkpoint_edge = find_causal_edge("created_checkpoint");
  const auto target_supervision_edge = find_causal_edge("target_supervision");
  const auto selection_signal_edge = find_causal_edge("selection_signal");
  check(causal_edge_vocabulary.size() == 7 &&
            loaded_checkpoint_edge != causal_edge_vocabulary.end() &&
            loaded_checkpoint_edge->source == "checkpoint" &&
            loaded_checkpoint_edge->target == "runtime_job" &&
            loaded_checkpoint_edge->proof_field ==
                "closure.causal_exposures[].input_checkpoints" &&
            loaded_checkpoint_edge->closure_reachability_edge &&
            !loaded_checkpoint_edge->leakage_relevant_when_forbidden &&
            created_checkpoint_edge != causal_edge_vocabulary.end() &&
            created_checkpoint_edge->source == "runtime_job" &&
            created_checkpoint_edge->target == "checkpoint" &&
            created_checkpoint_edge->proof_field ==
                "closure.causal_exposures[].output_checkpoint" &&
            created_checkpoint_edge->closure_reachability_edge &&
            target_supervision_edge != causal_edge_vocabulary.end() &&
            target_supervision_edge->source == "source_row_footprint" &&
            target_supervision_edge->target == "runtime_job" &&
            target_supervision_edge->proof_field ==
                "closure.causal_exposures[].target_footprint" &&
            target_supervision_edge->leakage_relevant_when_forbidden &&
            selection_signal_edge != causal_edge_vocabulary.end() &&
            selection_signal_edge->proof_field ==
                "closure.causal_exposures[].anchor_range" &&
            selection_signal_edge->leakage_relevant_when_forbidden,
        "causal edge vocabulary should expose checkpoint reachability and "
        "forbidden row-footprint edge labels");
  check(causal_edge_summary.schema ==
                "kikijyeba.lattice.causal_edge.summary.v1" &&
            causal_edge_summary.edge_count == 7 &&
            causal_edge_summary.source_row_edge_count == 4 &&
            causal_edge_summary.checkpoint_reachability_edge_count == 2 &&
            causal_edge_summary.leakage_relevant_edge_count == 5 &&
            causal_edge_summary.mutation_edge_count == 1 &&
            causal_edge_summary.checkpoint_edge_leakage_overlap_count == 0 &&
            causal_edge_summary.empty_field_count == 0 &&
            causal_edge_summary.source_row_use_edges_present &&
            causal_edge_summary.checkpoint_load_create_edges_present &&
            causal_edge_summary.mutation_edge_present &&
            causal_edge_summary
                .checkpoint_reachability_distinct_from_leakage_edges &&
            causal_edge_summary.selection_signal_is_leakage_relevant &&
            causal_edge_summary
                .evaluation_metric_is_leakage_relevant_when_forbidden &&
            causal_edge_summary.all_edges_have_proof_fields &&
            causal_edge_summary.summary_self_check_passed &&
            causal_edge_summary.summary_issue_count == 0 &&
            causal_edge_summary.missing_required_edges.empty() &&
            causal_edge_summary.summary_issues.empty() &&
            std::find(causal_edge_summary.edge_names.begin(),
                      causal_edge_summary.edge_names.end(),
                      "loaded_checkpoint") !=
                causal_edge_summary.edge_names.end() &&
            std::find(causal_edge_summary.edge_names.begin(),
                      causal_edge_summary.edge_names.end(),
                      "selection_signal") !=
                causal_edge_summary.edge_names.end(),
        "causal edge summary should self-check source-row, checkpoint "
        "reachability, mutation, and leakage-relevance edge families");
  const auto selection_signal_policy_vocabulary =
      target::lattice_selection_signal_policy_vocabulary();
  const auto find_selection_signal_policy = [&](const std::string &policy) {
    return std::find_if(
        selection_signal_policy_vocabulary.begin(),
        selection_signal_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto forbidden_selection_policy =
      find_selection_signal_policy("selection_signal_forbidden_use");
  const auto event_stream_policy =
      find_selection_signal_policy("selection_signal_event_stream_status");
  const auto selection_path_policy =
      find_selection_signal_policy("selection_signal_path_policy");
  check(selection_signal_policy_vocabulary.size() == 4 &&
            std::all_of(selection_signal_policy_vocabulary.begin(),
                        selection_signal_policy_vocabulary.end(),
                        [](const auto &entry) {
                          return entry.requires_closure_complete &&
                                 !entry.runtime_executor;
                        }) &&
            forbidden_selection_policy !=
                selection_signal_policy_vocabulary.end() &&
            forbidden_selection_policy->forbidden_use &&
            forbidden_selection_policy->status_effect == "exposure_failed" &&
            forbidden_selection_policy->source_footprint_basis ==
                "closure.causal_exposures[].anchor_range" &&
            event_stream_policy != selection_signal_policy_vocabulary.end() &&
            event_stream_policy->first_class_event_stream &&
            event_stream_policy->proof_field ==
                "scan_exposure.selection_signal_facts[]" &&
            selection_path_policy != selection_signal_policy_vocabulary.end() &&
            selection_path_policy->status_effect ==
                "known_selector_path_fail_closed_when_forbidden",
        "selection-signal policy vocabulary should expose first-class V2 "
        "selector events while keeping them read-only");
  const auto selection_signal_policy_summary =
      target::lattice_selection_signal_policy_summary();
  check(
      selection_signal_policy_summary.schema ==
              "kikijyeba.lattice.selection_signal_policy.summary.v1" &&
          selection_signal_policy_summary.policy_count == 4 &&
          selection_signal_policy_summary.forbidden_use_policy_count == 2 &&
          selection_signal_policy_summary.requires_closure_complete_count ==
              4 &&
          selection_signal_policy_summary.first_class_event_stream_count == 1 &&
          selection_signal_policy_summary.runtime_executor_count == 0 &&
          selection_signal_policy_summary.exposure_failed_policy_count == 1 &&
          selection_signal_policy_summary.future_policy_count == 1 &&
          selection_signal_policy_summary.empty_field_count == 0 &&
          selection_signal_policy_summary.forbidden_use_policy_present &&
          selection_signal_policy_summary.causal_edge_policy_present &&
          selection_signal_policy_summary.event_stream_available &&
          selection_signal_policy_summary.known_selector_path_fail_closed &&
          selection_signal_policy_summary.anchor_range_is_v1_source_footprint &&
          selection_signal_policy_summary
              .all_policies_require_closure_complete &&
          selection_signal_policy_summary
              .first_class_event_stream_is_read_only &&
          selection_signal_policy_summary.no_runtime_executor &&
          selection_signal_policy_summary.all_policies_have_claim_text &&
          selection_signal_policy_summary.summary_self_check_passed &&
          selection_signal_policy_summary.summary_issue_count == 0 &&
          selection_signal_policy_summary.summary_issues.empty() &&
          std::find(selection_signal_policy_summary.policy_names.begin(),
                    selection_signal_policy_summary.policy_names.end(),
                    "selection_signal_path_policy") !=
              selection_signal_policy_summary.policy_names.end(),
      "selection-signal policy summary should self-check V2 selector-event "
      "visibility and fail-closed forbidden-use behavior");
  const auto derived_rule_vocabulary =
      target::lattice_derived_query_rule_vocabulary();
  const auto find_derived_rule = [&](const std::string &relation) {
    return std::find_if(
        derived_rule_vocabulary.begin(), derived_rule_vocabulary.end(),
        [&](const auto &entry) { return entry.relation == relation; });
  };
  const auto relation_has_input = [](const auto &entry,
                                     const std::string &input_relation) {
    return std::find(entry->input_relations.begin(),
                     entry->input_relations.end(),
                     input_relation) != entry->input_relations.end();
  };
  const auto dependency_rule = find_derived_rule("dependency_satisfied");
  const auto all_dependencies_rule =
      find_derived_rule("all_dependencies_satisfied");
  const auto coverage_rule = find_derived_rule("coverage_satisfied");
  const auto all_coverage_rule = find_derived_rule("all_coverage_satisfied");
  const auto closure_clean_rule = find_derived_rule("closure_clean_if_checked");
  const auto checkpoint_ancestor_rule =
      find_derived_rule("checkpoint_ancestor");
  const auto unresolved_lineage_rule = find_derived_rule("unresolved_lineage");
  const auto leakage_rule = find_derived_rule("forbidden_overlap");
  const auto no_forbidden_rule = find_derived_rule("no_forbidden_overlap");
  const auto stale_cache_rule = find_derived_rule("stale_cache");
  const auto plan_rule = find_derived_rule("plan_deficit");
  const auto certificate_check_rule =
      find_derived_rule("proof_certificate_check_passed");
  const auto no_blocking_rule = find_derived_rule("no_blocking_deficit");
  const auto status_rule = find_derived_rule("status_satisfied");
  const auto target_rule = find_derived_rule("target_satisfied");
  check(
      derived_rule_vocabulary.size() == 18 &&
          std::all_of(derived_rule_vocabulary.begin(),
                      derived_rule_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.read_only && !entry.runtime_executor &&
                               !entry.projection_scope.empty() &&
                               !entry.projection_quantifier.empty() &&
                               !entry.empty_projection_policy.empty() &&
                               !entry.result_projection_scope.empty() &&
                               !entry.result_projection_quantifier.empty() &&
                               !entry.result_empty_projection_policy.empty();
                      }) &&
          dependency_rule != derived_rule_vocabulary.end() &&
          dependency_rule->projection_scope == "per_dependency_row" &&
          dependency_rule->projection_quantifier == "per_row" &&
          dependency_rule->empty_projection_policy == "not_applicable" &&
          dependency_rule->result_projection_scope ==
              "all_dependency_rows_compact" &&
          dependency_rule->result_projection_quantifier == "all" &&
          dependency_rule->result_empty_projection_policy == "vacuously_true" &&
          all_dependencies_rule != derived_rule_vocabulary.end() &&
          all_dependencies_rule->proof_field ==
              "proof_certificate.dependencies[]" &&
          all_dependencies_rule->rule.find("dependency_satisfied") !=
              std::string::npos &&
          all_dependencies_rule->projection_scope ==
              "all_dependency_rows_compact" &&
          all_dependencies_rule->projection_quantifier == "all" &&
          all_dependencies_rule->empty_projection_policy == "vacuously_true" &&
          all_dependencies_rule->result_projection_scope ==
              "all_dependency_rows_compact" &&
          all_dependencies_rule->result_projection_quantifier == "all" &&
          all_dependencies_rule->result_empty_projection_policy ==
              "vacuously_true" &&
          coverage_rule != derived_rule_vocabulary.end() &&
          coverage_rule->proof_field == "proof_certificate.coverage[]" &&
          coverage_rule->db_cacheable &&
          coverage_rule->projection_scope == "per_coverage_row" &&
          coverage_rule->projection_quantifier == "per_row" &&
          coverage_rule->empty_projection_policy == "not_applicable" &&
          coverage_rule->result_projection_scope ==
              "all_coverage_rows_compact" &&
          coverage_rule->result_projection_quantifier == "all" &&
          coverage_rule->result_empty_projection_policy == "vacuously_true" &&
          all_coverage_rule != derived_rule_vocabulary.end() &&
          all_coverage_rule->proof_field == "proof_certificate.coverage[]" &&
          all_coverage_rule->rule.find("coverage_satisfied") !=
              std::string::npos &&
          all_coverage_rule->projection_scope == "all_coverage_rows_compact" &&
          all_coverage_rule->projection_quantifier == "all" &&
          all_coverage_rule->empty_projection_policy == "vacuously_true" &&
          all_coverage_rule->result_projection_scope ==
              "all_coverage_rows_compact" &&
          all_coverage_rule->result_projection_quantifier == "all" &&
          all_coverage_rule->result_empty_projection_policy ==
              "vacuously_true" &&
          closure_clean_rule != derived_rule_vocabulary.end() &&
          closure_clean_rule->proof_field == "proof_certificate.closure" &&
          closure_clean_rule->rule.find("closure_complete") !=
              std::string::npos &&
          checkpoint_ancestor_rule != derived_rule_vocabulary.end() &&
          checkpoint_ancestor_rule->proof_field ==
              "checkpoint_closure.facts[]" &&
          checkpoint_ancestor_rule->rule.find("ancestor_exposure") !=
              std::string::npos &&
          checkpoint_ancestor_rule->projection_scope ==
              "per_checkpoint_ancestor" &&
          checkpoint_ancestor_rule->result_projection_scope ==
              "any_checkpoint_ancestor" &&
          checkpoint_ancestor_rule->result_projection_quantifier == "any" &&
          checkpoint_ancestor_rule->result_empty_projection_policy ==
              "false_when_empty" &&
          unresolved_lineage_rule != derived_rule_vocabulary.end() &&
          unresolved_lineage_rule->proof_field ==
              "checkpoint_closure.unresolved_input_checkpoints[]" &&
          unresolved_lineage_rule->rule.find("identity_mismatch") !=
              std::string::npos &&
          unresolved_lineage_rule->projection_scope ==
              "per_unresolved_lineage_witness" &&
          unresolved_lineage_rule->result_projection_scope ==
              "any_unresolved_lineage_witness" &&
          unresolved_lineage_rule->result_projection_quantifier == "any" &&
          leakage_rule != derived_rule_vocabulary.end() &&
          leakage_rule->proof_field ==
              "proof_certificate.leakage.overlap_witnesses[]" &&
          leakage_rule->rule.find("protected_split") != std::string::npos &&
          leakage_rule->projection_scope == "per_overlap_witness" &&
          leakage_rule->projection_quantifier == "per_row" &&
          leakage_rule->empty_projection_policy == "not_applicable" &&
          leakage_rule->result_projection_scope == "any_overlap_witness" &&
          leakage_rule->result_projection_quantifier == "any" &&
          leakage_rule->result_empty_projection_policy == "false_when_empty" &&
          no_forbidden_rule != derived_rule_vocabulary.end() &&
          no_forbidden_rule->proof_field == "proof_certificate.leakage" &&
          no_forbidden_rule->rule.find("forbidden_overlap") !=
              std::string::npos &&
          no_forbidden_rule->result_projection_scope == "target" &&
          no_forbidden_rule->projection_quantifier == "none" &&
          no_forbidden_rule->empty_projection_policy == "true_when_empty" &&
          no_forbidden_rule->result_projection_quantifier == "none" &&
          no_forbidden_rule->result_empty_projection_policy ==
              "true_when_empty" &&
          stale_cache_rule != derived_rule_vocabulary.end() &&
          stale_cache_rule->proof_field ==
              "runtime_index_cache_validation.issues[]" &&
          stale_cache_rule->rule.find("cache_valid") != std::string::npos &&
          stale_cache_rule->projection_scope == "per_cache_validation_issue" &&
          stale_cache_rule->result_projection_scope ==
              "any_cache_validation_issue" &&
          stale_cache_rule->result_projection_quantifier == "any" &&
          stale_cache_rule->result_empty_projection_policy ==
              "false_when_empty" &&
          !stale_cache_rule->db_cacheable &&
          plan_rule != derived_rule_vocabulary.end() &&
          !plan_rule->db_cacheable &&
          plan_rule->fail_closed_policy.find("never execute") !=
              std::string::npos &&
          plan_rule->projection_scope == "per_plan_relevant_deficit" &&
          plan_rule->projection_quantifier == "per_row" &&
          plan_rule->empty_projection_policy == "not_applicable" &&
          plan_rule->result_projection_scope == "any_plan_relevant_deficit" &&
          plan_rule->result_projection_quantifier == "any" &&
          plan_rule->result_empty_projection_policy == "false_when_empty" &&
          certificate_check_rule != derived_rule_vocabulary.end() &&
          certificate_check_rule->proof_field == "proof_certificate_check" &&
          certificate_check_rule->db_cacheable &&
          no_blocking_rule != derived_rule_vocabulary.end() &&
          no_blocking_rule->proof_field == "deficits[]" &&
          no_blocking_rule->db_cacheable &&
          status_rule != derived_rule_vocabulary.end() &&
          status_rule->proof_field == "status" &&
          status_rule->rule.find("target_status") != std::string::npos &&
          target_rule != derived_rule_vocabulary.end() &&
          target_rule->proof_field == "status" &&
          relation_has_input(target_rule, "status_satisfied") &&
          relation_has_input(target_rule, "identity_match") &&
          relation_has_input(target_rule, "all_dependencies_satisfied") &&
          relation_has_input(target_rule, "all_coverage_satisfied") &&
          relation_has_input(target_rule, "closure_clean_if_checked") &&
          relation_has_input(target_rule, "no_forbidden_overlap") &&
          relation_has_input(target_rule, "proof_certificate_check_passed") &&
          relation_has_input(target_rule, "no_blocking_deficit") &&
          target_rule->rule.find("status_satisfied") != std::string::npos &&
          target_rule->rule.find("no_forbidden_overlap") != std::string::npos &&
          target_rule->projection_scope == "target" &&
          target_rule->projection_quantifier == "all" &&
          target_rule->empty_projection_policy == "fixed_nonempty" &&
          target_rule->result_projection_scope == "target" &&
          target_rule->result_projection_quantifier == "all" &&
          target_rule->result_empty_projection_policy == "fixed_nonempty",
      "derived query rule vocabulary should expose read-only Datalog-style "
      "relations with rule/result projection scopes/quantifiers/empty policies "
      "and without "
      "runtime "
      "execution "
      "authority");
  const auto projection_semantics_vocabulary =
      target::lattice_derived_query_projection_semantics_vocabulary();
  const auto find_projection_semantics = [&](const std::string &field,
                                             const std::string &value) {
    return std::find_if(projection_semantics_vocabulary.begin(),
                        projection_semantics_vocabulary.end(),
                        [&](const auto &entry) {
                          return entry.field == field && entry.value == value;
                        });
  };
  const auto quantifier_all =
      find_projection_semantics("projection_quantifier", "all");
  const auto quantifier_any =
      find_projection_semantics("projection_quantifier", "any");
  const auto quantifier_none =
      find_projection_semantics("projection_quantifier", "none");
  const auto empty_vacuous =
      find_projection_semantics("empty_projection_policy", "vacuously_true");
  const auto empty_false =
      find_projection_semantics("empty_projection_policy", "false_when_empty");
  const auto empty_true =
      find_projection_semantics("empty_projection_policy", "true_when_empty");
  const auto empty_fixed =
      find_projection_semantics("empty_projection_policy", "fixed_nonempty");
  const auto target_scope =
      find_projection_semantics("projection_scope", "target");
  const auto dependency_scope =
      find_projection_semantics("projection_scope", "per_dependency_row");
  const auto all_dependency_scope = find_projection_semantics(
      "projection_scope", "all_dependency_rows_compact");
  const auto coverage_scope =
      find_projection_semantics("projection_scope", "per_coverage_row");
  const auto all_coverage_scope = find_projection_semantics(
      "projection_scope", "all_coverage_rows_compact");
  const auto closure_scope =
      find_projection_semantics("projection_scope", "root_checkpoint_closure");
  const auto checkpoint_ancestor_scope =
      find_projection_semantics("projection_scope", "per_checkpoint_ancestor");
  const auto any_checkpoint_ancestor_scope =
      find_projection_semantics("projection_scope", "any_checkpoint_ancestor");
  const auto unresolved_lineage_scope = find_projection_semantics(
      "projection_scope", "per_unresolved_lineage_witness");
  const auto any_unresolved_lineage_scope = find_projection_semantics(
      "projection_scope", "any_unresolved_lineage_witness");
  const auto overlap_scope =
      find_projection_semantics("projection_scope", "per_overlap_witness");
  const auto any_overlap_scope =
      find_projection_semantics("projection_scope", "any_overlap_witness");
  const auto warning_scope =
      find_projection_semantics("projection_scope", "per_warning_result");
  const auto any_warning_scope =
      find_projection_semantics("projection_scope", "any_warning_result");
  const auto cache_issue_scope = find_projection_semantics(
      "projection_scope", "per_cache_validation_issue");
  const auto any_cache_issue_scope = find_projection_semantics(
      "projection_scope", "any_cache_validation_issue");
  const auto plan_deficit_scope = find_projection_semantics(
      "projection_scope", "per_plan_relevant_deficit");
  const auto any_plan_deficit_scope = find_projection_semantics(
      "projection_scope", "any_plan_relevant_deficit");
  const auto alias_projection_scope =
      find_projection_semantics("projection_scope", "projection_scope");
  const auto alias_projection_quantifier = find_projection_semantics(
      "projection_quantifier", "projection_quantifier");
  const auto alias_empty_policy = find_projection_semantics(
      "empty_projection_policy", "empty_projection_policy");
  check(
      projection_semantics_vocabulary.size() == 31 &&
          quantifier_all != projection_semantics_vocabulary.end() &&
          quantifier_all->compact_projection &&
          quantifier_all->meaning.find("every summarized row") !=
              std::string::npos &&
          quantifier_any != projection_semantics_vocabulary.end() &&
          quantifier_any->compact_projection &&
          quantifier_none != projection_semantics_vocabulary.end() &&
          quantifier_none->compact_projection &&
          empty_vacuous != projection_semantics_vocabulary.end() &&
          empty_vacuous->meaning.find("universal quantification") !=
              std::string::npos &&
          empty_false != projection_semantics_vocabulary.end() &&
          empty_false->compact_projection &&
          empty_true != projection_semantics_vocabulary.end() &&
          empty_true->compact_projection &&
          empty_fixed != projection_semantics_vocabulary.end() &&
          empty_fixed->meaning.find("fixed premise set") != std::string::npos &&
          target_scope != projection_semantics_vocabulary.end() &&
          !target_scope->result_alias &&
          dependency_scope != projection_semantics_vocabulary.end() &&
          !dependency_scope->compact_projection &&
          all_dependency_scope != projection_semantics_vocabulary.end() &&
          all_dependency_scope->compact_projection &&
          coverage_scope != projection_semantics_vocabulary.end() &&
          all_coverage_scope != projection_semantics_vocabulary.end() &&
          all_coverage_scope->compact_projection &&
          closure_scope != projection_semantics_vocabulary.end() &&
          checkpoint_ancestor_scope != projection_semantics_vocabulary.end() &&
          any_checkpoint_ancestor_scope !=
              projection_semantics_vocabulary.end() &&
          any_checkpoint_ancestor_scope->compact_projection &&
          unresolved_lineage_scope != projection_semantics_vocabulary.end() &&
          any_unresolved_lineage_scope !=
              projection_semantics_vocabulary.end() &&
          any_unresolved_lineage_scope->compact_projection &&
          overlap_scope != projection_semantics_vocabulary.end() &&
          any_overlap_scope != projection_semantics_vocabulary.end() &&
          any_overlap_scope->compact_projection &&
          warning_scope != projection_semantics_vocabulary.end() &&
          any_warning_scope != projection_semantics_vocabulary.end() &&
          any_warning_scope->compact_projection &&
          cache_issue_scope != projection_semantics_vocabulary.end() &&
          any_cache_issue_scope != projection_semantics_vocabulary.end() &&
          any_cache_issue_scope->compact_projection &&
          plan_deficit_scope != projection_semantics_vocabulary.end() &&
          any_plan_deficit_scope != projection_semantics_vocabulary.end() &&
          any_plan_deficit_scope->compact_projection &&
          alias_projection_scope != projection_semantics_vocabulary.end() &&
          alias_projection_scope->result_alias &&
          alias_projection_quantifier !=
              projection_semantics_vocabulary.end() &&
          alias_projection_quantifier->result_alias &&
          alias_empty_policy != projection_semantics_vocabulary.end() &&
          alias_empty_policy->result_alias,
      "derived query projection semantics vocabulary should define "
      "quantifier, scope, empty-policy, and compatibility-alias meanings");
  const auto db_cache_policy_vocabulary =
      target::lattice_db_cache_policy_vocabulary();
  const auto find_db_cache_policy = [&](const std::string &policy) {
    return std::find_if(
        db_cache_policy_vocabulary.begin(), db_cache_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto runtime_files_policy =
      find_db_cache_policy("runtime_files_source_of_truth");
  const auto derived_cache_policy =
      find_db_cache_policy("derived_relation_read_cache");
  const auto plan_cache_policy =
      find_db_cache_policy("plan_projection_not_evidence");
  const auto checkpoint_cache_policy =
      find_db_cache_policy("checkpoint_digest_cache_preview");
  const auto executor_policy = find_db_cache_policy("executor_boundary");
  check(db_cache_policy_vocabulary.size() == 5 &&
            std::all_of(db_cache_policy_vocabulary.begin(),
                        db_cache_policy_vocabulary.end(),
                        [](const auto &entry) {
                          return !entry.db_writes_evidence &&
                                 entry.rebuildable_from_runtime_files &&
                                 !entry.runtime_executor &&
                                 !entry.contract_identity_authority;
                        }) &&
            runtime_files_policy != db_cache_policy_vocabulary.end() &&
            runtime_files_policy->source_of_truth.find("job.manifest") !=
                std::string::npos &&
            runtime_files_policy->cached_relations.empty() &&
            derived_cache_policy != db_cache_policy_vocabulary.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "all_dependencies_satisfied") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "all_coverage_satisfied") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "closure_clean_if_checked") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "no_forbidden_overlap") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "proof_certificate_check_passed") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "no_blocking_deficit") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "status_satisfied") !=
                derived_cache_policy->cached_relations.end() &&
            std::find(derived_cache_policy->cached_relations.begin(),
                      derived_cache_policy->cached_relations.end(),
                      "target_satisfied") !=
                derived_cache_policy->cached_relations.end() &&
            derived_cache_policy->fail_closed_policy.find("recomputed") !=
                std::string::npos &&
            plan_cache_policy != db_cache_policy_vocabulary.end() &&
            plan_cache_policy->allowed_use.find("Advice") ==
                std::string::npos &&
            plan_cache_policy->allowed_use.find("advice only") !=
                std::string::npos &&
            checkpoint_cache_policy != db_cache_policy_vocabulary.end() &&
            checkpoint_cache_policy->fail_closed_policy.find(
                "protocol contract identity") != std::string::npos &&
            executor_policy != db_cache_policy_vocabulary.end() &&
            executor_policy->fail_closed_policy.find("execute waves") !=
                std::string::npos,
        "DB cache policy vocabulary should keep runtime files authoritative, "
        "cache rows rebuildable, planning non-evidence, and Runtime Hero the "
        "only executor");
  const auto evidence_abstraction_vocabulary =
      target::lattice_evidence_abstraction_vocabulary();
  const auto find_abstraction = [&](const std::string &abstraction) {
    return std::find_if(
        evidence_abstraction_vocabulary.begin(),
        evidence_abstraction_vocabulary.end(),
        [&](const auto &entry) { return entry.abstraction == abstraction; });
  };
  const auto coverage_abstraction =
      find_abstraction("coverage_load_abstraction");
  const auto leakage_abstraction = find_abstraction("leakage_abstraction");
  const auto source_receipt_abstraction =
      find_abstraction("source_receipt_audit_abstraction");
  const auto plan_abstraction = find_abstraction("plan_abstraction");
  const auto order_abstraction = find_abstraction("evidence_order_abstraction");
  check(
      evidence_abstraction_vocabulary.size() == 9 &&
          std::all_of(evidence_abstraction_vocabulary.begin(),
                      evidence_abstraction_vocabulary.end(),
                      [](const auto &entry) { return entry.read_only; }) &&
          coverage_abstraction != evidence_abstraction_vocabulary.end() &&
          coverage_abstraction->soundness_condition.find("idempotent union") !=
              std::string::npos &&
          coverage_abstraction->join_semantics.find("interval union") !=
              std::string::npos &&
          leakage_abstraction != evidence_abstraction_vocabulary.end() &&
          leakage_abstraction->conservative_policy.find(
              "cannot prove no leakage") != std::string::npos &&
          source_receipt_abstraction != evidence_abstraction_vocabulary.end() &&
          source_receipt_abstraction->soundness_condition.find(
              "never imply coverage") != std::string::npos &&
          source_receipt_abstraction->conservative_policy.find(
              "audit traceability only") != std::string::npos &&
          plan_abstraction != evidence_abstraction_vocabulary.end() &&
          !plan_abstraction->db_cacheable &&
          plan_abstraction->soundness_condition.find("proof deficits") !=
              std::string::npos &&
          order_abstraction != evidence_abstraction_vocabulary.end() &&
          order_abstraction->join_semantics.find("Pareto dominance") !=
              std::string::npos,
      "evidence abstraction vocabulary should expose concrete-to-abstract "
      "soundness and conservative policies");
  const auto product_state_vocabulary =
      target::lattice_product_evidence_state_vocabulary();
  const auto find_product_state_factor = [&](const std::string &factor) {
    return std::find_if(
        product_state_vocabulary.begin(), product_state_vocabulary.end(),
        [&](const auto &entry) { return entry.factor == factor; });
  };
  const auto identity_factor = find_product_state_factor("identity");
  const auto coverage_factor = find_product_state_factor("coverage_lattice");
  const auto load_factor = find_product_state_factor("exposure_load_monoid");
  const auto leakage_factor = find_product_state_factor("leakage_predicate");
  const auto node_support_factor =
      find_product_state_factor("node_support_matrix");
  const auto source_receipt_factor =
      find_product_state_factor("source_receipt_audit");
  const auto deficit_factor = find_product_state_factor("deficit_vector");
  check(product_state_vocabulary.size() == 9 &&
            std::all_of(product_state_vocabulary.begin(),
                        product_state_vocabulary.end(),
                        [](const auto &entry) {
                          return entry.join_requires_identity_match;
                        }) &&
            identity_factor != product_state_vocabulary.end() &&
            identity_factor->partial_order ==
                "equal_identity_or_incomparable" &&
            identity_factor->status_monotone_dimension &&
            coverage_factor != product_state_vocabulary.end() &&
            coverage_factor->mathematical_object.find("interval_union") !=
                std::string::npos &&
            coverage_factor->status_monotone_dimension &&
            load_factor != product_state_vocabulary.end() &&
            load_factor->mathematical_object.find("monoid") !=
                std::string::npos &&
            !load_factor->status_monotone_dimension &&
            leakage_factor != product_state_vocabulary.end() &&
            leakage_factor->partial_order.find("forbidden_overlap") !=
                std::string::npos &&
            leakage_factor->status_monotone_dimension &&
            node_support_factor != product_state_vocabulary.end() &&
            node_support_factor->mathematical_object.find("matrix") !=
                std::string::npos &&
            !node_support_factor->status_monotone_dimension &&
            source_receipt_factor != product_state_vocabulary.end() &&
            source_receipt_factor->mathematical_object == "audit_receipt_set" &&
            source_receipt_factor->target_effect.find("audit only") !=
                std::string::npos &&
            !source_receipt_factor->status_monotone_dimension &&
            deficit_factor != product_state_vocabulary.end() &&
            !deficit_factor->db_cacheable,
        "product evidence-state vocabulary should expose the factorized "
        "partial order and identity-scoped join policy");
  const auto join_law_vocabulary = target::lattice_join_law_vocabulary();
  const auto find_join_law = [&](const std::string &factor) {
    return std::find_if(
        join_law_vocabulary.begin(), join_law_vocabulary.end(),
        [&](const auto &entry) { return entry.factor == factor; });
  };
  const auto coverage_join = find_join_law("coverage_lattice");
  const auto load_join = find_join_law("exposure_load_monoid");
  const auto closure_join = find_join_law("closure_graph");
  const auto leakage_join = find_join_law("leakage_witness_set");
  const auto node_support_join = find_join_law("node_support_matrix");
  const auto deficit_join = find_join_law("deficit_vector");
  check(
      join_law_vocabulary.size() == 8 &&
          std::all_of(join_law_vocabulary.begin(), join_law_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.associative && entry.commutative &&
                               entry.requires_identity_match;
                      }) &&
          coverage_join != join_law_vocabulary.end() &&
          coverage_join->idempotent && !coverage_join->additive &&
          coverage_join->join_operator == "canonical_interval_union" &&
          coverage_join->cache_requirement.find("missing complement") !=
              std::string::npos &&
          load_join != join_law_vocabulary.end() && !load_join->idempotent &&
          load_join->additive &&
          load_join->duplicate_policy.find("distinct repeated exposures") !=
              std::string::npos &&
          closure_join != join_law_vocabulary.end() &&
          closure_join->idempotent &&
          closure_join->unsafe_join_policy.find("unresolved inputs") !=
              std::string::npos &&
          leakage_join != join_law_vocabulary.end() &&
          leakage_join->idempotent &&
          leakage_join->unsafe_join_policy.find("protected-split proof fail") !=
              std::string::npos &&
          node_support_join != join_law_vocabulary.end() &&
          node_support_join->additive &&
          node_support_join->unsafe_join_policy.find("cross-component") !=
              std::string::npos &&
          deficit_join != join_law_vocabulary.end() &&
          !deficit_join->cache_safe &&
          deficit_join->unsafe_join_policy.find("cannot satisfy a target") !=
              std::string::npos,
      "join-law vocabulary should make idempotent, additive, identity-scoped, "
      "and fail-closed merge rules explicit for future caches");
  const auto node_support_scope_policy_vocabulary =
      target::lattice_node_support_scope_policy_vocabulary();
  const auto find_node_support_policy = [&](const std::string &policy) {
    return std::find_if(
        node_support_scope_policy_vocabulary.begin(),
        node_support_scope_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto mdn_node_support_policy =
      find_node_support_policy("mdn_node_support_scope");
  const auto vicreg_boundary_policy =
      find_node_support_policy("vicreg_shared_encoder_boundary");
  const auto future_representation_policy =
      find_node_support_policy("future_representation_node_support_fact");
  const auto join_boundary_policy =
      find_node_support_policy("node_support_matrix_join_boundary");
  check(node_support_scope_policy_vocabulary.size() == 4 &&
            std::all_of(node_support_scope_policy_vocabulary.begin(),
                        node_support_scope_policy_vocabulary.end(),
                        [](const auto &entry) {
                          return !entry.vicreg_per_node_readiness_authority &&
                                 !entry.synthetic_backfill_allowed &&
                                 !entry.runtime_executor;
                        }) &&
            mdn_node_support_policy !=
                node_support_scope_policy_vocabulary.end() &&
            mdn_node_support_policy->mdn_node_support_authority &&
            mdn_node_support_policy->forbidden_claim.find("VICReg") !=
                std::string::npos &&
            vicreg_boundary_policy !=
                node_support_scope_policy_vocabulary.end() &&
            !vicreg_boundary_policy->mdn_node_support_authority &&
            vicreg_boundary_policy->allowed_claim.find("shared VICReg") !=
                std::string::npos &&
            future_representation_policy !=
                node_support_scope_policy_vocabulary.end() &&
            !future_representation_policy->available_in_v1 &&
            future_representation_policy->future_evidence_required.find(
                "NodeLift") != std::string::npos &&
            join_boundary_policy !=
                node_support_scope_policy_vocabulary.end() &&
            join_boundary_policy->mdn_node_support_authority &&
            join_boundary_policy->forbidden_claim.find("cross-component") !=
                std::string::npos,
        "node-support scope policy should prevent MDN node rows from becoming "
        "VICReg per-node readiness claims");
  const auto node_support_scope_policy_summary =
      target::lattice_node_support_scope_policy_summary();
  check(
      node_support_scope_policy_summary.schema ==
              "kikijyeba.lattice.node_support_scope_policy.summary.v1" &&
          node_support_scope_policy_summary.policy_count == 4 &&
          node_support_scope_policy_summary.available_in_v1_count == 3 &&
          node_support_scope_policy_summary.deferred_future_count == 1 &&
          node_support_scope_policy_summary.mdn_node_support_authority_count ==
              2 &&
          node_support_scope_policy_summary
                  .vicreg_per_node_readiness_authority_count == 0 &&
          node_support_scope_policy_summary.synthetic_backfill_allowed_count ==
              0 &&
          node_support_scope_policy_summary.runtime_executor_count == 0 &&
          node_support_scope_policy_summary.empty_field_count == 0 &&
          node_support_scope_policy_summary.mdn_node_support_scope_available &&
          node_support_scope_policy_summary
              .vicreg_shared_encoder_boundary_available &&
          node_support_scope_policy_summary
              .future_representation_node_support_deferred &&
          node_support_scope_policy_summary
              .node_support_matrix_join_boundary_available &&
          node_support_scope_policy_summary
              .no_vicreg_per_node_readiness_authority &&
          node_support_scope_policy_summary.no_synthetic_backfill &&
          node_support_scope_policy_summary.no_runtime_executor &&
          node_support_scope_policy_summary
              .future_representation_requires_nodelift &&
          node_support_scope_policy_summary
              .mdn_scope_forbids_vicreg_per_node_readiness &&
          node_support_scope_policy_summary
              .vicreg_boundary_forbids_mdn_backfill &&
          node_support_scope_policy_summary
              .matrix_join_boundary_forbids_cross_component &&
          node_support_scope_policy_summary.all_policies_have_claim_text &&
          node_support_scope_policy_summary.summary_self_check_passed &&
          node_support_scope_policy_summary.summary_issue_count == 0 &&
          node_support_scope_policy_summary.summary_issues.empty() &&
          std::find(node_support_scope_policy_summary.policy_names.begin(),
                    node_support_scope_policy_summary.policy_names.end(),
                    "future_representation_node_support_fact") !=
              node_support_scope_policy_summary.policy_names.end(),
      "node-support scope policy summary should self-check the MDN-only "
      "support boundary, VICReg non-claim, and no-synthetic-backfill rule");
  const auto validation_performance_scope_policy_vocabulary =
      target::lattice_validation_performance_scope_policy_vocabulary();
  const auto find_validation_performance_policy =
      [&](const std::string &policy) {
        return std::find_if(
            validation_performance_scope_policy_vocabulary.begin(),
            validation_performance_scope_policy_vocabulary.end(),
            [&](const auto &entry) { return entry.policy == policy; });
      };
  const auto validation_readiness_policy =
      find_validation_performance_policy("validation_eval_readiness_scope");
  const auto evaluation_coverage_policy = find_validation_performance_policy(
      "evaluation_metric_coverage_not_quality");
  const auto calibration_warning_policy =
      find_validation_performance_policy("mdn_calibration_warning_not_gate");
  const auto future_performance_policy = find_validation_performance_policy(
      "future_validation_performance_target");
  check(validation_performance_scope_policy_vocabulary.size() == 4 &&
            std::all_of(validation_performance_scope_policy_vocabulary.begin(),
                        validation_performance_scope_policy_vocabulary.end(),
                        [](const auto &entry) {
                          return entry.uncertainty_required_for_future_gate &&
                                 !entry.runtime_executor;
                        }) &&
            validation_readiness_policy !=
                validation_performance_scope_policy_vocabulary.end() &&
            validation_readiness_policy->evaluation_readiness_authority &&
            !validation_readiness_policy->performance_gate_authority &&
            validation_readiness_policy->forbidden_claim.find(
                "model quality") != std::string::npos &&
            evaluation_coverage_policy !=
                validation_performance_scope_policy_vocabulary.end() &&
            evaluation_coverage_policy->allowed_claim.find(
                "trusted validation anchors") != std::string::npos &&
            !evaluation_coverage_policy->performance_gate_authority &&
            calibration_warning_policy !=
                validation_performance_scope_policy_vocabulary.end() &&
            calibration_warning_policy->authority ==
                "non_blocking_warning_visibility" &&
            !calibration_warning_policy->performance_gate_authority &&
            future_performance_policy !=
                validation_performance_scope_policy_vocabulary.end() &&
            !future_performance_policy->available_in_v1 &&
            future_performance_policy->performance_gate_authority,
        "validation performance scope policy should keep V1 evaluation "
        "readiness separate from future performance gates");
  const auto validation_performance_scope_policy_summary =
      target::lattice_validation_performance_scope_policy_summary();
  check(
      validation_performance_scope_policy_summary.schema ==
              "kikijyeba.lattice.validation_performance_scope_policy.summary."
              "v1" &&
          validation_performance_scope_policy_summary.policy_count == 4 &&
          validation_performance_scope_policy_summary.available_in_v1_count ==
              3 &&
          validation_performance_scope_policy_summary.deferred_future_count ==
              1 &&
          validation_performance_scope_policy_summary
                  .evaluation_readiness_authority_count == 2 &&
          validation_performance_scope_policy_summary
                  .performance_gate_authority_count == 1 &&
          validation_performance_scope_policy_summary
                  .uncertainty_required_for_future_gate_count == 4 &&
          validation_performance_scope_policy_summary.runtime_executor_count ==
              0 &&
          validation_performance_scope_policy_summary.empty_field_count == 0 &&
          validation_performance_scope_policy_summary
              .validation_eval_readiness_scope_available &&
          validation_performance_scope_policy_summary
              .evaluation_metric_coverage_not_quality_present &&
          validation_performance_scope_policy_summary
              .mdn_calibration_warning_not_gate_present &&
          validation_performance_scope_policy_summary
              .future_validation_performance_target_deferred &&
          validation_performance_scope_policy_summary
              .v1_evaluation_readiness_not_performance_gate &&
          validation_performance_scope_policy_summary
              .future_gate_requires_uncertainty &&
          validation_performance_scope_policy_summary
              .future_gate_mentions_selection_leakage &&
          validation_performance_scope_policy_summary
              .all_policies_require_uncertainty_for_future_gate &&
          validation_performance_scope_policy_summary.no_runtime_executor &&
          validation_performance_scope_policy_summary
              .all_policies_have_claim_text &&
          validation_performance_scope_policy_summary
              .summary_self_check_passed &&
          validation_performance_scope_policy_summary.summary_issue_count ==
              0 &&
          validation_performance_scope_policy_summary.summary_issues.empty() &&
          std::find(
              validation_performance_scope_policy_summary.policy_names.begin(),
              validation_performance_scope_policy_summary.policy_names.end(),
              "future_validation_performance_target") !=
              validation_performance_scope_policy_summary.policy_names.end(),
      "validation performance scope summary should self-check that V1 "
      "validation eval readiness is not performance acceptance");
  const auto representation_geometry_vocabulary =
      target::lattice_representation_geometry_vocabulary();
  const auto find_representation_geometry_metric =
      [&](const std::string &metric) {
        return std::find_if(
            representation_geometry_vocabulary.begin(),
            representation_geometry_vocabulary.end(),
            [&](const auto &entry) { return entry.metric == metric; });
      };
  const auto covariance_metric =
      find_representation_geometry_metric("mean_covariance_loss");
  const auto effective_rank_metric =
      find_representation_geometry_metric("representation_effective_rank");
  const auto condition_metric =
      find_representation_geometry_metric("representation_condition_number");
  const auto isotropy_metric =
      find_representation_geometry_metric("representation_isotropy_score");
  const auto finite_parameter_metric =
      find_representation_geometry_metric("finite_parameter_check");
  const auto augmented_count_metric =
      find_representation_geometry_metric("augmented_valid_feature_count");
  const auto augmented_valid_fraction_metric =
      find_representation_geometry_metric(
          "mean_augmented_valid_feature_fraction");
  const auto augmented_retention_metric = find_representation_geometry_metric(
      "mean_augmented_feature_retention_fraction");
  check(
      representation_geometry_vocabulary.size() == 18 &&
          std::all_of(representation_geometry_vocabulary.begin(),
                      representation_geometry_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.warning_kind == "representation_health" &&
                               entry.readiness_effect ==
                                   "non_blocking_warning_only" &&
                               entry.v1_authority ==
                                   "representation_health_facts" &&
                               entry.available_in_v1 && !entry.performance_gate;
                      }) &&
          std::count_if(
              representation_geometry_vocabulary.begin(),
              representation_geometry_vocabulary.end(),
              [](const auto &entry) { return entry.geometry_metric; }) == 7 &&
          covariance_metric != representation_geometry_vocabulary.end() &&
          covariance_metric->statistic_family == "vicreg_loss_component" &&
          covariance_metric->threshold_direction == "above" &&
          effective_rank_metric != representation_geometry_vocabulary.end() &&
          effective_rank_metric->geometry_metric &&
          effective_rank_metric->threshold_direction == "below" &&
          condition_metric != representation_geometry_vocabulary.end() &&
          condition_metric->unit == "condition_number" &&
          condition_metric->threshold_direction == "above" &&
          isotropy_metric != representation_geometry_vocabulary.end() &&
          isotropy_metric->unit == "fraction" &&
          isotropy_metric->threshold_direction == "below" &&
          finite_parameter_metric != representation_geometry_vocabulary.end() &&
          finite_parameter_metric->unit == "boolean" &&
          finite_parameter_metric->future_hard_gate_candidate &&
          augmented_count_metric != representation_geometry_vocabulary.end() &&
          augmented_count_metric->statistic_family ==
              "augmented_view_support" &&
          augmented_count_metric->unit == "count" &&
          augmented_count_metric->threshold_direction == "below" &&
          augmented_valid_fraction_metric !=
              representation_geometry_vocabulary.end() &&
          augmented_valid_fraction_metric->statistic_family ==
              "augmented_view_support" &&
          augmented_valid_fraction_metric->unit == "fraction" &&
          augmented_valid_fraction_metric->threshold_direction == "below" &&
          augmented_retention_metric !=
              representation_geometry_vocabulary.end() &&
          augmented_retention_metric->statistic_family ==
              "augmented_view_support" &&
          augmented_retention_metric->unit == "fraction" &&
          augmented_retention_metric->threshold_direction == "below",
      "representation geometry vocabulary should expose VICReg health and "
      "geometry metrics as V1 non-blocking warning visibility, not "
      "performance gates");
  const auto representation_geometry_summary =
      target::lattice_representation_geometry_summary();
  check(
      representation_geometry_summary.schema ==
              "kikijyeba.lattice.representation_geometry.summary.v1" &&
          representation_geometry_summary.metric_count == 18 &&
          representation_geometry_summary.available_in_v1_count == 18 &&
          representation_geometry_summary.geometry_metric_count == 7 &&
          representation_geometry_summary.performance_gate_count == 0 &&
          representation_geometry_summary.future_hard_gate_candidate_count ==
              11 &&
          representation_geometry_summary.non_blocking_warning_count == 18 &&
          representation_geometry_summary
                  .representation_health_authority_count == 18 &&
          representation_geometry_summary.empty_field_count == 0 &&
          representation_geometry_summary.loss_component_metrics_present &&
          representation_geometry_summary.optimization_health_metrics_present &&
          representation_geometry_summary.projection_support_metric_present &&
          representation_geometry_summary.adapter_support_metric_present &&
          representation_geometry_summary
              .augmented_view_support_metrics_present &&
          representation_geometry_summary.finite_parameter_check_present &&
          representation_geometry_summary.embedding_spectrum_geometry_present &&
          representation_geometry_summary.threshold_directions_present &&
          representation_geometry_summary.all_metrics_available_in_v1 &&
          representation_geometry_summary.all_metrics_non_blocking_warnings &&
          representation_geometry_summary
              .all_metrics_use_representation_health_authority &&
          representation_geometry_summary.no_performance_gates &&
          representation_geometry_summary
              .future_gate_candidates_are_not_active_gates &&
          representation_geometry_summary.all_metrics_have_runtime_fields &&
          representation_geometry_summary.summary_self_check_passed &&
          representation_geometry_summary.summary_issue_count == 0 &&
          representation_geometry_summary.summary_issues.empty() &&
          std::find(representation_geometry_summary.metric_names.begin(),
                    representation_geometry_summary.metric_names.end(),
                    "representation_effective_rank_fraction") !=
              representation_geometry_summary.metric_names.end(),
      "representation geometry summary should self-check V1 non-blocking "
      "VICReg health and embedding-geometry visibility");
  const auto performance_uncertainty_vocabulary =
      target::lattice_performance_uncertainty_policy_vocabulary();
  const auto find_performance_uncertainty_policy =
      [&](const std::string &policy) {
        return std::find_if(
            performance_uncertainty_vocabulary.begin(),
            performance_uncertainty_vocabulary.end(),
            [&](const auto &entry) { return entry.policy == policy; });
      };
  const auto support_uncertainty_policy =
      find_performance_uncertainty_policy("support_fraction_wilson_v1");
  const auto scoring_uncertainty_policy =
      find_performance_uncertainty_policy("proper_scoring_loss_future");
  const auto calibration_uncertainty_policy =
      find_performance_uncertainty_policy("calibration_fraction_future");
  const auto pit_uncertainty_policy =
      find_performance_uncertainty_policy("pit_uniformity_future");
  const auto selection_safe_gate_policy = find_performance_uncertainty_policy(
      "selection_safe_performance_gate_future");
  check(
      performance_uncertainty_vocabulary.size() == 6 &&
          std::all_of(performance_uncertainty_vocabulary.begin(),
                      performance_uncertainty_vocabulary.end(),
                      [](const auto &entry) {
                        return !entry.performance_gate_authority &&
                               !entry.point_estimate_gate_allowed &&
                               entry.confidence_bound_required &&
                               entry.selection_leakage_policy_required;
                      }) &&
          support_uncertainty_policy !=
              performance_uncertainty_vocabulary.end() &&
          support_uncertainty_policy->available_in_v1 &&
          support_uncertainty_policy->estimator == "wilson_score_interval_95" &&
          support_uncertainty_policy->gate_rule.find(
              "conservative confidence bound") != std::string::npos &&
          scoring_uncertainty_policy !=
              performance_uncertainty_vocabulary.end() &&
          !scoring_uncertainty_policy->available_in_v1 &&
          scoring_uncertainty_policy->confidence_policy.find("upper") !=
              std::string::npos &&
          calibration_uncertainty_policy !=
              performance_uncertainty_vocabulary.end() &&
          calibration_uncertainty_policy->estimator.find("wilson") !=
              std::string::npos &&
          pit_uncertainty_policy != performance_uncertainty_vocabulary.end() &&
          pit_uncertainty_policy->required_runtime_fields.front() ==
              "pit_sample_count" &&
          selection_safe_gate_policy !=
              performance_uncertainty_vocabulary.end() &&
          selection_safe_gate_policy->future_requirement.find(
              "selection-event") != std::string::npos,
      "performance uncertainty policy should require conservative uncertainty "
      "bounds and selection-leakage policy before future performance gates");
  const auto performance_uncertainty_summary =
      target::lattice_performance_uncertainty_policy_summary();
  check(
      performance_uncertainty_summary.schema ==
              "kikijyeba.lattice.performance_uncertainty_policy.summary.v1" &&
          performance_uncertainty_summary.policy_count == 6 &&
          performance_uncertainty_summary.available_in_v1_count == 1 &&
          performance_uncertainty_summary.deferred_future_count == 5 &&
          performance_uncertainty_summary.performance_gate_authority_count ==
              0 &&
          performance_uncertainty_summary.point_estimate_gate_allowed_count ==
              0 &&
          performance_uncertainty_summary.confidence_bound_required_count ==
              6 &&
          performance_uncertainty_summary
                  .selection_leakage_policy_required_count == 6 &&
          performance_uncertainty_summary.empty_field_count == 0 &&
          performance_uncertainty_summary
              .support_fraction_wilson_v1_available &&
          performance_uncertainty_summary.proper_scoring_loss_deferred &&
          performance_uncertainty_summary.calibration_fraction_deferred &&
          performance_uncertainty_summary.pit_uniformity_deferred &&
          performance_uncertainty_summary
              .node_stratified_performance_deferred &&
          performance_uncertainty_summary.selection_safe_gate_policy_deferred &&
          performance_uncertainty_summary
              .all_policies_require_confidence_bounds &&
          performance_uncertainty_summary
              .all_policies_require_selection_leakage_policy &&
          performance_uncertainty_summary.no_point_estimate_gates &&
          performance_uncertainty_summary.no_v1_performance_gate_authority &&
          performance_uncertainty_summary.future_policies_deferred &&
          performance_uncertainty_summary.all_policies_have_claim_text &&
          performance_uncertainty_summary.summary_self_check_passed &&
          performance_uncertainty_summary.summary_issue_count == 0 &&
          performance_uncertainty_summary.summary_issues.empty() &&
          std::find(performance_uncertainty_summary.policy_names.begin(),
                    performance_uncertainty_summary.policy_names.end(),
                    "selection_safe_performance_gate_future") !=
              performance_uncertainty_summary.policy_names.end(),
      "performance uncertainty policy summary should self-check conservative "
      "future-gate uncertainty and selection-leakage requirements");
  const auto validation_performance_evidence_vocabulary =
      target::lattice_validation_performance_evidence_policy_vocabulary();
  const auto find_validation_performance_evidence_policy =
      [&](const std::string &policy) {
        return std::find_if(
            validation_performance_evidence_vocabulary.begin(),
            validation_performance_evidence_vocabulary.end(),
            [&](const auto &entry) { return entry.policy == policy; });
      };
  const auto validation_support_policy =
      find_validation_performance_evidence_policy(
          "valid_target_fraction_wilson_visibility");
  const auto validation_mean_nll_policy =
      find_validation_performance_evidence_policy(
          "mean_nll_warning_visibility");
  const auto validation_syntax_guard =
      find_validation_performance_evidence_policy(
          "performance_acceptance_syntax_guard");
  check(validation_performance_evidence_vocabulary.size() == 5 &&
            std::all_of(validation_performance_evidence_vocabulary.begin(),
                        validation_performance_evidence_vocabulary.end(),
                        [](const auto &entry) {
                          return !entry.performance_gate_authority &&
                                 entry.requires_active_identity &&
                                 entry.requires_exact_checkpoint_binding &&
                                 entry.requires_selection_signal_clean &&
                                 std::find(entry.fail_closed_cases.begin(),
                                           entry.fail_closed_cases.end(),
                                           "missing_uncertainty") !=
                                     entry.fail_closed_cases.end() &&
                                 std::find(entry.fail_closed_cases.begin(),
                                           entry.fail_closed_cases.end(),
                                           "wrong_checkpoint_binding") !=
                                     entry.fail_closed_cases.end() &&
                                 std::find(entry.fail_closed_cases.begin(),
                                           entry.fail_closed_cases.end(),
                                           "stale_identity") !=
                                     entry.fail_closed_cases.end() &&
                                 std::find(entry.fail_closed_cases.begin(),
                                           entry.fail_closed_cases.end(),
                                           "selector_leakage") !=
                                     entry.fail_closed_cases.end();
                        }) &&
            validation_support_policy !=
                validation_performance_evidence_vocabulary.end() &&
            validation_support_policy->available_in_v3c &&
            validation_support_policy->metric == "valid_target_fraction" &&
            validation_support_policy->uncertainty_method ==
                "wilson_score_interval_95" &&
            validation_support_policy->uncertainty_bound_fields.size() == 2 &&
            validation_mean_nll_policy !=
                validation_performance_evidence_vocabulary.end() &&
            validation_mean_nll_policy->available_in_v3c &&
            validation_mean_nll_policy->warning_visibility &&
            !validation_mean_nll_policy->requires_uncertainty_bounds &&
            validation_mean_nll_policy->uncertainty_bound_fields.empty() &&
            validation_syntax_guard !=
                validation_performance_evidence_vocabulary.end() &&
            !validation_syntax_guard->available_in_v3c &&
            validation_syntax_guard->target_syntax.find(
                "validation_performance") != std::string::npos,
        "validation performance evidence policy should bind metrics to split, "
        "checkpoint, sample, uncertainty, and fail-closed selector rules");
  const auto validation_performance_evidence_summary =
      target::lattice_validation_performance_evidence_policy_summary();
  check(
      validation_performance_evidence_summary.schema ==
              "kikijyeba.lattice.validation_performance_evidence_policy."
              "summary.v1" &&
          validation_performance_evidence_summary.policy_count == 5 &&
          validation_performance_evidence_summary.available_in_v3c_count == 2 &&
          validation_performance_evidence_summary.deferred_future_count == 3 &&
          validation_performance_evidence_summary
                  .performance_gate_authority_count == 0 &&
          validation_performance_evidence_summary.warning_visibility_count ==
              2 &&
          validation_performance_evidence_summary.future_gate_candidate_count ==
              3 &&
          validation_performance_evidence_summary
                  .active_identity_required_count == 5 &&
          validation_performance_evidence_summary
                  .exact_checkpoint_binding_required_count == 5 &&
          validation_performance_evidence_summary
                  .selection_signal_clean_required_count == 5 &&
          validation_performance_evidence_summary
                  .point_estimate_without_uncertainty_count == 1 &&
          validation_performance_evidence_summary
                  .point_estimate_without_uncertainty_gate_count == 0 &&
          validation_performance_evidence_summary
                  .missing_fail_closed_case_count == 0 &&
          validation_performance_evidence_summary.empty_field_count == 0 &&
          validation_performance_evidence_summary
              .finite_supported_metrics_documented &&
          validation_performance_evidence_summary
              .fact_fields_name_split_checkpoint_sample_uncertainty &&
          validation_performance_evidence_summary
              .available_point_estimates_are_bounded_or_warning_only &&
          validation_performance_evidence_summary
              .no_point_estimate_gate_without_uncertainty &&
          validation_performance_evidence_summary
              .exact_checkpoint_binding_required &&
          validation_performance_evidence_summary.active_identity_required &&
          validation_performance_evidence_summary
              .selection_signal_clean_required &&
          validation_performance_evidence_summary.default_warning_visibility &&
          validation_performance_evidence_summary
              .no_v3c_performance_gate_authority &&
          validation_performance_evidence_summary
              .readiness_syntax_distinct_from_performance_acceptance &&
          validation_performance_evidence_summary
              .failure_cases_fail_closed_named &&
          validation_performance_evidence_summary
              .mean_nll_warning_only_until_uncertainty &&
          validation_performance_evidence_summary.summary_self_check_passed &&
          validation_performance_evidence_summary.summary_issue_count == 0 &&
          validation_performance_evidence_summary.summary_issues.empty() &&
          std::find(
              validation_performance_evidence_summary.supported_metrics.begin(),
              validation_performance_evidence_summary.supported_metrics.end(),
              "mean_nll") !=
              validation_performance_evidence_summary.supported_metrics.end(),
      "validation performance evidence summary should self-check V3-C finite "
      "metrics, checkpoint binding, uncertainty policy, and fail-closed "
      "posture");
  const auto mdn_distribution_vocabulary =
      target::lattice_mdn_distribution_calibration_vocabulary();
  const auto find_mdn_distribution_metric = [&](const std::string &metric) {
    return std::find_if(
        mdn_distribution_vocabulary.begin(), mdn_distribution_vocabulary.end(),
        [&](const auto &entry) { return entry.metric == metric; });
  };
  const auto mean_nll_metric = find_mdn_distribution_metric("mean_nll");
  const auto pit_metric = find_mdn_distribution_metric("pit_ks_statistic");
  const auto crps_metric = find_mdn_distribution_metric("crps");
  check(mdn_distribution_vocabulary.size() == 6 &&
            std::all_of(mdn_distribution_vocabulary.begin(),
                        mdn_distribution_vocabulary.end(),
                        [](const auto &entry) {
                          return entry.warning_kind ==
                                     "mdn_distribution_calibration" &&
                                 entry.readiness_effect ==
                                     "non_blocking_warning_only" &&
                                 !entry.performance_gate &&
                                 entry.threshold_direction == "above";
                        }) &&
            mean_nll_metric != mdn_distribution_vocabulary.end() &&
            mean_nll_metric->available_in_v1 &&
            mean_nll_metric->required_runtime_fields.front() ==
                "lattice.exposure.fact.mean_nll" &&
            pit_metric != mdn_distribution_vocabulary.end() &&
            !pit_metric->available_in_v1 && pit_metric->unit == "fraction" &&
            crps_metric != mdn_distribution_vocabulary.end() &&
            crps_metric->uncertainty_method.find("future") != std::string::npos,
        "MDN distribution-calibration vocabulary should expose V1 mean NLL "
        "warnings and future non-gating calibration metrics");
  const auto mdn_distribution_summary =
      target::lattice_mdn_distribution_calibration_summary();
  check(
      mdn_distribution_summary.schema ==
              "kikijyeba.lattice.mdn_distribution_calibration.summary.v1" &&
          mdn_distribution_summary.metric_count == 6 &&
          mdn_distribution_summary.available_in_v1_count == 1 &&
          mdn_distribution_summary.deferred_future_count == 5 &&
          mdn_distribution_summary.performance_gate_count == 0 &&
          mdn_distribution_summary.non_blocking_warning_count == 6 &&
          mdn_distribution_summary.above_threshold_direction_count == 6 &&
          mdn_distribution_summary.future_uncertainty_method_count == 5 &&
          mdn_distribution_summary.empty_field_count == 0 &&
          mdn_distribution_summary.mean_nll_available_in_v1 &&
          mdn_distribution_summary.crps_deferred_future &&
          mdn_distribution_summary.pit_ks_deferred_future &&
          mdn_distribution_summary.interval_calibration_deferred_future &&
          mdn_distribution_summary.tail_calibration_deferred_future &&
          mdn_distribution_summary.calibration_slope_deferred_future &&
          mdn_distribution_summary.all_metrics_non_blocking_warnings &&
          mdn_distribution_summary.all_metrics_high_bad_above &&
          mdn_distribution_summary.no_performance_gates &&
          mdn_distribution_summary.future_metrics_require_uncertainty_methods &&
          mdn_distribution_summary.only_mean_nll_available_in_v1 &&
          mdn_distribution_summary.all_metrics_have_runtime_fields &&
          mdn_distribution_summary.summary_self_check_passed &&
          mdn_distribution_summary.summary_issue_count == 0 &&
          mdn_distribution_summary.summary_issues.empty() &&
          std::find(mdn_distribution_summary.metric_names.begin(),
                    mdn_distribution_summary.metric_names.end(),
                    "tail_coverage_error") !=
              mdn_distribution_summary.metric_names.end(),
      "MDN distribution-calibration summary should self-check V1 mean-NLL "
      "warning visibility and future non-gating calibration metrics");
  const auto mdn_diagnostic_vocabulary =
      target::lattice_mdn_distribution_calibration_diagnostic_vocabulary();
  const auto find_mdn_diagnostic_metric = [&](const std::string &metric) {
    return std::find_if(
        mdn_diagnostic_vocabulary.begin(), mdn_diagnostic_vocabulary.end(),
        [&](const auto &entry) { return entry.metric == metric; });
  };
  const auto aggregate_nll_diagnostic = find_mdn_diagnostic_metric("mean_nll");
  const auto channel_nll_diagnostic =
      find_mdn_diagnostic_metric("mean_nll_per_channel_max");
  const auto horizon_nll_diagnostic =
      find_mdn_diagnostic_metric("mean_nll_per_horizon_max");
  const auto per_node_nll_diagnostic =
      find_mdn_diagnostic_metric("per_node_mean_nll_max");
  const auto pit_diagnostic = find_mdn_diagnostic_metric("pit_ks_statistic");
  const auto interval_diagnostic =
      find_mdn_diagnostic_metric("predictive_interval_coverage_error");
  const auto tail_diagnostic =
      find_mdn_diagnostic_metric("tail_coverage_error");
  check(mdn_diagnostic_vocabulary.size() == 8 &&
            std::all_of(mdn_diagnostic_vocabulary.begin(),
                        mdn_diagnostic_vocabulary.end(),
                        [](const auto &entry) {
                          return entry.validation_run_binding_required &&
                                 entry.exact_mdn_checkpoint_required &&
                                 entry.representation_checkpoint_required &&
                                 entry.split_policy_identity_required &&
                                 entry.active_identity_required &&
                                 entry.warning_only &&
                                 !entry.performance_gate &&
                                 !entry.runtime_fields.empty();
                        }) &&
            aggregate_nll_diagnostic != mdn_diagnostic_vocabulary.end() &&
            aggregate_nll_diagnostic->diagnostic_family ==
                "proper_scoring_loss" &&
            aggregate_nll_diagnostic->uncertainty_method ==
                "none_point_estimate_only" &&
            channel_nll_diagnostic != mdn_diagnostic_vocabulary.end() &&
            channel_nll_diagnostic->availability.find("available") == 0 &&
            horizon_nll_diagnostic != mdn_diagnostic_vocabulary.end() &&
            horizon_nll_diagnostic->availability.find("available") == 0 &&
            per_node_nll_diagnostic != mdn_diagnostic_vocabulary.end() &&
            per_node_nll_diagnostic->evidence_basis ==
                "filtered_node_exposure_facts" &&
            pit_diagnostic != mdn_diagnostic_vocabulary.end() &&
            pit_diagnostic->uncertainty_method.find("future") !=
                std::string::npos &&
            interval_diagnostic != mdn_diagnostic_vocabulary.end() &&
            !interval_diagnostic->uncertainty_bound_fields.empty() &&
            tail_diagnostic != mdn_diagnostic_vocabulary.end() &&
            !tail_diagnostic->uncertainty_bound_fields.empty(),
        "MDN diagnostic vocabulary should name V3-D aggregate, stratified, "
        "per-node, and future calibration metrics with checkpoint bindings");
  const auto mdn_diagnostic_summary =
      target::lattice_mdn_distribution_calibration_diagnostic_summary();
  check(mdn_diagnostic_summary.schema ==
                "kikijyeba.lattice.mdn_distribution_calibration_diagnostic."
                "summary.v1" &&
            mdn_diagnostic_summary.diagnostic_count == 8 &&
            mdn_diagnostic_summary.available_metric_count == 4 &&
            mdn_diagnostic_summary.future_metric_count == 4 &&
            mdn_diagnostic_summary.warning_only_count == 8 &&
            mdn_diagnostic_summary.performance_gate_count == 0 &&
            mdn_diagnostic_summary.validation_binding_required_count == 8 &&
            mdn_diagnostic_summary.exact_mdn_checkpoint_required_count == 8 &&
            mdn_diagnostic_summary.representation_checkpoint_required_count ==
                8 &&
            mdn_diagnostic_summary.split_policy_identity_required_count == 8 &&
            mdn_diagnostic_summary.active_identity_required_count == 8 &&
            mdn_diagnostic_summary.point_estimate_only_count == 4 &&
            mdn_diagnostic_summary.future_uncertainty_count == 4 &&
            mdn_diagnostic_summary.empty_field_count == 0 &&
            mdn_diagnostic_summary.finite_first_set_declared &&
            mdn_diagnostic_summary.proper_scoring_metrics_present &&
            mdn_diagnostic_summary.pit_histogram_summary_deferred &&
            mdn_diagnostic_summary.interval_and_tail_coverage_deferred &&
            mdn_diagnostic_summary.per_node_calibration_present &&
            mdn_diagnostic_summary
                .all_diagnostics_bind_identity_and_checkpoints &&
            mdn_diagnostic_summary.all_diagnostics_warning_only &&
            mdn_diagnostic_summary.no_hard_calibration_gates &&
            mdn_diagnostic_summary.available_point_estimates_are_warning_only &&
            mdn_diagnostic_summary.future_metrics_have_uncertainty_methods &&
            mdn_diagnostic_summary.all_diagnostics_have_runtime_fields &&
            mdn_diagnostic_summary.summary_self_check_passed &&
            mdn_diagnostic_summary.summary_issue_count == 0 &&
            mdn_diagnostic_summary.summary_issues.empty() &&
            std::find(mdn_diagnostic_summary.diagnostic_metrics.begin(),
                      mdn_diagnostic_summary.diagnostic_metrics.end(),
                      "per_node_mean_nll_max") !=
                mdn_diagnostic_summary.diagnostic_metrics.end(),
        "MDN diagnostic summary should self-check the V3-D warning-only "
        "diagnostic boundary and future uncertainty posture");
  const auto numeric_dimension_vocabulary =
      target::lattice_target_numeric_dimension_vocabulary();
  const auto find_numeric_dimension = [&](const std::string &context,
                                          const std::string &field) {
    return std::find_if(
        numeric_dimension_vocabulary.begin(),
        numeric_dimension_vocabulary.end(), [&](const auto &entry) {
          return entry.context == context && entry.field == field;
        });
  };
  const auto coverage_dimension = find_numeric_dimension(
      "LATTICE_REQUIRES.exposure_coverage", "CURSOR_EPOCHS");
  const auto anchor_fraction_dimension = find_numeric_dimension(
      "LATTICE_WARN.anchor_domain_health", "ACCEPTED_FRACTION_BELOW");
  const auto condition_dimension = find_numeric_dimension(
      "LATTICE_WARN.representation_health.representation_condition_number",
      "ABOVE");
  const auto node_floor_count_dimension = find_numeric_dimension(
      "LATTICE_WARN.node_support_floor.count_metrics", "BELOW");
  const auto mdn_nll_dimension = find_numeric_dimension(
      "LATTICE_WARN.mdn_distribution_calibration.mean_nll", "ABOVE");
  const auto mdn_fraction_dimension = find_numeric_dimension(
      "LATTICE_WARN.mdn_distribution_calibration.fraction_metrics", "ABOVE");
  check(numeric_dimension_vocabulary.size() == 26 &&
            coverage_dimension != numeric_dimension_vocabulary.end() &&
            coverage_dimension->unit == "coverage_fraction" &&
            coverage_dimension->has_minimum &&
            std::abs(coverage_dimension->minimum - 0.0) < 1e-12 &&
            coverage_dimension->has_maximum &&
            std::abs(coverage_dimension->maximum - 1.0) < 1e-12 &&
            !coverage_dimension->integral &&
            anchor_fraction_dimension != numeric_dimension_vocabulary.end() &&
            anchor_fraction_dimension->unit == "fraction" &&
            anchor_fraction_dimension->threshold_direction == "below" &&
            condition_dimension != numeric_dimension_vocabulary.end() &&
            condition_dimension->unit == "condition_number" &&
            std::abs(condition_dimension->minimum - 1.0) < 1e-12 &&
            condition_dimension->threshold_direction == "above" &&
            node_floor_count_dimension != numeric_dimension_vocabulary.end() &&
            node_floor_count_dimension->unit == "count" &&
            node_floor_count_dimension->integral &&
            mdn_nll_dimension != numeric_dimension_vocabulary.end() &&
            mdn_nll_dimension->unit == "nll" &&
            mdn_nll_dimension->threshold_direction == "above" &&
            mdn_fraction_dimension != numeric_dimension_vocabulary.end() &&
            mdn_fraction_dimension->unit == "fraction" &&
            mdn_fraction_dimension->has_maximum,
        "target numeric dimension vocabulary should expose DSL units, bounds, "
        "integrality, and threshold directions");
  const auto numeric_dimension_summary =
      target::lattice_target_numeric_dimension_summary();
  check(
      numeric_dimension_summary.schema ==
              "kikijyeba.lattice.target_numeric_dimension.summary.v1" &&
          numeric_dimension_summary.dimension_count == 26 &&
          numeric_dimension_summary.unit_count == 8 &&
          numeric_dimension_summary.numeric_kind_count == 4 &&
          numeric_dimension_summary.closed_unit_interval_count == 12 &&
          numeric_dimension_summary.non_negative_integer_count == 8 &&
          numeric_dimension_summary.non_negative_real_count == 5 &&
          numeric_dimension_summary.real_at_least_one_count == 1 &&
          numeric_dimension_summary.integral_count == 8 &&
          numeric_dimension_summary.bounded_unit_interval_count == 12 &&
          numeric_dimension_summary.minimum_direction_count == 10 &&
          numeric_dimension_summary.above_direction_count == 8 &&
          numeric_dimension_summary.below_direction_count == 4 &&
          numeric_dimension_summary.metric_declared_direction_count == 3 &&
          numeric_dimension_summary.planning_budget_direction_count == 1 &&
          numeric_dimension_summary.empty_field_count == 0 &&
          numeric_dimension_summary.malformed_bound_count == 0 &&
          numeric_dimension_summary.duplicate_context_field_count == 0 &&
          numeric_dimension_summary.all_dimensions_have_units &&
          numeric_dimension_summary.all_dimensions_have_numeric_kind &&
          numeric_dimension_summary.all_context_fields_unique &&
          numeric_dimension_summary.bounds_are_well_formed &&
          numeric_dimension_summary.unit_interval_rows_are_bounded &&
          numeric_dimension_summary.integral_rows_are_count_like &&
          numeric_dimension_summary.threshold_directions_are_known &&
          numeric_dimension_summary.coverage_cursor_epochs_dimension_present &&
          numeric_dimension_summary
              .repeated_load_cursor_epoch_dimension_present &&
          numeric_dimension_summary.warning_anchor_fraction_dimension_present &&
          numeric_dimension_summary
              .representation_condition_number_dimension_present &&
          numeric_dimension_summary
              .node_support_floor_count_dimension_present &&
          numeric_dimension_summary.mdn_mean_nll_dimension_present &&
          numeric_dimension_summary.coverage_and_load_units_separated &&
          numeric_dimension_summary.summary_self_check_passed &&
          numeric_dimension_summary.summary_issue_count == 0 &&
          numeric_dimension_summary.summary_issues.empty() &&
          std::find(numeric_dimension_summary.units.begin(),
                    numeric_dimension_summary.units.end(),
                    "cursor_epoch") != numeric_dimension_summary.units.end(),
      "target numeric dimension summary should self-check units, bounds, "
      "integrality, threshold directions, and key V1 rows");
  const auto monotonicity_vocabulary =
      target::lattice_monotonicity_invariant_vocabulary();
  const auto find_monotonicity = [&](const std::string &invariant) {
    return std::find_if(
        monotonicity_vocabulary.begin(), monotonicity_vocabulary.end(),
        [&](const auto &entry) { return entry.invariant == invariant; });
  };
  const auto clean_target_invariant =
      find_monotonicity("clean_evidence_growth_target_satisfaction");
  const auto leakage_invariant =
      find_monotonicity("leakage_cleanliness_antitone");
  const auto pareto_invariant = find_monotonicity("pareto_order_no_scalar");
  const auto visibility_invariant =
      find_monotonicity("visibility_dimensions_do_not_dominate");
  check(monotonicity_vocabulary.size() == 8 &&
            clean_target_invariant != monotonicity_vocabulary.end() &&
            clean_target_invariant->target_status_monotone &&
            clean_target_invariant->order == "upper_set_over_clean_evidence" &&
            std::find(clean_target_invariant->weaker_dimensions.begin(),
                      clean_target_invariant->weaker_dimensions.end(),
                      "forbidden_overlap") !=
                clean_target_invariant->weaker_dimensions.end() &&
            leakage_invariant != monotonicity_vocabulary.end() &&
            leakage_invariant->failure_mode.find("fails leakage") !=
                std::string::npos &&
            leakage_invariant->target_status_monotone &&
            pareto_invariant != monotonicity_vocabulary.end() &&
            pareto_invariant->failure_mode.find("incomparable") !=
                std::string::npos &&
            visibility_invariant != monotonicity_vocabulary.end() &&
            std::find(visibility_invariant->neutral_dimensions.begin(),
                      visibility_invariant->neutral_dimensions.end(),
                      "warning_result_count") !=
                visibility_invariant->neutral_dimensions.end(),
        "monotonicity invariant vocabulary should expose clean-growth, "
        "anti-monotone, Pareto, and visibility-only semantics");
  const auto proof_obligation_vocabulary =
      target::lattice_proof_obligation_vocabulary();
  const auto find_obligation = [&](const std::string &obligation) {
    return std::find_if(
        proof_obligation_vocabulary.begin(), proof_obligation_vocabulary.end(),
        [&](const auto &entry) { return entry.obligation == obligation; });
  };
  const auto identity_obligation = find_obligation("identity");
  const auto leakage_obligation = find_obligation("leakage");
  const auto node_support_obligation = find_obligation("node_support");
  const auto deficits_obligation = find_obligation("deficits");
  const auto certificate_check_obligation =
      find_obligation("certificate_check");
  check(proof_obligation_vocabulary.size() == 10 &&
            identity_obligation != proof_obligation_vocabulary.end() &&
            identity_obligation->certificate_field ==
                "proof_certificate.identity" &&
            identity_obligation->required_for_target_satisfaction &&
            identity_obligation->planner_relevant &&
            leakage_obligation != proof_obligation_vocabulary.end() &&
            leakage_obligation->derived_relation == "no_forbidden_overlap" &&
            leakage_obligation->failure_status == "exposure_failed" &&
            leakage_obligation->non_claiming_condition.find(
                "unchecked leakage") != std::string::npos &&
            node_support_obligation != proof_obligation_vocabulary.end() &&
            !node_support_obligation->required_for_target_satisfaction &&
            node_support_obligation->failure_status == "none" &&
            deficits_obligation != proof_obligation_vocabulary.end() &&
            deficits_obligation->deficit_kind.find("certificate") !=
                std::string::npos &&
            deficits_obligation->deficit_kind.find("model_state") !=
                std::string::npos &&
            deficits_obligation->deficit_kind.find("artifact") !=
                std::string::npos &&
            deficits_obligation->deficit_kind.find("metric") !=
                std::string::npos &&
            certificate_check_obligation != proof_obligation_vocabulary.end() &&
            certificate_check_obligation->required_for_target_satisfaction &&
            certificate_check_obligation->failure_status == "blocked" &&
            !certificate_check_obligation->contributes_to_certificate_digest &&
            certificate_check_obligation->self_check_basis.find("digest") !=
                std::string::npos,
        "proof obligation vocabulary should map certificate fields to "
        "relations, status effects, non-claiming semantics, and planner "
        "relevance");
  const auto digest_policy_vocabulary =
      target::lattice_proof_certificate_digest_policy_vocabulary();
  const auto digest_policy_summary =
      target::lattice_proof_certificate_digest_policy_summary();
  const auto find_digest_policy = [&](const std::string &surface) {
    return std::find_if(
        digest_policy_vocabulary.begin(), digest_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.surface == surface; });
  };
  const auto digest_self_policy = find_digest_policy("certificate_digest");
  const auto target_split_digest_policy =
      find_digest_policy("target_and_split_identity");
  const auto identity_digest_policy = find_digest_policy("identity_proof");
  const auto dependency_digest_policy = find_digest_policy("dependency_proofs");
  const auto coverage_digest_policy = find_digest_policy("coverage_proofs");
  const auto closure_digest_policy = find_digest_policy("closure_causal_graph");
  const auto checkpoint_preview_digest_policy =
      find_digest_policy("checkpoint_identity_previews");
  const auto leakage_digest_policy = find_digest_policy("leakage_proof");
  const auto node_support_digest_policy =
      find_digest_policy("node_support_summaries");
  const auto warning_plan_digest_policy =
      find_digest_policy("warnings_deficits_and_plans");
  const auto policy_metadata_digest_policy =
      find_digest_policy("evidence_order_and_policy_vocabularies");
  check(
      digest_policy_vocabulary.size() == 11 &&
          digest_self_policy != digest_policy_vocabulary.end() &&
          !digest_self_policy->contributes_to_certificate_digest &&
          digest_self_policy->status_authority &&
          digest_self_policy->excluded_fields.find(
              "warning_anchor_scope_policy_vocabulary") != std::string::npos &&
          digest_self_policy->excluded_fields.find("derived_query_results") !=
              std::string::npos &&
          digest_self_policy->self_check_rule.find(
              "canonical_lattice_target_proof_certificate_text") !=
              std::string::npos &&
          target_split_digest_policy != digest_policy_vocabulary.end() &&
          target_split_digest_policy->contributes_to_certificate_digest &&
          target_split_digest_policy->status_authority &&
          identity_digest_policy != digest_policy_vocabulary.end() &&
          identity_digest_policy->contributes_to_certificate_digest &&
          identity_digest_policy->status_authority &&
          dependency_digest_policy != digest_policy_vocabulary.end() &&
          dependency_digest_policy->contributes_to_certificate_digest &&
          dependency_digest_policy->status_authority &&
          coverage_digest_policy != digest_policy_vocabulary.end() &&
          coverage_digest_policy->contributes_to_certificate_digest &&
          coverage_digest_policy->included_fields.find(
              "exposure load summary") != std::string::npos &&
          closure_digest_policy != digest_policy_vocabulary.end() &&
          closure_digest_policy->contributes_to_certificate_digest &&
          closure_digest_policy->boundary.find("path/exposure-fact") !=
              std::string::npos &&
          checkpoint_preview_digest_policy != digest_policy_vocabulary.end() &&
          checkpoint_preview_digest_policy->contributes_to_certificate_digest &&
          checkpoint_preview_digest_policy->visibility_only &&
          checkpoint_preview_digest_policy->audit_only &&
          leakage_digest_policy != digest_policy_vocabulary.end() &&
          leakage_digest_policy->contributes_to_certificate_digest &&
          leakage_digest_policy->status_authority &&
          node_support_digest_policy != digest_policy_vocabulary.end() &&
          node_support_digest_policy->contributes_to_certificate_digest &&
          !node_support_digest_policy->status_authority &&
          node_support_digest_policy->visibility_only &&
          warning_plan_digest_policy != digest_policy_vocabulary.end() &&
          !warning_plan_digest_policy->contributes_to_certificate_digest &&
          warning_plan_digest_policy->advisory_only &&
          warning_plan_digest_policy->visibility_only &&
          warning_plan_digest_policy->excluded_fields.find(
              "warning_anchor_scope_policy_vocabulary") != std::string::npos &&
          policy_metadata_digest_policy != digest_policy_vocabulary.end() &&
          !policy_metadata_digest_policy->contributes_to_certificate_digest &&
          policy_metadata_digest_policy->visibility_only &&
          policy_metadata_digest_policy->excluded_fields.find(
              "derived_query_results") != std::string::npos &&
          std::none_of(digest_policy_vocabulary.begin(),
                       digest_policy_vocabulary.end(),
                       [](const auto &entry) {
                         return entry.advisory_only &&
                                entry.contributes_to_certificate_digest;
                       }),
      "proof certificate digest policy vocabulary should expose which proof "
      "surfaces are hashed and keep warnings, plans, and policy metadata "
      "outside the digest");
  check(
      digest_policy_summary.schema ==
              "kikijyeba.lattice.proof_certificate_digest_policy.summary.v1" &&
          digest_policy_summary.policy_count == 11 &&
          digest_policy_summary.digest_contributing_count == 8 &&
          digest_policy_summary.digest_excluded_count == 3 &&
          digest_policy_summary.status_authority_count == 7 &&
          digest_policy_summary.advisory_only_count == 1 &&
          digest_policy_summary.visibility_only_count == 4 &&
          digest_policy_summary.audit_only_count == 2 &&
          digest_policy_summary.digest_non_status_authority_count == 2 &&
          digest_policy_summary.status_authority_not_digest_count == 1 &&
          digest_policy_summary.advisory_digest_overlap_count == 0 &&
          digest_policy_summary.status_authority_visibility_overlap_count ==
              0 &&
          digest_policy_summary.empty_policy_field_count == 0 &&
          digest_policy_summary.all_required_surfaces_present &&
          digest_policy_summary.digest_has_core_status_authority_surfaces &&
          digest_policy_summary.certificate_digest_self_excluded &&
          digest_policy_summary.advisory_report_surfaces_outside_digest &&
          digest_policy_summary.visibility_surfaces_do_not_drive_status &&
          digest_policy_summary.digest_contributors_have_self_checks &&
          digest_policy_summary.all_policies_have_boundary_text &&
          digest_policy_summary.summary_self_check_passed &&
          digest_policy_summary.summary_issue_count == 0 &&
          digest_policy_summary.missing_required_surfaces.empty() &&
          digest_policy_summary.summary_issues.empty() &&
          std::find(digest_policy_summary.digest_contributing_surfaces.begin(),
                    digest_policy_summary.digest_contributing_surfaces.end(),
                    "checkpoint_identity_previews") !=
              digest_policy_summary.digest_contributing_surfaces.end() &&
          std::find(digest_policy_summary.digest_contributing_surfaces.begin(),
                    digest_policy_summary.digest_contributing_surfaces.end(),
                    "node_support_summaries") !=
              digest_policy_summary.digest_contributing_surfaces.end() &&
          std::find(digest_policy_summary.digest_excluded_surfaces.begin(),
                    digest_policy_summary.digest_excluded_surfaces.end(),
                    "warnings_deficits_and_plans") !=
              digest_policy_summary.digest_excluded_surfaces.end() &&
          std::find(digest_policy_summary.digest_excluded_surfaces.begin(),
                    digest_policy_summary.digest_excluded_surfaces.end(),
                    "evidence_order_and_policy_vocabularies") !=
              digest_policy_summary.digest_excluded_surfaces.end(),
      "proof certificate digest policy summary should self-check hashed "
      "proof surfaces, excluded advisory/report surfaces, and non-status "
      "visibility bindings");
  const auto checkpoint_identity_policy_vocabulary =
      target::lattice_checkpoint_identity_policy_vocabulary();
  const auto find_checkpoint_policy = [&](const std::string &policy) {
    return std::find_if(
        checkpoint_identity_policy_vocabulary.begin(),
        checkpoint_identity_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto path_policy = find_checkpoint_policy("path_exposure_closure_v1");
  const auto preview_policy =
      find_checkpoint_policy("checkpoint_identity_preview_v1");
  const auto exact_binding_policy =
      find_checkpoint_policy("exact_loaded_checkpoint_binding_v1");
  const auto digest_policy =
      find_checkpoint_policy("checkpoint_digest_cache_preview_v1");
  check(
      checkpoint_identity_policy_vocabulary.size() == 4 &&
          path_policy != checkpoint_identity_policy_vocabulary.end() &&
          path_policy->closure_authority && !path_policy->db_cache_key_ready &&
          path_policy->contract_identity_effect.find(
              "not protocol contract identity") != std::string::npos &&
          preview_policy != checkpoint_identity_policy_vocabulary.end() &&
          !preview_policy->closure_authority &&
          std::find(preview_policy->preview_fields.begin(),
                    preview_policy->preview_fields.end(),
                    "checkpoint_file_digest") !=
              preview_policy->preview_fields.end() &&
          exact_binding_policy != checkpoint_identity_policy_vocabulary.end() &&
          exact_binding_policy->binding_requirement.find(
              "runtime-loaded checkpoint paths") != std::string::npos &&
          digest_policy != checkpoint_identity_policy_vocabulary.end() &&
          digest_policy->v2_promotion.find("checkpoint_id/file_digest") !=
              std::string::npos &&
          !digest_policy->db_cache_key_ready,
      "checkpoint identity policy vocabulary should keep V1 closure "
      "path/exposure based while exposing ids and digests as preview-only "
      "metadata");
  const auto checkpoint_selection_policy_vocabulary =
      target::lattice_checkpoint_selection_policy_vocabulary();
  const auto checkpoint_selection_policy_summary =
      target::lattice_checkpoint_selection_policy_summary();
  const auto find_checkpoint_selection_policy = [&](const std::string &policy) {
    return std::find_if(
        checkpoint_selection_policy_vocabulary.begin(),
        checkpoint_selection_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto checkpoint_source_selection =
      find_checkpoint_selection_policy("latest_satisfying_checkpoint_source");
  const auto evaluated_selection = find_checkpoint_selection_policy(
      "latest_satisfying_evaluated_checkpoint_source");
  const auto plan_input_selection =
      find_checkpoint_selection_policy("latest_satisfying_plan_input_hint");
  const auto pareto_not_selector =
      find_checkpoint_selection_policy("pareto_order_not_checkpoint_selector");
  check(
      checkpoint_selection_policy_vocabulary.size() == 4 &&
          std::all_of(checkpoint_selection_policy_vocabulary.begin(),
                      checkpoint_selection_policy_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.requires_target_satisfied &&
                               !entry.performance_selector &&
                               !entry.pareto_optimizer &&
                               !entry.contract_identity_authority &&
                               !entry.runtime_executor;
                      }) &&
          checkpoint_source_selection !=
              checkpoint_selection_policy_vocabulary.end() &&
          checkpoint_source_selection->deterministic_readiness_selector &&
          checkpoint_source_selection->forbidden_claim.find("best metric") !=
              std::string::npos &&
          evaluated_selection != checkpoint_selection_policy_vocabulary.end() &&
          evaluated_selection->exact_runtime_binding_required &&
          evaluated_selection->binding_requirement.find("loaded MDN") !=
              std::string::npos &&
          plan_input_selection !=
              checkpoint_selection_policy_vocabulary.end() &&
          plan_input_selection->surface.find("PLAN_INPUT") !=
              std::string::npos &&
          plan_input_selection->selection_rule.find("symbolic") !=
              std::string::npos &&
          plan_input_selection->binding_requirement.find(
              "subsequent runtime report") != std::string::npos &&
          pareto_not_selector != checkpoint_selection_policy_vocabulary.end() &&
          !pareto_not_selector->deterministic_readiness_selector &&
          pareto_not_selector->future_extension.find("best_nondominated") !=
              std::string::npos,
      "checkpoint selection policy vocabulary should keep latest_satisfying "
      "deterministic and separate from performance or Pareto ranking");
  check(
      checkpoint_selection_policy_summary.schema ==
              "kikijyeba.lattice.checkpoint_selection_policy.summary.v1" &&
          checkpoint_selection_policy_summary.policy_count == 4 &&
          checkpoint_selection_policy_summary.latest_satisfying_surface_count ==
              3 &&
          checkpoint_selection_policy_summary
                  .deterministic_readiness_selector_count == 3 &&
          checkpoint_selection_policy_summary.requires_target_satisfied_count ==
              4 &&
          checkpoint_selection_policy_summary
                  .exact_runtime_binding_required_count == 2 &&
          checkpoint_selection_policy_summary.performance_selector_count == 0 &&
          checkpoint_selection_policy_summary.pareto_optimizer_count == 0 &&
          checkpoint_selection_policy_summary
                  .contract_identity_authority_count == 0 &&
          checkpoint_selection_policy_summary.runtime_executor_count == 0 &&
          checkpoint_selection_policy_summary.empty_policy_field_count == 0 &&
          checkpoint_selection_policy_summary
              .all_policies_require_satisfied_target &&
          checkpoint_selection_policy_summary
              .latest_satisfying_is_readiness_selection &&
          checkpoint_selection_policy_summary
              .validation_eval_requires_exact_runtime_binding &&
          checkpoint_selection_policy_summary
              .plan_input_hint_is_runtime_model_state_advice &&
          checkpoint_selection_policy_summary
              .pareto_order_is_not_checkpoint_selector &&
          checkpoint_selection_policy_summary.no_performance_selector &&
          checkpoint_selection_policy_summary.no_pareto_optimizer &&
          checkpoint_selection_policy_summary.no_contract_identity_authority &&
          checkpoint_selection_policy_summary.no_runtime_executor &&
          checkpoint_selection_policy_summary.all_policies_have_claim_text &&
          checkpoint_selection_policy_summary.summary_self_check_passed &&
          checkpoint_selection_policy_summary.summary_issue_count == 0 &&
          checkpoint_selection_policy_summary.summary_issues.empty() &&
          std::find(checkpoint_selection_policy_summary
                        .deterministic_selector_policies.begin(),
                    checkpoint_selection_policy_summary
                        .deterministic_selector_policies.end(),
                    "latest_satisfying_plan_input_hint") !=
              checkpoint_selection_policy_summary
                  .deterministic_selector_policies.end() &&
          std::find(checkpoint_selection_policy_summary
                        .exact_runtime_binding_policies.begin(),
                    checkpoint_selection_policy_summary
                        .exact_runtime_binding_policies.end(),
                    "latest_satisfying_evaluated_checkpoint_source") !=
              checkpoint_selection_policy_summary.exact_runtime_binding_policies
                  .end(),
      "checkpoint selection policy summary should self-check readiness "
      "selection, exact runtime checkpoint binding, and non-performance "
      "selector boundaries");
  const auto plan_advice_scope_policy_vocabulary =
      target::lattice_plan_advice_scope_policy_vocabulary();
  const auto find_plan_advice_policy = [&](const std::string &policy) {
    return std::find_if(
        plan_advice_scope_policy_vocabulary.begin(),
        plan_advice_scope_policy_vocabulary.end(),
        [&](const auto &entry) { return entry.policy == policy; });
  };
  const auto plan_basis_policy =
      find_plan_advice_policy("plan_basis_deficit_projection");
  const auto suggested_wave_policy =
      find_plan_advice_policy("suggested_wave_advice_only");
  const auto plan_input_policy =
      find_plan_advice_policy("plan_input_model_state_hint");
  const auto attempt_budget_policy =
      find_plan_advice_policy("plan_attempt_budget_guard");
  const auto runtime_executor_boundary =
      find_plan_advice_policy("runtime_hero_executor_boundary");
  check(
      plan_advice_scope_policy_vocabulary.size() == 5 &&
          std::all_of(plan_advice_scope_policy_vocabulary.begin(),
                      plan_advice_scope_policy_vocabulary.end(),
                      [](const auto &entry) {
                        return !entry.evidence_authority &&
                               !entry.lattice_executor &&
                               !entry.affects_contract_identity;
                      }) &&
          plan_basis_policy != plan_advice_scope_policy_vocabulary.end() &&
          plan_basis_policy->advice_only &&
          plan_basis_policy->forbidden_claim.find("proof certificate") !=
              std::string::npos &&
          suggested_wave_policy != plan_advice_scope_policy_vocabulary.end() &&
          suggested_wave_policy->forbidden_claim.find("dispatch") !=
              std::string::npos &&
          plan_input_policy != plan_advice_scope_policy_vocabulary.end() &&
          plan_input_policy->surface.find("PLAN_INPUT") != std::string::npos &&
          plan_input_policy->source_relation.find("symbolic") !=
              std::string::npos &&
          plan_input_policy->forbidden_claim.find("contract identity") !=
              std::string::npos &&
          attempt_budget_policy != plan_advice_scope_policy_vocabulary.end() &&
          attempt_budget_policy->attempt_budget_guard &&
          attempt_budget_policy->forbidden_claim.find("runtime attempt") !=
              std::string::npos &&
          runtime_executor_boundary !=
              plan_advice_scope_policy_vocabulary.end() &&
          !runtime_executor_boundary->advice_only &&
          runtime_executor_boundary->scheduler_authority &&
          runtime_executor_boundary->execution_boundary.find(
              "Runtime Hero is the only executor") != std::string::npos,
      "plan advice scope policy should keep plan_basis and suggested waves "
      "advisory while preserving Runtime Hero as the executor boundary");
  const auto operational_scope_vocabulary =
      target::lattice_operational_readiness_v1_scope_vocabulary();
  const auto operational_scope_summary =
      target::lattice_operational_readiness_v1_scope_summary();
  const auto find_operational_scope = [&](const std::string &item) {
    return std::find_if(operational_scope_vocabulary.begin(),
                        operational_scope_vocabulary.end(),
                        [&](const auto &entry) { return entry.item == item; });
  };
  const auto read_only_scope =
      find_operational_scope("read_only_evidence_authority");
  const auto train_core_scope = find_operational_scope("train_core_readiness");
  const auto validation_scope =
      find_operational_scope("validation_evaluation_readiness");
  const auto db_scope = find_operational_scope("db_index");
  const auto performance_scope = find_operational_scope("performance_targets");
  const auto checkpoint_identity_scope =
      find_operational_scope("stable_checkpoint_identity");
  const auto source_receipt_scope =
      find_operational_scope("structured_source_receipt_facts");
  const auto selection_event_scope =
      find_operational_scope("first_class_selection_signal_events");
  const auto vicreg_node_support_scope =
      find_operational_scope("vicreg_node_support_facts");
  check(operational_scope_vocabulary.size() == 12 &&
            operational_scope_summary.schema ==
                "kikijyeba.lattice.operational_readiness_v1.scope_summary."
                "v1" &&
            operational_scope_summary.item_count == 12 &&
            operational_scope_summary.included_in_v1_count == 6 &&
            operational_scope_summary.deferred_beyond_v1_count == 6 &&
            operational_scope_summary.included_and_deferred_count == 0 &&
            operational_scope_summary.read_only_evidence_authority_count == 6 &&
            operational_scope_summary.runtime_executor_authority_count == 0 &&
            operational_scope_summary.performance_gate_authority_count == 0 &&
            operational_scope_summary.db_source_of_truth_authority_count == 0 &&
            operational_scope_summary.empty_claim_field_count == 0 &&
            operational_scope_summary.included_deferred_partition_complete &&
            operational_scope_summary.included_items_are_read_only &&
            operational_scope_summary.no_runtime_executor_authority &&
            operational_scope_summary.no_performance_gate_authority &&
            operational_scope_summary.no_db_source_of_truth_authority &&
            operational_scope_summary.all_scope_items_have_claim_text &&
            operational_scope_summary.summary_self_check_passed &&
            operational_scope_summary.summary_issue_count == 0 &&
            operational_scope_summary.summary_issues.empty() &&
            std::count_if(operational_scope_vocabulary.begin(),
                          operational_scope_vocabulary.end(),
                          [](const auto &entry) {
                            return entry.included_in_v1 &&
                                   !entry.deferred_beyond_v1;
                          }) == 6 &&
            std::count_if(operational_scope_vocabulary.begin(),
                          operational_scope_vocabulary.end(),
                          [](const auto &entry) {
                            return entry.deferred_beyond_v1 &&
                                   !entry.included_in_v1;
                          }) == 6 &&
            std::none_of(operational_scope_vocabulary.begin(),
                         operational_scope_vocabulary.end(),
                         [](const auto &entry) {
                           return entry.runtime_executor_authority ||
                                  entry.performance_gate_authority ||
                                  entry.db_source_of_truth_authority;
                         }) &&
            read_only_scope != operational_scope_vocabulary.end() &&
            read_only_scope->read_only_evidence_authority &&
            read_only_scope->excluded_claim.find("scheduler") !=
                std::string::npos &&
            train_core_scope != operational_scope_vocabulary.end() &&
            train_core_scope->included_in_v1 &&
            validation_scope != operational_scope_vocabulary.end() &&
            validation_scope->excluded_claim.find("model quality") !=
                std::string::npos &&
            db_scope != operational_scope_vocabulary.end() &&
            db_scope->deferred_beyond_v1 &&
            performance_scope != operational_scope_vocabulary.end() &&
            performance_scope->deferred_beyond_v1 &&
            checkpoint_identity_scope != operational_scope_vocabulary.end() &&
            checkpoint_identity_scope->deferred_beyond_v1 &&
            source_receipt_scope != operational_scope_vocabulary.end() &&
            source_receipt_scope->deferred_beyond_v1 &&
            selection_event_scope != operational_scope_vocabulary.end() &&
            selection_event_scope->deferred_beyond_v1 &&
            vicreg_node_support_scope != operational_scope_vocabulary.end() &&
            vicreg_node_support_scope->deferred_beyond_v1,
        "Operational Readiness V1 scope vocabulary should expose included "
        "evidence-authority claims and explicitly deferred V2 work");
  const auto operational_gate_vocabulary =
      target::lattice_operational_readiness_v1_gate_vocabulary();
  const auto operational_gate_summary =
      target::lattice_operational_readiness_v1_gate_summary();
  const auto find_operational_gate = [&](const std::string &gate) {
    return std::find_if(operational_gate_vocabulary.begin(),
                        operational_gate_vocabulary.end(),
                        [&](const auto &entry) { return entry.gate == gate; });
  };
  const auto read_only_gate =
      find_operational_gate("read_only_authority_boundary");
  const auto identity_gate = find_operational_gate("active_identity_binding");
  const auto vicreg_gate =
      find_operational_gate("legacy_node_vicreg_train_core_ready");
  const auto mdn_gate = find_operational_gate("node_mdn_train_core_ready");
  const auto validation_leakage_gate =
      find_operational_gate("train_core_no_validation_leakage");
  const auto test_leakage_gate =
      find_operational_gate("train_core_no_test_leakage");
  const auto validation_eval_gate =
      find_operational_gate("validation_eval_exact_checkpoint_binding");
  const auto warnings_gate =
      find_operational_gate("warnings_visibility_nonblocking");
  const auto node_support_gate =
      find_operational_gate("mdn_node_support_visibility");
  const auto hero_surface_gate =
      find_operational_gate("proof_receipt_and_hero_surface");
  check(
      operational_gate_vocabulary.size() == 10 &&
          operational_gate_summary.schema ==
              "kikijyeba.lattice.operational_readiness_v1.gate_summary.v1" &&
          operational_gate_summary.gate_count == 10 &&
          operational_gate_summary.required_for_v1_count == 10 &&
          operational_gate_summary.read_only_evidence_authority_count == 10 &&
          operational_gate_summary.runtime_executor_authority_count == 0 &&
          operational_gate_summary.performance_gate_authority_count == 0 &&
          operational_gate_summary.db_source_of_truth_authority_count == 0 &&
          operational_gate_summary.empty_hero_surface_count == 0 &&
          operational_gate_summary.empty_pass_condition_count == 0 &&
          operational_gate_summary.empty_fail_condition_count == 0 &&
          operational_gate_summary.unique_target_id_count == 5 &&
          operational_gate_summary.all_gates_required_for_v1 &&
          operational_gate_summary.all_gates_read_only &&
          operational_gate_summary.no_runtime_executor_authority &&
          operational_gate_summary.no_performance_gate_authority &&
          operational_gate_summary.no_db_source_of_truth_authority &&
          operational_gate_summary.all_gates_have_hero_surface &&
          operational_gate_summary.all_gates_have_pass_fail_conditions &&
          operational_gate_summary.all_v1_targets_referenced &&
          operational_gate_summary.missing_v1_target_ids.empty() &&
          operational_gate_summary.summary_self_check_passed &&
          operational_gate_summary.summary_issue_count == 0 &&
          operational_gate_summary.summary_issues.empty() &&
          std::all_of(operational_gate_vocabulary.begin(),
                      operational_gate_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.required_for_v1 &&
                               entry.read_only_evidence_authority &&
                               !entry.runtime_executor_authority &&
                               !entry.performance_gate_authority &&
                               !entry.db_source_of_truth_authority;
                      }) &&
          read_only_gate != operational_gate_vocabulary.end() &&
          read_only_gate->fail_condition.find("dispatches waves") !=
              std::string::npos &&
          std::find(read_only_gate->hero_surfaces.begin(),
                    read_only_gate->hero_surfaces.end(),
                    "deficit_vector_planning_summary") !=
              read_only_gate->hero_surfaces.end() &&
          identity_gate != operational_gate_vocabulary.end() &&
          std::find(identity_gate->target_ids.begin(),
                    identity_gate->target_ids.end(),
                    "node_mdn_validation_eval_ready") !=
              identity_gate->target_ids.end() &&
          identity_gate->v1_boundary.find("checkpoint paths") !=
              std::string::npos &&
          vicreg_gate != operational_gate_vocabulary.end() &&
          vicreg_gate->pass_condition.find("satisfied") != std::string::npos &&
          std::find(vicreg_gate->hero_surfaces.begin(),
                    vicreg_gate->hero_surfaces.end(),
                    "representation_geometry_summary") !=
              vicreg_gate->hero_surfaces.end() &&
          mdn_gate != operational_gate_vocabulary.end() &&
          mdn_gate->required_evidence.find("exact representation checkpoint") !=
              std::string::npos &&
          validation_leakage_gate != operational_gate_vocabulary.end() &&
          validation_leakage_gate->pass_condition.find("validation") !=
              std::string::npos &&
          test_leakage_gate != operational_gate_vocabulary.end() &&
          test_leakage_gate->pass_condition.find("selection_signal") !=
              std::string::npos &&
          std::find(test_leakage_gate->hero_surfaces.begin(),
                    test_leakage_gate->hero_surfaces.end(),
                    "selection_signal_policy_summary") !=
              test_leakage_gate->hero_surfaces.end() &&
          validation_eval_gate != operational_gate_vocabulary.end() &&
          validation_eval_gate->fail_condition.find("wrong MDN checkpoint") !=
              std::string::npos &&
          validation_eval_gate->v1_boundary.find("not model quality") !=
              std::string::npos &&
          std::find(validation_eval_gate->hero_surfaces.begin(),
                    validation_eval_gate->hero_surfaces.end(),
                    "checkpoint_selection_policy_summary") !=
              validation_eval_gate->hero_surfaces.end() &&
          std::find(validation_eval_gate->hero_surfaces.begin(),
                    validation_eval_gate->hero_surfaces.end(),
                    "validation_performance_evidence_policy_summary") !=
              validation_eval_gate->hero_surfaces.end() &&
          std::find(validation_eval_gate->hero_surfaces.begin(),
                    validation_eval_gate->hero_surfaces.end(),
                    "validation_performance_scope_policy_summary") !=
              validation_eval_gate->hero_surfaces.end() &&
          warnings_gate != operational_gate_vocabulary.end() &&
          warnings_gate->pass_condition.find("target status") !=
              std::string::npos &&
          std::find(warnings_gate->hero_surfaces.begin(),
                    warnings_gate->hero_surfaces.end(),
                    "warning_anchor_scope_policy_vocabulary") !=
              warnings_gate->hero_surfaces.end() &&
          std::find(warnings_gate->hero_surfaces.begin(),
                    warnings_gate->hero_surfaces.end(),
                    "validation_performance_evidence_policy_summary") !=
              warnings_gate->hero_surfaces.end() &&
          std::find(warnings_gate->hero_surfaces.begin(),
                    warnings_gate->hero_surfaces.end(),
                    "performance_uncertainty_policy_summary") !=
              warnings_gate->hero_surfaces.end() &&
          std::find(warnings_gate->hero_surfaces.begin(),
                    warnings_gate->hero_surfaces.end(),
                    "mdn_distribution_calibration_summary") !=
              warnings_gate->hero_surfaces.end() &&
          std::find(warnings_gate->hero_surfaces.begin(),
                    warnings_gate->hero_surfaces.end(),
                    "mdn_distribution_calibration_diagnostic_summary") !=
              warnings_gate->hero_surfaces.end() &&
          node_support_gate != operational_gate_vocabulary.end() &&
          node_support_gate->v1_boundary.find("MDN-head visibility") !=
              std::string::npos &&
          std::find(node_support_gate->hero_surfaces.begin(),
                    node_support_gate->hero_surfaces.end(),
                    "node_support_scope_policy_summary") !=
              node_support_gate->hero_surfaces.end() &&
          std::find(node_support_gate->hero_surfaces.begin(),
                    node_support_gate->hero_surfaces.end(),
                    "valid_target_uncertainty_summary") !=
              node_support_gate->hero_surfaces.end() &&
          hero_surface_gate != operational_gate_vocabulary.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "proof_certificate_digest_policy_vocabulary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "proof_certificate_digest_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "deficit_vector_planning_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "checkpoint_selection_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "operational_readiness_v1_scope_vocabulary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "operational_readiness_v1_gate_vocabulary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "operational_readiness_v1_gate_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.mathematical_readiness_v1_vocabulary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "proof_certificate_digest_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.deficit_vector_planning_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.node_support_scope_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.selection_signal_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.source_key_coordinate_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.source_receipt_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.valid_target_uncertainty_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "validation_performance_evidence_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target."
                    "validation_performance_scope_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.performance_uncertainty_policy_summary") !=
              hero_surface_gate->hero_surfaces.end() &&
          std::find(hero_surface_gate->hero_surfaces.begin(),
                    hero_surface_gate->hero_surfaces.end(),
                    "explain_target.mdn_distribution_calibration_summary") !=
              hero_surface_gate->hero_surfaces.end(),
      "Operational Readiness V1 gate vocabulary should expose the concrete "
      "acceptance gates and keep each gate read-only, non-performance, and "
      "non-DB-backed");
  const auto contract_identity_boundary_vocabulary =
      target::lattice_contract_identity_boundary_vocabulary();
  const auto contract_identity_boundary_summary =
      target::lattice_contract_identity_boundary_summary();
  const auto find_identity_boundary = [&](const std::string &surface) {
    return std::find_if(
        contract_identity_boundary_vocabulary.begin(),
        contract_identity_boundary_vocabulary.end(),
        [&](const auto &entry) { return entry.surface == surface; });
  };
  const auto protocol_boundary =
      find_identity_boundary("protocol_contract_fingerprint");
  const auto graph_cursor_boundary =
      find_identity_boundary("graph_order_fingerprint|source_cursor_token");
  const auto target_spec_boundary =
      find_identity_boundary("target_spec_fingerprint");
  const auto checkpoint_model_state_boundary =
      find_identity_boundary("checkpoint_paths_and_loaded_model_state");
  const auto plan_boundary =
      find_identity_boundary("plan_inputs_and_wave_knobs");
  const auto warning_boundary =
      find_identity_boundary("warning_clauses_and_results");
  const auto source_audit_boundary =
      find_identity_boundary("source_receipts_and_key_windows");
  check(
      contract_identity_boundary_vocabulary.size() == 8 &&
          contract_identity_boundary_summary.schema ==
              "kikijyeba.lattice.contract_identity_boundary.summary.v1" &&
          contract_identity_boundary_summary.boundary_count == 8 &&
          contract_identity_boundary_summary.protocol_contract_identity_count ==
              1 &&
          contract_identity_boundary_summary.target_spec_identity_count == 1 &&
          contract_identity_boundary_summary.active_runtime_identity_count ==
              3 &&
          contract_identity_boundary_summary.runtime_model_state_count == 1 &&
          contract_identity_boundary_summary.plan_advice_count == 1 &&
          contract_identity_boundary_summary.warning_visibility_count == 1 &&
          contract_identity_boundary_summary.audit_metadata_count == 1 &&
          contract_identity_boundary_summary
                  .runtime_model_state_identity_overlap_count == 0 &&
          contract_identity_boundary_summary
                  .plan_advice_identity_overlap_count == 0 &&
          contract_identity_boundary_summary.warning_identity_overlap_count ==
              0 &&
          contract_identity_boundary_summary.audit_identity_overlap_count ==
              0 &&
          contract_identity_boundary_summary.empty_policy_field_count == 0 &&
          contract_identity_boundary_summary
              .model_state_is_not_contract_identity &&
          contract_identity_boundary_summary
              .advice_warning_audit_not_identity &&
          contract_identity_boundary_summary.active_identity_surfaces_present &&
          contract_identity_boundary_summary.proof_identity_surfaces_present &&
          contract_identity_boundary_summary.all_boundaries_have_policy_text &&
          contract_identity_boundary_summary.missing_required_surfaces
              .empty() &&
          contract_identity_boundary_summary.summary_self_check_passed &&
          contract_identity_boundary_summary.summary_issue_count == 0 &&
          contract_identity_boundary_summary.summary_issues.empty() &&
          protocol_boundary != contract_identity_boundary_vocabulary.end() &&
          protocol_boundary->protocol_contract_identity &&
          protocol_boundary->active_runtime_identity &&
          protocol_boundary->excluded_from.find("model-state") !=
              std::string::npos &&
          graph_cursor_boundary !=
              contract_identity_boundary_vocabulary.end() &&
          graph_cursor_boundary->active_runtime_identity &&
          graph_cursor_boundary->proof_authority.find("coverage and leakage") !=
              std::string::npos &&
          target_spec_boundary != contract_identity_boundary_vocabulary.end() &&
          target_spec_boundary->target_spec_identity &&
          target_spec_boundary->excluded_from.find("plan") !=
              std::string::npos &&
          target_spec_boundary->excluded_from.find("warning") !=
              std::string::npos &&
          checkpoint_model_state_boundary !=
              contract_identity_boundary_vocabulary.end() &&
          checkpoint_model_state_boundary->runtime_model_state &&
          !checkpoint_model_state_boundary->protocol_contract_identity &&
          !checkpoint_model_state_boundary->target_spec_identity &&
          checkpoint_model_state_boundary->excluded_from.find(
              "protocol contract") != std::string::npos &&
          plan_boundary != contract_identity_boundary_vocabulary.end() &&
          plan_boundary->plan_advice &&
          !plan_boundary->protocol_contract_identity &&
          !plan_boundary->target_spec_identity &&
          plan_boundary->excluded_from.find("target_spec_fingerprint") !=
              std::string::npos &&
          warning_boundary != contract_identity_boundary_vocabulary.end() &&
          warning_boundary->warning_visibility &&
          warning_boundary->excluded_from.find("target status") !=
              std::string::npos &&
          source_audit_boundary !=
              contract_identity_boundary_vocabulary.end() &&
          source_audit_boundary->audit_metadata &&
          source_audit_boundary->excluded_from.find("row-index coverage") !=
              std::string::npos &&
          std::none_of(contract_identity_boundary_vocabulary.begin(),
                       contract_identity_boundary_vocabulary.end(),
                       [](const auto &entry) {
                         return entry.runtime_model_state &&
                                (entry.protocol_contract_identity ||
                                 entry.target_spec_identity);
                       }),
      "contract identity boundary vocabulary should separate active identity "
      "and target proof identity from runtime model-state, advice, warnings, "
      "and audit metadata");
  const auto mathematical_readiness_vocabulary =
      target::lattice_mathematical_readiness_v1_vocabulary();
  const auto mathematical_readiness_summary =
      target::lattice_mathematical_readiness_v1_summary();
  const auto find_math_readiness = [&](const std::string &item) {
    return std::find_if(mathematical_readiness_vocabulary.begin(),
                        mathematical_readiness_vocabulary.end(),
                        [&](const auto &entry) { return entry.item == item; });
  };
  const auto math_surface_present = [&](const auto readiness,
                                        const std::string &surface) {
    return readiness != mathematical_readiness_vocabulary.end() &&
           std::find(readiness->hero_surfaces.begin(),
                     readiness->hero_surfaces.end(),
                     surface) != readiness->hero_surfaces.end();
  };
  const auto partial_order_readiness =
      find_math_readiness("evidence_partial_order");
  const auto coverage_load_readiness =
      find_math_readiness("coverage_load_dual_algebra");
  const auto leakage_readiness =
      find_math_readiness("protected_split_dilation");
  const auto causal_graph_readiness =
      find_math_readiness("checkpoint_causal_graph");
  const auto certificate_readiness = find_math_readiness("proof_certificates");
  const auto abstraction_readiness =
      find_math_readiness("abstract_interpretation");
  const auto node_support_readiness =
      find_math_readiness("node_support_matrix");
  const auto representation_readiness =
      find_math_readiness("representation_geometry");
  const auto uncertainty_readiness =
      find_math_readiness("performance_uncertainty");
  const auto mdn_calibration_readiness =
      find_math_readiness("mdn_distribution_calibration");
  const auto deficit_readiness = find_math_readiness("deficit_vector_planning");
  const auto pareto_readiness = find_math_readiness("pareto_evidence_order");
  const auto datalog_readiness = find_math_readiness("datalog_derived_queries");
  const auto source_key_readiness =
      find_math_readiness("source_key_coordinate_map");
  const auto contract_boundary_readiness =
      find_math_readiness("contract_identity_boundary");
  const auto dsl_dimension_readiness =
      find_math_readiness("dsl_dimensional_analysis");
  check(
      mathematical_readiness_vocabulary.size() == 16 &&
          mathematical_readiness_summary.schema ==
              "kikijyeba.lattice.mathematical_readiness_v1.summary.v1" &&
          mathematical_readiness_summary.item_count == 16 &&
          mathematical_readiness_summary.included_in_v1_count == 16 &&
          mathematical_readiness_summary.deferred_beyond_v1_count == 0 &&
          mathematical_readiness_summary.read_only_count == 16 &&
          mathematical_readiness_summary.runtime_executor_count == 0 &&
          mathematical_readiness_summary.performance_gate_count == 0 &&
          mathematical_readiness_summary.db_source_of_truth_count == 0 &&
          mathematical_readiness_summary.empty_hero_surface_count == 0 &&
          mathematical_readiness_summary.all_items_included_in_v1 &&
          mathematical_readiness_summary.no_items_deferred_beyond_v1 &&
          mathematical_readiness_summary.all_items_read_only &&
          mathematical_readiness_summary.no_runtime_executor &&
          mathematical_readiness_summary.no_performance_gate &&
          mathematical_readiness_summary.no_db_source_of_truth &&
          mathematical_readiness_summary.summary_self_check_passed &&
          mathematical_readiness_summary.summary_issue_count == 0 &&
          mathematical_readiness_summary.summary_issues.empty() &&
          std::all_of(mathematical_readiness_vocabulary.begin(),
                      mathematical_readiness_vocabulary.end(),
                      [](const auto &entry) {
                        return entry.included_in_v1 &&
                               !entry.deferred_beyond_v1 && entry.read_only &&
                               !entry.runtime_executor &&
                               !entry.performance_gate &&
                               !entry.db_source_of_truth;
                      }) &&
          partial_order_readiness != mathematical_readiness_vocabulary.end() &&
          std::find(partial_order_readiness->hero_surfaces.begin(),
                    partial_order_readiness->hero_surfaces.end(),
                    "join_law_vocabulary") !=
              partial_order_readiness->hero_surfaces.end() &&
          math_surface_present(
              partial_order_readiness,
              "explain_target.product_evidence_state_vocabulary") &&
          math_surface_present(
              partial_order_readiness,
              "explain_target.monotonicity_invariant_vocabulary") &&
          math_surface_present(partial_order_readiness,
                               "explain_target.join_law_vocabulary") &&
          coverage_load_readiness != mathematical_readiness_vocabulary.end() &&
          coverage_load_readiness->mathematical_claim.find("idempotent") !=
              std::string::npos &&
          coverage_load_readiness->mathematical_claim.find("additive") !=
              std::string::npos &&
          math_surface_present(
              coverage_load_readiness,
              "explain_target.exposure_measure_algebra_vocabulary") &&
          math_surface_present(coverage_load_readiness,
                               "exposure_measure_algebra_summary") &&
          math_surface_present(
              coverage_load_readiness,
              "explain_target.exposure_measure_algebra_summary") &&
          leakage_readiness != mathematical_readiness_vocabulary.end() &&
          math_surface_present(leakage_readiness,
                               "explain_target.leakage_rule_vocabulary") &&
          math_surface_present(leakage_readiness, "leakage_rule_summary") &&
          math_surface_present(leakage_readiness,
                               "explain_target.leakage_rule_summary") &&
          causal_graph_readiness != mathematical_readiness_vocabulary.end() &&
          math_surface_present(causal_graph_readiness,
                               "explain_target.causal_edge_vocabulary") &&
          math_surface_present(causal_graph_readiness, "causal_edge_summary") &&
          math_surface_present(causal_graph_readiness,
                               "explain_target.causal_edge_summary") &&
          math_surface_present(causal_graph_readiness,
                               "selection_signal_policy_summary") &&
          math_surface_present(
              causal_graph_readiness,
              "explain_target.selection_signal_policy_summary") &&
          math_surface_present(
              causal_graph_readiness,
              "explain_target.checkpoint_identity_policy_vocabulary") &&
          certificate_readiness != mathematical_readiness_vocabulary.end() &&
          certificate_readiness->proof_basis.find("schema") !=
              std::string::npos &&
          std::find(certificate_readiness->hero_surfaces.begin(),
                    certificate_readiness->hero_surfaces.end(),
                    "explain_target.proof_obligation_vocabulary") !=
              certificate_readiness->hero_surfaces.end() &&
          std::find(certificate_readiness->hero_surfaces.begin(),
                    certificate_readiness->hero_surfaces.end(),
                    "explain_target."
                    "proof_certificate_digest_policy_vocabulary") !=
              certificate_readiness->hero_surfaces.end() &&
          std::find(certificate_readiness->hero_surfaces.begin(),
                    certificate_readiness->hero_surfaces.end(),
                    "proof_certificate_digest_policy_summary") !=
              certificate_readiness->hero_surfaces.end() &&
          std::find(certificate_readiness->hero_surfaces.begin(),
                    certificate_readiness->hero_surfaces.end(),
                    "explain_target."
                    "proof_certificate_digest_policy_summary") !=
              certificate_readiness->hero_surfaces.end() &&
          abstraction_readiness != mathematical_readiness_vocabulary.end() &&
          math_surface_present(
              abstraction_readiness,
              "explain_target.evidence_abstraction_vocabulary") &&
          math_surface_present(abstraction_readiness,
                               "explain_target.db_cache_policy_vocabulary") &&
          node_support_readiness != mathematical_readiness_vocabulary.end() &&
          std::find(node_support_readiness->hero_surfaces.begin(),
                    node_support_readiness->hero_surfaces.end(),
                    "warning_anchor_scope_policy_vocabulary") !=
              node_support_readiness->hero_surfaces.end() &&
          math_surface_present(
              node_support_readiness,
              "explain_target.node_support_scope_policy_vocabulary") &&
          math_surface_present(node_support_readiness,
                               "node_support_scope_policy_summary") &&
          math_surface_present(
              node_support_readiness,
              "explain_target.node_support_scope_policy_summary") &&
          math_surface_present(
              node_support_readiness,
              "explain_target.warning_anchor_scope_policy_vocabulary") &&
          math_surface_present(node_support_readiness,
                               "explain_target.join_law_vocabulary") &&
          representation_readiness != mathematical_readiness_vocabulary.end() &&
          math_surface_present(
              representation_readiness,
              "explain_target.representation_geometry_vocabulary") &&
          math_surface_present(representation_readiness,
                               "representation_geometry_summary") &&
          math_surface_present(
              representation_readiness,
              "explain_target.representation_geometry_summary") &&
          uncertainty_readiness != mathematical_readiness_vocabulary.end() &&
          uncertainty_readiness->future_work.find("selection-leakage") !=
              std::string::npos &&
          math_surface_present(
              uncertainty_readiness,
              "explain_target.valid_target_uncertainty_vocabulary") &&
          math_surface_present(uncertainty_readiness,
                               "valid_target_uncertainty_summary") &&
          math_surface_present(
              uncertainty_readiness,
              "explain_target.valid_target_uncertainty_summary") &&
          math_surface_present(
              uncertainty_readiness,
              "explain_target.performance_uncertainty_policy_vocabulary") &&
          math_surface_present(uncertainty_readiness,
                               "performance_uncertainty_policy_summary") &&
          math_surface_present(
              uncertainty_readiness,
              "explain_target.performance_uncertainty_policy_summary") &&
          math_surface_present(
              uncertainty_readiness,
              "validation_performance_evidence_policy_summary") &&
          math_surface_present(uncertainty_readiness,
                               "explain_target.validation_performance_evidence_"
                               "policy_summary") &&
          math_surface_present(uncertainty_readiness,
                               "validation_performance_scope_policy_summary") &&
          math_surface_present(
              uncertainty_readiness,
              "explain_target.validation_performance_scope_policy_summary") &&
          mdn_calibration_readiness !=
              mathematical_readiness_vocabulary.end() &&
          math_surface_present(
              mdn_calibration_readiness,
              "explain_target.mdn_distribution_calibration_vocabulary") &&
          math_surface_present(mdn_calibration_readiness,
                               "mdn_distribution_calibration_summary") &&
          math_surface_present(
              mdn_calibration_readiness,
              "explain_target.mdn_distribution_calibration_summary") &&
          math_surface_present(
              mdn_calibration_readiness,
              "mdn_distribution_calibration_diagnostic_vocabulary") &&
          math_surface_present(
              mdn_calibration_readiness,
              "explain_target."
              "mdn_distribution_calibration_diagnostic_vocabulary") &&
          math_surface_present(
              mdn_calibration_readiness,
              "mdn_distribution_calibration_diagnostic_summary") &&
          math_surface_present(
              mdn_calibration_readiness,
              "explain_target."
              "mdn_distribution_calibration_diagnostic_summary") &&
          deficit_readiness != mathematical_readiness_vocabulary.end() &&
          std::find(deficit_readiness->hero_surfaces.begin(),
                    deficit_readiness->hero_surfaces.end(),
                    "explain_target.plan_advice_scope_policy_vocabulary") !=
              deficit_readiness->hero_surfaces.end() &&
          math_surface_present(deficit_readiness,
                               "explain_target.deficit_priority_vocabulary") &&
          math_surface_present(deficit_readiness,
                               "deficit_vector_planning_summary") &&
          math_surface_present(
              deficit_readiness,
              "explain_target.deficit_vector_planning_summary") &&
          pareto_readiness != mathematical_readiness_vocabulary.end() &&
          pareto_readiness->boundary.find("latest_satisfying") !=
              std::string::npos &&
          math_surface_present(
              pareto_readiness,
              "explain_target.evidence_order_dimension_vocabulary") &&
          math_surface_present(
              pareto_readiness,
              "explain_target.evidence_order_relation_vocabulary") &&
          std::find(pareto_readiness->hero_surfaces.begin(),
                    pareto_readiness->hero_surfaces.end(),
                    "explain_target."
                    "checkpoint_selection_policy_vocabulary") !=
              pareto_readiness->hero_surfaces.end() &&
          math_surface_present(pareto_readiness,
                               "checkpoint_selection_policy_summary") &&
          math_surface_present(
              pareto_readiness,
              "explain_target.checkpoint_selection_policy_summary") &&
          math_surface_present(contract_boundary_readiness,
                               "checkpoint_selection_policy_summary") &&
          math_surface_present(
              contract_boundary_readiness,
              "explain_target.checkpoint_selection_policy_summary") &&
          datalog_readiness != mathematical_readiness_vocabulary.end() &&
          std::find(datalog_readiness->hero_surfaces.begin(),
                    datalog_readiness->hero_surfaces.end(),
                    "explain_target.derived_query_rule_vocabulary") !=
              datalog_readiness->hero_surfaces.end() &&
          math_surface_present(
              datalog_readiness,
              "explain_target."
              "derived_query_projection_semantics_vocabulary") &&
          std::find(datalog_readiness->hero_surfaces.begin(),
                    datalog_readiness->hero_surfaces.end(),
                    "derived_query_results") !=
              datalog_readiness->hero_surfaces.end() &&
          source_key_readiness != mathematical_readiness_vocabulary.end() &&
          source_key_readiness->boundary.find("row-index") !=
              std::string::npos &&
          math_surface_present(
              source_key_readiness,
              "explain_target.source_key_coordinate_policy_vocabulary") &&
          math_surface_present(source_key_readiness,
                               "source_key_coordinate_policy_summary") &&
          math_surface_present(
              source_key_readiness,
              "explain_target.source_key_coordinate_policy_summary") &&
          math_surface_present(
              source_key_readiness,
              "explain_target.source_receipt_policy_vocabulary") &&
          math_surface_present(source_key_readiness,
                               "source_receipt_policy_summary") &&
          math_surface_present(
              source_key_readiness,
              "explain_target.source_receipt_policy_summary") &&
          dsl_dimension_readiness != mathematical_readiness_vocabulary.end() &&
          std::find(dsl_dimension_readiness->hero_surfaces.begin(),
                    dsl_dimension_readiness->hero_surfaces.end(),
                    "explain_target.target_numeric_dimension_vocabulary") !=
              dsl_dimension_readiness->hero_surfaces.end() &&
          math_surface_present(dsl_dimension_readiness,
                               "target_numeric_dimension_summary") &&
          math_surface_present(
              dsl_dimension_readiness,
              "explain_target.target_numeric_dimension_summary") &&
          contract_boundary_readiness !=
              mathematical_readiness_vocabulary.end() &&
          contract_boundary_readiness->boundary.find("checkpoint paths") !=
              std::string::npos &&
          std::find(contract_boundary_readiness->hero_surfaces.begin(),
                    contract_boundary_readiness->hero_surfaces.end(),
                    "explain_target.contract_identity_boundary_vocabulary") !=
              contract_boundary_readiness->hero_surfaces.end() &&
          math_surface_present(
              contract_boundary_readiness,
              "explain_target.checkpoint_identity_policy_vocabulary") &&
          math_surface_present(
              contract_boundary_readiness,
              "explain_target.checkpoint_selection_policy_vocabulary"),
      "mathematical readiness V1 vocabulary should crosswalk the math "
      "strengthening points to Hero surfaces while keeping V1 read-only");

  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = root;
  options.split_policy = split_policy;
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_ranged";
  options.active_identity.vicreg_assembly_fingerprint = "vicreg_1";
  options.active_identity.mdn_assembly_fingerprint = "mdn_1";

  auto symbolic_plan_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = malformed_plan_input_source;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  REQUIRE_CHECKPOINT_EXISTS = true;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = symbolic_plan_input_receiver;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  PLAN_MODE = run|debug;
  PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:malformed_plan_input_source;
  MAX_WAVES = 1;
};
)DSL");
  for (auto &spec : symbolic_plan_specs) {
    if (spec.target_id == "malformed_plan_input_source") {
      spec.target_spec_fingerprint.clear();
    }
  }
  target::lattice_target_evaluator_t symbolic_plan_eval(symbolic_plan_specs,
                                                        options);
  const auto symbolic_plan =
      symbolic_plan_eval.evaluate("symbolic_plan_input_receiver");
  check(symbolic_plan.status ==
                target::lattice_target_status_t::missing_report &&
            symbolic_plan.plan_ready,
        "plan-input receiver should stay a normal missing-report plan target");
  check(
      symbolic_plan.suggested_wave.input_mdn_checkpoint ==
          "latest_satisfying:malformed_plan_input_source",
      "PLAN_INPUT_MDN_CHECKPOINT should remain a symbolic advisory reference");
  check(symbolic_plan.suggested_wave.input_mdn_checkpoint.find(root.string()) ==
            std::string::npos,
        "symbolic PLAN_INPUT_MDN_CHECKPOINT must not be converted into a "
        "runtime-root checkpoint path");
  check(symbolic_plan.proof_certificate.dependencies.empty() &&
            symbolic_plan.proof_certificate_check.passed,
        "symbolic plan-input hints must not become proof dependencies or "
        "certificate claims");

  target::lattice_target_evaluator_t empty_eval(specs, options);
  const auto missing_rep = empty_eval.evaluate("vicreg_representation_ready");
  check(missing_rep.status == target::lattice_target_status_t::missing_report,
        "missing representation evidence should be missing_report");
  check(missing_rep.plan_ready, "missing representation should be plannable");
  check(missing_rep.suggested_wave.target ==
            "wikimyei.representation.encoding.vicreg",
        "representation plan should target VICReg");
  check(!missing_rep.deficits.empty() &&
            missing_rep.deficits.front().key ==
                "artifact:job_manifest_report" &&
            missing_rep.deficits.front().kind == "artifact" &&
            missing_rep.deficits.front().status == "missing" &&
            missing_rep.deficits.front().priority == 35 &&
            missing_rep.deficits.front().priority_class == "artifact",
        "missing representation evidence should include an artifact deficit");
  check(missing_rep.plan_basis.available &&
            missing_rep.plan_basis.suggested_action ==
                "wikimyei.representation.encoding.vicreg train|debug" &&
            !missing_rep.plan_basis.deficit_keys.empty() &&
            missing_rep.plan_basis.deficit_keys.front() ==
                "artifact:job_manifest_report" &&
            missing_rep.plan_basis.deficit_priority_classes.size() == 1 &&
            missing_rep.plan_basis.deficit_priority_classes.front() ==
                "artifact" &&
            missing_rep.plan_basis.primary_deficit_key ==
                "artifact:job_manifest_report" &&
            missing_rep.plan_basis.primary_deficit_priority == 35 &&
            missing_rep.plan_basis.primary_deficit_priority_class == "artifact",
        "missing representation plan basis explains the suggested wave");
  expect_plan_basis_projects_deficits(
      missing_rep,
      "missing representation plan basis should project proof deficits");
  check(missing_rep.proof_certificate_check.passed &&
            missing_rep.proof_certificate_check.issues.empty(),
        "missing representation certificate should self-check as a "
        "non-claiming proof envelope: " +
            join_strings(missing_rep.proof_certificate_check.issues, "; "));

  const auto blocked_mdn = empty_eval.evaluate("node_mdn_ready");
  check(blocked_mdn.status == target::lattice_target_status_t::blocked,
        "node MDN should block on missing representation target");
  check(blocked_mdn.suggested_wave.target ==
            "wikimyei.representation.encoding.vicreg",
        "blocked node MDN should reuse upstream wave plan");
  check(!blocked_mdn.deficits.empty() &&
            blocked_mdn.deficits.front().key == "dependency:upstream_target" &&
            blocked_mdn.deficits.front().kind == "dependency" &&
            blocked_mdn.deficits.front().priority == 30 &&
            blocked_mdn.deficits.front().priority_class == "dependency",
        "blocked node MDN should include an upstream dependency deficit");
  check(blocked_mdn.plan_basis.available &&
            !blocked_mdn.plan_basis.deficit_keys.empty() &&
            blocked_mdn.plan_basis.deficit_keys.front() ==
                "dependency:upstream_target" &&
            blocked_mdn.plan_basis.deficit_priority_classes.size() == 1 &&
            blocked_mdn.plan_basis.deficit_priority_classes.front() ==
                "dependency" &&
            blocked_mdn.plan_basis.primary_deficit_key ==
                "dependency:upstream_target" &&
            blocked_mdn.plan_basis.primary_deficit_priority == 30 &&
            blocked_mdn.plan_basis.primary_deficit_priority_class ==
                "dependency",
        "blocked node MDN plan basis points at the upstream proof deficit");
  expect_plan_basis_projects_deficits(
      blocked_mdn, "blocked node MDN plan basis should project proof deficits");
  check(blocked_mdn.proof_certificate_check.passed &&
            blocked_mdn.proof_certificate_check.issues.empty(),
        "blocked dependency certificate should self-check as a non-claiming "
        "proof envelope");

  const auto cyclic_dependency_specs = target::decode_lattice_targets_from_dsl(
      R"DSL(
LATTICE_TARGET {
  TARGET_ID = cyclic_dependency_a;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = cyclic_dependency_b;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_DEPENDS {
  TARGET_ID = cyclic_dependency_a;
  UPSTREAM_TARGET_ID = cyclic_dependency_b;
};

LATTICE_DEPENDS {
  TARGET_ID = cyclic_dependency_b;
  UPSTREAM_TARGET_ID = cyclic_dependency_a;
};
)DSL");
  target::lattice_target_evaluator_t cyclic_dependency_eval(
      cyclic_dependency_specs, options);
  const auto cyclic_dependency =
      cyclic_dependency_eval.evaluate("cyclic_dependency_a");
  check(cyclic_dependency.status == target::lattice_target_status_t::blocked,
        "cyclic target dependencies should block evaluation");
  check(std::any_of(cyclic_dependency.deficits.begin(),
                    cyclic_dependency.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "dependency:target_cycle" &&
                             deficit.kind == "dependency" &&
                             deficit.dimension == "target_cycle" &&
                             deficit.status == "blocked" &&
                             deficit.priority == 30 &&
                             deficit.priority_class == "dependency";
                    }),
        "cyclic target dependencies should expose a target-cycle dependency "
        "deficit");
  expect_plan_basis_projects_deficits(
      cyclic_dependency,
      "cyclic target dependencies should project proof deficits");

  const auto missing_dependency_source_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = missing_upstream_consumer;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = absent_upstream_target;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = missing_checkpoint_source_consumer;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  CHECKPOINT_SOURCE = latest_satisfying:absent_checkpoint_source_target;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = missing_evaluated_source_consumer;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:absent_evaluated_source_target;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  target::lattice_target_evaluator_t missing_dependency_source_eval(
      missing_dependency_source_specs, options);
  const auto missing_upstream_dependency =
      missing_dependency_source_eval.evaluate("missing_upstream_consumer");
  check(missing_upstream_dependency.status ==
            target::lattice_target_status_t::blocked,
        "missing upstream target references should block evaluation");
  check(missing_upstream_dependency.proof_certificate_check.passed,
        "missing upstream target references should keep a well-formed "
        "non-claiming proof certificate: " +
            join_strings(
                missing_upstream_dependency.proof_certificate_check.issues,
                "; "));
  check(std::any_of(missing_upstream_dependency.deficits.begin(),
                    missing_upstream_dependency.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "dependency:upstream_target" &&
                             deficit.kind == "dependency" &&
                             deficit.status == "missing";
                    }),
        "missing upstream target references should expose an upstream "
        "dependency deficit");
  expect_plan_basis_projects_deficits(
      missing_upstream_dependency,
      "missing upstream target references should project proof deficits");

  const auto missing_checkpoint_source_dependency =
      missing_dependency_source_eval.evaluate(
          "missing_checkpoint_source_consumer");
  check(missing_checkpoint_source_dependency.status ==
            target::lattice_target_status_t::blocked,
        "missing CHECKPOINT_SOURCE references should block evaluation");
  check(missing_checkpoint_source_dependency.proof_certificate_check.passed,
        "missing CHECKPOINT_SOURCE references should keep a well-formed "
        "non-claiming proof certificate: " +
            join_strings(missing_checkpoint_source_dependency
                             .proof_certificate_check.issues,
                         "; "));
  check(
      std::any_of(missing_checkpoint_source_dependency.deficits.begin(),
                  missing_checkpoint_source_dependency.deficits.end(),
                  [](const auto &deficit) {
                    return deficit.key == "dependency:checkpoint_source" &&
                           deficit.kind == "dependency" &&
                           deficit.status == "missing";
                  }),
      "missing CHECKPOINT_SOURCE references should expose a checkpoint-source "
      "dependency deficit");
  expect_plan_basis_projects_deficits(
      missing_checkpoint_source_dependency,
      "missing CHECKPOINT_SOURCE references should project proof deficits");

  const auto missing_evaluated_source_dependency =
      missing_dependency_source_eval.evaluate(
          "missing_evaluated_source_consumer");
  check(missing_evaluated_source_dependency.status ==
            target::lattice_target_status_t::blocked,
        "missing EVALUATED_CHECKPOINT_SOURCE references should block "
        "evaluation");
  check(missing_evaluated_source_dependency.proof_certificate_check.passed,
        "missing EVALUATED_CHECKPOINT_SOURCE references should keep a "
        "well-formed non-claiming proof certificate: " +
            join_strings(missing_evaluated_source_dependency
                             .proof_certificate_check.issues,
                         "; "));
  check(std::any_of(missing_evaluated_source_dependency.deficits.begin(),
                    missing_evaluated_source_dependency.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "dependency:evaluated_checkpoint_source" &&
                             deficit.kind == "dependency" &&
                             deficit.status == "missing";
                    }),
        "missing EVALUATED_CHECKPOINT_SOURCE references should expose an "
        "evaluated-checkpoint-source dependency deficit");
  expect_plan_basis_projects_deficits(
      missing_evaluated_source_dependency,
      "missing EVALUATED_CHECKPOINT_SOURCE references should project proof "
      "deficits");

  write_representation_job(root, root / "rep.pt");
  target::lattice_target_evaluator_t rep_eval(specs, options);
  const auto rep_ready = rep_eval.evaluate("vicreg_representation_ready");
  check(rep_ready.status == target::lattice_target_status_t::satisfied,
        "representation should be satisfied");

  const auto mdn_missing = rep_eval.evaluate("node_mdn_ready");
  check(mdn_missing.status == target::lattice_target_status_t::missing_report,
        "node MDN should be missing_report before an MDN job exists");
  check(mdn_missing.suggested_wave.target ==
            "wikimyei.inference.expected_value.mdn",
        "node MDN should plan MDN wave once representation is ready");

  write_mdn_job(root, root / "mdn.pt", root / "rep.pt");
  target::lattice_target_evaluator_t full_eval(specs, options);
  const auto mdn_ready = full_eval.evaluate("node_mdn_ready");
  check(mdn_ready.status == target::lattice_target_status_t::satisfied,
        "node MDN should be satisfied, got " +
            std::string(target::lattice_target_status_name(mdn_ready.status)) +
            " certificate_issues=" +
            join_strings(mdn_ready.proof_certificate_check.issues, "; "));
  auto malformed_upstream_specs = specs;
  auto malformed_vicreg_target = std::find_if(
      malformed_upstream_specs.begin(), malformed_upstream_specs.end(),
      [](const target::lattice_target_spec_t &spec) {
        return spec.target_id == "vicreg_representation_ready";
      });
  check(malformed_vicreg_target != malformed_upstream_specs.end(),
        "test specs should include VICReg upstream target");
  malformed_vicreg_target->target_spec_fingerprint.clear();
  target::lattice_target_evaluator_t malformed_upstream_eval(
      malformed_upstream_specs, options);
  const auto mdn_blocked_by_malformed_upstream =
      malformed_upstream_eval.evaluate("node_mdn_ready");
  check(
      mdn_blocked_by_malformed_upstream.status ==
              target::lattice_target_status_t::blocked &&
          mdn_blocked_by_malformed_upstream.proof_certificate_check.passed &&
          std::any_of(mdn_blocked_by_malformed_upstream.deficits.begin(),
                      mdn_blocked_by_malformed_upstream.deficits.end(),
                      [](const auto &deficit) {
                        return deficit.key == "dependency:upstream_target" &&
                               deficit.status == "blocked";
                      }),
      "dependent target should treat malformed upstream proof certificates as "
      "blocked dependencies");

  target::lattice_target_evaluator_options_t no_identity_options{};
  no_identity_options.runtime_root = root;
  target::lattice_target_evaluator_t no_identity_eval(specs,
                                                      no_identity_options);
  const auto blocked_missing_identity =
      no_identity_eval.evaluate("vicreg_representation_ready");
  check(blocked_missing_identity.status ==
            target::lattice_target_status_t::blocked,
        "required active identity should block when not provided");
  check(std::any_of(blocked_missing_identity.deficits.begin(),
                    blocked_missing_identity.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "identity:contract_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "missing" &&
                             deficit.priority == 10 &&
                             deficit.priority_class == "identity";
                    }),
        "missing active contract identity should expose an identity deficit");
  expect_plan_basis_projects_deficits(
      blocked_missing_identity,
      "missing active contract identity should project proof deficits");

  auto no_component_identity_options = options;
  no_component_identity_options.active_identity.vicreg_assembly_fingerprint
      .clear();
  target::lattice_target_evaluator_t no_component_identity_eval(
      specs, no_component_identity_options);
  const auto blocked_missing_component_identity =
      no_component_identity_eval.evaluate("vicreg_representation_ready");
  check(std::any_of(blocked_missing_component_identity.deficits.begin(),
                    blocked_missing_component_identity.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "identity:component_assembly_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "missing";
                    }),
        "missing active component identity should expose an identity deficit");

  const auto graph_anchor_identity_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = graph_anchor_identity_required;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};
)DSL");
  auto no_graph_identity_options = options;
  no_graph_identity_options.active_identity.graph_order_fingerprint.clear();
  target::lattice_target_evaluator_t no_graph_identity_eval(
      graph_anchor_identity_specs, no_graph_identity_options);
  const auto blocked_missing_graph_identity =
      no_graph_identity_eval.evaluate("graph_anchor_identity_required");
  check(std::any_of(blocked_missing_graph_identity.deficits.begin(),
                    blocked_missing_graph_identity.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "identity:graph_order_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "missing";
                    }),
        "missing active graph-order identity should expose an identity "
        "deficit");

  auto no_source_cursor_options = options;
  no_source_cursor_options.active_identity.source_cursor_token.clear();
  target::lattice_target_evaluator_t no_source_cursor_eval(
      graph_anchor_identity_specs, no_source_cursor_options);
  const auto blocked_missing_source_cursor =
      no_source_cursor_eval.evaluate("graph_anchor_identity_required");
  check(std::any_of(blocked_missing_source_cursor.deficits.begin(),
                    blocked_missing_source_cursor.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "identity:source_cursor_token" &&
                             deficit.kind == "identity" &&
                             deficit.status == "missing";
                    }),
        "missing active source-cursor identity should expose an identity "
        "deficit");

  const auto stale_graph_root = make_tmp_dir("stale_graph_runtime_identity");
  const auto stale_graph_dir = stale_graph_root / "stale_graph_representation";
  write_text(stale_graph_dir / "job.manifest",
             "job_id=stale_graph_representation\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "protocol_contract_fingerprint=contract_1\n"
             "graph_order_fingerprint=old_graph\n"
             "source_cursor_token=cursor_ranged\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(stale_graph_dir / "job.state", "status=completed\n");
  auto stale_graph_options = options;
  stale_graph_options.runtime_root = stale_graph_root;
  target::lattice_target_evaluator_t stale_graph_eval(
      graph_anchor_identity_specs, stale_graph_options);
  const auto stale_graph_identity =
      stale_graph_eval.evaluate("graph_anchor_identity_required");
  check(stale_graph_identity.status ==
            target::lattice_target_status_t::stale_contract,
        "stale graph-order runtime evidence should be stale");
  check(std::any_of(stale_graph_identity.deficits.begin(),
                    stale_graph_identity.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "identity:graph_order_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "mismatch" &&
                             deficit.priority == 10 &&
                             deficit.priority_class == "identity";
                    }),
        "stale graph-order runtime evidence should expose an identity deficit");
  expect_plan_basis_projects_deficits(
      stale_graph_identity,
      "stale graph-order runtime evidence should project proof deficits");
  std::filesystem::remove_all(stale_graph_root);

  const auto stale_cursor_root = make_tmp_dir("stale_cursor_runtime_identity");
  const auto stale_cursor_dir =
      stale_cursor_root / "stale_cursor_representation";
  write_text(stale_cursor_dir / "job.manifest",
             "job_id=stale_cursor_representation\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "protocol_contract_fingerprint=contract_1\n"
             "graph_order_fingerprint=graph_1\n"
             "source_cursor_token=old_cursor\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(stale_cursor_dir / "job.state", "status=completed\n");
  auto stale_cursor_options = options;
  stale_cursor_options.runtime_root = stale_cursor_root;
  target::lattice_target_evaluator_t stale_cursor_eval(
      graph_anchor_identity_specs, stale_cursor_options);
  const auto stale_cursor_identity =
      stale_cursor_eval.evaluate("graph_anchor_identity_required");
  check(stale_cursor_identity.status ==
            target::lattice_target_status_t::stale_contract,
        "stale source-cursor runtime evidence should be stale");
  check(std::any_of(stale_cursor_identity.deficits.begin(),
                    stale_cursor_identity.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "identity:source_cursor_token" &&
                             deficit.kind == "identity" &&
                             deficit.status == "mismatch" &&
                             deficit.priority == 10 &&
                             deficit.priority_class == "identity";
                    }),
        "stale source-cursor runtime evidence should expose an identity "
        "deficit");
  expect_plan_basis_projects_deficits(
      stale_cursor_identity,
      "stale source-cursor runtime evidence should project proof deficits");
  std::filesystem::remove_all(stale_cursor_root);

  const auto no_rep_root = make_tmp_dir("no_rep");
  write_representation_job(no_rep_root, no_rep_root / "rep.pt");
  write_mdn_without_representation_checkpoint(no_rep_root,
                                              no_rep_root / "mdn.pt");
  target::lattice_target_evaluator_options_t no_rep_options = options;
  no_rep_options.runtime_root = no_rep_root;
  target::lattice_target_evaluator_t no_rep_eval(specs, no_rep_options);
  const auto no_rep_mdn = no_rep_eval.evaluate("node_mdn_ready");
  check(no_rep_mdn.status ==
            target::lattice_target_status_t::missing_checkpoint,
        "node MDN readiness should require a loaded representation checkpoint");
  check(std::any_of(no_rep_mdn.deficits.begin(), no_rep_mdn.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "model_state:representation_checkpoint" &&
                             deficit.status == "missing";
                    }),
        "node MDN missing representation checkpoint should expose a "
        "model-state deficit");
  std::filesystem::remove_all(no_rep_root);

  const auto candidate_root = make_tmp_dir("candidate_scan");
  write_representation_job(candidate_root, candidate_root / "rep.pt");
  write_bad_representation_job(candidate_root, "newer_bad_representation");
  target::lattice_target_evaluator_options_t candidate_options = options;
  candidate_options.runtime_root = candidate_root;
  target::lattice_target_evaluator_t candidate_eval(specs, candidate_options);
  const auto scan_ready =
      candidate_eval.evaluate("vicreg_representation_ready");
  check(scan_ready.status == target::lattice_target_status_t::satisfied,
        "an older valid candidate should satisfy readiness when the newest "
        "candidate fails");
  std::filesystem::remove_all(candidate_root);

  const auto running_root = make_tmp_dir("running_status");
  write_bad_representation_job(running_root, "running_representation",
                               "running");
  target::lattice_target_evaluator_options_t running_options = options;
  running_options.runtime_root = running_root;
  target::lattice_target_evaluator_t running_eval(specs, running_options);
  const auto running_result =
      running_eval.evaluate("vicreg_representation_ready");
  check(running_result.status == target::lattice_target_status_t::metric_failed,
        "non-completed candidate job status should not satisfy readiness");
  check(std::any_of(running_result.deficits.begin(),
                    running_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:job_status" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "not_completed" &&
                             deficit.priority == 35 &&
                             deficit.priority_class == "artifact";
                    }),
        "non-completed candidate job status should expose an artifact "
        "deficit");
  check(running_result.plan_basis.available &&
            running_result.plan_basis.primary_deficit_key ==
                "artifact:job_status" &&
            running_result.plan_basis.primary_deficit_priority == 35 &&
            running_result.plan_basis.primary_deficit_priority_class ==
                "artifact",
        "non-completed candidate job status plan basis should point at the "
        "artifact deficit");
  expect_plan_basis_projects_deficits(
      running_result,
      "non-completed candidate status should project deficits into plan_basis");
  std::filesystem::remove_all(running_root);

  const auto budget_root = make_tmp_dir("budget");
  const auto budget_specs =
      target::decode_lattice_targets_from_dsl(single_rep_target_dsl(1));
  write_bad_representation_job(budget_root, "bad_representation");
  target::lattice_target_evaluator_options_t budget_options = options;
  budget_options.runtime_root = budget_root;
  target::lattice_target_evaluator_t budget_eval(budget_specs, budget_options);
  const auto budget_result =
      budget_eval.evaluate("vicreg_representation_ready");
  check(budget_result.status ==
            target::lattice_target_status_t::missing_checkpoint,
        "bad representation should be missing checkpoint");
  check(!budget_result.plan_ready, "MAX_WAVES should suppress planning once "
                                   "matching attempts are exhausted");
  check(!budget_result.plan_basis.available &&
            !budget_result.plan_basis.deficit_keys.empty() &&
            budget_result.plan_basis.deficit_keys.front() ==
                "artifact:checkpoint" &&
            budget_result.plan_basis.primary_deficit_priority == 35 &&
            budget_result.plan_basis.primary_deficit_priority_class ==
                "artifact" &&
            budget_result.plan_basis.reason.find("no plan-ready wave") !=
                std::string::npos,
        "plan basis should not advertise an available wave when MAX_WAVES "
        "suppresses planning");
  expect_plan_basis_projects_deficits(
      budget_result,
      "budget-exhausted plan basis should project proof deficits");
  std::filesystem::remove_all(budget_root);

  const auto stale_budget_root = make_tmp_dir("stale_budget");
  const auto stale_dir = stale_budget_root / "stale_representation";
  write_text(stale_dir / "job.manifest",
             "job_id=stale_representation\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_old\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(stale_dir / "job.state", "status=completed\n"
                                      "optimizer_steps=1\n"
                                      "last_loss=0.20\n"
                                      "checkpoint_written=false\n");
  write_text(stale_dir / "representation.report", "optimizer_steps=1\n"
                                                  "mean_loss=0.20\n"
                                                  "checkpoint_written=false\n");
  target::lattice_target_evaluator_options_t stale_budget_options = options;
  stale_budget_options.runtime_root = stale_budget_root;
  target::lattice_target_evaluator_t stale_budget_eval(budget_specs,
                                                       stale_budget_options);
  const auto stale_budget_result =
      stale_budget_eval.evaluate("vicreg_representation_ready");
  check(stale_budget_result.status ==
            target::lattice_target_status_t::stale_contract,
        "stale contract attempts should be reported as stale");
  check(std::any_of(stale_budget_result.deficits.begin(),
                    stale_budget_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "identity:contract_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "mismatch" &&
                             deficit.priority == 10 &&
                             deficit.priority_class == "identity";
                    }),
        "stale contract evidence should expose an identity deficit");
  check(stale_budget_result.plan_basis.available &&
            stale_budget_result.plan_basis.primary_deficit_key ==
                "identity:contract_fingerprint" &&
            stale_budget_result.plan_basis.primary_deficit_priority == 10 &&
            stale_budget_result.plan_basis.primary_deficit_priority_class ==
                "identity",
        "stale contract plan basis should point at the identity deficit");
  expect_plan_basis_projects_deficits(
      stale_budget_result,
      "stale contract evidence should project deficits into plan_basis");
  check(stale_budget_result.plan_ready,
        "MAX_WAVES should ignore stale-contract attempts for active planning "
        "budget");
  std::filesystem::remove_all(stale_budget_root);

  const auto stale_component_root = make_tmp_dir("stale_component");
  const auto stale_component_dir =
      stale_component_root / "stale_component_representation";
  write_text(stale_component_dir / "job.manifest",
             "job_id=stale_component_representation\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=old_vicreg\n");
  write_text(stale_component_dir / "job.state", "status=completed\n"
                                                "optimizer_steps=1\n"
                                                "last_loss=0.20\n"
                                                "checkpoint_written=false\n");
  write_text(stale_component_dir / "representation.report",
             "optimizer_steps=1\n"
             "mean_loss=0.20\n"
             "checkpoint_written=false\n");
  target::lattice_target_evaluator_options_t stale_component_options = options;
  stale_component_options.runtime_root = stale_component_root;
  target::lattice_target_evaluator_t stale_component_eval(
      budget_specs, stale_component_options);
  const auto stale_component_result =
      stale_component_eval.evaluate("vicreg_representation_ready");
  check(stale_component_result.status ==
            target::lattice_target_status_t::stale_component,
        "stale component attempts should be reported as stale");
  check(std::any_of(stale_component_result.deficits.begin(),
                    stale_component_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "identity:component_assembly_fingerprint" &&
                             deficit.kind == "identity" &&
                             deficit.status == "mismatch" &&
                             deficit.priority == 10 &&
                             deficit.priority_class == "identity";
                    }),
        "stale component evidence should expose an identity deficit");
  expect_plan_basis_projects_deficits(
      stale_component_result,
      "stale component evidence should project deficits into plan_basis");
  std::filesystem::remove_all(stale_component_root);

  const auto missing_report_root = make_tmp_dir("missing_component_report");
  const auto missing_report_dir = missing_report_root / "rep_missing_report";
  write_text(missing_report_dir / "job.manifest",
             "job_id=rep_missing_report\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(missing_report_dir / "job.state",
             "status=completed\n"
             "optimizer_steps=2\n"
             "last_loss=0.20\n"
             "checkpoint_written=true\n"
             "checkpoint_path=rep_missing_report.pt\n");
  write_text(missing_report_dir / "rep_missing_report.pt", "checkpoint");
  target::lattice_target_evaluator_options_t missing_report_options = options;
  missing_report_options.runtime_root = missing_report_root;
  target::lattice_target_evaluator_t missing_report_eval(
      specs, missing_report_options);
  const auto missing_report_result =
      missing_report_eval.evaluate("vicreg_representation_ready");
  check(missing_report_result.status ==
            target::lattice_target_status_t::missing_report,
        "candidate without a component report should not satisfy readiness");
  check(std::any_of(missing_report_result.deficits.begin(),
                    missing_report_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:component_report" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing" &&
                             deficit.priority == 35 &&
                             deficit.priority_class == "artifact";
                    }),
        "missing component reports should expose a component-report artifact "
        "deficit");
  expect_plan_basis_projects_deficits(
      missing_report_result,
      "missing component reports should project deficits into plan_basis");
  std::filesystem::remove_all(missing_report_root);

  const auto missing_state_root = make_tmp_dir("missing_state");
  const auto missing_state_dir = missing_state_root / "rep_missing_state";
  write_text(missing_state_dir / "job.manifest",
             "job_id=rep_missing_state\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(missing_state_dir / "representation.report",
             "optimizer_steps=2\n"
             "mean_loss=0.20\n"
             "checkpoint_written=true\n"
             "checkpoint_path=rep_missing_state.pt\n");
  write_text(missing_state_dir / "rep_missing_state.pt", "checkpoint");
  target::lattice_target_evaluator_options_t missing_state_options = options;
  missing_state_options.runtime_root = missing_state_root;
  target::lattice_target_evaluator_t missing_state_eval(specs,
                                                        missing_state_options);
  const auto missing_state_result =
      missing_state_eval.evaluate("vicreg_representation_ready");
  check(missing_state_result.status ==
            target::lattice_target_status_t::metric_failed,
        "candidate without job.state must not satisfy readiness");
  check(std::any_of(missing_state_result.deficits.begin(),
                    missing_state_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:job_state" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing" &&
                             deficit.priority == 35 &&
                             deficit.priority_class == "artifact";
                    }),
        "missing job.state files should expose a job-state artifact deficit");
  expect_plan_basis_projects_deficits(
      missing_state_result,
      "missing job.state files should project deficits into plan_basis");
  std::filesystem::remove_all(missing_state_root);

  const auto low_optimizer_root = make_tmp_dir("low_optimizer_metric");
  const auto low_optimizer_dir = low_optimizer_root / "rep_low_optimizer";
  const auto low_optimizer_checkpoint = low_optimizer_root / "rep_low.pt";
  write_text(low_optimizer_dir / "job.manifest",
             "job_id=rep_low_optimizer\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "graph_order_fingerprint=graph_1\n"
             "source_cursor_token=cursor_ranged\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(low_optimizer_dir / "job.state",
             std::string("status=completed\n"
                         "optimizer_steps=1\n"
                         "last_loss=0.20\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 low_optimizer_checkpoint.string() + "\n");
  write_text(low_optimizer_dir / "representation.report",
             std::string("optimizer_steps=1\n"
                         "mean_loss=0.20\n"
                         "checkpoint_written=true\n"
                         "checkpoint_path=") +
                 low_optimizer_checkpoint.string() + "\n");
  write_text(low_optimizer_checkpoint, "checkpoint");
  target::lattice_target_evaluator_options_t low_optimizer_options = options;
  low_optimizer_options.runtime_root = low_optimizer_root;
  target::lattice_target_evaluator_t low_optimizer_eval(specs,
                                                        low_optimizer_options);
  const auto low_optimizer_result =
      low_optimizer_eval.evaluate("vicreg_representation_ready");
  check(low_optimizer_result.status ==
            target::lattice_target_status_t::metric_failed,
        "candidate below MIN_OPTIMIZER_STEPS must not satisfy readiness");
  check(std::any_of(low_optimizer_result.deficits.begin(),
                    low_optimizer_result.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "metric:optimizer_steps" &&
                             deficit.kind == "metric" &&
                             deficit.status == "below_required" &&
                             deficit.priority == 70 &&
                             deficit.priority_class == "metric" &&
                             deficit.unit == "count" &&
                             std::abs(deficit.actual - 1.0) < 1e-12;
                    }),
        "optimizer effort readiness failure should expose a metric deficit");
  check(low_optimizer_result.plan_basis.available &&
            low_optimizer_result.plan_basis.primary_deficit_key ==
                "metric:optimizer_steps" &&
            low_optimizer_result.plan_basis.primary_deficit_priority == 70 &&
            low_optimizer_result.plan_basis.primary_deficit_priority_class ==
                "metric",
        "optimizer effort failure plan basis should point at the metric "
        "deficit");
  expect_plan_basis_projects_deficits(
      low_optimizer_result,
      "optimizer effort failure should project deficits into plan_basis");
  std::filesystem::remove_all(low_optimizer_root);

  const auto relative_root = make_tmp_dir("relative");
  const auto relative_job_dir = relative_root / "rep_relative";
  write_text(relative_job_dir / "job.manifest",
             "job_id=rep_relative\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(relative_job_dir / "job.state",
             "status=completed\n"
             "optimizer_steps=2\n"
             "last_loss=0.25\n"
             "checkpoint_written=true\n"
             "checkpoint_path=relative_rep.pt\n");
  write_text(relative_job_dir / "representation.report",
             "optimizer_steps=2\n"
             "mean_loss=0.20\n"
             "checkpoint_written=true\n"
             "checkpoint_path=relative_rep.pt\n");
  write_text(relative_job_dir / "relative_rep.pt", "checkpoint");
  target::lattice_target_evaluator_options_t relative_options = options;
  relative_options.runtime_root = relative_root;
  target::lattice_target_evaluator_t relative_eval(specs, relative_options);
  const auto relative_ready =
      relative_eval.evaluate("vicreg_representation_ready");
  check(relative_ready.status == target::lattice_target_status_t::satisfied,
        "relative checkpoint paths should resolve against the job directory");
  std::filesystem::remove_all(relative_root);

  const auto exposure_root = make_tmp_dir("exposure");
  const auto exposure_checkpoint = exposure_root / "rep_ranged.pt";
  write_ranged_representation_job(exposure_root, exposure_checkpoint, 0, 100);
  const auto coverage_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  exposure::lattice_exposure_ledger_t ledger{};
  const auto exposure_fact =
      make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  const auto exposure_fact_digest =
      exposure::exposure_fact_digest(exposure_fact);
  ledger.add(exposure_fact);
  target::lattice_target_evaluator_options_t exposure_options = options;
  exposure_options.runtime_root = exposure_root;
  exposure_options.exposure_ledger = &ledger;
  target::lattice_target_evaluator_t exposure_eval(coverage_specs,
                                                   exposure_options);
  const auto exposure_ready =
      exposure_eval.evaluate("ranged_representation_ready");
  check(exposure_ready.status == target::lattice_target_status_t::satisfied,
        "full observed_input exposure coverage should satisfy target");
  check(exposure_ready.exposure_summaries.size() == 1,
        "exposure target should report one exposure-load summary");
  check(exposure_ready.exposure_summaries.front().use ==
            exposure::exposure_use_t::observed_input,
        "representation summary reports observed input use");
  check(exposure_ready.exposure_summaries.front().unique_covered_anchors == 100,
        "representation summary reports unique coverage");
  check(exposure_ready.exposure_summaries.front().loaded_anchor_events == 100,
        "representation summary reports loaded anchor events");
  check(
      std::abs(exposure_ready.exposure_summaries.front().cursor_exposure_load -
               1.0) < 1e-12,
      "representation summary reports one cursor epoch");
  check(exposure_ready.exposure_summaries.front().unique_coverage_algebra ==
                exposure::k_unique_coverage_algebra &&
            exposure_ready.exposure_summaries.front().load_algebra ==
                exposure::k_load_algebra,
        "target exposure summary carries algebra names for coverage and load");
  check(exposure_ready.proof_certificate.identity.contract_match &&
            exposure_ready.proof_certificate.identity.component_match &&
            exposure_ready.proof_certificate.identity.graph_order_match &&
            exposure_ready.proof_certificate.identity.source_cursor_match,
        "proof certificate records active identity matches");
  check(!exposure_ready.proof_certificate.target_spec_fingerprint.empty() &&
            exposure_ready.proof_certificate.target_spec_fingerprint ==
                coverage_specs.front().target_spec_fingerprint &&
            exposure_ready.proof_certificate.split_policy_fingerprint ==
                split_policy_fingerprint,
        "proof certificate binds to target and split policy fingerprints");
  check(!exposure_ready.proof_certificate.certificate_digest.empty() &&
            exposure_ready.proof_certificate.certificate_digest ==
                target::lattice_target_proof_certificate_digest(
                    exposure_ready.proof_certificate),
        "proof certificate carries a deterministic digest over proof content");
  check(exposure_ready.proof_certificate_check.passed &&
            exposure_ready.proof_certificate_check.issues.empty(),
        "proof certificate passes local self-check");
  auto missing_certificate_identity_specs = coverage_specs;
  missing_certificate_identity_specs.front().target_spec_fingerprint.clear();
  target::lattice_target_evaluator_t missing_certificate_identity_eval(
      missing_certificate_identity_specs, exposure_options);
  const auto missing_certificate_identity_ready =
      missing_certificate_identity_eval.evaluate("ranged_representation_ready");
  check(missing_certificate_identity_ready.status ==
            target::lattice_target_status_t::blocked,
        "a satisfied target with a malformed proof certificate should fail "
        "closed");
  check(!missing_certificate_identity_ready.proof_certificate_check.passed &&
            std::any_of(missing_certificate_identity_ready
                            .proof_certificate_check.issues.begin(),
                        missing_certificate_identity_ready
                            .proof_certificate_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find(
                                     "missing target_spec_fingerprint") !=
                                 std::string::npos;
                        }),
        "malformed proof certificate should report the missing target "
        "fingerprint");
  check(
      !missing_certificate_identity_ready.plan_basis.available &&
          missing_certificate_identity_ready.deficits.size() == 1 &&
          missing_certificate_identity_ready.deficits.front().key ==
              "certificate:proof_certificate_check" &&
          missing_certificate_identity_ready.deficits.front().status ==
              "failed" &&
          missing_certificate_identity_ready.deficits.front().priority == 15 &&
          missing_certificate_identity_ready.deficits.front().priority_class ==
              "certificate" &&
          missing_certificate_identity_ready.deficits.front().message ==
              "proof certificate self-check failed" &&
          missing_certificate_identity_ready.plan_basis.deficit_keys.size() ==
              1 &&
          missing_certificate_identity_ready.plan_basis.deficit_keys.front() ==
              "certificate:proof_certificate_check" &&
          missing_certificate_identity_ready.evidence_order_vector
                  .blocking_deficit_count == 1 &&
          !missing_certificate_identity_ready.evidence_order_vector
               .proof_certificate_check_passed,
      "malformed proof certificate should not advertise a runtime wave and "
      "should project a certificate deficit");
  expect_plan_basis_projects_deficits(
      missing_certificate_identity_ready,
      "malformed proof certificate should project deficits into plan_basis");
  check(
      exposure_ready.evidence_order_vector.order == "pareto_partial_order" &&
          exposure_ready.evidence_order_vector.available &&
          exposure_ready.evidence_order_vector.satisfied &&
          exposure_ready.evidence_order_vector.proof_certificate_check_passed &&
          exposure_ready.evidence_order_vector.identity_contract_match &&
          exposure_ready.evidence_order_vector.identity_component_match &&
          exposure_ready.evidence_order_vector.identity_graph_order_match &&
          exposure_ready.evidence_order_vector.identity_source_cursor_match &&
          std::abs(exposure_ready.evidence_order_vector
                       .min_unique_coverage_fraction -
                   1.0) < 1e-12 &&
          std::abs(
              exposure_ready.evidence_order_vector.max_cursor_exposure_load -
              1.0) < 1e-12 &&
          exposure_ready.evidence_order_vector.closure_checked &&
          exposure_ready.evidence_order_vector.closure_complete &&
          !exposure_ready.evidence_order_vector.leakage_checked &&
          exposure_ready.evidence_order_vector.leakage_passed &&
          !exposure_ready.evidence_order_vector
               .selection_signal_leakage_found &&
          exposure_ready.evidence_order_vector.source_key_audit_count == 1 &&
          exposure_ready.evidence_order_vector.source_key_audit_issue_count ==
              0 &&
          exposure_ready.evidence_order_vector
                  .source_key_affine_inconsistent_count == 0 &&
          exposure_ready.evidence_order_vector.blocking_deficit_count == 0 &&
          exposure_ready.evidence_order_vector.unresolved_lineage_count == 0 &&
          exposure_ready.evidence_order_vector.warning_result_count == 0 &&
          exposure_ready.evidence_order_vector.triggered_warning_count == 0 &&
          exposure_ready.evidence_order_vector.unavailable_warning_count == 0 &&
          exposure_ready.evidence_order_vector.warning_count == 0,
      "evidence order vector exposes Pareto dimensions without a scalar "
      "score");
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector,
            exposure_ready.evidence_order_vector) ==
                target::lattice_evidence_order_relation_t::equivalent &&
            std::string(target::lattice_evidence_order_relation_name(
                target::lattice_evidence_order_relation_t::equivalent)) ==
                "equivalent",
        "evidence order comparator reports identical vectors as equivalent");
  auto unavailable_order_vector = exposure_ready.evidence_order_vector;
  unavailable_order_vector.available = false;
  check(target::compare_lattice_evidence_order_vectors(
            unavailable_order_vector, exposure_ready.evidence_order_vector) ==
            target::lattice_evidence_order_relation_t::unavailable,
        "evidence order comparator reports unavailable vectors explicitly");
  auto selector_leaked_order_vector = exposure_ready.evidence_order_vector;
  selector_leaked_order_vector.selection_signal_leakage_found = true;
  check(
      target::compare_lattice_evidence_order_vectors(
          selector_leaked_order_vector, exposure_ready.evidence_order_vector) ==
              target::lattice_evidence_order_relation_t::selector_leaked &&
          std::string(target::lattice_evidence_order_relation_name(
              target::lattice_evidence_order_relation_t::selector_leaked)) ==
              "selector_leaked",
      "evidence order comparator excludes selector-leaked evidence before "
      "dominance");
  auto lower_coverage_vector = exposure_ready.evidence_order_vector;
  lower_coverage_vector.min_unique_coverage_fraction = 0.75;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, lower_coverage_vector) ==
                target::lattice_evidence_order_relation_t::left_dominates &&
            target::compare_lattice_evidence_order_vectors(
                lower_coverage_vector, exposure_ready.evidence_order_vector) ==
                target::lattice_evidence_order_relation_t::right_dominates,
        "evidence order comparator respects higher coverage dominance");
  auto warning_tradeoff_vector = exposure_ready.evidence_order_vector;
  warning_tradeoff_vector.min_unique_coverage_fraction = 0.90;
  auto coverage_tradeoff_vector = exposure_ready.evidence_order_vector;
  coverage_tradeoff_vector.warning_result_count = 1;
  coverage_tradeoff_vector.triggered_warning_count = 1;
  coverage_tradeoff_vector.warning_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            warning_tradeoff_vector, coverage_tradeoff_vector) ==
            target::lattice_evidence_order_relation_t::incomparable,
        "evidence order comparator preserves Pareto incomparability");
  auto high_load_warning_vector = exposure_ready.evidence_order_vector;
  high_load_warning_vector.max_cursor_exposure_load = 2.0;
  high_load_warning_vector.warning_result_count = 1;
  high_load_warning_vector.triggered_warning_count = 1;
  high_load_warning_vector.warning_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            high_load_warning_vector, exposure_ready.evidence_order_vector) ==
            target::lattice_evidence_order_relation_t::incomparable,
        "evidence order comparator treats higher repeated load with more "
        "warnings as incomparable");
  auto unavailable_warning_vector = exposure_ready.evidence_order_vector;
  unavailable_warning_vector.warning_result_count = 1;
  unavailable_warning_vector.unavailable_warning_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, unavailable_warning_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator treats unavailable warning measurements as "
        "a weaker warning surface");
  auto stale_warning_alias_vector = exposure_ready.evidence_order_vector;
  stale_warning_alias_vector.triggered_warning_count = 1;
  stale_warning_alias_vector.warning_count = 0;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, stale_warning_alias_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator uses structured triggered_warning_count "
        "rather than the legacy warning_count alias");
  auto visibility_only_vector = exposure_ready.evidence_order_vector;
  visibility_only_vector.source_key_audit_count += 7;
  visibility_only_vector.warning_result_count += 3;
  visibility_only_vector.warning_count += 3;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, visibility_only_vector) ==
            target::lattice_evidence_order_relation_t::equivalent,
        "evidence order comparator ignores visibility-only dimensions and the "
        "legacy warning_count alias for dominance");
  auto source_key_issue_vector = exposure_ready.evidence_order_vector;
  source_key_issue_vector.source_key_audit_issue_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, source_key_issue_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator treats source-key audit issues as a weaker "
        "audit surface");
  auto affine_issue_vector = exposure_ready.evidence_order_vector;
  affine_issue_vector.source_key_affine_inconsistent_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, affine_issue_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator treats affine source-key mismatches as a "
        "weaker audit surface");
  auto balanced_node_support_vector = exposure_ready.evidence_order_vector;
  balanced_node_support_vector.max_node_support_coefficient_of_variation = 0.1;
  balanced_node_support_vector.max_node_support_gini = 0.05;
  auto imbalanced_node_support_vector = exposure_ready.evidence_order_vector;
  imbalanced_node_support_vector.max_node_support_coefficient_of_variation =
      0.4;
  imbalanced_node_support_vector.max_node_support_gini = 0.2;
  check(target::compare_lattice_evidence_order_vectors(
            balanced_node_support_vector, imbalanced_node_support_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator treats lower node-support imbalance as "
        "stronger");
  auto unresolved_lineage_vector = exposure_ready.evidence_order_vector;
  unresolved_lineage_vector.unresolved_lineage_count = 1;
  check(target::compare_lattice_evidence_order_vectors(
            exposure_ready.evidence_order_vector, unresolved_lineage_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "evidence order comparator treats unresolved lineage as weaker");
  auto mismatched_digest_certificate = exposure_ready.proof_certificate;
  mismatched_digest_certificate.certificate_digest = "not_the_certificate";
  check(!target::verify_lattice_target_proof_certificate(
             mismatched_digest_certificate)
             .passed,
        "proof certificate self-check rejects mismatched certificate digests");
  auto missing_digest_certificate = exposure_ready.proof_certificate;
  missing_digest_certificate.certificate_digest.clear();
  check(!target::verify_lattice_target_proof_certificate(
             missing_digest_certificate)
             .passed,
        "proof certificate self-check rejects missing certificate digests");
  auto bad_coverage_algebra_certificate = exposure_ready.proof_certificate;
  bad_coverage_algebra_certificate.coverage.front().algebra =
      exposure::k_load_algebra;
  bad_coverage_algebra_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          bad_coverage_algebra_certificate);
  check(!target::verify_lattice_target_proof_certificate(
             bad_coverage_algebra_certificate)
             .passed,
        "proof certificate self-check rejects non-idempotent coverage algebra");
  auto bad_load_algebra_certificate = exposure_ready.proof_certificate;
  bad_load_algebra_certificate.coverage.front().load_summary.load_algebra =
      exposure::k_unique_coverage_algebra;
  bad_load_algebra_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          bad_load_algebra_certificate);
  check(!target::verify_lattice_target_proof_certificate(
             bad_load_algebra_certificate)
             .passed,
        "proof certificate self-check rejects non-additive load algebra");
  auto dependency_order_certificate = exposure_ready.proof_certificate;
  target::lattice_target_proof_certificate_t::dependency_proof_t dep_a{};
  dep_a.kind = "upstream_target";
  dep_a.source_target_id = "a_ready";
  dep_a.status = "satisfied";
  target::lattice_target_proof_certificate_t::dependency_proof_t dep_b{};
  dep_b.kind = "checkpoint_source";
  dep_b.source_target_id = "b_ready";
  dep_b.status = "blocked";
  dep_b.expected_mdn_checkpoint = exposure_checkpoint;
  dependency_order_certificate.dependencies = {dep_a, dep_b};
  const auto dependency_order_digest =
      target::lattice_target_proof_certificate_digest(
          dependency_order_certificate);
  std::reverse(dependency_order_certificate.dependencies.begin(),
               dependency_order_certificate.dependencies.end());
  check(dependency_order_digest ==
            target::lattice_target_proof_certificate_digest(
                dependency_order_certificate),
        "proof certificate digest is canonical across dependency order");
  auto coverage_order_certificate = exposure_ready.proof_certificate;
  auto evaluation_coverage = coverage_order_certificate.coverage.front();
  evaluation_coverage.use = "evaluation_metric";
  evaluation_coverage.require_mutated_component = false;
  evaluation_coverage.load_summary.use =
      exposure::exposure_use_t::evaluation_metric;
  evaluation_coverage.load_summary.require_mutated_component = false;
  coverage_order_certificate.coverage.push_back(evaluation_coverage);
  const auto coverage_order_digest =
      target::lattice_target_proof_certificate_digest(
          coverage_order_certificate);
  std::reverse(coverage_order_certificate.coverage.begin(),
               coverage_order_certificate.coverage.end());
  check(coverage_order_digest ==
            target::lattice_target_proof_certificate_digest(
                coverage_order_certificate),
        "proof certificate digest is canonical across coverage proof order");
  auto tampered_certificate = exposure_ready.proof_certificate;
  tampered_certificate.certificate_digest.clear();
  ++tampered_certificate.coverage.front().covered_anchors;
  check(
      !target::verify_lattice_target_proof_certificate(tampered_certificate)
           .passed,
      "proof certificate self-check rejects inconsistent coverage arithmetic");
  auto tampered_duplicate_coverage_certificate =
      exposure_ready.proof_certificate;
  tampered_duplicate_coverage_certificate.certificate_digest.clear();
  tampered_duplicate_coverage_certificate.coverage.push_back(
      tampered_duplicate_coverage_certificate.coverage.front());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_duplicate_coverage_certificate)
             .passed,
        "proof certificate self-check rejects duplicate coverage obligations");
  auto tampered_coverage_algebra_certificate = exposure_ready.proof_certificate;
  tampered_coverage_algebra_certificate.certificate_digest.clear();
  tampered_coverage_algebra_certificate.coverage.front().algebra =
      "additive_interval_measure";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_algebra_certificate)
             .passed,
        "proof certificate self-check rejects non-idempotent coverage "
        "algebras");
  auto tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front().load_summary.cursor_exposure_load =
      0.5;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent exposure-load "
        "arithmetic");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.unique_coverage_fraction = 0.5;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent exposure unique "
        "coverage fractions");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.optimizer_steps_per_unique_anchor += 1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent optimizer effort "
        "density per unique anchor");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.optimizer_steps_per_cursor_epoch += 1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent optimizer effort "
        "density per cursor epoch");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  ++tampered_load_certificate.coverage.front().load_summary.target_range.begin;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects exposure-load summaries over a "
        "different target range");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front().load_summary.load_unit =
      "coverage_fraction";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent exposure-load "
        "algebra or units");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.valid_target_cursor_epochs =
      std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects non-finite exposure-load "
        "measures");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.valid_target_cursor_epochs =
      tampered_load_certificate.coverage.front()
          .load_summary.cursor_exposure_load +
      1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects valid-target cursor epochs above "
        "exposure load");
  tampered_load_certificate = exposure_ready.proof_certificate;
  tampered_load_certificate.certificate_digest.clear();
  tampered_load_certificate.coverage.front()
      .load_summary.optimizer_steps_per_unique_anchor = -1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_certificate)
             .passed,
        "proof certificate self-check rejects negative exposure-load "
        "measures");
  auto tampered_load_intervals_certificate = exposure_ready.proof_certificate;
  tampered_load_intervals_certificate.certificate_digest.clear();
  ++tampered_load_intervals_certificate.coverage.front()
        .load_summary.loaded_anchor_events;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_intervals_certificate)
             .passed,
        "proof certificate self-check rejects exposure-load anchor events that "
        "do not match contributing interval measure");
  tampered_load_intervals_certificate = exposure_ready.proof_certificate;
  tampered_load_intervals_certificate.certificate_digest.clear();
  --tampered_load_intervals_certificate.coverage.front()
        .load_summary.unique_covered_anchors;
  auto &tampered_unique_load =
      tampered_load_intervals_certificate.coverage.front().load_summary;
  tampered_unique_load.unique_coverage_fraction =
      static_cast<double>(tampered_unique_load.unique_covered_anchors) /
      static_cast<double>(tampered_unique_load.target_range.length());
  tampered_unique_load.optimizer_steps_per_unique_anchor =
      static_cast<double>(tampered_unique_load.optimizer_steps_total) /
      static_cast<double>(tampered_unique_load.unique_covered_anchors);
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_intervals_certificate)
             .passed,
        "proof certificate self-check rejects exposure-load unique coverage "
        "counts that disagree with coverage proof");
  tampered_load_intervals_certificate = exposure_ready.proof_certificate;
  tampered_load_intervals_certificate.certificate_digest.clear();
  tampered_load_intervals_certificate.coverage.front().load_summary.fact_count =
      2;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_intervals_certificate)
             .passed,
        "proof certificate self-check rejects exposure-load fact counts that "
        "do not match contributing intervals");
  auto tampered_load_filter_certificate = exposure_ready.proof_certificate;
  tampered_load_filter_certificate.certificate_digest.clear();
  tampered_load_filter_certificate.coverage.front().load_summary.use =
      exposure::exposure_use_t::target_supervision;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_filter_certificate)
             .passed,
        "proof certificate self-check rejects coverage/load exposure-use "
        "mismatches");
  tampered_load_filter_certificate = exposure_ready.proof_certificate;
  tampered_load_filter_certificate.certificate_digest.clear();
  tampered_load_filter_certificate.coverage.front()
      .load_summary.require_mutated_component = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_filter_certificate)
             .passed,
        "proof certificate self-check rejects coverage/load mutation filter "
        "mismatch");
  tampered_load_filter_certificate = exposure_ready.proof_certificate;
  tampered_load_filter_certificate.certificate_digest.clear();
  tampered_load_filter_certificate.coverage.front()
      .load_summary.component_scope = "node_component";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_filter_certificate)
             .passed,
        "proof certificate self-check rejects unknown load component scopes");
  tampered_load_filter_certificate = exposure_ready.proof_certificate;
  tampered_load_filter_certificate.certificate_digest.clear();
  tampered_load_filter_certificate.coverage.front()
      .load_summary.component_scope = "all_components";
  tampered_load_filter_certificate.coverage.front()
      .load_summary.target_component.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_filter_certificate)
             .passed,
        "proof certificate self-check rejects coverage proofs that broaden "
        "load scope beyond the target component");
  tampered_load_filter_certificate = exposure_ready.proof_certificate;
  tampered_load_filter_certificate.certificate_digest.clear();
  tampered_load_filter_certificate.coverage.front()
      .load_summary.target_component.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_load_filter_certificate)
             .passed,
        "proof certificate self-check rejects target-component load summaries "
        "without a target component");
  auto tampered_coverage_causal_certificate = exposure_ready.proof_certificate;
  tampered_coverage_causal_certificate.certificate_digest.clear();
  tampered_coverage_causal_certificate.closure.causal_exposures.front()
      .completed_anchor_range =
      exposure::anchor_interval_t{.begin = 10, .end = 100};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_causal_certificate)
             .passed,
        "proof certificate self-check rejects coverage intervals without "
        "causal closure support");
  tampered_coverage_causal_certificate = exposure_ready.proof_certificate;
  tampered_coverage_causal_certificate.certificate_digest.clear();
  tampered_coverage_causal_certificate.closure.causal_exposures.front()
      .component_assembly_fingerprint = "old_vicreg_assembly";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_causal_certificate)
             .passed,
        "proof certificate self-check rejects target-component coverage "
        "support from a stale component assembly");
  auto tampered_causal_multiplicity_certificate =
      exposure_ready.proof_certificate;
  tampered_causal_multiplicity_certificate.certificate_digest.clear();
  const auto duplicate_contribution =
      tampered_causal_multiplicity_certificate.coverage.front()
          .contributing_intervals.front();
  tampered_causal_multiplicity_certificate.coverage.front()
      .contributing_intervals.push_back(duplicate_contribution);
  auto &tampered_causal_multiplicity_load =
      tampered_causal_multiplicity_certificate.coverage.front().load_summary;
  tampered_causal_multiplicity_load.loaded_anchor_events +=
      duplicate_contribution.length();
  tampered_causal_multiplicity_load.fact_count += 1;
  tampered_causal_multiplicity_load.cursor_exposure_load =
      static_cast<double>(
          tampered_causal_multiplicity_load.loaded_anchor_events) /
      static_cast<double>(
          tampered_causal_multiplicity_load.target_range.length());
  tampered_causal_multiplicity_load.optimizer_steps_per_cursor_epoch =
      static_cast<double>(
          tampered_causal_multiplicity_load.optimizer_steps_total) /
      tampered_causal_multiplicity_load.cursor_exposure_load;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_multiplicity_certificate)
             .passed,
        "proof certificate self-check rejects repeated exposure load without "
        "matching causal exposure multiplicity");
  auto tampered_no_trials_certificate = exposure_ready.proof_certificate;
  tampered_no_trials_certificate.certificate_digest.clear();
  tampered_no_trials_certificate.coverage.front()
      .load_summary.valid_target_fraction_estimate = 0.0;
  tampered_no_trials_certificate.coverage.front()
      .load_summary.valid_target_wilson_lower_95 = 0.0;
  tampered_no_trials_certificate.coverage.front()
      .load_summary.valid_target_wilson_upper_95 = 0.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_no_trials_certificate)
             .passed,
        "proof certificate self-check rejects finite valid-target uncertainty "
        "estimates without trials");
  auto tampered_identity_certificate = exposure_ready.proof_certificate;
  tampered_identity_certificate.certificate_digest.clear();
  tampered_identity_certificate.identity.source_cursor_match = true;
  tampered_identity_certificate.identity.evidence_source_cursor_token = "wrong";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_identity_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent identity match "
        "booleans");
  tampered_identity_certificate = exposure_ready.proof_certificate;
  tampered_identity_certificate.certificate_digest.clear();
  tampered_identity_certificate.identity.active_contract_fingerprint.clear();
  tampered_identity_certificate.identity.evidence_contract_fingerprint.clear();
  tampered_identity_certificate.identity.contract_match = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_identity_certificate)
             .passed,
        "proof certificate self-check rejects vacuous required contract "
        "identity matches");
  tampered_identity_certificate = exposure_ready.proof_certificate;
  tampered_identity_certificate.certificate_digest.clear();
  tampered_identity_certificate.identity.expected_component_fingerprint.clear();
  tampered_identity_certificate.identity.evidence_component_fingerprint.clear();
  tampered_identity_certificate.identity.component_match = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_identity_certificate)
             .passed,
        "proof certificate self-check rejects vacuous required component "
        "identity matches");
  tampered_identity_certificate = exposure_ready.proof_certificate;
  tampered_identity_certificate.certificate_digest.clear();
  tampered_identity_certificate.identity.active_graph_order_fingerprint.clear();
  tampered_identity_certificate.identity.evidence_graph_order_fingerprint
      .clear();
  tampered_identity_certificate.identity.graph_order_match = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_identity_certificate)
             .passed,
        "proof certificate self-check rejects vacuous required graph-order "
        "identity matches");
  tampered_identity_certificate = exposure_ready.proof_certificate;
  tampered_identity_certificate.certificate_digest.clear();
  tampered_identity_certificate.identity.active_source_cursor_token.clear();
  tampered_identity_certificate.identity.evidence_source_cursor_token.clear();
  tampered_identity_certificate.identity.source_cursor_match = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_identity_certificate)
             .passed,
        "proof certificate self-check rejects vacuous required source-cursor "
        "identity matches");
  auto tampered_envelope_certificate = exposure_ready.proof_certificate;
  tampered_envelope_certificate.certificate_digest.clear();
  tampered_envelope_certificate.schema =
      "kikijyeba.lattice.target_certificate.v0";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_envelope_certificate)
             .passed,
        "proof certificate self-check rejects an unknown certificate schema");
  tampered_envelope_certificate = exposure_ready.proof_certificate;
  tampered_envelope_certificate.certificate_digest.clear();
  tampered_envelope_certificate.target_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_envelope_certificate)
             .passed,
        "proof certificate self-check rejects missing target ids");
  tampered_envelope_certificate = exposure_ready.proof_certificate;
  tampered_envelope_certificate.certificate_digest.clear();
  tampered_envelope_certificate.target_spec_fingerprint.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_envelope_certificate)
             .passed,
        "proof certificate self-check rejects missing target fingerprints");
  auto tampered_coverage_dimension_certificate =
      exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  tampered_coverage_dimension_certificate.coverage.front().use = "not_a_use";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects unknown coverage uses");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  tampered_coverage_dimension_certificate.coverage.front().required_fraction =
      1.5;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects out-of-range coverage "
        "fractions");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  tampered_coverage_dimension_certificate.coverage.front().required_fraction =
      0.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects coverage proofs without a "
        "positive requirement threshold");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  tampered_coverage_dimension_certificate.coverage.front().actual_fraction =
      0.99;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects coverage actual fractions that "
        "disagree with interval measure");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  --tampered_coverage_dimension_certificate.coverage.front().covered_anchors;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects covered-anchor counts that "
        "disagree with covered intervals");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  ++tampered_coverage_dimension_certificate.coverage.front().missing_anchors;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects missing-anchor counts that "
        "disagree with missing intervals");
  tampered_coverage_dimension_certificate = exposure_ready.proof_certificate;
  tampered_coverage_dimension_certificate.certificate_digest.clear();
  tampered_coverage_dimension_certificate.coverage.front().passed = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_dimension_certificate)
             .passed,
        "proof certificate self-check rejects coverage passed flags that "
        "disagree with measured coverage");
  auto tampered_vacuous_coverage_certificate = exposure_ready.proof_certificate;
  tampered_vacuous_coverage_certificate.certificate_digest.clear();
  auto &vacuous_coverage =
      tampered_vacuous_coverage_certificate.coverage.front();
  vacuous_coverage.target_range = {};
  vacuous_coverage.contributing_intervals.clear();
  vacuous_coverage.contributing_fact_digests.clear();
  vacuous_coverage.covered_intervals.clear();
  vacuous_coverage.missing_intervals.clear();
  vacuous_coverage.covered_anchors = 0;
  vacuous_coverage.missing_anchors = 0;
  vacuous_coverage.actual_fraction = 1.0;
  vacuous_coverage.passed = true;
  auto &vacuous_load = vacuous_coverage.load_summary;
  vacuous_load.target_range = {};
  vacuous_load.unique_covered_anchors = 0;
  vacuous_load.unique_coverage_fraction = 0.0;
  vacuous_load.loaded_anchor_events = 0;
  vacuous_load.cursor_exposure_load = 0.0;
  vacuous_load.fact_count = 0;
  vacuous_load.optimizer_steps_total = 0;
  vacuous_load.optimizer_steps_per_unique_anchor = 0.0;
  vacuous_load.optimizer_steps_per_cursor_epoch = 0.0;
  vacuous_load.valid_target_count_total = 0;
  vacuous_load.valid_target_cursor_epochs = 0.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_vacuous_coverage_certificate)
             .passed,
        "proof certificate self-check rejects vacuous coverage proofs without "
        "a target cursor range");
  auto tampered_coverage_union_certificate = exposure_ready.proof_certificate;
  tampered_coverage_union_certificate.certificate_digest.clear();
  tampered_coverage_union_certificate.coverage.front().covered_intervals = {
      exposure::anchor_interval_t{.begin = 0, .end = 50},
      exposure::anchor_interval_t{.begin = 25, .end = 75},
      exposure::anchor_interval_t{.begin = 75, .end = 100},
  };
  tampered_coverage_union_certificate.coverage.front().covered_anchors = 125;
  tampered_coverage_union_certificate.coverage.front().actual_fraction = 1.25;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_union_certificate)
             .passed,
        "proof certificate self-check rejects non-canonical covered interval "
        "unions");
  auto tampered_coverage_complement_certificate =
      exposure_ready.proof_certificate;
  tampered_coverage_complement_certificate.certificate_digest.clear();
  tampered_coverage_complement_certificate.coverage.front().missing_intervals =
      {exposure::anchor_interval_t{.begin = 90, .end = 100}};
  tampered_coverage_complement_certificate.coverage.front().missing_anchors =
      10;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_complement_certificate)
             .passed,
        "proof certificate self-check rejects missing intervals that are not "
        "the covered-range complement");
  check(exposure_ready.proof_certificate.coverage.size() == 1 &&
            exposure_ready.proof_certificate.coverage.front().use ==
                "observed_input" &&
            exposure_ready.proof_certificate.coverage.front().algebra ==
                "idempotent_interval_union" &&
            exposure_ready.proof_certificate.coverage.front().passed,
        "proof certificate records observed-input coverage proof");
  check(exposure_ready.proof_certificate.coverage.front()
                    .contributing_intervals.size() == 1 &&
            exposure_ready.proof_certificate.coverage.front()
                    .contributing_fact_digests.size() == 1 &&
            exposure_ready.proof_certificate.coverage.front()
                    .contributing_fact_digests.front() ==
                exposure_fact_digest &&
            exposure_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            exposure::exposure_fact_digest(
                exposure_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()) == exposure_fact_digest &&
            exposure_ready.proof_certificate.coverage.front()
                    .covered_intervals.size() == 1 &&
            exposure_ready.proof_certificate.coverage.front()
                    .covered_intervals.front()
                    .begin == 0 &&
            exposure_ready.proof_certificate.coverage.front()
                    .covered_intervals.front()
                    .end == 100,
        "coverage proof records the contributing and merged union intervals");
  auto tampered_coverage_digest_certificate = exposure_ready.proof_certificate;
  tampered_coverage_digest_certificate.certificate_digest.clear();
  tampered_coverage_digest_certificate.coverage.front()
      .contributing_fact_digests.front() = "not_in_closure";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_digest_certificate)
             .passed,
        "proof certificate self-check rejects coverage fact digests that do "
        "not bind to closure causal evidence");
  tampered_coverage_digest_certificate = exposure_ready.proof_certificate;
  tampered_coverage_digest_certificate.certificate_digest.clear();
  tampered_coverage_digest_certificate.coverage.front()
      .contributing_fact_digests.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_coverage_digest_certificate)
             .passed,
        "proof certificate self-check rejects coverage proofs without "
        "contributing fact digests");
  check(exposure_ready.proof_certificate.coverage.front()
                .missing_intervals.empty() &&
            exposure_ready.proof_certificate.coverage.front().missing_anchors ==
                0,
        "full coverage proof records no missing intervals");
  check(exposure_ready.proof_certificate.closure.checked &&
            exposure_ready.proof_certificate.closure.complete &&
            exposure_ready.proof_certificate.closure.fact_count == 1,
        "proof certificate records complete checkpoint closure proof");
  check(exposure_ready.proof_certificate.closure.causal_exposures.size() == 1 &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                    .target_component ==
                "wikimyei.representation.encoding.vicreg" &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                    .uses.size() == 1 &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                    .uses.front() == "observed_input" &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .mutated_component,
        "closure proof records labeled causal exposure details");
  check(exposure_ready.proof_certificate.closure.causal_exposures.front()
                    .source_key_footprint_precision ==
                "graph_anchor_key_window_v1" &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.available &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.complete &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.numeric &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.internally_monotone &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.order_preserving &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.affine_step_available &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                    .source_key_window_audit.reference_key_step == 10 &&
            exposure_ready.proof_certificate.closure.causal_exposures.front()
                .source_key_window_audit.affine_consistent,
        "closure proof records source-key order-preserving affine map audit");
  auto tampered_source_key_certificate = exposure_ready.proof_certificate;
  tampered_source_key_certificate.certificate_digest.clear();
  tampered_source_key_certificate.closure.causal_exposures.front()
      .source_key_window_audit.order_preserving = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_source_key_certificate)
             .passed,
        "proof certificate self-check rejects tampered source-key audit "
        "booleans");
  tampered_source_key_certificate = exposure_ready.proof_certificate;
  tampered_source_key_certificate.certificate_digest.clear();
  tampered_source_key_certificate.closure.causal_exposures.front()
      .source_key_window_audit.reference_key_step = 11;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_source_key_certificate)
             .passed,
        "proof certificate self-check rejects tampered source-key affine step");
  tampered_source_key_certificate = exposure_ready.proof_certificate;
  tampered_source_key_certificate.certificate_digest.clear();
  tampered_source_key_certificate.closure.causal_exposures.front()
      .observed_source_key_begin = "not_numeric";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_source_key_certificate)
             .passed,
        "proof certificate self-check rejects source-key audit mismatches "
        "against causal source-key windows");
  auto tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .fact_digest.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "fact digests");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().fact_digest =
      "wrong";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects mismatched causal exposure "
        "digests");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().job_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "runtime job identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .target_component.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "target components");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().uses.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "exposure uses");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().uses.push_back(
      tampered_causal_certificate.closure.causal_exposures.front()
          .uses.front());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects duplicate causal exposure uses");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().uses.push_back(
      "unknown_use");
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects unknown causal exposure uses");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().cursor_domain =
      "source_row";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposures outside the "
        "graph-anchor cursor domain");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .contract_fingerprint = "wrong_contract";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposures outside the "
        "active contract identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .graph_order_fingerprint = "wrong_graph";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposures outside the "
        "active graph-order identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .source_cursor_token = "wrong_cursor";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposures outside the "
        "active source-cursor identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .split_policy_fingerprint = "wrong_split_policy";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposures outside the "
        "active split-policy identity");
  auto missing_split_policy_fact_certificate = exposure_ready.proof_certificate;
  auto &missing_split_policy_fact =
      missing_split_policy_fact_certificate.coverage.front()
          .contributing_facts.front();
  missing_split_policy_fact.split_name = "train_core";
  missing_split_policy_fact.split_policy_fingerprint.clear();
  missing_split_policy_fact_certificate.coverage.front()
      .contributing_fact_digests.front() =
      exposure::exposure_fact_digest(missing_split_policy_fact);
  missing_split_policy_fact_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          missing_split_policy_fact_certificate);
  check(!target::verify_lattice_target_proof_certificate(
             missing_split_policy_fact_certificate)
             .passed,
        "proof certificate self-check rejects concrete split facts without "
        "split-policy identity");
  auto missing_split_policy_causal_certificate =
      exposure_ready.proof_certificate;
  auto &missing_split_policy_causal =
      missing_split_policy_causal_certificate.closure.causal_exposures.front();
  missing_split_policy_causal.split_name = "train_core";
  missing_split_policy_causal.split_policy_fingerprint.clear();
  missing_split_policy_causal_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          missing_split_policy_causal_certificate);
  check(!target::verify_lattice_target_proof_certificate(
             missing_split_policy_causal_certificate)
             .passed,
        "proof certificate self-check rejects concrete split causal exposures "
        "without split-policy identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .component_assembly_fingerprint.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "component assembly identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .observed_footprint = {};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal observed-input facts with "
        "empty source footprints");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .anchor_range = {};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts with empty "
        "anchor ranges");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .completed_anchor_range = {};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts with empty "
        "completed ranges");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .completed_anchor_range.end =
      tampered_causal_certificate.closure.causal_exposures.front()
          .anchor_range.end +
      1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal completed ranges outside "
        "anchor ranges");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().wave_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "runtime wave identity");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .wave_action.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "runtime wave action");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front().job_status =
      "failed";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects non-completed causal producer "
        "facts");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .input_checkpoints = {std::filesystem::path{}};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts with empty "
        "input checkpoint labels");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  const auto duplicate_input_checkpoint =
      tampered_causal_certificate.closure.checkpoint_path.parent_path() /
      "upstream.pt";
  tampered_causal_certificate.closure.causal_exposures.front()
      .input_checkpoints = {duplicate_input_checkpoint,
                            duplicate_input_checkpoint};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects duplicate causal input "
        "checkpoint labels");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .input_checkpoints = {
      tampered_causal_certificate.closure.causal_exposures.front()
          .output_checkpoint};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal checkpoint self loops");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .input_checkpoints = {
      tampered_causal_certificate.closure.checkpoint_path.parent_path() /
      "dangling-input.pt"};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal input checkpoint edges "
        "without a producer or unresolved witness");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .output_checkpoint.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .input_checkpoints = {
      tampered_causal_certificate.closure.checkpoint_path.parent_path() /
      "upstream.pt"};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects causal exposure facts without "
        "output checkpoint labels");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .output_checkpoint =
      tampered_causal_certificate.closure.checkpoint_path.parent_path() /
      "wrong-root.pt";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects closures without a causal root "
        "checkpoint producer");
  tampered_causal_certificate = exposure_ready.proof_certificate;
  tampered_causal_certificate.certificate_digest.clear();
  tampered_causal_certificate.closure.causal_exposures.front()
      .component_assembly_fingerprint = "old_vicreg_assembly";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_causal_certificate)
             .passed,
        "proof certificate self-check rejects root checkpoint producers from "
        "a stale component assembly");
  auto tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.checkpoint_path.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects checked closure proofs without "
        "checkpoint path");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.fact_count += 1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects closure fact-count mismatches");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.fact_count = 0;
  tampered_closure_certificate.closure.fact_digests.clear();
  tampered_closure_certificate.closure.causal_exposures.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects complete closure proofs without "
        "causal facts");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.unresolved_input_checkpoints = {
      tampered_closure_certificate.closure.checkpoint_path.parent_path() /
      "unresolved-despite-complete.pt"};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects complete closure proofs with "
        "unresolved checkpoint witnesses");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.complete = false;
  tampered_closure_certificate.closure.unresolved_input_checkpoints.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects incomplete closure without an "
        "unresolved lineage witness");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.complete = false;
  tampered_closure_certificate.closure.unresolved_input_checkpoints = {
      std::filesystem::path{}};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects incomplete closure with empty "
        "unresolved checkpoint witnesses");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.complete = false;
  const auto duplicate_unresolved_checkpoint =
      tampered_closure_certificate.closure.checkpoint_path.parent_path() /
      "missing-upstream.pt";
  tampered_closure_certificate.closure.unresolved_input_checkpoints = {
      duplicate_unresolved_checkpoint, duplicate_unresolved_checkpoint};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects incomplete closure with "
        "duplicate unresolved checkpoint witnesses");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.complete = false;
  tampered_closure_certificate.closure.unresolved_input_checkpoints = {
      tampered_closure_certificate.closure.checkpoint_path.parent_path() /
      "missing-but-not-an-input.pt"};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects unresolved checkpoint witnesses "
        "not reached by any causal input edge");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.complete = false;
  const auto resolved_despite_unresolved_checkpoint =
      tampered_closure_certificate.closure.checkpoint_path.parent_path() /
      "resolved-despite-unresolved.pt";
  auto resolved_despite_unresolved_producer =
      tampered_closure_certificate.closure.causal_exposures.front();
  resolved_despite_unresolved_producer.fact_digest =
      "resolved_despite_unresolved_digest";
  resolved_despite_unresolved_producer.output_checkpoint =
      resolved_despite_unresolved_checkpoint;
  resolved_despite_unresolved_producer.input_checkpoints.clear();
  tampered_closure_certificate.closure.causal_exposures.front()
      .input_checkpoints = {resolved_despite_unresolved_checkpoint};
  tampered_closure_certificate.closure.causal_exposures.push_back(
      resolved_despite_unresolved_producer);
  tampered_closure_certificate.closure.fact_digests.push_back(
      resolved_despite_unresolved_producer.fact_digest);
  tampered_closure_certificate.closure.fact_count = static_cast<std::int64_t>(
      tampered_closure_certificate.closure.causal_exposures.size());
  tampered_closure_certificate.closure.unresolved_input_checkpoints = {
      resolved_despite_unresolved_checkpoint};
  const auto resolved_despite_unresolved_check =
      target::verify_lattice_target_proof_certificate(
          tampered_closure_certificate);
  check(!resolved_despite_unresolved_check.passed &&
            std::find(resolved_despite_unresolved_check.issues.begin(),
                      resolved_despite_unresolved_check.issues.end(),
                      "closure unresolved checkpoint has producer") !=
                resolved_despite_unresolved_check.issues.end(),
        "proof certificate self-check rejects unresolved checkpoint witnesses "
        "that also have causal producers");
  tampered_closure_certificate = exposure_ready.proof_certificate;
  tampered_closure_certificate.certificate_digest.clear();
  tampered_closure_certificate.closure.checked = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_certificate)
             .passed,
        "proof certificate self-check rejects unchecked closure proofs that "
        "still carry closure claims");
  check(!exposure_ready.proof_certificate.leakage.checked,
        "proof certificate leaves leakage proof unchecked when no protected "
        "range exists");
  auto tampered_unchecked_leakage_certificate =
      exposure_ready.proof_certificate;
  tampered_unchecked_leakage_certificate.certificate_digest.clear();
  tampered_unchecked_leakage_certificate.leakage.forbidden_uses = {
      "observed_input"};
  tampered_unchecked_leakage_certificate.leakage.protected_range =
      exposure::anchor_interval_t{.begin = 20, .end = 30};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_unchecked_leakage_certificate)
             .passed,
        "proof certificate self-check rejects unchecked leakage proofs that "
        "still carry leakage claims");
  check(!exposure_ready.plan_basis.available &&
            exposure_ready.plan_basis.reason ==
                "target already satisfied; no wave suggested",
        "satisfied targets report no plan basis");

  auto extra_clean_fact = make_rep_exposure_fact(exposure_checkpoint, 25, 75);
  extra_clean_fact.job_id = "rep_ranged_extra_clean";
  extra_clean_fact.wave_id = "wave_ranged_extra_clean";
  exposure::lattice_exposure_ledger_t clean_growth_ledger{};
  clean_growth_ledger.add(make_rep_exposure_fact(exposure_checkpoint, 0, 100));
  clean_growth_ledger.add(extra_clean_fact);
  target::lattice_target_evaluator_options_t clean_growth_options =
      exposure_options;
  clean_growth_options.exposure_ledger = &clean_growth_ledger;
  target::lattice_target_evaluator_t clean_growth_eval(coverage_specs,
                                                       clean_growth_options);
  const auto clean_growth_ready =
      clean_growth_eval.evaluate("ranged_representation_ready");
  check(clean_growth_ready.status == target::lattice_target_status_t::satisfied,
        "target satisfaction should be monotone under added clean evidence");
  check(
      clean_growth_ready.exposure_summaries.front().unique_covered_anchors ==
              100 &&
          clean_growth_ready.exposure_summaries.front().loaded_anchor_events ==
              150 &&
          std::abs(clean_growth_ready.exposure_summaries.front()
                       .cursor_exposure_load -
                   1.5) < 1e-12,
      "clean evidence growth keeps idempotent coverage and increases "
      "additive load");
  check(clean_growth_ready.proof_certificate.coverage.front()
                    .covered_intervals.size() == 1 &&
            clean_growth_ready.proof_certificate.coverage.front()
                    .covered_intervals.front()
                    .begin == 0 &&
            clean_growth_ready.proof_certificate.coverage.front()
                    .covered_intervals.front()
                    .end == 100 &&
            clean_growth_ready.proof_certificate.coverage.front()
                    .contributing_intervals.size() == 2,
        "clean evidence growth preserves the same covered union while "
        "recording both contributing intervals");
  check(clean_growth_ready.proof_certificate_check.passed,
        "clean evidence growth proof certificate should self-check");
  check(std::abs(clean_growth_ready.evidence_order_vector
                     .min_unique_coverage_fraction -
                 1.0) < 1e-12 &&
            std::abs(clean_growth_ready.evidence_order_vector
                         .max_cursor_exposure_load -
                     1.5) < 1e-12 &&
            clean_growth_ready.evidence_order_vector.blocking_deficit_count ==
                0,
        "clean evidence growth strengthens the order vector by load without "
        "reducing unique coverage");
  check(target::compare_lattice_evidence_order_vectors(
            clean_growth_ready.evidence_order_vector,
            exposure_ready.evidence_order_vector) ==
            target::lattice_evidence_order_relation_t::left_dominates,
        "clean evidence growth dominates the baseline evidence vector");
  auto tampered_contributing_order_certificate =
      clean_growth_ready.proof_certificate;
  tampered_contributing_order_certificate.certificate_digest.clear();
  std::reverse(tampered_contributing_order_certificate.coverage.front()
                   .contributing_intervals.begin(),
               tampered_contributing_order_certificate.coverage.front()
                   .contributing_intervals.end());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_contributing_order_certificate)
             .passed,
        "proof certificate self-check rejects unsorted contributing coverage "
        "intervals");

  const auto monotone_forbid_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = clean_growth_with_external_guard;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 150;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 160;
  FORBID_EXPOSURE_USES = observed_input;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  target::lattice_target_evaluator_t clean_guard_eval(monotone_forbid_specs,
                                                      exposure_options);
  const auto clean_guard_ready =
      clean_guard_eval.evaluate("clean_growth_with_external_guard");
  check(clean_guard_ready.status == target::lattice_target_status_t::satisfied,
        "clean satisfying evidence should pass an external forbidden-range "
        "guard");
  auto forbidden_growth_fact =
      make_rep_exposure_fact(exposure_checkpoint, 150, 160);
  forbidden_growth_fact.job_id = "rep_ranged_forbidden_growth";
  forbidden_growth_fact.wave_id = "wave_ranged_forbidden_growth";
  exposure::lattice_exposure_ledger_t forbidden_growth_ledger{};
  forbidden_growth_ledger.add(
      make_rep_exposure_fact(exposure_checkpoint, 0, 100));
  forbidden_growth_ledger.add(forbidden_growth_fact);
  target::lattice_target_evaluator_options_t forbidden_growth_options =
      exposure_options;
  forbidden_growth_options.exposure_ledger = &forbidden_growth_ledger;
  target::lattice_target_evaluator_t forbidden_growth_eval(
      monotone_forbid_specs, forbidden_growth_options);
  const auto forbidden_growth_ready =
      forbidden_growth_eval.evaluate("clean_growth_with_external_guard");
  check(forbidden_growth_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "monotonicity is clean-only: added forbidden exposure must still fail "
        "closed");
  check(forbidden_growth_ready.proof_certificate.leakage.checked &&
            forbidden_growth_ready.proof_certificate.leakage.overlap_found,
        "forbidden evidence growth records a leakage proof instead of "
        "silently preserving readiness");
  check(
      forbidden_growth_ready.evidence_order_vector.leakage_checked &&
          !forbidden_growth_ready.evidence_order_vector.leakage_passed &&
          forbidden_growth_ready.evidence_order_vector.blocking_deficit_count >
              0,
      "forbidden evidence growth weakens the order vector through leakage "
      "and deficits");

  const auto warning_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = warned_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 1.5;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = high_optimizer_effort_density;
  KIND = effort_density;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  METRIC = optimizer_steps_per_cursor_epoch;
  ABOVE = 2.0;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = low_anchor_acceptance_fraction;
  KIND = anchor_domain_health;
  ACCEPTED_FRACTION_BELOW = 0.80;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = failed_fetch_probe_count;
  KIND = anchor_domain_health;
  SKIPPED_FAILED_FETCH_PROBE_ABOVE = 0;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = high_representation_variance_loss;
  KIND = representation_health;
  METRIC = mean_variance_loss;
  SCOPE = target_component;
  EFFECT = mutated_component;
  ABOVE = 0.25;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = low_representation_valid_channel_fraction;
  KIND = representation_health;
  METRIC = mean_adapter_valid_channel_time_fraction;
  SCOPE = target_component;
  EFFECT = mutated_component;
  BELOW = 0.80;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = low_representation_effective_rank_fraction;
  KIND = representation_health;
  METRIC = representation_effective_rank_fraction;
  SCOPE = target_component;
  EFFECT = mutated_component;
  BELOW = 0.75;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = high_representation_condition_number;
  KIND = representation_health;
  METRIC = representation_condition_number;
  SCOPE = target_component;
  EFFECT = mutated_component;
  ABOVE = 100;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = acceptable_representation_covariance_loss;
  KIND = representation_health;
  METRIC = mean_covariance_loss;
  SCOPE = target_component;
  EFFECT = mutated_component;
  ABOVE = 0.10;
};

LATTICE_WARN {
  TARGET_ID = warned_representation_ready;
  WARNING_ID = passing_representation_parameter_check;
  KIND = representation_health;
  METRIC = finite_parameter_check;
  SCOPE = target_component;
  EFFECT = mutated_component;
  BELOW = 0.50;
};
)DSL");
  auto warning_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  warning_fact.candidate_anchor_count = 100;
  warning_fact.accepted_anchor_count = 99;
  warning_fact.accepted_anchor_fraction = 0.99;
  warning_fact.skipped_failed_fetch_probe = 0;
  auto repeated_fact = warning_fact;
  repeated_fact.job_id = "rep_ranged_repeat";
  repeated_fact.optimizer_steps = 4;
  auto outside_warning_fact =
      make_rep_exposure_fact(exposure_checkpoint, 100, 200);
  outside_warning_fact.job_id = "rep_ranged_outside_warning";
  outside_warning_fact.wave_id = "wave_ranged_outside_warning";
  outside_warning_fact.candidate_anchor_count = 100;
  outside_warning_fact.accepted_anchor_count = 10;
  outside_warning_fact.accepted_anchor_fraction = 0.10;
  outside_warning_fact.skipped_failed_fetch_probe = 9;
  outside_warning_fact.mean_variance_loss = 9.0;
  outside_warning_fact.mean_covariance_loss = 9.0;
  outside_warning_fact.representation_effective_rank_fraction = 0.01;
  outside_warning_fact.representation_condition_number = 9999.0;
  exposure::lattice_exposure_ledger_t warning_ledger{};
  warning_ledger.add(warning_fact);
  warning_ledger.add(repeated_fact);
  warning_ledger.add(outside_warning_fact);
  target::lattice_target_evaluator_options_t warning_options = exposure_options;
  warning_options.split_policy = split_policy;
  warning_options.exposure_ledger = &warning_ledger;
  target::lattice_target_evaluator_t warning_eval(warning_specs,
                                                  warning_options);
  const auto warned_ready =
      warning_eval.evaluate("warned_representation_ready");
  check(warned_ready.status == target::lattice_target_status_t::satisfied,
        "warning clauses must not change satisfied target status");
  check(!warned_ready.plan_ready,
        "warning clauses must not make a satisfied target plan-ready");
  check(warned_ready.deficits.empty() && !warned_ready.plan_basis.available &&
            warned_ready.plan_basis.reason ==
                "target already satisfied; no wave suggested",
        "triggered warning clauses must not create proof deficits or planning "
        "basis");
  expect_plan_basis_projects_deficits(
      warned_ready, "warning-only ready plan basis should project no deficits");
  check(warned_ready.warning_results.size() == 10,
        "target should report every configured warning result");
  check(
      warned_ready.warning_results.front().status == "warning" &&
          warned_ready.warning_results.front().evidence_basis ==
              "exposure_load_summary" &&
          warned_ready.warning_results.front().threshold_triggered &&
          warned_ready.warning_results.front().threshold_relation ==
              "above_threshold" &&
          warned_ready.warning_results.front().threshold_direction == "above" &&
          warned_ready.warning_results.front().measurement_available &&
          warned_ready.warning_results.front().exposure_summary_available &&
          !warned_ready.warning_results.front()
               .node_support_summary_available &&
          std::abs(warned_ready.warning_results.front().measured_value - 2.0) <
              1e-12,
      "repeated exposure load should trigger a warning");
  check(warned_ready.warning_results.front().message.find(
            "above threshold 1") != std::string::npos,
        "triggered warning message should say above threshold");
  const auto clear_anchor_fraction_warning = std::find_if(
      warned_ready.warning_results.begin(), warned_ready.warning_results.end(),
      [](const auto &warning) {
        return warning.warning_id == "low_anchor_acceptance_fraction";
      });
  check(clear_anchor_fraction_warning != warned_ready.warning_results.end() &&
            clear_anchor_fraction_warning->status == "clear" &&
            clear_anchor_fraction_warning->evidence_basis ==
                "anchor_domain_facts" &&
            !clear_anchor_fraction_warning->threshold_triggered &&
            clear_anchor_fraction_warning->threshold_relation ==
                "not_below_threshold" &&
            clear_anchor_fraction_warning->threshold_direction == "below" &&
            clear_anchor_fraction_warning->measurement_available &&
            clear_anchor_fraction_warning->exposure_summary_available &&
            clear_anchor_fraction_warning->exposure_summary.fact_count == 2 &&
            std::abs(clear_anchor_fraction_warning->measured_value - 0.99) <
                1e-12 &&
            !clear_anchor_fraction_warning->node_support_summary_available &&
            clear_anchor_fraction_warning->message.find(
                "not below threshold 0.8") != std::string::npos,
        "clear anchor-domain accepted-fraction warning message should say not "
        "below threshold");
  const auto clear_failed_fetch_warning =
      std::find_if(warned_ready.warning_results.begin(),
                   warned_ready.warning_results.end(), [](const auto &warning) {
                     return warning.warning_id == "failed_fetch_probe_count";
                   });
  check(clear_failed_fetch_warning != warned_ready.warning_results.end() &&
            clear_failed_fetch_warning->status == "clear" &&
            !clear_failed_fetch_warning->threshold_triggered &&
            clear_failed_fetch_warning->threshold_relation ==
                "not_above_threshold" &&
            clear_failed_fetch_warning->threshold_direction == "above" &&
            clear_failed_fetch_warning->measurement_available &&
            clear_failed_fetch_warning->exposure_summary.fact_count == 2 &&
            std::abs(clear_failed_fetch_warning->measured_value - 0.0) <
                1e-12 &&
            clear_failed_fetch_warning->message.find("not above threshold 0") !=
                std::string::npos,
        "clear anchor-domain failed-fetch warning message should say not above "
        "threshold");
  const auto untrusted_anchor_warning_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = untrusted_anchor_warning_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
};

LATTICE_WARN {
  TARGET_ID = untrusted_anchor_warning_ready;
  WARNING_ID = untrusted_anchor_acceptance_low;
  KIND = anchor_domain_health;
  ACCEPTED_FRACTION_BELOW = 0.80;
};
)DSL");
  auto untrusted_anchor_warning_fact = warning_fact;
  untrusted_anchor_warning_fact.job_id = "rep_ranged_untrusted_anchor_warning";
  untrusted_anchor_warning_fact.wave_id =
      "wave_ranged_untrusted_anchor_warning";
  untrusted_anchor_warning_fact.coverage_precision =
      "requested_range_untrusted_v0";
  untrusted_anchor_warning_fact.completed_anchor_range =
      untrusted_anchor_warning_fact.anchor_range;
  untrusted_anchor_warning_fact.accepted_anchor_count = 10;
  untrusted_anchor_warning_fact.accepted_anchor_fraction = 0.10;
  exposure::lattice_exposure_ledger_t untrusted_anchor_warning_ledger{};
  untrusted_anchor_warning_ledger.add(warning_fact);
  untrusted_anchor_warning_ledger.add(untrusted_anchor_warning_fact);
  target::lattice_target_evaluator_options_t untrusted_anchor_warning_options =
      exposure_options;
  untrusted_anchor_warning_options.exposure_ledger =
      &untrusted_anchor_warning_ledger;
  target::lattice_target_evaluator_t untrusted_anchor_warning_eval(
      untrusted_anchor_warning_specs, untrusted_anchor_warning_options);
  const auto untrusted_anchor_warning_ready =
      untrusted_anchor_warning_eval.evaluate("untrusted_anchor_warning_ready");
  const auto untrusted_anchor_warning = std::find_if(
      untrusted_anchor_warning_ready.warning_results.begin(),
      untrusted_anchor_warning_ready.warning_results.end(),
      [](const auto &warning) {
        return warning.warning_id == "untrusted_anchor_acceptance_low";
      });
  check(untrusted_anchor_warning_ready.status ==
                target::lattice_target_status_t::satisfied &&
            untrusted_anchor_warning !=
                untrusted_anchor_warning_ready.warning_results.end() &&
            untrusted_anchor_warning->status == "warning" &&
            untrusted_anchor_warning->threshold_triggered &&
            untrusted_anchor_warning->warning_anchor_range.begin == 0 &&
            untrusted_anchor_warning->warning_anchor_range.end == 100 &&
            untrusted_anchor_warning->exposure_summary.fact_count == 2 &&
            std::abs(untrusted_anchor_warning->measured_value - 0.10) < 1e-12,
        "anchor-domain warnings should stay visible for in-range untrusted "
        "coverage facts without crediting them as readiness coverage");
  check(warned_ready.warning_results[4].status == "warning" &&
            warned_ready.warning_results[4].kind == "representation_health" &&
            warned_ready.warning_results[4].metric == "mean_variance_loss" &&
            warned_ready.warning_results[4].evidence_basis ==
                "representation_health_facts" &&
            warned_ready.warning_results[4].threshold_triggered &&
            warned_ready.warning_results[4].threshold_relation ==
                "above_threshold" &&
            warned_ready.warning_results[4].threshold_direction == "above" &&
            warned_ready.warning_results[4].measurement_available &&
            warned_ready.warning_results[4].exposure_summary_available &&
            warned_ready.warning_results[4].exposure_summary.fact_count == 2 &&
            !warned_ready.warning_results[4].node_support_summary_available &&
            std::abs(warned_ready.warning_results[4].measured_value - 0.30) <
                1e-12,
        "representation-health warning can trigger on high variance loss");
  check(warned_ready.warning_results[5].status == "warning" &&
            warned_ready.warning_results[5].metric ==
                "mean_adapter_valid_channel_time_fraction" &&
            warned_ready.warning_results[5].threshold_triggered &&
            warned_ready.warning_results[5].threshold_relation ==
                "below_threshold" &&
            warned_ready.warning_results[5].threshold_direction == "below" &&
            std::abs(warned_ready.warning_results[5].measured_value - 0.75) <
                1e-12,
        "representation-health warning can trigger on low adapter support");
  const auto low_effective_rank_warning =
      std::find_if(warned_ready.warning_results.begin(),
                   warned_ready.warning_results.end(), [](const auto &warning) {
                     return warning.warning_id ==
                            "low_representation_effective_rank_fraction";
                   });
  check(low_effective_rank_warning != warned_ready.warning_results.end() &&
            low_effective_rank_warning->status == "warning" &&
            low_effective_rank_warning->unit == "fraction" &&
            low_effective_rank_warning->threshold_triggered &&
            low_effective_rank_warning->threshold_relation ==
                "below_threshold" &&
            low_effective_rank_warning->threshold_direction == "below" &&
            std::abs(low_effective_rank_warning->measured_value - 0.50) < 1e-12,
        "representation-health warning can trigger on low effective rank "
        "fraction");
  const auto high_condition_number_warning = std::find_if(
      warned_ready.warning_results.begin(), warned_ready.warning_results.end(),
      [](const auto &warning) {
        return warning.warning_id == "high_representation_condition_number";
      });
  check(high_condition_number_warning != warned_ready.warning_results.end() &&
            high_condition_number_warning->status == "warning" &&
            high_condition_number_warning->unit == "condition_number" &&
            high_condition_number_warning->threshold_triggered &&
            high_condition_number_warning->threshold_relation ==
                "above_threshold" &&
            std::abs(high_condition_number_warning->measured_value - 625.0) <
                1e-12,
        "representation-health warning can trigger on high condition number");
  const auto clear_covariance_warning =
      std::find_if(warned_ready.warning_results.begin(),
                   warned_ready.warning_results.end(), [](const auto &warning) {
                     return warning.warning_id ==
                            "acceptable_representation_covariance_loss";
                   });
  check(clear_covariance_warning != warned_ready.warning_results.end() &&
            clear_covariance_warning->status == "clear" &&
            !clear_covariance_warning->threshold_triggered &&
            clear_covariance_warning->threshold_relation ==
                "not_above_threshold" &&
            clear_covariance_warning->message.find("not above threshold 0.1") !=
                std::string::npos,
        "clear representation-health ABOVE warning message should say not "
        "above threshold");
  const auto clear_parameter_warning = std::find_if(
      warned_ready.warning_results.begin(), warned_ready.warning_results.end(),
      [](const auto &warning) {
        return warning.warning_id == "passing_representation_parameter_check";
      });
  check(clear_parameter_warning != warned_ready.warning_results.end() &&
            clear_parameter_warning->status == "clear" &&
            clear_parameter_warning->message.find("not below threshold 0.5") !=
                std::string::npos,
        "clear representation-health BELOW warning message should say not "
        "below threshold");
  check(warned_ready.warnings.size() == 6,
        "only triggered warnings should be mirrored in the warnings list");
  check(std::count_if(warned_ready.warning_results.begin(),
                      warned_ready.warning_results.end(),
                      [](const auto &warning) {
                        return !warning.measurement_available;
                      }) == 0 &&
            warned_ready.evidence_order_vector.warning_result_count == 10 &&
            warned_ready.evidence_order_vector.triggered_warning_count == 6 &&
            warned_ready.evidence_order_vector.unavailable_warning_count == 0 &&
            warned_ready.evidence_order_vector.warning_count == 6,
        "warning measurement availability should be structured and projected "
        "into the evidence order vector");
  check(warned_ready.warning_summary.warning_result_count == 10 &&
            warned_ready.warning_summary.triggered_warning_count == 6 &&
            warned_ready.warning_summary.clear_measured_warning_count == 4 &&
            warned_ready.warning_summary.unavailable_warning_count == 0 &&
            warned_ready.warning_summary.warning_count == 6 &&
            warned_ready.warning_summary.above_threshold_count +
                    warned_ready.warning_summary.below_threshold_count ==
                6 &&
            warned_ready.warning_summary.not_above_threshold_count +
                    warned_ready.warning_summary.not_below_threshold_count ==
                4,
        "warning summary should aggregate triggered and clear measured "
        "warnings by threshold relation");
  expect_warnings_project_warning_results(
      warned_ready, "warning list should project triggered warning results");

  const auto proof_neutral_warning_specs_a =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = proof_neutral_warning_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
};

LATTICE_WARN {
  TARGET_ID = proof_neutral_warning_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 1.5;
};
)DSL");
  const auto proof_neutral_warning_specs_b =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = proof_neutral_warning_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
};

LATTICE_WARN {
  TARGET_ID = proof_neutral_warning_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 3.0;
};
)DSL");
  const auto proof_neutral_warning_specs_c =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = proof_neutral_warning_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
};

LATTICE_WARN {
  TARGET_ID = proof_neutral_warning_ready;
  WARNING_ID = high_observed_input_load;
  KIND = exposure_load;
  USE = observed_input;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  SCOPE = target_component;
  EFFECT = mutated_component;
  CURSOR_EPOCHS_ABOVE = 1.5;
};
)DSL");
  target::lattice_target_evaluator_t proof_neutral_warning_eval_a(
      proof_neutral_warning_specs_a, warning_options);
  target::lattice_target_evaluator_t proof_neutral_warning_eval_b(
      proof_neutral_warning_specs_b, warning_options);
  target::lattice_target_evaluator_t proof_neutral_warning_eval_c(
      proof_neutral_warning_specs_c, warning_options);
  const auto proof_neutral_warning_ready_a =
      proof_neutral_warning_eval_a.evaluate("proof_neutral_warning_ready");
  const auto proof_neutral_warning_ready_b =
      proof_neutral_warning_eval_b.evaluate("proof_neutral_warning_ready");
  const auto proof_neutral_warning_ready_c =
      proof_neutral_warning_eval_c.evaluate("proof_neutral_warning_ready");
  check(proof_neutral_warning_specs_a.front().target_spec_fingerprint ==
            proof_neutral_warning_specs_b.front().target_spec_fingerprint,
        "warning threshold changes should not alter target identity");
  check(proof_neutral_warning_specs_a.front().target_spec_fingerprint ==
            proof_neutral_warning_specs_c.front().target_spec_fingerprint,
        "warning interval changes should not alter target identity");
  check(proof_neutral_warning_ready_a.status ==
                target::lattice_target_status_t::satisfied &&
            proof_neutral_warning_ready_b.status ==
                target::lattice_target_status_t::satisfied &&
            proof_neutral_warning_ready_c.status ==
                target::lattice_target_status_t::satisfied,
        "warning threshold and interval changes should not alter readiness "
        "status");
  check(proof_neutral_warning_ready_a.warning_results.front().status ==
                "warning" &&
            proof_neutral_warning_ready_b.warning_results.front().status ==
                "clear",
        "warning threshold changes should still alter warning visibility");
  expect_warnings_project_warning_results(
      proof_neutral_warning_ready_a,
      "triggered proof-neutral warning list should project warning results");
  expect_warnings_project_warning_results(
      proof_neutral_warning_ready_b,
      "clear proof-neutral warning list should project warning results");
  check(proof_neutral_warning_ready_c.warning_results.front().status ==
                "clear" &&
            proof_neutral_warning_ready_c.warning_results.front()
                    .warning_anchor_range.begin == 100 &&
            proof_neutral_warning_ready_c.warning_results.front()
                    .warning_anchor_range.end == 200,
        "warning interval changes should alter only warning visibility and "
        "resolved warning range");
  expect_warnings_project_warning_results(
      proof_neutral_warning_ready_c,
      "range-scoped proof-neutral warning list should project warning results");
  check(proof_neutral_warning_ready_a.proof_certificate.certificate_digest ==
            proof_neutral_warning_ready_b.proof_certificate.certificate_digest,
        "warning threshold changes should not alter proof certificate digest");
  check(proof_neutral_warning_ready_a.proof_certificate.certificate_digest ==
            proof_neutral_warning_ready_c.proof_certificate.certificate_digest,
        "warning interval changes should not alter proof certificate digest");
  check(proof_neutral_warning_ready_a.proof_certificate_check.passed &&
            proof_neutral_warning_ready_b.proof_certificate_check.passed &&
            proof_neutral_warning_ready_c.proof_certificate_check.passed,
        "warning-neutral proof certificates should self-check");

  auto non_mutated_repeated_fact =
      make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  non_mutated_repeated_fact.job_id = "rep_ranged_repeat";
  non_mutated_repeated_fact.optimizer_steps = 4;
  non_mutated_repeated_fact.use.mutated_component = false;
  exposure::lattice_exposure_ledger_t non_mutated_warning_ledger{};
  non_mutated_warning_ledger.add(
      make_rep_exposure_fact(exposure_checkpoint, 0, 100));
  non_mutated_warning_ledger.add(non_mutated_repeated_fact);
  warning_options.exposure_ledger = &non_mutated_warning_ledger;
  target::lattice_target_evaluator_t non_mutated_warning_eval(warning_specs,
                                                              warning_options);
  const auto non_mutated_warned_ready =
      non_mutated_warning_eval.evaluate("warned_representation_ready");
  check(non_mutated_warned_ready.status ==
            target::lattice_target_status_t::satisfied,
        "non-mutated repeated facts should not affect readiness");
  check(
      non_mutated_warned_ready.warning_results.front().status == "clear" &&
          std::abs(
              non_mutated_warned_ready.warning_results.front().measured_value -
              1.0) < 1e-12,
      "non-mutated repeated facts should not count for mutated warnings");
  check(non_mutated_warned_ready.warning_results.front().message.find(
            "not above threshold 1") != std::string::npos,
        "clear exposure warning message should say not above threshold");
  const auto non_mutated_anchor_warning = std::find_if(
      non_mutated_warned_ready.warning_results.begin(),
      non_mutated_warned_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "low_anchor_acceptance_fraction";
      });
  check(non_mutated_anchor_warning !=
                non_mutated_warned_ready.warning_results.end() &&
            non_mutated_anchor_warning->status == "clear" &&
            !non_mutated_anchor_warning->measurement_available &&
            non_mutated_anchor_warning->message.find(
                "accepted fraction is unavailable for threshold 0.8") !=
                std::string::npos,
        "clear anchor-domain warning message should report unavailable "
        "accepted fractions");
  expect_warnings_project_warning_results(
      non_mutated_warned_ready,
      "non-mutated warning list should project warning results");

  auto no_rep_health_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  no_rep_health_fact.representation_health_available = false;
  no_rep_health_fact.mean_invariance_loss =
      std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.mean_variance_loss =
      std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.mean_covariance_loss =
      std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.last_grad_norm = std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.max_grad_norm = std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.total_valid_projection_rows = 0;
  no_rep_health_fact.mean_adapter_valid_channel_time_fraction =
      std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.augmented_valid_feature_count = 0;
  no_rep_health_fact.mean_augmented_valid_feature_fraction =
      std::numeric_limits<double>::quiet_NaN();
  no_rep_health_fact.mean_augmented_feature_retention_fraction =
      std::numeric_limits<double>::quiet_NaN();
  exposure::lattice_exposure_ledger_t no_rep_health_warning_ledger{};
  no_rep_health_warning_ledger.add(no_rep_health_fact);
  warning_options.exposure_ledger = &no_rep_health_warning_ledger;
  target::lattice_target_evaluator_t no_rep_health_warning_eval(
      warning_specs, warning_options);
  const auto no_rep_health_warned_ready =
      no_rep_health_warning_eval.evaluate("warned_representation_ready");
  const auto missing_rep_health_warning = std::find_if(
      no_rep_health_warned_ready.warning_results.begin(),
      no_rep_health_warned_ready.warning_results.end(),
      [](const auto &warning) {
        return warning.warning_id == "high_representation_variance_loss";
      });
  check(missing_rep_health_warning !=
                no_rep_health_warned_ready.warning_results.end() &&
            missing_rep_health_warning->status == "clear" &&
            missing_rep_health_warning->threshold_direction == "above" &&
            !missing_rep_health_warning->threshold_triggered &&
            missing_rep_health_warning->threshold_relation == "unavailable" &&
            !missing_rep_health_warning->measurement_available &&
            missing_rep_health_warning->message.find(
                "representation-health mean_variance_loss is unavailable for "
                "threshold 0.25") != std::string::npos,
        "clear representation-health warning message should report unavailable "
        "metrics with the configured threshold");
  check(no_rep_health_warned_ready.evidence_order_vector
                .unavailable_warning_count > 0,
        "unavailable representation-health warning measurements should weaken "
        "the evidence order vector without parsing messages");
  check(no_rep_health_warned_ready.warning_summary.unavailable_warning_count >
                0 &&
            no_rep_health_warned_ready.warning_summary
                    .unavailable_relation_count > 0,
        "warning summary should aggregate unavailable representation-health "
        "warning measurements");
  expect_warnings_project_warning_results(
      no_rep_health_warned_ready,
      "missing representation-health warning list should project warning "
      "results");

  target::lattice_target_evaluator_options_t auto_ledger_options = options;
  auto_ledger_options.runtime_root = exposure_root;
  target::lattice_target_evaluator_t auto_ledger_eval(coverage_specs,
                                                      auto_ledger_options);
  const auto auto_ledger =
      auto_ledger_eval.evaluate("ranged_representation_ready");
  check(
      auto_ledger.status == target::lattice_target_status_t::satisfied,
      "target evaluator should auto-build exposure ledger from runtime jobs, "
      "got " +
          std::string(target::lattice_target_status_name(auto_ledger.status)) +
          " certificate_issues=" +
          join_strings(auto_ledger.proof_certificate_check.issues, "; "));

  target::lattice_target_evaluator_options_t missing_split_policy_options =
      options;
  missing_split_policy_options.split_policy = std::nullopt;
  target::lattice_target_evaluator_t missing_split_policy_eval(
      split_backed_specs, missing_split_policy_options);
  const auto missing_split_policy =
      missing_split_policy_eval.evaluate("split_representation_ready");
  check(missing_split_policy.status == target::lattice_target_status_t::blocked,
        "split-backed targets should block when no split policy is available");
  check(std::any_of(missing_split_policy.deficits.begin(),
                    missing_split_policy.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:split_policy" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing" &&
                             deficit.priority == 35 &&
                             deficit.priority_class == "artifact";
                    }),
        "missing split policy should expose a split-policy artifact deficit");
  expect_plan_basis_projects_deficits(
      missing_split_policy,
      "missing split policy should project proof deficits");

  const auto unknown_split_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = unknown_split_representation_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = missing_train_split;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};
)DSL");
  target::lattice_target_evaluator_t unknown_split_eval(unknown_split_specs,
                                                        options);
  const auto unknown_split =
      unknown_split_eval.evaluate("unknown_split_representation_ready");
  check(unknown_split.status == target::lattice_target_status_t::blocked,
        "targets should block when a named split is not in the active split "
        "policy");
  check(std::any_of(unknown_split.deficits.begin(),
                    unknown_split.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:split_policy" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "unknown split references should expose a split-policy artifact "
        "deficit");
  expect_plan_basis_projects_deficits(
      unknown_split, "unknown split references should project proof deficits");

  const auto malformed_warning_split_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = malformed_warning_split_target;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = malformed_warning_split_target;
  WARNING_ID = malformed_warning_split_range;
  KIND = exposure_load;
  USE = observed_input;
  SPLIT = train_core;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  CURSOR_EPOCHS_ABOVE = 2.0;
};
)DSL");
  target::lattice_target_evaluator_t malformed_warning_split_eval(
      malformed_warning_split_specs, options);
  const auto malformed_warning_split =
      malformed_warning_split_eval.evaluate("malformed_warning_split_target");
  check(malformed_warning_split.status ==
            target::lattice_target_status_t::blocked,
        "warning clauses should block when they mix SPLIT and explicit anchor "
        "bounds");
  check(std::any_of(malformed_warning_split.deficits.begin(),
                    malformed_warning_split.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:split_policy" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "malformed split-backed warning ranges should expose a split-policy "
        "artifact deficit");
  expect_plan_basis_projects_deficits(
      malformed_warning_split,
      "malformed split-backed warning ranges should project proof deficits");

  target::lattice_target_evaluator_t split_plan_eval(split_backed_specs,
                                                     options);
  const auto split_plan =
      split_plan_eval.evaluate("split_representation_ready");
  check(split_plan.status == target::lattice_target_status_t::missing_report,
        "split-backed target should be missing before matching jobs exist");
  check(split_plan.suggested_wave.source_range == "anchor_index" &&
            split_plan.suggested_wave.anchor_index_begin.has_value() &&
            *split_plan.suggested_wave.anchor_index_begin == 0 &&
            split_plan.suggested_wave.anchor_index_end.has_value() &&
            *split_plan.suggested_wave.anchor_index_end == 100,
        "split-backed target plan should resolve TRAIN_SPLIT to anchor range");

  target::lattice_target_evaluator_options_t split_ready_options =
      exposure_options;
  split_ready_options.split_policy = split_policy;
  target::lattice_target_evaluator_t split_ready_eval(split_backed_specs,
                                                      split_ready_options);
  const auto split_ready =
      split_ready_eval.evaluate("split_representation_ready");
  check(split_ready.status == target::lattice_target_status_t::satisfied,
        "TRAIN_SPLIT should resolve target coverage while FORBID_SPLIT stays "
        "clean");
  check(split_ready.split_policy_fingerprint == split_policy_fingerprint,
        "target evaluation carries split policy fingerprint");

  const auto split_forbid_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = split_representation_leaks_train;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  FORBID_SPLIT = train_core;
  FORBID_EXPOSURE_USES = observed_input;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  target::lattice_target_evaluator_t split_forbid_eval(split_forbid_specs,
                                                       split_ready_options);
  const auto split_forbidden =
      split_forbid_eval.evaluate("split_representation_leaks_train");
  check(split_forbidden.status ==
            target::lattice_target_status_t::exposure_failed,
        "FORBID_SPLIT should resolve to forbidden source-row interval");

  const auto boundary_split_policy =
      split::decode_lattice_split_policy_from_dsl(R"DSL(
LATTICE_SPLIT_POLICY {
  POLICY_ID = boundary_holdout;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 110;
  PROTECT_FROM_USES = observed_input;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
)DSL");
  auto boundary_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  boundary_fact.observed_footprint =
      exposure::anchor_interval_t{.begin = -1, .end = 100};
  boundary_fact.source_input_length = 2;
  exposure::lattice_exposure_ledger_t boundary_ledger{};
  boundary_ledger.add(boundary_fact);
  target::lattice_target_evaluator_options_t boundary_options =
      exposure_options;
  boundary_options.split_policy = boundary_split_policy;
  boundary_options.exposure_ledger = &boundary_ledger;
  const auto boundary_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = split_representation_left_context_leak;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  target::lattice_target_evaluator_t boundary_eval(boundary_specs,
                                                   boundary_options);
  const auto boundary_forbidden =
      boundary_eval.evaluate("split_representation_left_context_leak");
  check(boundary_forbidden.status ==
            target::lattice_target_status_t::exposure_failed,
        "PROTECT_SPLIT should expand validation left by Hx before forbidden "
        "overlap checks");
  check(boundary_forbidden.proof_certificate.leakage.checked,
        "leakage proof should be recorded for PROTECT_SPLIT targets");
  check(boundary_forbidden.proof_certificate.leakage.split_name ==
            "validation_holdout",
        "leakage proof should name the protected split");
  check(boundary_forbidden.proof_certificate.leakage.base_range.begin == 100 &&
            boundary_forbidden.proof_certificate.leakage.base_range.end == 110,
        "leakage proof should preserve the undilated split range");
  check(boundary_forbidden.proof_certificate.leakage.protected_range.begin ==
                99 &&
            boundary_forbidden.proof_certificate.leakage.protected_range.end ==
                110,
        "leakage proof should expose the purge-expanded protected range");
  check(boundary_forbidden.proof_certificate.leakage.dilation_left == 1 &&
            boundary_forbidden.proof_certificate.leakage.dilation_right == 0,
        "leakage proof should expose left/right dilation widths");
  check(boundary_forbidden.proof_certificate.leakage.overlap_witnesses.size() ==
            1,
        "leakage proof should expose overlap witnesses");
  check(
      boundary_forbidden.proof_certificate.leakage.overlap_witnesses.front()
                  .use == exposure::exposure_use_t::observed_input &&
          boundary_forbidden.proof_certificate.leakage.overlap_witnesses.front()
                  .source_footprint.begin == -1 &&
          boundary_forbidden.proof_certificate.leakage.overlap_witnesses.front()
                  .source_footprint.end == 100 &&
          boundary_forbidden.proof_certificate.leakage.overlap_witnesses.front()
                  .intersection.begin == 99 &&
          boundary_forbidden.proof_certificate.leakage.overlap_witnesses.front()
                  .intersection.end == 100,
      "leakage witness should name the forbidden use, source footprint, and "
      "intersection");
  check(target::verify_lattice_target_proof_certificate(
            boundary_forbidden.proof_certificate)
            .passed,
        "leakage proof certificate should self-check protected split "
        "dilation");

  const auto selection_signal_split_policy =
      split::decode_lattice_split_policy_from_dsl(R"DSL(
LATTICE_SPLIT_POLICY {
  POLICY_ID = selection_signal_holdout;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 110;
  PROTECT_FROM_USES = selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
)DSL");
  auto selection_signal_fact =
      make_rep_exposure_fact(exposure_checkpoint, 100, 101);
  selection_signal_fact.use.observed_input = false;
  selection_signal_fact.use.target_supervision = false;
  selection_signal_fact.use.evaluation_metric = false;
  selection_signal_fact.use.selection_signal = true;
  selection_signal_fact.observed_footprint = {};
  selection_signal_fact.target_footprint = {};
  selection_signal_fact.source_input_length = 1;
  selection_signal_fact.source_future_length = 0;
  attach_source_key_window(selection_signal_fact);
  exposure::lattice_exposure_ledger_t selection_signal_ledger{};
  selection_signal_ledger.add(selection_signal_fact);
  check(selection_signal_ledger.selection_signal_facts().size() == 1 &&
            selection_signal_ledger.selection_signal_facts()
                    .front()
                    .selected_checkpoint == exposure_checkpoint &&
            selection_signal_ledger.selection_signal_facts()
                    .front()
                    .selector_id == selection_signal_fact.job_id,
        "selection_signal exposures should create first-class selector event "
        "facts");
  target::lattice_target_evaluator_options_t selection_signal_options =
      exposure_options;
  selection_signal_options.split_policy = selection_signal_split_policy;
  selection_signal_options.exposure_ledger = &selection_signal_ledger;
  const auto selection_signal_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = selection_signal_validation_leak;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  target::lattice_target_evaluator_t selection_signal_eval(
      selection_signal_specs, selection_signal_options);
  const auto selection_signal_forbidden =
      selection_signal_eval.evaluate("selection_signal_validation_leak");
  check(selection_signal_forbidden.status ==
            target::lattice_target_status_t::exposure_failed,
        "PROTECT_SPLIT should reject selection_signal exposure inside the "
        "protected split");
  check(selection_signal_forbidden.proof_certificate.leakage.forbidden_uses
                    .size() == 1 &&
            selection_signal_forbidden.proof_certificate.leakage.forbidden_uses
                    .front() == "selection_signal",
        "selection-signal leakage proof should keep the forbidden causal "
        "label");
  check(
      selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .size() == 1 &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .use == exposure::exposure_use_t::selection_signal &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .source_footprint.begin == 100 &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .source_footprint.end == 101 &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .intersection.begin == 100 &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .intersection.end == 101,
      "selection-signal leakage witness should use the anchor footprint as "
      "the source footprint");
  check(
      !selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
              .front()
              .selection_event_digest.empty() &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .selector_id == selection_signal_fact.job_id &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .selector_kind == "checkpoint_selection" &&
          selection_signal_forbidden.proof_certificate.leakage.overlap_witnesses
                  .front()
                  .selected_checkpoint == exposure_checkpoint,
      "selection-signal leakage witness should identify the selector event and "
      "selected checkpoint");
  check(target::verify_lattice_target_proof_certificate(
            selection_signal_forbidden.proof_certificate)
            .passed,
        "selection-signal leakage proof certificate should self-check");
  auto tampered_selection_signal_certificate =
      selection_signal_forbidden.proof_certificate;
  tampered_selection_signal_certificate.certificate_digest.clear();
  tampered_selection_signal_certificate.closure.causal_exposures.front()
      .uses.clear();
  tampered_selection_signal_certificate.closure.causal_exposures.front()
      .uses.push_back("observed_input");
  check(!target::verify_lattice_target_proof_certificate(
             tampered_selection_signal_certificate)
             .passed,
        "proof certificate self-check rejects selection-signal leakage "
        "witnesses whose causal label was removed from closure");

  auto tampered_leakage_rule_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_rule_certificate.certificate_digest.clear();
  tampered_leakage_rule_certificate.leakage.rule = "raw_interval_overlap";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_rule_certificate)
             .passed,
        "proof certificate self-check rejects unknown leakage proof rules");
  auto leakage_witness_order_certificate = boundary_forbidden.proof_certificate;
  auto second_leakage_witness =
      leakage_witness_order_certificate.leakage.overlap_witnesses.front();
  second_leakage_witness.fact_digest += "_second";
  second_leakage_witness.job_id += "_second";
  second_leakage_witness.intersection =
      exposure::anchor_interval_t{.begin = 99, .end = 100};
  leakage_witness_order_certificate.leakage.overlap_witnesses.push_back(
      second_leakage_witness);
  const auto leakage_witness_order_digest =
      target::lattice_target_proof_certificate_digest(
          leakage_witness_order_certificate);
  std::reverse(
      leakage_witness_order_certificate.leakage.overlap_witnesses.begin(),
      leakage_witness_order_certificate.leakage.overlap_witnesses.end());
  check(leakage_witness_order_digest ==
            target::lattice_target_proof_certificate_digest(
                leakage_witness_order_certificate),
        "proof certificate digest is canonical across leakage witness order");
  auto tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.dilation_left = 0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects tampered protected split "
        "dilation");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.dilation_left = -1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects negative protected split "
        "dilation");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.base_range = {};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects checked leakage proofs without "
        "an undilated base range");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.split_policy_fingerprint.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects named split leakage proofs "
        "without split policy fingerprint");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.closure.checked = false;
  tampered_leakage_certificate.closure.checkpoint_path.clear();
  tampered_leakage_certificate.closure.complete = false;
  tampered_leakage_certificate.closure.fact_count = 0;
  tampered_leakage_certificate.closure.unresolved_input_checkpoints.clear();
  tampered_leakage_certificate.closure.fact_digests.clear();
  tampered_leakage_certificate.closure.causal_exposures.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage proofs without checked "
        "checkpoint closure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.closure.complete = false;
  tampered_leakage_certificate.closure.unresolved_input_checkpoints.push_back(
      boundary_forbidden.proof_certificate.closure.checkpoint_path
          .parent_path() /
      "missing-upstream.pt");
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage proofs over incomplete "
        "checkpoint closure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_found = false;
  tampered_leakage_certificate.leakage.passed = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage overlap flags that "
        "disagree with witnesses");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.passed = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage passed flags that "
        "disagree with overlap status");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.clear();
  tampered_leakage_certificate.leakage.overlap_found = false;
  tampered_leakage_certificate.leakage.passed = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects clean leakage claims "
        "contradicted by causal closure overlaps");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.push_back(
      tampered_leakage_certificate.leakage.overlap_witnesses.front());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects duplicate leakage overlap "
        "witnesses");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .protected_range.begin = 100;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witness protected-range "
        "mismatch");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.forbidden_uses.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects checked leakage proofs without "
        "forbidden uses");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.forbidden_uses.push_back(
      tampered_leakage_certificate.leakage.forbidden_uses.front());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects duplicate leakage forbidden "
        "uses");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.forbidden_uses.push_back("future_magic");
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects unknown leakage forbidden uses");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.forbidden_uses = {"target_supervision"};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses whose use is "
        "not declared forbidden");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front().fact_digest =
      "not_in_closure";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses outside the "
        "checkpoint closure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .fact_digest.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses without fact "
        "digests");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front().job_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses without "
        "runtime job identity");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .wave_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses without "
        "runtime wave identity");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .target_component.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses without target "
        "components");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .target_component = "wikimyei.inference.expected_value.mdn";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses whose "
        "component disagrees with the causal exposure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front().split_name =
      "train_core";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses whose split "
        "disagrees with the causal exposure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .source_footprint = exposure::anchor_interval_t{.begin = 99, .end = 100};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses whose source "
        "footprint disagrees with the causal exposure");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .source_footprint = exposure::anchor_interval_t{.begin = 200, .end = 210};
  tampered_leakage_certificate.leakage.overlap_witnesses.front().intersection =
      exposure::anchor_interval_t{};
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses without a "
        "non-empty protected-footprint intersection");
  tampered_leakage_certificate = boundary_forbidden.proof_certificate;
  tampered_leakage_certificate.certificate_digest.clear();
  tampered_leakage_certificate.leakage.overlap_witnesses.front()
      .mutated_component = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_leakage_certificate)
             .passed,
        "proof certificate self-check rejects leakage witnesses missing "
        "required mutation");

  target::lattice_target_evaluator_options_t missing_ledger_options =
      auto_ledger_options;
  missing_ledger_options.auto_build_exposure_ledger = false;
  target::lattice_target_evaluator_t missing_ledger_eval(
      coverage_specs, missing_ledger_options);
  const auto missing_ledger =
      missing_ledger_eval.evaluate("ranged_representation_ready");
  check(missing_ledger.status == target::lattice_target_status_t::blocked,
        "coverage targets should block when no exposure ledger is provided");
  check(std::any_of(missing_ledger.deficits.begin(),
                    missing_ledger.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:exposure_ledger" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing" &&
                             deficit.priority == 35 &&
                             deficit.priority_class == "artifact";
                    }),
        "coverage targets without a ledger should expose an exposure-ledger "
        "artifact deficit");
  expect_plan_basis_projects_deficits(
      missing_ledger, "missing exposure ledger should project proof deficits");

  exposure::lattice_exposure_ledger_t empty_ledger{};
  target::lattice_target_evaluator_options_t empty_ledger_options =
      exposure_options;
  empty_ledger_options.exposure_ledger = &empty_ledger;
  target::lattice_target_evaluator_t empty_ledger_eval(coverage_specs,
                                                       empty_ledger_options);
  const auto empty_ledger_ready =
      empty_ledger_eval.evaluate("ranged_representation_ready");
  check(empty_ledger_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "coverage targets should fail when the ledger has no checkpoint "
        "closure facts");
  check(std::any_of(empty_ledger_ready.deficits.begin(),
                    empty_ledger_ready.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "artifact:checkpoint_closure_fact" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "missing checkpoint closure facts should expose an artifact deficit");
  expect_plan_basis_projects_deficits(
      empty_ledger_ready,
      "missing checkpoint closure facts should project proof deficits");

  exposure::lattice_exposure_ledger_t partial_ledger{};
  partial_ledger.add(make_rep_exposure_fact(exposure_checkpoint, 0, 50));
  target::lattice_target_evaluator_options_t partial_options = exposure_options;
  partial_options.exposure_ledger = &partial_ledger;
  target::lattice_target_evaluator_t partial_eval(coverage_specs,
                                                  partial_options);
  const auto partial_ready =
      partial_eval.evaluate("ranged_representation_ready");
  check(partial_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "partial observed_input exposure coverage should fail target");
  check(!partial_ready.exposure_summaries.empty() &&
            partial_ready.exposure_summaries.front().unique_covered_anchors ==
                50,
        "failed partial target still reports exposure-load summary");
  check(!partial_ready.deficits.empty() &&
            partial_ready.deficits.front().key == "coverage:observed_input" &&
            partial_ready.deficits.front().kind == "coverage" &&
            partial_ready.deficits.front().dimension == "observed_input" &&
            partial_ready.deficits.front().required == 1.0 &&
            partial_ready.deficits.front().actual == 0.5 &&
            partial_ready.deficits.front().priority == 60 &&
            partial_ready.deficits.front().priority_class == "coverage",
        "partial coverage failure should include a coverage deficit");
  check(partial_ready.proof_certificate.coverage.front().missing_anchors ==
                50 &&
            partial_ready.proof_certificate.coverage.front()
                    .missing_intervals.size() == 1 &&
            partial_ready.proof_certificate.coverage.front()
                    .missing_intervals.front()
                    .begin == 50 &&
            partial_ready.proof_certificate.coverage.front()
                    .missing_intervals.front()
                    .end == 100,
        "partial coverage proof records missing cursor intervals");
  check(partial_ready.deficits.front().missing_intervals.size() == 1 &&
            partial_ready.deficits.front().missing_intervals.front().begin ==
                50 &&
            partial_ready.deficits.front().missing_intervals.front().end == 100,
        "coverage deficit exposes missing cursor intervals");
  check(partial_ready.plan_basis.available &&
            partial_ready.plan_basis.deficit_keys.size() == 1 &&
            partial_ready.plan_basis.deficit_keys.front() ==
                "coverage:observed_input" &&
            partial_ready.plan_basis.deficit_priority_classes.size() == 1 &&
            partial_ready.plan_basis.deficit_priority_classes.front() ==
                "coverage" &&
            partial_ready.plan_basis.primary_deficit_key ==
                "coverage:observed_input" &&
            partial_ready.plan_basis.primary_deficit_priority == 60 &&
            partial_ready.plan_basis.primary_deficit_priority_class ==
                "coverage" &&
            partial_ready.plan_basis.target_range.begin == 0 &&
            partial_ready.plan_basis.target_range.end == 100 &&
            partial_ready.plan_basis.missing_intervals.size() == 1 &&
            partial_ready.plan_basis.missing_intervals.front().begin == 50 &&
            partial_ready.plan_basis.missing_intervals.front().end == 100,
        "coverage plan basis carries the cursor deficit interval");
  expect_plan_basis_projects_deficits(
      partial_ready, "coverage plan basis should project proof deficits");
  check(!partial_ready.proof_certificate.certificate_digest.empty() &&
            partial_ready.proof_certificate.certificate_digest !=
                exposure_ready.proof_certificate.certificate_digest,
        "proof certificate digest changes when proof coverage changes");

  exposure::lattice_exposure_ledger_t untrusted_ledger{};
  auto untrusted_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  untrusted_fact.coverage_precision = "requested_range_untrusted_v0";
  untrusted_fact.completed_anchor_range = {};
  untrusted_ledger.add(untrusted_fact);
  target::lattice_target_evaluator_options_t untrusted_options =
      exposure_options;
  untrusted_options.exposure_ledger = &untrusted_ledger;
  target::lattice_target_evaluator_t untrusted_eval(coverage_specs,
                                                    untrusted_options);
  const auto untrusted_ready =
      untrusted_eval.evaluate("ranged_representation_ready");
  check(untrusted_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "untrusted requested-range exposure must not satisfy target coverage");

  exposure::lattice_exposure_ledger_t wrong_graph_ledger{};
  wrong_graph_ledger.add(
      make_rep_exposure_fact(exposure_checkpoint, 0, 100, "graph_old"));
  target::lattice_target_evaluator_options_t wrong_graph_options =
      exposure_options;
  wrong_graph_options.exposure_ledger = &wrong_graph_ledger;
  target::lattice_target_evaluator_t wrong_graph_eval(coverage_specs,
                                                      wrong_graph_options);
  const auto wrong_graph_ready =
      wrong_graph_eval.evaluate("ranged_representation_ready");
  check(wrong_graph_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "same anchor exposure under a different graph order must not satisfy "
        "target coverage");
  check(std::any_of(wrong_graph_ready.deficits.begin(),
                    wrong_graph_ready.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:exposure_fact" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "graph/source-incompatible ledger facts should expose an exposure-fact "
        "artifact deficit");
  expect_plan_basis_projects_deficits(
      wrong_graph_ready,
      "graph/source-incompatible facts should project proof deficits");

  exposure::lattice_exposure_ledger_t wrong_cursor_ledger{};
  wrong_cursor_ledger.add(make_rep_exposure_fact(exposure_checkpoint, 0, 100,
                                                 "graph_1", "cursor_old"));
  target::lattice_target_evaluator_options_t wrong_cursor_options =
      exposure_options;
  wrong_cursor_options.exposure_ledger = &wrong_cursor_ledger;
  target::lattice_target_evaluator_t wrong_cursor_eval(coverage_specs,
                                                       wrong_cursor_options);
  const auto wrong_cursor_ready =
      wrong_cursor_eval.evaluate("ranged_representation_ready");
  check(wrong_cursor_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "same anchor exposure under a different source cursor must not satisfy "
        "target coverage");

  exposure::lattice_exposure_ledger_t stale_split_ledger{};
  auto stale_split_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  stale_split_fact.split_policy_fingerprint = "old_split_policy";
  stale_split_ledger.add(stale_split_fact);
  target::lattice_target_evaluator_options_t stale_split_options =
      exposure_options;
  stale_split_options.split_policy = split_policy;
  stale_split_options.exposure_ledger = &stale_split_ledger;
  target::lattice_target_evaluator_t stale_split_eval(coverage_specs,
                                                      stale_split_options);
  const auto stale_split_ready =
      stale_split_eval.evaluate("ranged_representation_ready");
  check(stale_split_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "exposure under a different recorded split policy must not satisfy "
        "target coverage");

  exposure::lattice_exposure_ledger_t split_identity_missing_ledger{};
  auto split_identity_missing_fact =
      make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  split_identity_missing_fact.split_name = "train_core";
  split_identity_missing_fact.split_policy_fingerprint.clear();
  split_identity_missing_ledger.add(split_identity_missing_fact);
  target::lattice_target_evaluator_options_t split_identity_missing_options =
      exposure_options;
  split_identity_missing_options.split_policy = split_policy;
  split_identity_missing_options.exposure_ledger =
      &split_identity_missing_ledger;
  target::lattice_target_evaluator_t split_identity_missing_eval(
      coverage_specs, split_identity_missing_options);
  const auto split_identity_missing_ready =
      split_identity_missing_eval.evaluate("ranged_representation_ready");
  check(split_identity_missing_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "concrete split exposure without split-policy identity must not "
        "satisfy target coverage");

  exposure::lattice_exposure_ledger_t unknown_split_policy_ledger{};
  auto unknown_split_policy_fact =
      make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  unknown_split_policy_fact.split_name = "unknown";
  unknown_split_policy_fact.split_policy_fingerprint.clear();
  unknown_split_policy_ledger.add(unknown_split_policy_fact);
  target::lattice_target_evaluator_options_t unknown_split_policy_options =
      exposure_options;
  unknown_split_policy_options.split_policy = split_policy;
  unknown_split_policy_options.exposure_ledger = &unknown_split_policy_ledger;
  target::lattice_target_evaluator_t unknown_split_policy_eval(
      coverage_specs, unknown_split_policy_options);
  const auto unknown_split_policy_ready =
      unknown_split_policy_eval.evaluate("ranged_representation_ready");
  check(unknown_split_policy_ready.status ==
            target::lattice_target_status_t::satisfied,
        "legacy unknown-split exposure without split-policy identity can still "
        "satisfy target coverage");

  const auto forbid_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_representation_no_validation_leak;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 100;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 20;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 30;
  FORBID_EXPOSURE_USES = observed_input|target_supervision;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  target::lattice_target_evaluator_t forbid_eval(forbid_specs,
                                                 exposure_options);
  const auto forbidden =
      forbid_eval.evaluate("ranged_representation_no_validation_leak");
  check(forbidden.status == target::lattice_target_status_t::exposure_failed,
        "forbidden exposure overlap should fail target");
  check(!forbidden.deficits.empty() &&
            forbidden.deficits.front().key == "leakage:protected_split" &&
            forbidden.deficits.front().kind == "leakage" &&
            forbidden.deficits.front().priority == 50 &&
            forbidden.deficits.front().priority_class == "leakage" &&
            forbidden.deficits.front().range.begin == 20 &&
            forbidden.deficits.front().range.end == 30,
        "leakage deficit keeps the protected source-row witness range");
  check(forbidden.plan_basis.target_range.empty() &&
            forbidden.plan_basis.primary_deficit_key ==
                "leakage:protected_split" &&
            forbidden.plan_basis.primary_deficit_priority == 50 &&
            forbidden.plan_basis.primary_deficit_priority_class == "leakage" &&
            forbidden.plan_basis.deficit_priority_classes.size() == 1 &&
            forbidden.plan_basis.deficit_priority_classes.front() ==
                "leakage" &&
            forbidden.plan_basis.missing_intervals.empty(),
        "plan basis should not present leakage source-row ranges as cursor "
        "coverage deficits");
  expect_plan_basis_projects_deficits(
      forbidden, "leakage-failed plan basis should project proof deficits");

  const auto mdn_exposure_root = make_tmp_dir("mdn_exposure");
  const auto mdn_exposure_checkpoint = mdn_exposure_root / "mdn_ranged.pt";
  const auto mdn_rep_checkpoint = mdn_exposure_root / "rep_ranged.pt";
  write_ranged_mdn_job(mdn_exposure_root, mdn_exposure_checkpoint,
                       mdn_rep_checkpoint, 100, 200);
  const auto mdn_coverage_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = imbalanced_node_support;
  KIND = node_support_balance;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  METRIC = valid_target_count_gini;
  ABOVE = 0.25;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = low_node_support_entropy;
  KIND = node_support_balance;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  METRIC = valid_target_count_normalized_entropy;
  BELOW = 0.25;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = weak_node_support;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  METRIC = weakest_valid_target_count;
  BELOW = 5;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = weak_left_half_node_support;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 150;
  METRIC = weakest_valid_target_count;
  BELOW = 2;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = low_aggregate_node_support_confidence;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  METRIC = valid_target_wilson_lower_95;
  BELOW = 0.80;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = missing_node_fraction_support;
  KIND = node_support_floor;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  METRIC = min_valid_target_fraction;
  BELOW = 0.10;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = high_mdn_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  EFFECT = mutated_component;
  METRIC = mean_nll;
  ABOVE = 0.35;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = high_per_node_mdn_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  EFFECT = mutated_component;
  METRIC = per_node_mean_nll_max;
  ABOVE = 0.35;
};

LATTICE_WARN {
  TARGET_ID = ranged_node_mdn_ready;
  WARNING_ID = unavailable_mdn_pit_statistic;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  EFFECT = mutated_component;
  METRIC = pit_ks_statistic;
  ABOVE = 0.20;
};
)DSL");
  exposure::lattice_exposure_ledger_t mdn_ledger{};
  const auto mdn_rep_fact =
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200);
  const auto mdn_rep_fact_digest = exposure::exposure_fact_digest(mdn_rep_fact);
  mdn_ledger.add(mdn_rep_fact);
  mdn_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(mdn_rep_fact));
  auto mdn_target_fact = make_mdn_exposure_fact(mdn_exposure_checkpoint,
                                                mdn_rep_checkpoint, 100, 200);
  const auto mdn_target_fact_digest =
      exposure::exposure_fact_digest(mdn_target_fact);
  mdn_ledger.add(mdn_target_fact);
  mdn_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(mdn_target_fact));
  for (std::int64_t i = 0; i < 2; ++i) {
    exposure::lattice_node_exposure_fact_t node_fact{};
    node_fact.parent_exposure_fact_digest = mdn_target_fact_digest;
    node_fact.contract_fingerprint = "contract_1";
    node_fact.graph_order_fingerprint = "graph_1";
    node_fact.source_cursor_token = "cursor_ranged";
    node_fact.component_assembly_fingerprint = "mdn_1";
    node_fact.target_component = "wikimyei.inference.expected_value.mdn";
    node_fact.job_id = "mdn_ranged";
    node_fact.wave_id = "wave_mdn_ranged";
    node_fact.job_status = "completed";
    node_fact.wave_action = "train";
    node_fact.node_id = i == 0 ? "BTC" : "ETH";
    node_fact.node_index = i;
    node_fact.anchor_range = mdn_target_fact.anchor_range;
    node_fact.completed_anchor_range = mdn_target_fact.completed_anchor_range;
    node_fact.use = mdn_target_fact.use;
    node_fact.active_row_count = i == 0 ? 100 : 10;
    node_fact.trained_row_count = node_fact.active_row_count;
    node_fact.valid_target_count = i == 0 ? 80 : 2;
    node_fact.valid_target_fraction = i == 0 ? 0.80 : 0.20;
    node_fact.mean_nll = i == 0 ? 0.30 : 0.40;
    node_fact.output_checkpoint = mdn_exposure_checkpoint;
    mdn_ledger.add_node(node_fact);
  }
  target::lattice_target_evaluator_options_t mdn_exposure_options = options;
  mdn_exposure_options.runtime_root = mdn_exposure_root;
  mdn_exposure_options.exposure_ledger = &mdn_ledger;
  target::lattice_target_evaluator_t mdn_exposure_eval(mdn_coverage_specs,
                                                       mdn_exposure_options);
  const auto mdn_exposure_ready =
      mdn_exposure_eval.evaluate("ranged_node_mdn_ready");
  check(mdn_exposure_ready.status == target::lattice_target_status_t::satisfied,
        "target supervision coverage should use anchor coverage, not shifted "
        "future rows");
  check(mdn_exposure_ready.exposure_summaries.size() == 1 &&
            mdn_exposure_ready.exposure_summaries.front().use ==
                exposure::exposure_use_t::target_supervision,
        "node MDN summary reports target supervision use");
  check(mdn_exposure_ready.exposure_summaries.front().component_scope ==
            "target_component",
        "node MDN summary is target-component scoped");
  check(mdn_exposure_ready.exposure_summaries.front().loaded_anchor_events ==
            100,
        "node MDN summary counts local target-supervision anchor events");
  const auto [mdn_ready_support_lower, mdn_ready_support_upper] =
      exposure::wilson_score_interval_95(25, 100);
  check(mdn_exposure_ready.exposure_summaries.front()
                    .valid_target_success_count_for_uncertainty == 25 &&
            mdn_exposure_ready.exposure_summaries.front()
                    .valid_target_opportunity_count_for_uncertainty == 100 &&
            std::abs(mdn_exposure_ready.exposure_summaries.front()
                         .valid_target_fraction_estimate -
                     0.25) < 1e-12 &&
            std::abs(mdn_exposure_ready.exposure_summaries.front()
                         .valid_target_wilson_lower_95 -
                     mdn_ready_support_lower) < 1e-12 &&
            std::abs(mdn_exposure_ready.exposure_summaries.front()
                         .valid_target_wilson_upper_95 -
                     mdn_ready_support_upper) < 1e-12,
        "node MDN exposure summary carries valid-target uncertainty interval");
  auto tampered_support_certificate = mdn_exposure_ready.proof_certificate;
  tampered_support_certificate.certificate_digest.clear();
  tampered_support_certificate.coverage.front()
      .load_summary.valid_target_wilson_lower_95 = 0.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_support_certificate)
             .passed,
        "proof certificate self-check rejects tampered valid-target "
        "uncertainty interval");
  tampered_support_certificate = mdn_exposure_ready.proof_certificate;
  tampered_support_certificate.certificate_digest.clear();
  tampered_support_certificate.coverage.front()
      .load_summary.valid_target_count_total = 20;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_support_certificate)
             .passed,
        "proof certificate self-check rejects valid-target uncertainty "
        "successes above total support");
  tampered_support_certificate = mdn_exposure_ready.proof_certificate;
  tampered_support_certificate.certificate_digest.clear();
  tampered_support_certificate.coverage.front()
      .load_summary.valid_target_opportunity_count_for_uncertainty = 20;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_support_certificate)
             .passed,
        "proof certificate self-check rejects valid-target uncertainty "
        "successes above opportunities");
  tampered_support_certificate = mdn_exposure_ready.proof_certificate;
  tampered_support_certificate.certificate_digest.clear();
  tampered_support_certificate.coverage.front()
      .load_summary.valid_target_fraction_estimate = 2.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_support_certificate)
             .passed,
        "proof certificate self-check rejects out-of-range valid-target "
        "uncertainty fractions");
  check(
      mdn_exposure_ready.node_support_summaries.size() == 1 &&
          mdn_exposure_ready.node_support_summaries.front().node_count == 2 &&
          mdn_exposure_ready.node_support_summaries.front().unique_node_count ==
              2 &&
          mdn_exposure_ready.node_support_summaries.front()
                  .target_supervision_support_row_count == 2 &&
          mdn_exposure_ready.node_support_summaries.front()
                  .mutating_support_row_count == 2 &&
          mdn_exposure_ready.node_support_summaries.front()
                  .non_mutating_support_row_count == 0 &&
          mdn_exposure_ready.node_support_summaries.front()
                  .weakest_valid_target_node_id == "ETH",
      "node MDN readiness includes derived node-support matrix summary");
  check(mdn_exposure_ready.proof_certificate.node_support_summaries.size() ==
                1 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .node_count == 2 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .unique_node_count == 2 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .target_supervision_support_row_count == 2 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .mutating_support_row_count == 2 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .weakest_valid_target_node_id == "ETH" &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .weakest_valid_target_denominator == 10,
        "node MDN certificate includes derived node-support matrix summary");
  const auto [expected_mdn_node_support_lower,
              expected_mdn_node_support_upper] =
      exposure::wilson_score_interval_95(82, 110);
  check(std::abs(
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                .valid_target_fraction_estimate -
            (82.0 / 110.0)) < 1e-12 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .valid_target_opportunity_count_total == 110 &&
            std::abs(mdn_exposure_ready.proof_certificate.node_support_summaries
                         .front()
                         .valid_target_wilson_lower_95 -
                     expected_mdn_node_support_lower) < 1e-12 &&
            std::abs(mdn_exposure_ready.proof_certificate.node_support_summaries
                         .front()
                         .valid_target_wilson_upper_95 -
                     expected_mdn_node_support_upper) < 1e-12,
        "node MDN certificate includes aggregate node-support Wilson interval");
  const auto [expected_weak_mdn_node_lower, expected_weak_mdn_node_upper] =
      exposure::wilson_score_interval_95(2, 10);
  (void)expected_weak_mdn_node_upper;
  check(std::abs(mdn_exposure_ready.evidence_order_vector
                     .aggregate_valid_target_wilson_lower_95 -
                 expected_mdn_node_support_lower) < 1e-12 &&
            std::abs(mdn_exposure_ready.evidence_order_vector
                         .weakest_node_valid_target_wilson_lower_95 -
                     expected_weak_mdn_node_lower) < 1e-12 &&
            std::abs(mdn_exposure_ready.evidence_order_vector
                         .max_node_support_coefficient_of_variation -
                     mdn_exposure_ready.proof_certificate.node_support_summaries
                         .front()
                         .valid_target_count_coefficient_of_variation) <
                1e-12 &&
            std::abs(
                mdn_exposure_ready.evidence_order_vector.max_node_support_gini -
                mdn_exposure_ready.proof_certificate.node_support_summaries
                    .front()
                    .valid_target_count_gini) < 1e-12,
        "evidence order vector exposes aggregate and weakest-node confidence "
        "plus node-support imbalance dimensions");
  const double expected_mdn_node_entropy =
      -((80.0 / 82.0) * std::log(80.0 / 82.0) +
        (2.0 / 82.0) * std::log(2.0 / 82.0)) /
      std::log(2.0);
  check(std::abs(
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                .valid_target_count_normalized_entropy -
            expected_mdn_node_entropy) < 1e-12,
        "node MDN certificate includes normalized support entropy");
  check(mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .support_rows.size() == 2 &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .support_rows.front()
                    .node_id == "BTC" &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .support_rows.front()
                    .parent_exposure_fact_digest == mdn_target_fact_digest &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .support_rows.back()
                    .node_id == "ETH" &&
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                    .support_rows.back()
                    .parent_exposure_fact_digest == mdn_target_fact_digest,
        "node MDN certificate carries ordered, provenance-bound support matrix "
        "rows");
  const auto wide_node_root = make_tmp_dir("wide_node_support");
  const auto wide_node_mdn_checkpoint = wide_node_root / "wide_mdn.pt";
  const auto wide_node_rep_checkpoint = wide_node_root / "wide_rep.pt";
  write_ranged_mdn_job(wide_node_root, wide_node_mdn_checkpoint,
                       wide_node_rep_checkpoint, 100, 200);
  exposure::lattice_exposure_ledger_t wide_node_ledger{};
  const auto wide_node_rep_fact =
      make_rep_exposure_fact(wide_node_rep_checkpoint, 100, 300);
  wide_node_ledger.add(wide_node_rep_fact);
  wide_node_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(wide_node_rep_fact));
  const auto wide_node_mdn_fact = make_mdn_exposure_fact(
      wide_node_mdn_checkpoint, wide_node_rep_checkpoint, 100, 300);
  const auto wide_node_mdn_digest =
      exposure::exposure_fact_digest(wide_node_mdn_fact);
  wide_node_ledger.add(wide_node_mdn_fact);
  wide_node_ledger.add_checkpoint(
      exposure::make_checkpoint_fact_from_exposure_fact(wide_node_mdn_fact));
  auto outside_mdn_warning_fact = make_mdn_exposure_fact(
      wide_node_mdn_checkpoint, wide_node_rep_checkpoint, 200, 300);
  outside_mdn_warning_fact.job_id = "mdn_ranged_outside_warning";
  outside_mdn_warning_fact.wave_id = "wave_mdn_ranged_outside_warning";
  outside_mdn_warning_fact.output_checkpoint =
      wide_node_root / "outside_mdn.pt";
  outside_mdn_warning_fact.mean_nll = 9.0;
  wide_node_ledger.add(outside_mdn_warning_fact);
  for (std::int64_t i = 0; i < 2; ++i) {
    exposure::lattice_node_exposure_fact_t node_fact{};
    node_fact.parent_exposure_fact_digest = wide_node_mdn_digest;
    node_fact.contract_fingerprint = "contract_1";
    node_fact.graph_order_fingerprint = "graph_1";
    node_fact.source_cursor_token = "cursor_ranged";
    node_fact.component_assembly_fingerprint = "mdn_1";
    node_fact.target_component = "wikimyei.inference.expected_value.mdn";
    node_fact.job_id = "mdn_ranged";
    node_fact.wave_id = "wave_mdn_ranged";
    node_fact.job_status = "completed";
    node_fact.wave_action = "train";
    node_fact.node_id = i == 0 ? "BTC" : "ETH";
    node_fact.node_index = i;
    node_fact.anchor_range =
        exposure::anchor_interval_t{.begin = 100, .end = 300};
    node_fact.completed_anchor_range = node_fact.anchor_range;
    node_fact.use = wide_node_mdn_fact.use;
    node_fact.active_row_count = i == 0 ? 100 : 20;
    node_fact.trained_row_count = node_fact.active_row_count;
    node_fact.valid_target_count = i == 0 ? 80 : 10;
    node_fact.valid_target_opportunity_count = i == 0 ? 100 : 20;
    node_fact.valid_target_fraction = i == 0 ? 0.80 : 0.50;
    node_fact.output_checkpoint = wide_node_mdn_checkpoint;
    wide_node_ledger.add_node(node_fact);
  }
  auto outside_node_fact = wide_node_ledger.node_facts().front();
  outside_node_fact.node_id = "SOL";
  outside_node_fact.node_index = 2;
  outside_node_fact.anchor_range =
      exposure::anchor_interval_t{.begin = 200, .end = 300};
  outside_node_fact.completed_anchor_range = outside_node_fact.anchor_range;
  outside_node_fact.active_row_count = 1000;
  outside_node_fact.trained_row_count = 1000;
  outside_node_fact.valid_target_count = 1000;
  outside_node_fact.valid_target_opportunity_count = 1000;
  outside_node_fact.valid_target_fraction = 1.0;
  wide_node_ledger.add_node(outside_node_fact);
  target::lattice_target_evaluator_options_t wide_node_options = options;
  wide_node_options.runtime_root = wide_node_root;
  wide_node_options.exposure_ledger = &wide_node_ledger;
  target::lattice_target_evaluator_t wide_node_eval(mdn_coverage_specs,
                                                    wide_node_options);
  const auto wide_node_ready = wide_node_eval.evaluate("ranged_node_mdn_ready");
  const auto [wide_node_lower, wide_node_upper] =
      exposure::wilson_score_interval_95(45, 60);
  check(wide_node_ready.status == target::lattice_target_status_t::satisfied &&
            wide_node_ready.proof_certificate.node_support_summaries.size() ==
                1 &&
            wide_node_ready.proof_certificate.node_support_summaries.front()
                    .node_count == 2 &&
            wide_node_ready.proof_certificate.node_support_summaries.front()
                    .active_row_count_total == 60 &&
            wide_node_ready.proof_certificate.node_support_summaries.front()
                    .valid_target_count_total == 45 &&
            wide_node_ready.proof_certificate.node_support_summaries.front()
                    .valid_target_opportunity_count_total == 60 &&
            wide_node_ready.proof_certificate.node_support_summaries.front()
                    .weakest_valid_target_count == 5 &&
            std::abs(
                wide_node_ready.proof_certificate.node_support_summaries.front()
                    .valid_target_wilson_lower_95 -
                wide_node_lower) < 1e-12 &&
            std::abs(
                wide_node_ready.proof_certificate.node_support_summaries.front()
                    .valid_target_wilson_upper_95 -
                wide_node_upper) < 1e-12,
        "node MDN certificate range-scopes wide node-support facts to the "
        "target anchor interval");
  const auto wide_node_mdn_nll_warning = std::find_if(
      wide_node_ready.warning_results.begin(),
      wide_node_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "high_mdn_mean_nll";
      });
  check(wide_node_mdn_nll_warning != wide_node_ready.warning_results.end() &&
            wide_node_mdn_nll_warning->exposure_summary.fact_count == 1 &&
            std::abs(wide_node_mdn_nll_warning->measured_value - 0.40) < 1e-12,
        "MDN distribution warnings ignore outside-checkpoint facts outside the "
        "target anchor interval");
  const auto &checkpoint_previews =
      mdn_exposure_ready.proof_certificate.closure.checkpoint_identity_previews;
  check(
      mdn_exposure_ready.proof_certificate.closure.resolution_authority ==
              "checkpoint_id_file_digest" &&
          !mdn_exposure_ready.proof_certificate.closure.legacy_path_fallback &&
          !mdn_exposure_ready.proof_certificate.closure.root_checkpoint_id
               .empty() &&
          !mdn_exposure_ready.proof_certificate.closure
               .root_checkpoint_file_digest.empty(),
      "node MDN certificate records checkpoint id/digest as closure "
      "authority when complete checkpoint facts are available");
  check(checkpoint_previews.size() == 2,
        "node MDN certificate includes checkpoint identity previews for "
        "closure checkpoints");
  const auto mdn_checkpoint_preview =
      std::find_if(checkpoint_previews.begin(), checkpoint_previews.end(),
                   [&](const auto &preview) {
                     return preview.checkpoint_path == mdn_exposure_checkpoint;
                   });
  check(mdn_checkpoint_preview != checkpoint_previews.end() &&
            !mdn_checkpoint_preview->checkpoint_id.empty() &&
            !mdn_checkpoint_preview->checkpoint_file_digest.empty() &&
            mdn_checkpoint_preview->component ==
                "wikimyei.inference.expected_value.mdn" &&
            mdn_checkpoint_preview->direct_exposure_digest ==
                mdn_target_fact_digest,
        "node MDN certificate binds the MDN checkpoint identity preview to "
        "the direct exposure digest");
  const auto rep_checkpoint_preview =
      std::find_if(checkpoint_previews.begin(), checkpoint_previews.end(),
                   [&](const auto &preview) {
                     return preview.checkpoint_path == mdn_rep_checkpoint;
                   });
  check(rep_checkpoint_preview != checkpoint_previews.end() &&
            rep_checkpoint_preview->component ==
                "wikimyei.representation.encoding.vicreg" &&
            rep_checkpoint_preview->direct_exposure_digest ==
                mdn_rep_fact_digest,
        "node MDN certificate includes upstream representation checkpoint "
        "identity preview");
  const auto [expected_mdn_node_lower, expected_mdn_node_upper] =
      exposure::wilson_score_interval_95(2, 10);
  check(std::abs(
            mdn_exposure_ready.proof_certificate.node_support_summaries.front()
                .weakest_valid_target_wilson_lower_95 -
            expected_mdn_node_lower) < 1e-12 &&
            std::abs(mdn_exposure_ready.proof_certificate.node_support_summaries
                         .front()
                         .weakest_valid_target_wilson_upper_95 -
                     expected_mdn_node_upper) < 1e-12,
        "node MDN certificate includes weakest-node Wilson support interval");
  check(mdn_exposure_ready.proof_certificate_check.passed,
        "node MDN certificate passes local self-check");
  auto tampered_checkpoint_preview_certificate =
      mdn_exposure_ready.proof_certificate;
  tampered_checkpoint_preview_certificate.certificate_digest.clear();
  tampered_checkpoint_preview_certificate.closure.checkpoint_identity_previews
      .front()
      .direct_exposure_digest = "not_in_closure";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_checkpoint_preview_certificate)
             .passed,
        "proof certificate self-check rejects checkpoint identity previews "
        "whose direct exposure digest is not in the closure");
  tampered_checkpoint_preview_certificate =
      mdn_exposure_ready.proof_certificate;
  tampered_checkpoint_preview_certificate.certificate_digest.clear();
  tampered_checkpoint_preview_certificate.closure.checkpoint_identity_previews
      .front()
      .component = "wikimyei.representation.encoding.vicreg";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_checkpoint_preview_certificate)
             .passed,
        "proof certificate self-check rejects checkpoint identity previews "
        "whose component disagrees with causal closure");
  auto tampered_closure_authority_certificate =
      mdn_exposure_ready.proof_certificate;
  tampered_closure_authority_certificate.certificate_digest.clear();
  tampered_closure_authority_certificate.closure.legacy_path_fallback = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_authority_certificate)
             .passed,
        "proof certificate self-check rejects id/digest closure authority with "
        "legacy path fallback");
  tampered_closure_authority_certificate = mdn_exposure_ready.proof_certificate;
  tampered_closure_authority_certificate.certificate_digest.clear();
  tampered_closure_authority_certificate.closure.identity_mismatches.push_back(
      "tampered identity mismatch");
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_authority_certificate)
             .passed,
        "proof certificate self-check rejects complete closure proofs with "
        "identity mismatches");
  auto node_summary_order_certificate = mdn_exposure_ready.proof_certificate;
  auto second_node_summary =
      node_summary_order_certificate.node_support_summaries.front();
  second_node_summary.split_name = "validation_holdout";
  second_node_summary.weakest_valid_target_node_id = "BTC";
  node_summary_order_certificate.node_support_summaries.push_back(
      second_node_summary);
  const auto node_summary_order_digest =
      target::lattice_target_proof_certificate_digest(
          node_summary_order_certificate);
  std::reverse(node_summary_order_certificate.node_support_summaries.begin(),
               node_summary_order_certificate.node_support_summaries.end());
  check(node_summary_order_digest ==
            target::lattice_target_proof_certificate_digest(
                node_summary_order_certificate),
        "proof certificate digest is canonical across node-support summary "
        "order");
  auto tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.push_back(
      tampered_node_certificate.node_support_summaries.front());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects duplicate node-support "
        "summaries");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  auto &empty_node_summary =
      tampered_node_certificate.node_support_summaries.front();
  empty_node_summary.node_count = 0;
  empty_node_summary.unique_node_count = 0;
  empty_node_summary.routed_row_count_total = 0;
  empty_node_summary.active_row_count_total = 0;
  empty_node_summary.trained_row_count_total = 0;
  empty_node_summary.evaluated_row_count_total = 0;
  empty_node_summary.valid_target_count_total = 0;
  empty_node_summary.valid_target_opportunity_count_total = 0;
  empty_node_summary.valid_target_fraction_estimate =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.valid_target_wilson_lower_95 =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.valid_target_wilson_upper_95 =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.finite_valid_target_fraction_count = 0;
  empty_node_summary.min_valid_target_fraction =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.max_valid_target_fraction =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.mean_valid_target_fraction =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.weakest_valid_target_node_id.clear();
  empty_node_summary.weakest_valid_target_node_index = -1;
  empty_node_summary.weakest_valid_target_count = 0;
  empty_node_summary.weakest_valid_target_denominator = 0;
  empty_node_summary.weakest_valid_target_fraction =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.weakest_valid_target_wilson_lower_95 =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.weakest_valid_target_wilson_upper_95 =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.valid_target_count_mean =
      std::numeric_limits<double>::quiet_NaN();
  empty_node_summary.valid_target_count_coefficient_of_variation = 0.0;
  empty_node_summary.valid_target_count_gini = 0.0;
  empty_node_summary.valid_target_count_normalized_entropy = 0.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects empty node-support summaries");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_mean = -1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent node support "
        "summary arithmetic");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_fraction_estimate = 2.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects out-of-range aggregate "
        "node-support fraction estimates");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_wilson_lower_95 = 0.90;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects aggregate node-support Wilson "
        "interval mismatches");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front().schema =
      "kikijyeba.lattice.node_support_summary.v0";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects unknown node-support summary "
        "schemas");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front().target_component =
      "wikimyei.representation.encoding.vicreg";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries attached "
        "to non-MDN components");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.closure.causal_exposures.erase(
      std::remove_if(tampered_node_certificate.closure.causal_exposures.begin(),
                     tampered_node_certificate.closure.causal_exposures.end(),
                     [](const target::lattice_target_proof_certificate_t::
                            closure_proof_t::causal_exposure_t &causal) {
                       return causal.target_component ==
                              "wikimyei.inference.expected_value.mdn";
                     }),
      tampered_node_certificate.closure.causal_exposures.end());
  tampered_node_certificate.closure.fact_digests.clear();
  for (const auto &causal :
       tampered_node_certificate.closure.causal_exposures) {
    tampered_node_certificate.closure.fact_digests.push_back(
        causal.fact_digest);
  }
  tampered_node_certificate.closure.fact_count = static_cast<std::int64_t>(
      tampered_node_certificate.closure.causal_exposures.size());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries without "
        "causal MDN exposure");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .use.target_supervision = false;
  tampered_node_certificate.node_support_summaries.front()
      .use.evaluation_metric = true;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries without "
        "matching causal MDN exposure use");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .use.mutated_component = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries without "
        "matching causal MDN mutation backing");
  auto non_mutating_node_support_certificate =
      mdn_exposure_ready.proof_certificate;
  non_mutating_node_support_certificate.certificate_digest.clear();
  auto &non_mutating_summary =
      non_mutating_node_support_certificate.node_support_summaries.front();
  non_mutating_summary.use.mutated_component = false;
  non_mutating_summary.mutating_support_row_count = 0;
  non_mutating_summary.non_mutating_support_row_count =
      non_mutating_summary.node_count;
  for (auto &row : non_mutating_summary.support_rows) {
    row.use.mutated_component = false;
  }
  non_mutating_node_support_certificate.certificate_digest =
      target::lattice_target_proof_certificate_digest(
          non_mutating_node_support_certificate);
  check(target::verify_lattice_target_proof_certificate(
            non_mutating_node_support_certificate)
            .passed,
        "proof certificate self-check accepts internally consistent "
        "non-mutating node-support summaries without causal mutation backing");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front().split_name =
      "validation_holdout";
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries without "
        "matching causal MDN exposure split");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front().unique_node_count =
      tampered_node_certificate.node_support_summaries.front().node_count + 1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support unique-node counts "
        "outside support row count");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front().unique_node_count =
      1;
  check(
      !target::verify_lattice_target_proof_certificate(
           tampered_node_certificate)
           .passed,
      "proof certificate self-check recomputes unique node count from support "
      "rows");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .target_supervision_support_row_count = 1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check recomputes node-support exposure-use "
        "incidence counts from matrix rows");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .mutating_support_row_count = 1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check recomputes node-support mutation "
        "incidence counts from matrix rows");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .use.target_supervision = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support use flags that "
        "disagree with matrix row incidence");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .finite_valid_target_fraction_count =
      tampered_node_certificate.node_support_summaries.front().node_count + 1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support finite fraction "
        "counts above node count");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .finite_valid_target_fraction_count = 0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects finite node-support fractions "
        "without finite samples");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .min_valid_target_fraction = std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support finite sample "
        "counts with non-finite fraction ranges");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_node_index =
      tampered_node_certificate.node_support_summaries.front().node_count;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects weakest-node indexes outside "
        "the node-support matrix");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .min_valid_target_fraction = 0.9;
  tampered_node_certificate.node_support_summaries.front()
      .max_valid_target_fraction = 0.8;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects inverted node-support fraction "
        "ranges");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .mean_valid_target_fraction = 0.95;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support mean fractions "
        "outside the min/max range");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_node_id.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support summaries without "
        "a weakest node id");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_fraction = 0.9;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent weakest-node "
        "fraction arithmetic");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_denominator =
      tampered_node_certificate.node_support_summaries.front()
          .weakest_valid_target_count -
      1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects weakest-node denominators below "
        "success counts");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_count = 0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_denominator = 0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_fraction = -0.25;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_lower_95 =
      std::numeric_limits<double>::quiet_NaN();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_upper_95 =
      std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects finite weakest-node fractions "
        "without denominator support");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_count = 0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_denominator = 0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_fraction = 0.0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_lower_95 = 0.0;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_upper_95 = 1.0;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects weakest-node Wilson bounds "
        "without denominator support");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_count = 50;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_denominator = 100;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_fraction = 0.5;
  const auto [tampered_weakest_lower, tampered_weakest_upper] =
      exposure::wilson_score_interval_95(50, 100);
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_lower_95 = tampered_weakest_lower;
  tampered_node_certificate.node_support_summaries.front()
      .weakest_valid_target_wilson_upper_95 = tampered_weakest_upper;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects weakest-node counts above the "
        "aggregate mean");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_gini = std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects missing node-support balance "
        "metrics when node support is positive");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_gini = 0.01;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check recomputes node-support gini from matrix "
        "rows");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .support_rows.front()
      .valid_target_count = 79;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects node-support matrix rows that no "
        "longer match aggregate balance metrics");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  std::reverse(tampered_node_certificate.node_support_summaries.front()
                   .support_rows.begin(),
               tampered_node_certificate.node_support_summaries.front()
                   .support_rows.end());
  const auto tampered_node_row_order_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_order_check.passed &&
            std::any_of(tampered_node_row_order_check.issues.begin(),
                        tampered_node_row_order_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("support rows not canonical") !=
                                 std::string::npos;
                        }),
        "proof certificate self-check rejects noncanonical node-support matrix "
        "row order");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .support_rows.front()
      .node_index =
      tampered_node_certificate.node_support_summaries.front().node_count;
  const auto tampered_node_row_index_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_index_check.passed &&
            std::any_of(tampered_node_row_index_check.issues.begin(),
                        tampered_node_row_index_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("node index out of range") !=
                                 std::string::npos;
                        }),
        "proof certificate self-check rejects node-support row indexes outside "
        "the matrix");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .support_rows.front()
      .parent_exposure_fact_digest.clear();
  const auto tampered_node_row_parent_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_parent_check.passed &&
            std::any_of(tampered_node_row_parent_check.issues.begin(),
                        tampered_node_row_parent_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("missing parent exposure digest") !=
                                 std::string::npos;
                        }),
        "proof certificate self-check rejects node-support rows without parent "
        "exposure provenance");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .support_rows.front()
      .parent_exposure_fact_digest = "not_in_closure";
  const auto tampered_node_row_closure_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_closure_check.passed &&
            std::any_of(tampered_node_row_closure_check.issues.begin(),
                        tampered_node_row_closure_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("mutating parent exposure not "
                                            "in closure") != std::string::npos;
                        }),
        "proof certificate self-check rejects node-support rows whose parent "
        "exposure is outside closure");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .support_rows.front()
      .use = exposure::exposure_use_set_t{};
  const auto tampered_node_row_use_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_use_check.passed &&
            std::any_of(tampered_node_row_use_check.issues.begin(),
                        tampered_node_row_use_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("missing exposure use") !=
                                 std::string::npos;
                        }),
        "proof certificate self-check rejects node-support rows without "
        "exposure-use provenance");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  auto &tampered_node_row =
      tampered_node_certificate.node_support_summaries.front()
          .support_rows.front();
  tampered_node_row.valid_target_count = 0;
  tampered_node_row.valid_target_denominator = 0;
  tampered_node_row.valid_target_fraction = 1.25;
  const auto tampered_node_row_fraction_check =
      target::verify_lattice_target_proof_certificate(
          tampered_node_certificate);
  check(!tampered_node_row_fraction_check.passed &&
            std::any_of(tampered_node_row_fraction_check.issues.begin(),
                        tampered_node_row_fraction_check.issues.end(),
                        [](const std::string &issue) {
                          return issue.find("support_row[0] valid fraction out "
                                            "of range") != std::string::npos;
                        }),
        "proof certificate self-check rejects out-of-range support-row valid "
        "fractions directly");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_coefficient_of_variation =
      std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects missing node-support variation "
        "metrics when node support is positive");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_normalized_entropy =
      std::numeric_limits<double>::quiet_NaN();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects missing node-support entropy "
        "metrics when node support is positive");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_normalized_entropy = 1.01;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects out-of-range node-support "
        "entropy metrics");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .valid_target_count_coefficient_of_variation = -0.01;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects negative node-support variation "
        "metrics");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .active_row_count_total =
      tampered_node_certificate.node_support_summaries.front()
          .routed_row_count_total +
      1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects active node rows above routed "
        "rows");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .trained_row_count_total =
      tampered_node_certificate.node_support_summaries.front()
          .active_row_count_total +
      1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent node row-count "
        "totals");
  tampered_node_certificate = mdn_exposure_ready.proof_certificate;
  tampered_node_certificate.certificate_digest.clear();
  tampered_node_certificate.node_support_summaries.front()
      .evaluated_row_count_total =
      tampered_node_certificate.node_support_summaries.front()
          .active_row_count_total +
      1;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_node_certificate)
             .passed,
        "proof certificate self-check rejects evaluated node rows above active "
        "rows");
  check(mdn_exposure_ready.warning_results.size() == 9,
        "node-support warning clauses should trigger without changing status");
  check(mdn_exposure_ready.warning_summary.warning_result_count == 9 &&
            mdn_exposure_ready.warning_summary.triggered_warning_count == 7 &&
            mdn_exposure_ready.warning_summary.clear_measured_warning_count ==
                1 &&
            mdn_exposure_ready.warning_summary.unavailable_warning_count == 1 &&
            mdn_exposure_ready.warning_summary.warning_count == 7,
        "node-support and MDN distribution warning summary should aggregate "
        "triggered, clear, and unavailable warning rows");
  const auto node_balance_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "imbalanced_node_support";
      });
  check(node_balance_warning != mdn_exposure_ready.warning_results.end() &&
            node_balance_warning->use == "target_supervision" &&
            node_balance_warning->effect == "mutated_component" &&
            node_balance_warning->evidence_basis ==
                "filtered_node_support_summary" &&
            node_balance_warning->threshold_triggered &&
            node_balance_warning->threshold_relation == "above_threshold" &&
            node_balance_warning->threshold_direction == "above" &&
            node_balance_warning->measurement_available &&
            !node_balance_warning->exposure_summary_available &&
            node_balance_warning->node_support_summary_available &&
            node_balance_warning->node_support_summary.node_count == 2 &&
            node_balance_warning->node_support_summary
                    .mutating_support_row_count == 2 &&
            node_balance_warning->message.find("above threshold 0.25") !=
                std::string::npos &&
            node_balance_warning->message.find(
                "weakest node ETH has valid_target_count 2") !=
                std::string::npos,
        "node-support balance warning should filter target-supervision "
        "mutating rows and explain the threshold and weakest-node support");
  const auto node_entropy_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "low_node_support_entropy";
      });
  check(node_entropy_warning != mdn_exposure_ready.warning_results.end() &&
            node_entropy_warning->status == "warning" &&
            node_entropy_warning->unit == "fraction" &&
            node_entropy_warning->threshold_triggered &&
            node_entropy_warning->threshold_relation == "below_threshold" &&
            node_entropy_warning->threshold_direction == "below" &&
            node_entropy_warning->message.find("below threshold 0.25") !=
                std::string::npos &&
            std::abs(node_entropy_warning->measured_value -
                     expected_mdn_node_entropy) < 1e-12,
        "node-support balance warning can trigger on low normalized entropy");
  const auto weak_node_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "weak_node_support";
      });
  check(weak_node_warning != mdn_exposure_ready.warning_results.end() &&
            weak_node_warning->threshold_triggered &&
            weak_node_warning->threshold_relation == "below_threshold" &&
            weak_node_warning->threshold_direction == "below" &&
            weak_node_warning->message.find("below threshold 5") !=
                std::string::npos &&
            weak_node_warning->message.find("weakest node ETH") !=
                std::string::npos,
        "node-support floor warning message should explain the threshold and "
        "weakest node");
  const auto weak_left_half_node_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "weak_left_half_node_support";
      });
  check(
      weak_left_half_node_warning != mdn_exposure_ready.warning_results.end() &&
          weak_left_half_node_warning->threshold_triggered &&
          weak_left_half_node_warning->threshold_relation ==
              "below_threshold" &&
          weak_left_half_node_warning->threshold_direction == "below" &&
          weak_left_half_node_warning->warning_anchor_range.begin == 100 &&
          weak_left_half_node_warning->warning_anchor_range.end == 150 &&
          weak_left_half_node_warning->node_support_summary_available &&
          weak_left_half_node_warning->node_support_summary
                  .weakest_valid_target_count == 1 &&
          weak_left_half_node_warning->node_support_summary
                  .weakest_valid_target_denominator == 5 &&
          std::abs(weak_left_half_node_warning->measured_value - 1.0) < 1e-12 &&
          weak_left_half_node_warning->message.find("below threshold 2") !=
              std::string::npos,
      "node-support floor warnings should derive metrics over the warning "
      "anchor interval, not the target-wide node matrix");
  const auto aggregate_support_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "low_aggregate_node_support_confidence";
      });
  check(aggregate_support_warning != mdn_exposure_ready.warning_results.end() &&
            aggregate_support_warning->status == "warning" &&
            aggregate_support_warning->unit == "fraction" &&
            aggregate_support_warning->threshold_triggered &&
            aggregate_support_warning->threshold_relation ==
                "below_threshold" &&
            aggregate_support_warning->threshold_direction == "below" &&
            aggregate_support_warning->message.find("below threshold 0.8") !=
                std::string::npos &&
            std::abs(aggregate_support_warning->measured_value -
                     expected_mdn_node_support_lower) < 1e-12,
        "node-support floor warning can trigger on low aggregate Wilson lower "
        "bound");
  const auto clear_node_fraction_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "missing_node_fraction_support";
      });
  check(clear_node_fraction_warning !=
                mdn_exposure_ready.warning_results.end() &&
            !clear_node_fraction_warning->threshold_triggered &&
            clear_node_fraction_warning->threshold_relation ==
                "not_below_threshold" &&
            clear_node_fraction_warning->threshold_direction == "below" &&
            clear_node_fraction_warning->message.find(
                "not below threshold 0.1") != std::string::npos,
        "clear node-support floor warning message should say not below "
        "threshold");
  const auto high_mdn_mean_nll_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "high_mdn_mean_nll";
      });
  check(
      high_mdn_mean_nll_warning != mdn_exposure_ready.warning_results.end() &&
          high_mdn_mean_nll_warning->kind == "mdn_distribution_calibration" &&
          high_mdn_mean_nll_warning->evidence_basis ==
              "mdn_distribution_facts" &&
          high_mdn_mean_nll_warning->unit == "nll" &&
          high_mdn_mean_nll_warning->use == "target_supervision" &&
          high_mdn_mean_nll_warning->threshold_triggered &&
          high_mdn_mean_nll_warning->threshold_relation == "above_threshold" &&
          high_mdn_mean_nll_warning->threshold_direction == "above" &&
          high_mdn_mean_nll_warning->measurement_available &&
          high_mdn_mean_nll_warning->exposure_summary_available &&
          high_mdn_mean_nll_warning->diagnostic_metric_family ==
              "proper_scoring_loss" &&
          high_mdn_mean_nll_warning->diagnostic_uncertainty_method ==
              "none_point_estimate_only" &&
          high_mdn_mean_nll_warning->diagnostic_sample_count == 25 &&
          high_mdn_mean_nll_warning->diagnostic_readiness_effect ==
              "non_blocking_warning_only" &&
          high_mdn_mean_nll_warning->diagnostic_active_contract_fingerprint ==
              "contract_1" &&
          high_mdn_mean_nll_warning
                  ->diagnostic_active_graph_order_fingerprint == "graph_1" &&
          high_mdn_mean_nll_warning->diagnostic_active_source_cursor_token ==
              "cursor_ranged" &&
          std::abs(high_mdn_mean_nll_warning->measured_value - 0.40) < 1e-12 &&
          high_mdn_mean_nll_warning->message.find("above threshold 0.35") !=
              std::string::npos,
      "MDN distribution warning can trigger on high mean NLL without "
      "changing readiness");
  const auto high_per_node_mdn_mean_nll_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "high_per_node_mdn_mean_nll";
      });
  check(high_per_node_mdn_mean_nll_warning !=
                mdn_exposure_ready.warning_results.end() &&
            high_per_node_mdn_mean_nll_warning->kind ==
                "mdn_distribution_calibration" &&
            high_per_node_mdn_mean_nll_warning->evidence_basis ==
                "filtered_node_exposure_facts" &&
            high_per_node_mdn_mean_nll_warning->unit == "nll" &&
            high_per_node_mdn_mean_nll_warning->threshold_triggered &&
            high_per_node_mdn_mean_nll_warning->measurement_available &&
            high_per_node_mdn_mean_nll_warning->exposure_summary_available &&
            high_per_node_mdn_mean_nll_warning->exposure_summary.fact_count ==
                2 &&
            high_per_node_mdn_mean_nll_warning->exposure_summary
                    .valid_target_count_total == 82 &&
            high_per_node_mdn_mean_nll_warning->diagnostic_metric_family ==
                "node_stratified_scoring_loss" &&
            high_per_node_mdn_mean_nll_warning->diagnostic_uncertainty_method ==
                "none_point_estimate_only" &&
            high_per_node_mdn_mean_nll_warning->diagnostic_sample_count == 82 &&
            std::abs(high_per_node_mdn_mean_nll_warning->measured_value -
                     0.40) < 1e-12,
        "MDN distribution diagnostics can warn on per-node mean NLL while "
        "remaining non-blocking");
  const auto unavailable_mdn_pit_warning = std::find_if(
      mdn_exposure_ready.warning_results.begin(),
      mdn_exposure_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "unavailable_mdn_pit_statistic";
      });
  check(
      unavailable_mdn_pit_warning != mdn_exposure_ready.warning_results.end() &&
          unavailable_mdn_pit_warning->kind == "mdn_distribution_calibration" &&
          unavailable_mdn_pit_warning->status == "clear" &&
          unavailable_mdn_pit_warning->unit == "fraction" &&
          unavailable_mdn_pit_warning->threshold_relation == "unavailable" &&
          !unavailable_mdn_pit_warning->measurement_available &&
          unavailable_mdn_pit_warning->message.find(
              "is unavailable for threshold 0.2") != std::string::npos,
      "future MDN calibration metrics should report unavailable instead of "
      "claiming calibration evidence");
  check(mdn_exposure_ready.warning_results.front()
                .node_support_summary.node_count == 2,
        "node-support warning result includes the derived node summary");
  expect_warnings_project_warning_results(
      mdn_exposure_ready,
      "node-support warning list should project warning results");
  exposure::lattice_exposure_ledger_t no_fraction_node_ledger{};
  no_fraction_node_ledger.add(
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200));
  no_fraction_node_ledger.add(mdn_target_fact);
  for (std::int64_t i = 0; i < 2; ++i) {
    exposure::lattice_node_exposure_fact_t node_fact{};
    node_fact.parent_exposure_fact_digest = mdn_target_fact_digest;
    node_fact.contract_fingerprint = "contract_1";
    node_fact.graph_order_fingerprint = "graph_1";
    node_fact.source_cursor_token = "cursor_ranged";
    node_fact.component_assembly_fingerprint = "mdn_1";
    node_fact.target_component = "wikimyei.inference.expected_value.mdn";
    node_fact.job_id = "mdn_ranged";
    node_fact.wave_id = "wave_mdn_ranged";
    node_fact.job_status = "completed";
    node_fact.wave_action = "train";
    node_fact.node_id = i == 0 ? "BTC" : "ETH";
    node_fact.node_index = i;
    node_fact.anchor_range = mdn_target_fact.anchor_range;
    node_fact.completed_anchor_range = mdn_target_fact.completed_anchor_range;
    node_fact.use = mdn_target_fact.use;
    node_fact.valid_target_fraction = std::numeric_limits<double>::quiet_NaN();
    node_fact.output_checkpoint = mdn_exposure_checkpoint;
    no_fraction_node_ledger.add_node(node_fact);
  }
  target::lattice_target_evaluator_options_t no_fraction_node_options =
      mdn_exposure_options;
  no_fraction_node_options.exposure_ledger = &no_fraction_node_ledger;
  target::lattice_target_evaluator_t no_fraction_node_eval(
      mdn_coverage_specs, no_fraction_node_options);
  const auto no_fraction_node_ready =
      no_fraction_node_eval.evaluate("ranged_node_mdn_ready");
  const auto missing_node_fraction_warning = std::find_if(
      no_fraction_node_ready.warning_results.begin(),
      no_fraction_node_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "missing_node_fraction_support";
      });
  check(missing_node_fraction_warning !=
                no_fraction_node_ready.warning_results.end() &&
            missing_node_fraction_warning->status == "clear" &&
            missing_node_fraction_warning->threshold_direction == "below" &&
            !missing_node_fraction_warning->threshold_triggered &&
            missing_node_fraction_warning->threshold_relation ==
                "unavailable" &&
            !missing_node_fraction_warning->measurement_available &&
            missing_node_fraction_warning->message.find(
                "node-support min_valid_target_fraction is unavailable for "
                "threshold 0.1") != std::string::npos,
        "clear node-support warning message should report unavailable metrics "
        "with the configured threshold");
  check(no_fraction_node_ready.evidence_order_vector.unavailable_warning_count >
            0,
        "unavailable node-support metrics should weaken the evidence order "
        "vector through structured availability");
  check(no_fraction_node_ready.warning_summary.unavailable_warning_count > 0 &&
            no_fraction_node_ready.warning_summary.unavailable_relation_count >
                0,
        "warning summary should aggregate unavailable node-support metrics");
  expect_warnings_project_warning_results(
      no_fraction_node_ready,
      "missing node-fraction warning list should project warning results");
  exposure::lattice_exposure_ledger_t no_node_support_ledger{};
  no_node_support_ledger.add(
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200));
  no_node_support_ledger.add(mdn_target_fact);
  target::lattice_target_evaluator_options_t no_node_support_options =
      mdn_exposure_options;
  no_node_support_options.exposure_ledger = &no_node_support_ledger;
  target::lattice_target_evaluator_t no_node_support_eval(
      mdn_coverage_specs, no_node_support_options);
  const auto no_node_support_ready =
      no_node_support_eval.evaluate("ranged_node_mdn_ready");
  const auto missing_summary_warning = std::find_if(
      no_node_support_ready.warning_results.begin(),
      no_node_support_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "weak_node_support";
      });
  check(missing_summary_warning !=
                no_node_support_ready.warning_results.end() &&
            missing_summary_warning->status == "clear" &&
            !missing_summary_warning->measurement_available &&
            missing_summary_warning->message.find(
                "node-support summary is unavailable for "
                "ranged_node_mdn_ready for threshold 5") != std::string::npos,
        "clear node-support warning message should report unavailable "
        "summaries with the configured threshold");
  check(
      no_node_support_ready.evidence_order_vector.unavailable_warning_count > 0,
      "missing node-support summaries should weaken the evidence order vector "
      "through structured availability");
  check(no_node_support_ready.warning_summary.unavailable_warning_count > 0 &&
            no_node_support_ready.warning_summary.unavailable_relation_count >
                0,
        "warning summary should aggregate missing node-support summaries");
  expect_warnings_project_warning_results(
      no_node_support_ready,
      "missing node-support warning list should project warning results");
  check(mdn_exposure_ready.proof_certificate.closure.causal_exposures.size() ==
            2,
        "node MDN closure proof includes both MDN and upstream VICReg causal "
        "exposures");
  check(mdn_exposure_ready.proof_certificate_check.passed,
        "node MDN closure causal graph passes certificate self-check");
  auto tampered_duplicate_digest_certificate =
      mdn_exposure_ready.proof_certificate;
  tampered_duplicate_digest_certificate.certificate_digest.clear();
  check(tampered_duplicate_digest_certificate.closure.fact_digests.size() >=
                2 &&
            tampered_duplicate_digest_certificate.closure.causal_exposures
                    .size() >= 2,
        "node MDN closure proof has enough causal facts to test duplicate "
        "digests");
  tampered_duplicate_digest_certificate.closure.fact_digests[1] =
      tampered_duplicate_digest_certificate.closure.fact_digests.front();
  tampered_duplicate_digest_certificate.closure.causal_exposures[1]
      .fact_digest =
      tampered_duplicate_digest_certificate.closure.causal_exposures.front()
          .fact_digest;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_duplicate_digest_certificate)
             .passed,
        "proof certificate self-check rejects duplicate closure fact digests");
  auto tampered_mdn_causal_certificate = mdn_exposure_ready.proof_certificate;
  tampered_mdn_causal_certificate.certificate_digest.clear();
  auto target_supervision_causal = std::find_if(
      tampered_mdn_causal_certificate.closure.causal_exposures.begin(),
      tampered_mdn_causal_certificate.closure.causal_exposures.end(),
      [](const target::lattice_target_proof_certificate_t::closure_proof_t::
             causal_exposure_t &causal) {
        return std::find(causal.uses.begin(), causal.uses.end(),
                         "target_supervision") != causal.uses.end();
      });
  check(target_supervision_causal !=
            tampered_mdn_causal_certificate.closure.causal_exposures.end(),
        "node MDN closure proof includes target-supervision causal exposure");
  if (target_supervision_causal !=
      tampered_mdn_causal_certificate.closure.causal_exposures.end()) {
    target_supervision_causal->target_footprint = {};
    check(!target::verify_lattice_target_proof_certificate(
               tampered_mdn_causal_certificate)
               .passed,
          "proof certificate self-check rejects causal target-supervision "
          "facts with empty source footprints");
  }
  auto causal_order_certificate = mdn_exposure_ready.proof_certificate;
  const auto causal_order_digest =
      target::lattice_target_proof_certificate_digest(causal_order_certificate);
  std::reverse(causal_order_certificate.closure.causal_exposures.begin(),
               causal_order_certificate.closure.causal_exposures.end());
  check(causal_order_digest == target::lattice_target_proof_certificate_digest(
                                   causal_order_certificate),
        "proof certificate digest is canonical across closure causal exposure "
        "order");
  auto tampered_closure_graph_certificate =
      mdn_exposure_ready.proof_certificate;
  tampered_closure_graph_certificate.certificate_digest.clear();
  auto detached_causal_output = std::find_if(
      tampered_closure_graph_certificate.closure.causal_exposures.begin(),
      tampered_closure_graph_certificate.closure.causal_exposures.end(),
      [&](const target::lattice_target_proof_certificate_t::closure_proof_t::
              causal_exposure_t &causal) {
        return causal.output_checkpoint.lexically_normal() !=
               tampered_closure_graph_certificate.closure.checkpoint_path
                   .lexically_normal();
      });
  check(detached_causal_output !=
            tampered_closure_graph_certificate.closure.causal_exposures.end(),
        "node MDN closure proof includes a non-root upstream producer");
  if (detached_causal_output !=
      tampered_closure_graph_certificate.closure.causal_exposures.end()) {
    detached_causal_output->output_checkpoint =
        tampered_closure_graph_certificate.closure.checkpoint_path
            .parent_path() /
        "detached-upstream.pt";
    check(!target::verify_lattice_target_proof_certificate(
               tampered_closure_graph_certificate)
               .passed,
          "proof certificate self-check rejects causal closure outputs not "
          "reached by any input checkpoint edge");
  }
  tampered_closure_graph_certificate = mdn_exposure_ready.proof_certificate;
  tampered_closure_graph_certificate.certificate_digest.clear();
  auto upstream_causal_input = std::find_if(
      tampered_closure_graph_certificate.closure.causal_exposures.begin(),
      tampered_closure_graph_certificate.closure.causal_exposures.end(),
      [&](const target::lattice_target_proof_certificate_t::closure_proof_t::
              causal_exposure_t &causal) {
        return causal.output_checkpoint.lexically_normal() !=
               tampered_closure_graph_certificate.closure.checkpoint_path
                   .lexically_normal();
      });
  if (upstream_causal_input !=
      tampered_closure_graph_certificate.closure.causal_exposures.end()) {
    upstream_causal_input->input_checkpoints.push_back(
        tampered_closure_graph_certificate.closure.checkpoint_path);
    check(!target::verify_lattice_target_proof_certificate(
               tampered_closure_graph_certificate)
               .passed,
          "proof certificate self-check rejects causal input checkpoint edges "
          "back to the closure root");
  }
  tampered_closure_graph_certificate = mdn_exposure_ready.proof_certificate;
  tampered_closure_graph_certificate.certificate_digest.clear();
  check(tampered_closure_graph_certificate.closure.causal_exposures.size() >= 2,
        "node MDN closure proof has enough causal facts to test disconnected "
        "cycles");
  auto disconnected_cycle_a =
      tampered_closure_graph_certificate.closure.causal_exposures.front();
  auto disconnected_cycle_b =
      tampered_closure_graph_certificate.closure.causal_exposures.back();
  const auto disconnected_cycle_a_path =
      tampered_closure_graph_certificate.closure.checkpoint_path.parent_path() /
      "disconnected-cycle-a.pt";
  const auto disconnected_cycle_b_path =
      tampered_closure_graph_certificate.closure.checkpoint_path.parent_path() /
      "disconnected-cycle-b.pt";
  disconnected_cycle_a.fact_digest = "disconnected_cycle_a_digest";
  disconnected_cycle_a.output_checkpoint = disconnected_cycle_a_path;
  disconnected_cycle_a.input_checkpoints = {disconnected_cycle_b_path};
  disconnected_cycle_b.fact_digest = "disconnected_cycle_b_digest";
  disconnected_cycle_b.output_checkpoint = disconnected_cycle_b_path;
  disconnected_cycle_b.input_checkpoints = {disconnected_cycle_a_path};
  tampered_closure_graph_certificate.closure.causal_exposures.push_back(
      disconnected_cycle_a);
  tampered_closure_graph_certificate.closure.causal_exposures.push_back(
      disconnected_cycle_b);
  tampered_closure_graph_certificate.closure.fact_digests.push_back(
      disconnected_cycle_a.fact_digest);
  tampered_closure_graph_certificate.closure.fact_digests.push_back(
      disconnected_cycle_b.fact_digest);
  tampered_closure_graph_certificate.closure.fact_count =
      static_cast<std::int64_t>(
          tampered_closure_graph_certificate.closure.causal_exposures.size());
  check(!target::verify_lattice_target_proof_certificate(
             tampered_closure_graph_certificate)
             .passed,
        "proof certificate self-check rejects disconnected causal checkpoint "
        "cycles outside the root closure");
  tampered_closure_graph_certificate = mdn_exposure_ready.proof_certificate;
  tampered_closure_graph_certificate.certificate_digest.clear();
  auto reachable_cycle_a = std::find_if(
      tampered_closure_graph_certificate.closure.causal_exposures.begin(),
      tampered_closure_graph_certificate.closure.causal_exposures.end(),
      [&](const target::lattice_target_proof_certificate_t::closure_proof_t::
              causal_exposure_t &causal) {
        return causal.output_checkpoint.lexically_normal() !=
               tampered_closure_graph_certificate.closure.checkpoint_path
                   .lexically_normal();
      });
  if (reachable_cycle_a !=
      tampered_closure_graph_certificate.closure.causal_exposures.end()) {
    auto reachable_cycle_b = *reachable_cycle_a;
    const auto reachable_cycle_a_path =
        reachable_cycle_a->output_checkpoint.lexically_normal();
    const auto reachable_cycle_b_path =
        tampered_closure_graph_certificate.closure.checkpoint_path
            .parent_path() /
        "reachable-cycle-b.pt";
    reachable_cycle_a->input_checkpoints = {reachable_cycle_b_path};
    reachable_cycle_b.fact_digest = "reachable_cycle_b_digest";
    reachable_cycle_b.output_checkpoint = reachable_cycle_b_path;
    reachable_cycle_b.input_checkpoints = {reachable_cycle_a_path};
    tampered_closure_graph_certificate.closure.causal_exposures.push_back(
        reachable_cycle_b);
    tampered_closure_graph_certificate.closure.fact_digests.push_back(
        reachable_cycle_b.fact_digest);
    tampered_closure_graph_certificate.closure.fact_count =
        static_cast<std::int64_t>(
            tampered_closure_graph_certificate.closure.causal_exposures.size());
    const auto reachable_cycle_check =
        target::verify_lattice_target_proof_certificate(
            tampered_closure_graph_certificate);
    check(!reachable_cycle_check.passed &&
              std::find(reachable_cycle_check.issues.begin(),
                        reachable_cycle_check.issues.end(),
                        "closure causal checkpoint cycle") !=
                  reachable_cycle_check.issues.end(),
          "proof certificate self-check rejects root-reachable causal "
          "checkpoint cycles");
  }

  const auto validation_eval_root = make_tmp_dir("validation_eval");
  const auto validation_eval_mdn_checkpoint =
      validation_eval_root / "mdn_train.pt";
  const auto validation_eval_rep_checkpoint =
      validation_eval_root / "rep_train.pt";
  write_ranged_mdn_job(validation_eval_root, validation_eval_mdn_checkpoint,
                       validation_eval_rep_checkpoint, 100, 160);
  write_ranged_mdn_eval_job(validation_eval_root,
                            validation_eval_mdn_checkpoint,
                            validation_eval_rep_checkpoint, 200, 210);
  exposure::lattice_exposure_ledger_t validation_eval_ledger{};
  validation_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  validation_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  const auto validation_eval_fact = exposure::make_exposure_fact_from_job_dir(
      validation_eval_root / "mdn_validation_eval");
  const auto validation_eval_fact_digest =
      exposure::exposure_fact_digest(validation_eval_fact);
  validation_eval_ledger.add(validation_eval_fact);
  for (const auto &node_fact : exposure::make_node_exposure_facts_from_job_dir(
           validation_eval_root / "mdn_validation_eval",
           validation_eval_fact)) {
    validation_eval_ledger.add_node(node_fact);
  }
  const auto validation_eval_specs = target::decode_lattice_targets_from_dsl(
      R"DSL(
LATTICE_TARGET {
  TARGET_ID = clean_node_mdn_train_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 160;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_TRAINED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = clean_node_mdn_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:clean_node_mdn_train_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 200;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 210;
  FORBID_EXPOSURE_USES = observed_input|target_supervision|selection_signal;
  FORBID_EXPOSURE_REQUIRES_MUTATED_COMPONENT = true;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = clean_node_mdn_validation_eval_ready;
  TARGET_CLASS = evaluation_readiness;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 200;
  ANCHOR_INDEX_END = 210;
  UPSTREAM_TARGET_ID = clean_node_mdn_no_validation_leakage;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:clean_node_mdn_no_validation_leakage;
  MIN_EVALUATION_METRIC_COVERAGE = 1.0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 1;
};

LATTICE_WARN {
  TARGET_ID = clean_node_mdn_validation_eval_ready;
  WARNING_ID = validation_eval_high_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = evaluation_metric;
  EFFECT = none;
  METRIC = mean_nll;
  ABOVE = 0.40;
};

LATTICE_WARN {
  TARGET_ID = clean_node_mdn_validation_eval_ready;
  WARNING_ID = validation_eval_high_per_node_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = evaluation_metric;
  EFFECT = none;
  METRIC = per_node_mean_nll_max;
  ABOVE = 0.40;
};
)DSL");
  target::lattice_target_evaluator_options_t validation_eval_options = options;
  validation_eval_options.runtime_root = validation_eval_root;
  validation_eval_options.exposure_ledger = &validation_eval_ledger;
  target::lattice_target_evaluator_t validation_eval(validation_eval_specs,
                                                     validation_eval_options);
  const auto validation_eval_ready =
      validation_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(validation_eval_ready.status ==
            target::lattice_target_status_t::satisfied,
        "run-mode validation evaluation should satisfy evaluation_metric "
        "coverage without writing a new checkpoint");
  check(validation_eval_ready.proof_certificate_check.passed &&
            validation_eval_ready.proof_certificate_check.issues.empty(),
        "validation evaluation proof certificate should be self-consistent");
  check(validation_eval_ready.proof_certificate.closure.checked &&
            validation_eval_ready.proof_certificate.closure.checkpoint_path ==
                validation_eval_mdn_checkpoint,
        "validation evaluation leakage proof should bind to the evaluated MDN "
        "checkpoint closure");
  check(validation_eval_ready.exposure_summaries.size() == 1 &&
            validation_eval_ready.exposure_summaries.front().use ==
                exposure::exposure_use_t::evaluation_metric,
        "validation evaluation target reports evaluation_metric exposure");
  check(validation_eval_ready.exposure_summaries.front().loaded_anchor_events ==
            10,
        "validation evaluation summary counts completed validation anchors");
  check(
      validation_eval_ready.exposure_summaries.front().optimizer_steps_total ==
          0,
      "validation evaluation coverage summary proves zero optimizer steps");
  const auto [validation_support_lower, validation_support_upper] =
      exposure::wilson_score_interval_95(6, 24);
  check(validation_eval_ready.exposure_summaries.front()
                    .valid_target_success_count_for_uncertainty == 6 &&
            validation_eval_ready.exposure_summaries.front()
                    .valid_target_opportunity_count_for_uncertainty == 24 &&
            std::abs(validation_eval_ready.exposure_summaries.front()
                         .valid_target_fraction_estimate -
                     0.25) < 1e-12 &&
            std::abs(validation_eval_ready.exposure_summaries.front()
                         .valid_target_wilson_lower_95 -
                     validation_support_lower) < 1e-12 &&
            std::abs(validation_eval_ready.exposure_summaries.front()
                         .valid_target_wilson_upper_95 -
                     validation_support_upper) < 1e-12,
        "validation evaluation summary carries valid-target uncertainty "
        "interval");
  const auto evaluated_dependency = std::find_if(
      validation_eval_ready.proof_certificate.dependencies.begin(),
      validation_eval_ready.proof_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  check(evaluated_dependency !=
                validation_eval_ready.proof_certificate.dependencies.end() &&
            evaluated_dependency->mdn_checkpoint_match &&
            evaluated_dependency->representation_checkpoint_match,
        "validation certificate proves exact evaluated checkpoint bindings");
  const auto validation_eval_mean_nll_warning = std::find_if(
      validation_eval_ready.warning_results.begin(),
      validation_eval_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "validation_eval_high_mean_nll";
      });
  check(
      validation_eval_mean_nll_warning !=
              validation_eval_ready.warning_results.end() &&
          validation_eval_mean_nll_warning->kind ==
              "mdn_distribution_calibration" &&
          validation_eval_mean_nll_warning->status == "warning" &&
          validation_eval_mean_nll_warning->use == "evaluation_metric" &&
          validation_eval_mean_nll_warning->effect == "any" &&
          validation_eval_mean_nll_warning->threshold_triggered &&
          validation_eval_mean_nll_warning->measurement_available &&
          validation_eval_mean_nll_warning->exposure_summary_available &&
          validation_eval_mean_nll_warning->diagnostic_metric_family ==
              "proper_scoring_loss" &&
          validation_eval_mean_nll_warning->diagnostic_uncertainty_method ==
              "none_point_estimate_only" &&
          validation_eval_mean_nll_warning->diagnostic_sample_count == 6 &&
          validation_eval_mean_nll_warning
              ->diagnostic_exact_checkpoint_binding_proven &&
          validation_eval_mean_nll_warning->diagnostic_mdn_checkpoint_loaded &&
          validation_eval_mean_nll_warning
              ->diagnostic_representation_checkpoint_loaded &&
          validation_eval_mean_nll_warning
                  ->diagnostic_evaluated_mdn_checkpoint ==
              validation_eval_mdn_checkpoint &&
          validation_eval_mean_nll_warning
                  ->diagnostic_representation_checkpoint ==
              validation_eval_rep_checkpoint &&
          validation_eval_mean_nll_warning
                  ->diagnostic_split_policy_fingerprint ==
              validation_eval_ready.split_policy_fingerprint &&
          validation_eval_mean_nll_warning
                  ->diagnostic_active_contract_fingerprint == "contract_1" &&
          validation_eval_mean_nll_warning
                  ->diagnostic_active_graph_order_fingerprint == "graph_1" &&
          validation_eval_mean_nll_warning
                  ->diagnostic_active_source_cursor_token == "cursor_ranged" &&
          validation_eval_mean_nll_warning->diagnostic_validation_split ==
              "validation_holdout" &&
          validation_eval_mean_nll_warning->diagnostic_validation_split_bound &&
          validation_eval_mean_nll_warning->diagnostic_readiness_effect ==
              "non_blocking_warning_only" &&
          std::abs(validation_eval_mean_nll_warning->measured_value - 0.45) <
              1e-12,
      "validation MDN diagnostic warning should prove exact checkpoint, split, "
      "and active identity binding without changing readiness");
  const auto validation_eval_per_node_nll_warning = std::find_if(
      validation_eval_ready.warning_results.begin(),
      validation_eval_ready.warning_results.end(), [](const auto &warning) {
        return warning.warning_id == "validation_eval_high_per_node_mean_nll";
      });
  check(validation_eval_per_node_nll_warning !=
                validation_eval_ready.warning_results.end() &&
            validation_eval_per_node_nll_warning->evidence_basis ==
                "filtered_node_exposure_facts" &&
            validation_eval_per_node_nll_warning->diagnostic_metric_family ==
                "node_stratified_scoring_loss" &&
            validation_eval_per_node_nll_warning->diagnostic_sample_count ==
                6 &&
            validation_eval_per_node_nll_warning
                ->diagnostic_exact_checkpoint_binding_proven &&
            validation_eval_per_node_nll_warning->exposure_summary.fact_count ==
                1 &&
            validation_eval_per_node_nll_warning->threshold_triggered &&
            std::abs(validation_eval_per_node_nll_warning->measured_value -
                     0.45) < 1e-12,
        "validation MDN diagnostic warning should expose finite per-node NLL "
        "samples separately from aggregate NLL");
  auto tampered_dependency_certificate =
      validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  if (evaluated_dependency !=
      validation_eval_ready.proof_certificate.dependencies.end()) {
    tampered_dependency_certificate.dependencies.push_back(
        *evaluated_dependency);
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects duplicate dependency proofs");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  auto tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->actual_mdn_checkpoint =
        tampered_dependency->expected_mdn_checkpoint.parent_path() /
        "wrong_mdn.pt";
    tampered_dependency->mdn_checkpoint_match = true;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent exact checkpoint "
        "dependency bindings");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->actual_representation_checkpoint =
        tampered_dependency->expected_representation_checkpoint.parent_path() /
        "wrong_representation.pt";
    tampered_dependency->representation_checkpoint_match = true;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects inconsistent representation "
        "checkpoint dependency bindings");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->mdn_checkpoint_match = false;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects satisfied checkpoint-source "
        "dependencies with false MDN checkpoint matches");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->representation_checkpoint_match = false;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects satisfied evaluated checkpoint "
        "dependencies with false representation checkpoint matches");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->actual_mdn_checkpoint.clear();
    tampered_dependency->mdn_checkpoint_match = false;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects satisfied checkpoint-source "
        "dependencies with missing checkpoint paths");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->actual_representation_checkpoint.clear();
    tampered_dependency->representation_checkpoint_match = false;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects satisfied evaluated checkpoint "
        "dependencies with missing representation paths");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->status = "blocked";
    tampered_dependency->expected_mdn_checkpoint.clear();
    tampered_dependency->actual_mdn_checkpoint.clear();
    tampered_dependency->mdn_checkpoint_match = true;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects vacuous MDN checkpoint matches");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->status = "blocked";
    tampered_dependency->expected_representation_checkpoint.clear();
    tampered_dependency->actual_representation_checkpoint.clear();
    tampered_dependency->representation_checkpoint_match = true;
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects vacuous representation "
        "checkpoint matches");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->status.clear();
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects dependencies without statuses");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->status = "half_satisfied";
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects unknown dependency statuses");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->kind.clear();
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects dependencies without kinds");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->kind = "model_state_hint";
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects unknown dependency kinds");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  tampered_dependency = std::find_if(
      tampered_dependency_certificate.dependencies.begin(),
      tampered_dependency_certificate.dependencies.end(),
      [](const target::lattice_target_proof_certificate_t::dependency_proof_t
             &proof) { return proof.kind == "evaluated_checkpoint_source"; });
  if (tampered_dependency !=
      tampered_dependency_certificate.dependencies.end()) {
    tampered_dependency->source_target_id.clear();
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects dependencies without source "
        "target ids");
  tampered_dependency_certificate = validation_eval_ready.proof_certificate;
  tampered_dependency_certificate.certificate_digest.clear();
  target::lattice_target_proof_certificate_t::dependency_proof_t
      upstream_dependency{};
  upstream_dependency.kind = "upstream_target";
  upstream_dependency.source_target_id = "node_mdn_train_core_ready";
  upstream_dependency.status = "satisfied";
  upstream_dependency.expected_mdn_checkpoint = validation_eval_mdn_checkpoint;
  upstream_dependency.actual_mdn_checkpoint = validation_eval_mdn_checkpoint;
  upstream_dependency.mdn_checkpoint_match = true;
  tampered_dependency_certificate.dependencies.push_back(upstream_dependency);
  check(!target::verify_lattice_target_proof_certificate(
             tampered_dependency_certificate)
             .passed,
        "proof certificate self-check rejects upstream dependencies carrying "
        "checkpoint bindings");
  check(validation_eval_ready.proof_certificate.coverage.size() == 1 &&
            validation_eval_ready.proof_certificate.coverage.front().use ==
                "evaluation_metric" &&
            validation_eval_ready.proof_certificate.coverage.front().passed,
        "validation certificate records evaluation-metric coverage proof");
  check(validation_eval_ready.proof_certificate.coverage.front()
                    .contributing_fact_digests.size() == 1 &&
            validation_eval_ready.proof_certificate.coverage.front()
                    .contributing_fact_digests.front() ==
                validation_eval_fact_digest &&
            validation_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            validation_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id,
        "validation certificate binds non-mutating evaluation coverage to the "
        "runtime exposure fact");
  exposure::lattice_exposure_ledger_t mutated_eval_ledger{};
  mutated_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  mutated_eval_ledger.add(make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                                                 validation_eval_rep_checkpoint,
                                                 100, 160));
  auto mutated_eval_fact = validation_eval_fact;
  mutated_eval_fact.job_id = "mutated_validation_eval";
  mutated_eval_fact.wave_action = "train";
  mutated_eval_fact.use.mutated_component = true;
  mutated_eval_fact.optimizer_steps = 1;
  mutated_eval_ledger.add(mutated_eval_fact);
  auto mutated_eval_options = validation_eval_options;
  mutated_eval_options.exposure_ledger = &mutated_eval_ledger;
  target::lattice_target_evaluator_t mutated_eval(validation_eval_specs,
                                                  mutated_eval_options);
  const auto mutated_eval_ready =
      mutated_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(
      mutated_eval_ready.status ==
              target::lattice_target_status_t::exposure_failed &&
          mutated_eval_ready.proof_certificate.coverage.size() == 1 &&
          mutated_eval_ready.proof_certificate.coverage.front()
              .contributing_facts.empty() &&
          mutated_eval_ready.exposure_summaries.size() == 1 &&
          mutated_eval_ready.exposure_summaries.front().loaded_anchor_events ==
              0,
      "validation evaluation coverage must ignore evaluation_metric facts "
      "that mutated model state");
  expect_validation_model_state_deficit(
      mutated_eval_ready,
      "validation evaluation evidence that mutates model state should expose a "
      "model-state mutation deficit");
  exposure::lattice_exposure_ledger_t optimizer_eval_ledger{};
  optimizer_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  optimizer_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  auto optimizer_eval_fact = validation_eval_fact;
  optimizer_eval_fact.job_id = "optimizer_validation_eval";
  optimizer_eval_fact.wave_action = "debug";
  optimizer_eval_fact.use.mutated_component = false;
  optimizer_eval_fact.optimizer_steps = 1;
  optimizer_eval_ledger.add(optimizer_eval_fact);
  auto optimizer_eval_options = validation_eval_options;
  optimizer_eval_options.exposure_ledger = &optimizer_eval_ledger;
  target::lattice_target_evaluator_t optimizer_eval(validation_eval_specs,
                                                    optimizer_eval_options);
  const auto optimizer_eval_ready =
      optimizer_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(optimizer_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            optimizer_eval_ready.proof_certificate.coverage.size() == 1 &&
            optimizer_eval_ready.proof_certificate.coverage.front()
                .contributing_facts.empty() &&
            optimizer_eval_ready.exposure_summaries.size() == 1 &&
            optimizer_eval_ready.exposure_summaries.front()
                    .loaded_anchor_events == 0,
        "validation evaluation coverage must ignore evaluation_metric facts "
        "that ran optimizer steps");
  expect_validation_model_state_deficit(
      optimizer_eval_ready,
      "validation evaluation evidence that ran optimizer steps should expose a "
      "model-state mutation deficit");
  exposure::lattice_exposure_ledger_t output_eval_ledger{};
  output_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  output_eval_ledger.add(make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                                                validation_eval_rep_checkpoint,
                                                100, 160));
  auto output_eval_fact = validation_eval_fact;
  output_eval_fact.job_id = "output_validation_eval";
  output_eval_fact.wave_action = "debug";
  output_eval_fact.use.mutated_component = false;
  output_eval_fact.optimizer_steps = 0;
  output_eval_fact.output_checkpoint =
      validation_eval_root / "unexpected_eval_output.pt";
  output_eval_ledger.add(output_eval_fact);
  auto output_eval_options = validation_eval_options;
  output_eval_options.exposure_ledger = &output_eval_ledger;
  target::lattice_target_evaluator_t output_eval(validation_eval_specs,
                                                 output_eval_options);
  const auto output_eval_ready =
      output_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(output_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            output_eval_ready.proof_certificate.coverage.size() == 1 &&
            output_eval_ready.proof_certificate.coverage.front()
                .contributing_facts.empty() &&
            output_eval_ready.exposure_summaries.size() == 1 &&
            output_eval_ready.exposure_summaries.front().loaded_anchor_events ==
                0,
        "validation evaluation coverage must ignore evaluation_metric facts "
        "that write output checkpoints");
  expect_validation_output_checkpoint_deficit(
      output_eval_ready,
      "validation evaluation evidence that writes an output checkpoint should "
      "expose a model-state output deficit");
  exposure::lattice_exposure_ledger_t mixed_eval_ledger{};
  mixed_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  mixed_eval_ledger.add(make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                                               validation_eval_rep_checkpoint,
                                               100, 160));
  mixed_eval_ledger.add(validation_eval_fact);
  auto mixed_optimizer_eval_fact = validation_eval_fact;
  mixed_optimizer_eval_fact.job_id = "mixed_optimizer_validation_eval";
  mixed_optimizer_eval_fact.wave_action = "debug";
  mixed_optimizer_eval_fact.use.mutated_component = false;
  mixed_optimizer_eval_fact.optimizer_steps = 1;
  mixed_eval_ledger.add(mixed_optimizer_eval_fact);
  auto mixed_eval_options = validation_eval_options;
  mixed_eval_options.exposure_ledger = &mixed_eval_ledger;
  target::lattice_target_evaluator_t mixed_eval(validation_eval_specs,
                                                mixed_eval_options);
  const auto mixed_eval_ready =
      mixed_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(mixed_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            mixed_eval_ready.proof_certificate.coverage.size() == 1 &&
            mixed_eval_ready.proof_certificate.coverage.front().passed &&
            mixed_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            mixed_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id,
        "clean validation evaluation coverage must not mask a second "
        "evaluation_metric fact that ran optimizer steps");
  expect_validation_model_state_deficit(
      mixed_eval_ready,
      "mixed clean and optimizer-step validation evidence should still expose "
      "a model-state mutation deficit");
  exposure::lattice_exposure_ledger_t mixed_output_eval_ledger{};
  mixed_output_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  mixed_output_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  mixed_output_eval_ledger.add(validation_eval_fact);
  auto mixed_output_eval_fact = validation_eval_fact;
  mixed_output_eval_fact.job_id = "mixed_output_validation_eval";
  mixed_output_eval_fact.wave_action = "debug";
  mixed_output_eval_fact.use.mutated_component = false;
  mixed_output_eval_fact.optimizer_steps = 0;
  mixed_output_eval_fact.output_checkpoint =
      validation_eval_root / "unexpected_mixed_eval_output.pt";
  mixed_output_eval_ledger.add(mixed_output_eval_fact);
  auto mixed_output_eval_options = validation_eval_options;
  mixed_output_eval_options.exposure_ledger = &mixed_output_eval_ledger;
  target::lattice_target_evaluator_t mixed_output_eval(
      validation_eval_specs, mixed_output_eval_options);
  const auto mixed_output_eval_ready =
      mixed_output_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(mixed_output_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            mixed_output_eval_ready.proof_certificate.coverage.size() == 1 &&
            mixed_output_eval_ready.proof_certificate.coverage.front().passed &&
            mixed_output_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            mixed_output_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id,
        "clean validation evaluation coverage must not mask a second "
        "evaluation_metric fact that writes an output checkpoint");
  expect_validation_output_checkpoint_deficit(
      mixed_output_eval_ready,
      "mixed clean and output-writing validation evidence should still expose "
      "a model-state output deficit");
  exposure::lattice_exposure_ledger_t out_of_range_bad_eval_ledger{};
  out_of_range_bad_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  out_of_range_bad_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  out_of_range_bad_eval_ledger.add(validation_eval_fact);
  auto out_of_range_mutated_eval_fact = validation_eval_fact;
  out_of_range_mutated_eval_fact.job_id = "out_of_range_mutated_eval";
  out_of_range_mutated_eval_fact.anchor_range = {.begin = 210, .end = 220};
  out_of_range_mutated_eval_fact.completed_anchor_range = {.begin = 210,
                                                           .end = 220};
  out_of_range_mutated_eval_fact.use.mutated_component = true;
  out_of_range_mutated_eval_fact.optimizer_steps = 1;
  out_of_range_bad_eval_ledger.add(out_of_range_mutated_eval_fact);
  auto out_of_range_output_eval_fact = validation_eval_fact;
  out_of_range_output_eval_fact.job_id = "out_of_range_output_eval";
  out_of_range_output_eval_fact.anchor_range = {.begin = 210, .end = 220};
  out_of_range_output_eval_fact.completed_anchor_range = {.begin = 210,
                                                          .end = 220};
  out_of_range_output_eval_fact.output_checkpoint =
      validation_eval_root / "out_of_range_eval_output.pt";
  out_of_range_bad_eval_ledger.add(out_of_range_output_eval_fact);
  auto out_of_range_extra_input_eval_fact = validation_eval_fact;
  out_of_range_extra_input_eval_fact.job_id = "out_of_range_extra_input_eval";
  out_of_range_extra_input_eval_fact.anchor_range = {.begin = 210, .end = 220};
  out_of_range_extra_input_eval_fact.completed_anchor_range = {.begin = 210,
                                                               .end = 220};
  out_of_range_extra_input_eval_fact.input_checkpoints.push_back(
      validation_eval_root / "out_of_range_extra_input.pt");
  out_of_range_bad_eval_ledger.add(out_of_range_extra_input_eval_fact);
  auto out_of_range_bad_eval_options = validation_eval_options;
  out_of_range_bad_eval_options.exposure_ledger = &out_of_range_bad_eval_ledger;
  target::lattice_target_evaluator_t out_of_range_bad_eval(
      validation_eval_specs, out_of_range_bad_eval_options);
  const auto out_of_range_bad_eval_ready =
      out_of_range_bad_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(out_of_range_bad_eval_ready.status ==
                target::lattice_target_status_t::satisfied &&
            out_of_range_bad_eval_ready.proof_certificate_check.passed &&
            out_of_range_bad_eval_ready.proof_certificate.coverage.size() ==
                1 &&
            out_of_range_bad_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            out_of_range_bad_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id &&
            out_of_range_bad_eval_ready.proof_certificate.coverage.front()
                    .load_summary.loaded_anchor_events == 10,
        "out-of-range validation evaluation model-state violations should not "
        "poison clean in-range evaluation coverage");
  exposure::lattice_exposure_ledger_t non_eval_model_state_ledger{};
  non_eval_model_state_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  non_eval_model_state_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  non_eval_model_state_ledger.add(validation_eval_fact);
  auto non_eval_mutating_fact = validation_eval_fact;
  non_eval_mutating_fact.job_id = "non_eval_mutating_validation_fact";
  non_eval_mutating_fact.use.evaluation_metric = false;
  non_eval_mutating_fact.use.observed_input = true;
  non_eval_mutating_fact.use.mutated_component = true;
  non_eval_mutating_fact.optimizer_steps = 1;
  non_eval_mutating_fact.output_checkpoint =
      validation_eval_root / "non_eval_mutating_output.pt";
  non_eval_model_state_ledger.add(non_eval_mutating_fact);
  auto non_eval_output_fact = validation_eval_fact;
  non_eval_output_fact.job_id = "non_eval_output_validation_fact";
  non_eval_output_fact.use.evaluation_metric = false;
  non_eval_output_fact.use.observed_input = true;
  non_eval_output_fact.output_checkpoint =
      validation_eval_root / "non_eval_output.pt";
  non_eval_output_fact.input_checkpoints.push_back(validation_eval_root /
                                                   "non_eval_extra_input.pt");
  non_eval_model_state_ledger.add(non_eval_output_fact);
  auto non_eval_model_state_options = validation_eval_options;
  non_eval_model_state_options.exposure_ledger = &non_eval_model_state_ledger;
  target::lattice_target_evaluator_t non_eval_model_state_eval(
      validation_eval_specs, non_eval_model_state_options);
  const auto non_eval_model_state_ready = non_eval_model_state_eval.evaluate(
      "clean_node_mdn_validation_eval_ready");
  check(non_eval_model_state_ready.status ==
                target::lattice_target_status_t::satisfied &&
            non_eval_model_state_ready.proof_certificate_check.passed &&
            non_eval_model_state_ready.proof_certificate.coverage.size() == 1 &&
            non_eval_model_state_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            non_eval_model_state_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id &&
            non_eval_model_state_ready.proof_certificate.coverage.front()
                    .load_summary.loaded_anchor_events == 10,
        "non-evaluation model-state facts over validation anchors should not "
        "poison clean evaluation_metric coverage");
  auto tampered_eval_coverage_certificate =
      validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  tampered_eval_coverage_certificate.coverage.front()
      .contributing_facts.clear();
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage without a "
        "local contributing fact witness");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  tampered_eval_coverage_certificate.coverage.front()
      .contributing_facts.front()
      .use.evaluation_metric = false;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "with the wrong exposure use");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.use.mutated_component = true;
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  {
    const auto tampered_mutation_check =
        target::verify_lattice_target_proof_certificate(
            tampered_eval_coverage_certificate);
    check(!tampered_mutation_check.passed &&
              std::any_of(tampered_mutation_check.issues.begin(),
                          tampered_mutation_check.issues.end(),
                          [](const auto &issue) {
                            return issue.find("evaluation coverage mutates "
                                              "component") != std::string::npos;
                          }),
          "proof certificate self-check rejects evaluation coverage witnesses "
          "that mutate model state");
  }
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.optimizer_steps = 1;
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  {
    const auto tampered_optimizer_check =
        target::verify_lattice_target_proof_certificate(
            tampered_eval_coverage_certificate);
    check(!tampered_optimizer_check.passed &&
              std::any_of(tampered_optimizer_check.issues.begin(),
                          tampered_optimizer_check.issues.end(),
                          [](const auto &issue) {
                            return issue.find("evaluation coverage has "
                                              "optimizer steps") !=
                                   std::string::npos;
                          }),
          "proof certificate self-check rejects evaluation coverage witnesses "
          "that ran optimizer steps");
  }
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_load =
        tampered_eval_coverage_certificate.coverage.front().load_summary;
    tampered_load.optimizer_steps_total = 1;
    tampered_load.optimizer_steps_per_unique_anchor =
        1.0 / static_cast<double>(tampered_load.unique_covered_anchors);
    tampered_load.optimizer_steps_per_cursor_epoch =
        1.0 / tampered_load.cursor_exposure_load;
  }
  {
    const auto tampered_load_check =
        target::verify_lattice_target_proof_certificate(
            tampered_eval_coverage_certificate);
    check(!tampered_load_check.passed &&
              std::any_of(tampered_load_check.issues.begin(),
                          tampered_load_check.issues.end(),
                          [](const auto &issue) {
                            return issue.find("evaluation coverage load has "
                                              "optimizer steps") !=
                                   std::string::npos;
                          }),
          "proof certificate self-check rejects evaluation coverage load "
          "summaries that report optimizer steps");
  }
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  tampered_eval_coverage_certificate.coverage.front()
      .contributing_facts.front()
      .completed_anchor_range.begin = 201;
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "whose completed range does not bind to the interval proof");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.output_checkpoint =
        validation_eval_root / "unexpected_eval_output.pt";
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "that write an output checkpoint");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.input_checkpoints = {
        validation_eval_rep_checkpoint,
        validation_eval_root / "wrong_mdn_input.pt",
    };
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "without the evaluated MDN checkpoint input edge");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.input_checkpoints = {
        validation_eval_mdn_checkpoint,
        validation_eval_root / "wrong_representation_input.pt",
    };
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "without the evaluated representation checkpoint input edge");
  tampered_eval_coverage_certificate = validation_eval_ready.proof_certificate;
  tampered_eval_coverage_certificate.certificate_digest.clear();
  {
    auto &tampered_fact = tampered_eval_coverage_certificate.coverage.front()
                              .contributing_facts.front();
    tampered_fact.input_checkpoints.push_back(validation_eval_root /
                                              "unexpected_extra_input.pt");
    tampered_eval_coverage_certificate.coverage.front()
        .contributing_fact_digests.front() =
        exposure::exposure_fact_digest(tampered_fact);
  }
  check(!target::verify_lattice_target_proof_certificate(
             tampered_eval_coverage_certificate)
             .passed,
        "proof certificate self-check rejects evaluation coverage witnesses "
        "with extra checkpoint input edges");
  auto malformed_evaluated_source_specs = validation_eval_specs;
  auto malformed_evaluated_source = std::find_if(
      malformed_evaluated_source_specs.begin(),
      malformed_evaluated_source_specs.end(),
      [](const target::lattice_target_spec_t &spec) {
        return spec.target_id == "clean_node_mdn_no_validation_leakage";
      });
  check(malformed_evaluated_source != malformed_evaluated_source_specs.end(),
        "validation eval fixture should include evaluated checkpoint source "
        "target");
  malformed_evaluated_source->target_spec_fingerprint.clear();
  auto malformed_evaluated_consumer = std::find_if(
      malformed_evaluated_source_specs.begin(),
      malformed_evaluated_source_specs.end(),
      [](const target::lattice_target_spec_t &spec) {
        return spec.target_id == "clean_node_mdn_validation_eval_ready";
      });
  check(malformed_evaluated_consumer != malformed_evaluated_source_specs.end(),
        "validation eval fixture should include evaluated checkpoint consumer "
        "target");
  malformed_evaluated_consumer->upstream_target_id.clear();
  target::lattice_target_evaluator_t malformed_evaluated_source_eval(
      malformed_evaluated_source_specs, validation_eval_options);
  const auto blocked_by_malformed_evaluated_source =
      malformed_evaluated_source_eval.evaluate(
          "clean_node_mdn_validation_eval_ready");
  check(blocked_by_malformed_evaluated_source.status ==
                target::lattice_target_status_t::blocked &&
            std::any_of(blocked_by_malformed_evaluated_source.deficits.begin(),
                        blocked_by_malformed_evaluated_source.deficits.end(),
                        [](const auto &deficit) {
                          return deficit.key ==
                                     "dependency:evaluated_checkpoint_source" &&
                                 deficit.status == "blocked";
                        }),
        "EVALUATED_CHECKPOINT_SOURCE latest_satisfying should fail closed when "
        "its source target has a malformed proof certificate");

  const auto checkpointless_eval_source_root =
      make_tmp_dir("checkpointless_evaluated_source");
  const auto checkpointless_eval_rep =
      checkpointless_eval_source_root / "rep.pt";
  const auto checkpointless_eval_source_dir =
      checkpointless_eval_source_root / "mdn_without_checkpoint";
  write_text(checkpointless_eval_rep, "representation checkpoint");
  write_text(checkpointless_eval_source_dir / "job.manifest",
             "job_id=mdn_without_checkpoint\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "mdn_assembly_fingerprint=mdn_1\n");
  write_text(checkpointless_eval_source_dir / "job.state",
             "status=completed\n"
             "optimizer_steps=0\n"
             "last_loss=0.20\n"
             "checkpoint_written=false\n");
  write_text(checkpointless_eval_source_dir / "inference.report",
             std::string("optimizer_steps=0\n"
                         "mean_loss=0.20\n"
                         "representation_checkpoint_loaded=true\n"
                         "allow_untrained_representation=false\n"
                         "representation_checkpoint_path=") +
                 checkpointless_eval_rep.string() +
                 "\n"
                 "mean_valid_node_target_fraction=0.25\n"
                 "last_active_node_head_count=1\n"
                 "last_evaluated_node_head_count=1\n"
                 "checkpoint_written=false\n");
  const auto checkpointless_eval_source_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mdn_without_checkpoint_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = validation_eval_requires_checkpoint_source;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:mdn_without_checkpoint_ready;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 200;
  ANCHOR_INDEX_END = 210;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  MIN_EVALUATION_METRIC_COVERAGE = 1.0;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  auto checkpointless_eval_source_options = options;
  checkpointless_eval_source_options.runtime_root =
      checkpointless_eval_source_root;
  target::lattice_target_evaluator_t checkpointless_eval_source_eval(
      checkpointless_eval_source_specs, checkpointless_eval_source_options);
  const auto checkpointless_eval_source =
      checkpointless_eval_source_eval.evaluate(
          "validation_eval_requires_checkpoint_source");
  check(checkpointless_eval_source.status ==
            target::lattice_target_status_t::missing_checkpoint,
        "EVALUATED_CHECKPOINT_SOURCE latest_satisfying should fail when the "
        "satisfied source target has no checkpoint artifact");
  check(
      checkpointless_eval_source.proof_certificate_check.passed,
      "evaluated-checkpoint artifact failure should keep a well-formed "
      "non-claiming proof certificate: " +
          join_strings(
              checkpointless_eval_source.proof_certificate_check.issues, "; "));
  check(std::any_of(checkpointless_eval_source.deficits.begin(),
                    checkpointless_eval_source.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:checkpoint" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "evaluated-checkpoint artifact failure should expose an artifact "
        "checkpoint deficit");
  check(std::any_of(checkpointless_eval_source.deficits.begin(),
                    checkpointless_eval_source.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "dependency:evaluated_checkpoint_source" &&
                             deficit.kind == "dependency" &&
                             deficit.status == "missing_checkpoint";
                    }),
        "evaluated-checkpoint artifact failure should expose an evaluated "
        "checkpoint source dependency deficit");
  expect_plan_basis_projects_deficits(
      checkpointless_eval_source,
      "evaluated-checkpoint artifact failure should project proof deficits");
  std::filesystem::remove_all(checkpointless_eval_source_root);

  exposure::lattice_exposure_ledger_t missing_eval_ledger{};
  missing_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  missing_eval_ledger.add(make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                                                 validation_eval_rep_checkpoint,
                                                 100, 160));
  target::lattice_target_evaluator_options_t missing_eval_options =
      validation_eval_options;
  missing_eval_options.exposure_ledger = &missing_eval_ledger;
  target::lattice_target_evaluator_t missing_eval(validation_eval_specs,
                                                  missing_eval_options);
  const auto missing_eval_ready =
      missing_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(missing_eval_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation target should fail without evaluation_metric "
        "exposure facts");
  exposure::lattice_exposure_ledger_t extra_eval_input_ledger{};
  extra_eval_input_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  extra_eval_input_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  auto extra_eval_input_fact = validation_eval_fact;
  extra_eval_input_fact.job_id = "validation_eval_extra_input_checkpoint";
  extra_eval_input_fact.input_checkpoints.push_back(
      validation_eval_root / "unexpected_extra_input.pt");
  extra_eval_input_ledger.add(extra_eval_input_fact);
  auto extra_eval_input_options = validation_eval_options;
  extra_eval_input_options.exposure_ledger = &extra_eval_input_ledger;
  target::lattice_target_evaluator_t extra_eval_input(validation_eval_specs,
                                                      extra_eval_input_options);
  const auto extra_eval_input_ready =
      extra_eval_input.evaluate("clean_node_mdn_validation_eval_ready");
  check(extra_eval_input_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation target should fail when an evaluation witness "
        "loads extra checkpoint model state");
  check(extra_eval_input_ready.proof_certificate.coverage.size() == 1 &&
            !extra_eval_input_ready.proof_certificate.coverage.front().passed &&
            extra_eval_input_ready.proof_certificate.coverage.front()
                .contributing_facts.empty() &&
            extra_eval_input_ready.proof_certificate.coverage.front()
                    .load_summary.loaded_anchor_events == 0,
        "validation evaluation with only bad checkpoint inputs should retain a "
        "self-consistent empty coverage proof");
  expect_validation_input_checkpoint_deficit(
      extra_eval_input_ready,
      "validation evaluation evidence with an extra checkpoint input should "
      "expose a model-state input deficit");
  exposure::lattice_exposure_ledger_t mixed_extra_eval_input_ledger{};
  mixed_extra_eval_input_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  mixed_extra_eval_input_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  mixed_extra_eval_input_ledger.add(validation_eval_fact);
  auto mixed_extra_eval_input_fact = validation_eval_fact;
  mixed_extra_eval_input_fact.job_id =
      "mixed_validation_eval_extra_input_checkpoint";
  mixed_extra_eval_input_fact.input_checkpoints.push_back(
      validation_eval_root / "unexpected_mixed_extra_input.pt");
  mixed_extra_eval_input_ledger.add(mixed_extra_eval_input_fact);
  auto mixed_extra_eval_input_options = validation_eval_options;
  mixed_extra_eval_input_options.exposure_ledger =
      &mixed_extra_eval_input_ledger;
  target::lattice_target_evaluator_t mixed_extra_eval_input(
      validation_eval_specs, mixed_extra_eval_input_options);
  const auto mixed_extra_eval_input_ready =
      mixed_extra_eval_input.evaluate("clean_node_mdn_validation_eval_ready");
  check(mixed_extra_eval_input_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "clean validation evaluation coverage must not mask a second "
        "evaluation_metric fact that loads extra checkpoint model state");
  check(mixed_extra_eval_input_ready.proof_certificate.coverage.size() == 1 &&
            mixed_extra_eval_input_ready.proof_certificate.coverage.front()
                .passed &&
            mixed_extra_eval_input_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            mixed_extra_eval_input_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == validation_eval_fact.job_id &&
            mixed_extra_eval_input_ready.proof_certificate.coverage.front()
                    .load_summary.loaded_anchor_events == 10,
        "mixed clean and bad-input validation evidence should keep the proof "
        "bound to the clean evaluation_metric witness");
  expect_validation_input_checkpoint_deficit(
      mixed_extra_eval_input_ready,
      "mixed clean and extra-input validation evidence should still expose a "
      "model-state input deficit");
  exposure::lattice_exposure_ledger_t partial_bad_input_eval_ledger{};
  partial_bad_input_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  partial_bad_input_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  auto partial_clean_eval_fact = validation_eval_fact;
  partial_clean_eval_fact.job_id = "partial_clean_validation_eval";
  partial_clean_eval_fact.anchor_range = {.begin = 200, .end = 205};
  partial_clean_eval_fact.completed_anchor_range = {.begin = 200, .end = 205};
  partial_bad_input_eval_ledger.add(partial_clean_eval_fact);
  auto partial_bad_input_eval_fact = validation_eval_fact;
  partial_bad_input_eval_fact.job_id = "partial_bad_input_validation_eval";
  partial_bad_input_eval_fact.anchor_range = {.begin = 205, .end = 210};
  partial_bad_input_eval_fact.completed_anchor_range = {.begin = 205,
                                                        .end = 210};
  partial_bad_input_eval_fact.input_checkpoints.push_back(
      validation_eval_root / "partial_bad_extra_input.pt");
  partial_bad_input_eval_ledger.add(partial_bad_input_eval_fact);
  auto partial_bad_input_eval_options = validation_eval_options;
  partial_bad_input_eval_options.exposure_ledger =
      &partial_bad_input_eval_ledger;
  target::lattice_target_evaluator_t partial_bad_input_eval(
      validation_eval_specs, partial_bad_input_eval_options);
  const auto partial_bad_input_eval_ready =
      partial_bad_input_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(partial_bad_input_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            partial_bad_input_eval_ready.proof_certificate.coverage.size() ==
                1 &&
            !partial_bad_input_eval_ready.proof_certificate.coverage.front()
                 .passed &&
            partial_bad_input_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            partial_bad_input_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == partial_clean_eval_fact.job_id &&
            partial_bad_input_eval_ready.proof_certificate.coverage.front()
                    .covered_anchors == 5 &&
            partial_bad_input_eval_ready.proof_certificate.coverage.front()
                    .missing_anchors == 5 &&
            std::abs(
                partial_bad_input_eval_ready.proof_certificate.coverage.front()
                    .actual_fraction -
                0.5) < 1e-12,
        "partial clean validation evaluation evidence should retain its "
        "coverage proof when the remaining interval has bad checkpoint input");
  expect_validation_input_checkpoint_deficit(
      partial_bad_input_eval_ready,
      "partial clean plus bad-input validation evidence should prioritize the "
      "model-state input deficit");
  exposure::lattice_exposure_ledger_t partial_mutating_eval_ledger{};
  partial_mutating_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  partial_mutating_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  auto partial_mutating_clean_fact = validation_eval_fact;
  partial_mutating_clean_fact.job_id = "partial_clean_mutating_validation_eval";
  partial_mutating_clean_fact.anchor_range = {.begin = 200, .end = 205};
  partial_mutating_clean_fact.completed_anchor_range = {.begin = 200,
                                                        .end = 205};
  partial_mutating_eval_ledger.add(partial_mutating_clean_fact);
  auto partial_mutating_bad_fact = validation_eval_fact;
  partial_mutating_bad_fact.job_id = "partial_mutating_validation_eval";
  partial_mutating_bad_fact.anchor_range = {.begin = 205, .end = 210};
  partial_mutating_bad_fact.completed_anchor_range = {.begin = 205, .end = 210};
  partial_mutating_bad_fact.use.mutated_component = true;
  partial_mutating_bad_fact.optimizer_steps = 1;
  partial_mutating_eval_ledger.add(partial_mutating_bad_fact);
  auto partial_mutating_eval_options = validation_eval_options;
  partial_mutating_eval_options.exposure_ledger = &partial_mutating_eval_ledger;
  target::lattice_target_evaluator_t partial_mutating_eval(
      validation_eval_specs, partial_mutating_eval_options);
  const auto partial_mutating_eval_ready =
      partial_mutating_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(partial_mutating_eval_ready.status ==
                target::lattice_target_status_t::exposure_failed &&
            partial_mutating_eval_ready.proof_certificate.coverage.size() ==
                1 &&
            !partial_mutating_eval_ready.proof_certificate.coverage.front()
                 .passed &&
            partial_mutating_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.size() == 1 &&
            partial_mutating_eval_ready.proof_certificate.coverage.front()
                    .contributing_facts.front()
                    .job_id == partial_mutating_clean_fact.job_id &&
            partial_mutating_eval_ready.proof_certificate.coverage.front()
                    .covered_anchors == 5 &&
            partial_mutating_eval_ready.proof_certificate.coverage.front()
                    .missing_anchors == 5 &&
            std::abs(
                partial_mutating_eval_ready.proof_certificate.coverage.front()
                    .actual_fraction -
                0.5) < 1e-12,
        "partial clean validation evaluation evidence should retain its "
        "coverage proof when the remaining interval mutates model state");
  expect_validation_model_state_deficit(
      partial_mutating_eval_ready,
      "partial clean plus mutating validation evidence should prioritize the "
      "model-state mutation deficit");
  exposure::lattice_exposure_ledger_t partial_output_eval_ledger{};
  partial_output_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 160));
  partial_output_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 160));
  auto partial_output_clean_fact = validation_eval_fact;
  partial_output_clean_fact.job_id = "partial_clean_output_validation_eval";
  partial_output_clean_fact.anchor_range = {.begin = 200, .end = 205};
  partial_output_clean_fact.completed_anchor_range = {.begin = 200, .end = 205};
  partial_output_eval_ledger.add(partial_output_clean_fact);
  auto partial_output_bad_fact = validation_eval_fact;
  partial_output_bad_fact.job_id = "partial_output_validation_eval";
  partial_output_bad_fact.anchor_range = {.begin = 205, .end = 210};
  partial_output_bad_fact.completed_anchor_range = {.begin = 205, .end = 210};
  partial_output_bad_fact.output_checkpoint =
      validation_eval_root / "partial_output_validation_eval.pt";
  partial_output_eval_ledger.add(partial_output_bad_fact);
  auto partial_output_eval_options = validation_eval_options;
  partial_output_eval_options.exposure_ledger = &partial_output_eval_ledger;
  target::lattice_target_evaluator_t partial_output_eval(
      validation_eval_specs, partial_output_eval_options);
  const auto partial_output_eval_ready =
      partial_output_eval.evaluate("clean_node_mdn_validation_eval_ready");
  check(
      partial_output_eval_ready.status ==
              target::lattice_target_status_t::exposure_failed &&
          partial_output_eval_ready.proof_certificate.coverage.size() == 1 &&
          !partial_output_eval_ready.proof_certificate.coverage.front()
               .passed &&
          partial_output_eval_ready.proof_certificate.coverage.front()
                  .contributing_facts.size() == 1 &&
          partial_output_eval_ready.proof_certificate.coverage.front()
                  .contributing_facts.front()
                  .job_id == partial_output_clean_fact.job_id &&
          partial_output_eval_ready.proof_certificate.coverage.front()
                  .covered_anchors == 5 &&
          partial_output_eval_ready.proof_certificate.coverage.front()
                  .missing_anchors == 5 &&
          std::abs(partial_output_eval_ready.proof_certificate.coverage.front()
                       .actual_fraction -
                   0.5) < 1e-12,
      "partial clean validation evaluation evidence should retain its "
      "coverage proof when the remaining interval writes output checkpoint");
  expect_validation_output_checkpoint_deficit(
      partial_output_eval_ready,
      "partial clean plus output-writing validation evidence should prioritize "
      "the model-state output deficit");

  auto evaluate_validation_fixture = [&](const std::string &label,
                                         bool load_mdn, bool load_rep,
                                         bool use_correct_mdn,
                                         bool use_correct_rep,
                                         bool write_output_checkpoint = false) {
    const auto root = make_tmp_dir(label);
    const auto trained_mdn = root / "mdn_train.pt";
    const auto trained_rep = root / "rep_train.pt";
    write_ranged_mdn_job(root, trained_mdn, trained_rep, 100, 160);
    const auto eval_mdn = use_correct_mdn ? trained_mdn : root / "wrong_mdn.pt";
    const auto eval_rep = use_correct_rep ? trained_rep : root / "wrong_rep.pt";
    if (!use_correct_rep) {
      write_text(eval_rep, "representation checkpoint");
    }
    write_ranged_mdn_eval_job(root, eval_mdn, eval_rep, 200, 210, load_mdn,
                              load_rep, write_output_checkpoint);
    exposure::lattice_exposure_ledger_t ledger{};
    ledger.add(make_rep_exposure_fact(trained_rep, 100, 160));
    ledger.add(make_mdn_exposure_fact(trained_mdn, trained_rep, 100, 160));
    ledger.add(exposure::make_exposure_fact_from_job_dir(
        root / "mdn_validation_eval"));
    target::lattice_target_evaluator_options_t eval_options = options;
    eval_options.runtime_root = root;
    eval_options.exposure_ledger = &ledger;
    target::lattice_target_evaluator_t evaluator(validation_eval_specs,
                                                 eval_options);
    const auto eval =
        evaluator.evaluate("clean_node_mdn_validation_eval_ready");
    std::filesystem::remove_all(root);
    return eval;
  };

  const auto no_mdn_loaded_eval = evaluate_validation_fixture(
      "validation_eval_no_mdn_checkpoint", /*load_mdn=*/false,
      /*load_rep=*/true, /*use_correct_mdn=*/true,
      /*use_correct_rep=*/true);
  check(no_mdn_loaded_eval.status ==
            target::lattice_target_status_t::missing_checkpoint,
        "validation evaluation should fail when no MDN checkpoint was loaded");
  check(std::any_of(no_mdn_loaded_eval.deficits.begin(),
                    no_mdn_loaded_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "model_state:input_checkpoint" &&
                             deficit.kind == "model_state" &&
                             deficit.status == "missing" &&
                             deficit.priority == 25 &&
                             deficit.priority_class == "model_state";
                    }),
        "validation evaluation without a loaded MDN checkpoint should expose a "
        "model-state input deficit");

  const auto wrong_mdn_loaded_eval = evaluate_validation_fixture(
      "validation_eval_wrong_mdn_checkpoint", /*load_mdn=*/true,
      /*load_rep=*/true, /*use_correct_mdn=*/false,
      /*use_correct_rep=*/true);
  check(wrong_mdn_loaded_eval.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation should fail when it loaded the wrong MDN "
        "checkpoint");
  check(std::any_of(wrong_mdn_loaded_eval.deficits.begin(),
                    wrong_mdn_loaded_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "dependency:mdn_checkpoint" &&
                             deficit.kind == "dependency" &&
                             deficit.dimension == "mdn_checkpoint" &&
                             deficit.status == "mismatch";
                    }),
        "wrong validation MDN checkpoint should include a checkpoint mismatch "
        "deficit");
  check(std::any_of(wrong_mdn_loaded_eval.deficits.begin(),
                    wrong_mdn_loaded_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "model_state:input_checkpoint" &&
                             deficit.kind == "model_state" &&
                             deficit.status == "mismatch";
                    }),
        "wrong validation MDN checkpoint should include a model-state input "
        "deficit");

  const auto wrong_rep_loaded_eval = evaluate_validation_fixture(
      "validation_eval_wrong_rep_checkpoint", /*load_mdn=*/true,
      /*load_rep=*/true, /*use_correct_mdn=*/true,
      /*use_correct_rep=*/false);
  check(wrong_rep_loaded_eval.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation should fail when it loaded the wrong "
        "representation checkpoint");
  check(std::any_of(wrong_rep_loaded_eval.deficits.begin(),
                    wrong_rep_loaded_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "dependency:representation_checkpoint" &&
                             deficit.kind == "dependency" &&
                             deficit.dimension == "representation_checkpoint" &&
                             deficit.status == "mismatch";
                    }),
        "wrong validation representation checkpoint should include a "
        "checkpoint mismatch deficit");
  check(std::any_of(wrong_rep_loaded_eval.deficits.begin(),
                    wrong_rep_loaded_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "model_state:input_checkpoint" &&
                             deficit.kind == "model_state" &&
                             deficit.status == "mismatch";
                    }),
        "wrong validation representation checkpoint should include a "
        "model-state input deficit");
  const auto writes_checkpoint_eval = evaluate_validation_fixture(
      "validation_eval_writes_checkpoint", /*load_mdn=*/true,
      /*load_rep=*/true, /*use_correct_mdn=*/true,
      /*use_correct_rep=*/true, /*write_output_checkpoint=*/true);
  check(writes_checkpoint_eval.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation should fail when it writes a new checkpoint");
  check(std::any_of(
            writes_checkpoint_eval.reasons.begin(),
            writes_checkpoint_eval.reasons.end(),
            [](const auto &reason) {
              return reason ==
                     target::k_lattice_read_only_validation_model_state_reason;
            }),
        "checkpoint-writing validation evaluation should explain the read-only "
        "model-state violation");
  check(std::any_of(writes_checkpoint_eval.deficits.begin(),
                    writes_checkpoint_eval.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "model_state:output_checkpoint" &&
                             deficit.kind == "model_state" &&
                             deficit.dimension == "output_checkpoint" &&
                             deficit.status == "forbidden" &&
                             deficit.priority == 25 &&
                             deficit.priority_class == "model_state";
                    }),
        "checkpoint-writing validation evaluation should expose a model-state "
        "output checkpoint deficit");
  check(writes_checkpoint_eval.plan_basis.available &&
            writes_checkpoint_eval.plan_basis.primary_deficit_key ==
                "model_state:output_checkpoint" &&
            writes_checkpoint_eval.plan_basis.primary_deficit_priority == 25 &&
            writes_checkpoint_eval.plan_basis.primary_deficit_priority_class ==
                "model_state",
        "plan basis should project checkpoint-writing validation evidence as a "
        "model-state deficit");
  expect_plan_basis_projects_deficits(
      writes_checkpoint_eval,
      "checkpoint-writing validation evaluation should project deficits into "
      "plan_basis");

  const auto latest_checkpoint_guard_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_node_mdn_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = ranged_node_mdn_no_validation_leakage;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:ranged_node_mdn_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  target::lattice_target_evaluator_t latest_checkpoint_guard_eval(
      latest_checkpoint_guard_specs, mdn_exposure_options);
  const auto latest_checkpoint_guard = latest_checkpoint_guard_eval.evaluate(
      "ranged_node_mdn_no_validation_leakage");
  check(latest_checkpoint_guard.status ==
            target::lattice_target_status_t::exposure_failed,
        "CHECKPOINT_SOURCE latest_satisfying guard should inspect the source "
        "target checkpoint closure");
  auto malformed_latest_checkpoint_guard_specs = latest_checkpoint_guard_specs;
  auto malformed_latest_source =
      std::find_if(malformed_latest_checkpoint_guard_specs.begin(),
                   malformed_latest_checkpoint_guard_specs.end(),
                   [](const target::lattice_target_spec_t &spec) {
                     return spec.target_id == "ranged_node_mdn_ready";
                   });
  check(malformed_latest_source !=
            malformed_latest_checkpoint_guard_specs.end(),
        "latest_satisfying fixture should include the source target");
  malformed_latest_source->target_spec_fingerprint.clear();
  target::lattice_target_evaluator_t malformed_latest_checkpoint_guard_eval(
      malformed_latest_checkpoint_guard_specs, mdn_exposure_options);
  const auto malformed_latest_checkpoint_guard =
      malformed_latest_checkpoint_guard_eval.evaluate(
          "ranged_node_mdn_no_validation_leakage");
  check(malformed_latest_checkpoint_guard.status ==
                target::lattice_target_status_t::blocked &&
            std::any_of(malformed_latest_checkpoint_guard.deficits.begin(),
                        malformed_latest_checkpoint_guard.deficits.end(),
                        [](const auto &deficit) {
                          return deficit.key ==
                                     "dependency:checkpoint_source" &&
                                 deficit.status == "blocked";
                        }),
        "CHECKPOINT_SOURCE latest_satisfying should fail closed when its "
        "source target has a malformed proof certificate");

  const auto checkpointless_source_root =
      make_tmp_dir("checkpointless_source_guard");
  const auto checkpointless_source_dir =
      checkpointless_source_root / "rep_without_checkpoint";
  write_text(checkpointless_source_dir / "job.manifest",
             "job_id=rep_without_checkpoint\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_action=train\n"
             "source_range_policy=all\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(checkpointless_source_dir / "job.state",
             "status=completed\n"
             "checkpoint_written=false\n");
  const auto checkpointless_source_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = rep_without_checkpoint_ready;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 1;
};

LATTICE_TARGET {
  TARGET_ID = rep_checkpoint_source_guard;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = representation_ready;
  COMPONENT = wikimyei.representation.encoding.vicreg;
  CHECKPOINT_SOURCE = latest_satisfying:rep_without_checkpoint_ready;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  auto checkpointless_source_options = options;
  checkpointless_source_options.runtime_root = checkpointless_source_root;
  target::lattice_target_evaluator_t checkpointless_source_eval(
      checkpointless_source_specs, checkpointless_source_options);
  const auto checkpointless_source_guard =
      checkpointless_source_eval.evaluate("rep_checkpoint_source_guard");
  check(checkpointless_source_guard.status ==
            target::lattice_target_status_t::missing_checkpoint,
        "CHECKPOINT_SOURCE latest_satisfying should fail when the satisfied "
        "source target has no checkpoint artifact");
  check(checkpointless_source_guard.proof_certificate_check.passed,
        "checkpoint-source artifact failure should keep a well-formed "
        "non-claiming proof certificate: " +
            join_strings(
                checkpointless_source_guard.proof_certificate_check.issues,
                "; "));
  check(std::any_of(checkpointless_source_guard.deficits.begin(),
                    checkpointless_source_guard.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "artifact:checkpoint" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "checkpoint-source artifact failure should expose an artifact "
        "checkpoint deficit");
  check(std::any_of(checkpointless_source_guard.deficits.begin(),
                    checkpointless_source_guard.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "dependency:checkpoint_source" &&
                             deficit.kind == "dependency" &&
                             deficit.status == "missing_checkpoint";
                    }),
        "checkpoint-source artifact failure should expose a checkpoint-source "
        "dependency deficit");
  expect_plan_basis_projects_deficits(
      checkpointless_source_guard,
      "checkpoint-source artifact failure should project proof deficits");
  std::filesystem::remove_all(checkpointless_source_root);

  const auto test_holdout_root = make_tmp_dir("test_holdout_guard");
  const auto test_holdout_mdn_checkpoint =
      test_holdout_root / "mdn_test_guard.pt";
  const auto test_holdout_rep_checkpoint =
      test_holdout_root / "rep_test_guard.pt";
  write_ranged_mdn_job(test_holdout_root, test_holdout_mdn_checkpoint,
                       test_holdout_rep_checkpoint, 100, 198);
  exposure::lattice_exposure_ledger_t test_holdout_ledger{};
  test_holdout_ledger.add(
      make_rep_exposure_fact(test_holdout_rep_checkpoint, 100, 198));
  test_holdout_ledger.add(make_mdn_exposure_fact(
      test_holdout_mdn_checkpoint, test_holdout_rep_checkpoint, 100, 198));
  target::lattice_target_evaluator_options_t test_holdout_options = options;
  test_holdout_options.runtime_root = test_holdout_root;
  test_holdout_options.exposure_ledger = &test_holdout_ledger;
  test_holdout_options.split_policy =
      split::decode_lattice_split_policy_from_dsl(R"DSL(
LATTICE_SPLIT_POLICY {
  POLICY_ID = test_holdout_guard;
  CURSOR_DOMAIN = ujcamei.graph_anchor;
  PURGE_LEFT_CONTEXT = auto_from_Hx;
  PURGE_RIGHT_FUTURE = auto_from_Hf;
};

LATTICE_SPLIT {
  SPLIT_ID = train_core;
  ROLE = train;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 198;
};

LATTICE_SPLIT {
  SPLIT_ID = validation_holdout;
  ROLE = validation;
  ANCHOR_INDEX_BEGIN = 200;
  ANCHOR_INDEX_END = 210;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};

LATTICE_SPLIT {
  SPLIT_ID = test_holdout;
  ROLE = test;
  ANCHOR_INDEX_BEGIN = 190;
  ANCHOR_INDEX_END = 200;
  PROTECT_FROM_USES = observed_input|target_supervision|selection_signal;
  PROTECT_REQUIRES_MUTATED_COMPONENT = true;
};
)DSL");
  const auto test_holdout_guard_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = mdn_train_ready_for_holdout_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  TRAIN_SPLIT = train_core;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_TRAINED_NODE_HEAD_COUNT = 1;
  MIN_TARGET_SUPERVISION_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};

LATTICE_TARGET {
  TARGET_ID = mdn_no_validation_leakage_for_holdout_guard;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:mdn_train_ready_for_holdout_guard;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = validation_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};

LATTICE_TARGET {
  TARGET_ID = mdn_no_test_leakage_for_holdout_guard;
  TARGET_CLASS = leakage_guard;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  CHECKPOINT_SOURCE = latest_satisfying:mdn_no_validation_leakage_for_holdout_guard;
  SOURCE_RANGE = all;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = false;
  MIN_OPTIMIZER_STEPS = 0;
  PROTECT_SPLIT = test_holdout;
  PLAN_MODE = run|debug;
  MAX_WAVES = 0;
};
)DSL");
  target::lattice_target_evaluator_t test_holdout_eval(test_holdout_guard_specs,
                                                       test_holdout_options);
  const auto validation_clean_for_test_guard =
      test_holdout_eval.evaluate("mdn_no_validation_leakage_for_holdout_guard");
  check(validation_clean_for_test_guard.status ==
            target::lattice_target_status_t::satisfied,
        "validation holdout guard should pass when only test holdout is "
        "overlapped");
  const auto test_leaking_guard =
      test_holdout_eval.evaluate("mdn_no_test_leakage_for_holdout_guard");
  check(test_leaking_guard.status ==
            target::lattice_target_status_t::exposure_failed,
        "test holdout guard should fail on purge-expanded test overlap");

  exposure::lattice_exposure_ledger_t unresolved_mdn_ledger{};
  unresolved_mdn_ledger.add(make_mdn_exposure_fact(
      mdn_exposure_checkpoint, mdn_rep_checkpoint, 100, 200));
  target::lattice_target_evaluator_options_t unresolved_mdn_options =
      mdn_exposure_options;
  unresolved_mdn_options.exposure_ledger = &unresolved_mdn_ledger;
  target::lattice_target_evaluator_t unresolved_mdn_eval(
      mdn_coverage_specs, unresolved_mdn_options);
  const auto unresolved_mdn_ready =
      unresolved_mdn_eval.evaluate("ranged_node_mdn_ready");
  check(unresolved_mdn_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "MDN checkpoint exposure must fail closed when the loaded "
        "representation checkpoint has no producer fact");
  check(std::any_of(unresolved_mdn_ready.deficits.begin(),
                    unresolved_mdn_ready.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key == "closure:checkpoint_lineage" &&
                             deficit.kind == "closure" &&
                             deficit.status == "incomplete";
                    }),
        "unresolved MDN checkpoint inputs should expose a checkpoint-lineage "
        "deficit");

  exposure::lattice_exposure_ledger_t stale_rep_producer_ledger{};
  auto stale_rep_producer =
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200);
  stale_rep_producer.component_assembly_fingerprint = "old_vicreg";
  stale_rep_producer.job_id = "old_rep_ranged";
  stale_rep_producer_ledger.add(stale_rep_producer);
  stale_rep_producer_ledger.add(make_mdn_exposure_fact(
      mdn_exposure_checkpoint, mdn_rep_checkpoint, 100, 200));
  target::lattice_target_evaluator_options_t stale_rep_producer_options =
      mdn_exposure_options;
  stale_rep_producer_options.exposure_ledger = &stale_rep_producer_ledger;
  target::lattice_target_evaluator_t stale_rep_producer_eval(
      mdn_coverage_specs, stale_rep_producer_options);
  const auto stale_rep_producer_ready =
      stale_rep_producer_eval.evaluate("ranged_node_mdn_ready");
  check(stale_rep_producer_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "MDN readiness must fail when the loaded representation checkpoint has "
        "only stale producer facts");
  check(std::any_of(stale_rep_producer_ready.deficits.begin(),
                    stale_rep_producer_ready.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "artifact:"
                                 "representation_producer_exposure_fact" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "stale representation producers should expose a producer-fact "
        "artifact deficit");
  expect_plan_basis_projects_deficits(
      stale_rep_producer_ready,
      "stale representation producers should project proof deficits");

  exposure::lattice_exposure_ledger_t no_target_component_ledger{};
  no_target_component_ledger.add(
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200));
  no_target_component_ledger.add(
      make_rep_exposure_fact(mdn_exposure_checkpoint, 100, 200));
  target::lattice_target_evaluator_options_t no_target_component_options =
      mdn_exposure_options;
  no_target_component_options.exposure_ledger = &no_target_component_ledger;
  target::lattice_target_evaluator_t no_target_component_eval(
      mdn_coverage_specs, no_target_component_options);
  const auto no_target_component_ready =
      no_target_component_eval.evaluate("ranged_node_mdn_ready");
  check(no_target_component_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "checkpoint closure must include target-component-local evidence");
  check(std::any_of(no_target_component_ready.deficits.begin(),
                    no_target_component_ready.deficits.end(),
                    [](const auto &deficit) {
                      return deficit.key ==
                                 "artifact:target_component_exposure_fact" &&
                             deficit.kind == "artifact" &&
                             deficit.status == "missing";
                    }),
        "checkpoint closures without target-component-local exposure should "
        "expose an artifact deficit");
  expect_plan_basis_projects_deficits(
      no_target_component_ready,
      "target-component-local exposure gaps should project proof deficits");

  const auto mdn_observed_coverage_specs =
      target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_node_mdn_observed_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  MIN_OBSERVED_INPUT_COVERAGE = 1.0;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  exposure::lattice_exposure_ledger_t component_scope_ledger{};
  component_scope_ledger.add(
      make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200));
  component_scope_ledger.add(make_mdn_exposure_fact(
      mdn_exposure_checkpoint, mdn_rep_checkpoint, 150, 200));
  target::lattice_target_evaluator_options_t component_scope_options =
      mdn_exposure_options;
  component_scope_options.exposure_ledger = &component_scope_ledger;
  target::lattice_target_evaluator_t component_scope_eval(
      mdn_observed_coverage_specs, component_scope_options);
  const auto component_scope_ready =
      component_scope_eval.evaluate("ranged_node_mdn_observed_ready");
  check(component_scope_ready.status ==
            target::lattice_target_status_t::exposure_failed,
        "MDN readiness coverage must be target-component-local and not "
        "satisfied by upstream VICReg exposure");

  const auto mdn_forbid_specs = target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = ranged_node_mdn_no_future_leak;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 200;
  REQUIRE_CONTRACT_MATCH = true;
  REQUIRE_COMPONENT_MATCH = true;
  REQUIRE_CHECKPOINT_EXISTS = true;
  REQUIRE_FINITE_LOSS = true;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  REQUIRE_ACTIVE_NODE_HEAD_COUNT = 1;
  REQUIRE_EVALUATED_NODE_HEAD_COUNT = 1;
  FORBID_EXPOSURE_ANCHOR_INDEX_BEGIN = 200;
  FORBID_EXPOSURE_ANCHOR_INDEX_END = 201;
  FORBID_EXPOSURE_USES = target_supervision;
  PLAN_MODE = train|debug;
  MAX_WAVES = 3;
};
)DSL");
  target::lattice_target_evaluator_t mdn_forbid_eval(mdn_forbid_specs,
                                                     mdn_exposure_options);
  const auto mdn_forbidden =
      mdn_forbid_eval.evaluate("ranged_node_mdn_no_future_leak");
  check(mdn_forbidden.status ==
            target::lattice_target_status_t::exposure_failed,
        "forbidden overlap should still use shifted target source-row "
        "footprint");
  std::filesystem::remove_all(validation_eval_root);
  std::filesystem::remove_all(mdn_exposure_root);
  std::filesystem::remove_all(exposure_root);

  std::cout << "kikijyeba lattice target tests passed\n";
  std::filesystem::remove_all(root);
  return 0;
}
