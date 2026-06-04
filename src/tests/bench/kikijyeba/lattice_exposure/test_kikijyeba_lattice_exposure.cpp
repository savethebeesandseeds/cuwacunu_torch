#include "kikijyeba/lattice/exposure/exposure_ledger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
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

void append_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::app);
  check(out.is_open(), "failed to open output file: " + path.string());
  out << text;
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  check(in.is_open(), "failed to open input file: " + path.string());
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

bool has_filename(const std::vector<std::filesystem::path> &paths,
                  const std::string &filename) {
  return std::any_of(paths.begin(), paths.end(), [&](const auto &path) {
    return path.filename().string() == filename;
  });
}

bool contains_text(const std::vector<std::string> &items,
                   const std::string &needle) {
  return std::any_of(items.begin(), items.end(), [&](const auto &item) {
    return item.find(needle) != std::string::npos;
  });
}

void write_representation_job(const std::filesystem::path &dir,
                              std::int64_t begin, std::int64_t end,
                              const std::string &checkpoint_name) {
  write_text(
      dir / "job.manifest",
      "job_id=rep_job\n"
      "job_kind=representation_vicreg\n"
      "protocol_id=cwu_01v\n"
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

void write_channel_mdn_job(
    const std::filesystem::path &dir, std::int64_t begin, std::int64_t end,
    const std::filesystem::path &representation_checkpoint,
    const std::string &checkpoint_name) {
  write_text(
      dir / "job.manifest",
      "job_id=channel_mdn_job\n"
      "job_kind=channel_inference_mdn\n"
      "protocol_id=cwu_01v\n"
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
             "mean_nll_per_channel_target_feature=0.41,0.42,0.61,0.62,0.81,"
             "0.82\n"
             "mean_mixture_usage=0.25,0.75\n"
             "valid_target_count_per_channel=10,20,30\n"
             "valid_target_count_per_target_feature=27,33\n"
             "valid_target_count_per_channel_target_feature=4,6,9,11,14,16\n"
             "mdn_architecture=shared_slot_trunk.channel_adapter."
             "shared_feature_head.v2\n"
             "loss_reduction=balanced_channel_feature_mean\n"
             "feature_embedding_dim=4\n"
             "channel_adapter_rank=4\n"
             "shared_trunk=true\n"
             "channel_adapters_enabled=true\n"
             "shared_feature_head=true\n"
             "feature_embedding_enabled=true\n"
             "node_id_embedding=false\n"
             "cross_node_attention=false\n"
             "cross_channel_attention=false\n"
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
  write_text(dir / "runtime.result.fact", "fact_type=runtime.result.fact\n"
                                          "schema_version=1\n"
                                          "producer=runtime\n"
                                          "status=completed\n"
                                          "optimizer_steps=9\n"
                                          "checkpoint_written=true\n"
                                          "checkpoint_path=" +
                                              checkpoint_name +
                                              "\n"
                                              "model_state_mutated=true\n"
                                              "finite_parameter_check=true\n"
                                              "nonfinite_output_count=0\n"
                                              "mean_loss=0.40\n"
                                              "valid_target_fraction=0.50\n"
                                              "grad_norm_max_pre_clip=1200\n"
                                              "grad_clip_norm=5\n"
                                              "sigma_min=0.10\n"
                                              "sigma_mean=0.80\n"
                                              "sigma_max=1.50\n"
                                              "sigma_min_valid=0.11\n"
                                              "sigma_mean_valid=0.81\n"
                                              "sigma_max_valid=1.51\n"
                                              "mixture_entropy=0.60\n");
  write_text(dir / "runtime.checkpoint_io.fact",
             "fact_type=runtime.checkpoint_io.fact\n"
             "schema_version=1\n"
             "producer=runtime\n"
             "checkpoint_written=true\n"
             "checkpoint_path=" +
                 checkpoint_name +
                 "\n"
                 "representation_checkpoint_path=" +
                 representation_checkpoint.string() +
                 "\n"
                 "representation_checkpoint_loaded=true\n"
                 "mdn_checkpoint_path=\n"
                 "mdn_checkpoint_loaded=false\n"
                 "model_state_mutated=true\n");
  write_text(dir / "runtime.health_measurement.fact",
             "fact_type=runtime.health_measurement.fact\n"
             "schema_version=1\n"
             "producer=runtime\n"
             "finite_parameter_check=true\n"
             "nonfinite_output_count=0\n"
             "grad_norm_max_pre_clip=1200\n"
             "grad_clip_norm=5\n"
             "sigma_min=0.10\n"
             "sigma_mean=0.80\n"
             "sigma_max=1.50\n"
             "sigma_min_valid=0.11\n"
             "sigma_mean_valid=0.81\n"
             "sigma_max_valid=1.51\n"
             "mixture_entropy=0.60\n");
  write_text(dir / checkpoint_name, "channel mdn checkpoint");
}

void write_unproven_mutation_job(const std::filesystem::path &dir,
                                 const std::string &checkpoint_name) {
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  write_text(
      dir / "job.manifest",
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
  const auto channel_mdn_dir = root / "channel_mdn";
  const auto channel_mdn_checkpoint_path = channel_mdn_dir / "channel_mdn.pt";
  const auto unproven_dir = root / "unproven";
  const auto domain_warning_dir = root / "domain_warning";
  const auto bad_source_key_dir = root / "bad_source_key";
  const auto bad_source_key_order_dir = root / "bad_source_key_order";
  const auto bad_source_key_affine_dir = root / "bad_source_key_affine";
  const auto irregular_source_key_dir = root / "irregular_source_key";

  write_representation_job(rep_dir, 0, 100, "rep.pt");
  write_channel_mdn_job(channel_mdn_dir, 200, 320, rep_dir / "rep.pt",
                        "channel_mdn.pt");
  write_unproven_mutation_job(unproven_dir, "unproven.pt");
  write_text(rep_dir / "lattice.source_analytics.fact",
             "schema=kikijyeba.lattice.source_analytics.v1\n"
             "entropy=1.25\n"
             "entropy_rate=0.50\n"
             "information_density=0.75\n"
             "compression_ratio=2.50\n"
             "power_spectrum_entropy=0.33\n"
             "source_volatility=0.14\n"
             "feature_variance=0.04\n"
             "sample_validity_fraction=0.98\n"
             "missingness_fraction=0.02\n"
             "duplicate_sample_count=3\n"
             "source_health_level=warn\n"
             "visibility_only=true\n");
  write_text(rep_dir / "lattice.target_transform.fact",
             "schema=kikijyeba.lattice.target_transform.v1\n"
             "target_feature_ids=ev_close,ev_return\n"
             "horizon=3\n"
             "target_mode=log_return\n"
             "normalization_contract=zscore.train_core.v1\n"
             "inverse_transform_contract=exp_return.v1\n"
             "units=log_return\n"
             "target_mask_policy_digest=mask_policy_1\n"
             "support_surface_identity=BTC_ETH_SOL:h3\n"
             "support_surface_digest=support_surface_1\n"
             "artifact_contract_prerequisite=true\n"
             "visibility_only=true\n");
  write_text(rep_dir / "lattice.forecast_baseline.fact",
             "schema=kikijyeba.lattice.forecast_baseline.v1\n"
             "target_feature_ids=ev_close,ev_return\n"
             "horizon=3\n"
             "baseline_kind=previous_value\n"
             "baseline_parameters=lag=1\n"
             "target_transform_fact_digest=target_transform_digest_1\n"
             "support_count=42\n"
             "valid_count=40\n"
             "missing_count=2\n"
             "metric_status=computed\n"
             "baseline_mean_nll=1.50\n"
             "baseline_ev_mae=0.20\n"
             "baseline_ev_rmse=0.30\n"
             "baseline_signed_error=-0.01\n"
             "baseline_directional_accuracy=0.55\n"
             "evidence_prerequisite=true\n"
             "visibility_only=true\n");
  write_text(channel_mdn_dir / "lattice.forecast_eval.fact",
             "schema=kikijyeba.lattice.forecast_eval.v1\n"
             "target_feature_ids=ev_close,ev_return\n"
             "horizon=3\n"
             "support_count=42\n"
             "valid_count=40\n"
             "missing_count=2\n"
             "weakest_support_rows=7\n"
             "mean_nll=1.10\n"
             "mean_nll_per_channel=1.00,1.20\n"
             "mean_nll_per_target_feature=0.90,1.30\n"
             "mean_nll_per_channel_target_feature=0.80,1.00,1.20,1.40\n"
             "mean_nll_per_horizon=1.10\n"
             "valid_target_count_per_channel=20,20\n"
             "valid_target_count_per_target_feature=19,21\n"
             "valid_target_count_per_channel_target_feature=9,11,10,10\n"
             "valid_target_count_per_horizon=40\n"
             "ev_mae=0.16\n"
             "ev_rmse=0.24\n"
             "signed_error=-0.02\n"
             "directional_accuracy=0.62\n"
             "calibration_coverage=0.91\n"
             "pit_summary=uniform-ish\n"
             "sigma_scale_sanity=ok\n"
             "support_by_node=BTC:20,ETH:13,SOL:7\n"
             "support_by_channel=close:40\n"
             "support_by_target_feature=ev_close:40,ev_return:40\n"
             "support_by_horizon=h3:40\n"
             "forecast_artifact_digest=forecast_artifact_1\n"
             "evaluated_representation_checkpoint_digest=rep_checkpoint_1\n"
             "evaluated_mdn_checkpoint_digest=mdn_checkpoint_1\n"
             "target_transform_fact_digest=target_transform_digest_1\n"
             "baseline_fact_digests=forecast_baseline_digest_1\n"
             "selection_signal_fact_digests=selection_signal_digest_1\n"
             "artifact_evidence=true\n"
             "visibility_only=true\n"
             "model_state_mutation=false\n");
  write_text(channel_mdn_dir / "lattice.observer_belief.fact",
             "schema=kikijyeba.lattice.observer_belief.v1\n"
             "belief_kind=raw_nodelift_potential\n"
             "channel_consensus=BTC:0.62,ETH:0.58\n"
             "potential_surface_diagnostics=stable_surface\n"
             "nodelift_return_projection=potential_only\n"
             "covariance_coupling=diagonal_scenario_bank\n"
             "scenario_bank_digest=scenario_bank_1\n"
             "nodelift_residual_quality=ok\n"
             "projection_validation_scores=rmse:0.24,calibration:0.91\n"
             "confidence=0.64\n"
             "data_quality=0.88\n"
             "liquidity=0.72\n"
             "forecast_artifact_digest=forecast_artifact_1\n"
             "forecast_artifact_lineage=forecast_eval_digest_1\n"
             "feature_semantics_fingerprint=feature_semantics_1\n"
             "dock_binding_fingerprint=dock_binding_1\n"
             "artifact_evidence=true\n"
             "visibility_only=true\n"
             "raw_potential_tradable_return=false\n"
             "allocation_authority=false\n");
  write_text(channel_mdn_dir / "lattice.allocation_engine.fact",
             "schema=kikijyeba.lattice.allocation_engine.v1\n"
             "target_risky_node_weights=BTC:0.40,ETH:0.25,SOL:0.10\n"
             "reserve_node_id=USD_CASH\n"
             "reserve_node_source=base_policy\n"
             "base_policy_reserve_node_id=USD_CASH\n"
             "reserve_node_graph_bound=true\n"
             "reserve_weight=0.25\n"
             "turnover=0.12\n"
             "objective_terms=growth:0.70,cvar:0.20,cost:0.10\n"
             "cvar_loss=0.08\n"
             "transaction_cost_estimate=0.003\n"
             "constraint_diagnostics=all_constraints_satisfied\n"
             "cap_diagnostics=per_node_caps_ok\n"
             "scenario_growth_floor_status=met\n"
             "fallback_reasons=none\n"
             "derisk_reasons=none\n"
             "observer_belief_fact_digest=observer_belief_digest_1\n"
             "forecast_artifact_digest=forecast_artifact_1\n"
             "base_policy_digest=base_policy_1\n"
             "deterministic_artifact=true\n"
             "visibility_only=true\n"
             "allocation_authority=false\n"
             "execution_authority=false\n");
  const auto replay_artifact_dir =
      channel_mdn_dir / "artifacts" / "kikijyeba.environment.replay.v1";
  write_text(replay_artifact_dir / "runtime_replay_batches.index",
             "schema=kikijyeba.environment.replay.runtime_batch_index.v1\n"
             "entry_count=1\n"
             "entry_0_batch_id=batch_0\n");
  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index.v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=replay_experiment_1\n"
             "entry_0_runtime_run_id=runtime_run_1\n"
             "entry_0_environment_run_id=replay_env_1\n"
             "entry_0_report_path=runtime_replay_experiment.report\n"
             "entry_0_replay_bundle_count=2\n"
             "entry_0_policy_count=2\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n");
  write_text(
      replay_artifact_dir / "runtime_replay_experiment.report",
      std::string("schema=") +
          exposure::replay_environment_experiment_report_schema_id() + "\n" +
          "experiment_id=replay_experiment_1\n"
          "runtime_run_id=runtime_run_1\n"
          "environment_run_id=replay_env_1\n"
          "execution_profile_digest=execution_profile_digest_1\n"
          "policy_set_digest=policy_set_digest_1\n"
          "replay_environment_version=kikijyeba.environment.replay.v1\n"
          "replay_environment_component_assembly_id=replay_environment_v1\n"
          "replay_environment_world_mode=historical_replay\n"
          "replay_environment_api_contract=rl_compatible_reset_step\n"
          "replay_environment_spawn_model=episode_parallel_step_sequential\n"
          "replay_environment_range_source=ujcamei_component_stream_cursor\n"
          "replay_environment_source_range_policy=anchor_index_or_source_key\n"
          "replay_environment_source_order_policy=sequential\n"
          "replay_environment_range_resolution=runtime_resolved_cursor_"
          "identity\n"
          "replay_environment_observation_time_law=time_t_only\n"
          "replay_environment_realization_reveal=after_action_execution\n"
          "replay_environment_realization_key_policy=shared_key_per_frame\n"
          "replay_environment_action_kind=target_node_weights_with_base_"
          "reserve\n"
          "replay_environment_action_time_policy=decision_timestamp_after_"
          "knowledge_before_realization\n"
          "replay_environment_reserve_node_policy=graph_node_from_base_policy\n"
          "replay_environment_graph_node_universe_policy="
          "episode_spec_graph_node_ids\n"
          "replay_environment_reward_policy=post_execution_ledger_log_growth_"
          "drawdown_cost_turnover_invalid\n"
          "replay_environment_projection_validation=projected_log_return_vs_"
          "realized_asset_base_return\n"
          "replay_environment_policy_surface=policy_adapter\n"
          "replay_environment_action_policy_identity="
          "policy_adapter_must_match_action\n"
          "replay_environment_initial_policy_kind=deterministic_allocator_or_"
          "baseline\n"
          "replay_environment_experiment_task_identity=bundle_policy_task_"
          "indices\n"
          "replay_environment_experiment_run_identity=single_runtime_"
          "environment_"
          "run\n"
          "replay_environment_step_artifact_identity=episode_run_policy_"
          "cursor\n"
          "replay_environment_experiment_report_count_policy="
          "counts_match_evidence\n"
          "replay_environment_artifact_schema=cajtucu_ready_replay_artifacts\n"
          "replay_environment_lattice_fact_family=replay_environment\n"
          "replay_environment_lattice_target=replay_environment_artifact_"
          "ready\n"
          "replay_environment_require_resolved_cursor=true\n"
          "replay_environment_require_no_future_leakage=true\n"
          "replay_environment_require_projection_validation=true\n"
          "replay_environment_default_max_parallel_jobs=1\n"
          "experiment_requested_max_parallel_jobs=2\n"
          "experiment_resolved_parallelism=2\n"
          "time_law_expected_step_count=4\n"
          "time_law_observation_step_count=4\n"
          "time_law_action_step_count=4\n"
          "time_law_execution_step_count=4\n"
          "time_law_realization_after_action_count=4\n"
          "time_law_future_observation_violation_count=0\n"
          "mixed_future_realization_key_count=0\n"
          "projection_validation_step_count=4\n"
          "cajtucu_valid_trace_count=4\n"
          "cajtucu_invalid_trace_count=0\n"
          "cajtucu_missing_direct_reserve_edge_count=0\n"
          "cajtucu_nontradable_edge_reject_count=0\n"
          "cajtucu_below_min_notional_reject_count=0\n"
          "cajtucu_above_max_notional_reject_count=0\n"
          "cajtucu_insufficient_reserve_reject_count=0\n"
          "cajtucu_insufficient_units_reject_count=0\n"
          "cajtucu_invalid_sell_price_count=0\n"
          "cajtucu_large_equity_mismatch_count=0\n"
          "cajtucu_synthetic_market_step_count=0\n"
          "requested_order_count=4\n"
          "executed_order_count=4\n"
          "rejected_order_count=0\n"
          "partial_order_count=0\n"
          "episode_count=2\n"
          "episode_0_requested_anchor_index_begin=10\n"
          "episode_0_requested_anchor_index_end=12\n"
          "episode_0_requested_source_key_begin=1000\n"
          "episode_0_requested_source_key_end=1002\n"
          "episode_0_accepted_cursor_kind=graph_anchor\n"
          "episode_0_accepted_cursor_scope=episode\n"
          "episode_0_accepted_batch_cursor_token=cursor_0\n"
          "episode_0_accepted_anchor_index_begin=10\n"
          "episode_0_accepted_anchor_index_end=12\n"
          "episode_0_accepted_anchor_keys=1000,1001\n"
          "episode_1_requested_anchor_index_begin=20\n"
          "episode_1_requested_anchor_index_end=22\n"
          "episode_1_requested_source_key_begin=2000\n"
          "episode_1_requested_source_key_end=2002\n"
          "episode_1_accepted_cursor_kind=graph_anchor\n"
          "episode_1_accepted_cursor_scope=episode\n"
          "episode_1_accepted_batch_cursor_token=cursor_1\n"
          "episode_1_accepted_anchor_index_begin=20\n"
          "episode_1_accepted_anchor_index_end=22\n"
          "episode_1_accepted_anchor_keys=2000,2001\n"
          "mean_total_reward=1.5\n"
          "mean_total_log_growth=0.03\n"
          "mean_final_equity_base=103.0\n"
          "mean_max_drawdown=0.08\n"
          "mean_total_turnover=0.40\n"
          "mean_total_transaction_cost_base=0.015\n"
          "requested_notional_base=10.0\n"
          "executed_notional_base=9.8\n"
          "rejected_notional_base=0.0\n"
          "fill_ratio=0.98\n"
          "fee_cost_base=0.006\n"
          "spread_cost_base=0.005\n"
          "slippage_cost_base=0.004\n"
          "mean_target_weight_error_l1=0.02\n"
          "mean_target_weight_error_linf=0.01\n"
          "mean_projection_mae=0.012\n"
          "mean_projection_signed_bias=-0.002\n"
          "mean_projection_directional_accuracy=0.75\n"
          "mean_projection_interval_coverage=0.80\n"
          "artifact_evidence=true\n"
          "visibility_only=true\n"
          "replay_executor=false\n"
          "allocation_authority=false\n"
          "execution_authority=false\n"
          "readiness_authority=false\n"
          "quality_authority=false\n"
          "performance_authority=false\n"
          "market_readiness_authority=false\n"
          "deployment_authority=false\n"
          "checkpoint_selector=false\n"
          "coverage_authority=false\n"
          "leakage_authority=false\n"
          "contract_identity_authority=false\n");
  const auto replay_report_digest =
      exposure::replay_environment_report_digest_for_text(
          read_text(replay_artifact_dir / "runtime_replay_experiment.report"));
  append_text(replay_artifact_dir / "runtime_replay_experiments.index",
              "entry_0_report_digest=" + replay_report_digest + "\n");

  exposure::exposure_build_context_t train_context{};
  train_context.split_name = "train_core";
  train_context.split_role = exposure::exposure_split_role_t::train;
  train_context.split_policy_fingerprint = "split_policy_1";

  auto rep_fact =
      exposure::make_exposure_fact_from_job_dir(rep_dir, train_context);
  auto channel_mdn_fact =
      exposure::make_exposure_fact_from_job_dir(channel_mdn_dir, train_context);
  auto mdn_fact = channel_mdn_fact;
  check(channel_mdn_fact.runtime_result_fact_available &&
            channel_mdn_fact.runtime_checkpoint_io_fact_available &&
            channel_mdn_fact.runtime_health_measurement_fact_available,
        "channel MDN exposure fact records Runtime terminal fact availability");
  check(channel_mdn_fact.checkpoint_written &&
            channel_mdn_fact.model_state_mutated &&
            channel_mdn_fact.representation_checkpoint_loaded &&
            !channel_mdn_fact.mdn_checkpoint_loaded,
        "channel MDN exposure fact carries checkpoint I/O health fields");
  check(std::abs(channel_mdn_fact.max_grad_norm - 1200.0) < 1e-12 &&
            std::abs(channel_mdn_fact.grad_clip_norm - 5.0) < 1e-12 &&
            std::abs(channel_mdn_fact.sigma_min_valid - 0.11) < 1e-12 &&
            channel_mdn_fact.nonfinite_output_count == 0,
        "channel MDN exposure fact prefers Runtime terminal health metrics");
  auto scan =
      exposure::scan_exposure_ledger_from_runtime_root(root, train_context);
  check(scan.warnings.empty(), "runtime-root exposure scan has no warnings");
  check(scan.ledger.facts().size() == 3,
        "runtime-root exposure scan finds all job exposure facts");
  check(scan.ledger.node_facts().empty(),
        "runtime-root exposure scan does not derive node-scoped MDN facts");
  check(scan.ledger.representation_support_facts().size() == 4,
        "runtime-root exposure scan derives aggregate and node-indexed shared "
        "representation support facts");
  check(scan.ledger.source_receipt_facts().size() == 1,
        "runtime-root exposure scan derives structured source receipt facts");
  check(scan.ledger.source_analytics_facts().size() == 1,
        "runtime-root exposure scan derives visibility-only source analytics "
        "facts from source analytics sidecars");
  check(scan.ledger.target_transform_facts().size() == 1,
        "runtime-root exposure scan derives target transform contract facts "
        "from target transform sidecars");
  check(scan.ledger.forecast_baseline_facts().size() == 1,
        "runtime-root exposure scan derives forecast baseline evidence facts "
        "from forecast baseline sidecars");
  check(scan.ledger.forecast_eval_facts().size() == 1,
        "runtime-root exposure scan derives forecast evaluation artifact facts "
        "from forecast evaluation sidecars");
  const auto source_analytics_paths =
      exposure::source_analytics_fact_paths_for_job_dir(rep_dir);
  const auto source_analytics_numeric_paths =
      exposure::source_data_analytics_numeric_paths_for_job_dir(rep_dir);
  const auto source_analytics_symbolic_paths =
      exposure::source_data_analytics_symbolic_paths_for_job_dir(rep_dir);
  check(source_analytics_paths.size() == 4 &&
            has_filename(source_analytics_paths,
                         "lattice.source_analytics.fact") &&
            has_filename(source_analytics_paths, "source_analytics.fact") &&
            has_filename(source_analytics_paths,
                         "runtime.source_analytics.fact") &&
            has_filename(source_analytics_paths, "source.analytics.fact"),
        "source analytics producer contract accepts only the declared durable "
        "sidecar names");
  check(
      source_analytics_numeric_paths.size() == 6 &&
          has_filename(source_analytics_numeric_paths,
                       "data_analytics.v2.latest.lls") &&
          has_filename(source_analytics_numeric_paths,
                       "sequence_analytics.v2.latest.lls") &&
          has_filename(source_analytics_numeric_paths,
                       "embedding_sequence_analytics.v2.latest.lls") &&
          has_filename(source_analytics_numeric_paths,
                       "source_data_analytics.v2.latest.lls") &&
          has_filename(source_analytics_numeric_paths, "data_analytics.lls") &&
          has_filename(source_analytics_numeric_paths,
                       "source_data_analytics.lls"),
      "source analytics scanner-derived contract accepts the declared "
      "numeric source analytics payload names");
  check(source_analytics_symbolic_paths.size() == 6 &&
            has_filename(source_analytics_symbolic_paths,
                         "data_analytics.symbolic.v2.latest.lls") &&
            has_filename(source_analytics_symbolic_paths,
                         "sequence_analytics.symbolic.v2.latest.lls") &&
            has_filename(source_analytics_symbolic_paths,
                         "embedding_sequence_analytics.symbolic.v2.latest."
                         "lls") &&
            has_filename(source_analytics_symbolic_paths,
                         "source_data_analytics.symbolic.v2.latest.lls") &&
            has_filename(source_analytics_symbolic_paths,
                         "data_analytics.symbolic.lls") &&
            has_filename(source_analytics_symbolic_paths,
                         "source_data_analytics.symbolic.lls"),
        "source analytics scanner-derived contract accepts the declared "
        "symbolic source analytics payload names");
  const auto target_transform_paths =
      exposure::target_transform_fact_paths_for_job_dir(rep_dir);
  const auto forecast_baseline_paths =
      exposure::forecast_baseline_fact_paths_for_job_dir(rep_dir);
  const auto forecast_eval_paths =
      exposure::forecast_eval_fact_paths_for_job_dir(channel_mdn_dir);
  const auto observer_belief_paths =
      exposure::observer_belief_fact_paths_for_job_dir(channel_mdn_dir);
  const auto allocation_engine_paths =
      exposure::allocation_engine_fact_paths_for_job_dir(channel_mdn_dir);
  check(target_transform_paths.size() == 4 &&
            has_filename(target_transform_paths,
                         "lattice.target_transform.fact") &&
            has_filename(target_transform_paths, "target_transform.fact") &&
            has_filename(target_transform_paths,
                         "runtime.target_transform.fact") &&
            has_filename(target_transform_paths, "target.transform.fact"),
        "target transform producer contract accepts only the declared durable "
        "sidecar names");
  check(forecast_baseline_paths.size() == 8 &&
            has_filename(forecast_baseline_paths,
                         "lattice.forecast_baseline.fact") &&
            has_filename(forecast_baseline_paths, "forecast_baseline.fact") &&
            has_filename(forecast_baseline_paths,
                         "runtime.forecast_baseline.fact") &&
            has_filename(forecast_baseline_paths, "forecast.baseline.fact") &&
            has_filename(forecast_baseline_paths,
                         "lattice.forecast_baseline.previous_value.fact") &&
            has_filename(forecast_baseline_paths,
                         "lattice.forecast_baseline.zero_return.fact") &&
            has_filename(forecast_baseline_paths,
                         "lattice.forecast_baseline.moving_average.fact") &&
            has_filename(forecast_baseline_paths,
                         "lattice.forecast_baseline.last_valid_channel.fact"),
        "forecast baseline producer contract accepts the generic sidecars and "
        "the explicit deterministic baseline sidecars");
  check(forecast_eval_paths.size() == 4 &&
            has_filename(forecast_eval_paths, "lattice.forecast_eval.fact") &&
            has_filename(forecast_eval_paths, "forecast_eval.fact") &&
            has_filename(forecast_eval_paths, "runtime.forecast_eval.fact") &&
            has_filename(forecast_eval_paths, "forecast.eval.fact"),
        "forecast eval producer contract accepts only the declared durable "
        "sidecar names");
  check(
      observer_belief_paths.size() == 4 &&
          has_filename(observer_belief_paths, "lattice.observer_belief.fact") &&
          has_filename(observer_belief_paths, "observer_belief.fact") &&
          has_filename(observer_belief_paths, "runtime.observer_belief.fact") &&
          has_filename(observer_belief_paths, "observer.belief.fact"),
      "observer belief producer contract accepts only the declared durable "
      "sidecar names");
  check(allocation_engine_paths.size() == 4 &&
            has_filename(allocation_engine_paths,
                         "lattice.allocation_engine.fact") &&
            has_filename(allocation_engine_paths, "allocation_engine.fact") &&
            has_filename(allocation_engine_paths,
                         "runtime.allocation_engine.fact") &&
            has_filename(allocation_engine_paths, "allocation.engine.fact"),
        "allocation engine producer contract accepts only the declared durable "
        "sidecar names");

  const auto alias_fact_dir = root / "forecast_fact_aliases";
  write_text(alias_fact_dir / "target_transform.fact",
             "schema=kikijyeba.lattice.target_transform.v1\n"
             "feature_ids=ev_close\n"
             "forecast_horizon=2\n"
             "mode=return\n"
             "normalization=zscore.alias.v1\n"
             "inverse_transform=identity_return.v1\n"
             "target_units=return\n"
             "mask_policy_digest=mask_policy_alias\n"
             "support_surface_id=alias:h2\n"
             "target_surface_digest=target_surface_alias\n");
  write_text(alias_fact_dir / "lattice.forecast_baseline.zero_return.fact",
             "schema=kikijyeba.lattice.forecast_baseline.v1\n"
             "feature_ids=ev_close\n"
             "forecast_horizon=2\n"
             "target_transform_digest=target_transform_alias_digest\n"
             "evaluation_support_count=15\n"
             "valid_target_count=15\n"
             "baseline_metric_values=deferred_metrics\n");
  write_text(alias_fact_dir / "runtime.forecast_eval.fact",
             "schema=kikijyeba.lattice.forecast_eval.v1\n"
             "feature_ids=ev_close\n"
             "forecast_horizon=2\n"
             "evaluation_support_count=15\n"
             "valid_target_count=15\n"
             "mean_nll=1.00\n"
             "valid_target_count_per_horizon=15\n"
             "support_by_horizon=h2:15\n"
             "pit_histogram_summary=flat\n"
             "sigma_sanity=ok\n"
             "forecast_digest=forecast_alias_digest\n"
             "representation_checkpoint_digest=rep_alias_digest\n"
             "mdn_checkpoint_digest=mdn_alias_digest\n"
             "target_transform_digest=target_transform_alias_digest\n"
             "forecast_baseline_fact_digests=baseline_alias_digest\n");
  const auto alias_target_transforms =
      exposure::make_target_transform_facts_from_job_dir(alias_fact_dir,
                                                         rep_fact);
  const auto alias_forecast_baselines =
      exposure::make_forecast_baseline_facts_from_job_dir(alias_fact_dir,
                                                          rep_fact);
  const auto alias_forecast_evals =
      exposure::make_forecast_eval_facts_from_job_dir(alias_fact_dir,
                                                      channel_mdn_fact);
  check(alias_target_transforms.size() == 1 &&
            alias_target_transforms.front().target_feature_ids.size() == 1 &&
            alias_target_transforms.front().target_feature_ids.front() ==
                "ev_close" &&
            alias_target_transforms.front().horizon == 2 &&
            alias_target_transforms.front().target_mode == "return" &&
            alias_target_transforms.front().normalization_contract ==
                "zscore.alias.v1" &&
            alias_target_transforms.front().inverse_transform_contract ==
                "identity_return.v1" &&
            alias_target_transforms.front().units == "return" &&
            alias_target_transforms.front().target_mask_policy_digest ==
                "mask_policy_alias" &&
            alias_target_transforms.front().support_surface_identity ==
                "alias:h2" &&
            alias_target_transforms.front().support_surface_digest ==
                "target_surface_alias" &&
            exposure::target_transform_fact_issues(
                alias_target_transforms.front())
                .empty(),
        "target transform scanner accepts the declared writer aliases while "
        "requiring an interpretable transform contract");
  check(alias_forecast_baselines.size() == 1 &&
            alias_forecast_baselines.front().baseline_kind == "zero_return" &&
            alias_forecast_baselines.front().target_feature_ids.size() == 1 &&
            alias_forecast_baselines.front().horizon == 2 &&
            alias_forecast_baselines.front().support_count == 15 &&
            alias_forecast_baselines.front().valid_count == 15 &&
            alias_forecast_baselines.front().metric_status == "deferred_v1" &&
            exposure::forecast_baseline_fact_issues(
                alias_forecast_baselines.front())
                .empty(),
        "forecast baseline scanner accepts explicit baseline sidecars and "
        "normalizes deferred metric status without inventing metrics");
  check(
      alias_forecast_evals.size() == 1 &&
          alias_forecast_evals.front().target_feature_ids.size() == 1 &&
          alias_forecast_evals.front().horizon == 2 &&
          alias_forecast_evals.front().support_count == 15 &&
          alias_forecast_evals.front().valid_count == 15 &&
          alias_forecast_evals.front().valid_target_count_per_horizon.size() ==
              1 &&
          alias_forecast_evals.front().forecast_artifact_digest ==
              "forecast_alias_digest" &&
          alias_forecast_evals.front()
                  .evaluated_representation_checkpoint_digest ==
              "rep_alias_digest" &&
          alias_forecast_evals.front().evaluated_mdn_checkpoint_digest ==
              "mdn_alias_digest" &&
          alias_forecast_evals.front().baseline_fact_digests.size() == 1 &&
          exposure::forecast_eval_fact_issues(alias_forecast_evals.front())
              .empty(),
      "forecast eval scanner accepts runtime writer aliases while requiring "
      "checkpoint, transform, baseline, and horizon support bindings");
  check(scan.ledger.observer_belief_facts().size() == 1,
        "runtime-root exposure scan derives observer belief audit facts from "
        "observer belief sidecars");
  check(scan.ledger.allocation_engine_facts().size() == 1,
        "runtime-root exposure scan derives allocation engine audit facts from "
        "allocation engine sidecars");
  check(scan.ledger.replay_environment_facts().empty(),
        "runtime-root exposure scan leaves parked replay environment evidence "
        "parked by default");
  auto parked_environment_scan_options = exposure::exposure_scan_options_t{};
  parked_environment_scan_options.derive_replay_environment_facts = true;
  const auto parked_environment_scan =
      exposure::scan_exposure_ledger_from_runtime_root(
          root, train_context, parked_environment_scan_options);
  check(parked_environment_scan.ledger.replay_environment_facts().size() == 1,
        "explicit replay scan derives parked replay environment evidence from "
        "job-local replay indexes and reports");
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
      scan_representation_support_summary.exposure_fact_count == 3 &&
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
  const auto fact_family_registry = exposure::lattice_fact_family_registry();
  const auto fact_catalog_summary =
      exposure::summarize_lattice_fact_catalog(scan.ledger);
  const auto fact_integrity_summary =
      exposure::summarize_lattice_fact_integrity(scan.ledger);
  const auto is_core_proof_evidence_family =
      [](exposure::lattice_fact_family_t family) {
        return family == exposure::lattice_fact_family_t::exposure ||
               family == exposure::lattice_fact_family_t::checkpoint;
      };
  for (const auto &descriptor : fact_family_registry) {
    const std::string fact_schema = descriptor.fact_schema;
    const std::string summary_schema = descriptor.summary_schema;
    check(!std::string(descriptor.family_name).empty() &&
              !fact_schema.empty() && !summary_schema.empty() &&
              !std::string(descriptor.relation).empty() &&
              !std::string(descriptor.authority_model).empty() &&
              fact_schema.rfind("kikijyeba.lattice.", 0) == 0 &&
              summary_schema.rfind("kikijyeba.lattice.", 0) == 0 &&
              fact_schema.size() >= 3 &&
              fact_schema.substr(fact_schema.size() - 3) == ".v1" &&
              summary_schema.size() >= 3 &&
              summary_schema.substr(summary_schema.size() - 3) == ".v1",
          "every fact-family descriptor must declare stable v1 lattice "
          "schemas, names, relation, and authority model");
    check(!descriptor.target_kind && !descriptor.dispatchable &&
              !descriptor.runtime_executor && !descriptor.quality_authority &&
              !descriptor.performance_authority &&
              !descriptor.checkpoint_selector &&
              !descriptor.allocation_authority &&
              !descriptor.execution_authority &&
              !descriptor.market_readiness_authority &&
              !descriptor.deployment_authority && !descriptor.policy_gate &&
              !descriptor.target_dependency_authority &&
              !descriptor.runtime_wave_authority &&
              !descriptor.marshal_reachability &&
              !descriptor.checkpoint_source_authority &&
              !descriptor.plan_checkpoint_input_authority,
          "fact-family descriptors must not become target kinds, dispatch "
          "surfaces, policy gates, selectors, allocators, executors, or market "
          "decision authority");
    if (!is_core_proof_evidence_family(descriptor.family)) {
      check(!descriptor.readiness_authority && !descriptor.coverage_authority &&
                !descriptor.leakage_authority &&
                !descriptor.contract_identity_authority,
            "roadmap catalog families must not inherit core readiness, "
            "coverage, leakage, or contract-identity authority");
    }
  }
  check(
      fact_catalog_summary.quality_authority_family_count == 0 &&
          fact_catalog_summary.performance_authority_family_count == 0 &&
          fact_catalog_summary.checkpoint_selector_family_count == 0 &&
          fact_catalog_summary.allocation_authority_family_count == 0 &&
          fact_catalog_summary.execution_authority_family_count == 0 &&
          fact_catalog_summary.market_readiness_authority_family_count == 0 &&
          fact_catalog_summary.deployment_authority_family_count == 0 &&
          fact_catalog_summary.policy_gate_family_count == 0 &&
          fact_catalog_summary.target_dependency_authority_family_count == 0 &&
          fact_catalog_summary.runtime_wave_authority_family_count == 0 &&
          fact_catalog_summary.marshal_reachability_family_count == 0 &&
          fact_catalog_summary.checkpoint_source_authority_family_count == 0 &&
          fact_catalog_summary.plan_checkpoint_input_authority_family_count ==
              0 &&
          fact_catalog_summary.decision_authority_family_count == 0 &&
          fact_catalog_summary.decision_authority_clean,
      "fact catalog summary must report no decision-authority drift across "
      "quality, performance, selection, allocation, execution, policy, wave, "
      "marshal, checkpoint-source, or deployment surfaces");
  check(!scan.ledger.target_transform_facts().empty() &&
            !scan.ledger.forecast_baseline_facts().empty() &&
            !scan.ledger.forecast_eval_facts().empty() &&
            !scan.ledger.observer_belief_facts().empty() &&
            !scan.ledger.allocation_engine_facts().empty(),
        "mixed exposure fixture should include every proofable artifact fact "
        "family");
  const auto check_common_proofable_envelope =
      [&](const auto &fact, exposure::lattice_fact_family_t family,
          const std::string &digest, const std::string &label) {
        const auto envelope =
            exposure::make_lattice_fact_identity_envelope(fact, family, digest);
        check(envelope.schema ==
                      exposure::k_lattice_fact_identity_envelope_schema_v1 &&
                  envelope.fact_identity_contract_schema ==
                      exposure::k_lattice_fact_identity_contract_schema_v1 &&
                  envelope.fact_identity_contract_id ==
                      exposure::k_lattice_fact_identity_contract_id_v1 &&
                  envelope.schema_id == fact.schema &&
                  envelope.fact_family ==
                      exposure::lattice_fact_family_name(family) &&
                  envelope.fact_type == fact.fact_type &&
                  envelope.fact_id == digest &&
                  envelope.fact_digest == digest &&
                  envelope.protocol_id == fact.protocol_id &&
                  envelope.protocol_contract_fingerprint ==
                      fact.contract_fingerprint &&
                  envelope.graph_order_fingerprint ==
                      fact.graph_order_fingerprint &&
                  envelope.source_cursor_token == fact.source_cursor_token &&
                  envelope.split_name == fact.split_name &&
                  envelope.split_role ==
                      exposure::exposure_split_role_name(fact.split_role) &&
                  envelope.split_policy_fingerprint ==
                      fact.split_policy_fingerprint &&
                  envelope.anchor_range.has_value() &&
                  envelope.anchor_range->begin == fact.anchor_range.begin &&
                  envelope.anchor_range->end == fact.anchor_range.end &&
                  envelope.completed_anchor_range.has_value() &&
                  envelope.completed_anchor_range->begin ==
                      fact.completed_anchor_range.begin &&
                  envelope.completed_anchor_range->end ==
                      fact.completed_anchor_range.end &&
                  envelope.component_family_id ==
                      fact.target_component_family_id &&
                  envelope.component_assembly_fingerprint ==
                      fact.component_assembly_fingerprint &&
                  envelope.job_id == fact.job_id &&
                  envelope.wave_id == fact.wave_id &&
                  envelope.parent_exposure_fact_digests.size() == 1 &&
                  envelope.parent_exposure_fact_digests.front() ==
                      fact.parent_exposure_fact_digest &&
                  envelope.row_index_interval_authority &&
                  envelope.source_key_window_audit_only && envelope.read_only &&
                  !envelope.target_proof && !envelope.dispatchable &&
                  !envelope.runtime_executor &&
                  envelope.fact_families_are_not_target_kinds &&
                  !envelope.facts_used_for_target_satisfaction &&
                  !envelope.checkpoint_selected && !envelope.model_selector,
              label +
                  " identity envelope should preserve common catalog identity "
                  "without target, dispatch, selector, or model authority");
      };
  check_common_proofable_envelope(
      scan.ledger.target_transform_facts().front(),
      exposure::lattice_fact_family_t::target_transform,
      exposure::target_transform_fact_digest(
          scan.ledger.target_transform_facts().front()),
      "target-transform");
  check_common_proofable_envelope(
      scan.ledger.forecast_baseline_facts().front(),
      exposure::lattice_fact_family_t::forecast_baseline,
      exposure::forecast_baseline_fact_digest(
          scan.ledger.forecast_baseline_facts().front()),
      "forecast-baseline");
  check_common_proofable_envelope(
      scan.ledger.forecast_eval_facts().front(),
      exposure::lattice_fact_family_t::forecast_eval,
      exposure::forecast_eval_fact_digest(
          scan.ledger.forecast_eval_facts().front()),
      "forecast-eval");
  check_common_proofable_envelope(
      scan.ledger.observer_belief_facts().front(),
      exposure::lattice_fact_family_t::observer_belief,
      exposure::observer_belief_fact_digest(
          scan.ledger.observer_belief_facts().front()),
      "observer-belief");
  check_common_proofable_envelope(
      scan.ledger.allocation_engine_facts().front(),
      exposure::lattice_fact_family_t::allocation_engine,
      exposure::allocation_engine_fact_digest(
          scan.ledger.allocation_engine_facts().front()),
      "allocation-engine");
  check(!scan.ledger.forecast_eval_facts().empty(),
        "mixed exposure fixture should include forecast-eval catalog facts");
  const auto &first_forecast_eval = scan.ledger.forecast_eval_facts().front();
  const auto first_forecast_eval_digest =
      exposure::forecast_eval_fact_digest(first_forecast_eval);
  const auto forecast_eval_identity_envelope =
      exposure::make_lattice_fact_identity_envelope(
          first_forecast_eval, exposure::lattice_fact_family_t::forecast_eval,
          first_forecast_eval_digest);
  check(
      forecast_eval_identity_envelope.schema ==
              exposure::k_lattice_fact_identity_envelope_schema_v1 &&
          forecast_eval_identity_envelope.fact_identity_contract_schema ==
              exposure::k_lattice_fact_identity_contract_schema_v1 &&
          forecast_eval_identity_envelope.fact_identity_contract_id ==
              exposure::k_lattice_fact_identity_contract_id_v1 &&
          forecast_eval_identity_envelope.fact_family == "forecast_eval" &&
          forecast_eval_identity_envelope.fact_type == "forecast_eval" &&
          forecast_eval_identity_envelope.fact_digest ==
              first_forecast_eval_digest &&
          forecast_eval_identity_envelope.protocol_id ==
              first_forecast_eval.protocol_id &&
          forecast_eval_identity_envelope.protocol_contract_fingerprint ==
              first_forecast_eval.contract_fingerprint &&
          forecast_eval_identity_envelope.source_cursor_token ==
              first_forecast_eval.source_cursor_token &&
          forecast_eval_identity_envelope.split_name ==
              first_forecast_eval.split_name &&
          forecast_eval_identity_envelope.anchor_range.has_value() &&
          forecast_eval_identity_envelope.completed_anchor_range.has_value() &&
          forecast_eval_identity_envelope.parent_exposure_fact_digests.size() ==
              1 &&
          forecast_eval_identity_envelope.parent_exposure_fact_digests[0] ==
              first_forecast_eval.parent_exposure_fact_digest &&
          forecast_eval_identity_envelope.parent_forecast_artifact_digests
                  .size() == 1 &&
          forecast_eval_identity_envelope.parent_forecast_artifact_digests[0] ==
              first_forecast_eval.forecast_artifact_digest &&
          forecast_eval_identity_envelope.parent_fact_digests.size() >= 3 &&
          forecast_eval_identity_envelope.support_count.has_value() &&
          *forecast_eval_identity_envelope.support_count ==
              first_forecast_eval.support_count &&
          forecast_eval_identity_envelope.valid_count.has_value() &&
          *forecast_eval_identity_envelope.valid_count ==
              first_forecast_eval.valid_count &&
          forecast_eval_identity_envelope.row_index_interval_authority &&
          forecast_eval_identity_envelope.source_key_window_audit_only &&
          forecast_eval_identity_envelope.read_only &&
          !forecast_eval_identity_envelope.target_proof &&
          !forecast_eval_identity_envelope.dispatchable &&
          !forecast_eval_identity_envelope.runtime_executor &&
          forecast_eval_identity_envelope.fact_families_are_not_target_kinds &&
          !forecast_eval_identity_envelope.facts_used_for_target_satisfaction &&
          !forecast_eval_identity_envelope.checkpoint_selected &&
          !forecast_eval_identity_envelope.model_selector,
      "forecast-eval fact identity envelope should be a catalog-owned "
      "read-only projection with lineage and no decision authority");
  const auto source_analytics_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(),
      [](const auto &summary) { return summary.family == "source_analytics"; });
  const auto target_transform_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(),
      [](const auto &summary) { return summary.family == "target_transform"; });
  const auto forecast_baseline_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(), [](const auto &summary) {
        return summary.family == "forecast_baseline";
      });
  const auto forecast_eval_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(),
      [](const auto &summary) { return summary.family == "forecast_eval"; });
  const auto observer_belief_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(),
      [](const auto &summary) { return summary.family == "observer_belief"; });
  const auto allocation_engine_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(), [](const auto &summary) {
        return summary.family == "allocation_engine";
      });
  const auto replay_environment_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(), [](const auto &summary) {
        return summary.family == "replay_environment";
      });
  const auto policy_training_family = std::find_if(
      fact_catalog_summary.families.begin(),
      fact_catalog_summary.families.end(),
      [](const auto &summary) { return summary.family == "policy_training"; });
  check(
      fact_family_registry.size() == 14 &&
          exposure::parse_lattice_fact_family("source_analytics").has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.source_analytics.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("target_transform").has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.target_transform.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("forecast_baseline")
              .has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.forecast_baseline.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("forecast_eval").has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.forecast_eval.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("observer_belief").has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.observer_belief.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("allocation_engine")
              .has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.allocation_engine.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("replay_environment")
              .has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.replay_environment.v1")
              .has_value() &&
          exposure::parse_lattice_fact_family("policy_training").has_value() &&
          exposure::parse_lattice_fact_family(
              "kikijyeba.lattice.policy_training.v1")
              .has_value() &&
          fact_catalog_summary.family_count == 14 &&
          fact_catalog_summary.target_kind_family_count == 0 &&
          fact_catalog_summary.dispatchable_family_count == 0 &&
          fact_catalog_summary.runtime_executor_family_count == 0 &&
          fact_catalog_summary.catalog_is_read_only &&
          fact_catalog_summary.fact_families_are_not_target_kinds &&
          fact_catalog_summary.non_dispatchable_families_not_reachable &&
          fact_catalog_summary.issues.empty() &&
          source_analytics_family != fact_catalog_summary.families.end() &&
          source_analytics_family->fact_count == 1 &&
          source_analytics_family->parent_exposure_bound_count == 1 &&
          !source_analytics_family->target_kind &&
          !source_analytics_family->dispatchable &&
          !source_analytics_family->runtime_executor &&
          !source_analytics_family->readiness_authority &&
          !source_analytics_family->quality_authority &&
          !source_analytics_family->performance_authority &&
          !source_analytics_family->coverage_authority &&
          !source_analytics_family->leakage_authority &&
          !source_analytics_family->contract_identity_authority &&
          target_transform_family != fact_catalog_summary.families.end() &&
          target_transform_family->fact_count == 1 &&
          target_transform_family->parent_exposure_bound_count == 1 &&
          !target_transform_family->target_kind &&
          !target_transform_family->dispatchable &&
          !target_transform_family->runtime_executor &&
          !target_transform_family->readiness_authority &&
          !target_transform_family->coverage_authority &&
          !target_transform_family->leakage_authority &&
          !target_transform_family->contract_identity_authority &&
          forecast_baseline_family != fact_catalog_summary.families.end() &&
          forecast_baseline_family->fact_count == 1 &&
          forecast_baseline_family->parent_exposure_bound_count == 1 &&
          !forecast_baseline_family->target_kind &&
          !forecast_baseline_family->dispatchable &&
          !forecast_baseline_family->runtime_executor &&
          !forecast_baseline_family->readiness_authority &&
          !forecast_baseline_family->coverage_authority &&
          !forecast_baseline_family->leakage_authority &&
          !forecast_baseline_family->contract_identity_authority &&
          forecast_eval_family != fact_catalog_summary.families.end() &&
          forecast_eval_family->fact_count == 1 &&
          forecast_eval_family->parent_exposure_bound_count == 1 &&
          !forecast_eval_family->target_kind &&
          !forecast_eval_family->dispatchable &&
          !forecast_eval_family->runtime_executor &&
          !forecast_eval_family->readiness_authority &&
          !forecast_eval_family->coverage_authority &&
          !forecast_eval_family->leakage_authority &&
          !forecast_eval_family->contract_identity_authority &&
          observer_belief_family != fact_catalog_summary.families.end() &&
          observer_belief_family->fact_count == 1 &&
          observer_belief_family->parent_exposure_bound_count == 1 &&
          !observer_belief_family->target_kind &&
          !observer_belief_family->dispatchable &&
          !observer_belief_family->runtime_executor &&
          !observer_belief_family->readiness_authority &&
          !observer_belief_family->coverage_authority &&
          !observer_belief_family->leakage_authority &&
          !observer_belief_family->contract_identity_authority &&
          allocation_engine_family != fact_catalog_summary.families.end() &&
          allocation_engine_family->fact_count == 1 &&
          allocation_engine_family->parent_exposure_bound_count == 1 &&
          !allocation_engine_family->target_kind &&
          !allocation_engine_family->dispatchable &&
          !allocation_engine_family->runtime_executor &&
          !allocation_engine_family->readiness_authority &&
          !allocation_engine_family->coverage_authority &&
          !allocation_engine_family->leakage_authority &&
          !allocation_engine_family->contract_identity_authority &&
          replay_environment_family != fact_catalog_summary.families.end() &&
          replay_environment_family->fact_count == 0 &&
          replay_environment_family->parent_exposure_bound_count == 0 &&
          !replay_environment_family->target_kind &&
          !replay_environment_family->dispatchable &&
          !replay_environment_family->runtime_executor &&
          !replay_environment_family->readiness_authority &&
          !replay_environment_family->coverage_authority &&
          !replay_environment_family->leakage_authority &&
          !replay_environment_family->contract_identity_authority &&
          policy_training_family != fact_catalog_summary.families.end() &&
          policy_training_family->fact_count == 0 &&
          policy_training_family->parent_exposure_bound_count == 0 &&
          !policy_training_family->target_kind &&
          !policy_training_family->dispatchable &&
          !policy_training_family->runtime_executor &&
          !policy_training_family->readiness_authority &&
          !policy_training_family->coverage_authority &&
          !policy_training_family->leakage_authority &&
          !policy_training_family->contract_identity_authority,
      "fact catalog lists source analytics, target transforms, and forecast "
      "baselines/evals plus observer/allocation evidence as non-target, "
      "non-dispatchable fact families while replay remains parked by default");
  check(fact_integrity_summary.schema ==
                "kikijyeba.lattice.fact_integrity_summary.v1" &&
            fact_integrity_summary.inspected_family_count == 14 &&
            fact_integrity_summary.reported_family_count == 4 &&
            fact_integrity_summary.relation_declared_count == 7 &&
            fact_integrity_summary.relation_bound_count == 1 &&
            fact_integrity_summary.unresolved_relation_count == 6 &&
            fact_integrity_summary.identity_mismatch_count == 0 &&
            fact_integrity_summary.digest_mismatch_count == 0 &&
            fact_integrity_summary.warning_count == 7 &&
            !fact_integrity_summary.relation_integrity_clean &&
            fact_integrity_summary.read_only &&
            !fact_integrity_summary.target_proof &&
            !fact_integrity_summary.dispatchable &&
            !fact_integrity_summary.runtime_executor &&
            fact_integrity_summary.families_with_unresolved_relation.size() ==
                4 &&
            std::find(fact_integrity_summary.integrity_flags.begin(),
                      fact_integrity_summary.integrity_flags.end(),
                      "unresolved_relation") !=
                fact_integrity_summary.integrity_flags.end() &&
            std::find(fact_integrity_summary.issue_codes.begin(),
                      fact_integrity_summary.issue_codes.end(),
                      "channel_mdn_job:baseline_fact_digest_not_found") !=
                fact_integrity_summary.issue_codes.end(),
        "fact integrity rollup distinguishes declared relation digests from "
        "resolved identity-compatible bindings across fact families");
  const auto forecast_eval_only_integrity =
      exposure::summarize_lattice_fact_integrity(
          scan.ledger, {exposure::lattice_fact_family_t::forecast_eval});
  check(forecast_eval_only_integrity.inspected_family_count == 1 &&
            forecast_eval_only_integrity.reported_family_count == 1 &&
            forecast_eval_only_integrity.relation_declared_count == 3 &&
            forecast_eval_only_integrity.relation_bound_count == 0 &&
            forecast_eval_only_integrity.unresolved_relation_count == 3 &&
            forecast_eval_only_integrity.warning_count == 3 &&
            forecast_eval_only_integrity.families_with_unresolved_relation
                    .size() == 1 &&
            forecast_eval_only_integrity.families_with_unresolved_relation[0] ==
                "forecast_eval",
        "selected-family fact integrity rollup scopes unresolved lineage to "
        "the requested family");
  const auto runtime_index_cache =
      exposure::build_runtime_index_cache(root, train_context);
  const auto expected_runtime_index_rows =
      runtime_index_cache.fact_count +
      runtime_index_cache.node_exposure_fact_count +
      runtime_index_cache.checkpoint_fact_count +
      runtime_index_cache.source_receipt_fact_count +
      runtime_index_cache.source_analytics_fact_count +
      runtime_index_cache.target_transform_fact_count +
      runtime_index_cache.forecast_baseline_fact_count +
      runtime_index_cache.forecast_eval_fact_count +
      runtime_index_cache.observer_belief_fact_count +
      runtime_index_cache.allocation_engine_fact_count +
      runtime_index_cache.replay_environment_fact_count +
      runtime_index_cache.policy_training_fact_count +
      runtime_index_cache.selection_signal_fact_count +
      runtime_index_cache.representation_support_fact_count;
  check(runtime_index_cache.schema ==
                "kikijyeba.lattice.runtime_index_cache.v1" &&
            runtime_index_cache.source_of_truth ==
                "runtime_files_and_sidecars" &&
            !runtime_index_cache.db_writes_evidence &&
            !runtime_index_cache.runtime_executor &&
            runtime_index_cache.rebuildable_from_runtime_files &&
            runtime_index_cache.fact_count == 3 &&
            runtime_index_cache.node_exposure_fact_count == 0 &&
            runtime_index_cache.source_receipt_fact_count == 1 &&
            runtime_index_cache.source_analytics_fact_count == 1 &&
            runtime_index_cache.target_transform_fact_count == 1 &&
            runtime_index_cache.forecast_baseline_fact_count == 1 &&
            runtime_index_cache.forecast_eval_fact_count == 1 &&
            runtime_index_cache.observer_belief_fact_count == 1 &&
            runtime_index_cache.allocation_engine_fact_count == 1 &&
            runtime_index_cache.replay_environment_fact_count == 0 &&
            runtime_index_cache.policy_training_fact_count == 0 &&
            runtime_index_cache.representation_support_fact_count == 4 &&
            static_cast<std::int64_t>(runtime_index_cache.rows.size()) ==
                expected_runtime_index_rows &&
            !runtime_index_cache.runtime_metadata_digest.empty() &&
            !runtime_index_cache.watched_file_metadata_digest.empty() &&
            !runtime_index_cache.row_set_digest.empty() &&
            !runtime_index_cache.relation_counts.empty(),
        "runtime index cache is a rebuildable read model over runtime facts, "
        "not evidence or executor authority");
  const auto index_path = root / "indexes" / "lattice_runtime_index.v1.lls";
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
  write_text(rep_dir / "watched.report",
             "runtime read-model metadata changed\n");
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

  const auto &policy_parent_forecast_eval =
      parked_environment_scan.ledger.forecast_eval_facts().front();
  const auto &policy_parent_observer_belief =
      parked_environment_scan.ledger.observer_belief_facts().front();
  const auto &policy_parent_allocation_engine =
      parked_environment_scan.ledger.allocation_engine_facts().front();
  const auto &policy_parent_replay_environment =
      parked_environment_scan.ledger.replay_environment_facts().front();
  std::ostringstream policy_training_sidecar;
  policy_training_sidecar
      << "schema=kikijyeba.lattice.policy_training.v1\n"
      << "policy_id=wikimyei.policy.rl.ppo_portfolio.v0\n"
      << "policy_kind=ppo_policy_adapter\n"
      << "policy_architecture_digest=policy_architecture_1\n"
      << "training_config_digest=policy_training_config_1\n"
      << "training_range_digest=train_range_digest_1\n"
      << "validation_range_digest=validation_range_digest_1\n"
      << "test_range_digest=test_range_digest_1\n"
      << "environment_contract_id=kikijyeba.environment.replay.v1\n"
      << "observation_schema_digest=observation_schema_1\n"
      << "action_schema_digest="
         "kikijyeba.environment.action.target_weights.v1\n"
      << "reward_contract_digest=reward_contract_1\n"
      << "execution_profile_digest="
      << policy_parent_replay_environment.execution_profile_digest << "\n"
      << "training_schedule_mode=causal_walk_forward_training.v1\n"
      << "causal_schedule_schema_id="
         "kikijyeba.runtime.policy_training_causal_schedule.v1\n"
      << "causal_schedule_digest=causal_schedule_digest_1\n"
      << "causal_schedule_cursor_key_kind=numeric_anchor_index\n"
      << "causal_schedule_no_future_snapshot_use_source="
         "derived_from_artifact_fit_use_ledgers\n"
      << "normalization_fit_range_digest=train_range_digest_1\n"
      << "replay_buffer_source_range_digest=train_range_digest_1\n"
      << "early_stopping_policy_digest=early_stopping_validation_1\n"
      << "hyperparameter_selection_policy_digest="
         "hyperparameter_selection_validation_1\n"
      << "selector_split=validation\n"
      << "selector_policy_digest=selector_policy_1\n"
      << "parent_checkpoint_digest=sdu_seed_checkpoint_1\n"
      << "checkpoint_digest=ppo_checkpoint_1\n"
      << "parent_forecast_eval_fact_digest="
      << exposure::forecast_eval_fact_digest(policy_parent_forecast_eval)
      << "\n"
      << "parent_observer_belief_fact_digest="
      << exposure::observer_belief_fact_digest(policy_parent_observer_belief)
      << "\n"
      << "parent_allocation_engine_fact_digest="
      << exposure::allocation_engine_fact_digest(
             policy_parent_allocation_engine)
      << "\n"
      << "parent_replay_environment_fact_digest="
      << exposure::replay_environment_fact_digest(
             policy_parent_replay_environment)
      << "\n"
      << "random_seed=0\n"
      << "training_range_disjoint_validation=true\n"
      << "training_range_disjoint_test=true\n"
      << "validation_range_disjoint_test=true\n"
      << "normalization_fit_training_only=true\n"
      << "replay_buffer_training_only=true\n"
      << "reward_baseline_training_only=true\n"
      << "early_stopping_uses_validation_only=true\n"
      << "hyperparameter_selection_uses_validation_only=true\n"
      << "test_sealed_until_final_report=true\n"
      << "test_first_access_after_selection=true\n"
      << "runtime_job_kind_bound=true\n"
      << "policy_checkpoint_written=true\n"
      << "causal_schedule_readiness_eligible=true\n"
      << "causal_schedule_no_future_snapshot_use=true\n"
      << "offline_full_window_research=false\n"
      << "artifact_evidence=true\n"
      << "visibility_only=true\n"
      << "runtime_trainer=false\n"
      << "policy_training_authority=false\n"
      << "policy_selector=false\n";
  write_text(channel_mdn_dir / "lattice.policy_training.fact",
             policy_training_sidecar.str());
  const auto policy_training_scan =
      exposure::scan_exposure_ledger_from_runtime_root(
          root, train_context, parked_environment_scan_options);
  check(policy_training_scan.ledger.policy_training_facts().size() == 1,
        "explicit replay scan derives policy-training artifact evidence when "
        "a policy-training sidecar is present");
  const auto &policy_training_fact =
      policy_training_scan.ledger.policy_training_facts().front();
  const auto policy_training_summary = exposure::summarize_policy_trainings(
      policy_training_scan.ledger.facts(),
      policy_training_scan.ledger.policy_training_facts(),
      policy_training_scan.ledger.forecast_eval_facts(),
      policy_training_scan.ledger.observer_belief_facts(),
      policy_training_scan.ledger.allocation_engine_facts(),
      policy_training_scan.ledger.replay_environment_facts());
  check(
      policy_training_fact.random_seed == 0 &&
          policy_training_fact.random_seed_bound &&
          policy_training_fact.training_range_disjoint_validation &&
          policy_training_fact.training_range_disjoint_test &&
          policy_training_fact.validation_range_disjoint_test &&
          policy_training_fact.normalization_fit_training_only &&
          policy_training_fact.replay_buffer_training_only &&
          policy_training_fact.reward_baseline_training_only &&
          policy_training_fact.early_stopping_uses_validation_only &&
          policy_training_fact.hyperparameter_selection_uses_validation_only &&
          policy_training_fact.test_sealed_until_final_report &&
          policy_training_fact.test_first_access_after_selection &&
          policy_training_fact.runtime_job_kind_bound &&
          policy_training_fact.policy_checkpoint_written &&
          policy_training_fact.training_schedule_mode ==
              "causal_walk_forward_training.v1" &&
          !policy_training_fact.causal_schedule_digest.empty() &&
          policy_training_fact.causal_schedule_cursor_key_kind ==
              "numeric_anchor_index" &&
          policy_training_fact.causal_schedule_no_future_snapshot_use_source ==
              "derived_from_artifact_fit_use_ledgers" &&
          policy_training_fact.causal_schedule_readiness_eligible &&
          policy_training_fact.causal_schedule_no_future_snapshot_use &&
          !policy_training_fact.offline_full_window_research &&
          exposure::policy_training_fact_issues(policy_training_fact).empty() &&
          policy_training_summary.policy_training_fact_count == 1 &&
          policy_training_summary.policy_id_bound_count == 1 &&
          policy_training_summary.training_config_digest_bound_count == 1 &&
          policy_training_summary.training_range_digest_bound_count == 1 &&
          policy_training_summary.validation_range_digest_bound_count == 1 &&
          policy_training_summary.test_range_digest_bound_count == 1 &&
          policy_training_summary.execution_profile_digest_bound_count == 1 &&
          policy_training_summary.causal_schedule_digest_bound_count == 1 &&
          policy_training_summary.causal_schedule_ready_count == 1 &&
          policy_training_summary.causal_schedule_derived_source_count == 1 &&
          policy_training_summary.no_future_snapshot_use_count == 1 &&
          policy_training_summary.offline_full_window_research_count == 0 &&
          policy_training_summary.random_seed_bound_count == 1 &&
          policy_training_summary.forecast_eval_declared_count == 1 &&
          policy_training_summary.forecast_eval_bound_count == 1 &&
          policy_training_summary.observer_belief_declared_count == 1 &&
          policy_training_summary.observer_belief_bound_count == 1 &&
          policy_training_summary.allocation_engine_declared_count == 1 &&
          policy_training_summary.allocation_engine_bound_count == 1 &&
          policy_training_summary.replay_environment_declared_count == 1 &&
          policy_training_summary.replay_environment_bound_count == 1 &&
          policy_training_summary.disjoint_range_contract_count == 1 &&
          policy_training_summary.training_only_normalization_count == 1 &&
          policy_training_summary.training_only_replay_buffer_count == 1 &&
          policy_training_summary.training_only_reward_baseline_count == 1 &&
          policy_training_summary.validation_selector_policy_count == 1 &&
          policy_training_summary.sealed_test_count == 1 &&
          policy_training_summary.runtime_job_kind_bound_count == 1 &&
          policy_training_summary.policy_checkpoint_written_count == 1 &&
          policy_training_summary.warning_count == 0 &&
          policy_training_summary.issues.empty() &&
          policy_training_summary.artifact_evidence &&
          policy_training_summary.visibility_only &&
          !policy_training_summary.runtime_trainer &&
          !policy_training_summary.policy_training_authority &&
          !policy_training_summary.policy_selector,
      "policy-training artifact facts bind checkpoint lineage, split "
      "separation, training-only normalization/replay-buffer evidence, "
      "validation-only selector policy, sealed-test evidence, Runtime job "
      "kind, and parent replay evidence without becoming a trainer or "
      "policy selector");

  auto policy_training_future_snapshot = policy_training_fact;
  policy_training_future_snapshot.causal_schedule_no_future_snapshot_use =
      false;
  check(contains_text(exposure::policy_training_fact_issues(
                          policy_training_future_snapshot),
                      "causal_schedule_future_snapshot_use_not_proven"),
        "policy-training facts reject schedule evidence that does not prove "
        "no future snapshot use");

  auto policy_training_asserted_source = policy_training_fact;
  policy_training_asserted_source
      .causal_schedule_no_future_snapshot_use_source = "asserted_by_caller";
  check(contains_text(exposure::policy_training_fact_issues(
                          policy_training_asserted_source),
                      "unsupported_no_future_snapshot_use_source"),
        "policy-training facts reject caller-asserted no-future snapshot "
        "source");

  auto policy_training_opaque_cursor = policy_training_fact;
  policy_training_opaque_cursor.causal_schedule_cursor_key_kind =
      "opaque_unsortable";
  check(contains_text(exposure::policy_training_fact_issues(
                          policy_training_opaque_cursor),
                      "opaque_or_unsupported_causal_schedule_cursor_key_kind"),
        "policy-training facts reject opaque cursor key ordering");

  const auto scan_receipt_summary = exposure::summarize_source_receipts(
      scan.ledger.facts(), scan.ledger.source_receipt_facts());
  check(scan_receipt_summary.exposure_fact_count == 3 &&
            scan_receipt_summary.exposure_facts_with_receipts == 1 &&
            scan_receipt_summary.exposure_facts_missing_receipts == 2 &&
            scan_receipt_summary.source_receipt_fact_count == 1 &&
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
  const auto scan_source_analytics_summary =
      exposure::summarize_source_analytics(
          scan.ledger.facts(), scan.ledger.source_analytics_facts());
  const auto &rep_source_analytics =
      scan.ledger.source_analytics_facts().front();
  check(
      rep_source_analytics.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(rep_fact) &&
          rep_source_analytics.protocol_id == rep_fact.protocol_id &&
          rep_source_analytics.graph_order_fingerprint ==
              rep_fact.graph_order_fingerprint &&
          rep_source_analytics.source_cursor_token ==
              rep_fact.source_cursor_token &&
          rep_source_analytics.source_receipt_fact_count == 1 &&
          std::abs(rep_source_analytics.entropy - 1.25) < 1e-12 &&
          std::abs(rep_source_analytics.entropy_rate - 0.50) < 1e-12 &&
          std::abs(rep_source_analytics.information_density - 0.75) < 1e-12 &&
          std::abs(rep_source_analytics.compression_ratio - 2.50) < 1e-12 &&
          std::abs(rep_source_analytics.power_spectrum_entropy - 0.33) <
              1e-12 &&
          std::abs(rep_source_analytics.source_volatility - 0.14) < 1e-12 &&
          std::abs(rep_source_analytics.feature_variance - 0.04) < 1e-12 &&
          std::abs(rep_source_analytics.sample_validity_fraction - 0.98) <
              1e-12 &&
          std::abs(rep_source_analytics.missingness_fraction - 0.02) < 1e-12 &&
          rep_source_analytics.duplicate_sample_count == 3 &&
          rep_source_analytics.source_health_level == "warn" &&
          rep_source_analytics.visibility_only &&
          !rep_source_analytics.readiness_authority &&
          !rep_source_analytics.coverage_authority &&
          !rep_source_analytics.leakage_authority &&
          !rep_source_analytics.contract_identity_authority &&
          exposure::source_analytics_fact_digest(rep_source_analytics).size() ==
              16,
      "source analytics facts bind source health payloads to exposure identity "
      "without readiness, coverage, leakage, or contract authority");
  check(
      scan_source_analytics_summary.schema ==
              "kikijyeba.lattice.source_analytics_summary.v1" &&
          scan_source_analytics_summary.exposure_fact_count == 3 &&
          scan_source_analytics_summary.source_analytics_fact_count == 1 &&
          scan_source_analytics_summary.parent_exposure_fact_count == 1 &&
          scan_source_analytics_summary.source_cursor_bound_count == 1 &&
          scan_source_analytics_summary.graph_order_bound_count == 1 &&
          scan_source_analytics_summary.source_receipt_bound_count == 1 &&
          scan_source_analytics_summary.duplicate_sample_count_total == 3 &&
          scan_source_analytics_summary.train_source_analytics_fact_count ==
              1 &&
          scan_source_analytics_summary
                  .validation_source_analytics_fact_count == 0 &&
          scan_source_analytics_summary.source_regime_pair_count == 0 &&
          scan_source_analytics_summary.source_regime_shift_warning_count ==
              0 &&
          scan_source_analytics_summary.entropy.count == 1 &&
          std::abs(scan_source_analytics_summary.entropy.mean - 1.25) < 1e-12 &&
          scan_source_analytics_summary.sample_validity_fraction.count == 1 &&
          std::abs(scan_source_analytics_summary.sample_validity_fraction.mean -
                   0.98) < 1e-12 &&
          scan_source_analytics_summary.missingness_fraction.count == 1 &&
          std::abs(scan_source_analytics_summary.missingness_fraction.mean -
                   0.02) < 1e-12 &&
          scan_source_analytics_summary.source_volatility.count == 1 &&
          std::abs(scan_source_analytics_summary.source_volatility.mean -
                   0.14) < 1e-12 &&
          scan_source_analytics_summary.feature_variance.count == 1 &&
          std::abs(scan_source_analytics_summary.feature_variance.mean - 0.04) <
              1e-12 &&
          scan_source_analytics_summary.visibility_only &&
          !scan_source_analytics_summary.readiness_authority &&
          !scan_source_analytics_summary.coverage_authority &&
          !scan_source_analytics_summary.leakage_authority &&
          !scan_source_analytics_summary.contract_identity_authority &&
          scan_source_analytics_summary.row_index_authority_preserved &&
          scan_source_analytics_summary.warning_count == 0 &&
          scan_source_analytics_summary.issues.empty(),
      "source analytics summary is a read-only visibility surface and keeps "
      "row-index intervals as coverage/leakage authority");
  const auto source_alias_dir = root / "source_analytics_aliases";
  write_text(source_alias_dir / "source_analytics.fact",
             "schema=kikijyeba.lattice.source_analytics.v1\n"
             "source_entropy=4.50\n"
             "source_entropy_rate=1.10\n"
             "source_information_density=0.70\n"
             "source_compression_ratio=2.20\n"
             "source_power_spectrum_entropy=0.40\n"
             "volatility_mean=0.20\n"
             "variance_mean=0.03\n"
             "sample_valid_fraction=0.88\n"
             "missing_fraction=0.12\n"
             "source_duplicate_sample_count=2\n"
             "source_receipts=edge=BTC/USD@fixture\n"
             "receipt_fact_count=5\n"
             "health_level=warn\n"
             "source_visibility_only=true\n");
  const auto alias_source_analytics =
      exposure::make_source_analytics_facts_from_job_dir(source_alias_dir,
                                                         rep_fact);
  check(
      alias_source_analytics.size() == 1 &&
          alias_source_analytics.front().parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(rep_fact) &&
          alias_source_analytics.front().protocol_id == rep_fact.protocol_id &&
          alias_source_analytics.front().source_cursor_token ==
              rep_fact.source_cursor_token &&
          alias_source_analytics.front().source_receipt_fact_count == 5 &&
          std::abs(alias_source_analytics.front().entropy - 4.50) < 1e-12 &&
          std::abs(alias_source_analytics.front().entropy_rate - 1.10) <
              1e-12 &&
          std::abs(alias_source_analytics.front().information_density - 0.70) <
              1e-12 &&
          std::abs(alias_source_analytics.front().compression_ratio - 2.20) <
              1e-12 &&
          std::abs(alias_source_analytics.front().power_spectrum_entropy -
                   0.40) < 1e-12 &&
          std::abs(alias_source_analytics.front().source_volatility - 0.20) <
              1e-12 &&
          std::abs(alias_source_analytics.front().feature_variance - 0.03) <
              1e-12 &&
          std::abs(alias_source_analytics.front().sample_validity_fraction -
                   0.88) < 1e-12 &&
          std::abs(alias_source_analytics.front().missingness_fraction - 0.12) <
              1e-12 &&
          alias_source_analytics.front().duplicate_sample_count == 2 &&
          alias_source_analytics.front().source_health_level == "warn" &&
          alias_source_analytics.front().visibility_only &&
          !alias_source_analytics.front().readiness_authority &&
          !alias_source_analytics.front().coverage_authority &&
          !alias_source_analytics.front().leakage_authority &&
          !alias_source_analytics.front().contract_identity_authority &&
          exposure::source_analytics_fact_issues(alias_source_analytics.front())
              .empty(),
      "source analytics scanner accepts declared writer aliases while "
      "keeping the fact warning-only and parent-bound");
  auto validation_source_analytics = rep_source_analytics;
  validation_source_analytics.parent_exposure_fact_digest =
      "validation_source_parent_digest";
  validation_source_analytics.job_id = "validation_source_analytics_job";
  validation_source_analytics.split_name = "validation_holdout";
  validation_source_analytics.split_role =
      exposure::exposure_split_role_t::validation;
  validation_source_analytics.anchor_range = {.begin = 200, .end = 240};
  validation_source_analytics.completed_anchor_range =
      validation_source_analytics.anchor_range;
  validation_source_analytics.entropy = 2.05;
  validation_source_analytics.entropy_rate = 0.78;
  validation_source_analytics.information_density = 0.55;
  validation_source_analytics.compression_ratio = 3.20;
  validation_source_analytics.power_spectrum_entropy = 0.56;
  validation_source_analytics.source_volatility = 0.35;
  validation_source_analytics.feature_variance = 0.12;
  validation_source_analytics.sample_validity_fraction = 0.82;
  validation_source_analytics.missingness_fraction = 0.20;
  validation_source_analytics.duplicate_sample_count = 7;
  const auto source_regime_summary = exposure::summarize_source_analytics(
      {rep_fact}, {rep_source_analytics, validation_source_analytics});
  check(
      source_regime_summary.train_source_analytics_fact_count == 1 &&
          source_regime_summary.validation_source_analytics_fact_count == 1 &&
          source_regime_summary.source_regime_pair_count == 1 &&
          source_regime_summary.missingness_fraction_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .missingness_fraction_train_validation_delta.mean -
                   0.18) < 1e-12 &&
          source_regime_summary.sample_validity_fraction_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .sample_validity_fraction_train_validation_delta.mean -
                   (-0.16)) < 1e-12 &&
          source_regime_summary.entropy_train_validation_delta.count == 1 &&
          std::abs(source_regime_summary.entropy_train_validation_delta.mean -
                   0.80) < 1e-12 &&
          source_regime_summary.compression_ratio_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .compression_ratio_train_validation_delta.mean -
                   0.70) < 1e-12 &&
          source_regime_summary.power_spectrum_entropy_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .power_spectrum_entropy_train_validation_delta.mean -
                   0.23) < 1e-12 &&
          source_regime_summary.source_volatility_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .source_volatility_train_validation_delta.mean -
                   0.21) < 1e-12 &&
          source_regime_summary.feature_variance_train_validation_delta.count ==
              1 &&
          std::abs(source_regime_summary.feature_variance_train_validation_delta
                       .mean -
                   0.08) < 1e-12 &&
          source_regime_summary.duplicate_sample_count_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .duplicate_sample_count_train_validation_delta.mean -
                   4.0) < 1e-12 &&
          source_regime_summary.anchor_support_count_train_validation_delta
                  .count == 1 &&
          std::abs(source_regime_summary
                       .anchor_support_count_train_validation_delta.mean -
                   static_cast<double>(
                       validation_source_analytics.anchor_range.length() -
                       rep_source_analytics.anchor_range.length())) < 1e-12 &&
          source_regime_summary
                  .completed_anchor_support_count_train_validation_delta
                  .count == 1 &&
          std::abs(
              source_regime_summary
                  .completed_anchor_support_count_train_validation_delta.mean -
              static_cast<double>(
                  validation_source_analytics.completed_anchor_range.length() -
                  rep_source_analytics.completed_anchor_range.length())) <
              1e-12 &&
          source_regime_summary.source_regime_shift_warning_count >= 7 &&
          std::any_of(source_regime_summary.issues.begin(),
                      source_regime_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find("source_regime_missingness_fraction_"
                                          "increase_visibility_only") !=
                               std::string::npos;
                      }) &&
          std::any_of(source_regime_summary.issues.begin(),
                      source_regime_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "source_regime_sample_validity_fraction_"
                                   "drop_visibility_only") != std::string::npos;
                      }) &&
          std::any_of(source_regime_summary.issues.begin(),
                      source_regime_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "source_regime_completed_anchor_support_"
                                   "drop_visibility_only") != std::string::npos;
                      }) &&
          std::any_of(source_regime_summary.issues.begin(),
                      source_regime_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find("source_regime_volatility_shift_"
                                          "visibility_only") !=
                               std::string::npos;
                      }) &&
          std::any_of(source_regime_summary.issues.begin(),
                      source_regime_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "source_regime_feature_variance_shift_"
                                   "visibility_only") != std::string::npos;
                      }),
      "source analytics summarizes train-validation source regime drift as "
      "warning-only visibility");
  auto bad_source_analytics = rep_source_analytics;
  bad_source_analytics.coverage_authority = true;
  const auto bad_source_analytics_summary =
      exposure::summarize_source_analytics({rep_fact}, {bad_source_analytics});
  check(bad_source_analytics_summary.coverage_authority &&
            bad_source_analytics_summary.warning_count > 0 &&
            !bad_source_analytics_summary.issues.empty(),
        "source analytics authority drift is reported as warnings, not as a "
        "target-kind expansion");
  auto malformed_source_analytics = rep_source_analytics;
  malformed_source_analytics.source_cursor_token.clear();
  malformed_source_analytics.entropy = std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.entropy_rate =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.information_density =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.compression_ratio =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.power_spectrum_entropy =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.source_volatility =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.feature_variance =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.sample_validity_fraction =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.missingness_fraction =
      std::numeric_limits<double>::quiet_NaN();
  malformed_source_analytics.duplicate_sample_count = 0;
  malformed_source_analytics.source_health_level.clear();
  const auto malformed_source_analytics_issues =
      exposure::source_analytics_fact_issues(malformed_source_analytics);
  std::vector<std::string> malformed_source_analytics_warnings;
  exposure::append_source_analytics_scan_warning(
      malformed_source_analytics, root / "malformed_source_analytics",
      malformed_source_analytics_warnings);
  check(contains_text(malformed_source_analytics_issues,
                      "missing_source_cursor_token") &&
            contains_text(malformed_source_analytics_issues,
                          "missing_source_analytics_payload") &&
            malformed_source_analytics_warnings.size() == 1 &&
            contains_text(malformed_source_analytics_warnings,
                          "missing_source_analytics_payload"),
        "malformed source analytics facts remain visible but warn for missing "
        "cursor identity and source-health payload");
  const auto source_payload_dir = make_tmp_dir("source_analytics_payload");
  write_text(source_payload_dir / "data_analytics.v2.latest.lls",
             "schema:str = jkimyei.evaluation.data_analytics.v2\n"
             "sample_count[0,+inf):uint = 50\n"
             "valid_sample_count[0,+inf):uint = 45\n"
             "skipped_sample_count[0,+inf):uint = 5\n"
             "source_entropic_load[0,+inf):double = 3.125000000000\n"
             "source_volatility[0,+inf):double = 0.125000000000\n");
  write_text(source_payload_dir / "data_analytics.symbolic.v2.latest.lls",
             "schema:str = jkimyei.evaluation.data_analytics_symbolic.v2\n"
             "information_density_mean[0,1]:double = 0.625000000000\n"
             "compression_ratio_mean[0,+inf):double = 1.750000000000\n"
             "power_spectrum_entropy_mean[0,1]:double = 0.440000000000\n"
             "feature_variance_mean[0,+inf):double = 0.062500000000\n");
  const auto runtime_source_analytics =
      exposure::make_runtime_source_analytics_fact_from_exposure_fact(
          rep_fact, source_payload_dir);
  const auto payload_source_analytics =
      exposure::make_source_analytics_facts_from_job_dir(source_payload_dir,
                                                         rep_fact);
  check(
      payload_source_analytics.size() == 1 &&
          std::abs(runtime_source_analytics.entropy - 3.125) < 1e-12 &&
          std::abs(runtime_source_analytics.information_density - 0.625) <
              1e-12 &&
          std::abs(runtime_source_analytics.entropy_rate -
                   (0.625 * std::log2(3.0))) < 1e-12 &&
          std::abs(runtime_source_analytics.compression_ratio - 1.75) < 1e-12 &&
          std::abs(runtime_source_analytics.power_spectrum_entropy - 0.44) <
              1e-12 &&
          std::abs(runtime_source_analytics.source_volatility - 0.125) <
              1e-12 &&
          std::abs(runtime_source_analytics.feature_variance - 0.0625) <
              1e-12 &&
          std::abs(runtime_source_analytics.sample_validity_fraction - 0.90) <
              1e-12 &&
          std::abs(runtime_source_analytics.missingness_fraction - 0.10) <
              1e-12 &&
          runtime_source_analytics.source_health_level == "warn" &&
          runtime_source_analytics.visibility_only &&
          !runtime_source_analytics.readiness_authority &&
          !runtime_source_analytics.coverage_authority &&
          !runtime_source_analytics.leakage_authority &&
          !runtime_source_analytics.contract_identity_authority &&
          std::abs(payload_source_analytics.front().entropy - 3.125) < 1e-12 &&
          std::abs(payload_source_analytics.front().information_density -
                   0.625) < 1e-12 &&
          std::abs(payload_source_analytics.front().source_volatility - 0.125) <
              1e-12 &&
          std::abs(payload_source_analytics.front().feature_variance - 0.0625) <
              1e-12,
      "source analytics facts can reuse source data analytics .lls payloads "
      "while remaining warning-only visibility evidence");
  const auto scan_target_transform_summary =
      exposure::summarize_target_transforms(
          scan.ledger.facts(), scan.ledger.target_transform_facts());
  const auto &rep_target_transform =
      scan.ledger.target_transform_facts().front();
  check(
      rep_target_transform.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(rep_fact) &&
          rep_target_transform.protocol_id == rep_fact.protocol_id &&
          rep_target_transform.graph_order_fingerprint ==
              rep_fact.graph_order_fingerprint &&
          rep_target_transform.source_cursor_token ==
              rep_fact.source_cursor_token &&
          rep_target_transform.target_feature_ids.size() == 2 &&
          rep_target_transform.target_feature_ids[0] == "ev_close" &&
          rep_target_transform.target_feature_ids[1] == "ev_return" &&
          rep_target_transform.horizon == 3 &&
          rep_target_transform.target_mode == "log_return" &&
          rep_target_transform.normalization_contract ==
              "zscore.train_core.v1" &&
          rep_target_transform.inverse_transform_contract == "exp_return.v1" &&
          rep_target_transform.units == "log_return" &&
          rep_target_transform.target_mask_policy_digest == "mask_policy_1" &&
          rep_target_transform.support_surface_identity == "BTC_ETH_SOL:h3" &&
          rep_target_transform.support_surface_digest == "support_surface_1" &&
          rep_target_transform.artifact_contract_prerequisite &&
          rep_target_transform.visibility_only &&
          !rep_target_transform.readiness_authority &&
          !rep_target_transform.quality_authority &&
          !rep_target_transform.performance_authority &&
          !rep_target_transform.coverage_authority &&
          !rep_target_transform.leakage_authority &&
          !rep_target_transform.contract_identity_authority &&
          exposure::target_transform_fact_digest(rep_target_transform).size() ==
              16,
      "target transform facts bind interpretable target contracts to exposure "
      "identity without becoming quality or readiness authority");
  check(
      scan_target_transform_summary.schema ==
              "kikijyeba.lattice.target_transform_summary.v1" &&
          scan_target_transform_summary.exposure_fact_count == 3 &&
          scan_target_transform_summary.target_transform_fact_count == 1 &&
          scan_target_transform_summary.parent_exposure_fact_count == 1 &&
          scan_target_transform_summary.target_feature_id_count == 2 &&
          scan_target_transform_summary.unique_target_feature_id_count == 2 &&
          scan_target_transform_summary.horizon_bound_count == 1 &&
          scan_target_transform_summary.missing_units_count == 0 &&
          scan_target_transform_summary.missing_mask_policy_count == 0 &&
          scan_target_transform_summary.missing_normalization_contract_count ==
              0 &&
          scan_target_transform_summary
                  .missing_inverse_transform_contract_count == 0 &&
          scan_target_transform_summary.artifact_contract_prerequisite_count ==
              1 &&
          scan_target_transform_summary.artifact_contract_prerequisite &&
          scan_target_transform_summary.visibility_only &&
          !scan_target_transform_summary.readiness_authority &&
          !scan_target_transform_summary.quality_authority &&
          !scan_target_transform_summary.performance_authority &&
          !scan_target_transform_summary.coverage_authority &&
          !scan_target_transform_summary.leakage_authority &&
          !scan_target_transform_summary.contract_identity_authority &&
          scan_target_transform_summary.warning_count == 0 &&
          scan_target_transform_summary.issues.empty(),
      "target transform summary is an artifact-contract surface, not a "
      "forecast quality gate");
  auto bad_target_transform = rep_target_transform;
  bad_target_transform.quality_authority = true;
  const auto bad_target_transform_summary =
      exposure::summarize_target_transforms({rep_fact}, {bad_target_transform});
  check(bad_target_transform_summary.quality_authority &&
            bad_target_transform_summary.warning_count > 0 &&
            !bad_target_transform_summary.issues.empty(),
        "target transform authority drift is reported as warnings, not as a "
        "target-kind expansion");
  auto malformed_target_transform = rep_target_transform;
  malformed_target_transform.target_feature_ids.clear();
  malformed_target_transform.horizon = 0;
  malformed_target_transform.normalization_contract.clear();
  malformed_target_transform.inverse_transform_contract.clear();
  malformed_target_transform.units.clear();
  malformed_target_transform.target_mask_policy_digest.clear();
  const auto malformed_target_transform_issues =
      exposure::target_transform_fact_issues(malformed_target_transform);
  std::vector<std::string> malformed_target_transform_warnings;
  exposure::append_target_transform_scan_warning(
      malformed_target_transform, root / "malformed_target_transform",
      malformed_target_transform_warnings);
  check(
      contains_text(malformed_target_transform_issues,
                    "missing_target_feature_ids") &&
          contains_text(malformed_target_transform_issues, "missing_horizon") &&
          contains_text(malformed_target_transform_issues,
                        "missing_normalization_contract") &&
          contains_text(malformed_target_transform_issues,
                        "missing_inverse_transform_contract") &&
          contains_text(malformed_target_transform_issues, "missing_units") &&
          contains_text(malformed_target_transform_issues,
                        "missing_target_mask_policy_digest") &&
          malformed_target_transform_warnings.size() == 1 &&
          contains_text(malformed_target_transform_warnings,
                        "missing_target_mask_policy_digest"),
      "malformed target transform contracts remain visible but warn for "
      "missing interpretation fields");
  const auto scan_forecast_baseline_summary =
      exposure::summarize_forecast_baselines(
          scan.ledger.facts(), scan.ledger.forecast_baseline_facts(),
          scan.ledger.target_transform_facts());
  const auto &rep_forecast_baseline =
      scan.ledger.forecast_baseline_facts().front();
  check(
      rep_forecast_baseline.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(rep_fact) &&
          rep_forecast_baseline.protocol_id == rep_fact.protocol_id &&
          rep_forecast_baseline.graph_order_fingerprint ==
              rep_fact.graph_order_fingerprint &&
          rep_forecast_baseline.source_cursor_token ==
              rep_fact.source_cursor_token &&
          rep_forecast_baseline.target_feature_ids.size() == 2 &&
          rep_forecast_baseline.target_feature_ids[0] == "ev_close" &&
          rep_forecast_baseline.target_feature_ids[1] == "ev_return" &&
          rep_forecast_baseline.horizon == 3 &&
          rep_forecast_baseline.baseline_kind == "previous_value" &&
          rep_forecast_baseline.baseline_parameters == "lag=1" &&
          rep_forecast_baseline.target_transform_fact_digest ==
              "target_transform_digest_1" &&
          rep_forecast_baseline.support_count == 42 &&
          rep_forecast_baseline.valid_count == 40 &&
          rep_forecast_baseline.missing_count == 2 &&
          rep_forecast_baseline.metric_status == "computed" &&
          exposure::forecast_baseline_finite_metric_count(
              rep_forecast_baseline) == 5 &&
          std::abs(rep_forecast_baseline.baseline_mean_nll - 1.50) < 1e-12 &&
          std::abs(rep_forecast_baseline.baseline_ev_mae - 0.20) < 1e-12 &&
          std::abs(rep_forecast_baseline.baseline_ev_rmse - 0.30) < 1e-12 &&
          std::abs(rep_forecast_baseline.baseline_signed_error + 0.01) <
              1e-12 &&
          std::abs(rep_forecast_baseline.baseline_directional_accuracy - 0.55) <
              1e-12 &&
          rep_forecast_baseline.evidence_prerequisite &&
          rep_forecast_baseline.visibility_only &&
          !rep_forecast_baseline.readiness_authority &&
          !rep_forecast_baseline.quality_authority &&
          !rep_forecast_baseline.performance_authority &&
          !rep_forecast_baseline.checkpoint_selector &&
          !rep_forecast_baseline.coverage_authority &&
          !rep_forecast_baseline.leakage_authority &&
          !rep_forecast_baseline.contract_identity_authority &&
          exposure::forecast_baseline_fact_digest(rep_forecast_baseline)
                  .size() == 16,
      "forecast baseline facts bind deterministic baseline metrics to exposure "
      "identity without becoming quality, readiness, or checkpoint-selection "
      "authority");
  check(
      scan_forecast_baseline_summary.schema ==
              "kikijyeba.lattice.forecast_baseline_summary.v1" &&
          scan_forecast_baseline_summary.exposure_fact_count == 3 &&
          scan_forecast_baseline_summary.forecast_baseline_fact_count == 1 &&
          scan_forecast_baseline_summary.parent_exposure_fact_count == 1 &&
          scan_forecast_baseline_summary.target_feature_id_count == 2 &&
          scan_forecast_baseline_summary.unique_target_feature_id_count == 2 &&
          scan_forecast_baseline_summary.horizon_bound_count == 1 &&
          scan_forecast_baseline_summary.target_transform_declared_count == 1 &&
          scan_forecast_baseline_summary.target_transform_bound_count == 0 &&
          scan_forecast_baseline_summary.unresolved_target_transform_count ==
              1 &&
          scan_forecast_baseline_summary
                  .target_transform_identity_mismatch_count == 0 &&
          scan_forecast_baseline_summary.support_count_total == 42 &&
          scan_forecast_baseline_summary.valid_count_total == 40 &&
          scan_forecast_baseline_summary.missing_count_total == 2 &&
          scan_forecast_baseline_summary.unique_baseline_kind_count == 1 &&
          scan_forecast_baseline_summary.previous_value_baseline_count == 1 &&
          scan_forecast_baseline_summary.zero_return_baseline_count == 0 &&
          scan_forecast_baseline_summary.moving_average_baseline_count == 0 &&
          scan_forecast_baseline_summary.last_valid_channel_baseline_count ==
              0 &&
          scan_forecast_baseline_summary.unknown_baseline_kind_count == 0 &&
          scan_forecast_baseline_summary.missing_baseline_kind_count == 0 &&
          scan_forecast_baseline_summary.computed_metric_fact_count == 1 &&
          scan_forecast_baseline_summary.partial_metric_fact_count == 0 &&
          scan_forecast_baseline_summary.deferred_metric_fact_count == 0 &&
          scan_forecast_baseline_summary.missing_metric_status_count == 0 &&
          scan_forecast_baseline_summary.metric_status_mismatch_count == 0 &&
          scan_forecast_baseline_summary.computed_metric_value_count == 5 &&
          scan_forecast_baseline_summary.missing_target_transform_count == 0 &&
          scan_forecast_baseline_summary.evidence_prerequisite_count == 1 &&
          scan_forecast_baseline_summary.baseline_ev_mae.count == 1 &&
          std::abs(scan_forecast_baseline_summary.baseline_ev_mae.mean - 0.20) <
              1e-12 &&
          scan_forecast_baseline_summary.baseline_directional_accuracy.count ==
              1 &&
          std::abs(scan_forecast_baseline_summary.baseline_directional_accuracy
                       .mean -
                   0.55) < 1e-12 &&
          scan_forecast_baseline_summary.evidence_prerequisite &&
          scan_forecast_baseline_summary.visibility_only &&
          !scan_forecast_baseline_summary.readiness_authority &&
          !scan_forecast_baseline_summary.quality_authority &&
          !scan_forecast_baseline_summary.performance_authority &&
          !scan_forecast_baseline_summary.checkpoint_selector &&
          !scan_forecast_baseline_summary.coverage_authority &&
          !scan_forecast_baseline_summary.leakage_authority &&
          !scan_forecast_baseline_summary.contract_identity_authority &&
          scan_forecast_baseline_summary.warning_count > 0 &&
          !scan_forecast_baseline_summary.issues.empty(),
      "forecast baseline summary is an evidence prerequisite for later "
      "forecast-evaluation facts and reports unresolved transform lineage, not "
      "a forecast quality gate");
  auto resolved_forecast_baseline = rep_forecast_baseline;
  resolved_forecast_baseline.target_transform_fact_digest =
      exposure::target_transform_fact_digest(rep_target_transform);
  const auto resolved_forecast_baseline_summary =
      exposure::summarize_forecast_baselines(
          {rep_fact}, {resolved_forecast_baseline}, {rep_target_transform});
  check(resolved_forecast_baseline_summary.target_transform_declared_count ==
                1 &&
            resolved_forecast_baseline_summary.target_transform_bound_count ==
                1 &&
            resolved_forecast_baseline_summary
                    .unresolved_target_transform_count == 0 &&
            resolved_forecast_baseline_summary
                    .target_transform_identity_mismatch_count == 0 &&
            resolved_forecast_baseline_summary.warning_count == 0 &&
            resolved_forecast_baseline_summary.issues.empty(),
        "forecast baseline summaries only mark target transforms bound after "
        "the digest resolves under the same identity");
  auto bad_forecast_baseline = rep_forecast_baseline;
  bad_forecast_baseline.checkpoint_selector = true;
  const auto bad_forecast_baseline_summary =
      exposure::summarize_forecast_baselines({rep_fact},
                                             {bad_forecast_baseline});
  check(bad_forecast_baseline_summary.checkpoint_selector &&
            bad_forecast_baseline_summary.warning_count > 0 &&
            !bad_forecast_baseline_summary.issues.empty(),
        "forecast baseline authority drift is reported as warnings, not as a "
        "best-model selector");
  auto malformed_forecast_baseline = rep_forecast_baseline;
  malformed_forecast_baseline.baseline_kind.clear();
  malformed_forecast_baseline.target_transform_fact_digest.clear();
  malformed_forecast_baseline.support_count = 0;
  malformed_forecast_baseline.valid_count = 0;
  malformed_forecast_baseline.metric_status = "computed";
  malformed_forecast_baseline.baseline_directional_accuracy =
      std::numeric_limits<double>::quiet_NaN();
  const auto malformed_forecast_baseline_issues =
      exposure::forecast_baseline_fact_issues(malformed_forecast_baseline);
  std::vector<std::string> malformed_forecast_baseline_warnings;
  exposure::append_forecast_baseline_scan_warning(
      malformed_forecast_baseline, root / "malformed_forecast_baseline",
      malformed_forecast_baseline_warnings);
  check(contains_text(malformed_forecast_baseline_issues,
                      "missing_baseline_kind") &&
            contains_text(malformed_forecast_baseline_issues,
                          "missing_target_transform_fact_digest") &&
            contains_text(malformed_forecast_baseline_issues,
                          "missing_support_count") &&
            contains_text(malformed_forecast_baseline_issues,
                          "computed_metric_status_without_complete_metrics") &&
            malformed_forecast_baseline_warnings.size() == 1 &&
            contains_text(malformed_forecast_baseline_warnings,
                          "missing_target_transform_fact_digest"),
        "malformed forecast baseline facts remain visible but warn for "
        "missing transform, support, kind, and metric-status consistency");
  const auto scan_forecast_eval_summary = exposure::summarize_forecast_evals(
      scan.ledger.facts(), scan.ledger.forecast_eval_facts(),
      scan.ledger.target_transform_facts(),
      scan.ledger.forecast_baseline_facts(),
      scan.ledger.selection_signal_facts());
  const auto &mdn_forecast_eval = scan.ledger.forecast_eval_facts().front();
  check(
      mdn_forecast_eval.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(channel_mdn_fact) &&
          mdn_forecast_eval.protocol_id == channel_mdn_fact.protocol_id &&
          mdn_forecast_eval.graph_order_fingerprint ==
              channel_mdn_fact.graph_order_fingerprint &&
          mdn_forecast_eval.source_cursor_token ==
              channel_mdn_fact.source_cursor_token &&
          mdn_forecast_eval.target_feature_ids.size() == 2 &&
          mdn_forecast_eval.target_feature_ids[0] == "ev_close" &&
          mdn_forecast_eval.target_feature_ids[1] == "ev_return" &&
          mdn_forecast_eval.horizon == 3 &&
          mdn_forecast_eval.support_count == 42 &&
          mdn_forecast_eval.valid_count == 40 &&
          mdn_forecast_eval.missing_count == 2 &&
          mdn_forecast_eval.weakest_support_rows == 7 &&
          std::abs(mdn_forecast_eval.mean_nll - 1.10) < 1e-12 &&
          mdn_forecast_eval.mean_nll_per_channel.size() == 2 &&
          std::abs(mdn_forecast_eval.mean_nll_per_channel[1] - 1.20) < 1e-12 &&
          mdn_forecast_eval.mean_nll_per_target_feature.size() == 2 &&
          std::abs(mdn_forecast_eval.mean_nll_per_target_feature[0] - 0.90) <
              1e-12 &&
          mdn_forecast_eval.mean_nll_per_channel_target_feature.size() == 4 &&
          std::abs(mdn_forecast_eval.mean_nll_per_channel_target_feature[3] -
                   1.40) < 1e-12 &&
          mdn_forecast_eval.mean_nll_per_horizon.size() == 1 &&
          std::abs(mdn_forecast_eval.mean_nll_per_horizon[0] - 1.10) < 1e-12 &&
          mdn_forecast_eval.valid_target_count_per_channel.size() == 2 &&
          mdn_forecast_eval.valid_target_count_per_channel[0] == 20 &&
          mdn_forecast_eval.valid_target_count_per_target_feature.size() == 2 &&
          mdn_forecast_eval.valid_target_count_per_target_feature[1] == 21 &&
          mdn_forecast_eval.valid_target_count_per_channel_target_feature
                  .size() == 4 &&
          mdn_forecast_eval.valid_target_count_per_channel_target_feature[2] ==
              10 &&
          mdn_forecast_eval.valid_target_count_per_horizon.size() == 1 &&
          mdn_forecast_eval.valid_target_count_per_horizon[0] == 40 &&
          std::abs(mdn_forecast_eval.ev_mae - 0.16) < 1e-12 &&
          std::abs(mdn_forecast_eval.ev_rmse - 0.24) < 1e-12 &&
          std::abs(mdn_forecast_eval.signed_error + 0.02) < 1e-12 &&
          std::abs(mdn_forecast_eval.directional_accuracy - 0.62) < 1e-12 &&
          std::abs(mdn_forecast_eval.calibration_coverage - 0.91) < 1e-12 &&
          mdn_forecast_eval.pit_summary == "uniform-ish" &&
          mdn_forecast_eval.sigma_scale_sanity == "ok" &&
          mdn_forecast_eval.support_by_node == "BTC:20,ETH:13,SOL:7" &&
          mdn_forecast_eval.forecast_artifact_digest == "forecast_artifact_1" &&
          mdn_forecast_eval.evaluated_representation_checkpoint_digest ==
              "rep_checkpoint_1" &&
          mdn_forecast_eval.evaluated_mdn_checkpoint_digest ==
              "mdn_checkpoint_1" &&
          mdn_forecast_eval.target_transform_fact_digest ==
              "target_transform_digest_1" &&
          mdn_forecast_eval.baseline_fact_digests.size() == 1 &&
          mdn_forecast_eval.baseline_fact_digests[0] ==
              "forecast_baseline_digest_1" &&
          mdn_forecast_eval.selection_signal_fact_digests.size() == 1 &&
          mdn_forecast_eval.selection_signal_fact_digests[0] ==
              "selection_signal_digest_1" &&
          mdn_forecast_eval.artifact_evidence &&
          mdn_forecast_eval.visibility_only &&
          !mdn_forecast_eval.model_state_mutation &&
          !mdn_forecast_eval.readiness_authority &&
          !mdn_forecast_eval.quality_authority &&
          !mdn_forecast_eval.performance_authority &&
          !mdn_forecast_eval.checkpoint_selector &&
          !mdn_forecast_eval.coverage_authority &&
          !mdn_forecast_eval.leakage_authority &&
          !mdn_forecast_eval.contract_identity_authority &&
          exposure::forecast_eval_fact_digest(mdn_forecast_eval).size() == 16,
      "forecast eval facts bind evaluation metrics, checkpoint lineage, target "
      "transform, and baseline digests to exposure identity without becoming a "
      "quality gate");
  check(
      scan_forecast_eval_summary.schema ==
              "kikijyeba.lattice.forecast_eval_summary.v1" &&
          scan_forecast_eval_summary.exposure_fact_count == 3 &&
          scan_forecast_eval_summary.forecast_eval_fact_count == 1 &&
          scan_forecast_eval_summary.parent_exposure_fact_count == 1 &&
          scan_forecast_eval_summary.target_feature_id_count == 2 &&
          scan_forecast_eval_summary.unique_target_feature_id_count == 2 &&
          scan_forecast_eval_summary.horizon_bound_count == 1 &&
          scan_forecast_eval_summary.forecast_artifact_bound_count == 1 &&
          scan_forecast_eval_summary.representation_checkpoint_bound_count ==
              1 &&
          scan_forecast_eval_summary.mdn_checkpoint_bound_count == 1 &&
          scan_forecast_eval_summary.target_transform_declared_count == 1 &&
          scan_forecast_eval_summary.target_transform_bound_count == 0 &&
          scan_forecast_eval_summary.unresolved_target_transform_count == 1 &&
          scan_forecast_eval_summary.target_transform_identity_mismatch_count ==
              0 &&
          scan_forecast_eval_summary.baseline_declared_count == 1 &&
          scan_forecast_eval_summary.baseline_bound_count == 0 &&
          scan_forecast_eval_summary.unresolved_baseline_count == 1 &&
          scan_forecast_eval_summary.baseline_identity_mismatch_count == 0 &&
          scan_forecast_eval_summary.selection_signal_declared_count == 1 &&
          scan_forecast_eval_summary.selection_signal_audit_count == 0 &&
          scan_forecast_eval_summary.unresolved_selection_signal_count == 1 &&
          scan_forecast_eval_summary.selection_signal_identity_mismatch_count ==
              0 &&
          scan_forecast_eval_summary.support_count_total == 42 &&
          scan_forecast_eval_summary.valid_count_total == 40 &&
          scan_forecast_eval_summary.missing_count_total == 2 &&
          scan_forecast_eval_summary.nll_surface_bound_count == 1 &&
          scan_forecast_eval_summary.channel_support_surface_bound_count == 1 &&
          scan_forecast_eval_summary
                  .target_feature_support_surface_bound_count == 1 &&
          scan_forecast_eval_summary
                  .channel_target_feature_support_surface_bound_count == 1 &&
          scan_forecast_eval_summary.horizon_support_surface_bound_count == 1 &&
          scan_forecast_eval_summary.horizon_nll_surface_bound_count == 1 &&
          scan_forecast_eval_summary.calibration_coverage_bound_count == 1 &&
          scan_forecast_eval_summary.missing_calibration_coverage_count == 0 &&
          scan_forecast_eval_summary.pit_summary_bound_count == 1 &&
          scan_forecast_eval_summary.missing_pit_summary_count == 0 &&
          scan_forecast_eval_summary.sigma_scale_sanity_bound_count == 1 &&
          scan_forecast_eval_summary.missing_sigma_scale_sanity_count == 0 &&
          scan_forecast_eval_summary.calibration_visibility_warning_count ==
              0 &&
          scan_forecast_eval_summary.missing_forecast_artifact_count == 0 &&
          scan_forecast_eval_summary.missing_representation_checkpoint_count ==
              0 &&
          scan_forecast_eval_summary.missing_mdn_checkpoint_count == 0 &&
          scan_forecast_eval_summary.missing_target_transform_count == 0 &&
          scan_forecast_eval_summary.missing_baseline_count == 0 &&
          scan_forecast_eval_summary.model_state_mutation_count == 0 &&
          scan_forecast_eval_summary.artifact_evidence_count == 1 &&
          scan_forecast_eval_summary.mean_nll.count == 1 &&
          std::abs(scan_forecast_eval_summary.mean_nll.mean - 1.10) < 1e-12 &&
          scan_forecast_eval_summary.ev_mae.count == 1 &&
          std::abs(scan_forecast_eval_summary.ev_mae.mean - 0.16) < 1e-12 &&
          scan_forecast_eval_summary.directional_accuracy.count == 1 &&
          std::abs(scan_forecast_eval_summary.directional_accuracy.mean -
                   0.62) < 1e-12 &&
          scan_forecast_eval_summary.calibration_coverage.count == 1 &&
          std::abs(scan_forecast_eval_summary.calibration_coverage.mean -
                   0.91) < 1e-12 &&
          scan_forecast_eval_summary.skill_vs_baseline_ev_rmse.count == 0 &&
          scan_forecast_eval_summary.artifact_evidence &&
          scan_forecast_eval_summary.visibility_only &&
          !scan_forecast_eval_summary.model_state_mutation &&
          !scan_forecast_eval_summary.readiness_authority &&
          !scan_forecast_eval_summary.quality_authority &&
          !scan_forecast_eval_summary.performance_authority &&
          !scan_forecast_eval_summary.checkpoint_selector &&
          !scan_forecast_eval_summary.coverage_authority &&
          !scan_forecast_eval_summary.leakage_authority &&
          !scan_forecast_eval_summary.contract_identity_authority &&
          scan_forecast_eval_summary.warning_count > 0 &&
          !scan_forecast_eval_summary.issues.empty(),
      "forecast eval summary exposes model metrics and lineage as artifact "
      "evidence, including unresolved relation digests, without accepting "
      "forecast quality");
  auto channel_target_transform = rep_target_transform;
  exposure::populate_artifact_fact_identity(channel_target_transform,
                                            channel_mdn_fact);
  auto channel_forecast_baseline = rep_forecast_baseline;
  exposure::populate_artifact_fact_identity(channel_forecast_baseline,
                                            channel_mdn_fact);
  channel_forecast_baseline.target_transform_fact_digest =
      exposure::target_transform_fact_digest(channel_target_transform);
  exposure::lattice_selection_signal_fact_t channel_selection_signal{};
  channel_selection_signal.parent_exposure_fact_digest =
      exposure::exposure_fact_digest(channel_mdn_fact);
  channel_selection_signal.contract_fingerprint =
      channel_mdn_fact.contract_fingerprint;
  channel_selection_signal.protocol_id = channel_mdn_fact.protocol_id;
  channel_selection_signal.graph_order_fingerprint =
      channel_mdn_fact.graph_order_fingerprint;
  channel_selection_signal.source_cursor_token =
      channel_mdn_fact.source_cursor_token;
  channel_selection_signal.split_policy_fingerprint =
      channel_mdn_fact.split_policy_fingerprint;
  channel_selection_signal.component_assembly_fingerprint =
      channel_mdn_fact.component_assembly_fingerprint;
  channel_selection_signal.target_component_family_id =
      channel_mdn_fact.target_component_family_id;
  channel_selection_signal.job_id = channel_mdn_fact.job_id;
  channel_selection_signal.wave_id = channel_mdn_fact.wave_id;
  channel_selection_signal.split_name = channel_mdn_fact.split_name;
  channel_selection_signal.split_role = channel_mdn_fact.split_role;
  channel_selection_signal.anchor_range = channel_mdn_fact.anchor_range;
  channel_selection_signal.completed_anchor_range =
      channel_mdn_fact.completed_anchor_range;
  channel_selection_signal.selector_id = "mdn_selection_signal";
  channel_selection_signal.selector_split = channel_mdn_fact.split_name;
  channel_selection_signal.selector_metric =
      "runtime_reported_checkpoint_metric";
  channel_selection_signal.tie_policy = "runtime_reported_tie_policy";
  channel_selection_signal.selected_checkpoint_source = "runtime_report";
  channel_selection_signal.selected_checkpoint = "/tmp/mdn.checkpoint";
  channel_selection_signal.candidate_checkpoints = {"/tmp/mdn.checkpoint"};
  channel_selection_signal.candidate_checkpoint_count = 1;
  channel_selection_signal.candidate_checkpoint_digest =
      exposure::selection_signal_candidate_checkpoint_digest(
          channel_selection_signal.candidate_checkpoints);
  channel_selection_signal.selection_footprint = channel_mdn_fact.anchor_range;
  auto resolved_forecast_eval = mdn_forecast_eval;
  resolved_forecast_eval.target_transform_fact_digest =
      exposure::target_transform_fact_digest(channel_target_transform);
  resolved_forecast_eval.baseline_fact_digests = {
      exposure::forecast_baseline_fact_digest(channel_forecast_baseline)};
  resolved_forecast_eval.selection_signal_fact_digests = {
      exposure::selection_signal_fact_digest(channel_selection_signal)};
  const auto resolved_forecast_eval_summary =
      exposure::summarize_forecast_evals(
          {channel_mdn_fact}, {resolved_forecast_eval},
          {channel_target_transform}, {channel_forecast_baseline},
          {channel_selection_signal});
  check(
      resolved_forecast_eval_summary.target_transform_bound_count == 1 &&
          resolved_forecast_eval_summary.baseline_bound_count == 1 &&
          resolved_forecast_eval_summary.selection_signal_audit_count == 1 &&
          resolved_forecast_eval_summary.unresolved_target_transform_count ==
              0 &&
          resolved_forecast_eval_summary.unresolved_baseline_count == 0 &&
          resolved_forecast_eval_summary.unresolved_selection_signal_count ==
              0 &&
          resolved_forecast_eval_summary.skill_vs_baseline_mean_nll.count ==
              1 &&
          std::abs(
              resolved_forecast_eval_summary.skill_vs_baseline_mean_nll.mean -
              ((1.50 - 1.10) / 1.50)) < 1e-12 &&
          resolved_forecast_eval_summary.skill_vs_baseline_ev_rmse.count == 1 &&
          std::abs(
              resolved_forecast_eval_summary.skill_vs_baseline_ev_rmse.mean -
              0.20) < 1e-12 &&
          resolved_forecast_eval_summary.directional_accuracy_delta_vs_baseline
                  .count == 1 &&
          std::abs(resolved_forecast_eval_summary
                       .directional_accuracy_delta_vs_baseline.mean -
                   0.07) < 1e-12 &&
          resolved_forecast_eval_summary.calibration_visibility_warning_count ==
              0 &&
          resolved_forecast_eval_summary.warning_count == 0 &&
          resolved_forecast_eval_summary.issues.empty(),
      "forecast eval summaries only mark transform, baseline, and selection "
      "signal lineage bound when each digest resolves under the same "
      "identity");
  auto weak_calibration_forecast_eval = resolved_forecast_eval;
  weak_calibration_forecast_eval.job_id = "weak_calibration_forecast_eval";
  weak_calibration_forecast_eval.calibration_coverage = 0.62;
  weak_calibration_forecast_eval.pit_summary.clear();
  weak_calibration_forecast_eval.sigma_scale_sanity =
      "sigma_summary_unavailable";
  const auto weak_calibration_summary = exposure::summarize_forecast_evals(
      {channel_mdn_fact}, {weak_calibration_forecast_eval},
      {channel_target_transform}, {channel_forecast_baseline},
      {channel_selection_signal});
  check(
      weak_calibration_summary.calibration_coverage_bound_count == 1 &&
          weak_calibration_summary.missing_calibration_coverage_count == 0 &&
          weak_calibration_summary.pit_summary_bound_count == 0 &&
          weak_calibration_summary.missing_pit_summary_count == 1 &&
          weak_calibration_summary.sigma_scale_sanity_bound_count == 0 &&
          weak_calibration_summary.missing_sigma_scale_sanity_count == 1 &&
          weak_calibration_summary.calibration_visibility_warning_count == 3 &&
          weak_calibration_summary.warning_count == 3 &&
          std::any_of(weak_calibration_summary.issues.begin(),
                      weak_calibration_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "forecast_eval_calibration_coverage_low_"
                                   "overconfidence_visibility_only") !=
                               std::string::npos;
                      }) &&
          std::any_of(weak_calibration_summary.issues.begin(),
                      weak_calibration_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find("forecast_eval_missing_pit_summary_"
                                          "visibility_only") !=
                               std::string::npos;
                      }) &&
          std::any_of(weak_calibration_summary.issues.begin(),
                      weak_calibration_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "forecast_eval_missing_sigma_scale_sanity_"
                                   "visibility_only") != std::string::npos;
                      }) &&
          weak_calibration_summary.artifact_evidence &&
          weak_calibration_summary.visibility_only &&
          !weak_calibration_summary.quality_authority &&
          !weak_calibration_summary.performance_authority,
      "forecast eval calibration, PIT, and sigma diagnostics are warning-only "
      "visibility rather than forecast-quality acceptance");
  const auto channel_selection_event_digest =
      exposure::selection_signal_event_digest(channel_selection_signal);
  auto channel_selection_with_parent_eval = channel_selection_signal;
  channel_selection_with_parent_eval.parent_evaluation_fact_digests = {
      exposure::forecast_eval_fact_digest(resolved_forecast_eval)};
  check(exposure::selection_signal_fact_digest(
            channel_selection_with_parent_eval) ==
                channel_selection_event_digest &&
            exposure::selection_signal_provenance_digest(
                channel_selection_with_parent_eval) !=
                channel_selection_event_digest,
        "selection_signal event digests stay stable when parent evaluation "
        "digests enrich the provenance surface");
  exposure::lattice_exposure_ledger_t selection_eval_link_ledger{};
  selection_eval_link_ledger.add_selection_signal(channel_selection_signal);
  selection_eval_link_ledger.add_forecast_eval(resolved_forecast_eval);
  const auto &linked_selection_signal =
      selection_eval_link_ledger.selection_signal_facts().front();
  const auto linked_selection_summary = exposure::summarize_selection_signals(
      {channel_mdn_fact}, selection_eval_link_ledger.selection_signal_facts(),
      selection_eval_link_ledger.forecast_eval_facts());
  check(linked_selection_signal.parent_evaluation_fact_digests.size() == 1 &&
            linked_selection_signal.parent_evaluation_fact_digests.front() ==
                exposure::forecast_eval_fact_digest(resolved_forecast_eval) &&
            exposure::selection_signal_fact_digest(linked_selection_signal) ==
                channel_selection_event_digest &&
            linked_selection_summary.parent_evaluation_fact_bound_count == 1 &&
            linked_selection_summary.parent_evaluation_fact_declared_count ==
                1 &&
            linked_selection_summary.unresolved_parent_evaluation_fact_count ==
                0 &&
            linked_selection_summary
                    .parent_evaluation_fact_identity_mismatch_count == 0 &&
            linked_selection_summary
                    .parent_evaluation_back_reference_mismatch_count == 0 &&
            linked_selection_summary.issues.empty(),
        "selection_signal facts bind parent evaluation digests after forecast "
        "eval insertion without changing event identity");
  exposure::lattice_exposure_ledger_t reverse_selection_eval_link_ledger{};
  reverse_selection_eval_link_ledger.add_forecast_eval(resolved_forecast_eval);
  reverse_selection_eval_link_ledger.add_selection_signal(
      channel_selection_signal);
  check(reverse_selection_eval_link_ledger.selection_signal_facts()
                    .front()
                    .parent_evaluation_fact_digests.size() == 1 &&
            reverse_selection_eval_link_ledger.selection_signal_facts()
                    .front()
                    .parent_evaluation_fact_digests.front() ==
                exposure::forecast_eval_fact_digest(resolved_forecast_eval),
        "selection_signal parent evaluation binding is independent of scan "
        "insertion order");
  auto selection_with_unresolved_parent = channel_selection_signal;
  selection_with_unresolved_parent.parent_evaluation_fact_digests = {
      "missing_forecast_eval_digest"};
  const auto unresolved_parent_summary = exposure::summarize_selection_signals(
      {channel_mdn_fact}, {selection_with_unresolved_parent},
      {resolved_forecast_eval});
  check(
      unresolved_parent_summary.parent_evaluation_fact_declared_count == 1 &&
          unresolved_parent_summary.parent_evaluation_fact_bound_count == 0 &&
          unresolved_parent_summary.unresolved_parent_evaluation_fact_count ==
              1 &&
          std::any_of(unresolved_parent_summary.issues.begin(),
                      unresolved_parent_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "parent_evaluation_fact_digest_not_found") !=
                               std::string::npos;
                      }),
      "selection_signal summaries report unresolved parent evaluation "
      "digests as provenance integrity issues");
  auto parent_eval_missing_backref = resolved_forecast_eval;
  parent_eval_missing_backref.selection_signal_fact_digests = {
      "other_selection_signal_digest"};
  auto selection_with_missing_backref_parent = channel_selection_signal;
  selection_with_missing_backref_parent.parent_evaluation_fact_digests = {
      exposure::forecast_eval_fact_digest(parent_eval_missing_backref)};
  const auto missing_backref_summary = exposure::summarize_selection_signals(
      {channel_mdn_fact}, {selection_with_missing_backref_parent},
      {parent_eval_missing_backref});
  check(
      missing_backref_summary.parent_evaluation_fact_declared_count == 1 &&
          missing_backref_summary.parent_evaluation_fact_bound_count == 0 &&
          missing_backref_summary
                  .parent_evaluation_back_reference_mismatch_count == 1 &&
          std::any_of(missing_backref_summary.issues.begin(),
                      missing_backref_summary.issues.end(),
                      [](const auto &issue) {
                        return issue.find(
                                   "parent_evaluation_missing_selection_signal_"
                                   "back_reference") != std::string::npos;
                      }),
      "selection_signal summaries require parent forecast evaluations to "
      "reference the same stable selection event digest");
  auto bad_forecast_eval = mdn_forecast_eval;
  bad_forecast_eval.quality_authority = true;
  const auto bad_forecast_eval_summary = exposure::summarize_forecast_evals(
      {channel_mdn_fact}, {bad_forecast_eval});
  check(bad_forecast_eval_summary.quality_authority &&
            bad_forecast_eval_summary.warning_count > 0 &&
            !bad_forecast_eval_summary.issues.empty(),
        "forecast eval authority drift is reported as warnings, not as "
        "forecast_quality_ready");
  auto malformed_forecast_eval = mdn_forecast_eval;
  malformed_forecast_eval.support_count = 0;
  malformed_forecast_eval.valid_count = 0;
  malformed_forecast_eval.forecast_artifact_digest.clear();
  malformed_forecast_eval.evaluated_representation_checkpoint_digest.clear();
  malformed_forecast_eval.evaluated_mdn_checkpoint_digest.clear();
  malformed_forecast_eval.target_transform_fact_digest.clear();
  malformed_forecast_eval.baseline_fact_digests.clear();
  malformed_forecast_eval.support_by_horizon.clear();
  malformed_forecast_eval.valid_target_count_per_horizon.clear();
  malformed_forecast_eval.model_state_mutation = true;
  const auto malformed_forecast_eval_issues =
      exposure::forecast_eval_fact_issues(malformed_forecast_eval);
  std::vector<std::string> malformed_forecast_eval_warnings;
  exposure::append_forecast_eval_scan_warning(malformed_forecast_eval,
                                              root / "malformed_forecast_eval",
                                              malformed_forecast_eval_warnings);
  check(
      contains_text(malformed_forecast_eval_issues, "missing_support_count") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_forecast_artifact_digest") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_evaluated_representation_checkpoint_"
                        "digest") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_evaluated_mdn_checkpoint_digest") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_target_transform_fact_digest") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_baseline_fact_digests") &&
          contains_text(malformed_forecast_eval_issues,
                        "missing_horizon_support_surface") &&
          contains_text(malformed_forecast_eval_issues,
                        "forecast_eval_must_remain_artifact_evidence_only") &&
          malformed_forecast_eval_warnings.size() == 1 &&
          contains_text(malformed_forecast_eval_warnings,
                        "missing_forecast_artifact_digest"),
      "malformed forecast eval facts remain visible but warn for missing "
      "lineage, support, baseline, transform, and mutation constraints");
  const auto scan_observer_belief_summary =
      exposure::summarize_observer_beliefs(scan.ledger.facts(),
                                           scan.ledger.observer_belief_facts(),
                                           scan.ledger.forecast_eval_facts());
  const auto &mdn_observer_belief = scan.ledger.observer_belief_facts().front();
  check(
      mdn_observer_belief.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(channel_mdn_fact) &&
          mdn_observer_belief.protocol_id == channel_mdn_fact.protocol_id &&
          mdn_observer_belief.graph_order_fingerprint ==
              channel_mdn_fact.graph_order_fingerprint &&
          mdn_observer_belief.source_cursor_token ==
              channel_mdn_fact.source_cursor_token &&
          mdn_observer_belief.belief_kind == "raw_nodelift_potential" &&
          mdn_observer_belief.channel_consensus == "BTC:0.62,ETH:0.58" &&
          mdn_observer_belief.potential_surface_diagnostics ==
              "stable_surface" &&
          mdn_observer_belief.nodelift_return_projection == "potential_only" &&
          mdn_observer_belief.covariance_coupling == "diagonal_scenario_bank" &&
          mdn_observer_belief.scenario_bank_digest == "scenario_bank_1" &&
          mdn_observer_belief.nodelift_residual_quality == "ok" &&
          mdn_observer_belief.projection_validation_scores ==
              "rmse:0.24,calibration:0.91" &&
          std::abs(mdn_observer_belief.confidence - 0.64) < 1e-12 &&
          std::abs(mdn_observer_belief.data_quality - 0.88) < 1e-12 &&
          std::abs(mdn_observer_belief.liquidity - 0.72) < 1e-12 &&
          mdn_observer_belief.forecast_artifact_digest ==
              "forecast_artifact_1" &&
          mdn_observer_belief.forecast_artifact_lineage ==
              "forecast_eval_digest_1" &&
          mdn_observer_belief.feature_semantics_fingerprint ==
              "feature_semantics_1" &&
          mdn_observer_belief.dock_binding_fingerprint == "dock_binding_1" &&
          mdn_observer_belief.artifact_evidence &&
          mdn_observer_belief.visibility_only &&
          !mdn_observer_belief.raw_potential_tradable_return &&
          !mdn_observer_belief.allocation_authority &&
          !mdn_observer_belief.readiness_authority &&
          !mdn_observer_belief.quality_authority &&
          !mdn_observer_belief.performance_authority &&
          !mdn_observer_belief.checkpoint_selector &&
          !mdn_observer_belief.coverage_authority &&
          !mdn_observer_belief.leakage_authority &&
          !mdn_observer_belief.contract_identity_authority &&
          !mdn_observer_belief.market_readiness_authority &&
          exposure::observer_belief_fact_digest(mdn_observer_belief).size() ==
              16,
      "observer belief facts bind deterministic post-inference diagnostics to "
      "forecast lineage without making raw NodeLift potential tradable or "
      "allocatable");
  const auto observer_alias_dir = root / "observer_belief_aliases";
  write_text(observer_alias_dir / "runtime.observer_belief.fact",
             "schema=kikijyeba.lattice.observer_belief.v1\n"
             "observer_belief_kind=allocation_belief\n"
             "observer_channel_consensus=BTC:0.70,ETH:0.65\n"
             "nodelift_potential_diagnostics=projected_surface\n"
             "return_projection=allocation_projection\n"
             "scenario_covariance_coupling=full_scenario_bank\n"
             "scenario_digest=scenario_alias_digest\n"
             "residual_quality=stable\n"
             "projection_validation_summary=rmse:0.19,calibration:0.90\n"
             "observer_confidence=0.81\n"
             "observer_data_quality=0.82\n"
             "observer_liquidity=0.83\n"
             "forecast_digest=forecast_alias_digest\n"
             "forecast_lineage=forecast_eval_alias_digest\n"
             "feature_semantics_digest=feature_semantics_alias\n"
             "dock_binding_digest=dock_binding_alias\n"
             "artifact_evidence=true\n"
             "visibility_only=true\n"
             "raw_nodelift_potential_tradable_return=false\n");
  const auto alias_observer_beliefs =
      exposure::make_observer_belief_facts_from_job_dir(observer_alias_dir,
                                                        channel_mdn_fact);
  check(
      alias_observer_beliefs.size() == 1 &&
          alias_observer_beliefs.front().parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(channel_mdn_fact) &&
          alias_observer_beliefs.front().protocol_id ==
              channel_mdn_fact.protocol_id &&
          alias_observer_beliefs.front().source_cursor_token ==
              channel_mdn_fact.source_cursor_token &&
          alias_observer_beliefs.front().belief_kind == "allocation_belief" &&
          alias_observer_beliefs.front().channel_consensus ==
              "BTC:0.70,ETH:0.65" &&
          alias_observer_beliefs.front().potential_surface_diagnostics ==
              "projected_surface" &&
          alias_observer_beliefs.front().nodelift_return_projection ==
              "allocation_projection" &&
          alias_observer_beliefs.front().covariance_coupling ==
              "full_scenario_bank" &&
          alias_observer_beliefs.front().scenario_bank_digest ==
              "scenario_alias_digest" &&
          alias_observer_beliefs.front().nodelift_residual_quality ==
              "stable" &&
          alias_observer_beliefs.front().projection_validation_scores ==
              "rmse:0.19,calibration:0.90" &&
          std::abs(alias_observer_beliefs.front().confidence - 0.81) < 1e-12 &&
          std::abs(alias_observer_beliefs.front().data_quality - 0.82) <
              1e-12 &&
          std::abs(alias_observer_beliefs.front().liquidity - 0.83) < 1e-12 &&
          alias_observer_beliefs.front().forecast_artifact_digest ==
              "forecast_alias_digest" &&
          alias_observer_beliefs.front().forecast_artifact_lineage ==
              "forecast_eval_alias_digest" &&
          alias_observer_beliefs.front().feature_semantics_fingerprint ==
              "feature_semantics_alias" &&
          alias_observer_beliefs.front().dock_binding_fingerprint ==
              "dock_binding_alias" &&
          alias_observer_beliefs.front().artifact_evidence &&
          alias_observer_beliefs.front().visibility_only &&
          !alias_observer_beliefs.front().raw_potential_tradable_return &&
          !alias_observer_beliefs.front().allocation_authority &&
          !alias_observer_beliefs.front().readiness_authority &&
          !alias_observer_beliefs.front().quality_authority &&
          !alias_observer_beliefs.front().performance_authority &&
          !alias_observer_beliefs.front().market_readiness_authority &&
          exposure::observer_belief_fact_issues(alias_observer_beliefs.front())
              .empty(),
      "observer belief scanner accepts declared writer aliases while "
      "distinguishing allocation belief artifacts from allocation authority");
  check(
      scan_observer_belief_summary.schema ==
              "kikijyeba.lattice.observer_belief_summary.v1" &&
          scan_observer_belief_summary.exposure_fact_count == 3 &&
          scan_observer_belief_summary.observer_belief_fact_count == 1 &&
          scan_observer_belief_summary.parent_exposure_fact_count == 1 &&
          scan_observer_belief_summary.raw_nodelift_potential_count == 1 &&
          scan_observer_belief_summary.allocation_belief_count == 0 &&
          scan_observer_belief_summary.forecast_artifact_bound_count == 1 &&
          scan_observer_belief_summary
                  .forecast_artifact_lineage_declared_count == 1 &&
          scan_observer_belief_summary.forecast_artifact_lineage_bound_count ==
              0 &&
          scan_observer_belief_summary
                  .unresolved_forecast_artifact_lineage_count == 1 &&
          scan_observer_belief_summary
                  .forecast_artifact_lineage_identity_mismatch_count == 0 &&
          scan_observer_belief_summary
                  .forecast_artifact_digest_mismatch_count == 0 &&
          scan_observer_belief_summary.channel_consensus_bound_count == 1 &&
          scan_observer_belief_summary
                  .potential_surface_diagnostics_bound_count == 1 &&
          scan_observer_belief_summary.nodelift_return_projection_bound_count ==
              1 &&
          scan_observer_belief_summary.covariance_coupling_bound_count == 1 &&
          scan_observer_belief_summary.scenario_bank_bound_count == 1 &&
          scan_observer_belief_summary.nodelift_residual_quality_bound_count ==
              1 &&
          scan_observer_belief_summary
                  .projection_validation_scores_bound_count == 1 &&
          scan_observer_belief_summary.feature_semantics_bound_count == 1 &&
          scan_observer_belief_summary.dock_binding_bound_count == 1 &&
          scan_observer_belief_summary.missing_belief_kind_count == 0 &&
          scan_observer_belief_summary.missing_forecast_artifact_count == 0 &&
          scan_observer_belief_summary.missing_channel_consensus_count == 0 &&
          scan_observer_belief_summary
                  .missing_potential_surface_diagnostics_count == 0 &&
          scan_observer_belief_summary
                  .missing_nodelift_return_projection_count == 0 &&
          scan_observer_belief_summary.missing_covariance_coupling_count == 0 &&
          scan_observer_belief_summary.missing_scenario_bank_count == 0 &&
          scan_observer_belief_summary
                  .missing_nodelift_residual_quality_count == 0 &&
          scan_observer_belief_summary
                  .missing_projection_validation_scores_count == 0 &&
          scan_observer_belief_summary.missing_feature_semantics_count == 0 &&
          scan_observer_belief_summary.missing_dock_binding_count == 0 &&
          scan_observer_belief_summary.low_confidence_count == 1 &&
          scan_observer_belief_summary.low_data_quality_count == 0 &&
          scan_observer_belief_summary.low_liquidity_count == 0 &&
          scan_observer_belief_summary.diagnostic_completeness_warning_count ==
              0 &&
          scan_observer_belief_summary.observer_quality_warning_count == 1 &&
          scan_observer_belief_summary.artifact_evidence_count == 1 &&
          scan_observer_belief_summary.confidence.count == 1 &&
          std::abs(scan_observer_belief_summary.confidence.mean - 0.64) <
              1e-12 &&
          scan_observer_belief_summary.data_quality.count == 1 &&
          std::abs(scan_observer_belief_summary.data_quality.mean - 0.88) <
              1e-12 &&
          scan_observer_belief_summary.liquidity.count == 1 &&
          std::abs(scan_observer_belief_summary.liquidity.mean - 0.72) <
              1e-12 &&
          scan_observer_belief_summary.artifact_evidence &&
          scan_observer_belief_summary.visibility_only &&
          !scan_observer_belief_summary.raw_potential_tradable_return &&
          !scan_observer_belief_summary.allocation_authority &&
          !scan_observer_belief_summary.readiness_authority &&
          !scan_observer_belief_summary.quality_authority &&
          !scan_observer_belief_summary.performance_authority &&
          !scan_observer_belief_summary.checkpoint_selector &&
          !scan_observer_belief_summary.coverage_authority &&
          !scan_observer_belief_summary.leakage_authority &&
          !scan_observer_belief_summary.contract_identity_authority &&
          !scan_observer_belief_summary.market_readiness_authority &&
          scan_observer_belief_summary.warning_count > 0 &&
          !scan_observer_belief_summary.issues.empty(),
      "observer belief summary remains visibility-only evidence and separates "
      "raw NodeLift potential from allocation belief authority while reporting "
      "unresolved forecast lineage");
  auto resolved_observer_belief = mdn_observer_belief;
  resolved_observer_belief.forecast_artifact_lineage =
      exposure::forecast_eval_fact_digest(resolved_forecast_eval);
  const auto resolved_observer_belief_summary =
      exposure::summarize_observer_beliefs({channel_mdn_fact},
                                           {resolved_observer_belief},
                                           {resolved_forecast_eval});
  check(resolved_observer_belief_summary
                    .forecast_artifact_lineage_declared_count == 1 &&
            resolved_observer_belief_summary
                    .forecast_artifact_lineage_bound_count == 1 &&
            resolved_observer_belief_summary
                    .unresolved_forecast_artifact_lineage_count == 0 &&
            resolved_observer_belief_summary
                    .forecast_artifact_digest_mismatch_count == 0 &&
            resolved_observer_belief_summary.low_confidence_count == 1 &&
            resolved_observer_belief_summary.observer_quality_warning_count ==
                1 &&
            resolved_observer_belief_summary.warning_count == 1 &&
            std::find(resolved_observer_belief_summary.issues.begin(),
                      resolved_observer_belief_summary.issues.end(),
                      "channel_mdn_job:"
                      "observer_belief_low_confidence_visibility_only") !=
                resolved_observer_belief_summary.issues.end(),
        "observer belief summaries bind forecast lineage after the forecast "
        "eval digest resolves while low confidence remains warning-only "
        "visibility");
  auto bad_observer_belief = mdn_observer_belief;
  bad_observer_belief.raw_potential_tradable_return = true;
  const auto bad_observer_belief_summary = exposure::summarize_observer_beliefs(
      {channel_mdn_fact}, {bad_observer_belief});
  check(bad_observer_belief_summary.raw_potential_tradable_return &&
            bad_observer_belief_summary.warning_count > 0 &&
            !bad_observer_belief_summary.issues.empty(),
        "observer belief authority drift is reported as warnings, not as "
        "tradable-return or allocation authority");
  auto malformed_observer_belief = mdn_observer_belief;
  malformed_observer_belief.source_cursor_token.clear();
  malformed_observer_belief.belief_kind.clear();
  malformed_observer_belief.forecast_artifact_digest.clear();
  malformed_observer_belief.scenario_bank_digest.clear();
  malformed_observer_belief.feature_semantics_fingerprint.clear();
  malformed_observer_belief.dock_binding_fingerprint.clear();
  const auto malformed_observer_belief_issues =
      exposure::observer_belief_fact_issues(malformed_observer_belief);
  std::vector<std::string> malformed_observer_belief_warnings;
  exposure::append_observer_belief_scan_warning(
      malformed_observer_belief, root / "malformed_observer_belief",
      malformed_observer_belief_warnings);
  check(contains_text(malformed_observer_belief_issues,
                      "missing_source_cursor_token") &&
            contains_text(malformed_observer_belief_issues,
                          "missing_belief_kind") &&
            contains_text(malformed_observer_belief_issues,
                          "missing_forecast_artifact_digest") &&
            contains_text(malformed_observer_belief_issues,
                          "missing_scenario_bank_digest") &&
            contains_text(malformed_observer_belief_issues,
                          "missing_feature_semantics_fingerprint") &&
            contains_text(malformed_observer_belief_issues,
                          "missing_dock_binding_fingerprint") &&
            malformed_observer_belief_warnings.size() == 1 &&
            contains_text(malformed_observer_belief_warnings,
                          "missing_belief_kind"),
        "malformed observer belief facts remain visible but warn for missing "
        "identity, kind, forecast, scenario, feature, and dock bindings");
  const auto scan_allocation_engine_summary =
      exposure::summarize_allocation_engines(
          scan.ledger.facts(), scan.ledger.allocation_engine_facts(),
          scan.ledger.observer_belief_facts(),
          scan.ledger.forecast_eval_facts());
  const auto &mdn_allocation_engine =
      scan.ledger.allocation_engine_facts().front();
  check(mdn_allocation_engine.parent_exposure_fact_digest ==
                exposure::exposure_fact_digest(channel_mdn_fact) &&
            mdn_allocation_engine.protocol_id == channel_mdn_fact.protocol_id &&
            mdn_allocation_engine.graph_order_fingerprint ==
                channel_mdn_fact.graph_order_fingerprint &&
            mdn_allocation_engine.source_cursor_token ==
                channel_mdn_fact.source_cursor_token &&
            mdn_allocation_engine.target_risky_node_weights ==
                "BTC:0.40,ETH:0.25,SOL:0.10" &&
            mdn_allocation_engine.reserve_node_id == "USD_CASH" &&
            mdn_allocation_engine.reserve_node_source == "base_policy" &&
            mdn_allocation_engine.base_policy_reserve_node_id == "USD_CASH" &&
            mdn_allocation_engine.reserve_node_graph_bound &&
            std::abs(mdn_allocation_engine.reserve_weight - 0.25) < 1e-12 &&
            std::abs(mdn_allocation_engine.turnover - 0.12) < 1e-12 &&
            mdn_allocation_engine.objective_terms ==
                "growth:0.70,cvar:0.20,cost:0.10" &&
            std::abs(mdn_allocation_engine.cvar_loss - 0.08) < 1e-12 &&
            std::abs(mdn_allocation_engine.transaction_cost_estimate - 0.003) <
                1e-12 &&
            mdn_allocation_engine.constraint_diagnostics ==
                "all_constraints_satisfied" &&
            mdn_allocation_engine.cap_diagnostics == "per_node_caps_ok" &&
            mdn_allocation_engine.scenario_growth_floor_status == "met" &&
            mdn_allocation_engine.fallback_reasons == "none" &&
            mdn_allocation_engine.derisk_reasons == "none" &&
            mdn_allocation_engine.observer_belief_fact_digest ==
                "observer_belief_digest_1" &&
            mdn_allocation_engine.forecast_artifact_digest ==
                "forecast_artifact_1" &&
            mdn_allocation_engine.base_policy_digest == "base_policy_1" &&
            mdn_allocation_engine.deterministic_artifact &&
            mdn_allocation_engine.visibility_only &&
            !mdn_allocation_engine.allocation_authority &&
            !mdn_allocation_engine.execution_authority &&
            !mdn_allocation_engine.readiness_authority &&
            !mdn_allocation_engine.quality_authority &&
            !mdn_allocation_engine.performance_authority &&
            !mdn_allocation_engine.market_readiness_authority &&
            !mdn_allocation_engine.deployment_authority &&
            !mdn_allocation_engine.checkpoint_selector &&
            !mdn_allocation_engine.coverage_authority &&
            !mdn_allocation_engine.leakage_authority &&
            !mdn_allocation_engine.contract_identity_authority &&
            exposure::allocation_engine_fact_digest(mdn_allocation_engine)
                    .size() == 16,
        "allocation engine facts bind deterministic allocation output "
        "diagnostics to observer belief and forecast lineage without becoming "
        "Lattice allocation or execution authority");
  const auto allocation_alias_dir = root / "allocation_engine_aliases";
  write_text(allocation_alias_dir / "allocation.engine.fact",
             "schema=kikijyeba.lattice.allocation_engine.v1\n"
             "risky_node_weights=BTC:0.30,ETH:0.20\n"
             "reserve_asset_node_id=USD_CASH\n"
             "reserve_asset_source=base_policy\n"
             "base_policy_reserve_asset_node_id=USD_CASH\n"
             "reserve_asset_graph_bound=true\n"
             "reserve_weight=0.50\n"
             "allocation_turnover=0.05\n"
             "allocation_objective_terms=growth:0.60,cvar:0.30,cost:0.10\n"
             "allocation_cvar_loss=0.04\n"
             "estimated_transaction_cost=0.001\n"
             "allocation_constraints=alias_constraints_ok\n"
             "allocation_cap_diagnostics=alias_caps_ok\n"
             "growth_floor_status=met\n"
             "fallback_reason_contract=none\n"
             "derisk_reason_contract=none\n"
             "observer_belief_digest=observer_alias_digest\n"
             "forecast_digest=forecast_alias_digest\n"
             "base_policy_fingerprint=base_policy_alias\n"
             "deterministic_artifact=true\n"
             "visibility_only=true\n");
  const auto alias_allocation_engines =
      exposure::make_allocation_engine_facts_from_job_dir(allocation_alias_dir,
                                                          channel_mdn_fact);
  check(
      alias_allocation_engines.size() == 1 &&
          alias_allocation_engines.front().parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(channel_mdn_fact) &&
          alias_allocation_engines.front().protocol_id ==
              channel_mdn_fact.protocol_id &&
          alias_allocation_engines.front().source_cursor_token ==
              channel_mdn_fact.source_cursor_token &&
          alias_allocation_engines.front().target_risky_node_weights ==
              "BTC:0.30,ETH:0.20" &&
          alias_allocation_engines.front().reserve_node_id == "USD_CASH" &&
          alias_allocation_engines.front().reserve_node_source ==
              "base_policy" &&
          alias_allocation_engines.front().base_policy_reserve_node_id ==
              "USD_CASH" &&
          alias_allocation_engines.front().reserve_node_graph_bound &&
          std::abs(alias_allocation_engines.front().reserve_weight - 0.50) <
              1e-12 &&
          std::abs(alias_allocation_engines.front().turnover - 0.05) < 1e-12 &&
          alias_allocation_engines.front().objective_terms ==
              "growth:0.60,cvar:0.30,cost:0.10" &&
          std::abs(alias_allocation_engines.front().cvar_loss - 0.04) < 1e-12 &&
          std::abs(alias_allocation_engines.front().transaction_cost_estimate -
                   0.001) < 1e-12 &&
          alias_allocation_engines.front().constraint_diagnostics ==
              "alias_constraints_ok" &&
          alias_allocation_engines.front().cap_diagnostics == "alias_caps_ok" &&
          alias_allocation_engines.front().scenario_growth_floor_status ==
              "met" &&
          alias_allocation_engines.front().fallback_reasons == "none" &&
          alias_allocation_engines.front().derisk_reasons == "none" &&
          alias_allocation_engines.front().observer_belief_fact_digest ==
              "observer_alias_digest" &&
          alias_allocation_engines.front().forecast_artifact_digest ==
              "forecast_alias_digest" &&
          alias_allocation_engines.front().base_policy_digest ==
              "base_policy_alias" &&
          alias_allocation_engines.front().deterministic_artifact &&
          alias_allocation_engines.front().visibility_only &&
          !alias_allocation_engines.front().allocation_authority &&
          !alias_allocation_engines.front().execution_authority &&
          !alias_allocation_engines.front().readiness_authority &&
          !alias_allocation_engines.front().market_readiness_authority &&
          !alias_allocation_engines.front().deployment_authority &&
          exposure::allocation_engine_fact_issues(
              alias_allocation_engines.front())
              .empty(),
      "allocation engine scanner accepts declared writer aliases while keeping "
      "allocation output audit-only and non-executing");
  check(
      scan_allocation_engine_summary.schema ==
              "kikijyeba.lattice.allocation_engine_summary.v1" &&
          scan_allocation_engine_summary.exposure_fact_count == 3 &&
          scan_allocation_engine_summary.allocation_engine_fact_count == 1 &&
          scan_allocation_engine_summary.parent_exposure_fact_count == 1 &&
          scan_allocation_engine_summary.reserve_node_bound_count == 1 &&
          scan_allocation_engine_summary
                  .reserve_node_source_base_policy_count == 1 &&
          scan_allocation_engine_summary.base_policy_reserve_node_bound_count ==
              1 &&
          scan_allocation_engine_summary.reserve_node_base_policy_match_count ==
              1 &&
          scan_allocation_engine_summary.reserve_node_graph_bound_count == 1 &&
          scan_allocation_engine_summary.observer_belief_declared_count == 1 &&
          scan_allocation_engine_summary.observer_belief_bound_count == 0 &&
          scan_allocation_engine_summary.unresolved_observer_belief_count ==
              1 &&
          scan_allocation_engine_summary
                  .observer_belief_identity_mismatch_count == 0 &&
          scan_allocation_engine_summary.forecast_artifact_declared_count ==
              1 &&
          scan_allocation_engine_summary.forecast_artifact_bound_count == 1 &&
          scan_allocation_engine_summary.unresolved_forecast_artifact_count ==
              0 &&
          scan_allocation_engine_summary
                  .forecast_artifact_identity_mismatch_count == 0 &&
          scan_allocation_engine_summary
                  .observer_forecast_artifact_mismatch_count == 0 &&
          scan_allocation_engine_summary.base_policy_bound_count == 1 &&
          scan_allocation_engine_summary.objective_terms_bound_count == 1 &&
          scan_allocation_engine_summary.cvar_bound_count == 1 &&
          scan_allocation_engine_summary.transaction_cost_bound_count == 1 &&
          scan_allocation_engine_summary.constraint_diagnostics_bound_count ==
              1 &&
          scan_allocation_engine_summary.cap_diagnostics_bound_count == 1 &&
          scan_allocation_engine_summary
                  .scenario_growth_floor_status_bound_count == 1 &&
          scan_allocation_engine_summary.scenario_growth_floor_met_count == 1 &&
          scan_allocation_engine_summary
                  .scenario_growth_floor_attention_count == 0 &&
          scan_allocation_engine_summary.fallback_reason_contract_count == 1 &&
          scan_allocation_engine_summary.fallback_reason_none_count == 1 &&
          scan_allocation_engine_summary.fallback_reason_active_count == 0 &&
          scan_allocation_engine_summary.derisk_reason_contract_count == 1 &&
          scan_allocation_engine_summary.derisk_reason_none_count == 1 &&
          scan_allocation_engine_summary.derisk_reason_active_count == 0 &&
          scan_allocation_engine_summary.deterministic_artifact_count == 1 &&
          scan_allocation_engine_summary.missing_reserve_node_count == 0 &&
          scan_allocation_engine_summary.missing_reserve_node_source_count ==
              0 &&
          scan_allocation_engine_summary
                  .missing_base_policy_reserve_node_count == 0 &&
          scan_allocation_engine_summary.reserve_node_source_mismatch_count ==
              0 &&
          scan_allocation_engine_summary
                  .reserve_node_base_policy_mismatch_count == 0 &&
          scan_allocation_engine_summary.reserve_node_not_graph_bound_count ==
              0 &&
          scan_allocation_engine_summary.missing_observer_belief_count == 0 &&
          scan_allocation_engine_summary.missing_forecast_artifact_count == 0 &&
          scan_allocation_engine_summary.missing_base_policy_count == 0 &&
          scan_allocation_engine_summary.missing_cap_diagnostics_count == 0 &&
          scan_allocation_engine_summary
                  .missing_scenario_growth_floor_status_count == 0 &&
          scan_allocation_engine_summary.allocation_diagnostic_warning_count ==
              0 &&
          scan_allocation_engine_summary.reserve_weight.count == 1 &&
          std::abs(scan_allocation_engine_summary.reserve_weight.mean - 0.25) <
              1e-12 &&
          scan_allocation_engine_summary.turnover.count == 1 &&
          std::abs(scan_allocation_engine_summary.turnover.mean - 0.12) <
              1e-12 &&
          scan_allocation_engine_summary.cvar_loss.count == 1 &&
          std::abs(scan_allocation_engine_summary.cvar_loss.mean - 0.08) <
              1e-12 &&
          scan_allocation_engine_summary.transaction_cost_estimate.count == 1 &&
          std::abs(
              scan_allocation_engine_summary.transaction_cost_estimate.mean -
              0.003) < 1e-12 &&
          scan_allocation_engine_summary.deterministic_artifact &&
          scan_allocation_engine_summary.visibility_only &&
          !scan_allocation_engine_summary.allocation_authority &&
          !scan_allocation_engine_summary.execution_authority &&
          !scan_allocation_engine_summary.readiness_authority &&
          !scan_allocation_engine_summary.quality_authority &&
          !scan_allocation_engine_summary.performance_authority &&
          !scan_allocation_engine_summary.market_readiness_authority &&
          !scan_allocation_engine_summary.deployment_authority &&
          !scan_allocation_engine_summary.checkpoint_selector &&
          !scan_allocation_engine_summary.coverage_authority &&
          !scan_allocation_engine_summary.leakage_authority &&
          !scan_allocation_engine_summary.contract_identity_authority &&
          scan_allocation_engine_summary.warning_count > 0 &&
          !scan_allocation_engine_summary.issues.empty(),
      "allocation engine summary exposes objective/cost/CVaR/fallback "
      "diagnostics as audit evidence and reports unresolved observer lineage, "
      "not allocation acceptance");
  auto resolved_allocation_engine = mdn_allocation_engine;
  resolved_allocation_engine.observer_belief_fact_digest =
      exposure::observer_belief_fact_digest(resolved_observer_belief);
  const auto resolved_allocation_engine_summary =
      exposure::summarize_allocation_engines(
          {channel_mdn_fact}, {resolved_allocation_engine},
          {resolved_observer_belief}, {resolved_forecast_eval});
  check(
      resolved_allocation_engine_summary.observer_belief_bound_count == 1 &&
          resolved_allocation_engine_summary.forecast_artifact_bound_count ==
              1 &&
          resolved_allocation_engine_summary.unresolved_observer_belief_count ==
              0 &&
          resolved_allocation_engine_summary
                  .unresolved_forecast_artifact_count == 0 &&
          resolved_allocation_engine_summary
                  .observer_forecast_artifact_mismatch_count == 0 &&
          resolved_allocation_engine_summary.warning_count == 0 &&
          resolved_allocation_engine_summary.issues.empty(),
      "allocation summaries bind observer and forecast artifact lineage only "
      "after those references resolve under the same identity");
  auto attention_allocation_engine = resolved_allocation_engine;
  attention_allocation_engine.scenario_growth_floor_status = "violated";
  attention_allocation_engine.fallback_reasons = "risk_parity_fallback";
  attention_allocation_engine.derisk_reasons =
      "mean_confidence_below_threshold";
  const auto attention_allocation_summary =
      exposure::summarize_allocation_engines(
          {channel_mdn_fact}, {attention_allocation_engine},
          {resolved_observer_belief}, {resolved_forecast_eval});
  check(
      attention_allocation_summary.scenario_growth_floor_status_bound_count ==
              1 &&
          attention_allocation_summary.scenario_growth_floor_met_count == 0 &&
          attention_allocation_summary.scenario_growth_floor_attention_count ==
              1 &&
          attention_allocation_summary.fallback_reason_contract_count == 1 &&
          attention_allocation_summary.fallback_reason_none_count == 0 &&
          attention_allocation_summary.fallback_reason_active_count == 1 &&
          attention_allocation_summary.derisk_reason_contract_count == 1 &&
          attention_allocation_summary.derisk_reason_none_count == 0 &&
          attention_allocation_summary.derisk_reason_active_count == 1 &&
          attention_allocation_summary.allocation_diagnostic_warning_count ==
              3 &&
          attention_allocation_summary.warning_count == 3 &&
          std::find(attention_allocation_summary.issues.begin(),
                    attention_allocation_summary.issues.end(),
                    "channel_mdn_job:"
                    "allocation_engine_scenario_growth_floor_attention_"
                    "visibility_only") !=
              attention_allocation_summary.issues.end() &&
          std::find(attention_allocation_summary.issues.begin(),
                    attention_allocation_summary.issues.end(),
                    "channel_mdn_job:"
                    "allocation_engine_active_fallback_reason_"
                    "visibility_only") !=
              attention_allocation_summary.issues.end() &&
          std::find(attention_allocation_summary.issues.begin(),
                    attention_allocation_summary.issues.end(),
                    "channel_mdn_job:"
                    "allocation_engine_active_derisk_reason_visibility_only") !=
              attention_allocation_summary.issues.end(),
      "allocation summaries surface fallback, de-risk, and growth-floor "
      "attention as warning-only diagnostics");
  auto bad_allocation_engine = mdn_allocation_engine;
  bad_allocation_engine.execution_authority = true;
  const auto bad_allocation_engine_summary =
      exposure::summarize_allocation_engines({channel_mdn_fact},
                                             {bad_allocation_engine});
  check(bad_allocation_engine_summary.execution_authority &&
            bad_allocation_engine_summary.warning_count > 0 &&
            !bad_allocation_engine_summary.issues.empty(),
        "allocation engine authority drift is reported as warnings, not as "
        "execution routing or market readiness");
  auto bad_reserve_allocation_engine = mdn_allocation_engine;
  bad_reserve_allocation_engine.reserve_node_source = "allocator_output";
  bad_reserve_allocation_engine.base_policy_reserve_node_id = "EUR_CASH";
  bad_reserve_allocation_engine.reserve_node_graph_bound = false;
  const auto bad_reserve_allocation_summary =
      exposure::summarize_allocation_engines({channel_mdn_fact},
                                             {bad_reserve_allocation_engine});
  check(bad_reserve_allocation_summary.reserve_node_source_mismatch_count ==
                1 &&
            bad_reserve_allocation_summary
                    .reserve_node_base_policy_mismatch_count == 1 &&
            bad_reserve_allocation_summary.reserve_node_not_graph_bound_count ==
                1 &&
            std::find(bad_reserve_allocation_summary.issues.begin(),
                      bad_reserve_allocation_summary.issues.end(),
                      "channel_mdn_job:reserve_node_source_not_base_policy") !=
                bad_reserve_allocation_summary.issues.end() &&
            std::find(bad_reserve_allocation_summary.issues.begin(),
                      bad_reserve_allocation_summary.issues.end(),
                      "channel_mdn_job:reserve_node_base_policy_mismatch") !=
                bad_reserve_allocation_summary.issues.end() &&
            std::find(bad_reserve_allocation_summary.issues.begin(),
                      bad_reserve_allocation_summary.issues.end(),
                      "channel_mdn_job:reserve_node_not_graph_bound") !=
                bad_reserve_allocation_summary.issues.end(),
        "allocation summaries warn when the reserve node is not graph-bound "
        "BasePolicy evidence");
  auto malformed_allocation_engine = mdn_allocation_engine;
  malformed_allocation_engine.source_cursor_token.clear();
  malformed_allocation_engine.reserve_node_id.clear();
  malformed_allocation_engine.reserve_node_source.clear();
  malformed_allocation_engine.base_policy_reserve_node_id.clear();
  malformed_allocation_engine.reserve_node_graph_bound = false;
  malformed_allocation_engine.observer_belief_fact_digest.clear();
  malformed_allocation_engine.forecast_artifact_digest.clear();
  malformed_allocation_engine.base_policy_digest.clear();
  malformed_allocation_engine.objective_terms.clear();
  malformed_allocation_engine.cvar_loss =
      std::numeric_limits<double>::quiet_NaN();
  malformed_allocation_engine.transaction_cost_estimate =
      std::numeric_limits<double>::quiet_NaN();
  malformed_allocation_engine.constraint_diagnostics.clear();
  malformed_allocation_engine.fallback_reasons.clear();
  malformed_allocation_engine.derisk_reasons.clear();
  malformed_allocation_engine.deterministic_artifact = false;
  malformed_allocation_engine.execution_authority = true;
  const auto malformed_allocation_engine_issues =
      exposure::allocation_engine_fact_issues(malformed_allocation_engine);
  std::vector<std::string> malformed_allocation_engine_warnings;
  exposure::append_allocation_engine_scan_warning(
      malformed_allocation_engine, root / "malformed_allocation_engine",
      malformed_allocation_engine_warnings);
  check(contains_text(malformed_allocation_engine_issues,
                      "missing_source_cursor_token") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_reserve_graph_node") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_reserve_node_source") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_base_policy_reserve_node_id") &&
            contains_text(malformed_allocation_engine_issues,
                          "reserve_node_not_graph_bound") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_observer_belief_fact_digest") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_forecast_artifact_digest") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_base_policy_digest") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_objective_terms") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_cvar_loss") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_transaction_cost_estimate") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_constraint_diagnostics") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_fallback_reason_contract") &&
            contains_text(malformed_allocation_engine_issues,
                          "missing_derisk_reason_contract") &&
            contains_text(malformed_allocation_engine_issues,
                          "allocation_engine_must_remain_audit_evidence_"
                          "only") &&
            malformed_allocation_engine_warnings.size() == 1 &&
            contains_text(malformed_allocation_engine_warnings,
                          "missing_reserve_graph_node"),
        "malformed allocation engine facts remain visible but warn for missing "
        "reserve, lineage, base-policy, objective, cost, and reason contracts");
  const auto scan_replay_environment_summary =
      exposure::summarize_replay_environments(
          parked_environment_scan.ledger.facts(),
          parked_environment_scan.ledger.replay_environment_facts());
  const auto &mdn_replay_environment =
      parked_environment_scan.ledger.replay_environment_facts().front();
  check(
      mdn_replay_environment.parent_exposure_fact_digest ==
              exposure::exposure_fact_digest(channel_mdn_fact) &&
          mdn_replay_environment.protocol_id == channel_mdn_fact.protocol_id &&
          mdn_replay_environment.graph_order_fingerprint ==
              channel_mdn_fact.graph_order_fingerprint &&
          mdn_replay_environment.source_cursor_token ==
              channel_mdn_fact.source_cursor_token &&
          mdn_replay_environment.experiment_id == "replay_experiment_1" &&
          mdn_replay_environment.runtime_run_id == "runtime_run_1" &&
          mdn_replay_environment.environment_run_id == "replay_env_1" &&
          mdn_replay_environment.execution_profile_digest ==
              "execution_profile_digest_1" &&
          mdn_replay_environment.policy_set_digest == "policy_set_digest_1" &&
          mdn_replay_environment.runtime_replay_batch_index_schema ==
              "kikijyeba.environment.replay.runtime_batch_index.v1" &&
          mdn_replay_environment.runtime_replay_experiment_index_schema ==
              "kikijyeba.environment.replay.runtime_experiment_index.v1" &&
          mdn_replay_environment.replay_experiment_report_schema ==
              exposure::replay_environment_experiment_report_schema_id() &&
          mdn_replay_environment.experiment_index_report_digest ==
              replay_report_digest &&
          mdn_replay_environment.experiment_report_digest ==
              replay_report_digest &&
          mdn_replay_environment.replay_environment_version ==
              "kikijyeba.environment.replay.v1" &&
          mdn_replay_environment.replay_environment_component_assembly_id ==
              "replay_environment_v1" &&
          mdn_replay_environment.replay_environment_world_mode ==
              "historical_replay" &&
          mdn_replay_environment.replay_environment_api_contract ==
              "rl_compatible_reset_step" &&
          mdn_replay_environment.replay_environment_range_source ==
              "ujcamei_component_stream_cursor" &&
          mdn_replay_environment.replay_environment_source_range_policy ==
              "anchor_index_or_source_key" &&
          mdn_replay_environment.replay_environment_source_order_policy ==
              "sequential" &&
          mdn_replay_environment.replay_environment_observation_time_law ==
              "time_t_only" &&
          mdn_replay_environment.replay_environment_realization_reveal ==
              "after_action_execution" &&
          mdn_replay_environment.replay_environment_realization_key_policy ==
              "shared_key_per_frame" &&
          mdn_replay_environment.replay_environment_action_kind ==
              "target_node_weights_with_base_reserve" &&
          mdn_replay_environment.replay_environment_action_time_policy ==
              "decision_timestamp_after_knowledge_before_realization" &&
          mdn_replay_environment.replay_environment_reserve_node_policy ==
              "graph_node_from_base_policy" &&
          mdn_replay_environment
                  .replay_environment_graph_node_universe_policy ==
              "episode_spec_graph_node_ids" &&
          mdn_replay_environment.replay_environment_policy_surface ==
              "policy_adapter" &&
          mdn_replay_environment.replay_environment_action_policy_identity ==
              "policy_adapter_must_match_action" &&
          mdn_replay_environment.replay_environment_initial_policy_kind ==
              "deterministic_allocator_or_baseline" &&
          mdn_replay_environment.replay_environment_experiment_task_identity ==
              "bundle_policy_task_indices" &&
          mdn_replay_environment.replay_environment_experiment_run_identity ==
              "single_runtime_environment_run" &&
          mdn_replay_environment.replay_environment_step_artifact_identity ==
              "episode_run_policy_cursor" &&
          mdn_replay_environment
                  .replay_environment_experiment_report_count_policy ==
              "counts_match_evidence" &&
          mdn_replay_environment.replay_environment_artifact_schema ==
              "cajtucu_ready_replay_artifacts" &&
          mdn_replay_environment.replay_environment_lattice_target ==
              "replay_environment_artifact_ready" &&
          mdn_replay_environment.replay_environment_require_resolved_cursor &&
          mdn_replay_environment.replay_environment_require_no_future_leakage &&
          mdn_replay_environment
              .replay_environment_require_projection_validation &&
          mdn_replay_environment.replay_environment_default_max_parallel_jobs ==
              1 &&
          mdn_replay_environment.experiment_requested_max_parallel_jobs == 2 &&
          mdn_replay_environment.experiment_resolved_parallelism == 2 &&
          mdn_replay_environment.batch_entry_count == 1 &&
          mdn_replay_environment.experiment_entry_count == 1 &&
          mdn_replay_environment.replay_bundle_count == 2 &&
          mdn_replay_environment.policy_count == 2 &&
          mdn_replay_environment.attempted_count == 2 &&
          mdn_replay_environment.completed_count == 2 &&
          mdn_replay_environment.episode_count == 2 &&
          mdn_replay_environment.episode_requested_range_bound_count == 2 &&
          mdn_replay_environment.episode_cursor_bound_count == 2 &&
          mdn_replay_environment.episode_anchor_interval_bound_count == 2 &&
          mdn_replay_environment.episode_anchor_keys_bound_count == 2 &&
          mdn_replay_environment.time_law_expected_step_count == 4 &&
          mdn_replay_environment.time_law_observation_step_count == 4 &&
          mdn_replay_environment.time_law_action_step_count == 4 &&
          mdn_replay_environment.time_law_execution_step_count == 4 &&
          mdn_replay_environment.time_law_realization_after_action_count == 4 &&
          mdn_replay_environment.time_law_future_observation_violation_count ==
              0 &&
          mdn_replay_environment.mixed_future_realization_key_count == 0 &&
          mdn_replay_environment.projection_validation_step_count == 4 &&
          mdn_replay_environment.cajtucu_valid_trace_count == 4 &&
          mdn_replay_environment.cajtucu_invalid_trace_count == 0 &&
          mdn_replay_environment.cajtucu_missing_direct_reserve_edge_count ==
              0 &&
          mdn_replay_environment.cajtucu_nontradable_edge_reject_count == 0 &&
          mdn_replay_environment.cajtucu_below_min_notional_reject_count == 0 &&
          mdn_replay_environment.cajtucu_above_max_notional_reject_count == 0 &&
          mdn_replay_environment.cajtucu_insufficient_reserve_reject_count ==
              0 &&
          mdn_replay_environment.cajtucu_insufficient_units_reject_count == 0 &&
          mdn_replay_environment.cajtucu_invalid_sell_price_count == 0 &&
          mdn_replay_environment.cajtucu_large_equity_mismatch_count == 0 &&
          mdn_replay_environment.cajtucu_synthetic_market_step_count == 0 &&
          mdn_replay_environment.requested_order_count == 4 &&
          mdn_replay_environment.executed_order_count == 4 &&
          mdn_replay_environment.rejected_order_count == 0 &&
          mdn_replay_environment.partial_order_count == 0 &&
          std::abs(mdn_replay_environment.mean_total_reward - 1.5) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_total_log_growth - 0.03) <
              1e-12 &&
          std::abs(mdn_replay_environment.mean_final_equity_base - 103.0) <
              1e-12 &&
          std::abs(mdn_replay_environment.mean_max_drawdown - 0.08) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_total_turnover - 0.40) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_total_transaction_cost_base -
                   0.015) < 1e-12 &&
          std::abs(mdn_replay_environment.requested_notional_base - 10.0) <
              1e-12 &&
          std::abs(mdn_replay_environment.executed_notional_base - 9.8) <
              1e-12 &&
          std::abs(mdn_replay_environment.rejected_notional_base - 0.0) <
              1e-12 &&
          std::abs(mdn_replay_environment.fill_ratio - 0.98) < 1e-12 &&
          std::abs(mdn_replay_environment.fee_cost_base - 0.006) < 1e-12 &&
          std::abs(mdn_replay_environment.spread_cost_base - 0.005) < 1e-12 &&
          std::abs(mdn_replay_environment.slippage_cost_base - 0.004) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_target_weight_error_l1 - 0.02) <
              1e-12 &&
          std::abs(mdn_replay_environment.mean_target_weight_error_linf -
                   0.01) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_projection_mae - 0.012) <
              1e-12 &&
          std::abs(mdn_replay_environment.mean_projection_signed_bias + 0.002) <
              1e-12 &&
          std::abs(mdn_replay_environment.mean_projection_directional_accuracy -
                   0.75) < 1e-12 &&
          std::abs(mdn_replay_environment.mean_projection_interval_coverage -
                   0.80) < 1e-12 &&
          mdn_replay_environment.artifact_evidence &&
          mdn_replay_environment.visibility_only &&
          !mdn_replay_environment.replay_executor &&
          !mdn_replay_environment.allocation_authority &&
          !mdn_replay_environment.execution_authority &&
          !mdn_replay_environment.readiness_authority &&
          !mdn_replay_environment.quality_authority &&
          !mdn_replay_environment.performance_authority &&
          !mdn_replay_environment.market_readiness_authority &&
          !mdn_replay_environment.deployment_authority &&
          !mdn_replay_environment.checkpoint_selector &&
          !mdn_replay_environment.coverage_authority &&
          !mdn_replay_environment.leakage_authority &&
          !mdn_replay_environment.contract_identity_authority &&
          exposure::replay_environment_fact_digest(mdn_replay_environment)
                  .size() == 16,
      "replay environment facts bind replay indexes and experiment reports "
      "to parent runtime evidence without becoming execution authority");
  check(
      scan_replay_environment_summary.schema ==
              "kikijyeba.lattice.replay_environment_summary.v1" &&
          scan_replay_environment_summary.exposure_fact_count == 3 &&
          scan_replay_environment_summary.replay_environment_fact_count == 1 &&
          scan_replay_environment_summary.parent_exposure_fact_count == 1 &&
          scan_replay_environment_summary.batch_index_bound_count == 1 &&
          scan_replay_environment_summary.experiment_index_bound_count == 1 &&
          scan_replay_environment_summary.experiment_report_bound_count == 1 &&
          scan_replay_environment_summary.batch_index_schema_bound_count == 1 &&
          scan_replay_environment_summary.experiment_index_schema_bound_count ==
              1 &&
          scan_replay_environment_summary
                  .experiment_report_schema_bound_count == 1 &&
          scan_replay_environment_summary
                  .experiment_index_report_digest_bound_count == 1 &&
          scan_replay_environment_summary
                  .experiment_report_digest_bound_count == 1 &&
          scan_replay_environment_summary
                  .experiment_report_digest_match_count == 1 &&
          scan_replay_environment_summary
                  .experiment_report_digest_mismatch_count == 0 &&
          scan_replay_environment_summary.experiment_id_bound_count == 1 &&
          scan_replay_environment_summary.runtime_run_id_bound_count == 1 &&
          scan_replay_environment_summary.environment_run_id_bound_count == 1 &&
          scan_replay_environment_summary
                  .execution_profile_digest_bound_count == 1 &&
          scan_replay_environment_summary.policy_set_digest_bound_count == 1 &&
          scan_replay_environment_summary.replay_contract_version_bound_count ==
              1 &&
          scan_replay_environment_summary
                  .replay_contract_component_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_world_mode_bound_count == 1 &&
          scan_replay_environment_summary.replay_contract_api_bound_count ==
              1 &&
          scan_replay_environment_summary
                  .replay_contract_spawn_model_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_range_source_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_source_range_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_source_order_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_range_resolution_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_policy_surface_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_time_law_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_realization_reveal_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_realization_key_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_action_kind_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_action_time_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_reserve_node_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_graph_node_universe_policy_bound_count ==
              1 &&
          scan_replay_environment_summary
                  .replay_contract_reward_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_projection_validation_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_action_policy_identity_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_initial_policy_kind_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_task_identity_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_experiment_run_identity_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_step_artifact_identity_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_report_count_policy_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_parallelism_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_artifact_schema_bound_count == 1 &&
          scan_replay_environment_summary
                  .replay_contract_lattice_binding_bound_count == 1 &&
          scan_replay_environment_summary.replay_contract_guard_bound_count ==
              1 &&
          scan_replay_environment_summary.completed_all_attempted_count == 1 &&
          scan_replay_environment_summary
                  .projection_validation_metric_bound_count == 1 &&
          scan_replay_environment_summary.time_law_step_evidence_bound_count ==
              1 &&
          scan_replay_environment_summary
                  .projection_validation_step_evidence_bound_count == 1 &&
          scan_replay_environment_summary
                  .episode_requested_range_bound_count_total == 2 &&
          scan_replay_environment_summary.episode_cursor_bound_count_total ==
              2 &&
          scan_replay_environment_summary
                  .episode_anchor_interval_bound_count_total == 2 &&
          scan_replay_environment_summary
                  .episode_anchor_keys_bound_count_total == 2 &&
          scan_replay_environment_summary.time_law_expected_step_count_total ==
              4 &&
          scan_replay_environment_summary
                  .time_law_observation_step_count_total == 4 &&
          scan_replay_environment_summary.time_law_action_step_count_total ==
              4 &&
          scan_replay_environment_summary.time_law_execution_step_count_total ==
              4 &&
          scan_replay_environment_summary
                  .time_law_realization_after_action_count_total == 4 &&
          scan_replay_environment_summary
                  .time_law_future_observation_violation_count_total == 0 &&
          scan_replay_environment_summary
                  .mixed_future_realization_key_count_total == 0 &&
          scan_replay_environment_summary
                  .projection_validation_step_count_total == 4 &&
          scan_replay_environment_summary.cajtucu_valid_trace_count_total ==
              4 &&
          scan_replay_environment_summary.cajtucu_invalid_trace_count_total ==
              0 &&
          scan_replay_environment_summary
                  .cajtucu_synthetic_market_step_count_total == 0 &&
          scan_replay_environment_summary.requested_order_count_total == 4 &&
          scan_replay_environment_summary.executed_order_count_total == 4 &&
          scan_replay_environment_summary.rejected_order_count_total == 0 &&
          scan_replay_environment_summary.partial_order_count_total == 0 &&
          scan_replay_environment_summary.missing_batch_index_count == 0 &&
          scan_replay_environment_summary.missing_experiment_index_count == 0 &&
          scan_replay_environment_summary.missing_experiment_report_count ==
              0 &&
          scan_replay_environment_summary
                  .missing_execution_profile_digest_count == 0 &&
          scan_replay_environment_summary.missing_policy_set_digest_count ==
              0 &&
          scan_replay_environment_summary.incomplete_replay_attempt_count ==
              0 &&
          scan_replay_environment_summary
                  .missing_projection_validation_metric_count == 0 &&
          scan_replay_environment_summary
                  .missing_time_law_step_evidence_count == 0 &&
          scan_replay_environment_summary
                  .missing_projection_validation_step_evidence_count == 0 &&
          scan_replay_environment_summary
                  .missing_cajtucu_trace_evidence_count == 0 &&
          scan_replay_environment_summary.cajtucu_invalid_trace_fact_count ==
              0 &&
          scan_replay_environment_summary.cajtucu_synthetic_market_fact_count ==
              0 &&
          scan_replay_environment_summary.missing_cajtucu_cost_metric_count ==
              0 &&
          scan_replay_environment_summary.invalid_cajtucu_order_metric_count ==
              0 &&
          scan_replay_environment_summary
                  .invalid_cajtucu_notional_metric_count == 0 &&
          scan_replay_environment_summary.invalid_cajtucu_fill_ratio_count ==
              0 &&
          scan_replay_environment_summary
                  .invalid_cajtucu_target_tracking_count == 0 &&
          scan_replay_environment_summary.time_law_violation_count == 0 &&
          scan_replay_environment_summary
                  .missing_episode_requested_range_count == 0 &&
          scan_replay_environment_summary
                  .missing_episode_cursor_evidence_count == 0 &&
          scan_replay_environment_summary
                  .missing_episode_anchor_interval_count == 0 &&
          scan_replay_environment_summary.missing_episode_anchor_keys_count ==
              0 &&
          scan_replay_environment_summary.artifact_evidence_count == 1 &&
          scan_replay_environment_summary.batch_entry_count_total == 1 &&
          scan_replay_environment_summary.experiment_entry_count_total == 1 &&
          scan_replay_environment_summary.replay_bundle_count_total == 2 &&
          scan_replay_environment_summary.policy_count_total == 2 &&
          scan_replay_environment_summary.attempted_count_total == 2 &&
          scan_replay_environment_summary.completed_count_total == 2 &&
          scan_replay_environment_summary.mean_total_reward.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_total_reward.mean -
                   1.5) < 1e-12 &&
          scan_replay_environment_summary.mean_total_log_growth.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_total_log_growth.mean -
                   0.03) < 1e-12 &&
          scan_replay_environment_summary.mean_final_equity_base.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_final_equity_base.mean -
                   103.0) < 1e-12 &&
          scan_replay_environment_summary.mean_max_drawdown.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_max_drawdown.mean -
                   0.08) < 1e-12 &&
          scan_replay_environment_summary.mean_total_turnover.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_total_turnover.mean -
                   0.40) < 1e-12 &&
          scan_replay_environment_summary.mean_total_transaction_cost_base
                  .count == 1 &&
          std::abs(scan_replay_environment_summary
                       .mean_total_transaction_cost_base.mean -
                   0.015) < 1e-12 &&
          scan_replay_environment_summary.requested_notional_base.count == 1 &&
          std::abs(
              scan_replay_environment_summary.requested_notional_base.mean -
              10.0) < 1e-12 &&
          scan_replay_environment_summary.executed_notional_base.count == 1 &&
          std::abs(scan_replay_environment_summary.executed_notional_base.mean -
                   9.8) < 1e-12 &&
          scan_replay_environment_summary.fill_ratio.count == 1 &&
          std::abs(scan_replay_environment_summary.fill_ratio.mean - 0.98) <
              1e-12 &&
          scan_replay_environment_summary.fee_cost_base.count == 1 &&
          std::abs(scan_replay_environment_summary.fee_cost_base.mean - 0.006) <
              1e-12 &&
          scan_replay_environment_summary.spread_cost_base.count == 1 &&
          std::abs(scan_replay_environment_summary.spread_cost_base.mean -
                   0.005) < 1e-12 &&
          scan_replay_environment_summary.slippage_cost_base.count == 1 &&
          std::abs(scan_replay_environment_summary.slippage_cost_base.mean -
                   0.004) < 1e-12 &&
          scan_replay_environment_summary.mean_target_weight_error_l1.count ==
              1 &&
          std::abs(
              scan_replay_environment_summary.mean_target_weight_error_l1.mean -
              0.02) < 1e-12 &&
          scan_replay_environment_summary.mean_target_weight_error_linf.count ==
              1 &&
          std::abs(scan_replay_environment_summary.mean_target_weight_error_linf
                       .mean -
                   0.01) < 1e-12 &&
          scan_replay_environment_summary.mean_projection_mae.count == 1 &&
          std::abs(scan_replay_environment_summary.mean_projection_mae.mean -
                   0.012) < 1e-12 &&
          scan_replay_environment_summary.mean_projection_signed_bias.count ==
              1 &&
          std::abs(
              scan_replay_environment_summary.mean_projection_signed_bias.mean +
              0.002) < 1e-12 &&
          scan_replay_environment_summary.mean_projection_directional_accuracy
                  .count == 1 &&
          std::abs(scan_replay_environment_summary
                       .mean_projection_directional_accuracy.mean -
                   0.75) < 1e-12 &&
          scan_replay_environment_summary.mean_projection_interval_coverage
                  .count == 1 &&
          std::abs(scan_replay_environment_summary
                       .mean_projection_interval_coverage.mean -
                   0.80) < 1e-12 &&
          scan_replay_environment_summary.artifact_evidence &&
          scan_replay_environment_summary.visibility_only &&
          !scan_replay_environment_summary.replay_executor &&
          !scan_replay_environment_summary.allocation_authority &&
          !scan_replay_environment_summary.execution_authority &&
          !scan_replay_environment_summary.readiness_authority &&
          !scan_replay_environment_summary.quality_authority &&
          !scan_replay_environment_summary.performance_authority &&
          !scan_replay_environment_summary.market_readiness_authority &&
          !scan_replay_environment_summary.deployment_authority &&
          !scan_replay_environment_summary.checkpoint_selector &&
          !scan_replay_environment_summary.coverage_authority &&
          !scan_replay_environment_summary.leakage_authority &&
          !scan_replay_environment_summary.contract_identity_authority &&
          scan_replay_environment_summary.warning_count == 0 &&
          scan_replay_environment_summary.issues.empty(),
      "replay environment summary exposes replay-loop metrics as read-only "
      "artifact evidence, not portfolio acceptance");
  auto bad_replay_environment = mdn_replay_environment;
  bad_replay_environment.replay_executor = true;
  const auto bad_replay_environment_summary =
      exposure::summarize_replay_environments({channel_mdn_fact},
                                              {bad_replay_environment});
  check(bad_replay_environment_summary.replay_executor &&
            bad_replay_environment_summary.warning_count > 0 &&
            !bad_replay_environment_summary.issues.empty(),
        "replay environment authority drift is reported as warnings, not as "
        "Runtime execution or market readiness");
  auto bad_time_law_replay_environment = mdn_replay_environment;
  bad_time_law_replay_environment.time_law_future_observation_violation_count =
      1;
  const auto bad_time_law_replay_environment_summary =
      exposure::summarize_replay_environments(
          {channel_mdn_fact}, {bad_time_law_replay_environment});
  check(bad_time_law_replay_environment_summary.time_law_violation_count == 1 &&
            bad_time_law_replay_environment_summary.warning_count > 0 &&
            !bad_time_law_replay_environment_summary.issues.empty(),
        "replay environment time-law violations are exposed as lattice "
        "warnings for parked environment audit visibility");
  auto missing_profile_replay_environment = mdn_replay_environment;
  missing_profile_replay_environment.execution_profile_digest.clear();
  missing_profile_replay_environment.policy_set_digest.clear();
  const auto missing_profile_issues = exposure::replay_environment_fact_issues(
      missing_profile_replay_environment);
  check(contains_text(missing_profile_issues,
                      "missing_execution_profile_digest") &&
            contains_text(missing_profile_issues, "missing_policy_set_digest"),
        "replay environment artifact readiness requires execution profile and "
        "policy set digest binding");
  auto bad_cajtucu_replay_environment = mdn_replay_environment;
  bad_cajtucu_replay_environment.cajtucu_valid_trace_count = 3;
  bad_cajtucu_replay_environment.cajtucu_invalid_trace_count = 1;
  bad_cajtucu_replay_environment.cajtucu_synthetic_market_step_count = 1;
  const auto bad_cajtucu_issues =
      exposure::replay_environment_fact_issues(bad_cajtucu_replay_environment);
  check(contains_text(bad_cajtucu_issues, "cajtucu_invalid_trace_evidence") &&
            contains_text(bad_cajtucu_issues, "cajtucu_synthetic_market_used"),
        "replay environment artifact readiness fails closed on invalid "
        "Cajtucu traces and synthetic execution markets");
  auto missing_cost_replay_environment = mdn_replay_environment;
  missing_cost_replay_environment.requested_order_count = 0;
  missing_cost_replay_environment.mean_total_transaction_cost_base =
      std::numeric_limits<double>::quiet_NaN();
  missing_cost_replay_environment.fill_ratio =
      std::numeric_limits<double>::quiet_NaN();
  const auto missing_cost_issues =
      exposure::replay_environment_fact_issues(missing_cost_replay_environment);
  check(contains_text(missing_cost_issues, "missing_cajtucu_order_evidence") &&
            contains_text(missing_cost_issues,
                          "missing_or_invalid_cajtucu_cost_metrics") &&
            contains_text(missing_cost_issues, "invalid_cajtucu_fill_ratio"),
        "replay environment artifact readiness requires cost-aware order, "
        "cost, and fill-ratio evidence");

  check(rep_fact.target_component_family_id ==
            "wikimyei.representation.encoding.vicreg",
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
            rep_receipt.protocol_id == rep_fact.protocol_id &&
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
          selection_signal_facts.front().protocol_id ==
              selection_fact.protocol_id &&
          selection_signal_facts.front().selector_id == selection_fact.job_id &&
          selection_signal_facts.front().selector_kind ==
              "checkpoint_selection" &&
          selection_signal_facts.front().selector_split ==
              selection_fact.split_name &&
          selection_signal_facts.front().selector_metric ==
              "runtime_output_checkpoint_identity" &&
          selection_signal_facts.front().tie_policy ==
              "not_applicable_single_output_checkpoint" &&
          selection_signal_facts.front().selected_checkpoint ==
              selection_fact.output_checkpoint &&
          selection_signal_facts.front().selected_checkpoint_source ==
              "output_checkpoint" &&
          selection_signal_facts.front().candidate_checkpoint_count == 1 &&
          selection_signal_facts.front().candidate_checkpoints.size() == 1 &&
          selection_signal_facts.front().candidate_checkpoints.front() ==
              selection_fact.output_checkpoint.lexically_normal() &&
          selection_signal_facts.front().candidate_checkpoint_digest.size() ==
              16 &&
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
  check(
      selection_signal_ledger.selection_signal_facts().size() == 1 &&
          selection_signal_summary.selection_signal_fact_count == 1 &&
          selection_signal_summary.unique_selector_count == 1 &&
          selection_signal_summary.selected_checkpoint_count == 1 &&
          selection_signal_summary.missing_selected_checkpoint_count == 0 &&
          selection_signal_summary.selector_split_bound_count == 1 &&
          selection_signal_summary.selector_metric_bound_count == 1 &&
          selection_signal_summary.tie_policy_bound_count == 1 &&
          selection_signal_summary.candidate_checkpoint_bound_count == 1 &&
          selection_signal_summary.candidate_checkpoint_digest_bound_count ==
              1 &&
          selection_signal_summary.missing_candidate_checkpoint_count == 0 &&
          selection_signal_summary.missing_candidate_checkpoint_digest_count ==
              0 &&
          selection_signal_summary.candidate_checkpoint_digest_mismatch_count ==
              0 &&
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
            incomplete_selection_summary.missing_candidate_checkpoint_count ==
                1 &&
            std::any_of(incomplete_selection_summary.issues.begin(),
                        incomplete_selection_summary.issues.end(),
                        [](const auto &issue) {
                          return issue.find("missing_selected_checkpoint") !=
                                 std::string::npos;
                        }),
        "selection_signal events without a selected checkpoint stay visible as "
        "incomplete provenance");
  auto checkpoint_audit_fact = selection_fact;
  checkpoint_audit_fact.use.selection_signal = false;
  checkpoint_audit_fact.use.evaluation_metric = true;
  checkpoint_audit_fact.output_checkpoint.clear();
  checkpoint_audit_fact.input_checkpoints = {rep_dir / "z.pt",
                                             rep_dir / "a.pt"};
  const auto checkpoint_audit_signals =
      exposure::make_selection_signal_facts_from_exposure_fact(
          checkpoint_audit_fact);
  check(
      checkpoint_audit_signals.size() == 1 &&
          checkpoint_audit_signals.front().selector_rule ==
              "derived_evaluation_checkpoint_audit" &&
          checkpoint_audit_signals.front().selector_metric ==
              "runtime_loaded_checkpoint_identity" &&
          checkpoint_audit_signals.front().tie_policy ==
              "deterministic_input_checkpoint_path_order_no_ranking" &&
          checkpoint_audit_signals.front().candidate_checkpoint_count == 2 &&
          checkpoint_audit_signals.front().candidate_checkpoint_digest.size() ==
              16 &&
          checkpoint_audit_signals.front().selected_checkpoint ==
              (rep_dir / "a.pt").lexically_normal() &&
          checkpoint_audit_signals.front().selected_checkpoint_source ==
              "loaded_checkpoint",
      "evaluation checkpoint audits expose loaded-candidate identity without "
      "claiming best-checkpoint selection");
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
  check(representation_support_fact.protocol_id == rep_fact.protocol_id,
        "representation support fact inherits parent protocol id");
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
            node_fact_it->protocol_id == rep_fact.protocol_id &&
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
  check(exposure::canonical_representation_support_fact_text(
            representation_support_fact)
                .find("protocol_id=cwu_01v") != std::string::npos,
        "representation support canonical text includes protocol identity");

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
  check(parsed_checkpoint_fact.protocol_id == rep_fact.protocol_id,
        "checkpoint fact round-trips protocol id");
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

  check(mdn_fact.target_component_family_id ==
            "wikimyei.inference.expected_value.mdn",
        "MDN component decoded");
  check(mdn_fact.use.observed_input, "MDN consumes observed context");
  check(mdn_fact.use.target_supervision, "MDN consumes future supervision");
  check(mdn_fact.target_footprint.begin == 201 &&
            mdn_fact.target_footprint.end == 321,
        "MDN target source-row footprint decoded");
  check(mdn_fact.valid_target_count == 60, "MDN valid target count decoded");
  check(std::abs(mdn_fact.valid_target_fraction - 0.50) < 1e-12,
        "MDN valid target fraction decoded");
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
  check(
      channel_mdn_fact.inference_health_available &&
          std::abs(channel_mdn_fact.mean_sigma_mean - 0.80) < 1e-12 &&
          std::abs(channel_mdn_fact.min_sigma_min - 0.10) < 1e-12 &&
          std::abs(channel_mdn_fact.max_sigma_max - 1.50) < 1e-12 &&
          std::abs(channel_mdn_fact.mean_mixture_entropy - 0.60) < 1e-12 &&
          channel_mdn_fact.mean_nll_per_channel.size() == 3 &&
          std::abs(channel_mdn_fact.mean_nll_per_channel[2] - 0.80) < 1e-12 &&
          channel_mdn_fact.mean_nll_per_target_feature.size() == 2 &&
          std::abs(channel_mdn_fact.mean_nll_per_target_feature[1] - 0.70) <
              1e-12 &&
          channel_mdn_fact.mean_nll_per_channel_target_feature.size() == 6 &&
          std::abs(channel_mdn_fact.mean_nll_per_channel_target_feature[5] -
                   0.82) < 1e-12 &&
          channel_mdn_fact.mean_mixture_usage.size() == 2 &&
          std::abs(channel_mdn_fact.mean_mixture_usage[0] - 0.25) < 1e-12 &&
          channel_mdn_fact.valid_target_count_per_channel.size() == 3 &&
          channel_mdn_fact.valid_target_count_per_channel[2] == 30 &&
          channel_mdn_fact.valid_target_count_per_target_feature.size() == 2 &&
          channel_mdn_fact.valid_target_count_per_target_feature[1] == 33 &&
          channel_mdn_fact.valid_target_count_per_channel_target_feature
                  .size() == 6 &&
          channel_mdn_fact.valid_target_count_per_channel_target_feature[5] ==
              16 &&
          channel_mdn_fact.mdn_architecture ==
              "shared_slot_trunk.channel_adapter.shared_feature_head.v2" &&
          channel_mdn_fact.loss_reduction == "balanced_channel_feature_mean" &&
          channel_mdn_fact.feature_embedding_dim == 4 &&
          channel_mdn_fact.channel_adapter_rank == 4 &&
          channel_mdn_fact.shared_trunk &&
          channel_mdn_fact.channel_adapters_enabled &&
          channel_mdn_fact.shared_feature_head &&
          channel_mdn_fact.feature_embedding_enabled &&
          !channel_mdn_fact.node_id_embedding &&
          !channel_mdn_fact.cross_node_attention &&
          !channel_mdn_fact.cross_channel_attention &&
          channel_mdn_fact.nonfinite_output_count == 0 &&
          std::abs(channel_mdn_fact.last_grad_norm - 1.20) < 1e-12 &&
          std::abs(channel_mdn_fact.max_grad_norm - 1200.0) < 1e-12 &&
          std::abs(channel_mdn_fact.grad_clip_norm - 5.0) < 1e-12 &&
          channel_mdn_fact.finite_parameter_check,
      "channel MDN health fields prefer Runtime terminal facts and keep "
      "strict-channel report details");
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

  const auto channel_mdn_checkpoint_fact =
      exposure::make_checkpoint_fact_from_exposure_fact(channel_mdn_fact);
  const auto channel_mdn_checkpoint_text =
      exposure::canonical_checkpoint_fact_text(channel_mdn_checkpoint_fact);
  check(channel_mdn_checkpoint_fact.input_representation_assembly_id ==
                "vicreg_v1" &&
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
      exposure::anchor_interval_t{.begin = 200, .end = 320},
      exposure::exposure_use_t::target_supervision,
      /*require_mutated_component=*/false);
  const auto [mdn_support_lower, mdn_support_upper] =
      exposure::wilson_score_interval_95(60, 120);
  check(mdn_target_support_summary.valid_target_success_count_for_uncertainty ==
                60 &&
            mdn_target_support_summary
                    .valid_target_opportunity_count_for_uncertainty == 120,
        "MDN exposure summary derives valid-target support counts for "
        "uncertainty");
  check(std::abs(mdn_target_support_summary.valid_target_fraction_estimate -
                 0.50) < 1e-12 &&
            std::abs(mdn_target_support_summary.valid_target_wilson_lower_95 -
                     mdn_support_lower) < 1e-12 &&
            std::abs(mdn_target_support_summary.valid_target_wilson_upper_95 -
                     mdn_support_upper) < 1e-12,
        "MDN exposure summary reports valid-target Wilson interval");
  auto outside_mdn_support_fact = mdn_fact;
  outside_mdn_support_fact.job_id = "outside_mdn_support";
  outside_mdn_support_fact.anchor_range =
      exposure::anchor_interval_t{.begin = 320, .end = 440};
  outside_mdn_support_fact.completed_anchor_range =
      outside_mdn_support_fact.anchor_range;
  outside_mdn_support_fact.target_footprint =
      exposure::anchor_interval_t{.begin = 321, .end = 441};
  outside_mdn_support_fact.valid_target_count = 120;
  outside_mdn_support_fact.valid_target_fraction = 1.0;
  outside_mdn_support_fact.optimizer_steps = 99;
  const auto mdn_target_support_with_outside = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{mdn_fact,
                                                     outside_mdn_support_fact},
      exposure::anchor_interval_t{.begin = 200, .end = 320},
      exposure::exposure_use_t::target_supervision,
      /*require_mutated_component=*/false);
  check(
      mdn_target_support_with_outside.fact_count == 1 &&
          mdn_target_support_with_outside.loaded_anchor_events == 120 &&
          mdn_target_support_with_outside.optimizer_steps_total ==
              mdn_fact.optimizer_steps &&
          mdn_target_support_with_outside.valid_target_count_total == 60 &&
          mdn_target_support_with_outside
                  .valid_target_success_count_for_uncertainty == 60 &&
          mdn_target_support_with_outside
                  .valid_target_opportunity_count_for_uncertainty == 120 &&
          std::abs(mdn_target_support_with_outside.valid_target_cursor_epochs -
                   0.50) < 1e-12 &&
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
      exposure::anchor_interval_t{.begin = 260, .end = 380};
  half_overlap_mdn_support_fact.completed_anchor_range =
      half_overlap_mdn_support_fact.anchor_range;
  half_overlap_mdn_support_fact.target_footprint =
      exposure::anchor_interval_t{.begin = 261, .end = 381};
  half_overlap_mdn_support_fact.valid_target_count = 60;
  half_overlap_mdn_support_fact.valid_target_fraction = 0.5;
  const auto half_overlap_support_summary = exposure::exposure_load_for_use(
      std::vector<exposure::lattice_exposure_fact_t>{
          half_overlap_mdn_support_fact},
      exposure::anchor_interval_t{.begin = 200, .end = 320},
      exposure::exposure_use_t::target_supervision,
      /*require_mutated_component=*/false);
  const auto [half_overlap_lower, half_overlap_upper] =
      exposure::wilson_score_interval_95(30, 60);
  check(
      half_overlap_support_summary.fact_count == 1 &&
          half_overlap_support_summary.loaded_anchor_events == 60 &&
          std::abs(half_overlap_support_summary.cursor_exposure_load - 0.5) <
              1e-12 &&
          half_overlap_support_summary.valid_target_count_total == 30 &&
          half_overlap_support_summary
                  .valid_target_success_count_for_uncertainty == 30 &&
          half_overlap_support_summary
                  .valid_target_opportunity_count_for_uncertainty == 60 &&
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
  const auto closure = ledger.checkpoint_closure(channel_mdn_checkpoint_path);
  check(closure.size() == 2,
        "MDN checkpoint closure includes MDN and representation exposure");
  check(!ledger.checkpoint_closure_digest(channel_mdn_checkpoint_path).empty(),
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
      identity_ledger.checkpoint_closure_result(channel_mdn_checkpoint_path);
  check(identity_closure.complete() && identity_closure.facts.size() == 2,
        "checkpoint id/digest closure remains complete");
  check(identity_closure.resolution_authority == "checkpoint_id_file_digest" &&
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
      wrong_digest_ledger.checkpoint_closure_result(
          channel_mdn_checkpoint_path);
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
      wrong_id_ledger.checkpoint_closure_result(channel_mdn_checkpoint_path);
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
      closure, exposure::anchor_interval_t{.begin = 200, .end = 320},
      exposure::exposure_use_t::target_supervision,
      /*require_mutated_component=*/false);
  check(target_supervision_anchor_coverage.covered_anchors == 120,
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
      exposure::anchor_interval_t{.begin = 250, .end = 260};
  target_query.forbidden_uses = {exposure::exposure_use_t::target_supervision};
  target_query.require_mutated_component = false;
  check(exposure::has_forbidden_exposure_overlap(closure, target_query),
        "closure catches MDN target-supervision overlap");

  exposure::forbidden_exposure_query_t future_boundary_query{};
  future_boundary_query.forbidden_range =
      exposure::anchor_interval_t{.begin = 320, .end = 321};
  future_boundary_query.forbidden_uses = {
      exposure::exposure_use_t::target_supervision};
  future_boundary_query.require_mutated_component = false;
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

  const auto mtf_dir = root / "mtf_representation_job";
  write_text(mtf_dir / "job.manifest",
             "job_id=mtf_rep_job\n"
             "job_kind=channel_representation_mtf_jepa_mae_vicreg\n"
             "target_component_family_id="
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
             "source_cursor_token=cursor_mtf\n"
             "protocol_contract_fingerprint=contract_1\n"
             "mtf_jepa_mae_vicreg_assembly_fingerprint=mtf_1\n");
  write_text(mtf_dir / "job.state", "status=completed\n"
                                    "optimizer_steps=3\n"
                                    "last_loss=0.35\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=mtf.pt\n");
  write_text(mtf_dir / "mtf.pt", "mtf checkpoint");
  const auto mtf_checkpoint_digest =
      exposure::file_content_digest(mtf_dir / "mtf.pt");
  write_text(
      mtf_dir / "channel_representation.report",
      std::string("report_schema_id="
                  "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1\n"
                  "report_schema_version=1\n"
                  "report_writer_id=jkimyei.training.representation."
                  "mtf_jepa_mae_vicreg_graph_first_launcher\n"
                  "report_writer_version=1\n"
                  "optimizer_steps=3\n"
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
                  "anchor_batch_count=4\n"
                  "node_count=2\n"
                  "flattened_sample_count=8\n"
                  "channel_count=3\n"
                  "history_length=16\n"
                  "input_feature_width=5\n"
                  "recommended_graph_restore_shape=[B,N,C,De]\n"
                  "graph_restore_available=false\n"
                  "graph_restore_reason="
                  "runtime_reports_flattened_anchor_node_samples_only\n"
                  "reshape_lossless=true\n"
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
                  "checkpoint_bytes=14\n") +
          "checkpoint_digest_reported=" + mtf_checkpoint_digest + "\n");
  const auto mtf_fact = exposure::make_exposure_fact_from_job_dir(mtf_dir);
  check(mtf_fact.target_component_family_id ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "MTF representation fact preserves target component");
  check(mtf_fact.component_assembly_fingerprint == "mtf_1",
        "MTF representation fact uses MTF assembly fingerprint");
  check(mtf_fact.representation_health_available,
        "MTF representation health is available");
  check(mtf_fact.report_schema_id ==
                "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1" &&
            mtf_fact.report_schema_version == 1,
        "MTF schema identity is parsed");
  check(mtf_fact.representation_value_shape == "[B_flat,De]" &&
            mtf_fact.mtf_training_shape == "[B_flat,C,Hx,F]" &&
            mtf_fact.flattening_contract == "anchor_node_flatten.v1" &&
            mtf_fact.flattened_sample_count ==
                mtf_fact.anchor_batch_count * mtf_fact.node_count,
        "MTF flattened shape facts are parsed");
  check(mtf_fact.checkpoint_digest_reported == mtf_checkpoint_digest &&
            mtf_fact.checkpoint_digest_verified &&
            mtf_fact.checkpoint_file_exists,
        "MTF checkpoint digest facts are parsed");
  check(mtf_fact.run_data_kind == "market" &&
            mtf_fact.readiness_scope == "market_pretraining" &&
            !mtf_fact.market_readiness_claimed,
        "MTF run scope facts are parsed without market-readiness claim");
  check(mtf_fact.gradients_finite && mtf_fact.sample_valid_count == 8 &&
            mtf_fact.channel_valid_count == 24 &&
            mtf_fact.valid_latent_rows == 24,
        "MTF support fields are parsed");
  check(std::isfinite(mtf_fact.loss_jepa_mean) &&
            std::isfinite(mtf_fact.target_ema_distance) &&
            std::isfinite(mtf_fact.latent_std),
        "MTF loss and latent health fields are parsed");

  std::cout << "kikijyeba lattice exposure tests passed\n";
  std::filesystem::remove_all(root);
  return 0;
}
