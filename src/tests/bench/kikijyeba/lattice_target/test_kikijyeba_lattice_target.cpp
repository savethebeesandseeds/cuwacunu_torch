#include "kikijyeba/lattice/target/lattice_target_evaluator.h"

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

void write_text(const std::filesystem::path &path, const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open output file: " + path.string());
  out << text;
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
  fact.valid_target_fraction = 0.25;
  fact.output_checkpoint = checkpoint;
  fact.input_checkpoints.push_back(representation_checkpoint);
  fact.finite_loss = true;
  fact.mean_loss = 0.40;
  fact.mean_nll = 0.40;
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
    bool load_representation_checkpoint = true) {
  const auto dir = root / "mdn_validation_eval";
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
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=0\n"
                                "last_loss=0.45\n"
                                "wave_streamed_anchor_count=" +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=1\n"
                                    "wave_pulses_skipped=0\n"
                                    "skipped_batches=0\n"
                                    "checkpoint_written=false\n");
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
      "mean_valid_node_target_fraction=0.25\n"
      "last_active_node_head_count=3\n"
      "last_evaluated_node_head_count=3\n"
      "checkpoint_written=false\n";
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

  run_acceptance_fixture_regression();

  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = root;
  options.split_policy = split_policy;
  options.active_identity.protocol_contract_fingerprint = "contract_1";
  options.active_identity.graph_order_fingerprint = "graph_1";
  options.active_identity.source_cursor_token = "cursor_ranged";
  options.active_identity.vicreg_assembly_fingerprint = "vicreg_1";
  options.active_identity.mdn_assembly_fingerprint = "mdn_1";

  target::lattice_target_evaluator_t empty_eval(specs, options);
  const auto missing_rep = empty_eval.evaluate("vicreg_representation_ready");
  check(missing_rep.status == target::lattice_target_status_t::missing_report,
        "missing representation evidence should be missing_report");
  check(missing_rep.plan_ready, "missing representation should be plannable");
  check(missing_rep.suggested_wave.target ==
            "wikimyei.representation.encoding.vicreg",
        "representation plan should target VICReg");

  const auto blocked_mdn = empty_eval.evaluate("node_mdn_ready");
  check(blocked_mdn.status == target::lattice_target_status_t::blocked,
        "node MDN should block on missing representation target");
  check(blocked_mdn.suggested_wave.target ==
            "wikimyei.representation.encoding.vicreg",
        "blocked node MDN should reuse upstream wave plan");

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
        "node MDN should be satisfied");

  target::lattice_target_evaluator_options_t no_identity_options{};
  no_identity_options.runtime_root = root;
  target::lattice_target_evaluator_t no_identity_eval(specs,
                                                      no_identity_options);
  const auto blocked_missing_identity =
      no_identity_eval.evaluate("vicreg_representation_ready");
  check(blocked_missing_identity.status ==
            target::lattice_target_status_t::blocked,
        "required active identity should block when not provided");

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
  check(stale_budget_result.plan_ready,
        "MAX_WAVES should ignore stale-contract attempts for active planning "
        "budget");
  std::filesystem::remove_all(stale_budget_root);

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
  std::filesystem::remove_all(missing_state_root);

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
  ledger.add(make_rep_exposure_fact(exposure_checkpoint, 0, 100));
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
)DSL");
  auto repeated_fact = make_rep_exposure_fact(exposure_checkpoint, 0, 100);
  repeated_fact.job_id = "rep_ranged_repeat";
  repeated_fact.optimizer_steps = 4;
  exposure::lattice_exposure_ledger_t warning_ledger{};
  warning_ledger.add(make_rep_exposure_fact(exposure_checkpoint, 0, 100));
  warning_ledger.add(repeated_fact);
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
  check(warned_ready.warning_results.size() == 3,
        "target should report every configured warning result");
  check(warned_ready.warning_results.front().status == "warning" &&
            std::abs(warned_ready.warning_results.front().measured_value -
                     2.0) < 1e-12,
        "repeated exposure load should trigger a warning");
  check(warned_ready.warning_results.front().message.find(
            "above threshold 1") != std::string::npos,
        "triggered warning message should say above threshold");
  check(warned_ready.warnings.size() == 2,
        "only triggered warnings should be mirrored in the warnings list");

  repeated_fact.use.mutated_component = false;
  exposure::lattice_exposure_ledger_t non_mutated_warning_ledger{};
  non_mutated_warning_ledger.add(
      make_rep_exposure_fact(exposure_checkpoint, 0, 100));
  non_mutated_warning_ledger.add(repeated_fact);
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
  check(non_mutated_warned_ready.warning_results.back().status == "clear" &&
            non_mutated_warned_ready.warning_results.back().message.find(
                "not below threshold 0.8") != std::string::npos,
        "clear anchor-domain warning message should say not below threshold");

  target::lattice_target_evaluator_options_t auto_ledger_options = options;
  auto_ledger_options.runtime_root = exposure_root;
  target::lattice_target_evaluator_t auto_ledger_eval(coverage_specs,
                                                      auto_ledger_options);
  const auto auto_ledger =
      auto_ledger_eval.evaluate("ranged_representation_ready");
  check(auto_ledger.status == target::lattice_target_status_t::satisfied,
        "target evaluator should auto-build exposure ledger from runtime jobs");

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

  target::lattice_target_evaluator_options_t missing_ledger_options =
      auto_ledger_options;
  missing_ledger_options.auto_build_exposure_ledger = false;
  target::lattice_target_evaluator_t missing_ledger_eval(
      coverage_specs, missing_ledger_options);
  const auto missing_ledger =
      missing_ledger_eval.evaluate("ranged_representation_ready");
  check(missing_ledger.status == target::lattice_target_status_t::blocked,
        "coverage targets should block when no exposure ledger is provided");

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
)DSL");
  exposure::lattice_exposure_ledger_t mdn_ledger{};
  mdn_ledger.add(make_rep_exposure_fact(mdn_rep_checkpoint, 100, 200));
  mdn_ledger.add(make_mdn_exposure_fact(mdn_exposure_checkpoint,
                                        mdn_rep_checkpoint, 100, 200));
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

  const auto validation_eval_root = make_tmp_dir("validation_eval");
  const auto validation_eval_mdn_checkpoint =
      validation_eval_root / "mdn_train.pt";
  const auto validation_eval_rep_checkpoint =
      validation_eval_root / "rep_train.pt";
  write_ranged_mdn_job(validation_eval_root, validation_eval_mdn_checkpoint,
                       validation_eval_rep_checkpoint, 100, 199);
  write_ranged_mdn_eval_job(validation_eval_root,
                            validation_eval_mdn_checkpoint,
                            validation_eval_rep_checkpoint, 200, 210);
  exposure::lattice_exposure_ledger_t validation_eval_ledger{};
  validation_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 199));
  validation_eval_ledger.add(
      make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                             validation_eval_rep_checkpoint, 100, 199));
  validation_eval_ledger.add(exposure::make_exposure_fact_from_job_dir(
      validation_eval_root / "mdn_validation_eval"));
  const auto validation_eval_specs = target::decode_lattice_targets_from_dsl(
      R"DSL(
LATTICE_TARGET {
  TARGET_ID = clean_node_mdn_train_ready;
  TARGET_KIND = node_mdn_ready;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 100;
  ANCHOR_INDEX_END = 199;
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
  PLAN_MODE = run|debug;
  MAX_WAVES = 1;
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
  check(validation_eval_ready.exposure_summaries.size() == 1 &&
            validation_eval_ready.exposure_summaries.front().use ==
                exposure::exposure_use_t::evaluation_metric,
        "validation evaluation target reports evaluation_metric exposure");
  check(validation_eval_ready.exposure_summaries.front().loaded_anchor_events ==
            10,
        "validation evaluation summary counts completed validation anchors");

  exposure::lattice_exposure_ledger_t missing_eval_ledger{};
  missing_eval_ledger.add(
      make_rep_exposure_fact(validation_eval_rep_checkpoint, 100, 199));
  missing_eval_ledger.add(make_mdn_exposure_fact(validation_eval_mdn_checkpoint,
                                                 validation_eval_rep_checkpoint,
                                                 100, 199));
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

  auto evaluate_validation_fixture = [&](const std::string &label,
                                         bool load_mdn, bool load_rep,
                                         bool use_correct_mdn,
                                         bool use_correct_rep) {
    const auto root = make_tmp_dir(label);
    const auto trained_mdn = root / "mdn_train.pt";
    const auto trained_rep = root / "rep_train.pt";
    write_ranged_mdn_job(root, trained_mdn, trained_rep, 100, 199);
    const auto eval_mdn = use_correct_mdn ? trained_mdn : root / "wrong_mdn.pt";
    const auto eval_rep = use_correct_rep ? trained_rep : root / "wrong_rep.pt";
    if (!use_correct_rep) {
      write_text(eval_rep, "representation checkpoint");
    }
    write_ranged_mdn_eval_job(root, eval_mdn, eval_rep, 200, 210, load_mdn,
                              load_rep);
    exposure::lattice_exposure_ledger_t ledger{};
    ledger.add(make_rep_exposure_fact(trained_rep, 100, 199));
    ledger.add(make_mdn_exposure_fact(trained_mdn, trained_rep, 100, 199));
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

  const auto wrong_mdn_loaded_eval = evaluate_validation_fixture(
      "validation_eval_wrong_mdn_checkpoint", /*load_mdn=*/true,
      /*load_rep=*/true, /*use_correct_mdn=*/false,
      /*use_correct_rep=*/true);
  check(wrong_mdn_loaded_eval.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation should fail when it loaded the wrong MDN "
        "checkpoint");

  const auto wrong_rep_loaded_eval = evaluate_validation_fixture(
      "validation_eval_wrong_rep_checkpoint", /*load_mdn=*/true,
      /*load_rep=*/true, /*use_correct_mdn=*/true,
      /*use_correct_rep=*/false);
  check(wrong_rep_loaded_eval.status ==
            target::lattice_target_status_t::exposure_failed,
        "validation evaluation should fail when it loaded the wrong "
        "representation checkpoint");

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
