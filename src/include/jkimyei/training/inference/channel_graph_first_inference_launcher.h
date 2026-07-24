// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
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
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "jkimyei/training/inference/channel_mdn_conditioned_affine_shadow.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

namespace cuwacunu::jkimyei::training::inference {

struct channel_graph_first_inference_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  std::function<void(std::string_view)> learning_probe_report_sink{};
  std::filesystem::path representation_edge_feature_probe_path{};
  std::filesystem::path mdn_edge_context_feature_probe_path{};
  std::optional<channel_mdn_conditioned_affine_shadow_options_t>
      conditioned_affine_shadow{};
  bool force_empty_targets_for_test{false};
  cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::hero::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct channel_graph_first_inference_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string component_assembly_id{};
  std::string input_representation_assembly_id{};
  std::string context_contract{"graph_order.channel_node_representation.v1"};
  std::string context_value_shape{"[B,N,C,De]"};
  std::string output_contract{
      "graph_order.channel_node_future_distribution.v1"};
  std::string output_value_shape{
      "log_pi:[B,N,C,Df,K];mu_sigma:[B,N,C,Df,K];direct_edge_return:[B,N-1,C]"};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  int64_t channel_count{0};
  int64_t context_dim{0};
  int64_t future_horizon{0};
  std::string context_mode{"channel_context_strict"};
  std::string target_domain{"channel_node_future"};
  std::string target_mask_policy{"per_target_feature_valid"};
  std::string activity_target{"node_feature_support_mean"};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  int64_t feature_embedding_dim{0};
  int64_t channel_adapter_rank{0};
  std::string mdn_architecture{"shared_slot_trunk.channel_adapter.shared_"
                               "feature_head.direct_edge_readout.shared.v4"};
  std::string loss_reduction{"balanced_channel_feature_mean"};
  bool shared_trunk{true};
  bool channel_adapters_enabled{true};
  bool shared_feature_head{true};
  bool feature_embedding_enabled{true};
  bool node_id_embedding{false};
  bool cross_node_attention{false};
  bool cross_channel_attention{false};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double eps{std::numeric_limits<double>::quiet_NaN()};
  double edge_return_auxiliary_loss_weight{0.0};
  double edge_return_auxiliary_direction_weight{0.0};
  double edge_return_auxiliary_rank_weight{0.0};
  double edge_return_auxiliary_huber_beta{
      std::numeric_limits<double>::quiet_NaN()};
  double edge_return_auxiliary_logit_scale{
      std::numeric_limits<double>::quiet_NaN()};
  bool direct_edge_return_readout_enabled{false};
  double direct_edge_return_readout_loss_weight{0.0};
  double direct_edge_return_readout_direction_weight{0.0};
  double direct_edge_return_readout_rank_weight{0.0};
  double direct_edge_return_readout_huber_beta{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_logit_scale{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_target_scale{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_warmup_steps{0};
  double direct_edge_return_readout_warmup_nll_weight{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_post_warmup_nll_weight{
      std::numeric_limits<double>::quiet_NaN()};
  bool direct_edge_return_readout_warmup_direct_head_only{false};
  std::string direct_edge_return_readout_identity_mode{"shared"};
  int64_t direct_edge_return_readout_base_edge_count{0};
  int64_t direct_edge_return_readout_identity_embedding_dim{0};
  int64_t direct_edge_return_readout_adapter_hidden_dim{0};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
  bool allow_untrained_representation{false};
  std::string representation_checkpoint_path{};
  bool representation_checkpoint_loaded{false};
  std::string mdn_checkpoint_path{};
  bool mdn_checkpoint_loaded{false};
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
  bool all_target_masks_forced_empty{false};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_nll_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_nll_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_edge_return_auxiliary_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_edge_return_auxiliary_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_edge_return_auxiliary_regression_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_edge_return_auxiliary_regression_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_edge_return_auxiliary_direction_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_edge_return_auxiliary_direction_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_edge_return_auxiliary_rank_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_edge_return_auxiliary_rank_loss{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t edge_return_auxiliary_valid_count{0};
  int64_t edge_return_auxiliary_pairwise_valid_count{0};
  double last_direct_edge_return_readout_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_regression_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_regression_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_direction_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_direction_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_rank_loss{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_rank_loss{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_loss_valid_count{0};
  int64_t direct_edge_return_readout_loss_pairwise_valid_count{0};
  double last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_loss_abs_to_nll_abs_ratio{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_loss_abs_to_total_abs_ratio{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_loss_abs_to_total_abs_ratio{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_head_grad_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double max_direct_edge_return_readout_head_grad_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_head_grad_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_head_parameter_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_head_parameter_update_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double max_direct_edge_return_readout_head_parameter_update_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_direct_edge_return_readout_head_parameter_update_norm{
      std::numeric_limits<double>::quiet_NaN()};
  double last_direct_edge_return_readout_scheduled_nll_weight{1.0};
  bool last_direct_edge_return_readout_warmup_active{false};
  bool last_direct_edge_return_readout_direct_head_only_warmup_active{false};
  int64_t direct_edge_return_readout_warmup_step_count{0};
  int64_t direct_edge_return_readout_direct_head_only_warmup_step_count{0};
  int64_t last_valid_target_count{0};
  int64_t total_valid_target_count{0};
  int64_t forecast_ev_valid_count{0};
  double ev_mae{std::numeric_limits<double>::quiet_NaN()};
  double ev_rmse{std::numeric_limits<double>::quiet_NaN()};
  double signed_error{std::numeric_limits<double>::quiet_NaN()};
  double directional_accuracy{std::numeric_limits<double>::quiet_NaN()};
  std::vector<int64_t> forecast_ev_valid_count_per_channel{};
  std::vector<double> ev_mae_per_channel{};
  std::vector<double> ev_rmse_per_channel{};
  std::vector<double> signed_error_per_channel{};
  std::vector<double> directional_accuracy_per_channel{};
  std::vector<int64_t> forecast_ev_valid_count_per_target_feature{};
  std::vector<double> ev_mae_per_target_feature{};
  std::vector<double> ev_rmse_per_target_feature{};
  std::vector<double> signed_error_per_target_feature{};
  std::vector<double> directional_accuracy_per_target_feature{};
  std::vector<int64_t> forecast_ev_valid_count_per_channel_target_feature{};
  std::vector<double> ev_mae_per_channel_target_feature{};
  std::vector<double> ev_rmse_per_channel_target_feature{};
  std::vector<double> signed_error_per_channel_target_feature{};
  std::vector<double> directional_accuracy_per_channel_target_feature{};
  std::vector<int64_t> forecast_ev_valid_count_per_node{};
  std::vector<double> ev_mae_per_node{};
  std::vector<double> ev_rmse_per_node{};
  std::vector<double> signed_error_per_node{};
  std::vector<double> directional_accuracy_per_node{};
  std::string edge_return_projection_schema{
      "synthetic_edge_return_projection.anchor_v1"};
  int64_t edge_return_projection_quote_node_index{0};
  std::string edge_return_projection_quote_node_id{};
  std::string edge_return_projection_base_node_ids{};
  int64_t edge_return_projection_close_feature_index{3};
  int64_t edge_return_projection_valid_count{0};
  double edge_return_projection_ev_mae{
      std::numeric_limits<double>::quiet_NaN()};
  double edge_return_projection_ev_rmse{
      std::numeric_limits<double>::quiet_NaN()};
  double edge_return_projection_signed_error{
      std::numeric_limits<double>::quiet_NaN()};
  double edge_return_projection_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double edge_return_projection_correlation{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t edge_return_projection_pairwise_rank_valid_count{0};
  double edge_return_projection_pairwise_rank_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t edge_return_projection_best_asset_valid_count{0};
  double edge_return_projection_best_asset_agreement{
      std::numeric_limits<double>::quiet_NaN()};
  std::vector<int64_t> edge_return_projection_valid_count_per_edge{};
  std::vector<double> edge_return_projection_directional_accuracy_per_edge{};
  std::vector<int64_t> edge_return_projection_valid_count_per_channel{};
  std::vector<double> edge_return_projection_directional_accuracy_per_channel{};
  std::string direct_edge_return_readout_schema{
      "synthetic_direct_edge_return_readout.anchor_v1"};
  int64_t direct_edge_return_readout_quote_node_index{0};
  std::string direct_edge_return_readout_quote_node_id{};
  std::string direct_edge_return_readout_base_node_ids{};
  int64_t direct_edge_return_readout_close_feature_index{3};
  int64_t direct_edge_return_readout_valid_count{0};
  double direct_edge_return_readout_ev_mae{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_ev_rmse{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_signed_error{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_correlation{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_pairwise_rank_valid_count{0};
  double direct_edge_return_readout_pairwise_rank_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_best_asset_valid_count{0};
  double direct_edge_return_readout_best_asset_agreement{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_pred_mean{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_pred_std{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_pred_min{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_pred_max{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_realized_mean{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_realized_std{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_realized_min{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_realized_max{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_pred_to_realized_std_ratio{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_margin_eps{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_margin_valid_count{0};
  double direct_edge_return_readout_margin_directional_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_near_zero_target_count{0};
  double direct_edge_return_readout_near_zero_target_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_rank_margin_eps{
      std::numeric_limits<double>::quiet_NaN()};
  int64_t direct_edge_return_readout_margin_pairwise_rank_valid_count{0};
  double direct_edge_return_readout_margin_pairwise_rank_accuracy{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_edge_min{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_edge_max{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_edge_spread{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_channel_min{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_channel_max{
      std::numeric_limits<double>::quiet_NaN()};
  double direct_edge_return_readout_directional_accuracy_per_channel_spread{
      std::numeric_limits<double>::quiet_NaN()};
  std::vector<int64_t> direct_edge_return_readout_valid_count_per_edge{};
  std::vector<double>
      direct_edge_return_readout_directional_accuracy_per_edge{};
  std::vector<int64_t> direct_edge_return_readout_valid_count_per_channel{};
  std::vector<double>
      direct_edge_return_readout_directional_accuracy_per_channel{};
  double last_valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  double mean_valid_target_fraction{std::numeric_limits<double>::quiet_NaN()};
  double last_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double last_sigma_mean_valid{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean_valid{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min_valid{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max_valid{std::numeric_limits<double>::quiet_NaN()};
  double mean_mixture_entropy{std::numeric_limits<double>::quiet_NaN()};
  std::vector<double> mean_nll_per_channel{};
  std::vector<double> mean_nll_per_target_feature{};
  std::vector<double> mean_nll_per_channel_target_feature{};
  std::vector<double> mean_mixture_usage{};
  std::vector<int64_t> valid_target_count_per_channel{};
  std::vector<int64_t> valid_target_count_per_target_feature{};
  std::vector<int64_t> valid_target_count_per_channel_target_feature{};
  int64_t nonfinite_output_count{0};
  double last_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double max_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};
  bool representation_parameter_device_check{false};
  bool mdn_parameter_device_check{false};

  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  int64_t last_checkpoint_optimizer_step{0};
  std::string checkpoint_path{};
  std::string checkpoint_format{};
  bool report_written{false};
  int64_t report_write_count{0};
  int64_t last_report_attempted_step{0};
  std::string report_path{};
  bool representation_edge_feature_probe_written{false};
  int64_t representation_edge_feature_probe_row_count{0};
  std::string representation_edge_feature_probe_path{};
  std::string representation_edge_feature_probe_error{};
  bool mdn_edge_context_feature_probe_written{false};
  int64_t mdn_edge_context_feature_probe_row_count{0};
  std::string mdn_edge_context_feature_probe_path{};
  std::string mdn_edge_context_feature_probe_error{};
  bool runtime_lls_emitted{false};
  std::string nodelift_runtime_lls{};
  std::string representation_runtime_lls{};
  std::string mdn_runtime_lls{};

  static void append_double_list(std::ostringstream &oss, const char *key,
                                 const std::vector<double> &values) {
    oss << key << "=";
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << values[i];
    }
    oss << "\n";
  }

  static void append_i64_list(std::ostringstream &oss, const char *key,
                              const std::vector<int64_t> &values) {
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
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "component_assembly_id=" << component_assembly_id << "\n";
    oss << "input_representation_assembly_id="
        << input_representation_assembly_id << "\n";
    oss << "context_contract=" << context_contract << "\n";
    oss << "context_value_shape=" << context_value_shape << "\n";
    oss << "output_contract=" << output_contract << "\n";
    oss << "output_value_shape=" << output_value_shape << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "channel_count=" << channel_count << "\n";
    oss << "context_dim=" << context_dim << "\n";
    oss << "future_horizon=" << future_horizon << "\n";
    oss << "context_mode=" << context_mode << "\n";
    oss << "target_domain=" << target_domain << "\n";
    oss << "target_mask_policy=" << target_mask_policy << "\n";
    oss << "activity_target=" << activity_target << "\n";
    oss << "mixture_count=" << mixture_count << "\n";
    oss << "hidden_width=" << hidden_width << "\n";
    oss << "residual_depth=" << residual_depth << "\n";
    oss << "feature_embedding_dim=" << feature_embedding_dim << "\n";
    oss << "channel_adapter_rank=" << channel_adapter_rank << "\n";
    oss << "mdn_architecture=" << mdn_architecture << "\n";
    oss << "loss_reduction=" << loss_reduction << "\n";
    oss << "shared_trunk=" << (shared_trunk ? "true" : "false") << "\n";
    oss << "channel_adapters_enabled="
        << (channel_adapters_enabled ? "true" : "false") << "\n";
    oss << "shared_feature_head=" << (shared_feature_head ? "true" : "false")
        << "\n";
    oss << "feature_embedding_enabled="
        << (feature_embedding_enabled ? "true" : "false") << "\n";
    oss << "node_id_embedding=" << (node_id_embedding ? "true" : "false")
        << "\n";
    oss << "cross_node_attention=" << (cross_node_attention ? "true" : "false")
        << "\n";
    oss << "cross_channel_attention="
        << (cross_channel_attention ? "true" : "false") << "\n";
    oss << "sigma_min=" << sigma_min << "\n";
    oss << "sigma_max=" << sigma_max << "\n";
    oss << "eps=" << eps << "\n";
    oss << "edge_return_auxiliary_loss_weight="
        << edge_return_auxiliary_loss_weight << "\n";
    oss << "edge_return_auxiliary_direction_weight="
        << edge_return_auxiliary_direction_weight << "\n";
    oss << "edge_return_auxiliary_rank_weight="
        << edge_return_auxiliary_rank_weight << "\n";
    oss << "edge_return_auxiliary_huber_beta="
        << edge_return_auxiliary_huber_beta << "\n";
    oss << "edge_return_auxiliary_logit_scale="
        << edge_return_auxiliary_logit_scale << "\n";
    oss << "direct_edge_return_readout_enabled="
        << (direct_edge_return_readout_enabled ? "true" : "false") << "\n";
    oss << "direct_edge_return_readout_loss_weight="
        << direct_edge_return_readout_loss_weight << "\n";
    oss << "direct_edge_return_readout_direction_weight="
        << direct_edge_return_readout_direction_weight << "\n";
    oss << "direct_edge_return_readout_rank_weight="
        << direct_edge_return_readout_rank_weight << "\n";
    oss << "direct_edge_return_readout_huber_beta="
        << direct_edge_return_readout_huber_beta << "\n";
    oss << "direct_edge_return_readout_logit_scale="
        << direct_edge_return_readout_logit_scale << "\n";
    oss << "direct_edge_return_readout_target_scale="
        << direct_edge_return_readout_target_scale << "\n";
    oss << "direct_edge_return_readout_warmup_steps="
        << direct_edge_return_readout_warmup_steps << "\n";
    oss << "direct_edge_return_readout_warmup_nll_weight="
        << direct_edge_return_readout_warmup_nll_weight << "\n";
    oss << "direct_edge_return_readout_post_warmup_nll_weight="
        << direct_edge_return_readout_post_warmup_nll_weight << "\n";
    oss << "direct_edge_return_readout_warmup_direct_head_only="
        << (direct_edge_return_readout_warmup_direct_head_only ? "true"
                                                               : "false")
        << "\n";
    oss << "direct_edge_return_readout_identity_mode="
        << direct_edge_return_readout_identity_mode << "\n";
    oss << "direct_edge_return_readout_base_edge_count="
        << direct_edge_return_readout_base_edge_count << "\n";
    oss << "direct_edge_return_readout_identity_embedding_dim="
        << direct_edge_return_readout_identity_embedding_dim << "\n";
    oss << "direct_edge_return_readout_adapter_hidden_dim="
        << direct_edge_return_readout_adapter_hidden_dim << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
    oss << "allow_untrained_representation="
        << (allow_untrained_representation ? "true" : "false") << "\n";
    oss << "representation_checkpoint_path=" << representation_checkpoint_path
        << "\n";
    oss << "representation_checkpoint_loaded="
        << (representation_checkpoint_loaded ? "true" : "false") << "\n";
    oss << "mdn_checkpoint_path=" << mdn_checkpoint_path << "\n";
    oss << "mdn_checkpoint_loaded="
        << (mdn_checkpoint_loaded ? "true" : "false") << "\n";
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
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    oss << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    oss << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    oss << "target_coords=";
    for (std::size_t i = 0; i < target_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << target_coords[i];
    }
    oss << "\nnode_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << node_ids[i];
    }
    oss << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << edge_ids[i];
    }
    oss << "\nsteps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    oss << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    oss << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    oss << "all_target_masks_forced_empty="
        << (all_target_masks_forced_empty ? "true" : "false") << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_nll_loss=" << last_nll_loss << "\n";
    oss << "mean_nll_loss=" << mean_nll_loss << "\n";
    oss << "last_edge_return_auxiliary_loss=" << last_edge_return_auxiliary_loss
        << "\n";
    oss << "mean_edge_return_auxiliary_loss=" << mean_edge_return_auxiliary_loss
        << "\n";
    oss << "last_edge_return_auxiliary_regression_loss="
        << last_edge_return_auxiliary_regression_loss << "\n";
    oss << "mean_edge_return_auxiliary_regression_loss="
        << mean_edge_return_auxiliary_regression_loss << "\n";
    oss << "last_edge_return_auxiliary_direction_loss="
        << last_edge_return_auxiliary_direction_loss << "\n";
    oss << "mean_edge_return_auxiliary_direction_loss="
        << mean_edge_return_auxiliary_direction_loss << "\n";
    oss << "last_edge_return_auxiliary_rank_loss="
        << last_edge_return_auxiliary_rank_loss << "\n";
    oss << "mean_edge_return_auxiliary_rank_loss="
        << mean_edge_return_auxiliary_rank_loss << "\n";
    oss << "edge_return_auxiliary_valid_count="
        << edge_return_auxiliary_valid_count << "\n";
    oss << "edge_return_auxiliary_pairwise_valid_count="
        << edge_return_auxiliary_pairwise_valid_count << "\n";
    oss << "last_direct_edge_return_readout_loss="
        << last_direct_edge_return_readout_loss << "\n";
    oss << "mean_direct_edge_return_readout_loss="
        << mean_direct_edge_return_readout_loss << "\n";
    oss << "last_direct_edge_return_readout_regression_loss="
        << last_direct_edge_return_readout_regression_loss << "\n";
    oss << "mean_direct_edge_return_readout_regression_loss="
        << mean_direct_edge_return_readout_regression_loss << "\n";
    oss << "last_direct_edge_return_readout_direction_loss="
        << last_direct_edge_return_readout_direction_loss << "\n";
    oss << "mean_direct_edge_return_readout_direction_loss="
        << mean_direct_edge_return_readout_direction_loss << "\n";
    oss << "last_direct_edge_return_readout_rank_loss="
        << last_direct_edge_return_readout_rank_loss << "\n";
    oss << "mean_direct_edge_return_readout_rank_loss="
        << mean_direct_edge_return_readout_rank_loss << "\n";
    oss << "direct_edge_return_readout_loss_valid_count="
        << direct_edge_return_readout_loss_valid_count << "\n";
    oss << "direct_edge_return_readout_loss_pairwise_valid_count="
        << direct_edge_return_readout_loss_pairwise_valid_count << "\n";
    oss << "last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio="
        << last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio << "\n";
    oss << "mean_direct_edge_return_readout_loss_abs_to_nll_abs_ratio="
        << mean_direct_edge_return_readout_loss_abs_to_nll_abs_ratio << "\n";
    oss << "last_direct_edge_return_readout_loss_abs_to_total_abs_ratio="
        << last_direct_edge_return_readout_loss_abs_to_total_abs_ratio << "\n";
    oss << "mean_direct_edge_return_readout_loss_abs_to_total_abs_ratio="
        << mean_direct_edge_return_readout_loss_abs_to_total_abs_ratio << "\n";
    oss << "last_direct_edge_return_readout_head_grad_norm="
        << last_direct_edge_return_readout_head_grad_norm << "\n";
    oss << "max_direct_edge_return_readout_head_grad_norm="
        << max_direct_edge_return_readout_head_grad_norm << "\n";
    oss << "mean_direct_edge_return_readout_head_grad_norm="
        << mean_direct_edge_return_readout_head_grad_norm << "\n";
    oss << "last_direct_edge_return_readout_head_parameter_norm="
        << last_direct_edge_return_readout_head_parameter_norm << "\n";
    oss << "last_direct_edge_return_readout_head_parameter_update_norm="
        << last_direct_edge_return_readout_head_parameter_update_norm << "\n";
    oss << "max_direct_edge_return_readout_head_parameter_update_norm="
        << max_direct_edge_return_readout_head_parameter_update_norm << "\n";
    oss << "mean_direct_edge_return_readout_head_parameter_update_norm="
        << mean_direct_edge_return_readout_head_parameter_update_norm << "\n";
    oss << "last_direct_edge_return_readout_scheduled_nll_weight="
        << last_direct_edge_return_readout_scheduled_nll_weight << "\n";
    oss << "last_direct_edge_return_readout_warmup_active="
        << (last_direct_edge_return_readout_warmup_active ? "true" : "false")
        << "\n";
    oss << "last_direct_edge_return_readout_direct_head_only_warmup_active="
        << (last_direct_edge_return_readout_direct_head_only_warmup_active
                ? "true"
                : "false")
        << "\n";
    oss << "direct_edge_return_readout_warmup_step_count="
        << direct_edge_return_readout_warmup_step_count << "\n";
    oss << "direct_edge_return_readout_direct_head_only_warmup_step_count="
        << direct_edge_return_readout_direct_head_only_warmup_step_count
        << "\n";
    oss << "last_valid_target_count=" << last_valid_target_count << "\n";
    oss << "total_valid_target_count=" << total_valid_target_count << "\n";
    oss << "forecast_ev_valid_count=" << forecast_ev_valid_count << "\n";
    oss << "ev_mae=" << ev_mae << "\n";
    oss << "ev_rmse=" << ev_rmse << "\n";
    oss << "signed_error=" << signed_error << "\n";
    oss << "directional_accuracy=" << directional_accuracy << "\n";
    append_i64_list(oss, "forecast_ev_valid_count_per_channel",
                    forecast_ev_valid_count_per_channel);
    append_double_list(oss, "ev_mae_per_channel", ev_mae_per_channel);
    append_double_list(oss, "ev_rmse_per_channel", ev_rmse_per_channel);
    append_double_list(oss, "signed_error_per_channel",
                       signed_error_per_channel);
    append_double_list(oss, "directional_accuracy_per_channel",
                       directional_accuracy_per_channel);
    append_i64_list(oss, "forecast_ev_valid_count_per_target_feature",
                    forecast_ev_valid_count_per_target_feature);
    append_double_list(oss, "ev_mae_per_target_feature",
                       ev_mae_per_target_feature);
    append_double_list(oss, "ev_rmse_per_target_feature",
                       ev_rmse_per_target_feature);
    append_double_list(oss, "signed_error_per_target_feature",
                       signed_error_per_target_feature);
    append_double_list(oss, "directional_accuracy_per_target_feature",
                       directional_accuracy_per_target_feature);
    append_i64_list(oss, "forecast_ev_valid_count_per_channel_target_feature",
                    forecast_ev_valid_count_per_channel_target_feature);
    append_double_list(oss, "ev_mae_per_channel_target_feature",
                       ev_mae_per_channel_target_feature);
    append_double_list(oss, "ev_rmse_per_channel_target_feature",
                       ev_rmse_per_channel_target_feature);
    append_double_list(oss, "signed_error_per_channel_target_feature",
                       signed_error_per_channel_target_feature);
    append_double_list(oss, "directional_accuracy_per_channel_target_feature",
                       directional_accuracy_per_channel_target_feature);
    append_i64_list(oss, "forecast_ev_valid_count_per_node",
                    forecast_ev_valid_count_per_node);
    append_double_list(oss, "ev_mae_per_node", ev_mae_per_node);
    append_double_list(oss, "ev_rmse_per_node", ev_rmse_per_node);
    append_double_list(oss, "signed_error_per_node", signed_error_per_node);
    append_double_list(oss, "directional_accuracy_per_node",
                       directional_accuracy_per_node);
    oss << "edge_return_projection_schema=" << edge_return_projection_schema
        << "\n";
    oss << "edge_return_projection_quote_node_index="
        << edge_return_projection_quote_node_index << "\n";
    oss << "edge_return_projection_quote_node_id="
        << edge_return_projection_quote_node_id << "\n";
    oss << "edge_return_projection_base_node_ids="
        << edge_return_projection_base_node_ids << "\n";
    oss << "edge_return_projection_close_feature_index="
        << edge_return_projection_close_feature_index << "\n";
    oss << "edge_return_projection_valid_count="
        << edge_return_projection_valid_count << "\n";
    oss << "edge_return_projection_ev_mae=" << edge_return_projection_ev_mae
        << "\n";
    oss << "edge_return_projection_ev_rmse=" << edge_return_projection_ev_rmse
        << "\n";
    oss << "edge_return_projection_signed_error="
        << edge_return_projection_signed_error << "\n";
    oss << "edge_return_projection_directional_accuracy="
        << edge_return_projection_directional_accuracy << "\n";
    oss << "edge_return_projection_correlation="
        << edge_return_projection_correlation << "\n";
    oss << "edge_return_projection_pairwise_rank_valid_count="
        << edge_return_projection_pairwise_rank_valid_count << "\n";
    oss << "edge_return_projection_pairwise_rank_accuracy="
        << edge_return_projection_pairwise_rank_accuracy << "\n";
    oss << "edge_return_projection_best_asset_valid_count="
        << edge_return_projection_best_asset_valid_count << "\n";
    oss << "edge_return_projection_best_asset_agreement="
        << edge_return_projection_best_asset_agreement << "\n";
    append_i64_list(oss, "edge_return_projection_valid_count_per_edge",
                    edge_return_projection_valid_count_per_edge);
    append_double_list(oss,
                       "edge_return_projection_directional_accuracy_per_edge",
                       edge_return_projection_directional_accuracy_per_edge);
    append_i64_list(oss, "edge_return_projection_valid_count_per_channel",
                    edge_return_projection_valid_count_per_channel);
    append_double_list(
        oss, "edge_return_projection_directional_accuracy_per_channel",
        edge_return_projection_directional_accuracy_per_channel);
    oss << "direct_edge_return_readout_schema="
        << direct_edge_return_readout_schema << "\n";
    oss << "direct_edge_return_readout_quote_node_index="
        << direct_edge_return_readout_quote_node_index << "\n";
    oss << "direct_edge_return_readout_quote_node_id="
        << direct_edge_return_readout_quote_node_id << "\n";
    oss << "direct_edge_return_readout_base_node_ids="
        << direct_edge_return_readout_base_node_ids << "\n";
    oss << "direct_edge_return_readout_close_feature_index="
        << direct_edge_return_readout_close_feature_index << "\n";
    oss << "direct_edge_return_readout_valid_count="
        << direct_edge_return_readout_valid_count << "\n";
    oss << "direct_edge_return_readout_ev_mae="
        << direct_edge_return_readout_ev_mae << "\n";
    oss << "direct_edge_return_readout_ev_rmse="
        << direct_edge_return_readout_ev_rmse << "\n";
    oss << "direct_edge_return_readout_signed_error="
        << direct_edge_return_readout_signed_error << "\n";
    oss << "direct_edge_return_readout_directional_accuracy="
        << direct_edge_return_readout_directional_accuracy << "\n";
    oss << "direct_edge_return_readout_correlation="
        << direct_edge_return_readout_correlation << "\n";
    oss << "direct_edge_return_readout_pairwise_rank_valid_count="
        << direct_edge_return_readout_pairwise_rank_valid_count << "\n";
    oss << "direct_edge_return_readout_pairwise_rank_accuracy="
        << direct_edge_return_readout_pairwise_rank_accuracy << "\n";
    oss << "direct_edge_return_readout_best_asset_valid_count="
        << direct_edge_return_readout_best_asset_valid_count << "\n";
    oss << "direct_edge_return_readout_best_asset_agreement="
        << direct_edge_return_readout_best_asset_agreement << "\n";
    oss << "direct_edge_return_readout_pred_mean="
        << direct_edge_return_readout_pred_mean << "\n";
    oss << "direct_edge_return_readout_pred_std="
        << direct_edge_return_readout_pred_std << "\n";
    oss << "direct_edge_return_readout_pred_min="
        << direct_edge_return_readout_pred_min << "\n";
    oss << "direct_edge_return_readout_pred_max="
        << direct_edge_return_readout_pred_max << "\n";
    oss << "direct_edge_return_readout_realized_mean="
        << direct_edge_return_readout_realized_mean << "\n";
    oss << "direct_edge_return_readout_realized_std="
        << direct_edge_return_readout_realized_std << "\n";
    oss << "direct_edge_return_readout_realized_min="
        << direct_edge_return_readout_realized_min << "\n";
    oss << "direct_edge_return_readout_realized_max="
        << direct_edge_return_readout_realized_max << "\n";
    oss << "direct_edge_return_readout_pred_to_realized_std_ratio="
        << direct_edge_return_readout_pred_to_realized_std_ratio << "\n";
    oss << "direct_edge_return_readout_margin_eps="
        << direct_edge_return_readout_margin_eps << "\n";
    oss << "direct_edge_return_readout_margin_valid_count="
        << direct_edge_return_readout_margin_valid_count << "\n";
    oss << "direct_edge_return_readout_margin_directional_accuracy="
        << direct_edge_return_readout_margin_directional_accuracy << "\n";
    oss << "direct_edge_return_readout_near_zero_target_count="
        << direct_edge_return_readout_near_zero_target_count << "\n";
    oss << "direct_edge_return_readout_near_zero_target_fraction="
        << direct_edge_return_readout_near_zero_target_fraction << "\n";
    oss << "direct_edge_return_readout_rank_margin_eps="
        << direct_edge_return_readout_rank_margin_eps << "\n";
    oss << "direct_edge_return_readout_margin_pairwise_rank_valid_count="
        << direct_edge_return_readout_margin_pairwise_rank_valid_count << "\n";
    oss << "direct_edge_return_readout_margin_pairwise_rank_accuracy="
        << direct_edge_return_readout_margin_pairwise_rank_accuracy << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_edge_min="
        << direct_edge_return_readout_directional_accuracy_per_edge_min << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_edge_max="
        << direct_edge_return_readout_directional_accuracy_per_edge_max << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_edge_spread="
        << direct_edge_return_readout_directional_accuracy_per_edge_spread
        << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_channel_min="
        << direct_edge_return_readout_directional_accuracy_per_channel_min
        << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_channel_max="
        << direct_edge_return_readout_directional_accuracy_per_channel_max
        << "\n";
    oss << "direct_edge_return_readout_directional_accuracy_per_channel_spread="
        << direct_edge_return_readout_directional_accuracy_per_channel_spread
        << "\n";
    append_i64_list(oss, "direct_edge_return_readout_valid_count_per_edge",
                    direct_edge_return_readout_valid_count_per_edge);
    append_double_list(
        oss, "direct_edge_return_readout_directional_accuracy_per_edge",
        direct_edge_return_readout_directional_accuracy_per_edge);
    append_i64_list(oss, "direct_edge_return_readout_valid_count_per_channel",
                    direct_edge_return_readout_valid_count_per_channel);
    append_double_list(
        oss, "direct_edge_return_readout_directional_accuracy_per_channel",
        direct_edge_return_readout_directional_accuracy_per_channel);
    oss << "last_valid_target_fraction=" << last_valid_target_fraction << "\n";
    oss << "mean_valid_target_fraction=" << mean_valid_target_fraction << "\n";
    oss << "last_sigma_mean=" << last_sigma_mean << "\n";
    oss << "mean_sigma_mean=" << mean_sigma_mean << "\n";
    oss << "min_sigma_min=" << min_sigma_min << "\n";
    oss << "max_sigma_max=" << max_sigma_max << "\n";
    oss << "last_sigma_mean_valid=" << last_sigma_mean_valid << "\n";
    oss << "mean_sigma_mean_valid=" << mean_sigma_mean_valid << "\n";
    oss << "min_sigma_min_valid=" << min_sigma_min_valid << "\n";
    oss << "max_sigma_max_valid=" << max_sigma_max_valid << "\n";
    oss << "mean_mixture_entropy=" << mean_mixture_entropy << "\n";
    append_double_list(oss, "mean_nll_per_channel", mean_nll_per_channel);
    append_double_list(oss, "mean_nll_per_target_feature",
                       mean_nll_per_target_feature);
    append_double_list(oss, "mean_nll_per_channel_target_feature",
                       mean_nll_per_channel_target_feature);
    append_double_list(oss, "mean_mixture_usage", mean_mixture_usage);
    append_i64_list(oss, "valid_target_count_per_channel",
                    valid_target_count_per_channel);
    append_i64_list(oss, "valid_target_count_per_target_feature",
                    valid_target_count_per_target_feature);
    append_i64_list(oss, "valid_target_count_per_channel_target_feature",
                    valid_target_count_per_channel_target_feature);
    oss << "nonfinite_output_count=" << nonfinite_output_count << "\n";
    oss << "last_grad_norm=" << last_grad_norm << "\n";
    oss << "max_grad_norm=" << max_grad_norm << "\n";
    oss << "finite_parameter_check=" << finite_parameter_check << "\n";
    oss << "representation_parameter_device_check="
        << (representation_parameter_device_check ? "true" : "false") << "\n";
    oss << "mdn_parameter_device_check="
        << (mdn_parameter_device_check ? "true" : "false") << "\n";
    oss << "checkpoint_written=" << (checkpoint_written ? "true" : "false")
        << "\n";
    oss << "checkpoint_write_count=" << checkpoint_write_count << "\n";
    oss << "last_checkpoint_optimizer_step=" << last_checkpoint_optimizer_step
        << "\n";
    oss << "checkpoint_path=" << checkpoint_path << "\n";
    oss << "checkpoint_format=" << checkpoint_format << "\n";
    oss << "report_written=" << (report_written ? "true" : "false") << "\n";
    oss << "report_write_count=" << report_write_count << "\n";
    oss << "last_report_attempted_step=" << last_report_attempted_step << "\n";
    oss << "report_path=" << report_path << "\n";
    oss << "representation_edge_feature_probe_written="
        << (representation_edge_feature_probe_written ? "true" : "false")
        << "\n";
    oss << "representation_edge_feature_probe_row_count="
        << representation_edge_feature_probe_row_count << "\n";
    oss << "representation_edge_feature_probe_path="
        << representation_edge_feature_probe_path << "\n";
    oss << "representation_edge_feature_probe_error="
        << representation_edge_feature_probe_error << "\n";
    oss << "mdn_edge_context_feature_probe_written="
        << (mdn_edge_context_feature_probe_written ? "true" : "false") << "\n";
    oss << "mdn_edge_context_feature_probe_row_count="
        << mdn_edge_context_feature_probe_row_count << "\n";
    oss << "mdn_edge_context_feature_probe_path="
        << mdn_edge_context_feature_probe_path << "\n";
    oss << "mdn_edge_context_feature_probe_error="
        << mdn_edge_context_feature_probe_error << "\n";
    oss << "runtime_lls_emitted=" << (runtime_lls_emitted ? "true" : "false")
        << "\n";
    return oss.str();
  }
};

namespace channel_graph_first_inference_launcher_detail {

template <typename KeyT>
inline std::string optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  std::ostringstream oss;
  oss << *value;
  return oss.str();
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

[[nodiscard]] inline bool
parameters_are_on_device(const std::vector<torch::Tensor> &params,
                         const torch::Device &expected_device) {
  if (params.empty()) {
    return false;
  }
  for (const auto &param : params) {
    if (!param.defined()) {
      continue;
    }
    const auto actual_device = param.device();
    if (expected_device.is_cuda()) {
      if (!actual_device.is_cuda()) {
        return false;
      }
      if (expected_device.has_index() &&
          actual_device.index() != expected_device.index()) {
        return false;
      }
      continue;
    }
    if (actual_device != expected_device) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline torch::Tensor
int64_tensor_from_vector(const std::vector<int64_t> &values) {
  const auto opts = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, opts);
  }
  return torch::tensor(values, opts);
}

[[nodiscard]] inline std::vector<int64_t>
string_to_bytes(const std::string &value) {
  std::vector<int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<int64_t>(ch));
  }
  return bytes;
}

[[nodiscard]] inline std::pair<std::vector<int64_t>, std::vector<int64_t>>
string_list_to_lengths_and_bytes(const std::vector<std::string> &values) {
  std::vector<int64_t> lengths;
  std::vector<int64_t> bytes;
  lengths.reserve(values.size());
  for (const auto &value : values) {
    lengths.push_back(static_cast<int64_t>(value.size()));
    for (const unsigned char ch : value) {
      bytes.push_back(static_cast<int64_t>(ch));
    }
  }
  return {std::move(lengths), std::move(bytes)};
}

[[nodiscard]] inline std::vector<int64_t>
tensor_to_int64_vector(const torch::Tensor &tensor) {
  if (!tensor.defined()) {
    return {};
  }
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  std::vector<int64_t> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<int64_t, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

[[nodiscard]] inline bool metadata_string_matches(const torch::Tensor &tensor,
                                                  const std::string &expected) {
  return tensor_to_int64_vector(tensor) == string_to_bytes(expected);
}

[[nodiscard]] inline bool metadata_i64_matches(const torch::Tensor &tensor,
                                               int64_t expected) {
  return tensor.defined() && tensor.numel() == 1 &&
         tensor.to(torch::kCPU).to(torch::kInt64).template item<int64_t>() ==
             expected;
}

[[nodiscard]] inline bool metadata_bool_matches(const torch::Tensor &tensor,
                                                bool expected) {
  return metadata_i64_matches(tensor, expected ? 1 : 0);
}

[[nodiscard]] inline bool metadata_double_matches(const torch::Tensor &tensor,
                                                  double expected,
                                                  double tolerance = 1e-12) {
  if (!tensor.defined() || tensor.numel() != 1 || !std::isfinite(expected)) {
    return false;
  }
  const double actual =
      tensor.to(torch::kCPU).to(torch::kFloat64).template item<double>();
  return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}

[[nodiscard]] inline std::vector<double>
tensor_to_double_vector(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return {};
  }
  auto cpu = tensor.detach().to(torch::kCPU).to(torch::kFloat64).contiguous();
  cpu = cpu.reshape({cpu.numel()});
  std::vector<double> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<double, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

inline void accumulate_finite_vector(const torch::Tensor &tensor,
                                     std::vector<double> &sum,
                                     std::vector<int64_t> &count) {
  const auto values = tensor_to_double_vector(tensor);
  if (values.empty()) {
    return;
  }
  if (sum.size() < values.size()) {
    sum.resize(values.size(), 0.0);
    count.resize(values.size(), 0);
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (std::isfinite(values[i])) {
      sum[i] += values[i];
      ++count[i];
    }
  }
}

inline void accumulate_i64_vector(const torch::Tensor &tensor,
                                  std::vector<int64_t> &sum) {
  const auto values = tensor_to_int64_vector(tensor);
  if (values.empty()) {
    return;
  }
  if (sum.size() < values.size()) {
    sum.resize(values.size(), 0);
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    sum[i] += values[i];
  }
}

inline void accumulate_double_sum_vector(const torch::Tensor &tensor,
                                         std::vector<double> &sum) {
  const auto values = tensor_to_double_vector(tensor);
  if (values.empty()) {
    return;
  }
  if (sum.size() < values.size()) {
    sum.resize(values.size(), 0.0);
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (std::isfinite(values[i])) {
      sum[i] += values[i];
    }
  }
}

[[nodiscard]] inline std::vector<double>
mean_vector(const std::vector<double> &sum, const std::vector<int64_t> &count) {
  std::vector<double> out;
  out.reserve(sum.size());
  for (std::size_t i = 0; i < sum.size(); ++i) {
    out.push_back(i < count.size() && count[i] > 0
                      ? sum[i] / static_cast<double>(count[i])
                      : std::numeric_limits<double>::quiet_NaN());
  }
  return out;
}

[[nodiscard]] inline std::vector<double>
ratio_vector(const std::vector<double> &sum,
             const std::vector<int64_t> &count) {
  std::vector<double> out;
  out.reserve(sum.size());
  for (std::size_t i = 0; i < sum.size(); ++i) {
    out.push_back(i < count.size() && count[i] > 0
                      ? sum[i] / static_cast<double>(count[i])
                      : std::numeric_limits<double>::quiet_NaN());
  }
  return out;
}

[[nodiscard]] inline std::vector<double>
rmse_vector(const std::vector<double> &squared_sum,
            const std::vector<int64_t> &count) {
  std::vector<double> out;
  out.reserve(squared_sum.size());
  for (std::size_t i = 0; i < squared_sum.size(); ++i) {
    out.push_back(
        i < count.size() && count[i] > 0
            ? std::sqrt(squared_sum[i] / static_cast<double>(count[i]))
            : std::numeric_limits<double>::quiet_NaN());
  }
  return out;
}

[[nodiscard]] inline std::vector<double>
directional_accuracy_vector(const std::vector<int64_t> &correct,
                            const std::vector<int64_t> &count) {
  std::vector<double> out;
  out.reserve(correct.size());
  for (std::size_t i = 0; i < correct.size(); ++i) {
    out.push_back(i < count.size() && count[i] > 0
                      ? static_cast<double>(correct[i]) /
                            static_cast<double>(count[i])
                      : std::numeric_limits<double>::quiet_NaN());
  }
  return out;
}

[[nodiscard]] inline double
finite_min_value(const std::vector<double> &values) {
  double out = std::numeric_limits<double>::infinity();
  bool found = false;
  for (const double value : values) {
    if (std::isfinite(value)) {
      out = std::min(out, value);
      found = true;
    }
  }
  return found ? out : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline double
finite_max_value(const std::vector<double> &values) {
  double out = -std::numeric_limits<double>::infinity();
  bool found = false;
  for (const double value : values) {
    if (std::isfinite(value)) {
      out = std::max(out, value);
      found = true;
    }
  }
  return found ? out : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline double
finite_spread_value(const std::vector<double> &values) {
  const double min_value = finite_min_value(values);
  const double max_value = finite_max_value(values);
  return std::isfinite(min_value) && std::isfinite(max_value)
             ? max_value - min_value
             : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline double stddev_from_sums(const int64_t count,
                                             const double sum,
                                             const double squared_sum) {
  if (count <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double mean = sum / static_cast<double>(count);
  const double variance =
      std::max(0.0, squared_sum / static_cast<double>(count) - mean * mean);
  return std::sqrt(variance);
}

[[nodiscard]] inline double safe_abs_ratio(const double numerator,
                                           const double denominator) {
  if (!std::isfinite(numerator) || !std::isfinite(denominator)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  constexpr double kMinDenominator = 1.0e-12;
  const double abs_denominator = std::abs(denominator);
  if (abs_denominator < kMinDenominator) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::abs(numerator) / abs_denominator;
}

[[nodiscard]] inline double correlation_from_sums(
    const int64_t count, const double predicted_sum, const double realized_sum,
    const double predicted_squared_sum, const double realized_squared_sum,
    const double cross_sum) {
  if (count <= 1) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double n = static_cast<double>(count);
  const double predicted_variance_term =
      n * predicted_squared_sum - predicted_sum * predicted_sum;
  const double realized_variance_term =
      n * realized_squared_sum - realized_sum * realized_sum;
  const double covariance_term = n * cross_sum - predicted_sum * realized_sum;
  if (predicted_variance_term <= 0.0 || realized_variance_term <= 0.0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return covariance_term /
         std::sqrt(predicted_variance_term * realized_variance_term);
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
        std::string("[channel_graph_first_inference_launcher] failed to "
                    "create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void write_report_file(
    const std::filesystem::path &path,
    const channel_graph_first_inference_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] failed to open report path");
  }
  out << report.to_text();
}

inline void write_text_file(const std::filesystem::path &path,
                            const std::string &payload, const char *path_kind) {
  if (path.empty() || payload.empty()) {
    return;
  }
  ensure_parent_directory(path, path_kind);
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        std::string(
            "[channel_graph_first_inference_launcher] failed to open ") +
        path_kind + " path");
  }
  out << payload;
}

template <typename KeyT>
[[nodiscard]] inline int64_t append_representation_edge_feature_probe_rows(
    const std::filesystem::path &path,
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &representation_batch,
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        channel_mdn_input_batch_t<KeyT> &mdn_batch,
    int64_t close_feature_index = 3) {
  if (path.empty() || mdn_batch.node_ids.size() < 2) {
    return 0;
  }
  const auto close_it =
      std::find(mdn_batch.target_coords.begin(), mdn_batch.target_coords.end(),
                close_feature_index);
  if (close_it == mdn_batch.target_coords.end()) {
    return 0;
  }
  const int64_t close_slot = static_cast<int64_t>(
      std::distance(mdn_batch.target_coords.begin(), close_it));
  TORCH_CHECK(representation_batch.node_encoding.defined() &&
                  representation_batch.node_encoding.dim() == 4,
              "[channel_graph_first_inference_launcher] representation edge "
              "feature probe requires node_encoding [B,N,C,De]");
  TORCH_CHECK(mdn_batch.future.defined() && mdn_batch.future.dim() == 4,
              "[channel_graph_first_inference_launcher] representation edge "
              "feature probe requires future [B,N,C,Df]");
  TORCH_CHECK(mdn_batch.future_mask.defined() &&
                  mdn_batch.future_mask.dim() == 4,
              "[channel_graph_first_inference_launcher] representation edge "
              "feature probe requires future_mask [B,N,C,Df]");

  const auto context = representation_batch.node_encoding.detach()
                           .to(torch::kCPU)
                           .to(torch::kFloat64)
                           .contiguous();
  const auto future = mdn_batch.future.detach()
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  const auto future_mask = mdn_batch.future_mask.detach()
                               .to(torch::kCPU)
                               .to(torch::kBool)
                               .contiguous();
  const auto anchor_keys = mdn_batch.anchor_keys.detach()
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();

  const int64_t B = context.size(0);
  const int64_t N = context.size(1);
  const int64_t C = context.size(2);
  const int64_t De = context.size(3);
  TORCH_CHECK(future.size(0) == B && future.size(1) == N && future.size(2) == C,
              "[channel_graph_first_inference_launcher] future shape does not "
              "match representation context for edge feature probe");
  TORCH_CHECK(future_mask.sizes() == future.sizes(),
              "[channel_graph_first_inference_launcher] future_mask shape does "
              "not match future for edge feature probe");
  TORCH_CHECK(anchor_keys.defined() && anchor_keys.dim() == 1 &&
                  anchor_keys.size(0) == B,
              "[channel_graph_first_inference_launcher] anchor_keys shape does "
              "not match representation context for edge feature probe");
  if (close_slot < 0 || close_slot >= future.size(3)) {
    return 0;
  }

  ensure_parent_directory(path, "representation edge feature probe");
  const bool write_header =
      !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;
  std::ofstream out(path, std::ios::app);
  if (!out) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] failed to open "
        "representation edge feature probe path");
  }
  if (write_header) {
    out << "record_schema,anchor_key,anchor_index,anchor_local_index,edge_"
           "index,edge_id,"
           "base_node_id,quote_node_id,channel_index,"
           "target_edge_close_return,feature_count,feature_values\n";
  }

  constexpr int64_t quote_node_index = 0;
  int64_t row_count = 0;
  out << std::setprecision(17);
  for (int64_t b = 0; b < B; ++b) {
    const int64_t anchor_key =
        anchor_keys.select(0, b).template item<int64_t>();
    const int64_t anchor_index =
        static_cast<int64_t>(representation_batch.cursor.begin_anchor_index) +
        b;
    for (int64_t base = 1; base < N; ++base) {
      const int64_t edge_index = base - 1;
      const std::string edge_id =
          edge_index < static_cast<int64_t>(mdn_batch.edge_ids.size())
              ? mdn_batch.edge_ids[static_cast<std::size_t>(edge_index)]
              : std::string{};
      const auto &base_node_id =
          mdn_batch.node_ids[static_cast<std::size_t>(base)];
      const auto &quote_node_id =
          mdn_batch.node_ids[static_cast<std::size_t>(quote_node_index)];
      for (int64_t c = 0; c < C; ++c) {
        const bool base_valid =
            future_mask.index({b, base, c, close_slot}).template item<bool>();
        const bool quote_valid =
            future_mask.index({b, quote_node_index, c, close_slot})
                .template item<bool>();
        if (!base_valid || !quote_valid) {
          continue;
        }
        const double base_close =
            future.index({b, base, c, close_slot}).template item<double>();
        const double quote_close =
            future.index({b, quote_node_index, c, close_slot})
                .template item<double>();
        const double target_edge_close = base_close - quote_close;
        if (!std::isfinite(target_edge_close)) {
          continue;
        }
        const int64_t feature_count = De * 3;
        out << "kikijyeba.synthetic.representation_edge_feature_probe.v1,"
            << anchor_key << "," << anchor_index << "," << b << ","
            << edge_index << "," << edge_id << "," << base_node_id << ","
            << quote_node_id << "," << c << "," << target_edge_close << ","
            << feature_count << ",";
        bool first_feature = true;
        for (int64_t d = 0; d < De; ++d) {
          const double value =
              context.index({b, base, c, d}).template item<double>();
          if (!first_feature) {
            out << ";";
          }
          out << value;
          first_feature = false;
        }
        for (int64_t d = 0; d < De; ++d) {
          const double value = context.index({b, quote_node_index, c, d})
                                   .template item<double>();
          out << ";" << value;
        }
        for (int64_t d = 0; d < De; ++d) {
          const double value =
              context.index({b, base, c, d}).template item<double>() -
              context.index({b, quote_node_index, c, d})
                  .template item<double>();
          out << ";" << value;
        }
        out << "\n";
        ++row_count;
      }
    }
  }
  return row_count;
}

template <typename KeyT>
[[nodiscard]] inline int64_t append_mdn_edge_context_feature_probe_rows(
    const std::filesystem::path &path,
    const torch::Tensor &edge_context_features,
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &representation_batch,
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        channel_mdn_input_batch_t<KeyT> &mdn_batch,
    int64_t close_feature_index = 3) {
  if (path.empty() || mdn_batch.node_ids.size() < 2) {
    return 0;
  }
  const auto close_it =
      std::find(mdn_batch.target_coords.begin(), mdn_batch.target_coords.end(),
                close_feature_index);
  if (close_it == mdn_batch.target_coords.end()) {
    return 0;
  }
  const int64_t close_slot = static_cast<int64_t>(
      std::distance(mdn_batch.target_coords.begin(), close_it));
  TORCH_CHECK(edge_context_features.defined() &&
                  edge_context_features.dim() == 4,
              "[channel_graph_first_inference_launcher] MDN edge context "
              "feature probe requires [B,N-1,C,F]");
  TORCH_CHECK(mdn_batch.future.defined() && mdn_batch.future.dim() == 4,
              "[channel_graph_first_inference_launcher] MDN edge context "
              "feature probe requires future [B,N,C,Df]");
  TORCH_CHECK(mdn_batch.future_mask.defined() &&
                  mdn_batch.future_mask.dim() == 4,
              "[channel_graph_first_inference_launcher] MDN edge context "
              "feature probe requires future_mask [B,N,C,Df]");

  const auto context = edge_context_features.detach()
                           .to(torch::kCPU)
                           .to(torch::kFloat64)
                           .contiguous();
  const auto future = mdn_batch.future.detach()
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  const auto future_mask = mdn_batch.future_mask.detach()
                               .to(torch::kCPU)
                               .to(torch::kBool)
                               .contiguous();
  const auto anchor_keys = mdn_batch.anchor_keys.detach()
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();

  const int64_t B = context.size(0);
  const int64_t base_count = context.size(1);
  const int64_t C = context.size(2);
  const int64_t F = context.size(3);
  TORCH_CHECK(future.size(0) == B && future.size(1) == base_count + 1 &&
                  future.size(2) == C,
              "[channel_graph_first_inference_launcher] future shape does not "
              "match MDN edge context feature probe");
  TORCH_CHECK(future_mask.sizes() == future.sizes(),
              "[channel_graph_first_inference_launcher] future_mask shape does "
              "not match future for MDN edge context feature probe");
  TORCH_CHECK(anchor_keys.defined() && anchor_keys.dim() == 1 &&
                  anchor_keys.size(0) == B,
              "[channel_graph_first_inference_launcher] anchor_keys shape does "
              "not match MDN edge context feature probe");
  if (close_slot < 0 || close_slot >= future.size(3)) {
    return 0;
  }

  ensure_parent_directory(path, "MDN edge context feature probe");
  const bool write_header =
      !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;
  std::ofstream out(path, std::ios::app);
  if (!out) {
    throw std::runtime_error(
        "[channel_graph_first_inference_launcher] failed to open MDN edge "
        "context feature probe path");
  }
  if (write_header) {
    out << "record_schema,anchor_key,anchor_index,anchor_local_index,edge_"
           "index,edge_id,"
           "base_node_id,quote_node_id,channel_index,"
           "target_edge_close_return,feature_count,feature_values\n";
  }

  constexpr int64_t quote_node_index = 0;
  int64_t row_count = 0;
  out << std::setprecision(17);
  for (int64_t b = 0; b < B; ++b) {
    const int64_t anchor_key =
        anchor_keys.select(0, b).template item<int64_t>();
    const int64_t anchor_index =
        static_cast<int64_t>(representation_batch.cursor.begin_anchor_index) +
        b;
    for (int64_t edge_index = 0; edge_index < base_count; ++edge_index) {
      const int64_t base = edge_index + 1;
      const std::string edge_id =
          edge_index < static_cast<int64_t>(mdn_batch.edge_ids.size())
              ? mdn_batch.edge_ids[static_cast<std::size_t>(edge_index)]
              : std::string{};
      const auto &base_node_id =
          mdn_batch.node_ids[static_cast<std::size_t>(base)];
      const auto &quote_node_id =
          mdn_batch.node_ids[static_cast<std::size_t>(quote_node_index)];
      for (int64_t c = 0; c < C; ++c) {
        const bool base_valid =
            future_mask.index({b, base, c, close_slot}).template item<bool>();
        const bool quote_valid =
            future_mask.index({b, quote_node_index, c, close_slot})
                .template item<bool>();
        if (!base_valid || !quote_valid) {
          continue;
        }
        const double base_close =
            future.index({b, base, c, close_slot}).template item<double>();
        const double quote_close =
            future.index({b, quote_node_index, c, close_slot})
                .template item<double>();
        const double target_edge_close = base_close - quote_close;
        if (!std::isfinite(target_edge_close)) {
          continue;
        }
        out << "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1,"
            << anchor_key << "," << anchor_index << "," << b << ","
            << edge_index << "," << edge_id << "," << base_node_id << ","
            << quote_node_id << "," << c << "," << target_edge_close << "," << F
            << ",";
        for (int64_t d = 0; d < F; ++d) {
          if (d > 0) {
            out << ";";
          }
          out << context.index({b, edge_index, c, d}).template item<double>();
        }
        out << "\n";
        ++row_count;
      }
    }
  }
  return row_count;
}

[[nodiscard]] inline std::filesystem::path
runtime_lls_wave_batch_path(const std::filesystem::path &report_path,
                            const std::string &component_suffix) {
  return std::filesystem::path(report_path.string() + "." + component_suffix +
                               ".lls");
}

inline void write_component_runtime_lls_sidecars(
    const std::filesystem::path &report_path,
    const std::string &nodelift_runtime_lls,
    const std::string &representation_runtime_lls,
    const std::string &mdn_runtime_lls) {
  write_text_file(runtime_lls_wave_batch_path(report_path, "nodelift"),
                  nodelift_runtime_lls, "nodelift runtime lls");
  write_text_file(runtime_lls_wave_batch_path(report_path, "representation"),
                  representation_runtime_lls,
                  "channel representation runtime lls");
  write_text_file(runtime_lls_wave_batch_path(report_path, "mdn"),
                  mdn_runtime_lls, "channel MDN runtime lls");
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

inline void append_finite_double(
    cuwacunu::hero::lattice::runtime_report::runtime_lls_document_t &document,
    std::string key, double value, std::string domain = "(-inf,+inf)") {
  if (std::isfinite(value)) {
    document.entries.push_back(
        cuwacunu::hero::lattice::runtime_report::
            make_component_runtime_lls_double_entry(std::move(key), value,
                                                    std::move(domain)));
  }
}

template <typename KeyT>
inline std::string make_channel_mdn_runtime_lls(
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        channel_mdn_input_batch_t<KeyT> &batch,
    const cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_step_result_t &step,
    const std::string &component_assembly_id, const std::string &assembly_token,
    const std::string &dock_binding_token,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode,
    int64_t optimizer_steps, int64_t wave_pulse_index) {
  namespace lls = cuwacunu::hero::lattice::runtime_report;
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family_id = "wikimyei.inference.expected_value.mdn",
              .component_assembly_id = component_assembly_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = batch.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, batch.cursor),
          runtime_report_mode, std::numeric_limits<double>::quiet_NaN(),
          static_cast<std::uint64_t>(batch.context.numel()),
          static_cast<std::uint64_t>(step.valid_target_count),
          step.skipped ? 1 : 0);
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.inference.expected_value.mdn.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, batch.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "optimizer_steps", static_cast<std::uint64_t>(optimizer_steps)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "wave_pulse_index", static_cast<std::uint64_t>(wave_pulse_index)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "context_anchor_count",
      static_cast<std::uint64_t>(batch.context.size(0))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(batch.context.size(1))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "context_slot_count", static_cast<std::uint64_t>(batch.context.size(0) *
                                                       batch.context.size(1))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "channel_count", static_cast<std::uint64_t>(batch.context.size(2))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "context_dim", static_cast<std::uint64_t>(batch.context.size(3))));
  document.entries.push_back(
      lls::make_component_runtime_lls_uint_entry("future_horizon", 1));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "target_coord_count",
      static_cast<std::uint64_t>(batch.target_coords.size())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_target_count",
      static_cast<std::uint64_t>(step.valid_target_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "nonfinite_output_count",
      static_cast<std::uint64_t>(step.nonfinite_output_count)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("trained", !step.skipped));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("skipped", step.skipped));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "optimizer_step_applied", step.optimizer_step_applied));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "loss_preference", "lower_is_better"));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "loss_reduction", "balanced_channel_feature_mean"));
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "mdn_architecture", "shared_slot_trunk.channel_adapter.shared_feature_"
                          "head.direct_edge_readout.v4"));
  if (step.loss.defined() && step.loss.numel() > 0) {
    append_finite_double(
        document, "loss",
        step.loss.detach().to(torch::kCPU).to(torch::kFloat64).item<double>());
  }
  append_finite_double(document, "valid_target_fraction",
                       step.valid_target_fraction, "[0,1]");
  append_finite_double(document, "context_mask_fraction",
                       bool_fraction_or_nan(batch.context_mask), "[0,1]");
  append_finite_double(document, "future_mask_fraction",
                       bool_fraction_or_nan(batch.future_mask), "[0,1]");
  append_finite_double(document, "sigma_mean", step.sigma_mean, "[0,+inf)");
  append_finite_double(document, "sigma_min", step.sigma_min, "[0,+inf)");
  append_finite_double(document, "sigma_max", step.sigma_max, "[0,+inf)");
  append_finite_double(document, "sigma_mean_valid", step.sigma_mean_valid,
                       "[0,+inf)");
  append_finite_double(document, "sigma_min_valid", step.sigma_min_valid,
                       "[0,+inf)");
  append_finite_double(document, "sigma_max_valid", step.sigma_max_valid,
                       "[0,+inf)");
  append_finite_double(document, "mixture_entropy", step.mixture_entropy,
                       "[0,+inf)");
  append_finite_double(document, "grad_norm", step.grad_norm, "[0,+inf)");
  if (step.direct_edge_return_readout_loss.defined() &&
      step.direct_edge_return_readout_loss.numel() > 0) {
    append_finite_double(document, "direct_edge_return_readout_loss",
                         step.direct_edge_return_readout_loss.detach()
                             .to(torch::kCPU)
                             .to(torch::kFloat64)
                             .item<double>());
  }
  if (step.direct_edge_return_readout_regression_loss.defined() &&
      step.direct_edge_return_readout_regression_loss.numel() > 0) {
    append_finite_double(
        document, "direct_edge_return_readout_regression_loss",
        step.direct_edge_return_readout_regression_loss.detach()
            .to(torch::kCPU)
            .to(torch::kFloat64)
            .item<double>());
  }
  if (step.direct_edge_return_readout_direction_loss.defined() &&
      step.direct_edge_return_readout_direction_loss.numel() > 0) {
    append_finite_double(document, "direct_edge_return_readout_direction_loss",
                         step.direct_edge_return_readout_direction_loss.detach()
                             .to(torch::kCPU)
                             .to(torch::kFloat64)
                             .item<double>());
  }
  if (step.direct_edge_return_readout_rank_loss.defined() &&
      step.direct_edge_return_readout_rank_loss.numel() > 0) {
    append_finite_double(document, "direct_edge_return_readout_rank_loss",
                         step.direct_edge_return_readout_rank_loss.detach()
                             .to(torch::kCPU)
                             .to(torch::kFloat64)
                             .item<double>());
  }
  if (step.direct_edge_return_readout_valid_count > 0) {
    append_finite_double(
        document, "direct_edge_return_readout_directional_accuracy",
        static_cast<double>(
            step.direct_edge_return_readout_direction_correct_count) /
            static_cast<double>(step.direct_edge_return_readout_valid_count),
        "[0,1]");
  }
  if (step.direct_edge_return_readout_pairwise_rank_valid_count > 0) {
    append_finite_double(
        document, "direct_edge_return_readout_pairwise_rank_accuracy",
        static_cast<double>(
            step.direct_edge_return_readout_pairwise_rank_correct_count) /
            static_cast<double>(
                step.direct_edge_return_readout_pairwise_rank_valid_count),
        "[0,1]");
  }
  return lls::emit_component_runtime_lls_canonical(document);
}

inline void move_channel_mdn_input_to_device(
    cuwacunu::wikimyei::inference::expected_value::mdn::channel_mdn_input_t
        &input,
    torch::Dtype dtype, const torch::Device &device) {
  input.context =
      input.context.to(torch::TensorOptions().dtype(dtype).device(device));
  input.context_mask = input.context_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(device));
  input.future =
      input.future.to(torch::TensorOptions().dtype(dtype).device(device));
  input.future_mask = input.future_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(device));
}

struct mtf_representation_encoder_options_view_t {
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

class mtf_representation_encoder_adapter_t {
public:
  explicit mtf_representation_encoder_adapter_t(
      cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg::
          MtfJepaMaeVicreg &model)
      : model_(&model) {
    TORCH_CHECK(model_ != nullptr,
                "[channel_graph_first_inference_launcher] MTF model adapter "
                "requires a model");
  }

  [[nodiscard]] mtf_representation_encoder_options_view_t options() const {
    return {.dtype = (*model_)->config().dtype,
            .device = (*model_)->config().device};
  }

  [[nodiscard]] cuwacunu::wikimyei::representation::encoding::vicreg::
      channel_preserving_encoder_output_t
      encode(const torch::Tensor &data, const torch::Tensor &mask,
             bool detach_to_cpu) {
    namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
    torch::NoGradGuard no_grad;
    const bool was_training = (*model_)->is_training();
    (*model_)->eval();
    auto encoded = (*model_)->encode(data, mask);
    (*model_)->train(was_training);

    vicreg::channel_preserving_encoder_output_t out{};
    out.reduced = encoded.pooled_by_channel;
    out.reduced_mask = encoded.channel_valid_mask;
    out.sequence = encoded.pooled_by_channel.unsqueeze(/*dim=*/2);
    out.sequence_mask = encoded.channel_valid_mask.unsqueeze(/*dim=*/2);
    if (detach_to_cpu) {
      out.sequence = out.sequence.detach().to(torch::kCPU);
      out.sequence_mask = out.sequence_mask.detach().to(torch::kCPU);
      out.reduced = out.reduced.detach().to(torch::kCPU);
      out.reduced_mask = out.reduced_mask.detach().to(torch::kCPU);
    }
    return out;
  }

private:
  cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg::
      MtfJepaMaeVicreg *model_{nullptr};
};

template <typename KeyT> class channel_representation_stream_iface_t {
public:
  using batch_t = cuwacunu::wikimyei::representation::encoding::vicreg::stream::
      channel_representation_batch_t<KeyT>;

  virtual ~channel_representation_stream_iface_t() = default;
  [[nodiscard]] virtual bool has_next() const = 0;
  virtual void reset() = 0;
  virtual batch_t next() = 0;
};

template <typename KeyT, typename StreamT>
class channel_representation_stream_box_t final
    : public channel_representation_stream_iface_t<KeyT> {
public:
  using batch_t = typename channel_representation_stream_iface_t<KeyT>::batch_t;

  explicit channel_representation_stream_box_t(StreamT stream)
      : stream_(std::move(stream)) {}

  [[nodiscard]] bool has_next() const override { return stream_.has_next(); }

  void reset() override { stream_.reset(); }

  batch_t next() override { return stream_.next(); }

private:
  StreamT stream_;
};

inline void load_mtf_jepa_mae_vicreg_checkpoint_file(
    const std::filesystem::path &path,
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg::
        MtfJepaMaeVicreg &model,
    const torch::Device &device, int64_t expected_channel_count,
    int64_t expected_history_length, int64_t expected_input_width,
    int64_t expected_d_model, int64_t expected_latent_dim,
    int64_t expected_projector_dim) {
  TORCH_CHECK(!path.empty(),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint path is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint does not exist: ",
              path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);

  torch::Tensor saved_channel_count{};
  torch::Tensor saved_history_length{};
  torch::Tensor saved_input_width{};
  torch::Tensor saved_d_model{};
  torch::Tensor saved_latent_dim{};
  torch::Tensor saved_projector_dim{};
  root.read("meta/channel_count", saved_channel_count);
  root.read("meta/history_length", saved_history_length);
  root.read("meta/input_width", saved_input_width);
  root.read("meta/d_model", saved_d_model);
  root.read("meta/latent_dim", saved_latent_dim);
  root.read("meta/projector_dim", saved_projector_dim);

  TORCH_CHECK(metadata_i64_matches(saved_channel_count, expected_channel_count),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint channel_count does not match current config");
  TORCH_CHECK(
      metadata_i64_matches(saved_history_length, expected_history_length),
      "[channel_graph_first_inference_launcher] MTF representation checkpoint "
      "history_length does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_input_width, expected_input_width),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint input_width does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_d_model, expected_d_model),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint d_model does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_latent_dim, expected_latent_dim),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint latent_dim does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_projector_dim, expected_projector_dim),
              "[channel_graph_first_inference_launcher] MTF representation "
              "checkpoint projector_dim does not match current config");

  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);
}

template <typename EncoderT>
inline void load_vicreg_encoder_checkpoint_file(
    const std::filesystem::path &path, EncoderT &encoder,
    const torch::Device &device,
    const std::string &expected_component_assembly_id,
    const std::string &expected_representation_contract,
    const std::string &expected_channel_axis_policy,
    const std::string &expected_graph_order_fingerprint,
    const std::vector<std::string> &expected_node_ids,
    const std::vector<int64_t> &expected_required_feature_coords,
    const std::string &expected_cell_valid_policy,
    int64_t expected_channel_count, int64_t expected_history_length,
    int64_t expected_input_width, int64_t expected_encoding_dim,
    int64_t expected_feature_hidden_dim, int64_t expected_temporal_depth,
    double expected_recency_decay, double expected_min_valid_fraction,
    bool expected_use_missingness_indicators,
    double expected_vicreg_invariance_weight,
    double expected_vicreg_variance_weight,
    double expected_vicreg_covariance_weight,
    double expected_vicreg_variance_floor, double expected_vicreg_eps,
    int64_t expected_min_valid_rows, bool expected_skip_non_finite_loss) {
  TORCH_CHECK(!path.empty(),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint path is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint does not exist: ",
              path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);

  torch::Tensor saved_channel_count{};
  torch::Tensor saved_history_length{};
  torch::Tensor saved_input_width{};
  torch::Tensor saved_encoding_dim{};
  torch::Tensor saved_feature_hidden_dim{};
  torch::Tensor saved_temporal_depth{};
  torch::Tensor saved_recency_decay{};
  torch::Tensor saved_min_valid_fraction{};
  torch::Tensor saved_use_missingness_indicators{};
  torch::Tensor saved_vicreg_invariance_weight{};
  torch::Tensor saved_vicreg_variance_weight{};
  torch::Tensor saved_vicreg_covariance_weight{};
  torch::Tensor saved_vicreg_variance_floor{};
  torch::Tensor saved_vicreg_eps{};
  torch::Tensor saved_min_valid_rows{};
  torch::Tensor saved_skip_non_finite_loss{};
  torch::Tensor saved_component_assembly_id{};
  torch::Tensor saved_representation_contract{};
  torch::Tensor saved_channel_axis_policy{};
  torch::Tensor saved_cell_valid_policy{};
  torch::Tensor saved_required_feature_coords{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/channel_count", saved_channel_count);
  root.read("meta/history_length", saved_history_length);
  root.read("meta/input_width", saved_input_width);
  root.read("meta/encoding_dim", saved_encoding_dim);
  root.read("meta/feature_hidden_dim", saved_feature_hidden_dim);
  root.read("meta/temporal_depth", saved_temporal_depth);
  root.read("meta/recency_decay", saved_recency_decay);
  root.read("meta/min_valid_fraction", saved_min_valid_fraction);
  root.read("meta/use_missingness_indicators",
            saved_use_missingness_indicators);
  root.read("meta/vicreg_invariance_weight", saved_vicreg_invariance_weight);
  root.read("meta/vicreg_variance_weight", saved_vicreg_variance_weight);
  root.read("meta/vicreg_covariance_weight", saved_vicreg_covariance_weight);
  root.read("meta/vicreg_variance_floor", saved_vicreg_variance_floor);
  root.read("meta/vicreg_eps", saved_vicreg_eps);
  root.read("meta/min_valid_rows", saved_min_valid_rows);
  root.read("meta/skip_non_finite_loss", saved_skip_non_finite_loss);
  root.read("meta/component_assembly_id_bytes", saved_component_assembly_id);
  root.read("meta/representation_contract_bytes",
            saved_representation_contract);
  root.read("meta/channel_axis_policy_bytes", saved_channel_axis_policy);
  root.read("meta/cell_valid_policy_bytes", saved_cell_valid_policy);
  root.read("meta/required_feature_coords", saved_required_feature_coords);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  TORCH_CHECK(saved_channel_count.defined() &&
                  saved_channel_count.numel() == 1 &&
                  saved_channel_count.item<int64_t>() == expected_channel_count,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint channel_count does not match current config");
  TORCH_CHECK(
      saved_history_length.defined() && saved_history_length.numel() == 1 &&
          saved_history_length.item<int64_t>() == expected_history_length,
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint history_length does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_input_width, expected_input_width),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint input_width does not match current config");
  TORCH_CHECK(saved_encoding_dim.defined() && saved_encoding_dim.numel() == 1 &&
                  saved_encoding_dim.item<int64_t>() == expected_encoding_dim,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint encoding_dim does not match current config");
  TORCH_CHECK(metadata_i64_matches(saved_feature_hidden_dim,
                                   expected_feature_hidden_dim),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint feature_hidden_dim does not match current config");
  TORCH_CHECK(
      metadata_i64_matches(saved_temporal_depth, expected_temporal_depth),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint temporal_depth does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_recency_decay, expected_recency_decay),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint recency_decay does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_min_valid_fraction,
                                      expected_min_valid_fraction),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint min_valid_fraction does not match current config");
  TORCH_CHECK(metadata_bool_matches(saved_use_missingness_indicators,
                                    expected_use_missingness_indicators),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint use_missingness_indicators does not match current "
              "config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_invariance_weight,
                              expected_vicreg_invariance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg invariance weight does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_variance_weight,
                              expected_vicreg_variance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg variance weight does not match current config");
  TORCH_CHECK(
      metadata_double_matches(saved_vicreg_covariance_weight,
                              expected_vicreg_covariance_weight),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint VICReg covariance weight does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_vicreg_variance_floor,
                                      expected_vicreg_variance_floor),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint VICReg variance floor does not match current config");
  TORCH_CHECK(metadata_double_matches(saved_vicreg_eps, expected_vicreg_eps),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint VICReg eps does not match current config");
  TORCH_CHECK(
      metadata_i64_matches(saved_min_valid_rows, expected_min_valid_rows),
      "[channel_graph_first_inference_launcher] representation "
      "checkpoint min_valid_rows does not match current config");
  TORCH_CHECK(metadata_bool_matches(saved_skip_non_finite_loss,
                                    expected_skip_non_finite_loss),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint skip_non_finite_loss does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_component_assembly_id,
                                      expected_component_assembly_id),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint component_assembly_id does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_representation_contract,
                                      expected_representation_contract),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint contract does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_channel_axis_policy,
                                      expected_channel_axis_policy),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint channel_axis_policy does not match current config");
  TORCH_CHECK(metadata_string_matches(saved_cell_valid_policy,
                                      expected_cell_valid_policy),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint cell_valid_policy does not match current config");
  TORCH_CHECK(tensor_to_int64_vector(saved_required_feature_coords) ==
                  expected_required_feature_coords,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint required_feature_coords do not match current "
              "config");
  TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                  string_to_bytes(expected_graph_order_fingerprint),
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint graph fingerprint does not match current graph");

  auto [expected_node_lengths, expected_node_bytes] =
      string_list_to_lengths_and_bytes(expected_node_ids);
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                  expected_node_lengths,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint node_id lengths do not match current node order");
  TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                  expected_node_bytes,
              "[channel_graph_first_inference_launcher] representation "
              "checkpoint node_ids do not match current node order");

  torch::serialize::InputArchive encoder_archive;
  root.read("encoder", encoder_archive);
  encoder->load(encoder_archive);
}

template <typename EncoderT>
inline void freeze_vicreg_encoder(EncoderT &encoder) {
  encoder->eval();
  for (auto &parameter : encoder->parameters()) {
    parameter.set_requires_grad(false);
  }
}

[[nodiscard]] inline int64_t read_channel_mdn_optimizer_step_index(
    torch::serialize::InputArchive &root) {
  torch::Tensor saved_optimizer_step_index{};
  try {
    root.read("meta/optimizer_step_index", saved_optimizer_step_index);
  } catch (const std::exception &) {
    // Checkpoints written before the dedicated schedule counter used the
    // completed optimizer-step total as the only persisted equivalent.
    root.read("meta/optimizer_steps", saved_optimizer_step_index);
  }
  TORCH_CHECK(saved_optimizer_step_index.defined() &&
                  saved_optimizer_step_index.numel() == 1,
              "[channel_graph_first_inference_launcher] MDN checkpoint "
              "optimizer step index must be a scalar");
  const auto optimizer_step_index =
      saved_optimizer_step_index.to(torch::kCPU).template item<int64_t>();
  TORCH_CHECK(optimizer_step_index >= 0,
              "[channel_graph_first_inference_launcher] MDN checkpoint "
              "optimizer step index must be nonnegative");
  return optimizer_step_index;
}

inline void save_channel_mdn_checkpoint_file(
    const std::filesystem::path &path,
    const channel_graph_first_inference_training_report_t &report,
    cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_model_t &model) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive model_archive;
  model.model()->save(model_archive);
  root.write("model", model_archive);
  torch::serialize::OutputArchive optimizer_archive;
  model.optimizer().save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/channel_count", torch::tensor({report.channel_count}, i64));
  root.write("meta/context_dim", torch::tensor({report.context_dim}, i64));
  root.write("meta/future_horizon",
             torch::tensor({report.future_horizon}, i64));
  root.write("meta/mixture_count", torch::tensor({report.mixture_count}, i64));
  root.write("meta/hidden_width", torch::tensor({report.hidden_width}, i64));
  root.write("meta/residual_depth",
             torch::tensor({report.residual_depth}, i64));
  root.write("meta/feature_embedding_dim",
             torch::tensor({report.feature_embedding_dim}, i64));
  root.write("meta/channel_adapter_rank",
             torch::tensor({report.channel_adapter_rank}, i64));
  root.write(
      "meta/direct_edge_return_readout_base_edge_count",
      torch::tensor({report.direct_edge_return_readout_base_edge_count}, i64));
  root.write(
      "meta/direct_edge_return_readout_identity_embedding_dim",
      torch::tensor({report.direct_edge_return_readout_identity_embedding_dim},
                    i64));
  root.write("meta/direct_edge_return_readout_adapter_hidden_dim",
             torch::tensor(
                 {report.direct_edge_return_readout_adapter_hidden_dim}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write("meta/optimizer_step_index",
             torch::tensor({model.optimizer_step_index()}, i64));
  root.write(
      "meta/component_assembly_id_bytes",
      int64_tensor_from_vector(string_to_bytes(report.component_assembly_id)));
  root.write("meta/input_representation_assembly_id_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.input_representation_assembly_id)));
  root.write(
      "meta/context_contract_bytes",
      int64_tensor_from_vector(string_to_bytes(report.context_contract)));
  root.write("meta/output_contract_bytes",
             int64_tensor_from_vector(string_to_bytes(report.output_contract)));
  root.write("meta/context_mode_bytes",
             int64_tensor_from_vector(string_to_bytes(report.context_mode)));
  root.write("meta/target_domain_bytes",
             int64_tensor_from_vector(string_to_bytes(report.target_domain)));
  root.write(
      "meta/target_mask_policy_bytes",
      int64_tensor_from_vector(string_to_bytes(report.target_mask_policy)));
  root.write("meta/activity_target_bytes",
             int64_tensor_from_vector(string_to_bytes(report.activity_target)));
  root.write("meta/direct_edge_return_readout_identity_mode_bytes",
             int64_tensor_from_vector(string_to_bytes(
                 report.direct_edge_return_readout_identity_mode)));
  root.write("meta/target_coords",
             int64_tensor_from_vector(report.target_coords));
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  root.write("meta/sigma_min", torch::tensor({report.sigma_min}, f64));
  root.write("meta/sigma_max", torch::tensor({report.sigma_max}, f64));
  root.write("meta/eps", torch::tensor({report.eps}, f64));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

struct channel_mdn_checkpoint_identity_t {
  std::string component_assembly_id{};
  std::string input_representation_assembly_id{};
  std::string context_contract{};
  std::string output_contract{};
  std::string context_mode{};
  std::string target_domain{};
  std::string target_mask_policy{};
  std::string activity_target{};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<int64_t> target_coords{};
  int64_t channel_count{0};
  int64_t context_dim{0};
  int64_t future_horizon{0};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  int64_t feature_embedding_dim{0};
  int64_t channel_adapter_rank{0};
  std::string direct_edge_return_readout_identity_mode{"shared"};
  int64_t direct_edge_return_readout_base_edge_count{0};
  int64_t direct_edge_return_readout_identity_embedding_dim{0};
  int64_t direct_edge_return_readout_adapter_hidden_dim{0};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double eps{std::numeric_limits<double>::quiet_NaN()};
};

[[nodiscard]] inline channel_mdn_checkpoint_identity_t
checkpoint_identity_from_report(
    const channel_graph_first_inference_training_report_t &report) {
  channel_mdn_checkpoint_identity_t out{};
  out.component_assembly_id = report.component_assembly_id;
  out.input_representation_assembly_id =
      report.input_representation_assembly_id;
  out.context_contract = report.context_contract;
  out.output_contract = report.output_contract;
  out.context_mode = report.context_mode;
  out.target_domain = report.target_domain;
  out.target_mask_policy = report.target_mask_policy;
  out.activity_target = report.activity_target;
  out.graph_order_fingerprint = report.graph_order_fingerprint;
  out.node_ids = report.node_ids;
  out.target_coords = report.target_coords;
  out.channel_count = report.channel_count;
  out.context_dim = report.context_dim;
  out.future_horizon = report.future_horizon;
  out.mixture_count = report.mixture_count;
  out.hidden_width = report.hidden_width;
  out.residual_depth = report.residual_depth;
  out.feature_embedding_dim = report.feature_embedding_dim;
  out.channel_adapter_rank = report.channel_adapter_rank;
  out.direct_edge_return_readout_identity_mode =
      report.direct_edge_return_readout_identity_mode;
  out.direct_edge_return_readout_base_edge_count =
      report.direct_edge_return_readout_base_edge_count;
  out.direct_edge_return_readout_identity_embedding_dim =
      report.direct_edge_return_readout_identity_embedding_dim;
  out.direct_edge_return_readout_adapter_hidden_dim =
      report.direct_edge_return_readout_adapter_hidden_dim;
  out.sigma_min = report.sigma_min;
  out.sigma_max = report.sigma_max;
  out.eps = report.eps;
  return out;
}

inline void load_channel_mdn_checkpoint_file(
    const std::filesystem::path &path,
    cuwacunu::wikimyei::inference::expected_value::mdn::ChannelContextMdn
        &model,
    torch::optim::Optimizer *optimizer = nullptr,
    const channel_mdn_checkpoint_identity_t *expected_identity = nullptr,
    int64_t *optimizer_step_index = nullptr) {
  TORCH_CHECK(!path.empty(),
              "[channel_graph_first_inference_launcher] MDN checkpoint path "
              "is empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[channel_graph_first_inference_launcher] MDN checkpoint does "
              "not exist: ",
              path.string());

  torch::serialize::InputArchive root;
  root.load_from(path.string(), model->device);

  torch::Tensor saved_channel_count{};
  torch::Tensor saved_context_dim{};
  torch::Tensor saved_future_horizon{};
  torch::Tensor saved_mixture_count{};
  torch::Tensor saved_hidden_width{};
  torch::Tensor saved_residual_depth{};
  torch::Tensor saved_feature_embedding_dim{};
  torch::Tensor saved_channel_adapter_rank{};
  torch::Tensor saved_direct_readout_base_edge_count{};
  torch::Tensor saved_direct_readout_identity_embedding_dim{};
  torch::Tensor saved_direct_readout_adapter_hidden_dim{};
  torch::Tensor saved_component_assembly_id{};
  torch::Tensor saved_input_representation_assembly_id{};
  torch::Tensor saved_context_contract{};
  torch::Tensor saved_output_contract{};
  torch::Tensor saved_context_mode{};
  torch::Tensor saved_target_domain{};
  torch::Tensor saved_target_mask_policy{};
  torch::Tensor saved_activity_target{};
  torch::Tensor saved_direct_readout_identity_mode{};
  torch::Tensor saved_target_coords{};
  torch::Tensor saved_sigma_min{};
  torch::Tensor saved_sigma_max{};
  torch::Tensor saved_eps{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/channel_count", saved_channel_count);
  root.read("meta/context_dim", saved_context_dim);
  root.read("meta/future_horizon", saved_future_horizon);
  root.read("meta/mixture_count", saved_mixture_count);
  root.read("meta/hidden_width", saved_hidden_width);
  root.read("meta/residual_depth", saved_residual_depth);
  root.read("meta/feature_embedding_dim", saved_feature_embedding_dim);
  root.read("meta/channel_adapter_rank", saved_channel_adapter_rank);
  root.read("meta/direct_edge_return_readout_base_edge_count",
            saved_direct_readout_base_edge_count);
  root.read("meta/direct_edge_return_readout_identity_embedding_dim",
            saved_direct_readout_identity_embedding_dim);
  root.read("meta/direct_edge_return_readout_adapter_hidden_dim",
            saved_direct_readout_adapter_hidden_dim);
  root.read("meta/component_assembly_id_bytes", saved_component_assembly_id);
  root.read("meta/input_representation_assembly_id_bytes",
            saved_input_representation_assembly_id);
  root.read("meta/context_contract_bytes", saved_context_contract);
  root.read("meta/output_contract_bytes", saved_output_contract);
  root.read("meta/context_mode_bytes", saved_context_mode);
  root.read("meta/target_domain_bytes", saved_target_domain);
  root.read("meta/target_mask_policy_bytes", saved_target_mask_policy);
  root.read("meta/activity_target_bytes", saved_activity_target);
  root.read("meta/direct_edge_return_readout_identity_mode_bytes",
            saved_direct_readout_identity_mode);
  root.read("meta/target_coords", saved_target_coords);
  root.read("meta/sigma_min", saved_sigma_min);
  root.read("meta/sigma_max", saved_sigma_max);
  root.read("meta/eps", saved_eps);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  if (expected_identity != nullptr) {
    TORCH_CHECK(saved_channel_count.defined() &&
                    saved_channel_count.numel() == 1 &&
                    saved_channel_count.item<int64_t>() ==
                        expected_identity->channel_count,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "channel_count does not match current config");
    TORCH_CHECK(saved_context_dim.defined() && saved_context_dim.numel() == 1 &&
                    saved_context_dim.item<int64_t>() ==
                        expected_identity->context_dim,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_dim does not match current representation");
    TORCH_CHECK(saved_future_horizon.defined() &&
                    saved_future_horizon.numel() == 1 &&
                    saved_future_horizon.item<int64_t>() ==
                        expected_identity->future_horizon,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "future_horizon does not match current config");
    TORCH_CHECK(saved_mixture_count.defined() &&
                    saved_mixture_count.numel() == 1 &&
                    saved_mixture_count.item<int64_t>() ==
                        expected_identity->mixture_count,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "mixture_count does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_hidden_width,
                                     expected_identity->hidden_width),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "hidden_width does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_residual_depth,
                                     expected_identity->residual_depth),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "residual_depth does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_feature_embedding_dim,
                                     expected_identity->feature_embedding_dim),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "feature_embedding_dim does not match current config");
    TORCH_CHECK(metadata_i64_matches(saved_channel_adapter_rank,
                                     expected_identity->channel_adapter_rank),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "channel_adapter_rank does not match current config");
    TORCH_CHECK(
        metadata_i64_matches(
            saved_direct_readout_base_edge_count,
            expected_identity->direct_edge_return_readout_base_edge_count),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "direct_edge_return_readout_base_edge_count does not match "
        "current config");
    TORCH_CHECK(
        metadata_i64_matches(
            saved_direct_readout_identity_embedding_dim,
            expected_identity
                ->direct_edge_return_readout_identity_embedding_dim),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "direct_edge_return_readout_identity_embedding_dim does not match "
        "current config");
    TORCH_CHECK(
        metadata_i64_matches(
            saved_direct_readout_adapter_hidden_dim,
            expected_identity->direct_edge_return_readout_adapter_hidden_dim),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "direct_edge_return_readout_adapter_hidden_dim does not match "
        "current config");
    TORCH_CHECK(
        metadata_string_matches(saved_component_assembly_id,
                                expected_identity->component_assembly_id),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "component_assembly_id does not match current config");
    TORCH_CHECK(
        metadata_string_matches(
            saved_input_representation_assembly_id,
            expected_identity->input_representation_assembly_id),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "input_representation_assembly_id does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_context_contract,
                                        expected_identity->context_contract),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_contract does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_output_contract,
                                        expected_identity->output_contract),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "output_contract does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_context_mode,
                                        expected_identity->context_mode),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "context_mode does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_target_domain,
                                        expected_identity->target_domain),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_domain does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_target_mask_policy,
                                        expected_identity->target_mask_policy),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_mask_policy does not match current config");
    TORCH_CHECK(metadata_string_matches(saved_activity_target,
                                        expected_identity->activity_target),
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "activity_target does not match current config");
    TORCH_CHECK(
        metadata_string_matches(
            saved_direct_readout_identity_mode,
            expected_identity->direct_edge_return_readout_identity_mode),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "direct_edge_return_readout_identity_mode does not match current "
        "config");
    TORCH_CHECK(tensor_to_int64_vector(saved_target_coords) ==
                    expected_identity->target_coords,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "target_coords do not match current target policy");
    TORCH_CHECK(
        metadata_double_matches(saved_sigma_min, expected_identity->sigma_min),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "sigma_min does not match current config");
    TORCH_CHECK(
        metadata_double_matches(saved_sigma_max, expected_identity->sigma_max),
        "[channel_graph_first_inference_launcher] MDN checkpoint "
        "sigma_max does not match current config");
    TORCH_CHECK(metadata_double_matches(saved_eps, expected_identity->eps),
                "[channel_graph_first_inference_launcher] MDN checkpoint eps "
                "does not match current config");
    TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                    string_to_bytes(expected_identity->graph_order_fingerprint),
                "[channel_graph_first_inference_launcher] MDN checkpoint graph "
                "fingerprint does not match current graph");

    auto [expected_node_lengths, expected_node_bytes] =
        string_list_to_lengths_and_bytes(expected_identity->node_ids);
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                    expected_node_lengths,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "node_id lengths do not match current node order");
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                    expected_node_bytes,
                "[channel_graph_first_inference_launcher] MDN checkpoint "
                "node_ids do not match current node order");
  }

  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);

  if (optimizer != nullptr) {
    torch::serialize::InputArchive optimizer_archive;
    root.read("optimizer", optimizer_archive);
    optimizer->load(optimizer_archive);
  }
  if (optimizer_step_index != nullptr) {
    *optimizer_step_index = read_channel_mdn_optimizer_step_index(root);
  }
}

} // namespace channel_graph_first_inference_launcher_detail

template <typename DatatypeT> class channel_graph_first_inference_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::channel_graph_first_pipeline_builder_t<
          DatatypeT>;
  using key_t = typename builder_t::key_t;
  using mdn_out_t = cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut;
  using channel_mdn_input_batch_t = cuwacunu::wikimyei::inference::
      expected_value::mdn::stream::channel_mdn_input_batch_t<key_t>;
  using channel_representation_batch_t = cuwacunu::wikimyei::representation::
      encoding::vicreg::stream::channel_representation_batch_t<key_t>;
  using inference_batch_observer_t =
      std::function<void(const mdn_out_t &, const channel_mdn_input_batch_t &,
                         const channel_representation_batch_t &, int64_t)>;

  channel_graph_first_inference_launcher_t(
      builder_t builder,
      channel_graph_first_inference_launcher_options_t options = {},
      inference_batch_observer_t inference_batch_observer = {})
      : builder_(std::move(builder)), options_(std::move(options)),
        inference_batch_observer_(std::move(inference_batch_observer)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] channel_graph_first_inference_training_report_t
  dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    channel_graph_first_inference_training_report_t out{};
    out.training_id = builder_.bundle().channel_mdn_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.component_assembly_id =
        builder_.bundle().channel_mdn.component_assembly_id;
    out.input_representation_assembly_id =
        cuwacunu::kikijyeba::protocol::active_protocol_uses_mtf_jepa_mae_vicreg(
            builder_.bundle())
            ? cuwacunu::kikijyeba::protocol::
                  active_representation_component_assembly_id(builder_.bundle())
            : builder_.bundle().channel_mdn.input_representation_assembly_id;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.target_coords = builder_.bundle().channel_mdn.target_coords;
    if (!out.node_ids.empty()) {
      out.edge_return_projection_quote_node_id = out.node_ids.front();
      out.direct_edge_return_readout_quote_node_id = out.node_ids.front();
      std::ostringstream base_nodes;
      for (std::size_t i = 1; i < out.node_ids.size(); ++i) {
        if (i != 1) {
          base_nodes << ",";
        }
        base_nodes << out.node_ids[i];
      }
      out.edge_return_projection_base_node_ids = base_nodes.str();
      out.direct_edge_return_readout_base_node_ids = base_nodes.str();
    }
    out.channel_count = builder_.bundle().channel_mdn.channel_count;
    out.context_dim = plan.context_dim;
    out.future_horizon = builder_.bundle().channel_mdn.future_horizon;
    out.mixture_count = builder_.bundle().channel_mdn.mixture_count;
    out.hidden_width = builder_.bundle().channel_mdn.hidden_width;
    out.residual_depth = builder_.bundle().channel_mdn.residual_depth;
    out.feature_embedding_dim =
        builder_.bundle().channel_mdn.feature_embedding_dim;
    out.channel_adapter_rank =
        builder_.bundle().channel_mdn.channel_adapter_rank;
    out.sigma_min = builder_.bundle().channel_mdn.sigma_min;
    out.sigma_max = builder_.bundle().channel_mdn.sigma_max;
    out.eps = builder_.bundle().channel_mdn.eps;
    out.edge_return_auxiliary_loss_weight =
        builder_.bundle()
            .channel_mdn_training.mdn_edge_return_auxiliary_loss_weight;
    out.edge_return_auxiliary_direction_weight =
        builder_.bundle()
            .channel_mdn_training.mdn_edge_return_auxiliary_direction_weight;
    out.edge_return_auxiliary_rank_weight =
        builder_.bundle()
            .channel_mdn_training.mdn_edge_return_auxiliary_rank_weight;
    out.edge_return_auxiliary_huber_beta =
        builder_.bundle()
            .channel_mdn_training.mdn_edge_return_auxiliary_huber_beta;
    out.edge_return_auxiliary_logit_scale =
        builder_.bundle()
            .channel_mdn_training.mdn_edge_return_auxiliary_logit_scale;
    out.direct_edge_return_readout_enabled =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_enabled;
    out.direct_edge_return_readout_loss_weight =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_loss_weight;
    out.direct_edge_return_readout_direction_weight =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_direction_weight;
    out.direct_edge_return_readout_rank_weight =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_rank_weight;
    out.direct_edge_return_readout_huber_beta =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_huber_beta;
    out.direct_edge_return_readout_logit_scale =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_logit_scale;
    out.direct_edge_return_readout_target_scale =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_target_scale;
    out.direct_edge_return_readout_warmup_steps =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_warmup_steps;
    out.direct_edge_return_readout_warmup_nll_weight =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_warmup_nll_weight;
    out.direct_edge_return_readout_post_warmup_nll_weight =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_post_warmup_nll_weight;
    out.direct_edge_return_readout_warmup_direct_head_only =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_warmup_direct_head_only;
    out.direct_edge_return_readout_identity_mode =
        builder_.bundle()
            .channel_mdn_training.mdn_direct_edge_return_readout_identity_mode;
    out.direct_edge_return_readout_base_edge_count =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_base_edge_count;
    out.direct_edge_return_readout_identity_embedding_dim =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_identity_embedding_dim;
    out.direct_edge_return_readout_adapter_hidden_dim =
        builder_.bundle()
            .channel_mdn_training
            .mdn_direct_edge_return_readout_adapter_hidden_dim;
    out.mdn_architecture =
        "shared_slot_trunk.channel_adapter.shared_feature_head."
        "direct_edge_readout." +
        out.direct_edge_return_readout_identity_mode + ".v4";
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = builder_.bundle().channel_mdn_training.seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.allow_untrained_representation =
        builder_.bundle().channel_mdn_training.allow_untrained_representation;
    out.representation_checkpoint_path =
        builder_.bundle()
            .channel_mdn_training.input_representation_checkpoint_path;
    out.representation_checkpoint_loaded = false;
    out.mdn_checkpoint_path =
        builder_.bundle().channel_mdn_training.input_mdn_checkpoint_path;
    out.mdn_checkpoint_loaded = false;
    out.checkpoint_every =
        builder_.bundle().channel_mdn_training.checkpoint_every;
    out.report_every = builder_.bundle().channel_mdn_training.report_every;
    out.validation_every =
        builder_.bundle().channel_mdn_training.validation_every;
    out.wave_id = plan.wave_id;
    out.wave_mode = plan.wave_mode;
    out.target_action = effective_train_target() ? "train" : "run";
    out.wave_source_range_policy = plan.wave_source_range_policy;
    out.wave_source_order_policy = plan.wave_source_order_policy;
    out.requested_anchor_index_begin = plan.requested_anchor_index_begin;
    out.requested_anchor_index_end = plan.requested_anchor_index_end;
    out.requested_source_key_begin = plan.requested_source_key_begin;
    out.requested_source_key_end = plan.requested_source_key_end;
    out.runtime_report_mode =
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(builder_.bundle().wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] channel_graph_first_inference_training_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const auto &bundle = builder_.bundle();
    const bool use_mtf_representation =
        cuwacunu::kikijyeba::protocol::active_protocol_uses_mtf_jepa_mae_vicreg(
            bundle);
    const bool train_target = effective_train_target();
    if (options_.conditioned_affine_shadow.has_value() && train_target) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] conditioned affine "
          "shadow evaluation is forbidden when train_target=true");
    }
    if (options_.conditioned_affine_shadow.has_value() &&
        options_.force_empty_targets_for_test) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] conditioned affine "
          "shadow evaluation does not allow forced-empty targets");
    }

    const auto &training_spec = bundle.channel_mdn_training;
    const auto checkpoint_every = training_spec.checkpoint_every;
    const auto report_every = training_spec.report_every;
    if ((options_.write_report || (train_target && checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error("[channel_graph_first_inference_launcher] "
                               "report_path is required for "
                               "configured report or checkpoint output");
    }

    const auto seed_scope =
        channel_graph_first_inference_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(training_spec.seed),
            builder_.options().device);
    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            bundle.wave_settings, options_.runtime_report_mode);
    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    bool representation_checkpoint_loaded = false;
    using key_t = typename DatatypeT::key_type_t;
    std::unique_ptr<channel_graph_first_inference_launcher_detail::
                        channel_representation_stream_iface_t<key_t>>
        representation_stream;

    using vicreg_encoder_t = decltype(builder_.make_vicreg_encoder());
    std::unique_ptr<vicreg_encoder_t> vicreg_encoder_holder;
    using mtf_model_t = cuwacunu::wikimyei::representation::encoding::
        mtf_jepa_mae_vicreg::MtfJepaMaeVicreg;
    std::unique_ptr<mtf_model_t> mtf_model_holder;
    std::unique_ptr<channel_graph_first_inference_launcher_detail::
                        mtf_representation_encoder_adapter_t>
        mtf_adapter_holder;
    bool representation_on_device = false;

    if (training_spec.input_representation_checkpoint_path.empty() &&
        !training_spec.allow_untrained_representation) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] Channel MDN training "
          "requires a channel representation checkpoint unless "
          "ALLOW_UNTRAINED_REPRESENTATION is true for a smoke run");
    }
    if (!train_target && training_spec.input_mdn_checkpoint_path.empty()) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] Channel MDN run/eval "
          "requires INPUT_MDN_CHECKPOINT; refusing to evaluate an untrained "
          "channel-context MDN");
    }
    if (use_mtf_representation) {
      namespace mtf =
          cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
      namespace repstream =
          cuwacunu::wikimyei::representation::encoding::vicreg::stream;

      mtf_model_holder =
          std::make_unique<mtf_model_t>(bundle.mtf_jepa_mae_vicreg.config);
      (*mtf_model_holder)
          ->to(bundle.mtf_jepa_mae_vicreg.config.device,
               bundle.mtf_jepa_mae_vicreg.config.dtype);
      if (!training_spec.input_representation_checkpoint_path.empty()) {
        channel_graph_first_inference_launcher_detail::
            load_mtf_jepa_mae_vicreg_checkpoint_file(
                training_spec.input_representation_checkpoint_path,
                *mtf_model_holder, builder_.options().device,
                bundle.mtf_jepa_mae_vicreg.config.channel_count,
                bundle.mtf_jepa_mae_vicreg.config.history_length,
                bundle.mtf_jepa_mae_vicreg.config.input_width,
                bundle.mtf_jepa_mae_vicreg.config.d_model,
                bundle.mtf_jepa_mae_vicreg.config.latent_dim,
                bundle.mtf_jepa_mae_vicreg.config.projector_dim);
        representation_checkpoint_loaded = true;
      }
      channel_graph_first_inference_launcher_detail::freeze_vicreg_encoder(
          *mtf_model_holder);
      representation_on_device = channel_graph_first_inference_launcher_detail::
          parameters_are_on_device((*mtf_model_holder)->parameters(),
                                   builder_.options().device);
      mtf_adapter_holder =
          std::make_unique<channel_graph_first_inference_launcher_detail::
                               mtf_representation_encoder_adapter_t>(
              *mtf_model_holder);
      auto mtf_stream = repstream::channel_representation_stream_t<
          DatatypeT, channel_graph_first_inference_launcher_detail::
                         mtf_representation_encoder_adapter_t>(
          std::move(lifted_stream), *mtf_adapter_holder,
          /*require_finite_valid_features=*/true, /*detach_to_cpu=*/true,
          runtime_report_mode, bundle.mtf_jepa_mae_vicreg.component_assembly_id,
          cuwacunu::wikimyei::assembly::make_assembly_token(
              bundle.mtf_jepa_mae_vicreg_assembly.family,
              bundle.mtf_jepa_mae_vicreg_assembly.component_assembly_id,
              bundle.mtf_jepa_mae_vicreg_assembly.version_token),
          cuwacunu::kikijyeba::topology::dock_binding_token(
              bundle.dock_binding),
          bundle.mtf_jepa_mae_vicreg_assembly.family,
          "wikimyei.representation.mtf_jepa_mae_vicreg.runtime.v1",
          cuwacunu::kikijyeba::protocol::component_stream_wave_from_settings(
              bundle.wave_settings));
      using mtf_stream_t = decltype(mtf_stream);
      representation_stream = std::make_unique<
          channel_graph_first_inference_launcher_detail::
              channel_representation_stream_box_t<key_t, mtf_stream_t>>(
          std::move(mtf_stream));
    } else {
      vicreg_encoder_holder =
          std::make_unique<vicreg_encoder_t>(builder_.make_vicreg_encoder());
      if (!training_spec.input_representation_checkpoint_path.empty()) {
        channel_graph_first_inference_launcher_detail::
            load_vicreg_encoder_checkpoint_file(
                training_spec.input_representation_checkpoint_path,
                *vicreg_encoder_holder, builder_.options().device,
                bundle.vicreg.component_assembly_id,
                "graph_order.channel_node_representation.v1",
                "preserved_primary_output",
                bundle.source_plan.market_graph
                    .computed_graph_order_fingerprint(),
                bundle.source_plan.market_graph.node_ids,
                bundle.vicreg.required_feature_coords,
                cuwacunu::kikijyeba::protocol::
                    graph_first_pipeline_builder_detail::cell_valid_policy_name(
                        bundle.vicreg.cell_valid_policy),
                bundle.vicreg.channel_count, bundle.vicreg.history_length,
                bundle.vicreg.input_width, bundle.vicreg.encoding_dim,
                bundle.vicreg.feature_hidden_dim, bundle.vicreg.temporal_depth,
                bundle.vicreg.recency_decay, bundle.vicreg.min_valid_fraction,
                bundle.vicreg.use_missingness_indicators,
                bundle.vicreg.vicreg_invariance_weight,
                bundle.vicreg.vicreg_variance_weight,
                bundle.vicreg.vicreg_covariance_weight,
                bundle.vicreg.vicreg_variance_floor, bundle.vicreg.vicreg_eps,
                bundle.vicreg.min_valid_rows,
                bundle.vicreg.skip_non_finite_loss);
        representation_checkpoint_loaded = true;
      }
      channel_graph_first_inference_launcher_detail::freeze_vicreg_encoder(
          *vicreg_encoder_holder);
      representation_on_device = channel_graph_first_inference_launcher_detail::
          parameters_are_on_device((*vicreg_encoder_holder)->parameters(),
                                   builder_.options().device);
      auto vicreg_stream = builder_.make_channel_representation_stream(
          std::move(lifted_stream), *vicreg_encoder_holder,
          runtime_report_mode);
      using vicreg_stream_t = decltype(vicreg_stream);
      representation_stream = std::make_unique<
          channel_graph_first_inference_launcher_detail::
              channel_representation_stream_box_t<key_t, vicreg_stream_t>>(
          std::move(vicreg_stream));
    }

    auto report = dry_run_report();
    report.seed_scope = seed_scope;
    report.runtime_report_mode =
        cuwacunu::hero::runtime::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.representation_checkpoint_path =
        training_spec.input_representation_checkpoint_path;
    report.representation_checkpoint_loaded = representation_checkpoint_loaded;
    report.representation_parameter_device_check = representation_on_device;
    report.mdn_checkpoint_path = training_spec.input_mdn_checkpoint_path;
    report.mdn_checkpoint_loaded = false;
    report.all_target_masks_forced_empty =
        options_.force_empty_targets_for_test;
    if (!options_.representation_edge_feature_probe_path.empty()) {
      report.representation_edge_feature_probe_path =
          options_.representation_edge_feature_probe_path.string();
    }
    if (!options_.mdn_edge_context_feature_probe_path.empty()) {
      report.mdn_edge_context_feature_probe_path =
          options_.mdn_edge_context_feature_probe_path.string();
    }
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);

    std::unique_ptr<channel_mdn_conditioned_affine_shadow_t>
        conditioned_affine_shadow;
    if (options_.conditioned_affine_shadow.has_value()) {
      const bool uses_edge_embedding =
          report.direct_edge_return_readout_identity_mode == "edge_embedding" ||
          report.direct_edge_return_readout_identity_mode ==
              "edge_embedding_per_edge";
      const auto readout_feature_dim =
          3 * report.hidden_width +
          (uses_edge_embedding
               ? report.direct_edge_return_readout_identity_embedding_dim
               : 0);
      std::vector<std::filesystem::path> forbidden_report_paths{
          options_.report_path,
          options_.representation_edge_feature_probe_path,
          options_.mdn_edge_context_feature_probe_path,
      };
      channel_mdn_conditioned_affine_shadow_identity_t shadow_identity{
          .graph_order_fingerprint = report.graph_order_fingerprint,
          .node_ids = report.node_ids,
          .edge_ids = report.edge_ids,
          .target_coords = report.target_coords,
          .channel_count = report.channel_count,
          .readout_feature_dim = readout_feature_dim,
          .requested_anchor_index_begin = report.requested_anchor_index_begin,
          .requested_anchor_index_end = report.requested_anchor_index_end,
          .requested_source_key_begin = report.requested_source_key_begin,
          .requested_source_key_end = report.requested_source_key_end,
          .representation_checkpoint_path =
              training_spec.input_representation_checkpoint_path,
          .mdn_checkpoint_path = training_spec.input_mdn_checkpoint_path,
          .device = builder_.options().device,
          .forbidden_report_paths = std::move(forbidden_report_paths),
      };
      conditioned_affine_shadow =
          std::make_unique<channel_mdn_conditioned_affine_shadow_t>(
              *options_.conditioned_affine_shadow, std::move(shadow_identity));
      conditioned_affine_shadow->validate_source_cursor(
          report.source_cursor_token);
    }

    cuwacunu::wikimyei::inference::expected_value::mdn::
        channel_context_mdn_train_model_t *model_ptr = nullptr;
    std::unique_ptr<cuwacunu::wikimyei::inference::expected_value::mdn::
                        channel_context_mdn_train_model_t>
        model_holder;

    double loss_sum = 0.0;
    int64_t loss_count = 0;
    double nll_loss_sum = 0.0;
    int64_t nll_loss_count = 0;
    double edge_auxiliary_loss_sum = 0.0;
    int64_t edge_auxiliary_loss_count = 0;
    double edge_auxiliary_regression_loss_sum = 0.0;
    int64_t edge_auxiliary_regression_loss_count = 0;
    double edge_auxiliary_direction_loss_sum = 0.0;
    int64_t edge_auxiliary_direction_loss_count = 0;
    double edge_auxiliary_rank_loss_sum = 0.0;
    int64_t edge_auxiliary_rank_loss_count = 0;
    int64_t edge_auxiliary_valid_count = 0;
    int64_t edge_auxiliary_pairwise_valid_count = 0;
    double direct_readout_loss_sum = 0.0;
    int64_t direct_readout_loss_count = 0;
    double direct_readout_regression_loss_sum = 0.0;
    int64_t direct_readout_regression_loss_count = 0;
    double direct_readout_direction_loss_sum = 0.0;
    int64_t direct_readout_direction_loss_count = 0;
    double direct_readout_rank_loss_sum = 0.0;
    int64_t direct_readout_rank_loss_count = 0;
    int64_t direct_readout_loss_valid_count = 0;
    int64_t direct_readout_loss_pairwise_valid_count = 0;
    double direct_readout_loss_abs_to_nll_abs_ratio_sum = 0.0;
    int64_t direct_readout_loss_abs_to_nll_abs_ratio_count = 0;
    double direct_readout_loss_abs_to_total_abs_ratio_sum = 0.0;
    int64_t direct_readout_loss_abs_to_total_abs_ratio_count = 0;
    double direct_readout_head_grad_norm_sum = 0.0;
    int64_t direct_readout_head_grad_norm_count = 0;
    double direct_readout_head_grad_norm_max =
        -std::numeric_limits<double>::infinity();
    double direct_readout_head_parameter_update_norm_sum = 0.0;
    int64_t direct_readout_head_parameter_update_norm_count = 0;
    double direct_readout_head_parameter_update_norm_max =
        -std::numeric_limits<double>::infinity();
    double valid_fraction_sum = 0.0;
    int64_t valid_fraction_count = 0;
    double sigma_mean_sum = 0.0;
    int64_t sigma_mean_count = 0;
    double sigma_min = std::numeric_limits<double>::infinity();
    double sigma_max = -std::numeric_limits<double>::infinity();
    double sigma_mean_valid_sum = 0.0;
    int64_t sigma_mean_valid_count = 0;
    double sigma_min_valid = std::numeric_limits<double>::infinity();
    double sigma_max_valid = -std::numeric_limits<double>::infinity();
    double mixture_entropy_sum = 0.0;
    int64_t mixture_entropy_count = 0;
    double max_grad_norm = -std::numeric_limits<double>::infinity();
    double ev_abs_error_sum = 0.0;
    double ev_squared_error_sum = 0.0;
    double ev_signed_error_sum = 0.0;
    int64_t ev_metric_valid_count = 0;
    int64_t ev_direction_correct_count = 0;
    std::vector<int64_t> ev_valid_count_per_channel_sum;
    std::vector<int64_t> ev_direction_correct_count_per_channel_sum;
    std::vector<double> ev_abs_error_sum_per_channel;
    std::vector<double> ev_squared_error_sum_per_channel;
    std::vector<double> ev_signed_error_sum_per_channel;
    std::vector<int64_t> ev_valid_count_per_target_feature_sum;
    std::vector<int64_t> ev_direction_correct_count_per_target_feature_sum;
    std::vector<double> ev_abs_error_sum_per_target_feature;
    std::vector<double> ev_squared_error_sum_per_target_feature;
    std::vector<double> ev_signed_error_sum_per_target_feature;
    std::vector<int64_t> ev_valid_count_per_channel_target_feature_sum;
    std::vector<int64_t>
        ev_direction_correct_count_per_channel_target_feature_sum;
    std::vector<double> ev_abs_error_sum_per_channel_target_feature;
    std::vector<double> ev_squared_error_sum_per_channel_target_feature;
    std::vector<double> ev_signed_error_sum_per_channel_target_feature;
    std::vector<int64_t> ev_valid_count_per_node_sum;
    std::vector<int64_t> ev_direction_correct_count_per_node_sum;
    std::vector<double> ev_abs_error_sum_per_node;
    std::vector<double> ev_squared_error_sum_per_node;
    std::vector<double> ev_signed_error_sum_per_node;
    double edge_return_projection_abs_error_sum = 0.0;
    double edge_return_projection_squared_error_sum = 0.0;
    double edge_return_projection_signed_error_sum = 0.0;
    double edge_return_projection_pred_sum = 0.0;
    double edge_return_projection_realized_sum = 0.0;
    double edge_return_projection_pred_squared_sum = 0.0;
    double edge_return_projection_realized_squared_sum = 0.0;
    double edge_return_projection_cross_sum = 0.0;
    int64_t edge_return_projection_valid_count = 0;
    int64_t edge_return_projection_direction_correct_count = 0;
    int64_t edge_return_projection_pairwise_rank_valid_count = 0;
    int64_t edge_return_projection_pairwise_rank_correct_count = 0;
    int64_t edge_return_projection_best_asset_valid_count = 0;
    int64_t edge_return_projection_best_asset_correct_count = 0;
    std::vector<int64_t> edge_return_projection_valid_count_per_edge_sum;
    std::vector<int64_t>
        edge_return_projection_direction_correct_count_per_edge_sum;
    std::vector<int64_t> edge_return_projection_valid_count_per_channel_sum;
    std::vector<int64_t>
        edge_return_projection_direction_correct_count_per_channel_sum;
    double direct_edge_return_readout_abs_error_sum = 0.0;
    double direct_edge_return_readout_squared_error_sum = 0.0;
    double direct_edge_return_readout_signed_error_sum = 0.0;
    double direct_edge_return_readout_pred_sum = 0.0;
    double direct_edge_return_readout_realized_sum = 0.0;
    double direct_edge_return_readout_pred_squared_sum = 0.0;
    double direct_edge_return_readout_realized_squared_sum = 0.0;
    double direct_edge_return_readout_cross_sum = 0.0;
    double direct_edge_return_readout_pred_min =
        std::numeric_limits<double>::infinity();
    double direct_edge_return_readout_pred_max =
        -std::numeric_limits<double>::infinity();
    double direct_edge_return_readout_realized_min =
        std::numeric_limits<double>::infinity();
    double direct_edge_return_readout_realized_max =
        -std::numeric_limits<double>::infinity();
    int64_t direct_edge_return_readout_valid_count = 0;
    int64_t direct_edge_return_readout_direction_correct_count = 0;
    double direct_edge_return_readout_margin_eps =
        std::numeric_limits<double>::quiet_NaN();
    int64_t direct_edge_return_readout_margin_valid_count = 0;
    int64_t direct_edge_return_readout_margin_direction_correct_count = 0;
    int64_t direct_edge_return_readout_near_zero_target_count = 0;
    double direct_edge_return_readout_rank_margin_eps =
        std::numeric_limits<double>::quiet_NaN();
    int64_t direct_edge_return_readout_margin_pairwise_rank_valid_count = 0;
    int64_t direct_edge_return_readout_margin_pairwise_rank_correct_count = 0;
    int64_t direct_edge_return_readout_pairwise_rank_valid_count = 0;
    int64_t direct_edge_return_readout_pairwise_rank_correct_count = 0;
    int64_t direct_edge_return_readout_best_asset_valid_count = 0;
    int64_t direct_edge_return_readout_best_asset_correct_count = 0;
    std::vector<int64_t> direct_edge_return_readout_valid_count_per_edge_sum;
    std::vector<int64_t>
        direct_edge_return_readout_direction_correct_count_per_edge_sum;
    std::vector<int64_t> direct_edge_return_readout_valid_count_per_channel_sum;
    std::vector<int64_t>
        direct_edge_return_readout_direction_correct_count_per_channel_sum;
    std::vector<double> nll_per_channel_sum;
    std::vector<int64_t> nll_per_channel_count;
    std::vector<double> nll_per_target_feature_sum;
    std::vector<int64_t> nll_per_target_feature_count;
    std::vector<double> nll_per_channel_target_feature_sum;
    std::vector<int64_t> nll_per_channel_target_feature_count;
    std::vector<double> mixture_usage_sum;
    std::vector<int64_t> mixture_usage_count;
    std::vector<int64_t> valid_count_per_channel_sum;
    std::vector<int64_t> valid_count_per_target_feature_sum;
    std::vector<int64_t> valid_count_per_channel_target_feature_sum;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
      }
      if (nll_loss_count > 0) {
        report.mean_nll_loss =
            nll_loss_sum / static_cast<double>(nll_loss_count);
      }
      if (edge_auxiliary_loss_count > 0) {
        report.mean_edge_return_auxiliary_loss =
            edge_auxiliary_loss_sum /
            static_cast<double>(edge_auxiliary_loss_count);
      }
      if (edge_auxiliary_regression_loss_count > 0) {
        report.mean_edge_return_auxiliary_regression_loss =
            edge_auxiliary_regression_loss_sum /
            static_cast<double>(edge_auxiliary_regression_loss_count);
      }
      if (edge_auxiliary_direction_loss_count > 0) {
        report.mean_edge_return_auxiliary_direction_loss =
            edge_auxiliary_direction_loss_sum /
            static_cast<double>(edge_auxiliary_direction_loss_count);
      }
      if (edge_auxiliary_rank_loss_count > 0) {
        report.mean_edge_return_auxiliary_rank_loss =
            edge_auxiliary_rank_loss_sum /
            static_cast<double>(edge_auxiliary_rank_loss_count);
      }
      report.edge_return_auxiliary_valid_count = edge_auxiliary_valid_count;
      report.edge_return_auxiliary_pairwise_valid_count =
          edge_auxiliary_pairwise_valid_count;
      if (direct_readout_loss_count > 0) {
        report.mean_direct_edge_return_readout_loss =
            direct_readout_loss_sum /
            static_cast<double>(direct_readout_loss_count);
      }
      if (direct_readout_regression_loss_count > 0) {
        report.mean_direct_edge_return_readout_regression_loss =
            direct_readout_regression_loss_sum /
            static_cast<double>(direct_readout_regression_loss_count);
      }
      if (direct_readout_direction_loss_count > 0) {
        report.mean_direct_edge_return_readout_direction_loss =
            direct_readout_direction_loss_sum /
            static_cast<double>(direct_readout_direction_loss_count);
      }
      if (direct_readout_rank_loss_count > 0) {
        report.mean_direct_edge_return_readout_rank_loss =
            direct_readout_rank_loss_sum /
            static_cast<double>(direct_readout_rank_loss_count);
      }
      report.direct_edge_return_readout_loss_valid_count =
          direct_readout_loss_valid_count;
      report.direct_edge_return_readout_loss_pairwise_valid_count =
          direct_readout_loss_pairwise_valid_count;
      if (direct_readout_loss_abs_to_nll_abs_ratio_count > 0) {
        report.mean_direct_edge_return_readout_loss_abs_to_nll_abs_ratio =
            direct_readout_loss_abs_to_nll_abs_ratio_sum /
            static_cast<double>(direct_readout_loss_abs_to_nll_abs_ratio_count);
      }
      if (direct_readout_loss_abs_to_total_abs_ratio_count > 0) {
        report.mean_direct_edge_return_readout_loss_abs_to_total_abs_ratio =
            direct_readout_loss_abs_to_total_abs_ratio_sum /
            static_cast<double>(
                direct_readout_loss_abs_to_total_abs_ratio_count);
      }
      if (direct_readout_head_grad_norm_count > 0) {
        report.mean_direct_edge_return_readout_head_grad_norm =
            direct_readout_head_grad_norm_sum /
            static_cast<double>(direct_readout_head_grad_norm_count);
      }
      if (std::isfinite(direct_readout_head_grad_norm_max)) {
        report.max_direct_edge_return_readout_head_grad_norm =
            direct_readout_head_grad_norm_max;
      }
      if (direct_readout_head_parameter_update_norm_count > 0) {
        report.mean_direct_edge_return_readout_head_parameter_update_norm =
            direct_readout_head_parameter_update_norm_sum /
            static_cast<double>(
                direct_readout_head_parameter_update_norm_count);
      }
      if (std::isfinite(direct_readout_head_parameter_update_norm_max)) {
        report.max_direct_edge_return_readout_head_parameter_update_norm =
            direct_readout_head_parameter_update_norm_max;
      }
      if (valid_fraction_count > 0) {
        report.mean_valid_target_fraction =
            valid_fraction_sum / static_cast<double>(valid_fraction_count);
      }
      if (sigma_mean_count > 0) {
        report.mean_sigma_mean =
            sigma_mean_sum / static_cast<double>(sigma_mean_count);
      }
      if (std::isfinite(sigma_min)) {
        report.min_sigma_min = sigma_min;
      }
      if (std::isfinite(sigma_max)) {
        report.max_sigma_max = sigma_max;
      }
      if (sigma_mean_valid_count > 0) {
        report.mean_sigma_mean_valid =
            sigma_mean_valid_sum / static_cast<double>(sigma_mean_valid_count);
      }
      if (std::isfinite(sigma_min_valid)) {
        report.min_sigma_min_valid = sigma_min_valid;
      }
      if (std::isfinite(sigma_max_valid)) {
        report.max_sigma_max_valid = sigma_max_valid;
      }
      if (mixture_entropy_count > 0) {
        report.mean_mixture_entropy =
            mixture_entropy_sum / static_cast<double>(mixture_entropy_count);
      }
      if (std::isfinite(max_grad_norm)) {
        report.max_grad_norm = max_grad_norm;
      }
      report.forecast_ev_valid_count = ev_metric_valid_count;
      if (ev_metric_valid_count > 0) {
        report.ev_mae =
            ev_abs_error_sum / static_cast<double>(ev_metric_valid_count);
        report.ev_rmse = std::sqrt(ev_squared_error_sum /
                                   static_cast<double>(ev_metric_valid_count));
        report.signed_error =
            ev_signed_error_sum / static_cast<double>(ev_metric_valid_count);
        report.directional_accuracy =
            static_cast<double>(ev_direction_correct_count) /
            static_cast<double>(ev_metric_valid_count);
      }
      report.forecast_ev_valid_count_per_channel =
          ev_valid_count_per_channel_sum;
      report.ev_mae_per_channel =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_abs_error_sum_per_channel, ev_valid_count_per_channel_sum);
      report.ev_rmse_per_channel =
          channel_graph_first_inference_launcher_detail::rmse_vector(
              ev_squared_error_sum_per_channel, ev_valid_count_per_channel_sum);
      report.signed_error_per_channel =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_signed_error_sum_per_channel, ev_valid_count_per_channel_sum);
      report.directional_accuracy_per_channel =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  ev_direction_correct_count_per_channel_sum,
                  ev_valid_count_per_channel_sum);

      report.forecast_ev_valid_count_per_target_feature =
          ev_valid_count_per_target_feature_sum;
      report.ev_mae_per_target_feature =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_abs_error_sum_per_target_feature,
              ev_valid_count_per_target_feature_sum);
      report.ev_rmse_per_target_feature =
          channel_graph_first_inference_launcher_detail::rmse_vector(
              ev_squared_error_sum_per_target_feature,
              ev_valid_count_per_target_feature_sum);
      report.signed_error_per_target_feature =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_signed_error_sum_per_target_feature,
              ev_valid_count_per_target_feature_sum);
      report.directional_accuracy_per_target_feature =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  ev_direction_correct_count_per_target_feature_sum,
                  ev_valid_count_per_target_feature_sum);

      report.forecast_ev_valid_count_per_channel_target_feature =
          ev_valid_count_per_channel_target_feature_sum;
      report.ev_mae_per_channel_target_feature =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_abs_error_sum_per_channel_target_feature,
              ev_valid_count_per_channel_target_feature_sum);
      report.ev_rmse_per_channel_target_feature =
          channel_graph_first_inference_launcher_detail::rmse_vector(
              ev_squared_error_sum_per_channel_target_feature,
              ev_valid_count_per_channel_target_feature_sum);
      report.signed_error_per_channel_target_feature =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_signed_error_sum_per_channel_target_feature,
              ev_valid_count_per_channel_target_feature_sum);
      report.directional_accuracy_per_channel_target_feature =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  ev_direction_correct_count_per_channel_target_feature_sum,
                  ev_valid_count_per_channel_target_feature_sum);

      report.forecast_ev_valid_count_per_node = ev_valid_count_per_node_sum;
      report.ev_mae_per_node =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_abs_error_sum_per_node, ev_valid_count_per_node_sum);
      report.ev_rmse_per_node =
          channel_graph_first_inference_launcher_detail::rmse_vector(
              ev_squared_error_sum_per_node, ev_valid_count_per_node_sum);
      report.signed_error_per_node =
          channel_graph_first_inference_launcher_detail::ratio_vector(
              ev_signed_error_sum_per_node, ev_valid_count_per_node_sum);
      report.directional_accuracy_per_node =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  ev_direction_correct_count_per_node_sum,
                  ev_valid_count_per_node_sum);
      report.edge_return_projection_valid_count =
          edge_return_projection_valid_count;
      if (edge_return_projection_valid_count > 0) {
        report.edge_return_projection_ev_mae =
            edge_return_projection_abs_error_sum /
            static_cast<double>(edge_return_projection_valid_count);
        report.edge_return_projection_ev_rmse =
            std::sqrt(edge_return_projection_squared_error_sum /
                      static_cast<double>(edge_return_projection_valid_count));
        report.edge_return_projection_signed_error =
            edge_return_projection_signed_error_sum /
            static_cast<double>(edge_return_projection_valid_count);
        report.edge_return_projection_directional_accuracy =
            static_cast<double>(
                edge_return_projection_direction_correct_count) /
            static_cast<double>(edge_return_projection_valid_count);
        report.edge_return_projection_correlation =
            channel_graph_first_inference_launcher_detail::
                correlation_from_sums(
                    edge_return_projection_valid_count,
                    edge_return_projection_pred_sum,
                    edge_return_projection_realized_sum,
                    edge_return_projection_pred_squared_sum,
                    edge_return_projection_realized_squared_sum,
                    edge_return_projection_cross_sum);
      }
      report.edge_return_projection_pairwise_rank_valid_count =
          edge_return_projection_pairwise_rank_valid_count;
      if (edge_return_projection_pairwise_rank_valid_count > 0) {
        report.edge_return_projection_pairwise_rank_accuracy =
            static_cast<double>(
                edge_return_projection_pairwise_rank_correct_count) /
            static_cast<double>(
                edge_return_projection_pairwise_rank_valid_count);
      }
      report.edge_return_projection_best_asset_valid_count =
          edge_return_projection_best_asset_valid_count;
      if (edge_return_projection_best_asset_valid_count > 0) {
        report.edge_return_projection_best_asset_agreement =
            static_cast<double>(
                edge_return_projection_best_asset_correct_count) /
            static_cast<double>(edge_return_projection_best_asset_valid_count);
      }
      report.edge_return_projection_valid_count_per_edge =
          edge_return_projection_valid_count_per_edge_sum;
      report.edge_return_projection_directional_accuracy_per_edge =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  edge_return_projection_direction_correct_count_per_edge_sum,
                  edge_return_projection_valid_count_per_edge_sum);
      report.edge_return_projection_valid_count_per_channel =
          edge_return_projection_valid_count_per_channel_sum;
      report.edge_return_projection_directional_accuracy_per_channel =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  edge_return_projection_direction_correct_count_per_channel_sum,
                  edge_return_projection_valid_count_per_channel_sum);
      report.direct_edge_return_readout_valid_count =
          direct_edge_return_readout_valid_count;
      if (direct_edge_return_readout_valid_count > 0) {
        report.direct_edge_return_readout_ev_mae =
            direct_edge_return_readout_abs_error_sum /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_ev_rmse = std::sqrt(
            direct_edge_return_readout_squared_error_sum /
            static_cast<double>(direct_edge_return_readout_valid_count));
        report.direct_edge_return_readout_signed_error =
            direct_edge_return_readout_signed_error_sum /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_directional_accuracy =
            static_cast<double>(
                direct_edge_return_readout_direction_correct_count) /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_correlation =
            channel_graph_first_inference_launcher_detail::
                correlation_from_sums(
                    direct_edge_return_readout_valid_count,
                    direct_edge_return_readout_pred_sum,
                    direct_edge_return_readout_realized_sum,
                    direct_edge_return_readout_pred_squared_sum,
                    direct_edge_return_readout_realized_squared_sum,
                    direct_edge_return_readout_cross_sum);
        report.direct_edge_return_readout_pred_mean =
            direct_edge_return_readout_pred_sum /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_realized_mean =
            direct_edge_return_readout_realized_sum /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_pred_std =
            channel_graph_first_inference_launcher_detail::stddev_from_sums(
                direct_edge_return_readout_valid_count,
                direct_edge_return_readout_pred_sum,
                direct_edge_return_readout_pred_squared_sum);
        report.direct_edge_return_readout_realized_std =
            channel_graph_first_inference_launcher_detail::stddev_from_sums(
                direct_edge_return_readout_valid_count,
                direct_edge_return_readout_realized_sum,
                direct_edge_return_readout_realized_squared_sum);
        report.direct_edge_return_readout_pred_to_realized_std_ratio =
            channel_graph_first_inference_launcher_detail::safe_abs_ratio(
                report.direct_edge_return_readout_pred_std,
                report.direct_edge_return_readout_realized_std);
        if (std::isfinite(direct_edge_return_readout_pred_min)) {
          report.direct_edge_return_readout_pred_min =
              direct_edge_return_readout_pred_min;
        }
        if (std::isfinite(direct_edge_return_readout_pred_max)) {
          report.direct_edge_return_readout_pred_max =
              direct_edge_return_readout_pred_max;
        }
        if (std::isfinite(direct_edge_return_readout_realized_min)) {
          report.direct_edge_return_readout_realized_min =
              direct_edge_return_readout_realized_min;
        }
        if (std::isfinite(direct_edge_return_readout_realized_max)) {
          report.direct_edge_return_readout_realized_max =
              direct_edge_return_readout_realized_max;
        }
        report.direct_edge_return_readout_margin_eps =
            direct_edge_return_readout_margin_eps;
        report.direct_edge_return_readout_margin_valid_count =
            direct_edge_return_readout_margin_valid_count;
        if (direct_edge_return_readout_margin_valid_count > 0) {
          report.direct_edge_return_readout_margin_directional_accuracy =
              static_cast<double>(
                  direct_edge_return_readout_margin_direction_correct_count) /
              static_cast<double>(
                  direct_edge_return_readout_margin_valid_count);
        }
        report.direct_edge_return_readout_near_zero_target_count =
            direct_edge_return_readout_near_zero_target_count;
        report.direct_edge_return_readout_near_zero_target_fraction =
            static_cast<double>(
                direct_edge_return_readout_near_zero_target_count) /
            static_cast<double>(direct_edge_return_readout_valid_count);
        report.direct_edge_return_readout_rank_margin_eps =
            direct_edge_return_readout_rank_margin_eps;
        report.direct_edge_return_readout_margin_pairwise_rank_valid_count =
            direct_edge_return_readout_margin_pairwise_rank_valid_count;
        if (direct_edge_return_readout_margin_pairwise_rank_valid_count > 0) {
          report.direct_edge_return_readout_margin_pairwise_rank_accuracy =
              static_cast<double>(
                  direct_edge_return_readout_margin_pairwise_rank_correct_count) /
              static_cast<double>(
                  direct_edge_return_readout_margin_pairwise_rank_valid_count);
        }
      }
      report.direct_edge_return_readout_pairwise_rank_valid_count =
          direct_edge_return_readout_pairwise_rank_valid_count;
      if (direct_edge_return_readout_pairwise_rank_valid_count > 0) {
        report.direct_edge_return_readout_pairwise_rank_accuracy =
            static_cast<double>(
                direct_edge_return_readout_pairwise_rank_correct_count) /
            static_cast<double>(
                direct_edge_return_readout_pairwise_rank_valid_count);
      }
      report.direct_edge_return_readout_best_asset_valid_count =
          direct_edge_return_readout_best_asset_valid_count;
      if (direct_edge_return_readout_best_asset_valid_count > 0) {
        report.direct_edge_return_readout_best_asset_agreement =
            static_cast<double>(
                direct_edge_return_readout_best_asset_correct_count) /
            static_cast<double>(
                direct_edge_return_readout_best_asset_valid_count);
      }
      report.direct_edge_return_readout_valid_count_per_edge =
          direct_edge_return_readout_valid_count_per_edge_sum;
      report.direct_edge_return_readout_directional_accuracy_per_edge =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  direct_edge_return_readout_direction_correct_count_per_edge_sum,
                  direct_edge_return_readout_valid_count_per_edge_sum);
      report.direct_edge_return_readout_directional_accuracy_per_edge_min =
          channel_graph_first_inference_launcher_detail::finite_min_value(
              report.direct_edge_return_readout_directional_accuracy_per_edge);
      report.direct_edge_return_readout_directional_accuracy_per_edge_max =
          channel_graph_first_inference_launcher_detail::finite_max_value(
              report.direct_edge_return_readout_directional_accuracy_per_edge);
      report.direct_edge_return_readout_directional_accuracy_per_edge_spread =
          channel_graph_first_inference_launcher_detail::finite_spread_value(
              report.direct_edge_return_readout_directional_accuracy_per_edge);
      report.direct_edge_return_readout_valid_count_per_channel =
          direct_edge_return_readout_valid_count_per_channel_sum;
      report.direct_edge_return_readout_directional_accuracy_per_channel =
          channel_graph_first_inference_launcher_detail::
              directional_accuracy_vector(
                  direct_edge_return_readout_direction_correct_count_per_channel_sum,
                  direct_edge_return_readout_valid_count_per_channel_sum);
      report.direct_edge_return_readout_directional_accuracy_per_channel_min =
          channel_graph_first_inference_launcher_detail::finite_min_value(
              report
                  .direct_edge_return_readout_directional_accuracy_per_channel);
      report.direct_edge_return_readout_directional_accuracy_per_channel_max =
          channel_graph_first_inference_launcher_detail::finite_max_value(
              report
                  .direct_edge_return_readout_directional_accuracy_per_channel);
      report
          .direct_edge_return_readout_directional_accuracy_per_channel_spread =
          channel_graph_first_inference_launcher_detail::finite_spread_value(
              report
                  .direct_edge_return_readout_directional_accuracy_per_channel);
      report.mean_nll_per_channel =
          channel_graph_first_inference_launcher_detail::mean_vector(
              nll_per_channel_sum, nll_per_channel_count);
      report.mean_nll_per_target_feature =
          channel_graph_first_inference_launcher_detail::mean_vector(
              nll_per_target_feature_sum, nll_per_target_feature_count);
      report.mean_nll_per_channel_target_feature =
          channel_graph_first_inference_launcher_detail::mean_vector(
              nll_per_channel_target_feature_sum,
              nll_per_channel_target_feature_count);
      report.mean_mixture_usage =
          channel_graph_first_inference_launcher_detail::mean_vector(
              mixture_usage_sum, mixture_usage_count);
      report.valid_target_count_per_channel = valid_count_per_channel_sum;
      report.valid_target_count_per_target_feature =
          valid_count_per_target_feature_sum;
      report.valid_target_count_per_channel_target_feature =
          valid_count_per_channel_target_feature_sum;
    };

    auto write_checkpoint = [&]() {
      if (model_ptr == nullptr) {
        return;
      }
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".channel_mdn.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_channel_mdn_v2";
      channel_graph_first_inference_launcher_detail::
          save_channel_mdn_checkpoint_file(checkpoint_path, report, *model_ptr);
    };

    auto write_report = [&]() {
      refresh_running_report();
      report.last_report_attempted_step = report.steps_attempted;
      if (!options_.report_path.empty()) {
        report.report_path = options_.report_path.string();
      }
      if (options_.write_report) {
        report.report_written = true;
        ++report.report_write_count;
        channel_graph_first_inference_launcher_detail::write_report_file(
            options_.report_path, report);
      }
      if (options_.write_report && report.runtime_lls_emitted) {
        channel_graph_first_inference_launcher_detail::
            write_component_runtime_lls_sidecars(
                options_.report_path, report.nodelift_runtime_lls,
                report.representation_runtime_lls, report.mdn_runtime_lls);
      }
      if (options_.learning_probe_report_sink) {
        options_.learning_probe_report_sink(report.to_text());
      }
    };

    const int64_t max_steps = training_spec.max_steps;
    while (max_steps == 0 || report.steps_attempted < max_steps) {
      if (!representation_stream->has_next()) {
        if (!train_target || max_steps == 0) {
          break;
        }
        representation_stream->reset();
        if (!representation_stream->has_next()) {
          break;
        }
      }
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto channel_batch = representation_stream->next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(channel_batch.cursor.anchor_count());

      auto batch = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          make_channel_mdn_input_batch(channel_batch,
                                       builder_.channel_mdn_adapter_options());
      if (!options_.representation_edge_feature_probe_path.empty() &&
          report.representation_edge_feature_probe_error.empty()) {
        try {
          const auto rows = channel_graph_first_inference_launcher_detail::
              append_representation_edge_feature_probe_rows(
                  options_.representation_edge_feature_probe_path,
                  channel_batch, batch,
                  report.edge_return_projection_close_feature_index);
          if (rows > 0) {
            report.representation_edge_feature_probe_written = true;
            report.representation_edge_feature_probe_row_count += rows;
          }
        } catch (const std::exception &ex) {
          report.representation_edge_feature_probe_error = ex.what();
        }
      }
      auto input = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          to_channel_mdn_input(batch);
      if (options_.force_empty_targets_for_test) {
        input.future_mask.fill_(false);
      }
      channel_graph_first_inference_launcher_detail::
          move_channel_mdn_input_to_device(input, builder_.options().dtype,
                                           builder_.options().device);

      if (model_ptr == nullptr) {
        cuwacunu::wikimyei::inference::expected_value::mdn::
            DirectEdgeReturnHeadOptions direct_edge_options{};
        direct_edge_options.identity_mode =
            training_spec.mdn_direct_edge_return_readout_identity_mode;
        direct_edge_options.base_edge_count =
            training_spec.mdn_direct_edge_return_readout_base_edge_count;
        direct_edge_options.identity_embedding_dim =
            training_spec.mdn_direct_edge_return_readout_identity_embedding_dim;
        direct_edge_options.adapter_hidden_dim =
            training_spec.mdn_direct_edge_return_readout_adapter_hidden_dim;
        auto mdn = builder_.make_channel_context_mdn(
            /*context_dim=*/input.context.size(3),
            /*channel_count=*/input.context.size(2),
            /*horizon_count=*/1, std::move(direct_edge_options));
        auto train_options = cuwacunu::wikimyei::inference::expected_value::
            mdn::channel_context_mdn_train_options_from_spec(
                builder_.bundle().channel_mdn);
        train_options.grad_clip_norm = training_spec.grad_clip_norm;
        train_options.edge_return_auxiliary_loss_weight =
            training_spec.mdn_edge_return_auxiliary_loss_weight;
        train_options.edge_return_auxiliary_direction_weight =
            training_spec.mdn_edge_return_auxiliary_direction_weight;
        train_options.edge_return_auxiliary_rank_weight =
            training_spec.mdn_edge_return_auxiliary_rank_weight;
        train_options.edge_return_auxiliary_huber_beta =
            training_spec.mdn_edge_return_auxiliary_huber_beta;
        train_options.edge_return_auxiliary_logit_scale =
            training_spec.mdn_edge_return_auxiliary_logit_scale;
        train_options.direct_edge_return_readout_enabled =
            training_spec.mdn_direct_edge_return_readout_enabled;
        train_options.direct_edge_return_readout_loss_weight =
            training_spec.mdn_direct_edge_return_readout_loss_weight;
        train_options.direct_edge_return_readout_direction_weight =
            training_spec.mdn_direct_edge_return_readout_direction_weight;
        train_options.direct_edge_return_readout_rank_weight =
            training_spec.mdn_direct_edge_return_readout_rank_weight;
        train_options.direct_edge_return_readout_huber_beta =
            training_spec.mdn_direct_edge_return_readout_huber_beta;
        train_options.direct_edge_return_readout_logit_scale =
            training_spec.mdn_direct_edge_return_readout_logit_scale;
        train_options.direct_edge_return_readout_target_scale =
            training_spec.mdn_direct_edge_return_readout_target_scale;
        train_options.direct_edge_return_readout_warmup_steps =
            training_spec.mdn_direct_edge_return_readout_warmup_steps;
        train_options.direct_edge_return_readout_warmup_nll_weight =
            training_spec.mdn_direct_edge_return_readout_warmup_nll_weight;
        train_options.direct_edge_return_readout_post_warmup_nll_weight =
            training_spec.mdn_direct_edge_return_readout_post_warmup_nll_weight;
        train_options.direct_edge_return_readout_warmup_direct_head_only =
            training_spec
                .mdn_direct_edge_return_readout_warmup_direct_head_only;
        model_holder =
            std::make_unique<cuwacunu::wikimyei::inference::expected_value::
                                 mdn::channel_context_mdn_train_model_t>(
                std::move(mdn), training_spec.learning_rate, train_options);
        model_ptr = model_holder.get();
        if (!training_spec.input_mdn_checkpoint_path.empty()) {
          const auto expected_identity =
              channel_graph_first_inference_launcher_detail::
                  checkpoint_identity_from_report(report);
          auto *optimizer = train_target ? &model_ptr->optimizer() : nullptr;
          int64_t restored_optimizer_step_index = 0;
          channel_graph_first_inference_launcher_detail::
              load_channel_mdn_checkpoint_file(
                  training_spec.input_mdn_checkpoint_path, model_ptr->model(),
                  optimizer, &expected_identity,
                  &restored_optimizer_step_index);
          model_ptr->set_optimizer_step_index(restored_optimizer_step_index);
          report.mdn_checkpoint_loaded = true;
        }
        report.mdn_parameter_device_check =
            channel_graph_first_inference_launcher_detail::
                parameters_are_on_device(model_ptr->model()->parameters(),
                                         builder_.options().device);
      }

      torch::Tensor mdn_edge_context_features{};
      auto ensure_mdn_edge_context_features = [&]() -> const torch::Tensor & {
        if (!mdn_edge_context_features.defined()) {
          mdn_edge_context_features = model_ptr->direct_edge_context_features(
              input.context, input.context_mask);
        }
        return mdn_edge_context_features;
      };

      if (!train_target &&
          !options_.mdn_edge_context_feature_probe_path.empty() &&
          report.mdn_edge_context_feature_probe_error.empty()) {
        try {
          const auto rows = channel_graph_first_inference_launcher_detail::
              append_mdn_edge_context_feature_probe_rows(
                  options_.mdn_edge_context_feature_probe_path,
                  ensure_mdn_edge_context_features(), channel_batch, batch,
                  report.edge_return_projection_close_feature_index);
          if (rows > 0) {
            report.mdn_edge_context_feature_probe_written = true;
            report.mdn_edge_context_feature_probe_row_count += rows;
          }
        } catch (const std::exception &ex) {
          report.mdn_edge_context_feature_probe_error = ex.what();
        }
      }

      cuwacunu::wikimyei::inference::expected_value::mdn::
          channel_context_mdn_train_step_result_t step{};
      if (train_target) {
        step = model_ptr->train_one_batch(input);
      } else {
        const auto combined_mask = cuwacunu::wikimyei::inference::
            expected_value::mdn::combine_channel_context_and_future_mask(
                input.context_mask, input.future_mask);
        step.valid_target_count = combined_mask.sum().template item<int64_t>();
        step.valid_target_fraction =
            combined_mask.numel() == 0
                ? 0.0
                : static_cast<double>(step.valid_target_count) /
                      static_cast<double>(combined_mask.numel());
        if (step.valid_target_count == 0) {
          step.skipped = true;
          step.loss = torch::zeros({}, input.context.options());
        } else {
          auto out = model_ptr->forward(input.context, input.context_mask);
          step.nonfinite_output_count =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  channel_context_mdn_train_detail::nonfinite_count(out);
          auto nll_map =
              cuwacunu::wikimyei::inference::expected_value::mdn::mdn_nll_map(
                  out, input.future,
                  cuwacunu::wikimyei::inference::expected_value::mdn::
                      channel_mdn_nll_options_from_spec(
                          builder_.bundle().channel_mdn));
          cuwacunu::wikimyei::inference::expected_value::mdn::
              channel_context_mdn_train_detail::
                  accumulate_expected_value_metrics(step, out, input.future,
                                                    combined_mask,
                                                    input.target_coords);
          cuwacunu::wikimyei::inference::expected_value::mdn::
              channel_context_mdn_train_detail::
                  accumulate_direct_edge_return_readout_metrics(
                      step, out, input.future, combined_mask,
                      input.target_coords);
          step.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::
              mdn::mdn_masked_mean_per_channel(nll_map, combined_mask);
          step.nll_per_target_feature =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  mdn_masked_mean_per_target_feature(nll_map, combined_mask);
          step.nll_per_channel_target_feature = cuwacunu::wikimyei::inference::
              expected_value::mdn::mdn_masked_mean_per_channel_target_feature(
                  nll_map, combined_mask);
          step.valid_count_per_channel = cuwacunu::wikimyei::inference::
              expected_value::mdn::mdn_valid_count_per_channel(combined_mask);
          step.valid_count_per_target_feature =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  mdn_valid_count_per_target_feature(combined_mask);
          step.valid_count_per_channel_target_feature =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  mdn_valid_count_per_channel_target_feature(combined_mask);
          step.nll = cuwacunu::wikimyei::inference::expected_value::mdn::
              compute_channel_context_mdn_nll(
                  out, input,
                  cuwacunu::wikimyei::inference::expected_value::mdn::
                      channel_mdn_nll_options_from_spec(
                          builder_.bundle().channel_mdn));
          auto eval_train_options = cuwacunu::wikimyei::inference::
              expected_value::mdn::channel_context_mdn_train_options_from_spec(
                  builder_.bundle().channel_mdn);
          eval_train_options.edge_return_auxiliary_loss_weight =
              training_spec.mdn_edge_return_auxiliary_loss_weight;
          eval_train_options.edge_return_auxiliary_direction_weight =
              training_spec.mdn_edge_return_auxiliary_direction_weight;
          eval_train_options.edge_return_auxiliary_rank_weight =
              training_spec.mdn_edge_return_auxiliary_rank_weight;
          eval_train_options.edge_return_auxiliary_huber_beta =
              training_spec.mdn_edge_return_auxiliary_huber_beta;
          eval_train_options.edge_return_auxiliary_logit_scale =
              training_spec.mdn_edge_return_auxiliary_logit_scale;
          eval_train_options.direct_edge_return_readout_enabled =
              training_spec.mdn_direct_edge_return_readout_enabled;
          eval_train_options.direct_edge_return_readout_loss_weight =
              training_spec.mdn_direct_edge_return_readout_loss_weight;
          eval_train_options.direct_edge_return_readout_direction_weight =
              training_spec.mdn_direct_edge_return_readout_direction_weight;
          eval_train_options.direct_edge_return_readout_rank_weight =
              training_spec.mdn_direct_edge_return_readout_rank_weight;
          eval_train_options.direct_edge_return_readout_huber_beta =
              training_spec.mdn_direct_edge_return_readout_huber_beta;
          eval_train_options.direct_edge_return_readout_logit_scale =
              training_spec.mdn_direct_edge_return_readout_logit_scale;
          eval_train_options.direct_edge_return_readout_target_scale =
              training_spec.mdn_direct_edge_return_readout_target_scale;
          eval_train_options.direct_edge_return_readout_warmup_steps =
              training_spec.mdn_direct_edge_return_readout_warmup_steps;
          eval_train_options.direct_edge_return_readout_warmup_nll_weight =
              training_spec.mdn_direct_edge_return_readout_warmup_nll_weight;
          eval_train_options.direct_edge_return_readout_post_warmup_nll_weight =
              training_spec
                  .mdn_direct_edge_return_readout_post_warmup_nll_weight;
          eval_train_options
              .direct_edge_return_readout_warmup_direct_head_only =
              training_spec
                  .mdn_direct_edge_return_readout_warmup_direct_head_only;
          const auto edge_auxiliary = cuwacunu::wikimyei::inference::
              expected_value::mdn::channel_context_mdn_train_detail::
                  compute_edge_return_auxiliary_loss(
                      out, input.future, combined_mask, eval_train_options,
                      input.target_coords);
          const auto direct_readout = cuwacunu::wikimyei::inference::
              expected_value::mdn::channel_context_mdn_train_detail::
                  compute_direct_edge_return_readout_loss(
                      out, input.future, combined_mask, eval_train_options,
                      input.target_coords);
          cuwacunu::wikimyei::inference::expected_value::mdn::
              channel_context_mdn_train_detail::
                  record_scheduled_objective(
                      step, step.nll, edge_auxiliary, direct_readout,
                      eval_train_options, model_ptr->optimizer_step_index());
          step.sigma_mean =
              out.sigma.mean().to(torch::kCPU).template item<double>();
          step.sigma_min =
              out.sigma.min().to(torch::kCPU).template item<double>();
          step.sigma_max =
              out.sigma.max().to(torch::kCPU).template item<double>();
          const auto sigma_valid =
              cuwacunu::wikimyei::inference::expected_value::mdn::
                  channel_context_mdn_train_detail::masked_sigma_summary(
                      out, combined_mask);
          step.sigma_mean_valid = sigma_valid.mean;
          step.sigma_min_valid = sigma_valid.min;
          step.sigma_max_valid = sigma_valid.max;
          step.mixture_entropy = cuwacunu::wikimyei::inference::expected_value::
              mdn::channel_context_mdn_train_detail::masked_mixture_entropy(
                  out, combined_mask);
          step.mixture_usage = cuwacunu::wikimyei::inference::expected_value::
              mdn::channel_context_mdn_train_detail::mixture_usage(
                  out, combined_mask);
          step.gradients_finite =
              step.nonfinite_output_count == 0 &&
              torch::isfinite(step.loss).all().template item<bool>();
          if (inference_batch_observer_) {
            inference_batch_observer_(out, batch, channel_batch,
                                      report.wave_pulses_attempted);
          }
          if (conditioned_affine_shadow) {
            conditioned_affine_shadow->observe(
                ensure_mdn_edge_context_features(), input.future, combined_mask,
                input.target_coords,
                static_cast<int64_t>(channel_batch.cursor.begin_anchor_index),
                static_cast<int64_t>(channel_batch.cursor.anchor_count()));
          }
        }
      }

      const bool completed =
          train_target ? step.optimizer_step_applied : !step.skipped;
      if (completed) {
        ++report.steps_completed;
        ++report.wave_pulses_completed;
        if (step.optimizer_step_applied) {
          ++report.optimizer_steps;
        }
        report.last_loss = step.loss.defined() && step.loss.numel() > 0
                               ? step.loss.detach()
                                     .to(torch::kCPU)
                                     .to(torch::kFloat64)
                                     .template item<double>()
                               : std::numeric_limits<double>::quiet_NaN();
        if (std::isfinite(report.last_loss)) {
          loss_sum += report.last_loss;
          ++loss_count;
        }
        auto finite_scalar_or_nan = [](const torch::Tensor &value) {
          if (!value.defined() || value.numel() == 0) {
            return std::numeric_limits<double>::quiet_NaN();
          }
          const double scalar = value.detach()
                                    .to(torch::kCPU)
                                    .to(torch::kFloat64)
                                    .template item<double>();
          return std::isfinite(scalar)
                     ? scalar
                     : std::numeric_limits<double>::quiet_NaN();
        };
        report.last_nll_loss = finite_scalar_or_nan(step.nll);
        if (std::isfinite(report.last_nll_loss)) {
          nll_loss_sum += report.last_nll_loss;
          ++nll_loss_count;
        }
        report.last_edge_return_auxiliary_loss =
            finite_scalar_or_nan(step.edge_return_auxiliary_loss);
        if (std::isfinite(report.last_edge_return_auxiliary_loss)) {
          edge_auxiliary_loss_sum += report.last_edge_return_auxiliary_loss;
          ++edge_auxiliary_loss_count;
        }
        report.last_edge_return_auxiliary_regression_loss =
            finite_scalar_or_nan(step.edge_return_auxiliary_regression_loss);
        if (std::isfinite(report.last_edge_return_auxiliary_regression_loss)) {
          edge_auxiliary_regression_loss_sum +=
              report.last_edge_return_auxiliary_regression_loss;
          ++edge_auxiliary_regression_loss_count;
        }
        report.last_edge_return_auxiliary_direction_loss =
            finite_scalar_or_nan(step.edge_return_auxiliary_direction_loss);
        if (std::isfinite(report.last_edge_return_auxiliary_direction_loss)) {
          edge_auxiliary_direction_loss_sum +=
              report.last_edge_return_auxiliary_direction_loss;
          ++edge_auxiliary_direction_loss_count;
        }
        report.last_edge_return_auxiliary_rank_loss =
            finite_scalar_or_nan(step.edge_return_auxiliary_rank_loss);
        if (std::isfinite(report.last_edge_return_auxiliary_rank_loss)) {
          edge_auxiliary_rank_loss_sum +=
              report.last_edge_return_auxiliary_rank_loss;
          ++edge_auxiliary_rank_loss_count;
        }
        edge_auxiliary_valid_count += step.edge_return_auxiliary_valid_count;
        edge_auxiliary_pairwise_valid_count +=
            step.edge_return_auxiliary_pairwise_valid_count;
        report.last_direct_edge_return_readout_loss =
            finite_scalar_or_nan(step.direct_edge_return_readout_loss);
        if (std::isfinite(report.last_direct_edge_return_readout_loss)) {
          direct_readout_loss_sum +=
              report.last_direct_edge_return_readout_loss;
          ++direct_readout_loss_count;
        }
        report.last_direct_edge_return_readout_regression_loss =
            finite_scalar_or_nan(
                step.direct_edge_return_readout_regression_loss);
        if (std::isfinite(
                report.last_direct_edge_return_readout_regression_loss)) {
          direct_readout_regression_loss_sum +=
              report.last_direct_edge_return_readout_regression_loss;
          ++direct_readout_regression_loss_count;
        }
        report.last_direct_edge_return_readout_direction_loss =
            finite_scalar_or_nan(
                step.direct_edge_return_readout_direction_loss);
        if (std::isfinite(
                report.last_direct_edge_return_readout_direction_loss)) {
          direct_readout_direction_loss_sum +=
              report.last_direct_edge_return_readout_direction_loss;
          ++direct_readout_direction_loss_count;
        }
        report.last_direct_edge_return_readout_rank_loss =
            finite_scalar_or_nan(step.direct_edge_return_readout_rank_loss);
        if (std::isfinite(report.last_direct_edge_return_readout_rank_loss)) {
          direct_readout_rank_loss_sum +=
              report.last_direct_edge_return_readout_rank_loss;
          ++direct_readout_rank_loss_count;
        }
        report.last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio =
            channel_graph_first_inference_launcher_detail::safe_abs_ratio(
                report.last_direct_edge_return_readout_loss,
                report.last_nll_loss);
        if (std::isfinite(
                report
                    .last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio)) {
          direct_readout_loss_abs_to_nll_abs_ratio_sum +=
              report.last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio;
          ++direct_readout_loss_abs_to_nll_abs_ratio_count;
        }
        report.last_direct_edge_return_readout_loss_abs_to_total_abs_ratio =
            channel_graph_first_inference_launcher_detail::safe_abs_ratio(
                report.last_direct_edge_return_readout_loss, report.last_loss);
        if (std::isfinite(
                report
                    .last_direct_edge_return_readout_loss_abs_to_total_abs_ratio)) {
          direct_readout_loss_abs_to_total_abs_ratio_sum +=
              report
                  .last_direct_edge_return_readout_loss_abs_to_total_abs_ratio;
          ++direct_readout_loss_abs_to_total_abs_ratio_count;
        }
        report.last_direct_edge_return_readout_head_grad_norm =
            step.direct_edge_return_readout_head_grad_norm;
        if (std::isfinite(
                report.last_direct_edge_return_readout_head_grad_norm)) {
          direct_readout_head_grad_norm_sum +=
              report.last_direct_edge_return_readout_head_grad_norm;
          direct_readout_head_grad_norm_max =
              std::max(direct_readout_head_grad_norm_max,
                       report.last_direct_edge_return_readout_head_grad_norm);
          ++direct_readout_head_grad_norm_count;
        }
        report.last_direct_edge_return_readout_head_parameter_norm =
            step.direct_edge_return_readout_head_parameter_norm_after_step;
        report.last_direct_edge_return_readout_head_parameter_update_norm =
            step.direct_edge_return_readout_head_parameter_update_norm;
        if (std::isfinite(
                report
                    .last_direct_edge_return_readout_head_parameter_update_norm)) {
          direct_readout_head_parameter_update_norm_sum +=
              report.last_direct_edge_return_readout_head_parameter_update_norm;
          direct_readout_head_parameter_update_norm_max = std::max(
              direct_readout_head_parameter_update_norm_max,
              report
                  .last_direct_edge_return_readout_head_parameter_update_norm);
          ++direct_readout_head_parameter_update_norm_count;
        }
        report.last_direct_edge_return_readout_scheduled_nll_weight =
            step.direct_edge_return_readout_scheduled_nll_weight;
        report.last_direct_edge_return_readout_warmup_active =
            step.direct_edge_return_readout_warmup_active;
        report.last_direct_edge_return_readout_direct_head_only_warmup_active =
            step.direct_edge_return_readout_direct_head_only_warmup_active;
        if (step.direct_edge_return_readout_warmup_active) {
          ++report.direct_edge_return_readout_warmup_step_count;
        }
        if (step.direct_edge_return_readout_direct_head_only_warmup_active) {
          ++report
                .direct_edge_return_readout_direct_head_only_warmup_step_count;
        }
        direct_readout_loss_valid_count +=
            step.direct_edge_return_readout_loss_valid_count;
        direct_readout_loss_pairwise_valid_count +=
            step.direct_edge_return_readout_loss_pairwise_valid_count;
      } else if (step.skipped) {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }

      report.last_valid_target_count = step.valid_target_count;
      report.total_valid_target_count += step.valid_target_count;
      report.last_valid_target_fraction = step.valid_target_fraction;
      if (std::isfinite(step.valid_target_fraction)) {
        valid_fraction_sum += step.valid_target_fraction;
        ++valid_fraction_count;
      }
      report.last_sigma_mean = step.sigma_mean;
      if (std::isfinite(step.sigma_mean)) {
        sigma_mean_sum += step.sigma_mean;
        ++sigma_mean_count;
      }
      if (std::isfinite(step.sigma_min)) {
        sigma_min = std::min(sigma_min, step.sigma_min);
      }
      if (std::isfinite(step.sigma_max)) {
        sigma_max = std::max(sigma_max, step.sigma_max);
      }
      report.last_sigma_mean_valid = step.sigma_mean_valid;
      if (std::isfinite(step.sigma_mean_valid)) {
        sigma_mean_valid_sum += step.sigma_mean_valid;
        ++sigma_mean_valid_count;
      }
      if (std::isfinite(step.sigma_min_valid)) {
        sigma_min_valid = std::min(sigma_min_valid, step.sigma_min_valid);
      }
      if (std::isfinite(step.sigma_max_valid)) {
        sigma_max_valid = std::max(sigma_max_valid, step.sigma_max_valid);
      }
      if (std::isfinite(step.mixture_entropy)) {
        mixture_entropy_sum += step.mixture_entropy;
        ++mixture_entropy_count;
      }
      if (step.ev_metric_valid_count > 0) {
        ev_abs_error_sum += step.ev_abs_error_sum;
        ev_squared_error_sum += step.ev_squared_error_sum;
        ev_signed_error_sum += step.ev_signed_error_sum;
        ev_metric_valid_count += step.ev_metric_valid_count;
        ev_direction_correct_count += step.ev_direction_correct_count;
        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_valid_count_per_channel, ev_valid_count_per_channel_sum);
        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_direction_correct_count_per_channel,
            ev_direction_correct_count_per_channel_sum);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_abs_error_sum_per_channel,
                                         ev_abs_error_sum_per_channel);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_squared_error_sum_per_channel,
                                         ev_squared_error_sum_per_channel);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_signed_error_sum_per_channel,
                                         ev_signed_error_sum_per_channel);

        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_valid_count_per_target_feature,
            ev_valid_count_per_target_feature_sum);
        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_direction_correct_count_per_target_feature,
            ev_direction_correct_count_per_target_feature_sum);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_abs_error_sum_per_target_feature,
                ev_abs_error_sum_per_target_feature);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_squared_error_sum_per_target_feature,
                ev_squared_error_sum_per_target_feature);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_signed_error_sum_per_target_feature,
                ev_signed_error_sum_per_target_feature);

        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_valid_count_per_channel_target_feature,
            ev_valid_count_per_channel_target_feature_sum);
        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_direction_correct_count_per_channel_target_feature,
            ev_direction_correct_count_per_channel_target_feature_sum);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_abs_error_sum_per_channel_target_feature,
                ev_abs_error_sum_per_channel_target_feature);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_squared_error_sum_per_channel_target_feature,
                ev_squared_error_sum_per_channel_target_feature);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(
                step.ev_signed_error_sum_per_channel_target_feature,
                ev_signed_error_sum_per_channel_target_feature);

        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_valid_count_per_node, ev_valid_count_per_node_sum);
        channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
            step.ev_direction_correct_count_per_node,
            ev_direction_correct_count_per_node_sum);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_abs_error_sum_per_node,
                                         ev_abs_error_sum_per_node);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_squared_error_sum_per_node,
                                         ev_squared_error_sum_per_node);
        channel_graph_first_inference_launcher_detail::
            accumulate_double_sum_vector(step.ev_signed_error_sum_per_node,
                                         ev_signed_error_sum_per_node);
        if (step.edge_return_projection_valid_count > 0) {
          edge_return_projection_abs_error_sum +=
              step.edge_return_projection_abs_error_sum;
          edge_return_projection_squared_error_sum +=
              step.edge_return_projection_squared_error_sum;
          edge_return_projection_signed_error_sum +=
              step.edge_return_projection_signed_error_sum;
          edge_return_projection_pred_sum +=
              step.edge_return_projection_pred_sum;
          edge_return_projection_realized_sum +=
              step.edge_return_projection_realized_sum;
          edge_return_projection_pred_squared_sum +=
              step.edge_return_projection_pred_squared_sum;
          edge_return_projection_realized_squared_sum +=
              step.edge_return_projection_realized_squared_sum;
          edge_return_projection_cross_sum +=
              step.edge_return_projection_cross_sum;
          edge_return_projection_valid_count +=
              step.edge_return_projection_valid_count;
          edge_return_projection_direction_correct_count +=
              step.edge_return_projection_direction_correct_count;
          edge_return_projection_pairwise_rank_valid_count +=
              step.edge_return_projection_pairwise_rank_valid_count;
          edge_return_projection_pairwise_rank_correct_count +=
              step.edge_return_projection_pairwise_rank_correct_count;
          edge_return_projection_best_asset_valid_count +=
              step.edge_return_projection_best_asset_valid_count;
          edge_return_projection_best_asset_correct_count +=
              step.edge_return_projection_best_asset_correct_count;
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.edge_return_projection_valid_count_per_edge,
              edge_return_projection_valid_count_per_edge_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.edge_return_projection_direction_correct_count_per_edge,
              edge_return_projection_direction_correct_count_per_edge_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.edge_return_projection_valid_count_per_channel,
              edge_return_projection_valid_count_per_channel_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.edge_return_projection_direction_correct_count_per_channel,
              edge_return_projection_direction_correct_count_per_channel_sum);
        }
        if (step.direct_edge_return_readout_valid_count > 0) {
          direct_edge_return_readout_abs_error_sum +=
              step.direct_edge_return_readout_abs_error_sum;
          direct_edge_return_readout_squared_error_sum +=
              step.direct_edge_return_readout_squared_error_sum;
          direct_edge_return_readout_signed_error_sum +=
              step.direct_edge_return_readout_signed_error_sum;
          direct_edge_return_readout_pred_sum +=
              step.direct_edge_return_readout_pred_sum;
          direct_edge_return_readout_realized_sum +=
              step.direct_edge_return_readout_realized_sum;
          direct_edge_return_readout_pred_squared_sum +=
              step.direct_edge_return_readout_pred_squared_sum;
          direct_edge_return_readout_realized_squared_sum +=
              step.direct_edge_return_readout_realized_squared_sum;
          direct_edge_return_readout_cross_sum +=
              step.direct_edge_return_readout_cross_sum;
          if (std::isfinite(step.direct_edge_return_readout_pred_min)) {
            direct_edge_return_readout_pred_min =
                std::min(direct_edge_return_readout_pred_min,
                         step.direct_edge_return_readout_pred_min);
          }
          if (std::isfinite(step.direct_edge_return_readout_pred_max)) {
            direct_edge_return_readout_pred_max =
                std::max(direct_edge_return_readout_pred_max,
                         step.direct_edge_return_readout_pred_max);
          }
          if (std::isfinite(step.direct_edge_return_readout_realized_min)) {
            direct_edge_return_readout_realized_min =
                std::min(direct_edge_return_readout_realized_min,
                         step.direct_edge_return_readout_realized_min);
          }
          if (std::isfinite(step.direct_edge_return_readout_realized_max)) {
            direct_edge_return_readout_realized_max =
                std::max(direct_edge_return_readout_realized_max,
                         step.direct_edge_return_readout_realized_max);
          }
          direct_edge_return_readout_valid_count +=
              step.direct_edge_return_readout_valid_count;
          direct_edge_return_readout_direction_correct_count +=
              step.direct_edge_return_readout_direction_correct_count;
          direct_edge_return_readout_margin_eps =
              step.direct_edge_return_readout_margin_eps;
          direct_edge_return_readout_margin_valid_count +=
              step.direct_edge_return_readout_margin_valid_count;
          direct_edge_return_readout_margin_direction_correct_count +=
              step.direct_edge_return_readout_margin_direction_correct_count;
          direct_edge_return_readout_near_zero_target_count +=
              step.direct_edge_return_readout_near_zero_target_count;
          direct_edge_return_readout_rank_margin_eps =
              step.direct_edge_return_readout_rank_margin_eps;
          direct_edge_return_readout_margin_pairwise_rank_valid_count +=
              step.direct_edge_return_readout_margin_pairwise_rank_valid_count;
          direct_edge_return_readout_margin_pairwise_rank_correct_count +=
              step.direct_edge_return_readout_margin_pairwise_rank_correct_count;
          direct_edge_return_readout_pairwise_rank_valid_count +=
              step.direct_edge_return_readout_pairwise_rank_valid_count;
          direct_edge_return_readout_pairwise_rank_correct_count +=
              step.direct_edge_return_readout_pairwise_rank_correct_count;
          direct_edge_return_readout_best_asset_valid_count +=
              step.direct_edge_return_readout_best_asset_valid_count;
          direct_edge_return_readout_best_asset_correct_count +=
              step.direct_edge_return_readout_best_asset_correct_count;
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.direct_edge_return_readout_valid_count_per_edge,
              direct_edge_return_readout_valid_count_per_edge_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.direct_edge_return_readout_direction_correct_count_per_edge,
              direct_edge_return_readout_direction_correct_count_per_edge_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.direct_edge_return_readout_valid_count_per_channel,
              direct_edge_return_readout_valid_count_per_channel_sum);
          channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
              step.direct_edge_return_readout_direction_correct_count_per_channel,
              direct_edge_return_readout_direction_correct_count_per_channel_sum);
        }
      }
      report.nonfinite_output_count += step.nonfinite_output_count;
      report.last_grad_norm = step.grad_norm;
      if (std::isfinite(step.grad_norm)) {
        max_grad_norm = std::max(max_grad_norm, step.grad_norm);
      }
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.nll_per_channel, nll_per_channel_sum, nll_per_channel_count);
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.nll_per_target_feature, nll_per_target_feature_sum,
          nll_per_target_feature_count);
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.nll_per_channel_target_feature,
          nll_per_channel_target_feature_sum,
          nll_per_channel_target_feature_count);
      channel_graph_first_inference_launcher_detail::accumulate_finite_vector(
          step.mixture_usage, mixture_usage_sum, mixture_usage_count);
      channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
          step.valid_count_per_channel, valid_count_per_channel_sum);
      channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
          step.valid_count_per_target_feature,
          valid_count_per_target_feature_sum);
      channel_graph_first_inference_launcher_detail::accumulate_i64_vector(
          step.valid_count_per_channel_target_feature,
          valid_count_per_channel_target_feature_sum);
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 && step.gradients_finite) ? 1.0
                                                                          : 0.0;
      if (cuwacunu::hero::lattice::runtime_report::runtime_report_enabled(
              runtime_report_mode)) {
        report.nodelift_runtime_lls = batch.nodelift_runtime_lls;
        report.representation_runtime_lls = batch.representation_runtime_lls;
        report.mdn_runtime_lls = channel_graph_first_inference_launcher_detail::
            make_channel_mdn_runtime_lls(
                batch, step,
                builder_.bundle().channel_mdn.component_assembly_id,
                cuwacunu::wikimyei::assembly::make_assembly_token(
                    builder_.bundle().channel_mdn_assembly.family,
                    builder_.bundle()
                        .channel_mdn_assembly.component_assembly_id,
                    builder_.bundle().channel_mdn_assembly.version_token),
                cuwacunu::kikijyeba::topology::dock_binding_token(
                    builder_.bundle().dock_binding),
                cuwacunu::kikijyeba::protocol::
                    component_stream_wave_from_settings(
                        builder_.bundle().wave_settings),
                runtime_report_mode, report.optimizer_steps,
                report.wave_pulses_attempted);
        report.runtime_lls_emitted = true;
      }

      if (train_target && step.optimizer_step_applied && checkpoint_every > 0 &&
          report.optimizer_steps % checkpoint_every == 0) {
        write_checkpoint();
      }
      if (report_every > 0 && report.steps_attempted % report_every == 0) {
        write_report();
      }
    }

    refresh_running_report();
    if (!options_.write_report ||
        report.last_report_attempted_step != report.steps_attempted) {
      write_report();
    }
    if (conditioned_affine_shadow) {
      conditioned_affine_shadow->write_report_atomic(
          report.steps_attempted, report.steps_completed,
          report.wave_streamed_anchor_count);
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
        builder_.bundle().channel_mdn_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[channel_graph_first_inference_launcher] builder batch_size must "
          "match Channel MDN training BATCH_SIZE");
    }
  }

  builder_t builder_;
  channel_graph_first_inference_launcher_options_t options_{};
  inference_batch_observer_t inference_batch_observer_{};
};

} // namespace cuwacunu::jkimyei::training::inference
