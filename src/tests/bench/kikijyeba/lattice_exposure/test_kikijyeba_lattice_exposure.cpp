#include "kikijyeba/lattice/exposure/exposure_ledger.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <unistd.h>

namespace exposure = cuwacunu::kikijyeba::lattice::exposure;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  const auto dir = std::filesystem::temp_directory_path() /
                   ("cuwacunu_lattice_exposure_" + label + "_" +
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

void write_representation_job(const std::filesystem::path &dir,
                              std::int64_t begin, std::int64_t end,
                              const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=rep_job\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_rep\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "source_input_length=2\n"
                 "source_future_length=1\n"
                 "observed_source_row_begin=" +
                 std::to_string(begin - 1) +
                 "\n"
                 "observed_source_row_end=" +
                 std::to_string(end) +
                 "\n"
                 "target_source_row_begin=" +
                 std::to_string(begin + 1) +
                 "\n"
                 "target_source_row_end=" +
                 std::to_string(end + 1) +
                 "\n"
                 "source_footprint_precision=graph_anchor_row_index_v1\n"
                 "first_anchor_key=1000\n"
                 "last_anchor_key=1099\n"
                 "observed_source_key_begin=999\n"
                 "observed_source_key_end=1100\n"
                 "target_source_key_begin=1001\n"
                 "target_source_key_end=1101\n"
                 "source_key_footprint_precision=graph_anchor_key_window_v1\n"
                 "source_file_receipts=edge=BTCUSDT|instrument=BTCUSDT|"
                 "interval=1m|record_type=kline|source=/tmp/BTCUSDT.csv\n"
                 "accepted_anchor_count=" +
                 std::to_string(end - begin) +
                 "\n"
                 "source_cursor_token=cursor_rep\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=10\n"
                                "last_loss=0.25\n"
                                "wave_streamed_anchor_count=" +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=5\n"
                                    "wave_first_anchor_key=1000\n"
                                    "wave_last_anchor_key=1099\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint_name + "\n");
  write_text(dir / "representation.report", "optimizer_steps=10\n"
                                            "mean_loss=0.20\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=" +
                                                checkpoint_name + "\n");
  write_text(dir / checkpoint_name, "representation checkpoint");
}

void write_mdn_job(const std::filesystem::path &dir, std::int64_t begin,
                   std::int64_t end,
                   const std::filesystem::path &representation_checkpoint,
                   const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=mdn_job\n"
             "job_kind=inference_mdn\n"
             "target_component=wikimyei.inference.expected_value.mdn\n"
             "wave_id=wave_mdn\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.inference.expected_value.mdn\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=" +
                 std::to_string(begin) +
                 "\n"
                 "resolved_anchor_index_end=" +
                 std::to_string(end) +
                 "\n"
                 "source_input_length=2\n"
                 "source_future_length=1\n"
                 "observed_source_row_begin=" +
                 std::to_string(begin - 1) +
                 "\n"
                 "observed_source_row_end=" +
                 std::to_string(end) +
                 "\n"
                 "target_source_row_begin=" +
                 std::to_string(begin + 1) +
                 "\n"
                 "target_source_row_end=" +
                 std::to_string(end + 1) +
                 "\n"
                 "source_footprint_precision=graph_anchor_row_index_v1\n"
                 "first_anchor_key=1100\n"
                 "last_anchor_key=1199\n"
                 "observed_source_key_begin=1099\n"
                 "observed_source_key_end=1200\n"
                 "target_source_key_begin=1101\n"
                 "target_source_key_end=1201\n"
                 "source_key_footprint_precision=graph_anchor_key_window_v1\n"
                 "source_file_receipts=edge=BTCUSDT|instrument=BTCUSDT|"
                 "interval=1m|record_type=kline|source=/tmp/BTCUSDT.csv\n"
                 "accepted_anchor_count=" +
                 std::to_string(end - begin) +
                 "\n"
                 "source_cursor_token=cursor_mdn\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "mdn_assembly_fingerprint=mdn_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=7\n"
                                "last_loss=0.55\n"
                                "wave_streamed_anchor_count=" +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=3\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint_name + "\n");
  write_text(dir / "inference.report", "optimizer_steps=7\n"
                                       "mean_loss=0.50\n"
                                       "total_valid_target_count=42\n"
                                       "mean_valid_node_target_fraction=0.25\n"
                                       "target_coords=0,1\n"
                                       "node_ids=BTC,ETH,SOL\n"
                                       "routed_row_count_by_node=12,8,10\n"
                                       "active_row_count_by_node=10,0,5\n"
                                       "trained_row_count_by_node=10,0,5\n"
                                       "evaluated_row_count_by_node=0,0,0\n"
                                       "valid_target_count_by_node=10,0,5\n"
                                       "mean_nll_by_node=0.50,nan,0.75\n"
                                       "representation_checkpoint_path=" +
                                           representation_checkpoint.string() +
                                           "\n"
                                           "checkpoint_written=true\n"
                                           "checkpoint_path=" +
                                           checkpoint_name + "\n");
  write_text(dir / checkpoint_name, "mdn checkpoint");
}

void write_unproven_mutation_job(const std::filesystem::path &dir,
                                 const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=unproven_mutation\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_unproven\n"
             "wave_action=train\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=3\n"
                                "checkpoint_written=true\n"
                                "checkpoint_path=" +
                                    checkpoint_name + "\n");
  write_text(dir / "representation.report", "optimizer_steps=3\n"
                                            "mean_loss=0.20\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=" +
                                                checkpoint_name + "\n");
  write_text(dir / checkpoint_name, "checkpoint");
}

void write_anchor_domain_warning_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=domain_warning\n"
             "job_kind=representation_vicreg\n"
             "target_component=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_domain_warning\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=50\n"
             "accepted_anchor_count=50\n"
             "candidate_anchor_count=100\n"
             "skipped_missing_edge_coverage=15\n"
             "skipped_failed_fetch_probe=1\n"
             "duplicate_anchor_count=2\n"
             "common_left_key=1000\n"
             "common_right_key=2000\n"
             "reference_left_key=900\n"
             "reference_right_key=2100\n"
             "source_input_length=1\n"
             "source_future_length=1\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=50\n");
  write_text(dir / "representation.report", "optimizer_steps=1\n"
                                            "mean_loss=0.10\n");
}

} // namespace

int main() {
  const auto root = make_tmp_dir("main");
  const auto rep_dir = root / "rep";
  const auto mdn_dir = root / "mdn";
  const auto unproven_dir = root / "unproven";
  const auto domain_warning_dir = root / "domain_warning";

  write_representation_job(rep_dir, 0, 100, "rep.pt");
  write_mdn_job(mdn_dir, 100, 200, rep_dir / "rep.pt", "mdn.pt");
  write_unproven_mutation_job(unproven_dir, "unproven.pt");

  exposure::exposure_build_context_t train_context{};
  train_context.split_name = "train_core";
  train_context.split_role = exposure::exposure_split_role_t::train;
  train_context.split_policy_fingerprint = "split_policy_1";

  auto rep_fact =
      exposure::make_exposure_fact_from_job_dir(rep_dir, train_context);
  auto mdn_fact =
      exposure::make_exposure_fact_from_job_dir(mdn_dir, train_context);
  auto scan =
      exposure::scan_exposure_ledger_from_runtime_root(root, train_context);
  check(scan.warnings.empty(), "runtime-root exposure scan has no warnings");
  check(scan.ledger.facts().size() == 3,
        "runtime-root exposure scan finds all job exposure facts");
  check(scan.ledger.node_facts().size() == 3,
        "runtime-root exposure scan derives MDN node exposure facts");

  check(rep_fact.target_component == "wikimyei.representation.encoding.vicreg",
        "representation component decoded");
  check(rep_fact.fact_type == "exposure", "exposure fact type");
  check(exposure::canonical_exposure_fact_text(rep_fact).find(
            "fact_type=exposure") != std::string::npos,
        "canonical exposure text includes fact type");
  check(rep_fact.anchor_range.begin == 0 && rep_fact.anchor_range.end == 100,
        "representation anchor range decoded");
  check(rep_fact.graph_order_fingerprint == "graph_1",
        "graph order fingerprint decoded");
  check(rep_fact.job_status == "completed", "job status decoded");
  check(rep_fact.wave_action == "train", "wave action decoded");
  check(rep_fact.observed_footprint.begin == -1 &&
            rep_fact.observed_footprint.end == 100,
        "representation observed source-row footprint decoded");
  check(rep_fact.footprint_precision == "graph_anchor_row_index_v1",
        "source-row footprint precision decoded");
  check(rep_fact.first_anchor_key == "1000" &&
            rep_fact.last_anchor_key == "1099",
        "resolved anchor key bounds decoded");
  check(rep_fact.observed_source_key_begin == "999" &&
            rep_fact.observed_source_key_end == "1100",
        "observed source-key footprint decoded");
  check(rep_fact.target_source_key_begin == "1001" &&
            rep_fact.target_source_key_end == "1101",
        "target source-key footprint decoded");
  check(rep_fact.source_key_footprint_precision == "graph_anchor_key_window_v1",
        "source-key footprint precision decoded");
  check(rep_fact.source_file_receipts.find("edge=BTCUSDT") != std::string::npos,
        "source-file receipts decoded");
  check(rep_fact.candidate_anchor_count == 100 &&
            rep_fact.accepted_anchor_count == 100,
        "older accepted-only manifests infer candidate anchor count");
  check(std::abs(rep_fact.accepted_anchor_fraction - 1.0) < 1e-12,
        "older accepted-only manifests infer accepted anchor fraction");
  check(rep_fact.anchor_domain_warning_level == "none",
        "healthy inferred anchor domain has no warning");
  check(rep_fact.split_policy_fingerprint == "split_policy_1",
        "split policy fingerprint carried from build context");
  check(rep_fact.coverage_precision == "contiguous_completed_range_v1",
        "completed job with full streamed anchors gets completed coverage");
  check(rep_fact.completed_anchor_range.begin == 0 &&
            rep_fact.completed_anchor_range.end == 100,
        "completed coverage range matches requested range");
  check(rep_fact.use.observed_input, "representation observes input");
  check(!rep_fact.use.target_supervision,
        "representation has no supervised future target exposure");
  check(rep_fact.use.mutated_component, "representation mutates component");
  check(rep_fact.output_checkpoint == rep_dir / "rep.pt",
        "relative representation checkpoint resolved against job dir");

  const auto rep_sidecar = exposure::exposure_fact_path_for_job_dir(rep_dir);
  exposure::write_exposure_fact_sidecar(rep_sidecar, rep_fact);
  const auto rep_sidecar_fact =
      exposure::make_exposure_fact_from_sidecar_file(rep_sidecar, rep_dir);
  check(exposure::exposure_fact_digest(rep_sidecar_fact) ==
            exposure::exposure_fact_digest(rep_fact),
        "exposure sidecar round-trips to the same canonical digest");
  check(rep_sidecar_fact.split_policy_fingerprint == "split_policy_1",
        "exposure sidecar round-trips split policy fingerprint");
  check(rep_sidecar_fact.coverage_precision == "contiguous_completed_range_v1",
        "exposure sidecar round-trips coverage precision");

  const auto rep_checkpoint_fact =
      exposure::make_checkpoint_fact_from_exposure_fact(rep_fact);
  const auto rep_checkpoint_sidecar =
      exposure::checkpoint_fact_path_for_job_dir(rep_dir);
  exposure::write_checkpoint_fact_sidecar(rep_checkpoint_sidecar,
                                          rep_checkpoint_fact);
  const auto parsed_checkpoint_fact =
      exposure::make_checkpoint_fact_from_sidecar_file(rep_checkpoint_sidecar,
                                                       rep_dir);
  check(parsed_checkpoint_fact.schema == "kikijyeba.lattice.checkpoint.v1",
        "checkpoint fact schema decoded");
  check(parsed_checkpoint_fact.fact_type == "checkpoint_identity",
        "checkpoint fact type decoded");
  check(parsed_checkpoint_fact.checkpoint_id ==
            rep_checkpoint_fact.checkpoint_id,
        "checkpoint fact id round-trips");
  check(parsed_checkpoint_fact.checkpoint_file_digest ==
            rep_checkpoint_fact.checkpoint_file_digest,
        "checkpoint fact file digest round-trips");
  check(parsed_checkpoint_fact.created_by_exposure_fact_id ==
            exposure::exposure_fact_digest(rep_fact),
        "checkpoint fact links to exposure fact digest");

  const auto single_job_scan =
      exposure::scan_exposure_ledger_from_runtime_root(rep_dir, train_context);
  check(single_job_scan.warnings.empty(),
        "single-job scan accepts exposure/checkpoint sidecars");
  check(single_job_scan.ledger.facts().size() == 1,
        "single-job scan returns exposure sidecar fact");
  check(single_job_scan.ledger.checkpoint_facts().size() == 1,
        "single-job scan ingests checkpoint sidecar fact");

  const auto pref_dir = root / "sidecar_preference";
  write_representation_job(pref_dir, 200, 300, "pref.pt");
  auto pref_fact =
      exposure::make_exposure_fact_from_job_dir(pref_dir, train_context);
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(pref_dir), pref_fact);
  write_text(pref_dir / "representation.report", "optimizer_steps=10\n"
                                                 "mean_loss=999.0\n"
                                                 "checkpoint_written=true\n"
                                                 "checkpoint_path=pref.pt\n");
  const auto pref_scan =
      exposure::scan_exposure_ledger_from_runtime_root(pref_dir, train_context);
  check(pref_scan.ledger.facts().size() == 1,
        "sidecar-preference scan still returns one fact");
  check(!pref_scan.warnings.empty(),
        "sidecar-preference scan warns when derived fact disagrees");
  check(std::abs(pref_scan.ledger.facts().front().mean_loss -
                 pref_fact.mean_loss) < 1e-12,
        "scanner prefers sidecar fact over changed runtime report");

  check(mdn_fact.target_component == "wikimyei.inference.expected_value.mdn",
        "MDN component decoded");
  check(mdn_fact.use.observed_input, "MDN consumes observed context");
  check(mdn_fact.use.target_supervision, "MDN consumes future supervision");
  check(mdn_fact.target_footprint.begin == 101 &&
            mdn_fact.target_footprint.end == 201,
        "MDN target source-row footprint decoded");
  check(mdn_fact.valid_target_count == 42, "MDN valid target count decoded");
  check(std::abs(mdn_fact.valid_target_fraction - 0.25) < 1e-12,
        "MDN valid target fraction decoded");
  check(mdn_fact.input_checkpoints.size() == 1,
        "MDN input representation checkpoint decoded");
  const auto node_facts =
      exposure::make_node_exposure_facts_from_job_dir(mdn_dir, mdn_fact);
  check(node_facts.size() == 3, "MDN report derives one node fact per node");
  check(node_facts[0].node_id == "BTC" && node_facts[0].node_index == 0,
        "first MDN node exposure fact keeps node identity");
  check(node_facts[0].routed_row_count == 12 &&
            node_facts[0].active_row_count == 10 &&
            node_facts[0].trained_row_count == 10 &&
            node_facts[0].evaluated_row_count == 0,
        "first MDN node row support counts decoded");
  check(node_facts[0].valid_target_count == 10,
        "first MDN node valid target count decoded");
  check(std::abs(node_facts[0].valid_target_fraction - 0.50) < 1e-12,
        "first MDN node valid target fraction uses active rows and target "
        "coordinates");
  check(std::abs(node_facts[0].mean_nll - 0.50) < 1e-12,
        "first MDN node mean NLL decoded");
  check(node_facts[1].node_id == "ETH" && node_facts[1].valid_target_count == 0,
        "zero-support MDN node remains visible");
  check(node_facts[1].active_row_count == 0 &&
            node_facts[1].trained_row_count == 0,
        "zero-support MDN node row counts remain visible");
  check(std::isnan(node_facts[1].valid_target_fraction),
        "zero-support MDN node valid target fraction remains unknown");
  check(std::isnan(node_facts[1].mean_nll),
        "NaN MDN node metric remains an unknown metric");
  check(node_facts[2].node_id == "SOL" && node_facts[2].valid_target_count == 5,
        "third MDN node valid target count decoded");
  check(!node_facts[2].parent_exposure_fact_digest.empty(),
        "MDN node fact links to parent exposure fact digest");
  check(node_facts[2].target_component ==
            "wikimyei.inference.expected_value.mdn",
        "MDN node fact keeps component identity");

  auto unproven_fact =
      exposure::make_exposure_fact_from_job_dir(unproven_dir, train_context);
  check(!unproven_fact.use.mutated_component,
        "train wave without mutated_components proof does not mutate");

  write_anchor_domain_warning_job(domain_warning_dir);
  const auto domain_warning_fact = exposure::make_exposure_fact_from_job_dir(
      domain_warning_dir, train_context);
  check(domain_warning_fact.anchor_domain_warning_level == "warning",
        "anchor-domain health warnings are derived from manifest counts");
  check(domain_warning_fact.anchor_domain_warnings.find(
            "accepted_anchor_fraction_below_0.80") != std::string::npos,
        "low accepted anchor fraction warning is present");
  check(domain_warning_fact.anchor_domain_warnings.find(
            "skipped_failed_fetch_probe_nonzero") != std::string::npos,
        "failed fetch probe warning is present");
  check(domain_warning_fact.anchor_domain_warnings.find(
            "skipped_missing_edge_coverage_fraction_above_0.10") !=
            std::string::npos,
        "missing edge coverage warning is present");
  const auto domain_warning_scan =
      exposure::scan_exposure_ledger_from_runtime_root(domain_warning_dir,
                                                       train_context);
  check(!domain_warning_scan.warnings.empty(),
        "anchor-domain warning is surfaced by runtime-root scan");

  exposure::lattice_exposure_ledger_t ledger;
  ledger.add(rep_fact);
  ledger.add(mdn_fact);
  const auto closure = ledger.checkpoint_closure(mdn_dir / "mdn.pt");
  check(closure.size() == 2,
        "MDN checkpoint closure includes MDN and representation exposure");
  check(!ledger.checkpoint_closure_digest(mdn_dir / "mdn.pt").empty(),
        "checkpoint closure digest is present");

  const auto target_supervision_anchor_coverage = exposure::coverage_for_use(
      closure, exposure::anchor_interval_t{.begin = 100, .end = 200},
      exposure::exposure_use_t::target_supervision);
  check(target_supervision_anchor_coverage.covered_anchors == 100,
        "target supervision coverage uses anchor coverage, not future rows");
  check(std::abs(target_supervision_anchor_coverage.coverage_fraction - 1.0) <
            1e-12,
        "target supervision anchor coverage is complete");

  exposure::forbidden_exposure_query_t validation_query{};
  validation_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 50, .end = 60};
  validation_query.forbidden_uses = {
      exposure::exposure_use_t::observed_input,
      exposure::exposure_use_t::target_supervision};
  check(
      exposure::has_forbidden_exposure_overlap(closure, validation_query),
      "closure catches upstream representation overlap with validation range");

  exposure::forbidden_exposure_query_t pre_context_query{};
  pre_context_query.forbidden_range =
      exposure::anchor_interval_t{.begin = -1, .end = 0};
  pre_context_query.forbidden_uses = {exposure::exposure_use_t::observed_input};
  check(exposure::has_forbidden_exposure_overlap(closure, pre_context_query),
        "closure catches observed Hx pre-context outside anchor range");

  exposure::forbidden_exposure_query_t target_query{};
  target_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 150, .end = 160};
  target_query.forbidden_uses = {exposure::exposure_use_t::target_supervision};
  check(exposure::has_forbidden_exposure_overlap(closure, target_query),
        "closure catches MDN target-supervision overlap");

  exposure::forbidden_exposure_query_t future_boundary_query{};
  future_boundary_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 200, .end = 201};
  future_boundary_query.forbidden_uses = {
      exposure::exposure_use_t::target_supervision};
  check(
      exposure::has_forbidden_exposure_overlap(closure, future_boundary_query),
      "closure catches future Hf target footprint beyond anchor range");

  exposure::forbidden_exposure_query_t outside_query{};
  outside_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 250, .end = 260};
  outside_query.forbidden_uses = {exposure::exposure_use_t::observed_input,
                                  exposure::exposure_use_t::target_supervision};
  check(!exposure::has_forbidden_exposure_overlap(closure, outside_query),
        "outside forbidden interval has no overlap");

  auto manual_a = rep_fact;
  manual_a.anchor_range = exposure::anchor_interval_t{.begin = 0, .end = 50};
  manual_a.completed_anchor_range = manual_a.anchor_range;
  manual_a.observed_footprint = manual_a.anchor_range;
  auto manual_b = rep_fact;
  manual_b.anchor_range = exposure::anchor_interval_t{.begin = 50, .end = 100};
  manual_b.completed_anchor_range = manual_b.anchor_range;
  manual_b.observed_footprint = manual_b.anchor_range;
  const std::vector<exposure::lattice_exposure_fact_t> coverage_facts{manual_a,
                                                                      manual_b};
  const auto coverage = exposure::coverage_for_use(
      coverage_facts, exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(coverage.covered_anchors == 100, "union coverage count");
  check(std::abs(coverage.coverage_fraction - 1.0) < 1e-12,
        "union coverage fraction");
  const auto one_pass_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{manual_a, manual_b},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(one_pass_load.unique_covered_anchors == 100,
        "one-pass load unique coverage count");
  check(one_pass_load.loaded_anchor_events == 100,
        "one-pass load counts completed anchor events");
  check(std::abs(one_pass_load.cursor_exposure_load - 1.0) < 1e-12,
        "one-pass load is one cursor epoch");

  auto repeated = rep_fact;
  repeated.anchor_range = exposure::anchor_interval_t{.begin = 0, .end = 100};
  repeated.completed_anchor_range = repeated.anchor_range;
  repeated.observed_footprint = repeated.anchor_range;
  repeated.optimizer_steps = 5;
  const auto repeated_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, repeated},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(repeated_load.unique_covered_anchors == 100,
        "repeated exposure keeps unique coverage at one split");
  check(repeated_load.loaded_anchor_events == 200,
        "repeated exposure counts anchor events with repeats");
  check(std::abs(repeated_load.cursor_exposure_load - 2.0) < 1e-12,
        "repeated exposure load is two cursor epochs");
  check(repeated_load.fact_count == 2,
        "repeated load counts contributing facts");
  check(repeated_load.optimizer_steps_total == 15,
        "repeated load totals optimizer steps from contributing facts");

  auto overlap = rep_fact;
  overlap.anchor_range = exposure::anchor_interval_t{.begin = 50, .end = 150};
  overlap.completed_anchor_range = overlap.anchor_range;
  overlap.observed_footprint = overlap.anchor_range;
  const auto overlap_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, overlap},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(overlap_load.unique_covered_anchors == 100,
        "overlap load keeps unique coverage merged");
  check(overlap_load.loaded_anchor_events == 150,
        "overlap load counts repeated half-split exposure");
  check(std::abs(overlap_load.cursor_exposure_load - 1.5) < 1e-12,
        "overlap load is one and a half cursor epochs");

  auto untrusted = rep_fact;
  untrusted.coverage_precision = "requested_range_untrusted_v0";
  untrusted.completed_anchor_range = {};
  const auto untrusted_coverage = exposure::coverage_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{untrusted},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(untrusted_coverage.covered_anchors == 0,
        "untrusted requested-range coverage is not credited");
  const auto untrusted_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{untrusted},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(untrusted_load.loaded_anchor_events == 0,
        "untrusted requested-range load is not credited");

  auto not_mutated = rep_fact;
  not_mutated.use.mutated_component = false;
  const auto require_mutated_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{not_mutated},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input,
      /*require_mutated_component=*/true);
  check(require_mutated_load.loaded_anchor_events == 0,
        "non-mutated facts do not count when mutation is required");

  auto wrong_component = rep_fact;
  wrong_component.target_component = "other.component";
  const auto component_scoped_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{wrong_component},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input,
      /*require_mutated_component=*/true,
      "wikimyei.representation.encoding.vicreg");
  check(component_scoped_load.loaded_anchor_events == 0,
        "wrong component does not count for target-component scoped load");

  std::cout << "kikijyeba lattice exposure tests passed\n";
  std::filesystem::remove_all(root);
  return 0;
}
