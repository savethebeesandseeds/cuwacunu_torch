// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "wikimyei/assembly.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

namespace cuwacunu::jkimyei::training::representation {

struct mtf_jepa_mae_vicreg_graph_first_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::hero::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct mtf_jepa_mae_vicreg_graph_first_report_t {
  std::string report_schema_id{
      "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1"};
  int64_t report_schema_version{1};
  std::string report_writer_id{"jkimyei.training.representation."
                               "mtf_jepa_mae_vicreg_graph_first_launcher"};
  std::string report_writer_version{"1"};
  std::string training_id{};
  std::string config_path{};
  std::string component_assembly_id{};
  std::string representation_architecture{"mtf_jepa_mae_vicreg.v1"};
  std::string representation_contract{"standalone.mtf_jepa_mae_vicreg.v1"};
  std::string primary_value_shape{"[B_flat,De]"};
  std::string sequence_value_shape{"[B_flat,Ntok,De]"};
  std::string channel_value_shape{"[B_flat,C,De]"};
  std::string channel_axis_policy{"optional_channel_output"};
  std::string temporal_reduction{"mask_aware_token_pool"};
  std::string input_nodelift_shape{"[B,C,Hx,N,F]"};
  std::string mtf_training_shape{"[B_flat,C,Hx,F]"};
  std::string flattening_contract{"anchor_node_flatten.v1"};
  std::string recommended_graph_restore_shape{"[B,N,C,De]"};
  bool graph_restore_available{false};
  std::string graph_restore_reason{
      "runtime_reports_flattened_anchor_node_samples_only"};
  bool reshape_lossless{true};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::string cell_valid_policy{};
  int64_t anchor_batch_count{0};
  int64_t node_count{0};
  int64_t flattened_sample_count{0};
  int64_t channel_count{0};
  int64_t history_length{0};
  int64_t input_width{0};
  int64_t input_feature_width{0};
  int64_t d_model{0};
  int64_t encoding_dim{0};
  int64_t projector_dim{0};
  int64_t predictor_hidden_dim{0};
  int64_t num_encoder_layers{0};
  int64_t num_predictor_layers{0};
  int64_t num_decoder_layers{0};
  int64_t num_heads{0};
  double dropout{std::numeric_limits<double>::quiet_NaN()};
  double lambda_jepa{std::numeric_limits<double>::quiet_NaN()};
  double lambda_mae{std::numeric_limits<double>::quiet_NaN()};
  double lambda_tf_align{std::numeric_limits<double>::quiet_NaN()};
  double lambda_vicreg{std::numeric_limits<double>::quiet_NaN()};
  bool use_frequency_tokens{true};
  bool use_mae_decoder{true};
  bool use_jepa_loss{true};
  bool use_tf_align_loss{true};
  bool use_vicreg_loss{true};
  bool use_global_vicreg{true};
  bool use_channel_vicreg{false};
  double target_ema_tau{std::numeric_limits<double>::quiet_NaN()};
  bool use_target_ema{true};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  std::string wave_id{};
  std::string wave_mode{};
  std::string target_action{};
  std::string wave_source_range_policy{};
  std::string wave_source_order_policy{};
  int64_t requested_anchor_index_begin{-1};
  int64_t requested_anchor_index_end{-1};
  std::string requested_source_key_begin{};
  std::string requested_source_key_end{};
  std::string runtime_report_mode{};
  std::string stream_plan{};
  std::string source_cursor_token{};
  std::string split_name{"unknown"};
  std::string protected_split_name{"unknown"};
  std::string range_boundary_policy{"half_open"};
  int64_t source_range_start{-1};
  int64_t source_range_end{-1};
  int64_t completed_anchor_start{-1};
  int64_t completed_anchor_end{-1};
  int64_t checkpoint_trained_anchor_end{-1};
  int64_t protected_anchor_start{-1};
  int64_t protected_anchor_end{-1};
  std::string lineage_leakage_check{"unknown"};
  std::string lineage_leakage_reason{
      "lattice_target_evaluator_owns_protected_split_proof"};
  std::string run_data_kind{"market"};
  std::string readiness_scope{"market_pretraining"};
  bool synthetic_training_passed{false};
  bool market_readiness_claimed{false};
  int64_t source_anchor_count{0};
  int64_t wave_streamed_anchor_count{0};
  std::string wave_first_anchor_key{};
  std::string wave_last_anchor_key{};

  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};
  int64_t wave_pulses_attempted{0};
  int64_t wave_pulses_completed{0};
  int64_t wave_pulses_skipped{0};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double max_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};
  double gradients_finite{1.0};
  int64_t nonfinite_output_count{0};

  double loss_jepa_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_mae_time_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_mae_frequency_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_tf_align_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_vicreg_global_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_vicreg_channel_mean{std::numeric_limits<double>::quiet_NaN()};

  int64_t adapter_original_rows{0};
  int64_t adapter_valid_feature_count{0};
  int64_t adapter_valid_cell_any_count{0};
  int64_t adapter_valid_cell_all_count{0};
  double mean_valid_feature_fraction{std::numeric_limits<double>::quiet_NaN()};
  int64_t sample_valid_count{0};
  double sample_valid_fraction{std::numeric_limits<double>::quiet_NaN()};
  int64_t channel_valid_count{0};
  double channel_valid_fraction{std::numeric_limits<double>::quiet_NaN()};
  int64_t valid_latent_rows{0};
  int64_t tf_pair_count{0};
  int64_t tf_pair_valid_count{0};
  int64_t vicreg_global_valid_rows{0};
  int64_t vicreg_channel_valid_rows{0};
  int64_t context_starved_sample_count{0};
  int64_t reduced_targets_for_context_count{0};
  int64_t min_context_satisfied_count{0};
  double target_ema_distance{std::numeric_limits<double>::quiet_NaN()};
  double latent_std{std::numeric_limits<double>::quiet_NaN()};
  double latent_norm{std::numeric_limits<double>::quiet_NaN()};

  int64_t representation_embedding_dim{0};
  double representation_effective_rank{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_effective_rank_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_min_dimension_variance{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_max_dimension_variance{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_condition_number{
      std::numeric_limits<double>::quiet_NaN()};
  double representation_isotropy_score{
      std::numeric_limits<double>::quiet_NaN()};

  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  int64_t last_checkpoint_optimizer_step{0};
  std::string checkpoint_path{};
  std::string checkpoint_format{};
  std::string checkpoint_path_reported{};
  std::string checkpoint_digest_reported{};
  bool checkpoint_digest_verified{false};
  bool checkpoint_file_exists{false};
  int64_t checkpoint_bytes{0};
  int64_t checkpoint_mtime_ticks{0};
  std::string checkpoint_artifact_status{"not_written"};
  bool report_written{false};
  int64_t report_write_count{0};
  int64_t last_report_attempted_step{0};
  std::string report_path{};

  static void append_string_list(std::ostringstream &oss, const char *key,
                                 const std::vector<std::string> &values) {
    oss << key << "=";
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << values[i];
    }
    oss << "\n";
  }

  [[nodiscard]] std::string to_text() const {
    std::ostringstream oss;
    oss << "report_schema_id=" << report_schema_id << "\n";
    oss << "report_schema_version=" << report_schema_version << "\n";
    oss << "report_writer_id=" << report_writer_id << "\n";
    oss << "report_writer_version=" << report_writer_version << "\n";
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "component_assembly_id=" << component_assembly_id << "\n";
    oss << "representation_architecture=" << representation_architecture
        << "\n";
    oss << "representation_contract=" << representation_contract << "\n";
    oss << "primary_value_shape=" << primary_value_shape << "\n";
    oss << "sequence_value_shape=" << sequence_value_shape << "\n";
    oss << "channel_value_shape=" << channel_value_shape << "\n";
    oss << "channel_axis_policy=" << channel_axis_policy << "\n";
    oss << "temporal_reduction=" << temporal_reduction << "\n";
    oss << "input_nodelift_shape=" << input_nodelift_shape << "\n";
    oss << "mtf_training_shape=" << mtf_training_shape << "\n";
    oss << "flattening_contract=" << flattening_contract << "\n";
    oss << "recommended_graph_restore_shape=" << recommended_graph_restore_shape
        << "\n";
    oss << "graph_restore_available="
        << (graph_restore_available ? "true" : "false") << "\n";
    oss << "graph_restore_reason=" << graph_restore_reason << "\n";
    oss << "reshape_lossless=" << (reshape_lossless ? "true" : "false") << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "cell_valid_policy=" << cell_valid_policy << "\n";
    append_string_list(oss, "node_ids", node_ids);
    append_string_list(oss, "edge_ids", edge_ids);
    oss << "anchor_batch_count=" << anchor_batch_count << "\n";
    oss << "node_count=" << node_count << "\n";
    oss << "flattened_sample_count=" << flattened_sample_count << "\n";
    oss << "channel_count=" << channel_count << "\n";
    oss << "history_length=" << history_length << "\n";
    oss << "input_width=" << input_width << "\n";
    oss << "input_feature_width=" << input_feature_width << "\n";
    oss << "d_model=" << d_model << "\n";
    oss << "encoding_dim=" << encoding_dim << "\n";
    oss << "projector_dim=" << projector_dim << "\n";
    oss << "predictor_hidden_dim=" << predictor_hidden_dim << "\n";
    oss << "num_encoder_layers=" << num_encoder_layers << "\n";
    oss << "num_predictor_layers=" << num_predictor_layers << "\n";
    oss << "num_decoder_layers=" << num_decoder_layers << "\n";
    oss << "num_heads=" << num_heads << "\n";
    oss << "dropout=" << dropout << "\n";
    oss << "lambda_jepa=" << lambda_jepa << "\n";
    oss << "lambda_mae=" << lambda_mae << "\n";
    oss << "lambda_tf_align=" << lambda_tf_align << "\n";
    oss << "lambda_vicreg=" << lambda_vicreg << "\n";
    oss << "use_frequency_tokens=" << (use_frequency_tokens ? "true" : "false")
        << "\n";
    oss << "use_mae_decoder=" << (use_mae_decoder ? "true" : "false") << "\n";
    oss << "use_jepa_loss=" << (use_jepa_loss ? "true" : "false") << "\n";
    oss << "use_tf_align_loss=" << (use_tf_align_loss ? "true" : "false")
        << "\n";
    oss << "use_vicreg_loss=" << (use_vicreg_loss ? "true" : "false") << "\n";
    oss << "use_global_vicreg=" << (use_global_vicreg ? "true" : "false")
        << "\n";
    oss << "use_channel_vicreg=" << (use_channel_vicreg ? "true" : "false")
        << "\n";
    oss << "target_ema_tau=" << target_ema_tau << "\n";
    oss << "use_target_ema=" << (use_target_ema ? "true" : "false") << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
    oss << "checkpoint_every=" << checkpoint_every << "\n";
    oss << "report_every=" << report_every << "\n";
    oss << "validation_every=" << validation_every << "\n";
    oss << "wave_id=" << wave_id << "\n";
    oss << "wave_mode=" << wave_mode << "\n";
    oss << "target_action=" << target_action << "\n";
    oss << "wave_source_range_policy=" << wave_source_range_policy << "\n";
    oss << "wave_source_order_policy=" << wave_source_order_policy << "\n";
    oss << "requested_anchor_index_begin=" << requested_anchor_index_begin
        << "\n";
    oss << "requested_anchor_index_end=" << requested_anchor_index_end << "\n";
    oss << "requested_source_key_begin=" << requested_source_key_begin << "\n";
    oss << "requested_source_key_end=" << requested_source_key_end << "\n";
    oss << "runtime_report_mode=" << runtime_report_mode << "\n";
    oss << "stream_plan=" << stream_plan << "\n";
    oss << "source_cursor_token=" << source_cursor_token << "\n";
    oss << "split_name=" << split_name << "\n";
    oss << "protected_split_name=" << protected_split_name << "\n";
    oss << "range_boundary_policy=" << range_boundary_policy << "\n";
    oss << "source_range_start=" << source_range_start << "\n";
    oss << "source_range_end=" << source_range_end << "\n";
    oss << "completed_anchor_start=" << completed_anchor_start << "\n";
    oss << "completed_anchor_end=" << completed_anchor_end << "\n";
    oss << "checkpoint_trained_anchor_end=" << checkpoint_trained_anchor_end
        << "\n";
    oss << "protected_anchor_start=" << protected_anchor_start << "\n";
    oss << "protected_anchor_end=" << protected_anchor_end << "\n";
    oss << "lineage_leakage_check=" << lineage_leakage_check << "\n";
    oss << "lineage_leakage_reason=" << lineage_leakage_reason << "\n";
    oss << "run_data_kind=" << run_data_kind << "\n";
    oss << "readiness_scope=" << readiness_scope << "\n";
    oss << "synthetic_training_passed="
        << (synthetic_training_passed ? "true" : "false") << "\n";
    oss << "market_readiness_claimed="
        << (market_readiness_claimed ? "true" : "false") << "\n";
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    oss << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    oss << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    oss << "steps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    oss << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    oss << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_grad_norm=" << last_grad_norm << "\n";
    oss << "max_grad_norm=" << max_grad_norm << "\n";
    oss << "finite_parameter_check="
        << (finite_parameter_check != 0.0 ? "true" : "false") << "\n";
    oss << "gradients_finite=" << (gradients_finite != 0.0 ? "true" : "false")
        << "\n";
    oss << "nonfinite_output_count=" << nonfinite_output_count << "\n";
    oss << "loss_jepa_mean=" << loss_jepa_mean << "\n";
    oss << "loss_mae_time_mean=" << loss_mae_time_mean << "\n";
    oss << "loss_mae_frequency_mean=" << loss_mae_frequency_mean << "\n";
    oss << "loss_tf_align_mean=" << loss_tf_align_mean << "\n";
    oss << "loss_vicreg_global_mean=" << loss_vicreg_global_mean << "\n";
    oss << "loss_vicreg_channel_mean=" << loss_vicreg_channel_mean << "\n";
    oss << "adapter_original_rows=" << adapter_original_rows << "\n";
    oss << "adapter_valid_feature_count=" << adapter_valid_feature_count
        << "\n";
    oss << "adapter_valid_cell_any_count=" << adapter_valid_cell_any_count
        << "\n";
    oss << "adapter_valid_cell_all_count=" << adapter_valid_cell_all_count
        << "\n";
    oss << "mean_valid_feature_fraction=" << mean_valid_feature_fraction
        << "\n";
    oss << "sample_valid_count=" << sample_valid_count << "\n";
    oss << "sample_valid_fraction=" << sample_valid_fraction << "\n";
    oss << "channel_valid_count=" << channel_valid_count << "\n";
    oss << "channel_valid_fraction=" << channel_valid_fraction << "\n";
    oss << "valid_latent_rows=" << valid_latent_rows << "\n";
    oss << "total_valid_latent_rows=" << valid_latent_rows << "\n";
    oss << "tf_pair_count=" << tf_pair_count << "\n";
    oss << "tf_pair_valid_count=" << tf_pair_valid_count << "\n";
    oss << "vicreg_global_valid_rows=" << vicreg_global_valid_rows << "\n";
    oss << "vicreg_channel_valid_rows=" << vicreg_channel_valid_rows << "\n";
    oss << "context_starved_sample_count=" << context_starved_sample_count
        << "\n";
    oss << "reduced_targets_for_context_count="
        << reduced_targets_for_context_count << "\n";
    oss << "min_context_satisfied_count=" << min_context_satisfied_count
        << "\n";
    oss << "target_ema_distance=" << target_ema_distance << "\n";
    oss << "latent_std=" << latent_std << "\n";
    oss << "latent_norm=" << latent_norm << "\n";
    oss << "representation_embedding_dim=" << representation_embedding_dim
        << "\n";
    oss << "representation_effective_rank=" << representation_effective_rank
        << "\n";
    oss << "representation_effective_rank_fraction="
        << representation_effective_rank_fraction << "\n";
    oss << "representation_min_dimension_variance="
        << representation_min_dimension_variance << "\n";
    oss << "representation_max_dimension_variance="
        << representation_max_dimension_variance << "\n";
    oss << "representation_condition_number=" << representation_condition_number
        << "\n";
    oss << "representation_isotropy_score=" << representation_isotropy_score
        << "\n";
    oss << "checkpoint_written=" << (checkpoint_written ? "true" : "false")
        << "\n";
    oss << "checkpoint_write_count=" << checkpoint_write_count << "\n";
    oss << "last_checkpoint_optimizer_step=" << last_checkpoint_optimizer_step
        << "\n";
    oss << "checkpoint_path=" << checkpoint_path << "\n";
    oss << "checkpoint_format=" << checkpoint_format << "\n";
    oss << "checkpoint_path_reported=" << checkpoint_path_reported << "\n";
    oss << "checkpoint_digest_reported=" << checkpoint_digest_reported << "\n";
    oss << "checkpoint_digest_verified="
        << (checkpoint_digest_verified ? "true" : "false") << "\n";
    oss << "checkpoint_file_exists="
        << (checkpoint_file_exists ? "true" : "false") << "\n";
    oss << "checkpoint_bytes=" << checkpoint_bytes << "\n";
    oss << "checkpoint_mtime_ticks=" << checkpoint_mtime_ticks << "\n";
    oss << "checkpoint_artifact_status=" << checkpoint_artifact_status << "\n";
    oss << "report_written=" << (report_written ? "true" : "false") << "\n";
    oss << "report_write_count=" << report_write_count << "\n";
    oss << "last_report_attempted_step=" << last_report_attempted_step << "\n";
    oss << "report_path=" << report_path << "\n";
    return oss.str();
  }
};

namespace mtf_jepa_mae_vicreg_graph_first_launcher_detail {

inline double scalar_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .template item<double>();
}

inline int64_t scalar_i64_or_zero(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return 0;
  }
  return static_cast<int64_t>(std::llround(scalar_or_nan(tensor)));
}

inline double finite_or_zero(double value) {
  return std::isfinite(value) ? value : 0.0;
}

[[nodiscard]] inline bool
parameters_are_finite(const std::vector<torch::Tensor> &params,
                      bool inspect_grad) {
  for (const auto &param : params) {
    if (!torch::isfinite(param.detach()).all().template item<bool>()) {
      return false;
    }
    if (inspect_grad && param.grad().defined() &&
        !torch::isfinite(param.grad().detach()).all().template item<bool>()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline double
gradient_norm(const std::vector<torch::Tensor> &params) {
  double total_sq = 0.0;
  bool saw_grad = false;
  for (const auto &param : params) {
    if (!param.grad().defined()) {
      continue;
    }
    saw_grad = true;
    total_sq += param.grad()
                    .detach()
                    .to(torch::kCPU)
                    .to(torch::kFloat64)
                    .pow(2)
                    .sum()
                    .template item<double>();
  }
  return saw_grad ? std::sqrt(total_sq)
                  : std::numeric_limits<double>::quiet_NaN();
}

inline void clip_gradients(std::vector<torch::Tensor> &params, double clip_norm,
                           double current_norm) {
  if (clip_norm <= 0.0 || !std::isfinite(current_norm) ||
      current_norm <= clip_norm) {
    return;
  }
  const double scale = clip_norm / (current_norm + 1e-12);
  for (auto &param : params) {
    if (param.grad().defined()) {
      param.grad().mul_(scale);
    }
  }
}

inline std::string seed_torch_runtime(uint64_t seed,
                                      const torch::Device &device) {
  torch::manual_seed(seed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(seed);
    return "torch_manual_seed_cuda_manual_seed_all";
  }
  return "torch_manual_seed_cpu";
}

template <typename KeyT>
inline std::string optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  std::ostringstream oss;
  oss << *value;
  return oss.str();
}

inline void ensure_parent_directory(const std::filesystem::path &path,
                                    const char *path_kind) {
  const auto parent = path.parent_path();
  if (parent.empty()) {
    return;
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    throw std::runtime_error(
        std::string("[mtf_jepa_mae_vicreg_graph_first_launcher] failed to "
                    "create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void
write_report_file(const std::filesystem::path &path,
                  const mtf_jepa_mae_vicreg_graph_first_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_graph_first_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_graph_first_launcher] failed to open report "
        "path");
  }
  out << report.to_text();
}

struct file_artifact_summary_t {
  bool exists{false};
  bool digest_ok{false};
  std::string digest{};
  int64_t bytes{0};
  int64_t mtime_ticks{0};
  std::string status{"missing"};
};

[[nodiscard]] inline file_artifact_summary_t
summarize_file_artifact(const std::filesystem::path &path) {
  namespace assembly_detail = cuwacunu::wikimyei::assembly::assembly_detail;
  file_artifact_summary_t out{};
  if (path.empty()) {
    out.status = "empty_path";
    return out;
  }
  std::error_code ec;
  out.exists = std::filesystem::exists(path, ec);
  if (ec || !out.exists) {
    out.status = ec ? "exists_check_failed" : "missing";
    return out;
  }
  if (!std::filesystem::is_regular_file(path, ec) || ec) {
    out.status = ec ? "file_type_check_failed" : "not_regular_file";
    return out;
  }
  const auto size = std::filesystem::file_size(path, ec);
  if (!ec) {
    out.bytes = static_cast<int64_t>(size);
  }
  const auto mtime = std::filesystem::last_write_time(path, ec);
  if (!ec) {
    out.mtime_ticks = static_cast<int64_t>(mtime.time_since_epoch().count());
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    out.status = "open_failed";
    return out;
  }
  std::uint64_t hash = assembly_detail::kFnvOffsetBasis;
  assembly_detail::mix_hash_string(hash, "kikijyeba.lattice.exposure.v1");
  for (const unsigned char c : std::string_view("file:")) {
    assembly_detail::mix_hash_byte(hash, c);
  }
  bool saw_byte = false;
  std::array<char, 1 << 16> buffer{};
  while (in) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto n = in.gcount();
    if (n <= 0) {
      break;
    }
    saw_byte = true;
    for (std::streamsize i = 0; i < n; ++i) {
      assembly_detail::mix_hash_byte(
          hash,
          static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)]));
    }
  }
  if (in.bad()) {
    out.status = "read_failed";
    return out;
  }
  if (!saw_byte) {
    out.status = "empty_file";
    return out;
  }
  assembly_detail::mix_hash_byte(hash, 0xffu);
  out.digest = assembly_detail::hash_hex(hash);
  out.digest_ok = true;
  out.status = "verified";
  return out;
}

struct representation_geometry_summary_t {
  int64_t embedding_dim{0};
  double effective_rank{std::numeric_limits<double>::quiet_NaN()};
  double effective_rank_fraction{std::numeric_limits<double>::quiet_NaN()};
  double min_dimension_variance{std::numeric_limits<double>::quiet_NaN()};
  double max_dimension_variance{std::numeric_limits<double>::quiet_NaN()};
  double condition_number{std::numeric_limits<double>::quiet_NaN()};
  double isotropy_score{std::numeric_limits<double>::quiet_NaN()};

  [[nodiscard]] bool has_rows() const {
    return embedding_dim > 0 && std::isfinite(effective_rank);
  }
};

[[nodiscard]] inline representation_geometry_summary_t
summarize_representation_geometry(const torch::Tensor &z,
                                  const torch::Tensor &mask) {
  representation_geometry_summary_t out{};
  if (!z.defined() || !mask.defined() || z.dim() != 3 || mask.dim() != 2 ||
      z.size(0) != mask.size(0) || z.size(1) != mask.size(1)) {
    return out;
  }
  out.embedding_dim = z.size(2);
  if (out.embedding_dim <= 0) {
    return out;
  }
  auto rows = torch::nonzero(mask.to(torch::kBool).reshape({-1})).reshape({-1});
  if (rows.numel() == 0) {
    return out;
  }
  auto valid_rows = z.reshape({-1, out.embedding_dim})
                        .index_select(/*dim=*/0, rows)
                        .detach()
                        .to(torch::kCPU)
                        .to(torch::kFloat64);
  if (valid_rows.size(0) <= 1) {
    return out;
  }
  auto variances = valid_rows.var(/*dim=*/0, /*unbiased=*/false).clamp_min(0.0);
  variances = torch::where(torch::isfinite(variances), variances,
                           torch::zeros_like(variances));
  out.min_dimension_variance = variances.min().template item<double>();
  out.max_dimension_variance = variances.max().template item<double>();

  constexpr double eps = 1e-12;
  const double total_variance = variances.sum().template item<double>();
  if (total_variance <= eps) {
    out.effective_rank = 0.0;
    out.effective_rank_fraction = 0.0;
    out.condition_number = std::numeric_limits<double>::infinity();
    out.isotropy_score = 0.0;
    return out;
  }
  auto probabilities = variances / total_variance;
  const double entropy =
      -(probabilities * torch::log(probabilities.clamp_min(eps)))
           .sum()
           .template item<double>();
  out.effective_rank = std::exp(entropy);
  out.effective_rank_fraction =
      out.effective_rank / static_cast<double>(out.embedding_dim);
  out.condition_number =
      (out.max_dimension_variance + eps) / (out.min_dimension_variance + eps);
  out.isotropy_score =
      out.min_dimension_variance / (out.max_dimension_variance + eps);
  return out;
}

inline void
save_checkpoint_file(const std::filesystem::path &path,
                     const mtf_jepa_mae_vicreg_graph_first_report_t &report,
                     cuwacunu::wikimyei::representation::encoding::
                         mtf_jepa_mae_vicreg::MtfJepaMaeVicreg &model,
                     torch::optim::Optimizer &optimizer) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive model_archive;
  model->save(model_archive);
  root.write("model", model_archive);
  torch::serialize::OutputArchive optimizer_archive;
  optimizer.save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  root.write("meta/channel_count", torch::tensor({report.channel_count}, i64));
  root.write("meta/history_length",
             torch::tensor({report.history_length}, i64));
  root.write("meta/input_width", torch::tensor({report.input_width}, i64));
  root.write("meta/d_model", torch::tensor({report.d_model}, i64));
  root.write("meta/latent_dim", torch::tensor({report.encoding_dim}, i64));
  root.write("meta/projector_dim", torch::tensor({report.projector_dim}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write("meta/mean_loss", torch::tensor({report.mean_loss}, f64));
  root.write("meta/target_ema_distance",
             torch::tensor({report.target_ema_distance}, f64));
  root.save_to(path.string());
}

} // namespace mtf_jepa_mae_vicreg_graph_first_launcher_detail

template <typename DatatypeT> class mtf_jepa_mae_vicreg_graph_first_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::channel_graph_first_pipeline_builder_t<
          DatatypeT>;

  mtf_jepa_mae_vicreg_graph_first_launcher_t(
      builder_t builder,
      mtf_jepa_mae_vicreg_graph_first_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] mtf_jepa_mae_vicreg_graph_first_report_t
  dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    const auto &bundle = builder_.bundle();
    const auto &spec = bundle.mtf_jepa_mae_vicreg;
    const auto &cfg = spec.config;
    const auto &training = bundle.mtf_jepa_mae_vicreg_training;

    mtf_jepa_mae_vicreg_graph_first_report_t out{};
    out.training_id = training.training_id;
    out.config_path = bundle.config_path;
    out.component_assembly_id = spec.component_assembly_id;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.cell_valid_policy = plan.mask_profile;
    out.anchor_batch_count = static_cast<int64_t>(plan.effective_batch_size);
    out.node_count = plan.node_count;
    out.flattened_sample_count = out.anchor_batch_count * out.node_count;
    out.channel_count = cfg.channel_count;
    out.history_length = cfg.history_length;
    out.input_width = cfg.input_width;
    out.input_feature_width = cfg.input_width;
    out.d_model = cfg.d_model;
    out.encoding_dim = cfg.latent_dim;
    out.representation_embedding_dim = cfg.latent_dim;
    out.projector_dim = cfg.projector_dim;
    out.predictor_hidden_dim = cfg.predictor_hidden_dim;
    out.num_encoder_layers = cfg.num_encoder_layers;
    out.num_predictor_layers = cfg.num_predictor_layers;
    out.num_decoder_layers = cfg.num_decoder_layers;
    out.num_heads = cfg.num_heads;
    out.dropout = cfg.dropout;
    out.lambda_jepa = cfg.lambda_jepa;
    out.lambda_mae = cfg.lambda_mae;
    out.lambda_tf_align = cfg.lambda_tf_align;
    out.lambda_vicreg = cfg.lambda_vicreg;
    out.use_frequency_tokens = cfg.use_frequency_tokens;
    out.use_mae_decoder = cfg.use_mae_decoder;
    out.use_jepa_loss = cfg.use_jepa_loss;
    out.use_tf_align_loss = cfg.use_tf_align_loss;
    out.use_vicreg_loss = cfg.use_vicreg_loss;
    out.use_global_vicreg = cfg.use_global_vicreg;
    out.use_channel_vicreg = cfg.use_channel_vicreg;
    out.target_ema_tau = cfg.target_ema_tau;
    out.use_target_ema = cfg.use_target_ema;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = builder_.batch_size_source();
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = training.seed;
    out.seed_scope = cfg.device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.checkpoint_every = training.checkpoint_every;
    out.report_every = training.report_every;
    out.validation_every = training.validation_every;
    out.wave_id = plan.wave_id;
    out.wave_mode = plan.wave_mode;
    out.target_action = effective_train_target() ? "train" : "run";
    out.wave_source_range_policy = plan.wave_source_range_policy;
    out.wave_source_order_policy = plan.wave_source_order_policy;
    out.requested_anchor_index_begin = plan.requested_anchor_index_begin;
    out.requested_anchor_index_end = plan.requested_anchor_index_end;
    out.requested_source_key_begin = plan.requested_source_key_begin;
    out.requested_source_key_end = plan.requested_source_key_end;
    out.source_range_start = plan.requested_anchor_index_begin;
    out.source_range_end = plan.requested_anchor_index_end;
    out.completed_anchor_start = out.source_range_start;
    out.completed_anchor_end = out.source_range_end;
    out.checkpoint_trained_anchor_end = out.source_range_end;
    out.runtime_report_mode =
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(bundle.wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] mtf_jepa_mae_vicreg_graph_first_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const bool train_target = effective_train_target();
    const auto &bundle = builder_.bundle();
    const auto &training = bundle.mtf_jepa_mae_vicreg_training;
    if ((options_.write_report ||
         (train_target && training.checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error(
          "[mtf_jepa_mae_vicreg_graph_first_launcher] report_path is required "
          "for configured report or checkpoint output");
    }

    const auto seed_scope =
        mtf_jepa_mae_vicreg_graph_first_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(training.seed),
            bundle.mtf_jepa_mae_vicreg.config.device);
    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            bundle.wave_settings, options_.runtime_report_mode);
    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    namespace mtf =
        cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
    auto model = mtf::MtfJepaMaeVicreg(bundle.mtf_jepa_mae_vicreg.config);
    model->to(bundle.mtf_jepa_mae_vicreg.config.device,
              bundle.mtf_jepa_mae_vicreg.config.dtype);
    std::vector<torch::Tensor> params = model->parameters();
    torch::optim::Adam optimizer(
        params, torch::optim::AdamOptions(training.learning_rate));

    auto report = dry_run_report();
    report.seed_scope = seed_scope;
    report.runtime_report_mode =
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);
    if (report.source_range_start < 0) {
      report.source_range_start = 0;
    }
    if (report.source_range_end < 0) {
      report.source_range_end = report.source_anchor_count;
    }
    report.completed_anchor_start = report.source_range_start;
    report.completed_anchor_end = report.source_range_end;
    report.checkpoint_trained_anchor_end = report.source_range_end;

    double loss_sum = 0.0;
    double loss_jepa_sum = 0.0;
    double loss_mae_time_sum = 0.0;
    double loss_mae_frequency_sum = 0.0;
    double loss_tf_align_sum = 0.0;
    double loss_vicreg_global_sum = 0.0;
    double loss_vicreg_channel_sum = 0.0;
    int64_t loss_count = 0;
    double valid_feature_fraction_sum = 0.0;
    int64_t diagnostics_count = 0;
    double max_grad_norm = -std::numeric_limits<double>::infinity();
    double effective_rank_sum = 0.0;
    double effective_rank_fraction_sum = 0.0;
    double min_dimension_variance_sum = 0.0;
    double max_dimension_variance_sum = 0.0;
    double condition_number_sum = 0.0;
    double isotropy_score_sum = 0.0;
    int64_t geometry_count = 0;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
        report.loss_jepa_mean = loss_jepa_sum / static_cast<double>(loss_count);
        report.loss_mae_time_mean =
            loss_mae_time_sum / static_cast<double>(loss_count);
        report.loss_mae_frequency_mean =
            loss_mae_frequency_sum / static_cast<double>(loss_count);
        report.loss_tf_align_mean =
            loss_tf_align_sum / static_cast<double>(loss_count);
        report.loss_vicreg_global_mean =
            loss_vicreg_global_sum / static_cast<double>(loss_count);
        report.loss_vicreg_channel_mean =
            loss_vicreg_channel_sum / static_cast<double>(loss_count);
      }
      if (diagnostics_count > 0) {
        report.mean_valid_feature_fraction =
            valid_feature_fraction_sum / static_cast<double>(diagnostics_count);
      }
      if (std::isfinite(max_grad_norm)) {
        report.max_grad_norm = max_grad_norm;
      }
      if (geometry_count > 0) {
        report.representation_effective_rank =
            effective_rank_sum / static_cast<double>(geometry_count);
        report.representation_effective_rank_fraction =
            effective_rank_fraction_sum / static_cast<double>(geometry_count);
        report.representation_min_dimension_variance =
            min_dimension_variance_sum / static_cast<double>(geometry_count);
        report.representation_max_dimension_variance =
            max_dimension_variance_sum / static_cast<double>(geometry_count);
        report.representation_condition_number =
            condition_number_sum / static_cast<double>(geometry_count);
        report.representation_isotropy_score =
            isotropy_score_sum / static_cast<double>(geometry_count);
      }
      if (report.source_range_start >= 0 && report.source_range_end >= 0 &&
          report.wave_streamed_anchor_count > 0) {
        report.completed_anchor_start = report.source_range_start;
        report.completed_anchor_end = std::min(
            report.source_range_end,
            report.source_range_start + report.wave_streamed_anchor_count);
        report.checkpoint_trained_anchor_end = report.completed_anchor_end;
      }
    };

    auto write_checkpoint = [&]() {
      if (!train_target || report.optimizer_steps <= 0) {
        return;
      }
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".mtf_jepa_mae_vicreg.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_path_reported = report.checkpoint_path;
      report.checkpoint_format = "torch_archive_mtf_jepa_mae_vicreg_v1";
      mtf_jepa_mae_vicreg_graph_first_launcher_detail::save_checkpoint_file(
          checkpoint_path, report, model, optimizer);
      const auto artifact = mtf_jepa_mae_vicreg_graph_first_launcher_detail::
          summarize_file_artifact(checkpoint_path);
      report.checkpoint_file_exists = artifact.exists;
      report.checkpoint_digest_reported = artifact.digest;
      report.checkpoint_digest_verified = artifact.digest_ok;
      report.checkpoint_bytes = artifact.bytes;
      report.checkpoint_mtime_ticks = artifact.mtime_ticks;
      report.checkpoint_artifact_status = artifact.status;
    };

    auto write_report = [&]() {
      if (!options_.write_report) {
        return;
      }
      refresh_running_report();
      report.report_written = true;
      ++report.report_write_count;
      report.last_report_attempted_step = report.steps_attempted;
      report.report_path = options_.report_path.string();
      mtf_jepa_mae_vicreg_graph_first_launcher_detail::write_report_file(
          options_.report_path, report);
    };

    const int64_t max_steps = training.max_steps;
    while (max_steps == 0 || report.steps_attempted < max_steps) {
      if (!lifted_stream.has_next()) {
        if (!train_target || max_steps == 0) {
          break;
        }
        lifted_stream.reset();
        if (!lifted_stream.has_next()) {
          break;
        }
      }
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto lifted = lifted_stream.next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(lifted.cursor.anchor_count());
      if (auto first = lifted.cursor.first_anchor_key()) {
        if (report.wave_first_anchor_key.empty()) {
          report.wave_first_anchor_key =
              mtf_jepa_mae_vicreg_graph_first_launcher_detail::
                  optional_key_to_string(first);
        }
      }
      if (auto last = lifted.cursor.last_anchor_key()) {
        report.wave_last_anchor_key =
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::
                optional_key_to_string(last);
      }

      auto input = mtf::make_mtf_channel_node_input(lifted);
      report.anchor_batch_count = input.B_anchor;
      report.node_count = input.N;
      report.flattened_sample_count =
          input.data.defined() ? input.data.size(0) : input.B_anchor * input.N;
      if (input.data.defined() && input.data.dim() == 4) {
        report.channel_count = input.data.size(1);
        report.history_length = input.data.size(2);
        report.input_width = input.data.size(3);
        report.input_feature_width = input.data.size(3);
      }
      report.adapter_original_rows += input.diagnostics.original_rows;
      report.adapter_valid_feature_count +=
          input.diagnostics.valid_feature_count;
      report.adapter_valid_cell_any_count +=
          input.diagnostics.valid_cell_any_count;
      report.adapter_valid_cell_all_count +=
          input.diagnostics.valid_cell_all_count;
      valid_feature_fraction_sum += input.diagnostics.valid_feature_fraction;
      ++diagnostics_count;

      bool completed = false;
      bool gradients_finite = true;
      double grad_norm = std::numeric_limits<double>::quiet_NaN();
      mtf::mtf_jepa_mae_vicreg_output_t output{};
      if (train_target) {
        model->train();
        optimizer.zero_grad();
        output = model->forward(input.data, input.feature_mask);
        if (!torch::isfinite(output.loss).all().template item<bool>()) {
          ++report.nonfinite_output_count;
          ++report.skipped_batches;
          ++report.wave_pulses_skipped;
          continue;
        }
        output.loss.backward();
        gradients_finite = mtf_jepa_mae_vicreg_graph_first_launcher_detail::
            parameters_are_finite(params, /*inspect_grad=*/true);
        grad_norm =
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::gradient_norm(
                params);
        mtf_jepa_mae_vicreg_graph_first_launcher_detail::clip_gradients(
            params, training.grad_clip_norm, grad_norm);
        if (!gradients_finite || !std::isfinite(grad_norm)) {
          ++report.nonfinite_output_count;
          ++report.skipped_batches;
          ++report.wave_pulses_skipped;
          optimizer.zero_grad();
          continue;
        }
        optimizer.step();
        model->update_target_network();
        completed = true;
        ++report.optimizer_steps;
      } else {
        model->eval();
        torch::NoGradGuard no_grad;
        auto encoded = model->encode(input.data, input.feature_mask);
        output.embeddings = encoded.embeddings;
        output.pooled_embedding = encoded.pooled_embedding;
        output.pooled_by_channel = encoded.pooled_by_channel;
        output.pooled_time = encoded.pooled_time;
        output.pooled_frequency = encoded.pooled_frequency;
        output.sample_valid_mask = encoded.sample_valid_mask;
        output.channel_valid_mask = encoded.channel_valid_mask;
        output.loss = torch::zeros({}, input.data.options());
        completed = output.sample_valid_mask.any().template item<bool>();
      }

      if (completed) {
        ++report.steps_completed;
        ++report.wave_pulses_completed;
      } else {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }

      report.last_loss =
          mtf_jepa_mae_vicreg_graph_first_launcher_detail::scalar_or_nan(
              output.loss);
      report.last_grad_norm = grad_norm;
      if (std::isfinite(grad_norm)) {
        max_grad_norm = std::max(max_grad_norm, grad_norm);
      }
      report.gradients_finite =
          (report.gradients_finite != 0.0 && gradients_finite) ? 1.0 : 0.0;
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 &&
           mtf_jepa_mae_vicreg_graph_first_launcher_detail::
               parameters_are_finite(params, /*inspect_grad=*/false))
              ? 1.0
              : 0.0;

      if (output.sample_valid_mask.defined()) {
        report.sample_valid_count =
            output.sample_valid_mask.sum().template item<int64_t>();
        const int64_t denom = output.sample_valid_mask.numel();
        report.sample_valid_fraction =
            denom > 0 ? static_cast<double>(report.sample_valid_count) /
                            static_cast<double>(denom)
                      : std::numeric_limits<double>::quiet_NaN();
      }
      if (output.channel_valid_mask.defined()) {
        report.channel_valid_count =
            output.channel_valid_mask.sum().template item<int64_t>();
        const int64_t denom = output.channel_valid_mask.numel();
        report.channel_valid_fraction =
            denom > 0 ? static_cast<double>(report.channel_valid_count) /
                            static_cast<double>(denom)
                      : std::numeric_limits<double>::quiet_NaN();
      }

      const auto diag = [&](const char *key) -> torch::Tensor {
        const auto it = output.diagnostics.find(key);
        return it == output.diagnostics.end() ? torch::Tensor() : it->second;
      };
      const auto diag_double = [&](const char *key) {
        return mtf_jepa_mae_vicreg_graph_first_launcher_detail::scalar_or_nan(
            diag(key));
      };
      const auto diag_i64 = [&](const char *key) {
        return mtf_jepa_mae_vicreg_graph_first_launcher_detail::
            scalar_i64_or_zero(diag(key));
      };
      report.valid_latent_rows = diag_i64("valid_latent_rows");
      report.tf_pair_count = diag_i64("tf_pair_count");
      report.tf_pair_valid_count = diag_i64("tf_pair_valid_count");
      report.vicreg_global_valid_rows = diag_i64("vicreg_global_valid_rows");
      report.vicreg_channel_valid_rows = diag_i64("vicreg_channel_valid_rows");
      report.context_starved_sample_count =
          diag_i64("context_starved_sample_count");
      report.reduced_targets_for_context_count =
          diag_i64("reduced_targets_for_context_count");
      report.min_context_satisfied_count =
          diag_i64("min_context_satisfied_count");
      report.target_ema_distance = diag_double("target_ema_distance");
      report.latent_std = diag_double("latent_std");
      report.latent_norm = diag_double("latent_norm");

      if (std::isfinite(report.last_loss)) {
        loss_sum += report.last_loss;
        loss_jepa_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_jepa"));
        loss_mae_time_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_mae_time"));
        loss_mae_frequency_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_mae_frequency"));
        loss_tf_align_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_tf_align"));
        loss_vicreg_global_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_vicreg_global"));
        loss_vicreg_channel_sum +=
            mtf_jepa_mae_vicreg_graph_first_launcher_detail::finite_or_zero(
                diag_double("loss_vicreg_channel"));
        ++loss_count;
      }

      if (output.pooled_by_channel.defined() &&
          output.channel_valid_mask.defined()) {
        const auto geometry = mtf_jepa_mae_vicreg_graph_first_launcher_detail::
            summarize_representation_geometry(output.pooled_by_channel,
                                              output.channel_valid_mask);
        if (geometry.has_rows()) {
          report.representation_embedding_dim = geometry.embedding_dim;
          effective_rank_sum += geometry.effective_rank;
          effective_rank_fraction_sum += geometry.effective_rank_fraction;
          min_dimension_variance_sum += geometry.min_dimension_variance;
          max_dimension_variance_sum += geometry.max_dimension_variance;
          if (std::isfinite(geometry.condition_number)) {
            condition_number_sum += geometry.condition_number;
          } else {
            condition_number_sum = std::numeric_limits<double>::infinity();
          }
          isotropy_score_sum += geometry.isotropy_score;
          ++geometry_count;
        }
      }

      if (train_target && completed && training.checkpoint_every > 0 &&
          report.optimizer_steps % training.checkpoint_every == 0) {
        write_checkpoint();
      }
      if (training.report_every > 0 &&
          report.steps_attempted % training.report_every == 0) {
        write_report();
      }
    }

    refresh_running_report();
    if (train_target && report.optimizer_steps > 0 &&
        !report.checkpoint_written) {
      write_checkpoint();
    }
    if (!options_.write_report ||
        report.last_report_attempted_step != report.steps_attempted) {
      write_report();
    }
    return report;
  }

private:
  [[nodiscard]] bool effective_train_target() const {
    if (!options_.train_target_from_wave) {
      return options_.train_target;
    }
    return builder_.bundle().wave_settings.action ==
           cuwacunu::hero::runtime::settings::wave_action_t::train;
  }

  void validate_batch_size_contract() const {
    const auto expected = static_cast<std::size_t>(
        builder_.bundle().mtf_jepa_mae_vicreg_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[mtf_jepa_mae_vicreg_graph_first_launcher] builder batch_size must "
          "match MTF-JEPA-MAE-VICReg training BATCH_SIZE");
    }
  }

  builder_t builder_;
  mtf_jepa_mae_vicreg_graph_first_launcher_options_t options_{};
};

} // namespace cuwacunu::jkimyei::training::representation
