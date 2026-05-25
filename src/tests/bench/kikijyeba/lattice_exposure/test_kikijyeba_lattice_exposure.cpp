#include "kikijyeba/lattice/exposure/exposure_ledger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string remove_lines_with_prefix(const std::string &text,
                                     const std::vector<std::string> &prefixes) {
  std::istringstream lines(text);
  std::ostringstream out;
  std::string line;
  while (std::getline(lines, line)) {
    const bool remove = std::any_of(prefixes.begin(), prefixes.end(),
                                    [&](const std::string &prefix) {
                                      return line.rfind(prefix, /*pos=*/0) == 0;
                                    });
    if (!remove) {
      out << line << "\n";
    }
  }
  return out.str();
}

void write_representation_job(const std::filesystem::path &dir,
                              std::int64_t begin, std::int64_t end,
                              const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=rep_job\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
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
  write_text(dir / "representation.report",
             "optimizer_steps=10\n"
             "mean_loss=0.20\n"
             "representation_architecture="
             "global_fused_temporal_node_"
             "encoder.v1\n"
             "representation_contract="
             "graph_order.node_"
             "representation.v1\n"
             "node_ids=BTC,ETH,SOL\n"
             "primary_value_shape=[B,N,D_e] or "
             "[B,N,Hx,D_e]\n"
             "channel_axis_policy="
             "fused_before_primary_output\n"
             "mean_invariance_loss=0.10\n"
             "mean_variance_loss=0.30\n"
             "mean_covariance_loss=0.05\n"
             "last_grad_norm=1.50\n"
             "max_grad_norm=2.50\n"
             "last_valid_projection_rows=12\n"
             "total_valid_projection_rows=123\n"
             "adapter_original_rows=20\n"
             "adapter_kept_rows=18\n"
             "adapter_dropped_rows=2\n"
             "adapter_valid_channel_time_count=80\n"
             "adapter_kept_valid_channel_time_count=72\n"
             "mean_adapter_valid_channel_time_"
             "fraction=0.75\n"
             "mean_adapter_kept_valid_channel_time_"
             "fraction=0.80\n"
             "node_support_denominator=10,6,4\n"
             "node_valid_lifted_row_count=9,5,2\n"
             "node_valid_lifted_feature_count=100,80,20\n"
             "node_valid_lifted_cell_any_count=30,20,5\n"
             "node_valid_lifted_cell_all_count=25,10,1\n"
             "node_valid_projection_row_count=8,4,1\n"
             "node_valid_lifted_feature_fraction=0.50,0.74,0.25\n"
             "augmented_valid_feature_count=240\n"
             "mean_augmented_valid_feature_"
             "fraction=0.70\n"
             "mean_augmented_feature_retention_"
             "fraction=0.93\n"
             "finite_parameter_check=1\n"
             "representation_embedding_dim=16\n"
             "representation_effective_rank=8\n"
             "representation_effective_rank_"
             "fraction=0.50\n"
             "representation_min_dimension_"
             "variance=0.002\n"
             "representation_max_dimension_"
             "variance=1.25\n"
             "representation_condition_"
             "number=625\n"
             "representation_isotropy_"
             "score=0.42\n"
             "checkpoint_written=true\n"
             "checkpoint_path=" +
                 checkpoint_name + "\n");
  write_text(dir / "representation.report.nodelift.lls",
             "schema:str = wikimyei.expression.nodelift.srl.runtime.v1\n"
             "component_family_id:str = wikimyei.expression.nodelift.srl\n"
             "anchor_count[0,+inf):uint = 64\n"
             "node_count[0,+inf):uint = 3\n"
             "edge_count[0,+inf):uint = 3\n"
             "channel_count[0,+inf):uint = 3\n"
             "input_length[0,+inf):uint = 2\n"
             "future_length[0,+inf):uint = 1\n"
             "feature_width[0,+inf):uint = 9\n"
             "lift_future:bool = true\n"
             "future_lift_emitted:bool = true\n"
             "observed_node_mask_fraction[0,1]:double = 0.40\n"
             "observed_residual_energy_mean[0,+inf):double = 0.001\n"
             "observed_valid_edge_count_mean[0,+inf):double = 2.5\n"
             "future_node_mask_fraction[0,1]:double = 0.90\n"
             "future_residual_energy_mean[0,+inf):double = 0.002\n"
             "future_valid_edge_count_mean[0,+inf):double = 3.0\n");
  write_text(dir / checkpoint_name, "representation checkpoint");
}

void write_legacy_node_mdn_job(
    const std::filesystem::path &dir, std::int64_t begin, std::int64_t end,
    const std::filesystem::path &representation_checkpoint,
    const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=mdn_job\n"
             // Historical node-MDN evidence compatibility.
             "job_kind=inference_mdn\n"
             "target_component_family_id=wikimyei.inference.expected_value.mdn\n"
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
                                       "input_representation_assembly_id="
                                       "node_vicreg_v1\n"
                                       "context_mode=global_node_context\n"
                                       "context_contract=graph_order.node_"
                                       "representation.v1\n"
                                       "context_value_shape=[B_node,D_e]\n"
                                       "output_contract=graph_order.node_"
                                       "expected_value.v1\n"
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

void write_channel_mdn_job(
    const std::filesystem::path &dir, std::int64_t begin, std::int64_t end,
    const std::filesystem::path &representation_checkpoint,
    const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=channel_mdn_job\n"
             "job_kind=channel_inference_mdn\n"
             "target_component_family_id=wikimyei.inference.expected_value.mdn\n"
             "wave_id=wave_channel_mdn\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.inference.expected_value."
             "channel_mdn\n"
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
                 "accepted_anchor_count=" +
                 std::to_string(end - begin) +
                 "\n"
                 "source_cursor_token=cursor_channel_mdn\n"
                 "protocol_contract_fingerprint=contract_1\n"
                 "mdn_assembly_fingerprint=channel_mdn_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=9\n"
                                "last_loss=0.45\n"
                                "wave_streamed_anchor_count=" +
                                    std::to_string(end - begin) +
                                    "\n"
                                    "wave_pulses_completed=4\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=" +
                                    checkpoint_name + "\n");
  write_text(dir / "channel_inference.report",
             "optimizer_steps=9\n"
             "mean_loss=0.40\n"
             "total_valid_target_count=60\n"
             "mean_valid_target_fraction=0.50\n"
             "target_coords=0,1\n"
             "node_ids=BTC,ETH\n"
             "input_representation_assembly_id=vicreg_v1\n"
             "context_mode=channel_context_strict\n"
             "context_contract=graph_order.channel_node_representation.v1\n"
             "context_value_shape=[B,N,C,De]\n"
             "output_contract=graph_order.channel_node_future_distribution.v1\n"
             "output_value_shape=log_pi:[B,N,C,Df,K];mu_sigma:[B,N,C,Df,K]\n"
             "mean_sigma_mean=0.80\n"
             "min_sigma_min=0.10\n"
             "max_sigma_max=1.50\n"
             "mean_mixture_entropy=0.60\n"
             "mean_nll_per_channel=0.40,0.60,0.80\n"
             "mean_nll_per_target_feature=0.50,0.70\n"
             "mean_mixture_usage=0.25,0.75\n"
             "nonfinite_output_count=2\n"
             "last_grad_norm=1.20\n"
             "max_grad_norm=1.80\n"
             "finite_parameter_check=1\n"
             "representation_checkpoint_path=" +
                 representation_checkpoint.string() +
                 "\n"
                 "checkpoint_written=true\n"
                 "checkpoint_path=" +
                 checkpoint_name + "\n");
  write_text(dir / checkpoint_name, "channel mdn checkpoint");
}

void write_unproven_mutation_job(const std::filesystem::path &dir,
                                 const std::string &checkpoint_name) {
  write_text(dir / "job.manifest",
             "job_id=unproven_mutation\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
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
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
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

void write_bad_source_key_window_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=bad_source_key_window\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_bad_source_key_window\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "source_input_length=2\n"
             "source_future_length=1\n"
             "observed_source_row_begin=-1\n"
             "observed_source_row_end=10\n"
             "target_source_row_begin=1\n"
             "target_source_row_end=11\n"
             "source_footprint_precision=graph_anchor_row_index_v1\n"
             "first_anchor_key=1000\n"
             "last_anchor_key=900\n"
             "observed_source_key_begin=1100\n"
             "observed_source_key_end=1000\n"
             "target_source_key_begin=1001\n"
             "target_source_key_end=1101\n"
             "source_key_footprint_precision=graph_anchor_key_window_v1\n"
             "accepted_anchor_count=10\n"
             "source_cursor_token=cursor_bad_key\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=10\n"
                                "wave_pulses_completed=1\n"
                                "checkpoint_written=true\n"
                                "checkpoint_path=bad_key.pt\n");
  write_text(dir / "representation.report", "optimizer_steps=1\n"
                                            "mean_loss=0.10\n"
                                            "checkpoint_written=true\n"
                                            "checkpoint_path=bad_key.pt\n");
  write_text(dir / "bad_key.pt", "bad source key checkpoint");
}

void write_bad_source_key_order_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=bad_source_key_order\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_bad_source_key_order\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "source_input_length=2\n"
             "source_future_length=1\n"
             "observed_source_row_begin=-1\n"
             "observed_source_row_end=10\n"
             "target_source_row_begin=1\n"
             "target_source_row_end=11\n"
             "source_footprint_precision=graph_anchor_row_index_v1\n"
             "first_anchor_key=1000\n"
             "last_anchor_key=1009\n"
             "observed_source_key_begin=999\n"
             "observed_source_key_end=1010\n"
             "target_source_key_begin=900\n"
             "target_source_key_end=1011\n"
             "source_key_footprint_precision=graph_anchor_key_window_v1\n"
             "accepted_anchor_count=10\n"
             "source_cursor_token=cursor_bad_key_order\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=10\n"
                                "wave_pulses_completed=1\n"
                                "checkpoint_written=true\n"
                                "checkpoint_path=bad_key_order.pt\n");
  write_text(dir / "representation.report",
             "optimizer_steps=1\n"
             "mean_loss=0.10\n"
             "checkpoint_written=true\n"
             "checkpoint_path=bad_key_order.pt\n");
  write_text(dir / "bad_key_order.pt", "bad source key order checkpoint");
}

void write_bad_source_key_affine_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=bad_source_key_affine\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_bad_source_key_affine\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "source_input_length=2\n"
             "source_future_length=1\n"
             "observed_source_row_begin=-1\n"
             "observed_source_row_end=10\n"
             "target_source_row_begin=1\n"
             "target_source_row_end=11\n"
             "source_footprint_precision=graph_anchor_row_index_v1\n"
             "first_anchor_key=1000\n"
             "last_anchor_key=1009\n"
             "observed_source_key_begin=998\n"
             "observed_source_key_end=1010\n"
             "target_source_key_begin=1001\n"
             "target_source_key_end=1011\n"
             "source_key_footprint_precision=graph_anchor_key_window_v1\n"
             "accepted_anchor_count=10\n"
             "source_cursor_token=cursor_bad_key_affine\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=10\n"
                                "wave_pulses_completed=1\n"
                                "checkpoint_written=true\n"
                                "checkpoint_path=bad_key_affine.pt\n");
  write_text(dir / "representation.report",
             "optimizer_steps=1\n"
             "mean_loss=0.10\n"
             "checkpoint_written=true\n"
             "checkpoint_path=bad_key_affine.pt\n");
  write_text(dir / "bad_key_affine.pt", "bad source key affine checkpoint");
}

void write_irregular_source_key_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=irregular_source_key\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_irregular_source_key\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "source_input_length=1\n"
             "source_future_length=1\n"
             "observed_source_row_begin=0\n"
             "observed_source_row_end=10\n"
             "target_source_row_begin=0\n"
             "target_source_row_end=10\n"
             "source_footprint_precision=graph_anchor_row_index_v1\n"
             "first_anchor_key=1000\n"
             "last_anchor_key=1010\n"
             "observed_source_key_begin=1000\n"
             "observed_source_key_end=1010\n"
             "target_source_key_begin=1000\n"
             "target_source_key_end=1010\n"
             "source_key_footprint_precision=graph_anchor_key_window_v1\n"
             "source_file_receipts=edge=BTCUSDT|instrument=BTCUSDT|"
             "interval=1d|record_type=ohlcv|source=/tmp/BTCUSDT.csv\n"
             "accepted_anchor_count=10\n"
             "source_cursor_token=cursor_irregular_key\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=10\n"
                                "wave_pulses_completed=1\n"
                                "checkpoint_written=true\n"
                                "checkpoint_path=irregular_key.pt\n");
  write_text(dir / "representation.report",
             "optimizer_steps=1\n"
             "mean_loss=0.10\n"
             "checkpoint_written=true\n"
             "checkpoint_path=irregular_key.pt\n");
  write_text(dir / "irregular_key.pt", "irregular source key checkpoint");
}

void write_malformed_source_receipt_job(const std::filesystem::path &dir) {
  write_text(dir / "job.manifest",
             "job_id=bad_source_receipt\n"
             "job_kind=representation_vicreg\n"
             "target_component_family_id=wikimyei.representation.encoding.vicreg\n"
             "wave_id=wave_bad_source_receipt\n"
             "wave_action=train\n"
             "mutated_components=wikimyei.representation.encoding.vicreg\n"
             "graph_order_fingerprint=graph_1\n"
             "source_range_policy=anchor_index\n"
             "resolved_anchor_index_begin=0\n"
             "resolved_anchor_index_end=10\n"
             "source_input_length=1\n"
             "source_future_length=1\n"
             "source_file_receipts=edge=BTCUSDT|instrument=BTCUSDT|"
             "source=/tmp/BTCUSDT.csv\n"
             "accepted_anchor_count=10\n"
             "source_cursor_token=cursor_bad_receipt\n"
             "protocol_contract_fingerprint=contract_1\n"
             "vicreg_assembly_fingerprint=vicreg_1\n");
  write_text(dir / "job.state", "status=completed\n"
                                "optimizer_steps=1\n"
                                "wave_streamed_anchor_count=10\n");
  write_text(dir / "representation.report", "optimizer_steps=1\n"
                                            "mean_loss=0.10\n");
}

} // namespace

int main() {
  const auto root = make_tmp_dir("main");
  const auto rep_dir = root / "rep";
  const auto mdn_dir = root / "mdn";
  const auto channel_mdn_dir = root / "channel_mdn";
  const auto unproven_dir = root / "unproven";
  const auto domain_warning_dir = root / "domain_warning";
  const auto bad_source_key_dir = root / "bad_source_key";
  const auto bad_source_key_order_dir = root / "bad_source_key_order";
  const auto bad_source_key_affine_dir = root / "bad_source_key_affine";
  const auto irregular_source_key_dir = root / "irregular_source_key";

  write_representation_job(rep_dir, 0, 100, "rep.pt");
  write_legacy_node_mdn_job(mdn_dir, 100, 200, rep_dir / "rep.pt", "mdn.pt");
  write_channel_mdn_job(channel_mdn_dir, 200, 320, rep_dir / "rep.pt",
                        "channel_mdn.pt");
  write_unproven_mutation_job(unproven_dir, "unproven.pt");

  exposure::exposure_build_context_t train_context{};
  train_context.split_name = "train_core";
  train_context.split_role = exposure::exposure_split_role_t::train;
  train_context.split_policy_fingerprint = "split_policy_1";

  auto rep_fact =
      exposure::make_exposure_fact_from_job_dir(rep_dir, train_context);
  auto mdn_fact =
      exposure::make_exposure_fact_from_job_dir(mdn_dir, train_context);
  auto channel_mdn_fact =
      exposure::make_exposure_fact_from_job_dir(channel_mdn_dir, train_context);
  auto scan =
      exposure::scan_exposure_ledger_from_runtime_root(root, train_context);
  check(scan.warnings.empty(), "runtime-root exposure scan has no warnings");
  check(scan.ledger.facts().size() == 4,
        "runtime-root exposure scan finds all job exposure facts");
  check(scan.ledger.node_facts().size() == 3,
        "runtime-root exposure scan derives MDN node exposure facts");
  check(scan.ledger.representation_support_facts().size() == 4,
        "runtime-root exposure scan derives aggregate and node-indexed shared "
        "representation support facts");
  check(scan.ledger.source_receipt_facts().size() == 2,
        "runtime-root exposure scan derives structured source receipt facts");
  const auto scan_representation_support_summary =
      exposure::summarize_representation_support(
          scan.ledger.facts(), scan.ledger.representation_support_facts());
  const double expected_projection_mean = (8.0 + 4.0 + 1.0) / 3.0;
  const double expected_projection_cv =
      std::sqrt(
          ((8.0 - expected_projection_mean) * (8.0 - expected_projection_mean) +
           (4.0 - expected_projection_mean) * (4.0 - expected_projection_mean) +
           (1.0 - expected_projection_mean) *
               (1.0 - expected_projection_mean)) /
          3.0) /
      expected_projection_mean;
  const double expected_projection_gini =
      (2.0 * (1.0 + 2.0 * 4.0 + 3.0 * 8.0)) / (3.0 * 13.0) - (4.0 / 3.0);
  check(
      scan_representation_support_summary.exposure_fact_count == 4 &&
          scan_representation_support_summary
                  .representation_exposure_fact_count == 2 &&
          scan_representation_support_summary
                  .representation_support_fact_count == 4 &&
          scan_representation_support_summary.aggregate_fact_count == 1 &&
          scan_representation_support_summary.node_indexed_fact_count == 3 &&
          scan_representation_support_summary.mdn_backfill_fact_count == 0 &&
          scan_representation_support_summary.nodelift_runtime_fact_count ==
              4 &&
          scan_representation_support_summary.unique_node_count == 3 &&
          scan_representation_support_summary.visibility_only &&
          !scan_representation_support_summary.readiness_authority &&
          !scan_representation_support_summary.hard_gate_authority &&
          !scan_representation_support_summary.mdn_node_support_reused &&
          scan_representation_support_summary.shared_representation_scope &&
          scan_representation_support_summary.total_valid_projection_rows ==
              123 &&
          scan_representation_support_summary.adapter_original_rows_total ==
              20 &&
          scan_representation_support_summary.adapter_kept_rows_total == 18 &&
          scan_representation_support_summary.adapter_dropped_rows_total == 2 &&
          scan_representation_support_summary.node_support_denominator_total ==
              20 &&
          scan_representation_support_summary
                  .node_valid_lifted_feature_count_total == 200 &&
          scan_representation_support_summary
                  .node_valid_projection_rows_total == 13 &&
          scan_representation_support_summary.weakest_node_id == "SOL" &&
          scan_representation_support_summary.weakest_node_index == 2 &&
          scan_representation_support_summary
                  .weakest_node_valid_projection_rows == 1 &&
          scan_representation_support_summary
                  .weakest_node_support_denominator == 4 &&
          std::abs(scan_representation_support_summary
                       .weakest_node_valid_lifted_feature_fraction -
                   0.25) < 1e-12 &&
          std::abs(scan_representation_support_summary
                       .node_valid_projection_rows_mean -
                   expected_projection_mean) < 1e-12 &&
          std::abs(scan_representation_support_summary
                       .node_valid_projection_rows_coefficient_of_variation -
                   expected_projection_cv) < 1e-12 &&
          std::abs(scan_representation_support_summary
                       .node_valid_projection_rows_gini -
                   expected_projection_gini) < 1e-12 &&
          std::abs(scan_representation_support_summary
                       .mean_node_valid_lifted_feature_fraction -
                   ((0.50 + 0.74 + 0.25) / 3.0)) < 1e-12 &&
          std::abs(scan_representation_support_summary
                       .mean_adapter_valid_channel_time_fraction -
                   0.75) < 1e-12 &&
          scan_representation_support_summary.max_nodelift_node_count == 3 &&
          scan_representation_support_summary.issues.empty(),
      "representation support summary stays shared, visibility-only, and "
      "independent from MDN node support while exposing node rows");
  const auto runtime_index_cache =
      exposure::build_runtime_index_cache(root, train_context);
  const auto expected_runtime_index_rows =
      runtime_index_cache.fact_count +
      runtime_index_cache.node_exposure_fact_count +
      runtime_index_cache.checkpoint_fact_count +
      runtime_index_cache.source_receipt_fact_count +
      runtime_index_cache.selection_signal_fact_count +
      runtime_index_cache.representation_support_fact_count;
  check(runtime_index_cache.schema ==
                "kikijyeba.lattice.runtime_index_cache.v1" &&
            runtime_index_cache.source_of_truth ==
                "runtime_files_and_sidecars" &&
            !runtime_index_cache.db_writes_evidence &&
            !runtime_index_cache.runtime_executor &&
            runtime_index_cache.rebuildable_from_runtime_files &&
            runtime_index_cache.fact_count == 4 &&
            runtime_index_cache.node_exposure_fact_count == 3 &&
            runtime_index_cache.source_receipt_fact_count == 2 &&
            runtime_index_cache.representation_support_fact_count == 4 &&
            static_cast<std::int64_t>(runtime_index_cache.rows.size()) ==
                expected_runtime_index_rows &&
            !runtime_index_cache.runtime_metadata_digest.empty() &&
            !runtime_index_cache.watched_file_metadata_digest.empty() &&
            !runtime_index_cache.row_set_digest.empty() &&
            !runtime_index_cache.relation_counts.empty(),
        "runtime index cache is a rebuildable read model over runtime facts, "
        "not evidence or executor authority");
  const auto index_path =
      root / ".lattice_index" / "lattice_runtime_index.v1.lls";
  std::filesystem::create_directories(index_path.parent_path());
  exposure::write_runtime_index_cache(index_path, runtime_index_cache);
  const auto read_runtime_index_cache =
      exposure::read_runtime_index_cache(index_path);
  const auto valid_runtime_index_cache =
      exposure::validate_runtime_index_cache(read_runtime_index_cache, root);
  check(valid_runtime_index_cache.available &&
            valid_runtime_index_cache.schema_matches &&
            valid_runtime_index_cache.metadata_checked &&
            valid_runtime_index_cache.metadata_matches &&
            valid_runtime_index_cache.row_set_checked &&
            valid_runtime_index_cache.row_set_matches &&
            valid_runtime_index_cache.relation_counts_checked &&
            valid_runtime_index_cache.relation_counts_match &&
            valid_runtime_index_cache.cache_valid &&
            valid_runtime_index_cache.issues.empty(),
        "fresh runtime index cache validates against runtime file metadata");
  const auto watched_runtime_index_cache =
      exposure::validate_runtime_index_cache(
          read_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::
              watched_file_manifest);
  check(watched_runtime_index_cache.available &&
            watched_runtime_index_cache.schema_matches &&
            !watched_runtime_index_cache.metadata_checked &&
            watched_runtime_index_cache.watched_metadata_checked &&
            watched_runtime_index_cache.watched_metadata_matches &&
            watched_runtime_index_cache.row_set_checked &&
            watched_runtime_index_cache.row_set_matches &&
            watched_runtime_index_cache.relation_counts_checked &&
            watched_runtime_index_cache.relation_counts_match &&
            watched_runtime_index_cache.cache_valid &&
            watched_runtime_index_cache.issues.empty(),
        "watched-file runtime index validation checks only files that feed "
        "the lattice read model");
  const auto header_only_runtime_index_cache =
      exposure::validate_runtime_index_cache(
          read_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::header_only);
  check(header_only_runtime_index_cache.available &&
            header_only_runtime_index_cache.schema_matches &&
            !header_only_runtime_index_cache.metadata_checked &&
            !header_only_runtime_index_cache.metadata_matches &&
            !header_only_runtime_index_cache.watched_metadata_checked &&
            header_only_runtime_index_cache.row_set_checked &&
            header_only_runtime_index_cache.row_set_matches &&
            header_only_runtime_index_cache.relation_counts_checked &&
            header_only_runtime_index_cache.relation_counts_match &&
            header_only_runtime_index_cache.cache_valid &&
            header_only_runtime_index_cache.issues.empty(),
        "header-only runtime index validation checks cache authority fields "
        "without walking runtime metadata");
  exposure::lattice_runtime_index_query_t support_index_query{};
  support_index_query.relation = "representation_support";
  const auto support_index_query_result = exposure::query_runtime_index_cache(
      read_runtime_index_cache, support_index_query, 64);
  check(
      support_index_query_result.scanned_row_count ==
              static_cast<std::int64_t>(read_runtime_index_cache.rows.size()) &&
          support_index_query_result.matching_row_count == 4 &&
          support_index_query_result.rows.size() == 4,
      "runtime index query filters representation support rows by relation");
  exposure::lattice_runtime_index_query_t eth_support_index_query{};
  eth_support_index_query.relation = "representation_support";
  eth_support_index_query.key_contains = "|ETH|1";
  const auto eth_support_index_query_result =
      exposure::query_runtime_index_cache(read_runtime_index_cache,
                                          eth_support_index_query, 64);
  check(eth_support_index_query_result.matching_row_count == 1 &&
            eth_support_index_query_result.rows.size() == 1,
        "runtime index query filters rows by stable key substring");
  exposure::lattice_runtime_index_query_t exact_key_index_query{};
  exact_key_index_query.key = eth_support_index_query_result.rows.front().key;
  const auto exact_key_index_query_result = exposure::query_runtime_index_cache(
      read_runtime_index_cache, exact_key_index_query, 64);
  check(exact_key_index_query_result.matching_row_count == 1 &&
            exact_key_index_query_result.rows.size() == 1 &&
            exact_key_index_query_result.rows.front().key ==
                exact_key_index_query.key,
        "runtime index query filters rows by exact key");
  exposure::lattice_runtime_index_query_t exact_digest_index_query{};
  exact_digest_index_query.digest =
      eth_support_index_query_result.rows.front().digest;
  const auto exact_digest_index_query_result =
      exposure::query_runtime_index_cache(read_runtime_index_cache,
                                          exact_digest_index_query, 64);
  check(exact_digest_index_query_result.matching_row_count >= 1 &&
            std::all_of(exact_digest_index_query_result.rows.begin(),
                        exact_digest_index_query_result.rows.end(),
                        [&](const exposure::lattice_runtime_index_row_t &row) {
                          return row.digest == exact_digest_index_query.digest;
                        }),
        "runtime index query filters rows by exact digest");
  exposure::lattice_runtime_index_query_t digest_prefix_index_query{};
  digest_prefix_index_query.digest_prefix =
      eth_support_index_query_result.rows.front().digest.substr(0, 8);
  const auto digest_prefix_index_query_result =
      exposure::query_runtime_index_cache(read_runtime_index_cache,
                                          digest_prefix_index_query, 64);
  check(digest_prefix_index_query_result.matching_row_count >= 1 &&
            std::all_of(digest_prefix_index_query_result.rows.begin(),
                        digest_prefix_index_query_result.rows.end(),
                        [&](const exposure::lattice_runtime_index_row_t &row) {
                          return row.digest.rfind(
                                     digest_prefix_index_query.digest_prefix,
                                     /*pos=*/0) == 0;
                        }),
        "runtime index query filters rows by digest prefix");
  const auto live_runtime_index_cache =
      exposure::build_runtime_index_cache(root, train_context);
  const auto runtime_index_parity = exposure::compare_runtime_index_caches(
      read_runtime_index_cache, live_runtime_index_cache, 64);
  check(runtime_index_parity.checked && runtime_index_parity.passed &&
            runtime_index_parity.row_count_matches &&
            runtime_index_parity.relation_counts_match &&
            runtime_index_parity.rows_match &&
            runtime_index_parity.missing_from_index_count == 0 &&
            runtime_index_parity.extra_in_index_count == 0 &&
            runtime_index_parity.issues.empty(),
        "runtime index parity proves valid cache rows match a live scan");
  const auto support_index_query_all = exposure::query_runtime_index_cache(
      read_runtime_index_cache, support_index_query,
      std::numeric_limits<std::size_t>::max());
  const auto support_live_query_all = exposure::query_runtime_index_cache(
      live_runtime_index_cache, support_index_query,
      std::numeric_limits<std::size_t>::max());
  const auto support_query_parity =
      exposure::compare_runtime_index_query_results(support_index_query_all,
                                                    support_live_query_all, 64);
  check(support_query_parity.checked && support_query_parity.passed &&
            support_query_parity.rows_match &&
            support_query_parity.issues.empty(),
        "runtime index query answers match live-scan answers");
  auto tampered_runtime_index_cache = read_runtime_index_cache;
  tampered_runtime_index_cache.rows.pop_back();
  const auto tampered_runtime_index_parity =
      exposure::compare_runtime_index_caches(tampered_runtime_index_cache,
                                             live_runtime_index_cache, 64);
  check(tampered_runtime_index_parity.checked &&
            !tampered_runtime_index_parity.passed &&
            !tampered_runtime_index_parity.row_count_matches &&
            !tampered_runtime_index_parity.rows_match &&
            tampered_runtime_index_parity.missing_from_index_count == 1 &&
            std::find(tampered_runtime_index_parity.issues.begin(),
                      tampered_runtime_index_parity.issues.end(),
                      "row_digest_set_mismatch") !=
                tampered_runtime_index_parity.issues.end(),
        "runtime index parity rejects tampered cache rows");
  const auto tampered_runtime_index_validation =
      exposure::validate_runtime_index_cache(
          tampered_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::header_only);
  check(!tampered_runtime_index_validation.cache_valid &&
            tampered_runtime_index_validation.row_set_checked &&
            !tampered_runtime_index_validation.row_set_matches &&
            std::find(tampered_runtime_index_validation.issues.begin(),
                      tampered_runtime_index_validation.issues.end(),
                      "row_set_digest_mismatch") !=
                tampered_runtime_index_validation.issues.end(),
        "runtime index validation catches tampered cache rows without a live "
        "parity scan");
  write_text(root / "cache_probe.txt", "runtime metadata changed\n");
  const auto stale_runtime_index_cache =
      exposure::validate_runtime_index_cache(read_runtime_index_cache, root);
  check(stale_runtime_index_cache.available &&
            stale_runtime_index_cache.schema_matches &&
            stale_runtime_index_cache.metadata_checked &&
            !stale_runtime_index_cache.metadata_matches &&
            !stale_runtime_index_cache.cache_valid &&
            std::find(stale_runtime_index_cache.issues.begin(),
                      stale_runtime_index_cache.issues.end(),
                      "runtime_metadata_digest_mismatch") !=
                stale_runtime_index_cache.issues.end(),
        "stale runtime index cache fails closed on runtime metadata mismatch");
  const auto non_watched_stale_runtime_index_cache =
      exposure::validate_runtime_index_cache(
          read_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::
              watched_file_manifest);
  check(non_watched_stale_runtime_index_cache.available &&
            non_watched_stale_runtime_index_cache.watched_metadata_checked &&
            non_watched_stale_runtime_index_cache.watched_metadata_matches &&
            non_watched_stale_runtime_index_cache.cache_valid &&
            non_watched_stale_runtime_index_cache.issues.empty(),
        "watched-file runtime index validation ignores files that cannot "
        "change lattice index rows");
  write_text(root / "watched.report", "runtime read-model metadata changed\n");
  const auto watched_stale_runtime_index_cache =
      exposure::validate_runtime_index_cache(
          read_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::
              watched_file_manifest);
  check(watched_stale_runtime_index_cache.available &&
            watched_stale_runtime_index_cache.watched_metadata_checked &&
            !watched_stale_runtime_index_cache.watched_metadata_matches &&
            !watched_stale_runtime_index_cache.cache_valid &&
            std::find(watched_stale_runtime_index_cache.issues.begin(),
                      watched_stale_runtime_index_cache.issues.end(),
                      "watched_file_metadata_digest_mismatch") !=
                watched_stale_runtime_index_cache.issues.end(),
        "watched-file runtime index validation fails closed when an "
        "index-relevant runtime artifact changes");
  const auto stale_header_only_runtime_index_cache =
      exposure::validate_runtime_index_cache(
          read_runtime_index_cache, root,
          exposure::lattice_runtime_index_validation_strength_t::header_only);
  check(stale_header_only_runtime_index_cache.available &&
            stale_header_only_runtime_index_cache.schema_matches &&
            !stale_header_only_runtime_index_cache.metadata_checked &&
            stale_header_only_runtime_index_cache.cache_valid &&
            stale_header_only_runtime_index_cache.issues.empty(),
        "header-only runtime index validation stays an explicit structural "
        "check, not a freshness proof");
  const auto scan_receipt_summary = exposure::summarize_source_receipts(
      scan.ledger.facts(), scan.ledger.source_receipt_facts());
  check(scan_receipt_summary.exposure_fact_count == 4 &&
            scan_receipt_summary.exposure_facts_with_receipts == 2 &&
            scan_receipt_summary.exposure_facts_missing_receipts == 2 &&
            scan_receipt_summary.source_receipt_fact_count == 2 &&
            scan_receipt_summary.malformed_receipt_count == 0 &&
            scan_receipt_summary.unique_edge_count == 1 &&
            scan_receipt_summary.unique_instrument_count == 1 &&
            scan_receipt_summary.unique_source_count == 1 &&
            scan_receipt_summary.audit_only &&
            !scan_receipt_summary.coverage_authority &&
            !scan_receipt_summary.leakage_authority &&
            !scan_receipt_summary.contract_identity_authority &&
            scan_receipt_summary.row_index_authority_preserved &&
            scan_receipt_summary.issues.empty(),
        "source receipt summary preserves audit-only V2 receipt facts");

  check(rep_fact.target_component_family_id == "wikimyei.representation.encoding.vicreg",
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
  const auto rep_source_key_audit = exposure::audit_source_key_window(rep_fact);
  check(rep_source_key_audit.available && rep_source_key_audit.complete &&
            rep_source_key_audit.numeric &&
            rep_source_key_audit.internally_monotone &&
            rep_source_key_audit.order_preserving &&
            rep_source_key_audit.affine_step_available &&
            rep_source_key_audit.reference_key_step == 1 &&
            rep_source_key_audit.affine_consistent &&
            rep_source_key_audit.missing_endpoint_pair_count == 0 &&
            rep_source_key_audit.irregular_key_warning_count == 0 &&
            rep_source_key_audit.source_key_gap_warning_count == 0 &&
            rep_source_key_audit.row_source_key_mismatch_count == 0 &&
            !rep_source_key_audit.source_key_gap_found &&
            !rep_source_key_audit.row_source_key_mismatch_found &&
            rep_source_key_audit.issues.empty() &&
            rep_source_key_audit.endpoints.size() == 6,
        "source-key window audit treats regular key windows as an "
        "order-preserving affine coordinate map");
  check(rep_fact.source_file_receipts.find("edge=BTCUSDT") != std::string::npos,
        "source-file receipts decoded");
  const auto rep_receipts =
      exposure::make_source_receipt_facts_from_exposure_fact(rep_fact);
  check(!rep_receipts.missing && rep_receipts.issues.empty() &&
            rep_receipts.malformed_receipt_count == 0 &&
            rep_receipts.facts.size() == 1,
        "valid compact source receipts parse into structured receipt facts");
  const auto rep_source_key_summary =
      exposure::summarize_source_key_map_audits({rep_fact}, rep_receipts.facts);
  check(
      rep_source_key_summary.schema ==
              "kikijyeba.lattice.source_key_map_audit.summary.v1" &&
          rep_source_key_summary.exposure_fact_count == 1 &&
          rep_source_key_summary.source_receipt_fact_count == 1 &&
          rep_source_key_summary.source_receipt_bound_parent_count == 1 &&
          rep_source_key_summary.available_audit_count == 1 &&
          rep_source_key_summary.complete_count == 1 &&
          rep_source_key_summary.numeric_count == 1 &&
          rep_source_key_summary.internally_monotone_count == 1 &&
          rep_source_key_summary.order_preserving_count == 1 &&
          rep_source_key_summary.affine_step_available_count == 1 &&
          rep_source_key_summary.affine_consistent_count == 1 &&
          rep_source_key_summary.audits_with_graph_order_count == 1 &&
          rep_source_key_summary.audits_with_source_cursor_count == 1 &&
          rep_source_key_summary.audits_with_source_receipts_count == 1 &&
          rep_source_key_summary.source_key_gap_warning_count == 0 &&
          rep_source_key_summary.issue_count == 0 &&
          rep_source_key_summary.audit_only &&
          !rep_source_key_summary.coverage_authority &&
          !rep_source_key_summary.leakage_authority &&
          rep_source_key_summary.row_index_authority_preserved &&
          rep_source_key_summary.explicit_target_rule_required_for_blocking &&
          rep_source_key_summary.summary_self_check_passed &&
          rep_source_key_summary.summary_issue_count == 0,
      "source-key map audit summary binds audits to graph/source identity and "
      "source receipt facts while preserving row-index authority");
  const auto &rep_receipt = rep_receipts.facts.front();
  check(rep_receipt.parent_exposure_fact_digest ==
                exposure::exposure_fact_digest(rep_fact) &&
            rep_receipt.contract_fingerprint == rep_fact.contract_fingerprint &&
            rep_receipt.graph_order_fingerprint ==
                rep_fact.graph_order_fingerprint &&
            rep_receipt.source_cursor_token == rep_fact.source_cursor_token &&
            rep_receipt.split_policy_fingerprint ==
                rep_fact.split_policy_fingerprint &&
            rep_receipt.split_name == "train_core" &&
            rep_receipt.split_role == exposure::exposure_split_role_t::train &&
            rep_receipt.anchor_range.begin == rep_fact.anchor_range.begin &&
            rep_receipt.completed_anchor_range.end ==
                rep_fact.completed_anchor_range.end &&
            rep_receipt.edge == "BTCUSDT" &&
            rep_receipt.instrument == "BTCUSDT" &&
            rep_receipt.interval == "1m" &&
            rep_receipt.record_type == "kline" &&
            rep_receipt.source == "/tmp/BTCUSDT.csv" &&
            exposure::source_receipt_fact_digest(rep_receipt).size() == 16,
        "structured source receipt facts bind receipt provenance to exposure "
        "identity");
  auto multi_receipt_fact = rep_fact;
  multi_receipt_fact.source_file_receipts =
      "edge=BTCUSDT|instrument=BTCUSDT|interval=1m|record_type=kline|"
      "source=/tmp/BTCUSDT.csv;;edge=ETHUSDT|instrument=ETHUSDT|"
      "interval=1m|record_type=kline|source=/tmp/ETHUSDT.csv";
  const auto multi_receipts =
      exposure::make_source_receipt_facts_from_exposure_fact(
          multi_receipt_fact);
  check(multi_receipts.facts.size() == 2 &&
            multi_receipts.facts[0].receipt_index == 0 &&
            multi_receipts.facts[1].receipt_index == 1 &&
            multi_receipts.facts[1].edge == "ETHUSDT",
        "multiple compact source receipts split into indexed structured facts");
  auto missing_receipt_fact = rep_fact;
  missing_receipt_fact.source_file_receipts.clear();
  const auto missing_receipts =
      exposure::make_source_receipt_facts_from_exposure_fact(
          missing_receipt_fact);
  check(
      missing_receipts.missing && missing_receipts.facts.empty() &&
          missing_receipts.issues.empty(),
      "missing source receipts remain audit-only absence, not a scan warning");
  auto selection_fact = rep_fact;
  selection_fact.use.observed_input = false;
  selection_fact.use.target_supervision = false;
  selection_fact.use.evaluation_metric = false;
  selection_fact.use.selection_signal = true;
  selection_fact.output_checkpoint = rep_dir / "rep.pt";
  const auto selection_signal_facts =
      exposure::make_selection_signal_facts_from_exposure_fact(selection_fact);
  check(
      selection_signal_facts.size() == 1 &&
          selection_signal_facts.front().parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(selection_fact) &&
          selection_signal_facts.front().selector_id == selection_fact.job_id &&
          selection_signal_facts.front().selector_kind ==
              "checkpoint_selection" &&
          selection_signal_facts.front().selected_checkpoint ==
              selection_fact.output_checkpoint &&
          selection_signal_facts.front().selected_checkpoint_source ==
              "output_checkpoint" &&
          selection_signal_facts.front().selection_footprint.begin ==
              selection_fact.anchor_range.begin &&
          selection_signal_facts.front().selection_footprint.end ==
              selection_fact.anchor_range.end &&
          exposure::selection_signal_fact_digest(selection_signal_facts.front())
                  .size() == 16,
      "selection_signal exposure derives first-class selector event facts");
  exposure::lattice_exposure_ledger_t selection_signal_ledger{};
  selection_signal_ledger.add(selection_fact);
  const auto selection_signal_summary = exposure::summarize_selection_signals(
      selection_signal_ledger.facts(),
      selection_signal_ledger.selection_signal_facts());
  check(selection_signal_ledger.selection_signal_facts().size() == 1 &&
            selection_signal_summary.selection_signal_fact_count == 1 &&
            selection_signal_summary.unique_selector_count == 1 &&
            selection_signal_summary.selected_checkpoint_count == 1 &&
            selection_signal_summary.missing_selected_checkpoint_count == 0 &&
            selection_signal_summary.mutating_selector_count == 1 &&
            selection_signal_summary.first_class_event_stream &&
            selection_signal_summary.read_only_lattice_fact &&
            !selection_signal_summary.runtime_executor &&
            !selection_signal_summary.coverage_authority &&
            !selection_signal_summary.contract_identity_authority &&
            selection_signal_summary.leakage_relevant_when_forbidden &&
            selection_signal_summary.issues.empty(),
        "selection_signal summary exposes read-only first-class selector "
        "events without coverage or contract authority");
  auto incomplete_selection_fact = selection_fact;
  incomplete_selection_fact.output_checkpoint.clear();
  incomplete_selection_fact.input_checkpoints.clear();
  exposure::lattice_exposure_ledger_t incomplete_selection_ledger{};
  incomplete_selection_ledger.add(incomplete_selection_fact);
  const auto incomplete_selection_summary =
      exposure::summarize_selection_signals(
          incomplete_selection_ledger.facts(),
          incomplete_selection_ledger.selection_signal_facts());
  check(incomplete_selection_summary.missing_selected_checkpoint_count == 1 &&
            std::any_of(incomplete_selection_summary.issues.begin(),
                        incomplete_selection_summary.issues.end(),
                        [](const auto &issue) {
                          return issue.find("missing_selected_checkpoint") !=
                                 std::string::npos;
                        }),
        "selection_signal events without a selected checkpoint stay visible as "
        "incomplete provenance");
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
  check(
      rep_fact.representation_health_available &&
          std::abs(rep_fact.mean_invariance_loss - 0.10) < 1e-12 &&
          std::abs(rep_fact.mean_variance_loss - 0.30) < 1e-12 &&
          std::abs(rep_fact.mean_covariance_loss - 0.05) < 1e-12 &&
          std::abs(rep_fact.last_grad_norm - 1.50) < 1e-12 &&
          std::abs(rep_fact.max_grad_norm - 2.50) < 1e-12 &&
          rep_fact.total_valid_projection_rows == 123 &&
          std::abs(rep_fact.mean_adapter_valid_channel_time_fraction - 0.75) <
              1e-12 &&
          rep_fact.augmented_valid_feature_count == 240 &&
          std::abs(rep_fact.mean_augmented_valid_feature_fraction - 0.70) <
              1e-12 &&
          std::abs(rep_fact.mean_augmented_feature_retention_fraction - 0.93) <
              1e-12 &&
          rep_fact.finite_parameter_check &&
          rep_fact.representation_embedding_dim == 16 &&
          std::abs(rep_fact.representation_effective_rank - 8.0) < 1e-12 &&
          std::abs(rep_fact.representation_effective_rank_fraction - 0.50) <
              1e-12 &&
          std::abs(rep_fact.representation_min_dimension_variance - 0.002) <
              1e-12 &&
          std::abs(rep_fact.representation_max_dimension_variance - 1.25) <
              1e-12 &&
          std::abs(rep_fact.representation_condition_number - 625.0) < 1e-12 &&
          std::abs(rep_fact.representation_isotropy_score - 0.42) < 1e-12,
      "representation health fields are decoded from VICReg report");
  check(rep_fact.representation_architecture ==
                "global_fused_temporal_node_encoder.v1" &&
            rep_fact.representation_contract ==
                "graph_order.node_representation.v1" &&
            rep_fact.representation_value_shape ==
                "[B,N,D_e] or [B,N,Hx,D_e]" &&
            rep_fact.channel_axis_policy == "fused_before_primary_output",
        "representation contract fields are decoded from VICReg report");
  check(exposure::canonical_exposure_fact_text(rep_fact).find(
            "mean_variance_loss=0.3") != std::string::npos,
        "canonical exposure text includes representation health fields");
  check(exposure::canonical_exposure_fact_text(rep_fact).find(
            "representation_contract=graph_order.node_"
            "representation.v1") != std::string::npos,
        "canonical exposure text includes representation contract fields");
  check(rep_fact.output_checkpoint == rep_dir / "rep.pt",
        "relative representation checkpoint resolved against job dir");

  const auto representation_support_facts =
      exposure::make_representation_support_facts_from_job_dir(rep_dir,
                                                               rep_fact);
  check(representation_support_facts.size() == 4,
        "VICReg representation job derives aggregate plus node-indexed shared "
        "support facts");
  const auto &representation_support_fact =
      representation_support_facts.front();
  check(representation_support_fact.parent_exposure_fact_digest ==
            exposure::exposure_fact_digest(rep_fact),
        "representation support fact binds to parent exposure digest");
  check(representation_support_fact.support_scope ==
                "shared_representation_aggregate" &&
            !representation_support_fact.node_indexed &&
            representation_support_fact.visibility_only &&
            !representation_support_fact.readiness_authority &&
            !representation_support_fact.hard_gate_authority &&
            !representation_support_fact.mdn_node_support_reused,
        "representation support fact is visibility-only shared encoder "
        "evidence");
  check(representation_support_fact.last_valid_projection_rows == 12 &&
            representation_support_fact.total_valid_projection_rows == 123 &&
            representation_support_fact.adapter_original_rows == 20 &&
            representation_support_fact.adapter_kept_rows == 18 &&
            representation_support_fact.adapter_dropped_rows == 2 &&
            representation_support_fact.adapter_valid_channel_time_count ==
                80 &&
            representation_support_fact.adapter_kept_valid_channel_time_count ==
                72 &&
            std::abs(representation_support_fact
                         .mean_adapter_valid_channel_time_fraction -
                     0.75) < 1e-12 &&
            std::abs(representation_support_fact
                         .mean_adapter_kept_valid_channel_time_fraction -
                     0.80) < 1e-12,
        "representation support fact decodes adapter support aggregates");
  check(
      representation_support_fact.nodelift_runtime_available &&
          representation_support_fact.nodelift_anchor_count == 64 &&
          representation_support_fact.nodelift_node_count == 3 &&
          representation_support_fact.nodelift_edge_count == 3 &&
          representation_support_fact.nodelift_channel_count == 3 &&
          representation_support_fact.nodelift_input_length == 2 &&
          representation_support_fact.nodelift_future_length == 1 &&
          representation_support_fact.nodelift_feature_width == 9 &&
          representation_support_fact.nodelift_lift_future &&
          representation_support_fact.nodelift_future_lift_emitted &&
          std::abs(
              representation_support_fact.nodelift_observed_node_mask_fraction -
              0.40) < 1e-12 &&
          std::abs(
              representation_support_fact.nodelift_future_node_mask_fraction -
              0.90) < 1e-12,
      "representation support fact decodes typed NodeLift runtime LLS "
      "aggregates");
  const auto node_fact_it =
      std::find_if(representation_support_facts.begin(),
                   representation_support_facts.end(), [](const auto &fact) {
                     return fact.node_indexed && fact.node_id == "ETH";
                   });
  check(node_fact_it != representation_support_facts.end() &&
            node_fact_it->support_scope == "shared_representation_node" &&
            node_fact_it->node_index == 1 &&
            node_fact_it->node_support_denominator == 6 &&
            node_fact_it->node_valid_lifted_rows == 5 &&
            node_fact_it->node_valid_lifted_feature_count == 80 &&
            node_fact_it->node_valid_lifted_cell_any_count == 20 &&
            node_fact_it->node_valid_lifted_cell_all_count == 10 &&
            node_fact_it->node_valid_projection_rows == 4 &&
            std::abs(node_fact_it->node_valid_lifted_feature_fraction - 0.74) <
                1e-12 &&
            !node_fact_it->readiness_authority &&
            !node_fact_it->hard_gate_authority &&
            !node_fact_it->mdn_node_support_reused,
        "representation support derives honest node-indexed shared encoder "
        "visibility facts without turning them into readiness authority");
  check(
      exposure::representation_support_fact_digest(representation_support_fact)
              .size() == 16,
      "representation support fact has a stable digest");

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
  check(rep_sidecar_fact.representation_health_available &&
            std::abs(rep_sidecar_fact.mean_covariance_loss -
                     rep_fact.mean_covariance_loss) < 1e-12 &&
            rep_sidecar_fact.augmented_valid_feature_count ==
                rep_fact.augmented_valid_feature_count &&
            std::abs(rep_sidecar_fact.mean_augmented_valid_feature_fraction -
                     rep_fact.mean_augmented_valid_feature_fraction) < 1e-12 &&
            std::abs(
                rep_sidecar_fact.mean_augmented_feature_retention_fraction -
                rep_fact.mean_augmented_feature_retention_fraction) < 1e-12 &&
            rep_sidecar_fact.finite_parameter_check &&
            rep_sidecar_fact.representation_embedding_dim == 16 &&
            std::abs(rep_sidecar_fact.representation_effective_rank_fraction -
                     rep_fact.representation_effective_rank_fraction) < 1e-12,
        "exposure sidecar round-trips representation health fields");
  check(rep_sidecar_fact.representation_contract ==
                rep_fact.representation_contract &&
            rep_sidecar_fact.representation_value_shape ==
                rep_fact.representation_value_shape &&
            rep_sidecar_fact.channel_axis_policy ==
                rep_fact.channel_axis_policy,
        "exposure sidecar round-trips representation contract fields");

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
  check(parsed_checkpoint_fact.representation_architecture ==
                rep_fact.representation_architecture &&
            parsed_checkpoint_fact.representation_contract ==
                rep_fact.representation_contract &&
            parsed_checkpoint_fact.representation_value_shape ==
                rep_fact.representation_value_shape &&
            parsed_checkpoint_fact.channel_axis_policy ==
                rep_fact.channel_axis_policy,
        "checkpoint fact round-trips representation contract fields");

  const auto single_job_scan =
      exposure::scan_exposure_ledger_from_runtime_root(rep_dir, train_context);
  check(single_job_scan.warnings.empty(),
        "single-job scan accepts exposure/checkpoint sidecars");
  check(single_job_scan.ledger.facts().size() == 1,
        "single-job scan returns exposure sidecar fact");
  check(single_job_scan.ledger.checkpoint_facts().size() == 1,
        "single-job scan ingests checkpoint sidecar fact");
  check(single_job_scan.ledger.source_receipt_facts().size() == 1,
        "single-job scan derives source receipt facts from sidecar exposure");
  check(single_job_scan.ledger.representation_support_facts().size() == 4,
        "single-job scan derives aggregate and node-indexed representation "
        "support facts from representation reports");

  const auto legacy_health_dir = root / "legacy_health_sidecar";
  write_representation_job(legacy_health_dir, 300, 400, "legacy_health.pt");
  auto legacy_health_fact = exposure::make_exposure_fact_from_job_dir(
      legacy_health_dir, train_context);
  auto legacy_sidecar_fact = legacy_health_fact;
  legacy_sidecar_fact.representation_health_available = false;
  legacy_sidecar_fact.mean_invariance_loss =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.mean_variance_loss =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.mean_covariance_loss =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.last_grad_norm = std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.max_grad_norm = std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.total_valid_projection_rows = 0;
  legacy_sidecar_fact.mean_adapter_valid_channel_time_fraction =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.augmented_valid_feature_count = 0;
  legacy_sidecar_fact.mean_augmented_valid_feature_fraction =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.mean_augmented_feature_retention_fraction =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.finite_parameter_check = false;
  legacy_sidecar_fact.representation_embedding_dim = 0;
  legacy_sidecar_fact.representation_effective_rank =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.representation_effective_rank_fraction =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.representation_min_dimension_variance =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.representation_max_dimension_variance =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.representation_condition_number =
      std::numeric_limits<double>::quiet_NaN();
  legacy_sidecar_fact.representation_isotropy_score =
      std::numeric_limits<double>::quiet_NaN();
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(legacy_health_dir),
      legacy_sidecar_fact);
  exposure::write_checkpoint_fact_sidecar(
      exposure::checkpoint_fact_path_for_job_dir(legacy_health_dir),
      exposure::make_checkpoint_fact_from_exposure_fact(legacy_sidecar_fact));
  const auto legacy_health_scan =
      exposure::scan_exposure_ledger_from_runtime_root(legacy_health_dir,
                                                       train_context);
  check(
      legacy_health_scan.ledger.facts().size() == 1 &&
          legacy_health_scan.ledger.facts()
              .front()
              .representation_health_available &&
          std::abs(
              legacy_health_scan.ledger.facts().front().mean_variance_loss -
              legacy_health_fact.mean_variance_loss) < 1e-12 &&
          std::abs(legacy_health_scan.ledger.facts()
                       .front()
                       .representation_effective_rank_fraction -
                   legacy_health_fact.representation_effective_rank_fraction) <
              1e-12,
      "legacy sidecars are supplemented with representation health from "
      "runtime reports");
  const auto legacy_health_closure =
      legacy_health_scan.ledger.checkpoint_closure_result(
          legacy_health_fact.output_checkpoint);
  check(legacy_health_closure.resolution_authority ==
                "checkpoint_id_file_digest" &&
            !legacy_health_closure.legacy_path_fallback &&
            legacy_health_closure.root_checkpoint_id.size() == 16,
        "legacy sidecar supplementation preserves original sidecar digest as "
        "checkpoint lineage identity");
  check(std::any_of(legacy_health_scan.warnings.begin(),
                    legacy_health_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("legacy sidecar supplemented") !=
                             std::string::npos;
                    }),
        "legacy sidecar supplementation is reported as a scan warning");

  const auto legacy_geometry_dir = root / "legacy_geometry_sidecar";
  write_representation_job(legacy_geometry_dir, 400, 500, "legacy_geometry.pt");
  auto legacy_geometry_fact = exposure::make_exposure_fact_from_job_dir(
      legacy_geometry_dir, train_context);
  auto legacy_geometry_sidecar_fact = legacy_geometry_fact;
  legacy_geometry_sidecar_fact.representation_embedding_dim = 0;
  legacy_geometry_sidecar_fact.representation_effective_rank =
      std::numeric_limits<double>::quiet_NaN();
  legacy_geometry_sidecar_fact.representation_effective_rank_fraction =
      std::numeric_limits<double>::quiet_NaN();
  legacy_geometry_sidecar_fact.representation_min_dimension_variance =
      std::numeric_limits<double>::quiet_NaN();
  legacy_geometry_sidecar_fact.representation_max_dimension_variance =
      std::numeric_limits<double>::quiet_NaN();
  legacy_geometry_sidecar_fact.representation_condition_number =
      std::numeric_limits<double>::quiet_NaN();
  legacy_geometry_sidecar_fact.representation_isotropy_score =
      std::numeric_limits<double>::quiet_NaN();
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(legacy_geometry_dir),
      legacy_geometry_sidecar_fact);
  const auto legacy_geometry_scan =
      exposure::scan_exposure_ledger_from_runtime_root(legacy_geometry_dir,
                                                       train_context);
  check(legacy_geometry_scan.ledger.facts().size() == 1 &&
            legacy_geometry_scan.ledger.facts()
                .front()
                .representation_health_available &&
            legacy_geometry_scan.ledger.facts()
                    .front()
                    .representation_embedding_dim ==
                legacy_geometry_fact.representation_embedding_dim &&
            std::abs(legacy_geometry_scan.ledger.facts()
                         .front()
                         .representation_condition_number -
                     legacy_geometry_fact.representation_condition_number) <
                1e-12,
        "legacy sidecars with basic health preserve health and supplement "
        "missing representation geometry");

  const auto partial_rep_health_dir = root / "partial_rep_health_sidecar";
  write_representation_job(partial_rep_health_dir, 500, 600,
                           "partial_rep_health.pt");
  const auto partial_rep_health_fact =
      exposure::make_exposure_fact_from_job_dir(partial_rep_health_dir,
                                                train_context);
  const auto partial_rep_health_text = remove_lines_with_prefix(
      exposure::canonical_exposure_fact_text(partial_rep_health_fact),
      {"finite_parameter_check=", "total_valid_projection_rows=",
       "mean_adapter_valid_channel_time_fraction=",
       "augmented_valid_feature_count=",
       "mean_augmented_valid_feature_fraction=",
       "mean_augmented_feature_retention_fraction="});
  write_text(exposure::exposure_fact_path_for_job_dir(partial_rep_health_dir),
             partial_rep_health_text);
  const auto partial_rep_health_scan =
      exposure::scan_exposure_ledger_from_runtime_root(partial_rep_health_dir,
                                                       train_context);
  check(
      partial_rep_health_scan.ledger.facts().size() == 1 &&
          partial_rep_health_scan.ledger.facts()
              .front()
              .representation_health_available &&
          partial_rep_health_scan.ledger.facts()
                  .front()
                  .total_valid_projection_rows ==
              partial_rep_health_fact.total_valid_projection_rows &&
          std::abs(partial_rep_health_scan.ledger.facts()
                       .front()
                       .mean_adapter_valid_channel_time_fraction -
                   partial_rep_health_fact
                       .mean_adapter_valid_channel_time_fraction) < 1e-12 &&
          partial_rep_health_scan.ledger.facts()
                  .front()
                  .augmented_valid_feature_count ==
              partial_rep_health_fact.augmented_valid_feature_count &&
          std::abs(
              partial_rep_health_scan.ledger.facts()
                  .front()
                  .mean_augmented_valid_feature_fraction -
              partial_rep_health_fact.mean_augmented_valid_feature_fraction) <
              1e-12 &&
          std::abs(partial_rep_health_scan.ledger.facts()
                       .front()
                       .mean_augmented_feature_retention_fraction -
                   partial_rep_health_fact
                       .mean_augmented_feature_retention_fraction) < 1e-12 &&
          partial_rep_health_scan.ledger.facts().front().finite_parameter_check,
      "partial legacy representation sidecars supplement missing readiness "
      "health fields from runtime reports");

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

  check(mdn_fact.target_component_family_id == "wikimyei.inference.expected_value.mdn",
        "MDN component decoded");
  check(mdn_fact.use.observed_input, "MDN consumes observed context");
  check(mdn_fact.use.target_supervision, "MDN consumes future supervision");
  check(mdn_fact.target_footprint.begin == 101 &&
            mdn_fact.target_footprint.end == 201,
        "MDN target source-row footprint decoded");
  check(mdn_fact.valid_target_count == 42, "MDN valid target count decoded");
  check(std::abs(mdn_fact.valid_target_fraction - 0.25) < 1e-12,
        "MDN valid target fraction decoded");
  check(mdn_fact.context_mode == "global_node_context" &&
            mdn_fact.context_contract == "graph_order.node_representation.v1" &&
            mdn_fact.context_value_shape == "[B_node,D_e]" &&
            mdn_fact.input_representation_assembly_id == "node_vicreg_v1",
        "old fused MDN context contract fields decoded");
  check(channel_mdn_fact.target_component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "channel MDN component decoded");
  check(channel_mdn_fact.component_assembly_fingerprint == "channel_mdn_1",
        "channel MDN assembly fingerprint decoded");
  check(channel_mdn_fact.use.target_supervision,
        "channel MDN consumes future supervision");
  check(channel_mdn_fact.valid_target_count == 60,
        "channel MDN valid target count decoded");
  check(std::abs(channel_mdn_fact.valid_target_fraction - 0.50) < 1e-12,
        "channel MDN valid target fraction decoded");
  check(channel_mdn_fact.inference_health_available &&
            std::abs(channel_mdn_fact.mean_sigma_mean - 0.80) < 1e-12 &&
            std::abs(channel_mdn_fact.min_sigma_min - 0.10) < 1e-12 &&
            std::abs(channel_mdn_fact.max_sigma_max - 1.50) < 1e-12 &&
            std::abs(channel_mdn_fact.mean_mixture_entropy - 0.60) < 1e-12 &&
            channel_mdn_fact.mean_nll_per_channel.size() == 3 &&
            std::abs(channel_mdn_fact.mean_nll_per_channel[2] - 0.80) < 1e-12 &&
            channel_mdn_fact.mean_nll_per_target_feature.size() == 2 &&
            std::abs(channel_mdn_fact.mean_nll_per_target_feature[1] - 0.70) < 1e-12 &&
            channel_mdn_fact.mean_mixture_usage.size() == 2 &&
            std::abs(channel_mdn_fact.mean_mixture_usage[0] - 0.25) < 1e-12 &&
            channel_mdn_fact.nonfinite_output_count == 2 &&
            std::abs(channel_mdn_fact.last_grad_norm - 1.20) < 1e-12 &&
            std::abs(channel_mdn_fact.max_grad_norm - 1.80) < 1e-12 &&
            channel_mdn_fact.finite_parameter_check,
        "channel MDN health fields are decoded from strict-channel report");
  check(channel_mdn_fact.input_representation_assembly_id == "vicreg_v1" &&
            channel_mdn_fact.context_mode == "channel_context_strict" &&
            channel_mdn_fact.context_contract ==
                "graph_order.channel_node_representation.v1" &&
            channel_mdn_fact.context_value_shape == "[B,N,C,De]" &&
            channel_mdn_fact.output_contract ==
                "graph_order.channel_node_future_distribution.v1",
        "channel MDN contract fields are decoded from strict-channel report");
  check(exposure::canonical_exposure_fact_text(channel_mdn_fact)
                .find("mean_nll_per_channel=0.4,0.6,0.8") != std::string::npos,
        "canonical exposure text includes channel MDN health vectors");
  check(
      exposure::canonical_exposure_fact_text(channel_mdn_fact)
              .find("context_contract=graph_order.channel_node_representation."
                    "v1") != std::string::npos,
      "canonical exposure text includes channel MDN context contract");
  const auto channel_mdn_sidecar =
      exposure::exposure_fact_path_for_job_dir(channel_mdn_dir);
  exposure::write_exposure_fact_sidecar(channel_mdn_sidecar, channel_mdn_fact);
  const auto channel_mdn_sidecar_fact =
      exposure::make_exposure_fact_from_sidecar_file(channel_mdn_sidecar,
                                                     channel_mdn_dir);
  check(exposure::exposure_fact_digest(channel_mdn_sidecar_fact) ==
            exposure::exposure_fact_digest(channel_mdn_fact),
        "channel MDN exposure sidecar round-trips to the same digest");
  check(channel_mdn_sidecar_fact.inference_health_available &&
            channel_mdn_sidecar_fact.mean_mixture_usage.size() == 2 &&
            std::abs(channel_mdn_sidecar_fact.mean_mixture_usage[1] - 0.75) <
                1e-12,
        "channel MDN sidecar round-trips health vectors");
  check(channel_mdn_sidecar_fact.context_mode ==
                channel_mdn_fact.context_mode &&
            channel_mdn_sidecar_fact.context_contract ==
                channel_mdn_fact.context_contract &&
            channel_mdn_sidecar_fact.output_contract ==
                channel_mdn_fact.output_contract,
        "channel MDN sidecar round-trips context contract fields");
  check(channel_mdn_fact.input_checkpoints.size() == 1,
        "channel MDN input representation checkpoint decoded");

  const auto legacy_channel_mdn_health_dir =
      root / "legacy_channel_mdn_health_sidecar";
  write_channel_mdn_job(legacy_channel_mdn_health_dir, 500, 560,
                        root / "legacy_channel_rep.pt",
                        "legacy_channel_mdn.pt");
  const auto legacy_channel_mdn_fact =
      exposure::make_exposure_fact_from_job_dir(legacy_channel_mdn_health_dir,
                                                train_context);
  auto legacy_channel_mdn_sidecar_fact = legacy_channel_mdn_fact;
  legacy_channel_mdn_sidecar_fact.inference_health_available = false;
  legacy_channel_mdn_sidecar_fact.mean_sigma_mean =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.min_sigma_min =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.max_sigma_max =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.mean_mixture_entropy =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.mean_nll_per_channel.clear();
  legacy_channel_mdn_sidecar_fact.mean_nll_per_target_feature.clear();
  legacy_channel_mdn_sidecar_fact.mean_mixture_usage.clear();
  legacy_channel_mdn_sidecar_fact.nonfinite_output_count = 0;
  legacy_channel_mdn_sidecar_fact.last_grad_norm =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.max_grad_norm =
      std::numeric_limits<double>::quiet_NaN();
  legacy_channel_mdn_sidecar_fact.finite_parameter_check = false;
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(legacy_channel_mdn_health_dir),
      legacy_channel_mdn_sidecar_fact);
  const auto legacy_channel_mdn_health_scan =
      exposure::scan_exposure_ledger_from_runtime_root(
          legacy_channel_mdn_health_dir, train_context);
  check(legacy_channel_mdn_health_scan.ledger.facts().size() == 1 &&
            legacy_channel_mdn_health_scan.ledger.facts()
                .front()
                .inference_health_available &&
            std::abs(legacy_channel_mdn_health_scan.ledger.facts()
                         .front()
                         .mean_sigma_mean -
                     legacy_channel_mdn_fact.mean_sigma_mean) < 1e-12 &&
            legacy_channel_mdn_health_scan.ledger.facts()
                    .front()
                    .mean_mixture_usage.size() ==
                legacy_channel_mdn_fact.mean_mixture_usage.size() &&
            legacy_channel_mdn_health_scan.ledger.facts()
                    .front()
                    .nonfinite_output_count ==
                legacy_channel_mdn_fact.nonfinite_output_count &&
            legacy_channel_mdn_health_scan.ledger.facts()
                .front()
                .finite_parameter_check,
        "legacy channel MDN sidecars are supplemented with inference health "
        "from runtime reports");
  check(std::any_of(legacy_channel_mdn_health_scan.warnings.begin(),
                    legacy_channel_mdn_health_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("legacy sidecar supplemented with "
                                          "inference health") !=
                             std::string::npos;
                    }),
        "legacy channel MDN sidecar supplementation is reported as a scan "
        "warning");

  const auto partial_channel_mdn_health_dir =
      root / "partial_channel_mdn_health_sidecar";
  write_channel_mdn_job(partial_channel_mdn_health_dir, 560, 620,
                        root / "partial_channel_rep.pt",
                        "partial_channel_mdn.pt");
  const auto partial_channel_mdn_fact =
      exposure::make_exposure_fact_from_job_dir(partial_channel_mdn_health_dir,
                                                train_context);
  const auto partial_channel_mdn_text = remove_lines_with_prefix(
      exposure::canonical_exposure_fact_text(partial_channel_mdn_fact),
      {"finite_parameter_check=", "nonfinite_output_count=",
       "mean_mixture_usage="});
  write_text(
      exposure::exposure_fact_path_for_job_dir(partial_channel_mdn_health_dir),
      partial_channel_mdn_text);
  const auto partial_channel_mdn_health_scan =
      exposure::scan_exposure_ledger_from_runtime_root(
          partial_channel_mdn_health_dir, train_context);
  check(partial_channel_mdn_health_scan.ledger.facts().size() == 1 &&
            partial_channel_mdn_health_scan.ledger.facts()
                .front()
                .inference_health_available &&
            partial_channel_mdn_health_scan.ledger.facts()
                    .front()
                    .mean_mixture_usage.size() ==
                partial_channel_mdn_fact.mean_mixture_usage.size() &&
            partial_channel_mdn_health_scan.ledger.facts()
                    .front()
                    .nonfinite_output_count ==
                partial_channel_mdn_fact.nonfinite_output_count &&
            partial_channel_mdn_health_scan.ledger.facts()
                .front()
                .finite_parameter_check,
        "partial legacy channel MDN sidecars supplement missing readiness "
        "health fields from runtime reports");

  const auto channel_mdn_checkpoint_fact =
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact);
  const auto channel_mdn_checkpoint_text =
      exposure::canonical_checkpoint_fact_text(channel_mdn_checkpoint_fact);
  check(channel_mdn_checkpoint_fact.input_representation_assembly_id == "vicreg_v1" &&
            channel_mdn_checkpoint_fact.context_mode ==
                "channel_context_strict" &&
            channel_mdn_checkpoint_fact.context_contract ==
                "graph_order.channel_node_representation.v1" &&
            channel_mdn_checkpoint_fact.output_contract ==
                "graph_order.channel_node_future_distribution.v1",
        "channel MDN checkpoint fact carries strict-channel contract fields");
  check(channel_mdn_checkpoint_text.find(
            "context_contract=graph_order.channel_node_representation.v1") !=
            std::string::npos,
        "canonical checkpoint fact text includes channel MDN context contract");
  exposure::write_checkpoint_fact_sidecar(
      exposure::checkpoint_fact_path_for_job_dir(channel_mdn_dir),
      channel_mdn_checkpoint_fact);
  const auto channel_mdn_checkpoint_sidecar_fact =
      exposure::make_checkpoint_fact_from_sidecar_file(
          exposure::checkpoint_fact_path_for_job_dir(channel_mdn_dir),
          channel_mdn_dir);
  check(channel_mdn_checkpoint_sidecar_fact.input_representation_assembly_id ==
                channel_mdn_checkpoint_fact.input_representation_assembly_id &&
            channel_mdn_checkpoint_sidecar_fact.context_contract ==
                channel_mdn_checkpoint_fact.context_contract &&
            channel_mdn_checkpoint_sidecar_fact.output_contract ==
                channel_mdn_checkpoint_fact.output_contract,
        "channel MDN checkpoint sidecar round-trips strict-channel contract "
        "fields");
  const auto mdn_target_support_summary = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{mdn_fact},
      exposure::anchor_interval_t{.begin = 100, .end = 200},
      exposure::exposure_use_t::target_supervision);
  const auto [mdn_support_lower, mdn_support_upper] =
      exposure::wilson_score_interval_95(42, 168);
  check(mdn_target_support_summary.valid_target_success_count_for_uncertainty ==
                42 &&
            mdn_target_support_summary
                    .valid_target_opportunity_count_for_uncertainty == 168,
        "MDN exposure summary derives valid-target support counts for "
        "uncertainty");
  check(std::abs(mdn_target_support_summary.valid_target_fraction_estimate -
                 0.25) < 1e-12 &&
            std::abs(mdn_target_support_summary.valid_target_wilson_lower_95 -
                     mdn_support_lower) < 1e-12 &&
            std::abs(mdn_target_support_summary.valid_target_wilson_upper_95 -
                     mdn_support_upper) < 1e-12,
        "MDN exposure summary reports valid-target Wilson interval");
  auto outside_mdn_support_fact = mdn_fact;
  outside_mdn_support_fact.job_id = "outside_mdn_support";
  outside_mdn_support_fact.anchor_range =
      exposure::anchor_interval_t{.begin = 200, .end = 300};
  outside_mdn_support_fact.completed_anchor_range =
      outside_mdn_support_fact.anchor_range;
  outside_mdn_support_fact.target_footprint =
      exposure::anchor_interval_t{.begin = 201, .end = 301};
  outside_mdn_support_fact.valid_target_count = 100;
  outside_mdn_support_fact.valid_target_fraction = 1.0;
  outside_mdn_support_fact.optimizer_steps = 99;
  const auto mdn_target_support_with_outside = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{mdn_fact,
                                                     outside_mdn_support_fact},
      exposure::anchor_interval_t{.begin = 100, .end = 200},
      exposure::exposure_use_t::target_supervision);
  check(
      mdn_target_support_with_outside.fact_count == 1 &&
          mdn_target_support_with_outside.loaded_anchor_events == 100 &&
          mdn_target_support_with_outside.optimizer_steps_total ==
              mdn_fact.optimizer_steps &&
          mdn_target_support_with_outside.valid_target_count_total == 42 &&
          mdn_target_support_with_outside
                  .valid_target_success_count_for_uncertainty == 42 &&
          mdn_target_support_with_outside
                  .valid_target_opportunity_count_for_uncertainty == 168 &&
          std::abs(mdn_target_support_with_outside.valid_target_cursor_epochs -
                   0.25) < 1e-12 &&
          std::abs(
              mdn_target_support_with_outside.valid_target_wilson_lower_95 -
              mdn_support_lower) < 1e-12 &&
          std::abs(
              mdn_target_support_with_outside.valid_target_wilson_upper_95 -
              mdn_support_upper) < 1e-12,
      "non-overlapping MDN support facts do not inflate support uncertainty "
      "or load");
  auto half_overlap_mdn_support_fact = mdn_fact;
  half_overlap_mdn_support_fact.job_id = "half_overlap_mdn_support";
  half_overlap_mdn_support_fact.anchor_range =
      exposure::anchor_interval_t{.begin = 150, .end = 250};
  half_overlap_mdn_support_fact.completed_anchor_range =
      half_overlap_mdn_support_fact.anchor_range;
  half_overlap_mdn_support_fact.target_footprint =
      exposure::anchor_interval_t{.begin = 151, .end = 251};
  half_overlap_mdn_support_fact.valid_target_count = 40;
  half_overlap_mdn_support_fact.valid_target_fraction = 0.5;
  const auto half_overlap_support_summary = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{
          half_overlap_mdn_support_fact},
      exposure::anchor_interval_t{.begin = 100, .end = 200},
      exposure::exposure_use_t::target_supervision);
  const auto [half_overlap_lower, half_overlap_upper] =
      exposure::wilson_score_interval_95(20, 40);
  check(
      half_overlap_support_summary.fact_count == 1 &&
          half_overlap_support_summary.loaded_anchor_events == 50 &&
          std::abs(half_overlap_support_summary.cursor_exposure_load - 0.5) <
              1e-12 &&
          half_overlap_support_summary.valid_target_count_total == 20 &&
          half_overlap_support_summary
                  .valid_target_success_count_for_uncertainty == 20 &&
          half_overlap_support_summary
                  .valid_target_opportunity_count_for_uncertainty == 40 &&
          std::abs(half_overlap_support_summary.valid_target_cursor_epochs -
                   0.25) < 1e-12 &&
          std::abs(half_overlap_support_summary.valid_target_fraction_estimate -
                   0.5) < 1e-12 &&
          std::abs(half_overlap_support_summary.valid_target_wilson_lower_95 -
                   half_overlap_lower) < 1e-12 &&
          std::abs(half_overlap_support_summary.valid_target_wilson_upper_95 -
                   half_overlap_upper) < 1e-12,
      "partially overlapping MDN support facts range-scope support counts, "
      "cursor epochs, and Wilson uncertainty");
  check(mdn_fact.input_checkpoints.size() == 1,
        "MDN input representation checkpoint decoded");
  const auto node_facts =
      exposure::make_node_exposure_facts_from_job_dir(mdn_dir, mdn_fact);
  check(node_facts.size() == 3, "MDN report derives one node fact per node");
  check(exposure::make_representation_support_facts_from_job_dir(mdn_dir,
                                                                 mdn_fact)
            .empty(),
        "MDN node support is not backfilled into representation support "
        "facts");
  check(node_facts[0].node_id == "BTC" && node_facts[0].node_index == 0,
        "first MDN node exposure fact keeps node identity");
  check(node_facts[0].routed_row_count == 12 &&
            node_facts[0].active_row_count == 10 &&
            node_facts[0].trained_row_count == 10 &&
            node_facts[0].evaluated_row_count == 0,
        "first MDN node row support counts decoded");
  check(node_facts[0].valid_target_count == 10,
        "first MDN node valid target count decoded");
  check(node_facts[0].valid_target_opportunity_count == 20,
        "first MDN node valid target opportunity count decoded");
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
  check(node_facts[1].valid_target_opportunity_count == 0,
        "inactive zero-support MDN node does not claim target opportunities");
  check(std::isnan(node_facts[1].valid_target_fraction),
        "zero-support MDN node valid target fraction remains unknown");
  check(std::isnan(node_facts[1].mean_nll),
        "NaN MDN node metric remains an unknown metric");
  check(node_facts[2].node_id == "SOL" && node_facts[2].valid_target_count == 5,
        "third MDN node valid target count decoded");
  check(!node_facts[2].parent_exposure_fact_digest.empty(),
        "MDN node fact links to parent exposure fact digest");
  check(node_facts[2].target_component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "MDN node fact keeps component identity");
  const auto node_summary = exposure::summarize_node_support(
      node_facts, "wikimyei.inference.expected_value.mdn");
  check(node_summary.node_count == 3 && node_summary.unique_node_count == 3,
        "node summary counts MDN support rows and unique nodes");
  check(node_summary.observed_input_support_row_count == 3 &&
            node_summary.target_supervision_support_row_count == 3 &&
            node_summary.mutating_support_row_count == 3 &&
            node_summary.non_mutating_support_row_count == 0,
        "node summary exposes support-row incidence by use and mutation");
  auto mixed_support_rows = node_summary.support_rows;
  auto non_mutating_eval_row = mixed_support_rows.front();
  non_mutating_eval_row.parent_exposure_fact_digest = "eval_parent";
  non_mutating_eval_row.use.mutated_component = false;
  non_mutating_eval_row.trained_row_count = 0;
  non_mutating_eval_row.evaluated_row_count = 10;
  non_mutating_eval_row.valid_target_count = 10;
  non_mutating_eval_row.valid_target_denominator = 20;
  non_mutating_eval_row.valid_target_fraction = 0.50;
  mixed_support_rows.push_back(non_mutating_eval_row);
  const auto mixed_node_summary = exposure::summarize_node_support_rows(
      mixed_support_rows, "wikimyei.inference.expected_value.mdn",
      "train_core");
  const auto mutating_target_support = exposure::filter_node_support_summary(
      mixed_node_summary, exposure::exposure_use_t::target_supervision,
      /*require_mutated_component=*/true);
  check(mixed_node_summary.node_count == 4 &&
            mixed_node_summary.non_mutating_support_row_count == 1 &&
            mutating_target_support.node_count == 3 &&
            mutating_target_support.non_mutating_support_row_count == 0 &&
            mutating_target_support.valid_target_count_total == 15,
        "node summary filtering isolates mutating target-supervision support "
        "from non-mutating evaluation rows");
  check(node_summary.routed_row_count_total == 30 &&
            node_summary.active_row_count_total == 15 &&
            node_summary.trained_row_count_total == 15 &&
            node_summary.valid_target_count_total == 15,
        "node summary totals row and target support");
  check(node_summary.weakest_valid_target_node_id == "ETH" &&
            node_summary.weakest_valid_target_count == 0,
        "node summary identifies weakest valid-target node");
  const auto [expected_node_support_lower, expected_node_support_upper] =
      exposure::wilson_score_interval_95(15, 30);
  check(node_summary.valid_target_opportunity_count_total == 30 &&
            std::abs(node_summary.valid_target_fraction_estimate - 0.5) <
                1e-12 &&
            std::abs(node_summary.valid_target_wilson_lower_95 -
                     expected_node_support_lower) < 1e-12 &&
            std::abs(node_summary.valid_target_wilson_upper_95 -
                     expected_node_support_upper) < 1e-12,
        "node summary reports aggregate valid-target Wilson support interval");
  check(node_summary.weakest_valid_target_denominator == 0 &&
            std::isnan(node_summary.weakest_valid_target_fraction) &&
            std::isnan(node_summary.weakest_valid_target_wilson_lower_95) &&
            std::isnan(node_summary.weakest_valid_target_wilson_upper_95),
        "node summary does not invent weakest-node Wilson support without "
        "target-opportunity evidence");
  check(std::abs(node_summary.min_valid_target_fraction - 0.50) < 1e-12 &&
            std::abs(node_summary.max_valid_target_fraction - 0.50) < 1e-12 &&
            std::abs(node_summary.mean_valid_target_fraction - 0.50) < 1e-12,
        "node summary reports finite valid-target fraction range");
  check(std::abs(node_summary.valid_target_count_gini - (4.0 / 9.0)) < 1e-12,
        "node summary reports valid-target support gini");
  const double expected_node_entropy = -((10.0 / 15.0) * std::log(10.0 / 15.0) +
                                         (5.0 / 15.0) * std::log(5.0 / 15.0)) /
                                       std::log(3.0);
  check(std::abs(node_summary.valid_target_count_normalized_entropy -
                 expected_node_entropy) < 1e-12,
        "node summary reports normalized valid-target support entropy");
  std::vector<exposure::lattice_node_exposure_fact_t> wide_node_facts;
  for (std::int64_t i = 0; i < 2; ++i) {
    auto node_fact = node_facts.front();
    node_fact.node_id = i == 0 ? "BTC" : "ETH";
    node_fact.node_index = i;
    node_fact.anchor_range =
        exposure::anchor_interval_t{.begin = 100, .end = 300};
    node_fact.completed_anchor_range = node_fact.anchor_range;
    node_fact.active_row_count = i == 0 ? 100 : 20;
    node_fact.trained_row_count = node_fact.active_row_count;
    node_fact.valid_target_count = i == 0 ? 80 : 10;
    node_fact.valid_target_opportunity_count = i == 0 ? 100 : 20;
    node_fact.valid_target_fraction = i == 0 ? 0.80 : 0.50;
    wide_node_facts.push_back(std::move(node_fact));
  }
  auto outside_node_fact = wide_node_facts.front();
  outside_node_fact.node_id = "SOL";
  outside_node_fact.node_index = 2;
  outside_node_fact.anchor_range =
      exposure::anchor_interval_t{.begin = 200, .end = 300};
  outside_node_fact.completed_anchor_range = outside_node_fact.anchor_range;
  outside_node_fact.active_row_count = 1000;
  outside_node_fact.trained_row_count = 1000;
  outside_node_fact.valid_target_count = 1000;
  outside_node_fact.valid_target_opportunity_count = 1000;
  wide_node_facts.push_back(std::move(outside_node_fact));
  const auto ranged_node_summary = exposure::summarize_node_support_for_range(
      wide_node_facts, exposure::anchor_interval_t{.begin = 100, .end = 200},
      "wikimyei.inference.expected_value.mdn");
  const auto [ranged_node_lower, ranged_node_upper] =
      exposure::wilson_score_interval_95(45, 60);
  check(ranged_node_summary.node_count == 2 &&
            ranged_node_summary.unique_node_count == 2 &&
            ranged_node_summary.active_row_count_total == 60 &&
            ranged_node_summary.trained_row_count_total == 60 &&
            ranged_node_summary.valid_target_count_total == 45 &&
            ranged_node_summary.valid_target_opportunity_count_total == 60 &&
            ranged_node_summary.weakest_valid_target_node_id == "ETH" &&
            ranged_node_summary.weakest_valid_target_count == 5 &&
            ranged_node_summary.weakest_valid_target_denominator == 10 &&
            std::abs(ranged_node_summary.valid_target_fraction_estimate -
                     0.75) < 1e-12 &&
            std::abs(ranged_node_summary.valid_target_wilson_lower_95 -
                     ranged_node_lower) < 1e-12 &&
            std::abs(ranged_node_summary.valid_target_wilson_upper_95 -
                     ranged_node_upper) < 1e-12,
        "range-scoped node support excludes outside rows and scales partial "
        "node facts before Wilson support");

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
  write_bad_source_key_window_job(bad_source_key_dir);
  const auto bad_source_key_scan =
      exposure::scan_exposure_ledger_from_runtime_root(bad_source_key_dir,
                                                       train_context);
  const auto bad_source_key_audit = exposure::audit_source_key_window(
      bad_source_key_scan.ledger.facts().front());
  check(bad_source_key_audit.available &&
            !bad_source_key_audit.issues.empty() &&
            bad_source_key_audit.complete && bad_source_key_audit.numeric &&
            !bad_source_key_audit.internally_monotone &&
            bad_source_key_audit.irregular_key_warning_count == 1 &&
            bad_source_key_audit.source_key_gap_found,
        "source-key window audit reports non-monotone key intervals as "
        "structured evidence");
  check(std::any_of(bad_source_key_scan.warnings.begin(),
                    bad_source_key_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("source-key window warning") !=
                                 std::string::npos &&
                             warning.find("anchor_source_key_nonmonotone") !=
                                 std::string::npos &&
                             warning.find("observed_source_key_nonmonotone") !=
                                 std::string::npos;
                    }),
        "source-key window audit warns on non-monotone key intervals");
  write_bad_source_key_order_job(bad_source_key_order_dir);
  const auto bad_source_key_order_scan =
      exposure::scan_exposure_ledger_from_runtime_root(bad_source_key_order_dir,
                                                       train_context);
  const auto bad_source_key_order_audit = exposure::audit_source_key_window(
      bad_source_key_order_scan.ledger.facts().front());
  check(bad_source_key_order_audit.available &&
            bad_source_key_order_audit.internally_monotone &&
            !bad_source_key_order_audit.order_preserving &&
            std::any_of(bad_source_key_order_audit.issues.begin(),
                        bad_source_key_order_audit.issues.end(),
                        [](const auto &issue) {
                          return issue.find("source_key_order_inverted") !=
                                 std::string::npos;
                        }),
        "source-key window audit reports row-to-key order inversions as "
        "structured evidence");
  check(std::any_of(bad_source_key_order_scan.warnings.begin(),
                    bad_source_key_order_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("source-key window warning") !=
                                 std::string::npos &&
                             warning.find("anchor_begin_target_begin_"
                                          "source_key_order_inverted") !=
                                 std::string::npos;
                    }),
        "source-key window audit warns when row-to-key endpoint order is "
        "inverted");
  write_bad_source_key_affine_job(bad_source_key_affine_dir);
  const auto bad_source_key_affine_scan =
      exposure::scan_exposure_ledger_from_runtime_root(
          bad_source_key_affine_dir, train_context);
  const auto bad_source_key_affine_audit = exposure::audit_source_key_window(
      bad_source_key_affine_scan.ledger.facts().front());
  check(bad_source_key_affine_audit.available &&
            bad_source_key_affine_audit.internally_monotone &&
            bad_source_key_affine_audit.order_preserving &&
            bad_source_key_affine_audit.affine_step_available &&
            bad_source_key_affine_audit.reference_key_step == 1 &&
            !bad_source_key_affine_audit.affine_consistent &&
            bad_source_key_affine_audit.row_source_key_mismatch_found &&
            bad_source_key_affine_audit.row_source_key_mismatch_count == 1 &&
            bad_source_key_affine_audit.source_key_gap_warning_count == 1 &&
            std::any_of(bad_source_key_affine_audit.issues.begin(),
                        bad_source_key_affine_audit.issues.end(),
                        [](const auto &issue) {
                          return issue ==
                                 "observed_begin_source_key_affine_mismatch";
                        }),
        "source-key window audit reports affine row-to-key mismatches even "
        "when endpoint order is preserved");
  check(std::any_of(bad_source_key_affine_scan.warnings.begin(),
                    bad_source_key_affine_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("source-key window warning") !=
                                 std::string::npos &&
                             warning.find("observed_begin_source_key_"
                                          "affine_mismatch") !=
                                 std::string::npos;
                    }),
        "source-key window audit warns when a regular key window violates the "
        "affine row-to-key map");

  write_irregular_source_key_job(irregular_source_key_dir);
  const auto irregular_source_key_scan =
      exposure::scan_exposure_ledger_from_runtime_root(irregular_source_key_dir,
                                                       train_context);
  const auto irregular_source_key_audit = exposure::audit_source_key_window(
      irregular_source_key_scan.ledger.facts().front());
  check(irregular_source_key_audit.available &&
            irregular_source_key_audit.complete &&
            irregular_source_key_audit.numeric &&
            irregular_source_key_audit.internally_monotone &&
            irregular_source_key_audit.order_preserving &&
            !irregular_source_key_audit.affine_step_available &&
            !irregular_source_key_audit.affine_consistent &&
            irregular_source_key_audit.irregular_key_warning_count == 1 &&
            irregular_source_key_audit.source_key_gap_warning_count == 1 &&
            irregular_source_key_audit.source_key_gap_found &&
            std::any_of(irregular_source_key_audit.issues.begin(),
                        irregular_source_key_audit.issues.end(),
                        [](const auto &issue) {
                          return issue == "anchor_source_key_step_irregular";
                        }),
        "source-key window audit reports irregular monotone key steps as gap "
        "warnings without making row-index coverage non-authoritative");
  const auto irregular_source_key_summary =
      exposure::summarize_source_key_map_audits(
          irregular_source_key_scan.ledger.facts(),
          irregular_source_key_scan.ledger.source_receipt_facts());
  check(irregular_source_key_summary.available_audit_count == 1 &&
            irregular_source_key_summary.irregular_key_warning_count == 1 &&
            irregular_source_key_summary.source_key_gap_warning_count == 1 &&
            irregular_source_key_summary.audits_with_graph_order_count == 1 &&
            irregular_source_key_summary.audits_with_source_cursor_count == 1 &&
            irregular_source_key_summary.audits_with_source_receipts_count ==
                1 &&
            irregular_source_key_summary.issue_count == 1 &&
            irregular_source_key_summary.audit_only &&
            !irregular_source_key_summary.coverage_authority &&
            !irregular_source_key_summary.leakage_authority &&
            irregular_source_key_summary.row_index_authority_preserved &&
            irregular_source_key_summary.summary_self_check_passed,
        "source-key map summary keeps irregular key warnings audit-only and "
        "bound to receipts/identity");

  const auto bad_source_receipt_dir = root / "bad_source_receipt";
  write_malformed_source_receipt_job(bad_source_receipt_dir);
  const auto bad_source_receipt_scan =
      exposure::scan_exposure_ledger_from_runtime_root(bad_source_receipt_dir,
                                                       train_context);
  check(bad_source_receipt_scan.ledger.source_receipt_facts().empty(),
        "malformed source receipts do not create structured receipt facts");
  const auto bad_source_receipt_summary = exposure::summarize_source_receipts(
      bad_source_receipt_scan.ledger.facts(),
      bad_source_receipt_scan.ledger.source_receipt_facts());
  check(bad_source_receipt_summary.exposure_facts_with_receipts == 1 &&
            bad_source_receipt_summary.malformed_receipt_count == 1 &&
            !bad_source_receipt_summary.issues.empty(),
        "malformed source receipts are counted in audit-only receipt summary");
  check(std::any_of(bad_source_receipt_scan.warnings.begin(),
                    bad_source_receipt_scan.warnings.end(),
                    [](const auto &warning) {
                      return warning.find("source receipt warning") !=
                                 std::string::npos &&
                             warning.find("missing_interval") !=
                                 std::string::npos &&
                             warning.find("missing_record_type") !=
                                 std::string::npos;
                    }),
        "malformed source receipts are surfaced as receipt-provenance "
        "warnings");

  exposure::lattice_exposure_ledger_t ledger;
  ledger.add(rep_fact);
  ledger.add(mdn_fact);
  const auto closure = ledger.checkpoint_closure(mdn_dir / "mdn.pt");
  check(closure.size() == 2,
        "MDN checkpoint closure includes MDN and representation exposure");
  check(!ledger.checkpoint_closure_digest(mdn_dir / "mdn.pt").empty(),
        "checkpoint closure digest is present");

  const auto rep_identity_fact =
      exposure::make_checkpoint_fact_from_exposure_fact(rep_fact);
  const auto mdn_identity_fact =
      exposure::make_checkpoint_fact_from_exposure_fact(mdn_fact);
  exposure::lattice_exposure_ledger_t identity_ledger;
  identity_ledger.add(rep_fact);
  identity_ledger.add(mdn_fact);
  identity_ledger.add_checkpoint(rep_identity_fact);
  identity_ledger.add_checkpoint(mdn_identity_fact);
  const auto identity_closure =
      identity_ledger.checkpoint_closure_result(mdn_dir / "mdn.pt");
  check(identity_closure.complete() && identity_closure.facts.size() == 2,
        "checkpoint id/digest closure remains complete");
  check(identity_closure.resolution_authority == "checkpoint_id_file_digest" &&
            !identity_closure.legacy_path_fallback &&
            identity_closure.root_checkpoint_id ==
                mdn_identity_fact.checkpoint_id &&
            identity_closure.root_checkpoint_file_digest ==
                mdn_identity_fact.checkpoint_file_digest,
        "checkpoint closure promotes complete checkpoint facts to id/digest "
        "authority");
  const auto identity_lookup =
      identity_ledger.checkpoint_closure_result_for_identity(
          mdn_identity_fact.checkpoint_id,
          mdn_identity_fact.checkpoint_file_digest);
  check(identity_lookup.complete() && identity_lookup.facts.size() == 2 &&
            identity_lookup.resolution_authority == "checkpoint_id_file_digest",
        "checkpoint closure resolves directly by checkpoint id and file "
        "digest");

  const auto unknown_root_closure =
      identity_ledger.checkpoint_closure_result(root / "missing_root.pt");
  check(!unknown_root_closure.complete() &&
            unknown_root_closure.facts.empty() &&
            unknown_root_closure.resolution_authority == "unresolved_lineage" &&
            std::find(unknown_root_closure.unresolved_input_checkpoints.begin(),
                      unknown_root_closure.unresolved_input_checkpoints.end(),
                      root / "missing_root.pt") !=
                unknown_root_closure.unresolved_input_checkpoints.end(),
        "unknown root checkpoint paths should be unresolved instead of "
        "complete with zero facts");

  auto legacy_mdn_checkpoint_fact = mdn_identity_fact;
  legacy_mdn_checkpoint_fact.checkpoint_id.clear();
  legacy_mdn_checkpoint_fact.checkpoint_file_digest.clear();
  exposure::lattice_exposure_ledger_t legacy_identity_ledger;
  legacy_identity_ledger.add(rep_fact);
  legacy_identity_ledger.add(mdn_fact);
  legacy_identity_ledger.add_checkpoint(rep_identity_fact);
  legacy_identity_ledger.add_checkpoint(legacy_mdn_checkpoint_fact);
  const auto legacy_identity_closure =
      legacy_identity_ledger.checkpoint_closure_result(mdn_dir / "mdn.pt");
  check(legacy_identity_closure.complete() &&
            legacy_identity_closure.resolution_authority == "legacy_path" &&
            legacy_identity_closure.legacy_path_fallback,
        "checkpoint closure keeps missing id/digest evidence as explicit "
        "legacy path fallback");

  auto wrong_digest_checkpoint_fact = mdn_identity_fact;
  wrong_digest_checkpoint_fact.checkpoint_file_digest = "0000000000000000";
  wrong_digest_checkpoint_fact
      .checkpoint_id = exposure::exposure_digest_for_text(
      std::string("checkpoint|") +
      wrong_digest_checkpoint_fact.checkpoint_path.lexically_normal().string() +
      "|" + wrong_digest_checkpoint_fact.checkpoint_file_digest + "|" +
      wrong_digest_checkpoint_fact.direct_exposure_digest);
  exposure::lattice_exposure_ledger_t wrong_digest_ledger;
  wrong_digest_ledger.add(rep_fact);
  wrong_digest_ledger.add(mdn_fact);
  wrong_digest_ledger.add_checkpoint(rep_identity_fact);
  wrong_digest_ledger.add_checkpoint(wrong_digest_checkpoint_fact);
  const auto wrong_digest_closure =
      wrong_digest_ledger.checkpoint_closure_result(mdn_dir / "mdn.pt");
  check(!wrong_digest_closure.complete() &&
            wrong_digest_closure.resolution_authority ==
                "checkpoint_identity_failed" &&
            std::any_of(wrong_digest_closure.identity_mismatches.begin(),
                        wrong_digest_closure.identity_mismatches.end(),
                        [](const auto &mismatch) {
                          return mismatch.find("checkpoint_file_digest") !=
                                 std::string::npos;
                        }),
        "checkpoint closure fails closed when checkpoint file digest does not "
        "match runtime checkpoint bytes");

  auto missing_bytes_fact = mdn_fact;
  missing_bytes_fact.output_checkpoint = root / "missing_bytes.pt";
  auto missing_bytes_checkpoint_fact = mdn_identity_fact;
  missing_bytes_checkpoint_fact.checkpoint_path =
      missing_bytes_fact.output_checkpoint;
  missing_bytes_checkpoint_fact.checkpoint_file_digest = "1111111111111111";
  missing_bytes_checkpoint_fact.direct_exposure_digest =
      exposure::exposure_fact_digest(missing_bytes_fact);
  missing_bytes_checkpoint_fact.checkpoint_id =
      exposure::exposure_digest_for_text(
          std::string("checkpoint|") +
          missing_bytes_checkpoint_fact.checkpoint_path.lexically_normal()
              .string() +
          "|" + missing_bytes_checkpoint_fact.checkpoint_file_digest + "|" +
          missing_bytes_checkpoint_fact.direct_exposure_digest);
  exposure::lattice_exposure_ledger_t missing_bytes_ledger;
  missing_bytes_ledger.add(missing_bytes_fact);
  missing_bytes_ledger.add_checkpoint(missing_bytes_checkpoint_fact);
  const auto missing_bytes_closure =
      missing_bytes_ledger.checkpoint_closure_result(
          missing_bytes_fact.output_checkpoint);
  check(!missing_bytes_closure.complete() &&
            missing_bytes_closure.resolution_authority ==
                "checkpoint_identity_failed" &&
            std::any_of(
                missing_bytes_closure.identity_mismatches.begin(),
                missing_bytes_closure.identity_mismatches.end(),
                [](const auto &mismatch) {
                  return mismatch.find(
                             "checkpoint bytes could not be verified") !=
                             std::string::npos &&
                         mismatch.find("open_failed") != std::string::npos;
                }),
        "checkpoint closure fails closed when checkpoint bytes are missing");

  auto empty_bytes_fact = mdn_fact;
  empty_bytes_fact.output_checkpoint = root / "empty_bytes.pt";
  write_text(empty_bytes_fact.output_checkpoint, "");
  auto empty_bytes_checkpoint_fact = mdn_identity_fact;
  empty_bytes_checkpoint_fact.checkpoint_path =
      empty_bytes_fact.output_checkpoint;
  empty_bytes_checkpoint_fact.checkpoint_file_digest = "2222222222222222";
  empty_bytes_checkpoint_fact.direct_exposure_digest =
      exposure::exposure_fact_digest(empty_bytes_fact);
  empty_bytes_checkpoint_fact
      .checkpoint_id = exposure::exposure_digest_for_text(
      std::string("checkpoint|") +
      empty_bytes_checkpoint_fact.checkpoint_path.lexically_normal().string() +
      "|" + empty_bytes_checkpoint_fact.checkpoint_file_digest + "|" +
      empty_bytes_checkpoint_fact.direct_exposure_digest);
  exposure::lattice_exposure_ledger_t empty_bytes_ledger;
  empty_bytes_ledger.add(empty_bytes_fact);
  empty_bytes_ledger.add_checkpoint(empty_bytes_checkpoint_fact);
  const auto empty_bytes_closure = empty_bytes_ledger.checkpoint_closure_result(
      empty_bytes_fact.output_checkpoint);
  check(
      !empty_bytes_closure.complete() &&
          empty_bytes_closure.resolution_authority ==
              "checkpoint_identity_failed" &&
          std::any_of(empty_bytes_closure.identity_mismatches.begin(),
                      empty_bytes_closure.identity_mismatches.end(),
                      [](const auto &mismatch) {
                        return mismatch.find(
                                   "checkpoint bytes could not be verified") !=
                                   std::string::npos &&
                               mismatch.find("empty_file") != std::string::npos;
                      }),
      "checkpoint closure fails closed when checkpoint bytes are empty");

  auto stale_identity_fact = mdn_identity_fact;
  stale_identity_fact.checkpoint_file_digest = "3333333333333333";
  stale_identity_fact.direct_exposure_digest = "4444444444444444";
  stale_identity_fact.checkpoint_id = exposure::exposure_digest_for_text(
      std::string("checkpoint|") +
      stale_identity_fact.checkpoint_path.lexically_normal().string() + "|" +
      stale_identity_fact.checkpoint_file_digest + "|" +
      stale_identity_fact.direct_exposure_digest);
  exposure::lattice_exposure_ledger_t stale_identity_lookup_ledger;
  stale_identity_lookup_ledger.add(rep_fact);
  stale_identity_lookup_ledger.add(mdn_fact);
  stale_identity_lookup_ledger.add_checkpoint(rep_identity_fact);
  stale_identity_lookup_ledger.add_checkpoint(mdn_identity_fact);
  stale_identity_lookup_ledger.add_checkpoint(stale_identity_fact);
  const auto stale_identity_lookup =
      stale_identity_lookup_ledger.checkpoint_closure_result_for_identity(
          stale_identity_fact.checkpoint_id,
          stale_identity_fact.checkpoint_file_digest);
  check(!stale_identity_lookup.complete() &&
            stale_identity_lookup.resolution_authority ==
                "checkpoint_identity_failed" &&
            stale_identity_lookup.root_checkpoint_id ==
                mdn_identity_fact.checkpoint_id &&
            std::any_of(stale_identity_lookup.identity_mismatches.begin(),
                        stale_identity_lookup.identity_mismatches.end(),
                        [](const auto &mismatch) {
                          return mismatch.find(
                                     "resolved root checkpoint identity does "
                                     "not match requested") !=
                                 std::string::npos;
                        }),
        "checkpoint identity lookup should fail closed when a stale "
        "id/digest resolves to a path whose current producer identity differs");

  auto wrong_id_checkpoint_fact = mdn_identity_fact;
  wrong_id_checkpoint_fact.checkpoint_id = "ffffffffffffffff";
  exposure::lattice_exposure_ledger_t wrong_id_ledger;
  wrong_id_ledger.add(rep_fact);
  wrong_id_ledger.add(mdn_fact);
  wrong_id_ledger.add_checkpoint(rep_identity_fact);
  wrong_id_ledger.add_checkpoint(wrong_id_checkpoint_fact);
  const auto wrong_id_closure =
      wrong_id_ledger.checkpoint_closure_result(mdn_dir / "mdn.pt");
  check(!wrong_id_closure.complete() &&
            wrong_id_closure.resolution_authority ==
                "checkpoint_identity_failed" &&
            std::any_of(wrong_id_closure.identity_mismatches.begin(),
                        wrong_id_closure.identity_mismatches.end(),
                        [](const auto &mismatch) {
                          return mismatch.find(
                                     "checkpoint_id does not match") !=
                                 std::string::npos;
                        }),
        "checkpoint closure fails closed when checkpoint id does not bind to "
        "path, digest, and exposure fact");
  const auto wrong_digest_lookup =
      identity_ledger.checkpoint_closure_result_for_identity(
          mdn_identity_fact.checkpoint_id, "wrong_digest");
  check(!wrong_digest_lookup.complete() &&
            !wrong_digest_lookup.identity_mismatches.empty(),
        "checkpoint identity lookup fails closed on wrong digest");

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
  check(coverage.contributing_intervals.size() == 2 &&
            coverage.contributing_intervals.front().begin == 0 &&
            coverage.contributing_intervals.back().end == 100,
        "coverage reports sorted contributing intervals");
  check(coverage.contributing_fact_digests.size() == 2 &&
            coverage.contributing_fact_digests.front() ==
                exposure::exposure_fact_digest(manual_a) &&
            coverage.contributing_fact_digests.back() ==
                exposure::exposure_fact_digest(manual_b),
        "coverage reports fact digests beside contributing intervals");
  check(coverage.contributing_facts.size() == 2 &&
            coverage.contributing_facts.front().job_id == manual_a.job_id &&
            coverage.contributing_facts.back().job_id == manual_b.job_id,
        "coverage carries local contributing fact witnesses");
  check(coverage.covered_intervals.size() == 1 &&
            coverage.covered_intervals.front().begin == 0 &&
            coverage.covered_intervals.front().end == 100,
        "coverage reports merged idempotent union intervals");
  check(coverage.missing_intervals.empty() && coverage.missing_anchors == 0,
        "full coverage reports no missing intervals");
  const auto partial_coverage = exposure::coverage_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{manual_a},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(partial_coverage.covered_anchors == 50 &&
            partial_coverage.missing_anchors == 50,
        "partial coverage reports missing anchor count");
  check(partial_coverage.missing_intervals.size() == 1 &&
            partial_coverage.missing_intervals.front().begin == 50 &&
            partial_coverage.missing_intervals.front().end == 100,
        "partial coverage reports complement intervals");
  const auto one_pass_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{manual_a, manual_b},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(one_pass_load.unique_covered_anchors == 100,
        "one-pass load unique coverage count");
  check(one_pass_load.unique_coverage_algebra == "idempotent_interval_union" &&
            one_pass_load.load_algebra == "additive_interval_measure",
        "load summary names the distinct coverage/load algebras");
  check(one_pass_load.unique_coverage_unit == "coverage_fraction" &&
            one_pass_load.load_unit == "cursor_epoch" &&
            one_pass_load.anchor_event_unit == "anchor_event",
        "load summary names the proof units");
  const auto algebra_summary =
      exposure::lattice_exposure_measure_algebra_summary();
  check(algebra_summary.schema ==
                "kikijyeba.lattice.exposure_measure_algebra.summary.v1" &&
            algebra_summary.measure_count == 2 &&
            algebra_summary.idempotent_measure_count == 1 &&
            algebra_summary.additive_measure_count == 1 &&
            algebra_summary.coverage_measure_count == 1 &&
            algebra_summary.load_measure_count == 1 &&
            algebra_summary.dual_idempotent_additive_count == 0 &&
            algebra_summary.empty_field_count == 0 &&
            algebra_summary.unique_coverage_is_idempotent_union &&
            algebra_summary.exposure_load_is_additive_measure &&
            algebra_summary.coverage_not_additive &&
            algebra_summary.load_not_idempotent &&
            algebra_summary.measure_units_declared &&
            algebra_summary.coverage_and_load_units_distinct &&
            algebra_summary.summary_self_check_passed &&
            algebra_summary.summary_issue_count == 0 &&
            algebra_summary.summary_issues.empty() &&
            std::find(algebra_summary.measure_names.begin(),
                      algebra_summary.measure_names.end(), "unique_coverage") !=
                algebra_summary.measure_names.end() &&
            std::find(algebra_summary.measure_names.begin(),
                      algebra_summary.measure_names.end(),
                      "exposure_load") != algebra_summary.measure_names.end(),
        "exposure algebra summary should self-check idempotent coverage "
        "versus additive load");
  const auto source_key_policy_summary =
      exposure::lattice_source_key_coordinate_policy_summary();
  check(source_key_policy_summary.schema ==
                "kikijyeba.lattice.source_key_coordinate_policy.summary.v1" &&
            source_key_policy_summary.policy_count == 5 &&
            source_key_policy_summary.coverage_authority_count == 1 &&
            source_key_policy_summary.leakage_authority_count == 1 &&
            source_key_policy_summary.audit_only_count == 4 &&
            source_key_policy_summary.row_index_coordinate_count == 1 &&
            source_key_policy_summary.source_key_coordinate_count == 1 &&
            source_key_policy_summary.row_to_source_key_map_count == 3 &&
            source_key_policy_summary.empty_field_count == 0 &&
            source_key_policy_summary
                .row_index_is_coverage_and_leakage_authority &&
            source_key_policy_summary.source_key_rows_are_audit_only &&
            source_key_policy_summary
                .audit_rows_have_no_coverage_or_leakage_authority &&
            source_key_policy_summary.order_preserving_fields_declared &&
            source_key_policy_summary.affine_fields_declared &&
            source_key_policy_summary.gap_and_irregular_key_check_present &&
            source_key_policy_summary.gap_fields_declared &&
            source_key_policy_summary.all_policies_have_claim_text &&
            source_key_policy_summary.summary_self_check_passed &&
            source_key_policy_summary.summary_issue_count == 0 &&
            source_key_policy_summary.summary_issues.empty(),
        "source-key coordinate policy summary should self-check row-index "
        "authority and audit-only source-key maps");
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
            source_receipt_policy_summary.summary_issues.empty(),
        "source receipt policy summary should self-check audit-only receipts "
        "and row-index overlap authority");
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
          valid_target_uncertainty_summary.summary_issues.empty(),
      "valid-target uncertainty summary should self-check Wilson support "
      "scope bindings and no-trials non-claiming policy");
  check(one_pass_load.loaded_anchor_events == 100,
        "one-pass load counts completed anchor events");
  check(std::abs(one_pass_load.cursor_exposure_load - 1.0) < 1e-12,
        "one-pass load is one cursor epoch");
  const auto duplicate_coverage = exposure::coverage_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, rep_fact},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(duplicate_coverage.contributing_intervals.size() == 1 &&
            duplicate_coverage.contributing_fact_digests.size() == 1 &&
            duplicate_coverage.contributing_facts.size() == 1 &&
            duplicate_coverage.contributing_fact_digests.front() ==
                exposure::exposure_fact_digest(rep_fact) &&
            duplicate_coverage.covered_anchors == 100,
        "duplicate exposure fact digests collapse for coverage witnesses");
  const auto duplicate_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, rep_fact},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(duplicate_load.fact_count == 1 &&
            duplicate_load.loaded_anchor_events == 100 &&
            std::abs(duplicate_load.cursor_exposure_load - 1.0) < 1e-12 &&
            duplicate_load.optimizer_steps_total == rep_fact.optimizer_steps,
        "duplicate exposure fact digests do not add repeated load");

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

  auto outside = rep_fact;
  outside.anchor_range = exposure::anchor_interval_t{.begin = 100, .end = 150};
  outside.completed_anchor_range = outside.anchor_range;
  outside.observed_footprint = outside.anchor_range;
  outside.optimizer_steps = 7;
  const auto outside_coverage = exposure::coverage_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, outside},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(outside_coverage.contributing_facts.size() == 1 &&
            outside_coverage.contributing_facts.front().job_id ==
                rep_fact.job_id &&
            outside_coverage.covered_anchors == 100 &&
            outside_coverage.missing_anchors == 0,
        "non-overlapping exposure facts do not contribute coverage witnesses");
  const auto outside_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{rep_fact, outside},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(outside_load.fact_count == 1 &&
            outside_load.loaded_anchor_events == 100 &&
            outside_load.optimizer_steps_total == rep_fact.optimizer_steps &&
            std::abs(outside_load.cursor_exposure_load - 1.0) < 1e-12,
        "non-overlapping exposure facts do not add repeated load");

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

  auto dry_run = rep_fact;
  dry_run.job_status = "dry_run";
  dry_run.coverage_precision = "contiguous_completed_range_v1";
  dry_run.completed_anchor_range = dry_run.anchor_range;
  dry_run.use.mutated_component = true;
  const auto dry_run_coverage = exposure::coverage_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{dry_run},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(dry_run_coverage.covered_anchors == 0 &&
            dry_run_coverage.contributing_facts.empty(),
        "dry-run facts do not contribute readiness coverage even when a "
        "sidecar carries a completed range");
  const auto dry_run_load = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{dry_run},
      exposure::anchor_interval_t{.begin = 0, .end = 100},
      exposure::exposure_use_t::observed_input);
  check(dry_run_load.loaded_anchor_events == 0 && dry_run_load.fact_count == 0,
        "dry-run facts do not contribute repeated exposure load");
  exposure::forbidden_exposure_query_t dry_run_query{};
  dry_run_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 0, .end = 100};
  dry_run_query.forbidden_uses = {exposure::exposure_use_t::observed_input};
  check(!exposure::has_forbidden_exposure_overlap(
            std::vector<exposure::lattice_exposure_fact_t>{dry_run},
            dry_run_query),
        "dry-run facts do not create completed causal exposure witnesses");

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
  wrong_component.target_component_family_id = "other.component";
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
