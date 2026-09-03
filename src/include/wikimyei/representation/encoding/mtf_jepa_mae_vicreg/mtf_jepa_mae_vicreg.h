// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <ATen/Config.h>
#include <torch/torch.h>

#if AT_MKL_ENABLED()
extern "C" int MKL_Set_Num_Threads_Local(int nth);
#endif

namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg {

/*
 * Standalone representation pretraining model:
 * multi-scale time tokens + windowed frequency magnitudes, JEPA latent
 * prediction as the primary objective, a small auxiliary MAE decoder, and
 * a local VICReg-style stability head. This file intentionally does not depend
 * on the production VICReg representation implementation.
 * Downstream forecasting and MDN heads are intentionally out of scope here.
 */

enum class mtf_serving_pool_policy_t {
  all_tokens,
  time_only,
  frequency_only,
  domain_balanced,
  structured_cdsb_v1,
  structured_cdsb_sparse_v1,
};

enum class mtf_jepa_mask_policy_t {
  legacy_soft_overlap,
  paired_target_legacy_context_v1,
  support_separated_pair_v1,
};

enum class mtf_vicreg_view_pairing_policy_t {
  independent_weak,
  tied_weak,
  clean_identical,
};

[[nodiscard]] inline const char *
mtf_vicreg_view_pairing_policy_name(mtf_vicreg_view_pairing_policy_t policy) {
  switch (policy) {
  case mtf_vicreg_view_pairing_policy_t::independent_weak:
    return "independent_weak";
  case mtf_vicreg_view_pairing_policy_t::tied_weak:
    return "tied_weak";
  case mtf_vicreg_view_pairing_policy_t::clean_identical:
    return "clean_identical";
  }
  throw std::runtime_error(
      "[mtf_jepa_mae_vicreg] unknown VICReg view pairing policy");
}

[[nodiscard]] inline const char *
mtf_jepa_mask_policy_name(mtf_jepa_mask_policy_t policy) {
  switch (policy) {
  case mtf_jepa_mask_policy_t::legacy_soft_overlap:
    return "legacy_soft_overlap";
  case mtf_jepa_mask_policy_t::paired_target_legacy_context_v1:
    return "paired_target_legacy_context_v1";
  case mtf_jepa_mask_policy_t::support_separated_pair_v1:
    return "support_separated_pair_v1";
  }
  throw std::runtime_error("[mtf_jepa_mae_vicreg] unknown JEPA mask policy");
}

[[nodiscard]] inline const char *
mtf_serving_pool_policy_name(mtf_serving_pool_policy_t policy) {
  switch (policy) {
  case mtf_serving_pool_policy_t::all_tokens:
    return "all_tokens";
  case mtf_serving_pool_policy_t::time_only:
    return "time_only";
  case mtf_serving_pool_policy_t::frequency_only:
    return "frequency_only";
  case mtf_serving_pool_policy_t::domain_balanced:
    return "domain_balanced";
  case mtf_serving_pool_policy_t::structured_cdsb_v1:
    return "structured_cdsb_v1";
  case mtf_serving_pool_policy_t::structured_cdsb_sparse_v1:
    return "structured_cdsb_sparse_v1";
  }
  throw std::runtime_error("[mtf_jepa_mae_vicreg] unknown serving pool policy");
}

struct mtf_jepa_mae_vicreg_config_t {
  int64_t channel_count{1};
  int64_t history_length{64};
  int64_t input_width{9};

  int64_t d_model{32};
  int64_t latent_dim{32};
  int64_t projector_dim{64};
  int64_t predictor_hidden_dim{64};
  int64_t num_encoder_layers{2};
  int64_t num_predictor_layers{2};
  int64_t num_decoder_layers{1};
  int64_t num_heads{4};
  double dropout{0.10};

  std::vector<int64_t> time_scales{8, 16, 32, 64};
  std::vector<int64_t> scale_strides{4, 8, 16, 32};
  bool use_frequency_tokens{true};
  int64_t frequency_num_bins{16};
  bool frequency_log_magnitude{true};
  mtf_serving_pool_policy_t serving_pool_policy{
      mtf_serving_pool_policy_t::all_tokens};

  double mask_ratio_time{0.50};
  double mask_ratio_frequency{0.30};
  double mask_ratio_channel{0.10};
  double min_context_ratio{0.25};
  mtf_jepa_mask_policy_t jepa_mask_policy{
      mtf_jepa_mask_policy_t::legacy_soft_overlap};

  double lambda_jepa{1.0};
  double lambda_mae{0.25};
  double lambda_tf_align{0.10};
  double lambda_vicreg{0.05};

  double vicreg_sim_weight{25.0};
  double vicreg_var_weight{25.0};
  double vicreg_cov_weight{1.0};
  double vicreg_variance_floor{1.0};
  double vicreg_variance_epsilon{1e-4};

  double target_ema_tau{0.996};
  bool use_target_ema{true};
  bool stop_gradient_target{true};
  bool return_diagnostics{true};
  bool return_vicreg_debug_tensors{false};

  bool use_mae_decoder{true};
  bool use_jepa_loss{true};
  bool use_tf_align_loss{true};
  bool use_vicreg_loss{true};
  bool use_global_vicreg{true};
  bool use_channel_vicreg{false};
  bool stratify_channel_vicreg_by_channel{false};
  double lambda_global_vicreg{1.0};
  double lambda_channel_vicreg{1.0};
  bool use_raw_reconstruction_targets{true};
  bool strict_finite_loss{true};
  bool couple_time_frequency_masks{true};
  bool mask_same_window_across_domains{true};
  bool mask_same_channel_block{false};
  double max_context_target_time_overlap{0.50};

  double vicreg_view_gaussian_jitter_std{0.005};
  double vicreg_view_time_dropout_scale{0.10};
  mtf_vicreg_view_pairing_policy_t vicreg_view_pairing_policy{
      mtf_vicreg_view_pairing_policy_t::independent_weak};

  std::string augmentation_profile{"unset"};
  double gaussian_jitter_std{0.0};
  double feature_dropout_prob{0.0};
  double history_dropout_prob{0.0};
  int64_t time_crop_jitter_max{0};
  double time_dilation_min{1.0};
  double time_dilation_max{1.0};
  double time_warp_max{0.0};
  double amplitude_scale_min{1.0};
  double amplitude_scale_max{1.0};
  double amplitude_shift_std{0.0};
  double frequency_mask_ratio{0.0};
  double frequency_jitter_std{0.0};
  double phase_jitter_max{0.0};
  double channel_dropout_prob{0.0};
  double cross_channel_dropout_prob{0.0};
  double node_dropout_prob{0.0};
  double edge_dropout_prob{0.0};
  double magnitude_normalization_noise_std{0.0};

  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

struct mtf_input_t {
  torch::Tensor data{};         // [B,C,Hx,Dx]
  torch::Tensor feature_mask{}; // [B,C,Hx,Dx], bool
};

struct mtf_token_metadata_t {
  torch::Tensor start_index{}; // [N], int64
  torch::Tensor width{};       // [N], int64
  torch::Tensor scale_id{};    // [N], int64
  torch::Tensor channel_id{};  // [N], int64
  torch::Tensor domain_id{};   // [N], int64, 0=time, 1=frequency
};

struct mtf_token_batch_t {
  torch::Tensor tokens{};                           // [B,N,d_model]
  torch::Tensor reconstruction_targets{};           // [B,N,d_model], fallback
  torch::Tensor time_reconstruction_targets{};      // [B,N,2*Dx]
  torch::Tensor frequency_reconstruction_targets{}; // [B,N,K*Dx]
  torch::Tensor time_reconstruction_mask{};         // [B,N,2*Dx], bool
  torch::Tensor frequency_reconstruction_mask{};    // [B,N,K*Dx], bool
  torch::Tensor token_mask{};                       // [B,N], bool
  mtf_token_metadata_t metadata{};
};

struct jepa_context_target_mask_t {
  torch::Tensor context_mask{}; // [B,N], bool
  torch::Tensor target_mask{};  // [B,N], bool
  torch::Tensor valid_mask{};   // [B,N], bool
  torch::Tensor mask_ratio_actual{};
  int64_t num_context_tokens{0};
  int64_t num_target_tokens{0};
  int64_t hard_forbidden_count{0};
  int64_t soft_forbidden_count{0};
  int64_t relaxed_soft_forbidden_count{0};
  int64_t reduced_targets_for_context_count{0};
  int64_t context_starved_sample_count{0};
  int64_t min_context_satisfied_count{0};
  int64_t support_separated_sample_count{0};
  int64_t retained_legacy_context_count{0};
  int64_t replaced_context_count{0};
};

struct mtf_jepa_mae_vicreg_encode_output_t {
  torch::Tensor embeddings{};         // [B,N,latent_dim]
  torch::Tensor pooled_embedding{};   // [B,latent_dim]
  torch::Tensor pooled_by_channel{};  // [B,C,latent_dim]
  torch::Tensor pooled_time{};        // [B,latent_dim]
  torch::Tensor pooled_frequency{};   // [B,latent_dim]
  torch::Tensor token_mask{};         // [B,N]
  torch::Tensor sample_valid_mask{};  // [B]
  torch::Tensor channel_valid_mask{}; // [B,C]
  mtf_token_metadata_t metadata{};
};

struct mtf_jepa_mae_vicreg_output_t {
  torch::Tensor embeddings{};
  torch::Tensor pooled_embedding{};
  torch::Tensor pooled_by_channel{};
  torch::Tensor pooled_time{};
  torch::Tensor pooled_frequency{};
  torch::Tensor loss{};
  torch::Tensor loss_jepa{};
  torch::Tensor loss_mae{};
  torch::Tensor loss_mae_time{};
  torch::Tensor loss_mae_frequency{};
  torch::Tensor loss_tf_align{};
  torch::Tensor loss_vicreg{};
  torch::Tensor loss_vicreg_global{};
  torch::Tensor loss_vicreg_channel{};
  torch::Tensor jepa_target_mask{};
  torch::Tensor jepa_context_mask{};
  torch::Tensor vicreg_drawn_a_data{};
  torch::Tensor vicreg_drawn_a_feature_mask{};
  torch::Tensor vicreg_drawn_b_data{};
  torch::Tensor vicreg_drawn_b_feature_mask{};
  torch::Tensor vicreg_view_a_data{};
  torch::Tensor vicreg_view_a_feature_mask{};
  torch::Tensor vicreg_view_b_data{};
  torch::Tensor vicreg_view_b_feature_mask{};
  torch::Tensor vicreg_view_a_token_mask{};
  torch::Tensor vicreg_view_b_token_mask{};
  torch::Tensor vicreg_view_a_sample_valid_mask{};
  torch::Tensor vicreg_view_b_sample_valid_mask{};
  torch::Tensor vicreg_view_a_pooled_by_channel{};
  torch::Tensor vicreg_view_b_pooled_by_channel{};
  torch::Tensor vicreg_view_a_pooled_global{};
  torch::Tensor vicreg_view_b_pooled_global{};
  torch::Tensor vicreg_view_a_projected_global{};
  torch::Tensor vicreg_view_b_projected_global{};
  torch::Tensor vicreg_global_joint_mask{};
  torch::Tensor vicreg_channel_joint_mask{};
  int64_t vicreg_encoder_call_count{0};
  int64_t vicreg_projector_call_count{0};
  torch::Tensor sample_valid_mask{};
  torch::Tensor channel_valid_mask{};
  std::map<std::string, torch::Tensor> diagnostics{};
};

struct mae_decoder_output_t {
  torch::Tensor projected{};
  torch::Tensor time{};
  torch::Tensor frequency{};
};

struct vicreg_stability_loss_options_t {
  double invariance_weight{25.0};
  double variance_weight{25.0};
  double covariance_weight{1.0};
  double variance_floor{1.0};
  double eps{1e-4};
};

struct vicreg_stability_loss_result_t {
  torch::Tensor loss{};
  torch::Tensor invariance_loss{};
  torch::Tensor variance_loss{};
  torch::Tensor covariance_loss{};
  int64_t valid_rows{0};
  int64_t active_groups{0};
};

struct tf_alignment_result_t {
  torch::Tensor loss{};
  int64_t pair_count{0};
  int64_t pair_valid_count{0};
};

struct vicreg_branch_loss_result_t {
  torch::Tensor loss{};
  torch::Tensor global_loss{};
  torch::Tensor channel_loss{};
  torch::Tensor invariance_loss{};
  torch::Tensor variance_loss{};
  torch::Tensor covariance_loss{};
  torch::Tensor global_invariance_loss{};
  torch::Tensor global_variance_loss{};
  torch::Tensor global_covariance_loss{};
  torch::Tensor channel_invariance_loss{};
  torch::Tensor channel_variance_loss{};
  torch::Tensor channel_covariance_loss{};
  torch::Tensor drawn_a_data{};
  torch::Tensor drawn_a_feature_mask{};
  torch::Tensor drawn_b_data{};
  torch::Tensor drawn_b_feature_mask{};
  torch::Tensor view_a_data{};
  torch::Tensor view_a_feature_mask{};
  torch::Tensor view_b_data{};
  torch::Tensor view_b_feature_mask{};
  torch::Tensor view_a_token_mask{};
  torch::Tensor view_b_token_mask{};
  torch::Tensor view_a_sample_valid_mask{};
  torch::Tensor view_b_sample_valid_mask{};
  torch::Tensor view_a_pooled_by_channel{};
  torch::Tensor view_b_pooled_by_channel{};
  torch::Tensor view_a_pooled_global{};
  torch::Tensor view_b_pooled_global{};
  torch::Tensor view_a_projected_global{};
  torch::Tensor view_b_projected_global{};
  torch::Tensor global_joint_mask{};
  torch::Tensor channel_joint_mask{};
  int64_t encoder_call_count{0};
  int64_t projector_call_count{0};
  int64_t global_valid_rows{0};
  int64_t channel_valid_rows{0};
  int64_t channel_active_groups{0};
  int64_t valid_rows{0};
};

namespace vicreg_stability_detail {

inline torch::Tensor valid_rows(const torch::Tensor &z,
                                const torch::Tensor &mask) {
  TORCH_CHECK(z.defined(), "[mtf_jepa_mae_vicreg] stability z is undefined");
  TORCH_CHECK(mask.defined(),
              "[mtf_jepa_mae_vicreg] stability mask is undefined");
  TORCH_CHECK(z.dim() == 3,
              "[mtf_jepa_mae_vicreg] stability z must be [B,C,D]");
  TORCH_CHECK(mask.dim() == 2,
              "[mtf_jepa_mae_vicreg] stability mask must be [B,C]");
  TORCH_CHECK(z.size(0) == mask.size(0) && z.size(1) == mask.size(1),
              "[mtf_jepa_mae_vicreg] stability z/mask shape mismatch");
  const auto rows = mask.to(torch::kBool).reshape({-1}).nonzero().reshape({-1});
  return z.reshape({-1, z.size(2)}).index_select(/*dim=*/0, rows);
}

inline torch::Tensor variance_term(const torch::Tensor &z, double floor,
                                   double eps) {
  const auto std = torch::sqrt(z.var(/*dim=*/0, /*unbiased=*/false) + eps);
  return torch::relu(floor - std).mean();
}

inline torch::Tensor covariance_term(const torch::Tensor &z) {
  const auto rows = z.size(0);
  const auto dim = z.size(1);
  if (rows <= 1 || dim <= 1) {
    return torch::zeros({}, z.options());
  }
  const auto centered = z - z.mean(/*dim=*/0, /*keepdim=*/true);
  const auto cov =
      centered.transpose(0, 1).matmul(centered) / static_cast<double>(rows - 1);
  const auto off_diag = cov - torch::diag(torch::diag(cov));
  return off_diag.pow(2).sum() / static_cast<double>(dim);
}

} // namespace vicreg_stability_detail

[[nodiscard]] inline vicreg_stability_loss_result_t
compute_vicreg_stability_loss(
    const torch::Tensor &z1, const torch::Tensor &mask1,
    const torch::Tensor &z2, const torch::Tensor &mask2,
    const vicreg_stability_loss_options_t &options = {}) {
  TORCH_CHECK(z1.sizes() == z2.sizes(),
              "[mtf_jepa_mae_vicreg] stability z1/z2 shape mismatch");
  TORCH_CHECK(mask1.sizes() == mask2.sizes(),
              "[mtf_jepa_mae_vicreg] stability mask1/mask2 shape mismatch");
  const auto joint_mask =
      mask1.to(torch::kBool).logical_and(mask2.to(torch::kBool));
  auto rows1 = vicreg_stability_detail::valid_rows(z1, joint_mask);
  auto rows2 = vicreg_stability_detail::valid_rows(z2, joint_mask);

  vicreg_stability_loss_result_t out{};
  out.valid_rows = std::min(rows1.size(0), rows2.size(0));
  out.active_groups = out.valid_rows > 0 ? 1 : 0;
  if (out.valid_rows <= 0) {
    out.loss = torch::zeros({}, z1.options());
    out.invariance_loss = torch::zeros({}, z1.options());
    out.variance_loss = torch::zeros({}, z1.options());
    out.covariance_loss = torch::zeros({}, z1.options());
    return out;
  }
  if (rows1.size(0) != out.valid_rows) {
    rows1 = rows1.narrow(0, 0, out.valid_rows);
  }
  if (rows2.size(0) != out.valid_rows) {
    rows2 = rows2.narrow(0, 0, out.valid_rows);
  }

  out.invariance_loss = torch::mse_loss(rows1, rows2);
  out.variance_loss = 0.5 * (vicreg_stability_detail::variance_term(
                                 rows1, options.variance_floor, options.eps) +
                             vicreg_stability_detail::variance_term(
                                 rows2, options.variance_floor, options.eps));
  out.covariance_loss = 0.5 * (vicreg_stability_detail::covariance_term(rows1) +
                               vicreg_stability_detail::covariance_term(rows2));
  out.loss = options.invariance_weight * out.invariance_loss +
             options.variance_weight * out.variance_loss +
             options.covariance_weight * out.covariance_loss;
  return out;
}

[[nodiscard]] inline vicreg_stability_loss_result_t
compute_channel_stratified_vicreg_stability_loss(
    const torch::Tensor &z1, const torch::Tensor &mask1,
    const torch::Tensor &z2, const torch::Tensor &mask2,
    const vicreg_stability_loss_options_t &options = {}) {
  TORCH_CHECK(z1.sizes() == z2.sizes(),
              "[mtf_jepa_mae_vicreg] stratified z1/z2 shape mismatch");
  TORCH_CHECK(mask1.sizes() == mask2.sizes(),
              "[mtf_jepa_mae_vicreg] stratified mask1/mask2 shape mismatch");
  TORCH_CHECK(z1.dim() == 3 && mask1.dim() == 2,
              "[mtf_jepa_mae_vicreg] stratified inputs must be [B,C,D] and "
              "[B,C]");
  TORCH_CHECK(z1.size(0) == mask1.size(0) && z1.size(1) == mask1.size(1),
              "[mtf_jepa_mae_vicreg] stratified z/mask shape mismatch");
  TORCH_CHECK(z1.size(1) > 0,
              "[mtf_jepa_mae_vicreg] stratified inputs need a channel");

  vicreg_stability_loss_result_t out{};
  out.loss = torch::zeros({}, z1.options());
  out.invariance_loss = torch::zeros({}, z1.options());
  out.variance_loss = torch::zeros({}, z1.options());
  out.covariance_loss = torch::zeros({}, z1.options());
  for (int64_t channel = 0; channel < z1.size(1); ++channel) {
    const auto channel_result = compute_vicreg_stability_loss(
        z1.narrow(/*dim=*/1, channel, /*length=*/1),
        mask1.narrow(/*dim=*/1, channel, /*length=*/1),
        z2.narrow(/*dim=*/1, channel, /*length=*/1),
        mask2.narrow(/*dim=*/1, channel, /*length=*/1), options);
    out.valid_rows += channel_result.valid_rows;
    TORCH_CHECK(channel_result.valid_rows >= 2,
                "[mtf_jepa_mae_vicreg] stratified channel ", channel,
                " needs at least two jointly valid rows; got ",
                channel_result.valid_rows);
    out.loss = out.loss + channel_result.loss;
    out.invariance_loss = out.invariance_loss + channel_result.invariance_loss;
    out.variance_loss = out.variance_loss + channel_result.variance_loss;
    out.covariance_loss = out.covariance_loss + channel_result.covariance_loss;
    ++out.active_groups;
  }
  const auto divisor = static_cast<double>(z1.size(1));
  out.loss = out.loss / divisor;
  out.invariance_loss = out.invariance_loss / divisor;
  out.variance_loss = out.variance_loss / divisor;
  out.covariance_loss = out.covariance_loss / divisor;
  return out;
}

namespace detail {

// structured_cdsb_v1 is the exact, frozen production form of the CDSB
// readout established by PSM-1 and reproduced by SRR-1.  It is deliberately
// limited to that proven representation surface.
inline constexpr int64_t kStructuredCdsbChannelCount = 3;
inline constexpr int64_t kStructuredCdsbHistoryLength = 30;
inline constexpr int64_t kStructuredCdsbInputWidth = 9;
inline constexpr int64_t kStructuredCdsbDomainCount = 2;
inline constexpr int64_t kStructuredCdsbScaleCount = 4;
inline constexpr int64_t kStructuredCdsbTokensPerChannel = 24;
inline constexpr int64_t kStructuredCdsbTokenCount = 72;
inline constexpr int64_t kStructuredCdsbLatentDim = 32;
inline constexpr int64_t kStructuredCdsbCellCount = 16;
inline constexpr int64_t kStructuredCdsbProjectionInputWidth = 768;
inline constexpr int64_t kStructuredCdsbProjectionOutputWidth = 32;

inline constexpr std::array<int64_t, kStructuredCdsbScaleCount>
    kStructuredCdsbTokensPerScale{7, 3, 1, 1};
inline constexpr std::array<int64_t, kStructuredCdsbScaleCount>
    kStructuredCdsbTimeScales{8, 16, 32, 64};
inline constexpr std::array<int64_t, kStructuredCdsbScaleCount>
    kStructuredCdsbScaleStrides{4, 8, 16, 32};
inline constexpr std::array<int64_t, kStructuredCdsbScaleCount>
    kStructuredCdsbScaleCellOffsets{0, 3, 6, 7};
inline constexpr std::array<int64_t, kStructuredCdsbTokensPerChannel>
    kStructuredCdsbFrozenCellIds{
        0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  6,  7,
        8,  8,  9,  9,  9,  10, 10, 11, 12, 13, 14, 15};
inline constexpr std::array<int64_t, kStructuredCdsbCellCount>
    kStructuredCdsbFrozenCellCounts{2, 3, 2, 1, 1, 1, 1, 1,
                                    2, 3, 2, 1, 1, 1, 1, 1};
inline constexpr int64_t kStructuredCdsbDomainScaleGroupCount = 8;
inline constexpr std::array<int64_t, kStructuredCdsbTokensPerChannel>
    kStructuredCdsbFrozenDomainScaleGroupIds{
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3,
        4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6, 7};
inline constexpr std::array<int64_t, kStructuredCdsbTokensPerChannel>
    kStructuredCdsbFrozenStarts{
        0, 4, 8, 12, 16, 20, 22, 0, 8, 14, 0, 0,
        0, 4, 8, 12, 16, 20, 22, 0, 8, 14, 0, 0};
inline constexpr std::array<int64_t, kStructuredCdsbTokensPerChannel>
    kStructuredCdsbFrozenWidths{
        8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 30, 30,
        8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 30, 30};

inline void validate_structured_cdsb_v1_config(
    const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(config.channel_count == kStructuredCdsbChannelCount,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 requires exactly "
              "3 channels");
  TORCH_CHECK(config.history_length == kStructuredCdsbHistoryLength &&
                  config.input_width == kStructuredCdsbInputWidth &&
                  config.d_model == kStructuredCdsbLatentDim &&
                  config.latent_dim == kStructuredCdsbLatentDim,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 requires the frozen "
              "H=30,F=9,d_model=32,latent_dim=32 configuration");
  TORCH_CHECK(config.use_frequency_tokens,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 requires time and "
              "frequency tokens");
  TORCH_CHECK(
      config.time_scales ==
              std::vector<int64_t>(kStructuredCdsbTimeScales.begin(),
                                   kStructuredCdsbTimeScales.end()) &&
          config.scale_strides ==
              std::vector<int64_t>(kStructuredCdsbScaleStrides.begin(),
                                   kStructuredCdsbScaleStrides.end()),
      "[mtf_jepa_mae_vicreg] structured_cdsb_v1 requires scales "
      "8,16,32,64 and strides 4,8,16,32");
}

inline void validate_probability(double value, const char *name) {
  if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
    throw std::runtime_error(std::string("[mtf_jepa_mae_vicreg] invalid ") +
                             name);
  }
}

inline void validate_drop_probability(double value, const char *name) {
  if (!std::isfinite(value) || value < 0.0 || value >= 1.0) {
    throw std::runtime_error(std::string("[mtf_jepa_mae_vicreg] invalid ") +
                             name);
  }
}

inline std::vector<int64_t>
resolved_scale_strides(const mtf_jepa_mae_vicreg_config_t &config) {
  if (config.scale_strides.empty()) {
    std::vector<int64_t> out;
    out.reserve(config.time_scales.size());
    for (const auto scale : config.time_scales) {
      out.push_back(std::max<int64_t>(1, scale / 2));
    }
    return out;
  }
  return config.scale_strides;
}

inline void
validate_architecture_config(const mtf_jepa_mae_vicreg_config_t &config) {
  if (config.channel_count <= 0 || config.history_length <= 0 ||
      config.input_width <= 0 || config.d_model <= 0 ||
      config.latent_dim <= 0 || config.projector_dim <= 0 ||
      config.predictor_hidden_dim <= 0 || config.num_encoder_layers < 0 ||
      config.num_predictor_layers < 1 || config.num_decoder_layers < 1 ||
      config.num_heads <= 0 || config.frequency_num_bins <= 0) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] invalid architecture dimensions");
  }
  if (config.latent_dim % config.num_heads != 0) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] latent_dim must be divisible by num_heads");
  }
  if (config.time_scales.empty()) {
    throw std::runtime_error("[mtf_jepa_mae_vicreg] time_scales is empty");
  }
  const auto strides = resolved_scale_strides(config);
  if (strides.size() != config.time_scales.size()) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] scale_strides/time_scales size mismatch");
  }
  for (std::size_t i = 0; i < config.time_scales.size(); ++i) {
    if (config.time_scales[i] <= 0 || strides[i] <= 0) {
      throw std::runtime_error(
          "[mtf_jepa_mae_vicreg] time scale and stride must be positive");
    }
  }
  if (!config.use_frequency_tokens &&
      (config.serving_pool_policy ==
           mtf_serving_pool_policy_t::frequency_only ||
       config.serving_pool_policy ==
           mtf_serving_pool_policy_t::domain_balanced ||
       config.serving_pool_policy ==
           mtf_serving_pool_policy_t::structured_cdsb_v1 ||
       config.serving_pool_policy ==
           mtf_serving_pool_policy_t::structured_cdsb_sparse_v1)) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] serving pool policy requires frequency "
        "tokens");
  }
  if (config.serving_pool_policy ==
          mtf_serving_pool_policy_t::structured_cdsb_v1 ||
      config.serving_pool_policy ==
          mtf_serving_pool_policy_t::structured_cdsb_sparse_v1) {
    validate_structured_cdsb_v1_config(config);
  }
}

inline void
validate_training_config(const mtf_jepa_mae_vicreg_config_t &config) {
  if (!std::isfinite(config.dropout) || config.dropout < 0.0 ||
      config.dropout >= 1.0) {
    throw std::runtime_error("[mtf_jepa_mae_vicreg] invalid dropout");
  }
  validate_probability(config.mask_ratio_time, "mask_ratio_time");
  validate_probability(config.mask_ratio_frequency, "mask_ratio_frequency");
  validate_probability(config.mask_ratio_channel, "mask_ratio_channel");
  validate_probability(config.min_context_ratio, "min_context_ratio");
  validate_probability(config.max_context_target_time_overlap,
                       "max_context_target_time_overlap");
  if (config.jepa_mask_policy != mtf_jepa_mask_policy_t::legacy_soft_overlap &&
      !config.use_frequency_tokens) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] paired-target JEPA masks require "
        "frequency tokens");
  }
  if (!std::isfinite(config.vicreg_view_gaussian_jitter_std) ||
      config.vicreg_view_gaussian_jitter_std < 0.0) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] invalid VICReg view Gaussian jitter");
  }
  validate_probability(config.vicreg_view_time_dropout_scale,
                       "vicreg_view_time_dropout_scale");
  (void)mtf_vicreg_view_pairing_policy_name(
      config.vicreg_view_pairing_policy);
  if (config.lambda_jepa < 0.0 || config.lambda_mae < 0.0 ||
      config.lambda_tf_align < 0.0 || config.lambda_vicreg < 0.0 ||
      config.lambda_global_vicreg < 0.0 || config.lambda_channel_vicreg < 0.0 ||
      config.vicreg_sim_weight < 0.0 || config.vicreg_var_weight < 0.0 ||
      config.vicreg_cov_weight < 0.0 || !(config.vicreg_variance_floor > 0.0) ||
      !(config.vicreg_variance_epsilon > 0.0) ||
      !(config.target_ema_tau >= 0.0 && config.target_ema_tau <= 1.0)) {
    throw std::runtime_error("[mtf_jepa_mae_vicreg] invalid scalar option");
  }
  if (config.augmentation_profile.empty()) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] augmentation_profile is required");
  }
  if (config.gaussian_jitter_std < 0.0 || config.time_crop_jitter_max < 0 ||
      config.time_dilation_min <= 0.0 || config.time_dilation_max <= 0.0 ||
      config.time_dilation_min > config.time_dilation_max ||
      config.amplitude_scale_min <= 0.0 || config.amplitude_scale_max <= 0.0 ||
      config.amplitude_scale_min > config.amplitude_scale_max ||
      config.amplitude_shift_std < 0.0 || config.frequency_jitter_std < 0.0 ||
      config.phase_jitter_max < 0.0 ||
      config.magnitude_normalization_noise_std < 0.0) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] invalid augmentation scalar option");
  }
  validate_drop_probability(config.feature_dropout_prob,
                            "feature_dropout_prob");
  validate_drop_probability(config.history_dropout_prob,
                            "history_dropout_prob");
  validate_probability(config.time_warp_max, "time_warp_max");
  validate_drop_probability(config.frequency_mask_ratio,
                            "frequency_mask_ratio");
  validate_drop_probability(config.channel_dropout_prob,
                            "channel_dropout_prob");
  validate_drop_probability(config.cross_channel_dropout_prob,
                            "cross_channel_dropout_prob");
  validate_drop_probability(config.node_dropout_prob, "node_dropout_prob");
  if (config.edge_dropout_prob != 0.0) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg] edge_dropout_prob is not supported by the "
        "current MTF channel-node input");
  }
}

inline void validate_config(const mtf_jepa_mae_vicreg_config_t &config) {
  validate_architecture_config(config);
  validate_training_config(config);
}

inline mtf_input_t
canonicalize_input(const torch::Tensor &x, const torch::Tensor &feature_mask,
                   const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(x.defined(), "[mtf_jepa_mae_vicreg] input is undefined");
  TORCH_CHECK(x.is_floating_point(),
              "[mtf_jepa_mae_vicreg] input must be floating point");
  torch::Tensor data = x;
  torch::Tensor mask = feature_mask;
  if (data.dim() == 3) {
    TORCH_CHECK(config.channel_count == 1,
                "[mtf_jepa_mae_vicreg] rank-3 input requires "
                "channel_count=1");
    data = data.unsqueeze(1);
    if (mask.defined()) {
      mask = mask.unsqueeze(1);
    }
  }
  TORCH_CHECK(data.dim() == 4,
              "[mtf_jepa_mae_vicreg] input must be [B,Hx,Dx] or "
              "[B,C,Hx,Dx]");
  TORCH_CHECK(data.size(1) == config.channel_count &&
                  data.size(2) == config.history_length &&
                  data.size(3) == config.input_width,
              "[mtf_jepa_mae_vicreg] input shape does not match config");
  data =
      data.to(torch::TensorOptions().dtype(config.dtype).device(config.device));
  if (mask.defined()) {
    TORCH_CHECK(mask.sizes() == data.sizes(),
                "[mtf_jepa_mae_vicreg] feature_mask shape mismatch");
    mask = mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config.device));
  } else {
    mask = torch::ones(
        data.sizes(),
        torch::TensorOptions().dtype(torch::kBool).device(config.device));
  }
  mask = mask.logical_and(torch::isfinite(data));
  data = torch::where(mask, data, torch::zeros_like(data));
  return {data, mask};
}

inline double resolved_vicreg_view_time_dropout_prob(
    const mtf_jepa_mae_vicreg_config_t &config) {
  return std::min(0.10,
                  std::max(0.0, config.mask_ratio_time *
                                    config.vicreg_view_time_dropout_scale));
}

inline mtf_input_t apply_vicreg_weak_view_augmentation(
    const torch::Tensor &x, const torch::Tensor &feature_mask,
    const mtf_jepa_mae_vicreg_config_t &config) {
  auto input = canonicalize_input(x, feature_mask, config);
  auto mask = input.feature_mask.clone();
  const double feature_drop =
      std::min(0.20, std::max(0.0, config.mask_ratio_channel));
  if (feature_drop > 0.0) {
    const auto keep = torch::rand(input.data.sizes(), input.data.options())
                          .lt(1.0 - feature_drop);
    mask = mask.logical_and(keep);
  }
  const double time_drop = resolved_vicreg_view_time_dropout_prob(config);
  // Preserve the legacy RNG schedule when the experiment disables only the
  // effect: MASK_RATIO_TIME still owns whether this draw exists.
  if (config.mask_ratio_time > 0.0) {
    auto keep_shape = input.data.sizes().vec();
    keep_shape.back() = 1;
    const auto keep = torch::rand(keep_shape, input.data.options())
                          .lt(1.0 - time_drop)
                          .expand_as(input.data);
    mask = mask.logical_and(keep);
  }
  auto data = torch::where(mask, input.data, torch::zeros_like(input.data));
  data = data + torch::where(mask,
                             torch::randn_like(data) *
                                 config.vicreg_view_gaussian_jitter_std,
                             torch::zeros_like(data));
  data = torch::where(mask, data, torch::zeros_like(data));
  return {data, mask};
}

inline std::vector<std::pair<int64_t, int64_t>>
window_plan(int64_t history_length, int64_t scale, int64_t stride) {
  const int64_t width = std::max<int64_t>(1, std::min(scale, history_length));
  std::vector<std::pair<int64_t, int64_t>> out;
  if (history_length <= width) {
    out.push_back({0, width});
    return out;
  }
  for (int64_t start = 0; start + width <= history_length; start += stride) {
    out.push_back({start, width});
    if (start + width == history_length) {
      break;
    }
  }
  const int64_t final_start = history_length - width;
  if (out.empty() || out.back().first != final_start) {
    out.push_back({final_start, width});
  }
  return out;
}

inline torch::Tensor masked_patch_descriptor(const torch::Tensor &patch,
                                             const torch::Tensor &valid,
                                             double eps = 1e-6) {
  TORCH_CHECK(patch.dim() == 3, "[mtf_jepa_mae_vicreg] patch must be [B,W,F]");
  const auto valid_f = valid.to(patch.dtype());
  const auto denom = valid_f.sum(/*dim=*/1).clamp_min(1.0); // [B,F]
  const auto mean = (patch * valid_f).sum(/*dim=*/1) / denom;
  const auto mean_sq = (patch.pow(2) * valid_f).sum(/*dim=*/1) / denom;
  const auto var = (mean_sq - mean.pow(2)).clamp_min(0.0);
  const auto std = torch::sqrt(var + eps);
  return torch::cat({mean, std}, /*dim=*/1);
}

inline torch::Tensor masked_mean(const torch::Tensor &x,
                                 const torch::Tensor &mask) {
  TORCH_CHECK(x.dim() == 3, "[mtf_jepa_mae_vicreg] x must be [B,N,D]");
  TORCH_CHECK(mask.dim() == 2, "[mtf_jepa_mae_vicreg] mask must be [B,N]");
  const auto mask_f = mask.to(x.dtype()).unsqueeze(-1);
  const auto denom = mask_f.sum(/*dim=*/1).clamp_min(1.0);
  return (x * mask_f).sum(/*dim=*/1) / denom;
}

inline torch::Tensor masked_mse(const torch::Tensor &a, const torch::Tensor &b,
                                const torch::Tensor &mask) {
  const auto mask_f = mask.to(a.dtype()).unsqueeze(-1);
  const auto denom =
      (mask_f.sum() * static_cast<double>(a.size(-1))).clamp_min(1.0);
  return ((a - b).pow(2) * mask_f).sum() / denom;
}

inline torch::Tensor masked_weighted_mse(const torch::Tensor &a,
                                         const torch::Tensor &b,
                                         const torch::Tensor &token_mask,
                                         const torch::Tensor &descriptor_mask) {
  TORCH_CHECK(a.sizes() == b.sizes(),
              "[mtf_jepa_mae_vicreg] weighted MSE tensor shape mismatch");
  TORCH_CHECK(token_mask.dim() == 2 && descriptor_mask.dim() == 3,
              "[mtf_jepa_mae_vicreg] weighted MSE mask rank mismatch");
  TORCH_CHECK(token_mask.size(0) == a.size(0) &&
                  token_mask.size(1) == a.size(1),
              "[mtf_jepa_mae_vicreg] weighted MSE token mask shape mismatch");
  TORCH_CHECK(descriptor_mask.sizes() == a.sizes(),
              "[mtf_jepa_mae_vicreg] weighted MSE descriptor mask mismatch");
  const auto weights = token_mask.to(torch::kBool)
                           .unsqueeze(-1)
                           .logical_and(descriptor_mask.to(torch::kBool))
                           .to(a.dtype());
  return ((a - b).pow(2) * weights).sum() / weights.sum().clamp_min(1.0);
}

inline torch::Tensor domain_mask(const mtf_token_metadata_t &metadata,
                                 const torch::Tensor &token_mask,
                                 int64_t domain) {
  auto domain_ids = metadata.domain_id.to(token_mask.device());
  return token_mask.logical_and(domain_ids.eq(domain).unsqueeze(0));
}

inline torch::Tensor
metadata_features(const mtf_token_metadata_t &metadata,
                  const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(metadata.start_index.defined(),
              "[mtf_jepa_mae_vicreg] metadata is undefined");
  const auto opts =
      torch::TensorOptions().dtype(config.dtype).device(config.device);
  const auto denom_t =
      static_cast<double>(std::max<int64_t>(1, config.history_length));
  const auto denom_scale =
      static_cast<double>(std::max<int64_t>(1, config.time_scales.size() - 1));
  const auto denom_channel =
      static_cast<double>(std::max<int64_t>(1, config.channel_count - 1));
  const auto start = metadata.start_index.to(opts) / denom_t;
  const auto center =
      (metadata.start_index.to(opts) + 0.5 * metadata.width.to(opts)) / denom_t;
  const auto width = metadata.width.to(opts) / denom_t;
  const auto scale = metadata.scale_id.to(opts) / denom_scale;
  const auto channel = metadata.channel_id.to(opts) / denom_channel;
  const auto domain = metadata.domain_id.to(opts);
  return torch::stack({start, center, width, scale, channel, domain},
                      /*dim=*/1);
}

inline torch::Tensor finite_or_zero(const torch::Tensor &value) {
  return torch::where(torch::isfinite(value), value, torch::zeros_like(value));
}

inline torch::Tensor checked_loss(const torch::Tensor &value, const char *name,
                                  bool strict) {
  if (strict) {
    TORCH_CHECK(torch::isfinite(value).all().template item<bool>(),
                "[mtf_jepa_mae_vicreg] non-finite loss: ", name);
    return value;
  }
  return finite_or_zero(value);
}

inline bool starts_with(const std::string &text, const std::string &prefix) {
  return text.rfind(prefix, 0) == 0;
}

inline torch::Tensor channel_mask(const mtf_token_metadata_t &metadata,
                                  const torch::Tensor &token_mask,
                                  int64_t channel) {
  auto channel_ids = metadata.channel_id.to(token_mask.device());
  return token_mask.logical_and(channel_ids.eq(channel).unsqueeze(0));
}

inline torch::Tensor channel_domain_mask(const mtf_token_metadata_t &metadata,
                                         const torch::Tensor &token_mask,
                                         int64_t channel, int64_t domain) {
  auto channel_ids = metadata.channel_id.to(token_mask.device());
  auto domain_ids = metadata.domain_id.to(token_mask.device());
  return token_mask.logical_and(channel_ids.eq(channel).unsqueeze(0))
      .logical_and(domain_ids.eq(domain).unsqueeze(0));
}

inline torch::Tensor
channel_valid_mask(const mtf_token_metadata_t &metadata,
                   const torch::Tensor &token_mask,
                   const mtf_jepa_mae_vicreg_config_t &config) {
  std::vector<torch::Tensor> masks;
  masks.reserve(static_cast<std::size_t>(config.channel_count));
  for (int64_t c = 0; c < config.channel_count; ++c) {
    masks.push_back(channel_mask(metadata, token_mask, c).any(/*dim=*/1));
  }
  return torch::stack(masks, /*dim=*/1);
}

inline torch::Tensor
pooled_by_channel(const torch::Tensor &embeddings, const torch::Tensor &mask,
                  const mtf_token_metadata_t &metadata,
                  const mtf_jepa_mae_vicreg_config_t &config) {
  std::vector<torch::Tensor> pooled;
  pooled.reserve(static_cast<std::size_t>(config.channel_count));
  for (int64_t c = 0; c < config.channel_count; ++c) {
    pooled.push_back(masked_mean(embeddings, channel_mask(metadata, mask, c)));
  }
  return torch::stack(pooled, /*dim=*/1);
}

struct channel_pool_t {
  torch::Tensor values{};     // [B,C,D]
  torch::Tensor valid_mask{}; // [B,C], bool
};

struct structured_cdsb_v1_metadata_record_t {
  int64_t source_index{0};
  int64_t domain{0};
  int64_t scale{0};
  int64_t start{0};
  int64_t width{0};
};

struct structured_cdsb_v1_plan_t {
  std::array<std::array<int64_t, kStructuredCdsbTokensPerChannel>,
             kStructuredCdsbChannelCount>
      ordered_token_indices{};
  std::array<std::array<int64_t, kStructuredCdsbTokensPerChannel>,
             kStructuredCdsbChannelCount>
      ordered_cell_ids{};
  std::array<std::array<int64_t, kStructuredCdsbCellCount>,
             kStructuredCdsbChannelCount>
      cell_counts{};
};

struct structured_cdsb_v1_lifted_t {
  torch::Tensor values{};     // [B,3,768], input dtype/device
  torch::Tensor valid_mask{}; // [B,3], bool, input device
};

struct structured_cdsb_sparse_v1_lifted_t {
  torch::Tensor values{}; // [B,3,768], domain-scale-neutral completion
  torch::Tensor valid_mask{};                  // [B,3], computable
  torch::Tensor complete_mask{};               // [B,3], all 24 tokens
  torch::Tensor grouped_token_mask{};          // [B,3,24], source support
  torch::Tensor cell_support_mask{};           // [B,3,16], any token
  torch::Tensor cell_source_token_count{};      // [B,3,16], int64
  torch::Tensor cell_expected_token_count{};    // [16], int64
  torch::Tensor domain_scale_support_mask{};    // [B,3,8], bool
  torch::Tensor repeated_support_position_count{}; // [B,3], int64
};

[[nodiscard]] inline uint64_t structured_cdsb_v1_splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] inline double structured_cdsb_v1_uniform01(uint64_t value) {
  return static_cast<double>(structured_cdsb_v1_splitmix64(value) >> 11U) *
         (1.0 / 9007199254740992.0);
}

[[nodiscard]] inline double structured_cdsb_v1_signed_uniform(
    uint64_t value) {
  return 2.0 * structured_cdsb_v1_uniform01(value) - 1.0;
}

inline void structured_cdsb_v1_mix_hash_value(uint64_t &hash,
                                               uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] inline uint64_t
structured_cdsb_v1_stable_tensor_hash(const torch::Tensor &input) {
  TORCH_CHECK(input.defined(),
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 cannot hash an "
              "undefined tensor");
  const auto contiguous = input.detach().to(torch::kCPU).contiguous();
  uint64_t hash = 0xcbf29ce484222325ULL;
  structured_cdsb_v1_mix_hash_value(
      hash, static_cast<uint64_t>(contiguous.scalar_type()));
  structured_cdsb_v1_mix_hash_value(hash,
                                     static_cast<uint64_t>(contiguous.dim()));
  for (const int64_t size : contiguous.sizes()) {
    structured_cdsb_v1_mix_hash_value(hash, static_cast<uint64_t>(size));
  }
  const auto byte_count = static_cast<std::size_t>(contiguous.numel()) *
                          static_cast<std::size_t>(contiguous.element_size());
  const auto *bytes = static_cast<const uint8_t *>(contiguous.data_ptr());
  for (std::size_t index = 0; index < byte_count; ++index) {
    structured_cdsb_v1_mix_hash_value(hash,
                                       static_cast<uint64_t>(bytes[index]));
  }
  return hash;
}

[[nodiscard]] inline torch::Tensor structured_cdsb_v1_make_q0_cpu64() {
  constexpr uint64_t kProjectionTag = 0x7273736d5f74655fULL;
  torch::NoGradGuard no_grad;
  auto dense = torch::empty(
      {kStructuredCdsbProjectionInputWidth,
       kStructuredCdsbProjectionOutputWidth},
      torch::kFloat64);
  auto values = dense.accessor<double, 2>();
  for (int64_t row = 0; row < dense.size(0); ++row) {
    for (int64_t column = 0; column < dense.size(1); ++column) {
      const uint64_t projection_key = structured_cdsb_v1_splitmix64(
          kProjectionTag ^
          structured_cdsb_v1_splitmix64(static_cast<uint64_t>(row)) ^
          structured_cdsb_v1_splitmix64(static_cast<uint64_t>(column) <<
                                         32U));
      values[row][column] =
          structured_cdsb_v1_signed_uniform(projection_key);
    }
  }
  auto [projection, upper] = at::linalg_qr(dense, "reduced");
  const auto diagonal = upper.diagonal();
  const auto signs = torch::where(diagonal.lt(0.0),
                                  -torch::ones_like(diagonal),
                                  torch::ones_like(diagonal));
  return (projection * signs.unsqueeze(0)).contiguous();
}

[[nodiscard]] inline torch::Tensor structured_cdsb_v1_mean_basis_cpu64() {
  auto basis = torch::zeros(
      {kStructuredCdsbProjectionInputWidth,
       kStructuredCdsbProjectionOutputWidth},
      torch::kFloat64);
  auto values = basis.accessor<double, 2>();
  const double scale = 1.0 / std::sqrt(kStructuredCdsbTokensPerChannel);
  for (int64_t token = 0; token < kStructuredCdsbTokensPerChannel; ++token) {
    for (int64_t feature = 0; feature < kStructuredCdsbLatentDim; ++feature) {
      values[token * kStructuredCdsbLatentDim + feature][feature] = scale;
    }
  }
  return basis;
}

[[nodiscard]] inline torch::Tensor structured_cdsb_v1_make_qpsm_cpu64(
    const torch::Tensor &q0) {
  torch::NoGradGuard no_grad;
  const auto basis = structured_cdsb_v1_mean_basis_cpu64();
  const auto contrast_seed =
      q0 - basis.matmul(basis.transpose(0, 1).matmul(q0));
  auto [contrast, upper] = at::linalg_qr(contrast_seed, "reduced");
  const auto diagonal = upper.diagonal();
  const auto signs = torch::where(diagonal.lt(0.0),
                                  -torch::ones_like(diagonal),
                                  torch::ones_like(diagonal));
  contrast = contrast * signs.unsqueeze(0);
  return (basis / std::sqrt(kStructuredCdsbTokensPerChannel) +
          std::sqrt(static_cast<double>(kStructuredCdsbTokensPerChannel - 1) /
                    static_cast<double>(kStructuredCdsbTokensPerChannel)) *
              contrast)
      .contiguous();
}

class structured_cdsb_v1_projection_thread_guard_t final {
public:
  structured_cdsb_v1_projection_thread_guard_t() {
#if AT_MKL_ENABLED()
    previous_mkl_threads_ = ::MKL_Set_Num_Threads_Local(1);
#else
    TORCH_CHECK(
        at::get_num_threads() == 1,
        "[mtf_jepa_mae_vicreg] structured_cdsb_v1 projection construction "
        "requires one CPU thread on non-MKL builds");
#endif
  }

  ~structured_cdsb_v1_projection_thread_guard_t() {
#if AT_MKL_ENABLED()
    (void)::MKL_Set_Num_Threads_Local(previous_mkl_threads_);
#endif
  }

  structured_cdsb_v1_projection_thread_guard_t(
      const structured_cdsb_v1_projection_thread_guard_t &) = delete;
  structured_cdsb_v1_projection_thread_guard_t &operator=(
      const structured_cdsb_v1_projection_thread_guard_t &) = delete;

private:
#if AT_MKL_ENABLED()
  int previous_mkl_threads_{0};
#endif
};

[[nodiscard]] inline torch::Tensor structured_cdsb_v1_projection_for(
    const torch::TensorOptions &options) {
  static const torch::Tensor projection = [] {
    torch::NoGradGuard no_grad;
    structured_cdsb_v1_projection_thread_guard_t thread_guard;
    const auto q0 = structured_cdsb_v1_make_q0_cpu64();
    TORCH_CHECK(
        q0.sizes() ==
                torch::IntArrayRef({kStructuredCdsbProjectionInputWidth,
                                    kStructuredCdsbProjectionOutputWidth}) &&
            q0.scalar_type() == torch::kFloat64 && q0.device().is_cpu() &&
            q0.is_contiguous() && !q0.requires_grad() &&
            torch::isfinite(q0).all().item<bool>(),
        "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_0 tensor contract "
        "failed");
    TORCH_CHECK(structured_cdsb_v1_stable_tensor_hash(q0) ==
                    0xf8c9f35282de2ee0ULL,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_0 hash "
                "mismatch");

    const auto qpsm = structured_cdsb_v1_make_qpsm_cpu64(q0);
    TORCH_CHECK(
        qpsm.sizes() ==
                torch::IntArrayRef({kStructuredCdsbProjectionInputWidth,
                                    kStructuredCdsbProjectionOutputWidth}) &&
            qpsm.scalar_type() == torch::kFloat64 &&
            qpsm.device().is_cpu() && qpsm.is_contiguous() &&
            !qpsm.requires_grad() &&
            torch::isfinite(qpsm).all().item<bool>(),
        "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_psm tensor contract "
        "failed");
    TORCH_CHECK(structured_cdsb_v1_stable_tensor_hash(qpsm) ==
                    0xac8a43fd65b2c8a8ULL,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_psm hash "
                "mismatch");

    const auto identity =
        torch::eye(kStructuredCdsbProjectionOutputWidth, torch::kFloat64);
    const auto basis = structured_cdsb_v1_mean_basis_cpu64();
    const auto contrast =
        (qpsm - basis / std::sqrt(kStructuredCdsbTokensPerChannel)) /
        std::sqrt(
            static_cast<double>(kStructuredCdsbTokensPerChannel - 1) /
            static_cast<double>(kStructuredCdsbTokensPerChannel));
    const double orthogonality_error =
        (qpsm.transpose(0, 1).matmul(qpsm) - identity)
            .abs()
            .max()
            .item<double>();
    const double contrast_mean_error =
        basis.transpose(0, 1).matmul(contrast).abs().max().item<double>();
    const double block_sum_error =
        (qpsm.reshape({kStructuredCdsbTokensPerChannel,
                       kStructuredCdsbLatentDim,
                       kStructuredCdsbProjectionOutputWidth})
             .sum(0) -
         identity)
            .abs()
            .max()
            .item<double>();
    TORCH_CHECK(orthogonality_error <= 1.0e-10,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_psm "
                "orthogonality contract failed");
    TORCH_CHECK(contrast_mean_error <= 1.0e-10,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_psm mean "
                "contrast contract failed");
    TORCH_CHECK(block_sum_error <= 1.0e-10,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 Q_psm block-sum "
                "contract failed");
    return qpsm;
  }();
  // Always return fresh storage. A torch::Tensor handle is mutable even when
  // obtained from a const reference, so exposing the cached handle would let
  // an unrelated detail caller corrupt the versioned projection globally.
  // This remains the single options translation performed by each readout.
  return projection.to(options, /*non_blocking=*/false, /*copy=*/true)
      .contiguous();
}

[[nodiscard]] inline torch::Tensor structured_cdsb_v1_metadata_vector_cpu(
    const torch::Tensor &value, const char *name) {
  TORCH_CHECK(value.defined(),
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 missing metadata "
              "field ",
              name);
  TORCH_CHECK(value.dim() == 1 &&
                  value.numel() == kStructuredCdsbTokenCount,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 metadata field ",
              name, " must be [72]");
  TORCH_CHECK(value.scalar_type() == torch::kInt64,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 metadata field ",
              name, " must be int64");
  return value.to(torch::kCPU, torch::kInt64).contiguous();
}

[[nodiscard]] inline int64_t structured_cdsb_v1_compact_cell_id(
    int64_t domain, int64_t scale, int64_t rank, int64_t group_size) {
  const int64_t bin = std::min<int64_t>(
      2, (3 * (2 * rank + 1)) / (2 * group_size));
  const int64_t within_domain =
      scale < 2
          ? kStructuredCdsbScaleCellOffsets[static_cast<std::size_t>(scale)] +
                bin
          : kStructuredCdsbScaleCellOffsets[static_cast<std::size_t>(scale)];
  return domain * 8 + within_domain;
}

inline void structured_cdsb_v1_validate_encoded(
    const mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf_jepa_mae_vicreg_config_t &config) {
  validate_structured_cdsb_v1_config(config);
  TORCH_CHECK(encoded.embeddings.defined() &&
                  encoded.embeddings.dim() == 3 &&
                  encoded.embeddings.size(1) == kStructuredCdsbTokenCount &&
                  encoded.embeddings.size(2) == kStructuredCdsbLatentDim,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 embeddings must be "
              "[B,72,32]");
  TORCH_CHECK(encoded.embeddings.is_floating_point(),
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 embeddings must be "
              "floating point");
  TORCH_CHECK(torch::isfinite(encoded.embeddings).all().item<bool>(),
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 embeddings must be "
              "finite");
  TORCH_CHECK(encoded.token_mask.defined() &&
                  encoded.token_mask.dim() == 2 &&
                  encoded.token_mask.size(0) == encoded.embeddings.size(0) &&
                  encoded.token_mask.size(1) == kStructuredCdsbTokenCount &&
                  encoded.token_mask.scalar_type() == torch::kBool,
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 token_mask must be "
              "bool [B,72]");
  TORCH_CHECK(encoded.token_mask.device() == encoded.embeddings.device(),
              "[mtf_jepa_mae_vicreg] structured_cdsb_v1 token_mask and "
              "embeddings must share a device");
  TORCH_CHECK(
      encoded.sample_valid_mask.defined() &&
          encoded.sample_valid_mask.dim() == 1 &&
          encoded.sample_valid_mask.size(0) == encoded.embeddings.size(0) &&
          encoded.sample_valid_mask.scalar_type() == torch::kBool &&
          encoded.sample_valid_mask.device() == encoded.embeddings.device(),
      "[mtf_jepa_mae_vicreg] structured_cdsb_v1 sample_valid_mask must be "
      "bool [B] on the input device");
  TORCH_CHECK(
      encoded.channel_valid_mask.defined() &&
          encoded.channel_valid_mask.dim() == 2 &&
          encoded.channel_valid_mask.size(0) == encoded.embeddings.size(0) &&
          encoded.channel_valid_mask.size(1) ==
              kStructuredCdsbChannelCount &&
          encoded.channel_valid_mask.scalar_type() == torch::kBool &&
          encoded.channel_valid_mask.device() == encoded.embeddings.device(),
      "[mtf_jepa_mae_vicreg] structured_cdsb_v1 channel_valid_mask must be "
      "bool [B,3] on the input device");
}

[[nodiscard]] inline structured_cdsb_v1_plan_t
structured_cdsb_v1_build_plan(const mtf_token_metadata_t &metadata) {
  const auto channels = structured_cdsb_v1_metadata_vector_cpu(
      metadata.channel_id, "channel_id");
  const auto domains = structured_cdsb_v1_metadata_vector_cpu(
      metadata.domain_id, "domain_id");
  const auto scales = structured_cdsb_v1_metadata_vector_cpu(
      metadata.scale_id, "scale_id");
  const auto starts = structured_cdsb_v1_metadata_vector_cpu(
      metadata.start_index, "start_index");
  const auto widths =
      structured_cdsb_v1_metadata_vector_cpu(metadata.width, "width");
  const auto channel = channels.accessor<int64_t, 1>();
  const auto domain = domains.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();

  std::array<std::vector<structured_cdsb_v1_metadata_record_t>,
             kStructuredCdsbChannelCount>
      records{};
  for (int64_t token = 0; token < kStructuredCdsbTokenCount; ++token) {
    TORCH_CHECK(channel[token] >= 0 &&
                    channel[token] < kStructuredCdsbChannelCount,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 channel_id is "
                "outside [0,2]");
    TORCH_CHECK(domain[token] >= 0 &&
                    domain[token] < kStructuredCdsbDomainCount,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 domain_id is "
                "outside [0,1]");
    TORCH_CHECK(scale[token] >= 0 &&
                    scale[token] < kStructuredCdsbScaleCount,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 scale_id is "
                "outside [0,3]");
    TORCH_CHECK(start[token] >= 0 && width[token] > 0,
                "[mtf_jepa_mae_vicreg] structured_cdsb_v1 token start/width "
                "metadata is invalid");
    records[static_cast<std::size_t>(channel[token])].push_back(
        {.source_index = token,
         .domain = domain[token],
         .scale = scale[token],
         .start = start[token],
         .width = width[token]});
  }

  structured_cdsb_v1_plan_t plan{};
  std::array<std::array<int64_t, 4>, kStructuredCdsbTokensPerChannel>
      reference_layout{};
  for (int64_t channel_id = 0; channel_id < kStructuredCdsbChannelCount;
       ++channel_id) {
    auto &channel_records = records[static_cast<std::size_t>(channel_id)];
    TORCH_CHECK(
        channel_records.size() ==
            static_cast<std::size_t>(kStructuredCdsbTokensPerChannel),
        "[mtf_jepa_mae_vicreg] structured_cdsb_v1 each channel must contain "
        "exactly 24 tokens");
    std::sort(channel_records.begin(), channel_records.end(),
              [](const auto &left, const auto &right) {
                return std::tuple{left.domain, left.scale, left.start,
                                  left.width, left.source_index} <
                       std::tuple{right.domain, right.scale, right.start,
                                  right.width, right.source_index};
              });

    std::array<std::array<int64_t, kStructuredCdsbScaleCount>,
               kStructuredCdsbDomainCount>
        counts{};
    for (std::size_t position = 0; position < channel_records.size();
         ++position) {
      const auto &record = channel_records[position];
      if (position > 0) {
        const auto &previous = channel_records[position - 1];
        const bool unique_key =
            std::tuple{record.domain, record.scale, record.start,
                       record.width} !=
            std::tuple{previous.domain, previous.scale, previous.start,
                       previous.width};
        TORCH_CHECK(
            unique_key,
            "[mtf_jepa_mae_vicreg] structured_cdsb_v1 duplicate metadata "
            "makes rank bins ambiguous");
      }
      const std::array<int64_t, 4> layout_key{
          record.domain, record.scale, record.start, record.width};
      TORCH_CHECK(
          record.start == kStructuredCdsbFrozenStarts[position] &&
              record.width == kStructuredCdsbFrozenWidths[position],
          "[mtf_jepa_mae_vicreg] structured_cdsb_v1 start/width layout does "
          "not match the frozen H=30 plan");
      if (channel_id == 0) {
        reference_layout[position] = layout_key;
      } else {
        TORCH_CHECK(
            layout_key == reference_layout[position],
            "[mtf_jepa_mae_vicreg] structured_cdsb_v1 channels must share "
            "one metadata layout");
      }
      ++counts[static_cast<std::size_t>(record.domain)]
              [static_cast<std::size_t>(record.scale)];
    }
    for (int64_t domain_id = 0; domain_id < kStructuredCdsbDomainCount;
         ++domain_id) {
      for (int64_t scale_id = 0; scale_id < kStructuredCdsbScaleCount;
           ++scale_id) {
        TORCH_CHECK(
            counts[static_cast<std::size_t>(domain_id)]
                  [static_cast<std::size_t>(scale_id)] ==
                kStructuredCdsbTokensPerScale[
                    static_cast<std::size_t>(scale_id)],
            "[mtf_jepa_mae_vicreg] structured_cdsb_v1 each domain must have "
            "per-scale counts 7,3,1,1");
      }
    }

    std::array<int64_t, kStructuredCdsbCellCount> cell_counts{};
    for (int64_t position = 0; position < kStructuredCdsbTokensPerChannel;
         ++position) {
      const auto &record = channel_records[static_cast<std::size_t>(position)];
      int64_t group_begin = position;
      while (group_begin > 0) {
        const auto &candidate =
            channel_records[static_cast<std::size_t>(group_begin - 1)];
        if (candidate.domain != record.domain ||
            candidate.scale != record.scale) {
          break;
        }
        --group_begin;
      }
      const int64_t rank = position - group_begin;
      const int64_t group_size =
          counts[static_cast<std::size_t>(record.domain)]
                [static_cast<std::size_t>(record.scale)];
      const int64_t cell = structured_cdsb_v1_compact_cell_id(
          record.domain, record.scale, rank, group_size);
      TORCH_CHECK(
          cell ==
              kStructuredCdsbFrozenCellIds[static_cast<std::size_t>(position)],
          "[mtf_jepa_mae_vicreg] structured_cdsb_v1 metadata-derived cells "
          "do not match the frozen CDSB plan");
      plan.ordered_token_indices[static_cast<std::size_t>(channel_id)]
                                [static_cast<std::size_t>(position)] =
          record.source_index;
      plan.ordered_cell_ids[static_cast<std::size_t>(channel_id)]
                           [static_cast<std::size_t>(position)] = cell;
      ++cell_counts[static_cast<std::size_t>(cell)];
    }
    TORCH_CHECK(
        cell_counts == kStructuredCdsbFrozenCellCounts,
        "[mtf_jepa_mae_vicreg] structured_cdsb_v1 compact cells have "
        "unexpected cardinalities");
    plan.cell_counts[static_cast<std::size_t>(channel_id)] = cell_counts;
  }
  return plan;
}

[[nodiscard]] inline structured_cdsb_v1_lifted_t
structured_cdsb_v1_lift(const mtf_jepa_mae_vicreg_encode_output_t &encoded,
                        const mtf_jepa_mae_vicreg_config_t &config) {
  structured_cdsb_v1_validate_encoded(encoded, config);
  const auto plan = structured_cdsb_v1_build_plan(encoded.metadata);
  const auto device = encoded.embeddings.device();
  std::vector<torch::Tensor> grouped_channels;
  std::vector<torch::Tensor> grouped_masks;
  grouped_channels.reserve(kStructuredCdsbChannelCount);
  grouped_masks.reserve(kStructuredCdsbChannelCount);
  for (int64_t channel = 0; channel < kStructuredCdsbChannelCount; ++channel) {
    const auto &source =
        plan.ordered_token_indices[static_cast<std::size_t>(channel)];
    const std::vector<int64_t> indices(source.begin(), source.end());
    const auto index = torch::tensor(
        indices, torch::TensorOptions().dtype(torch::kInt64).device(device));
    grouped_channels.push_back(encoded.embeddings.index_select(1, index));
    grouped_masks.push_back(encoded.token_mask.index_select(1, index));
  }
  const auto grouped = torch::stack(grouped_channels, 1); // [B,3,24,32]
  const auto grouped_mask = torch::stack(grouped_masks, 1); // [B,3,24]

  std::vector<torch::Tensor> cell_means;
  cell_means.reserve(kStructuredCdsbCellCount);
  for (int64_t cell = 0; cell < kStructuredCdsbCellCount; ++cell) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < kStructuredCdsbTokensPerChannel;
         ++position) {
      if (kStructuredCdsbFrozenCellIds[static_cast<std::size_t>(position)] ==
          cell) {
        positions.push_back(position);
      }
    }
    const auto index = torch::tensor(
        positions, torch::TensorOptions().dtype(torch::kInt64).device(device));
    const auto selected = grouped.index_select(2, index);
    const auto selected_mask = grouped_mask.index_select(2, index);
    const auto all_valid = selected_mask.all(2);
    const auto raw_mean = selected.mean(2);
    const auto weight = selected_mask.to(selected.scalar_type()).unsqueeze(-1);
    const auto masked_mean =
        (selected * weight).sum(2) / weight.sum(2).clamp_min(1.0);
    cell_means.push_back(
        torch::where(all_valid.unsqueeze(-1), raw_mean, masked_mean));
  }

  // The frozen scientific contract covers complete channel blocks only.
  // Partial blocks fail closed and are exactly zeroed per channel.
  auto valid_mask = grouped_mask.all(2);
  valid_mask = valid_mask.logical_and(encoded.sample_valid_mask.unsqueeze(1));
  valid_mask = valid_mask.logical_and(encoded.channel_valid_mask);
  std::vector<torch::Tensor> lifted_positions;
  lifted_positions.reserve(kStructuredCdsbTokensPerChannel);
  for (const int64_t cell : kStructuredCdsbFrozenCellIds) {
    lifted_positions.push_back(cell_means[static_cast<std::size_t>(cell)]);
  }
  auto values =
      torch::stack(lifted_positions, 2)
          .reshape({encoded.embeddings.size(0), kStructuredCdsbChannelCount,
                    kStructuredCdsbProjectionInputWidth})
          .contiguous();
  values = torch::where(valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  return {.values = std::move(values),
          .valid_mask = std::move(valid_mask)};
}

[[nodiscard]] inline channel_pool_t structured_cdsb_v1_readout(
    const mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf_jepa_mae_vicreg_config_t &config) {
  auto lifted = structured_cdsb_v1_lift(encoded, config);
  const auto fixed_projection =
      structured_cdsb_v1_projection_for(encoded.embeddings.options());
  auto values = lifted.values.matmul(fixed_projection).contiguous();
  values = torch::where(lifted.valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  return {.values = std::move(values),
          .valid_mask = std::move(lifted.valid_mask)};
}

// Sparse-mask repair of the frozen CDSB contract.  V1 remains the authority
// for complete blocks.  Sparse cells are estimated only from tokenizer-valid
// source tokens; missing positions receive the neutral mean of their own
// (domain, scale) group before the unchanged Q_psm projection is applied.
[[nodiscard]] inline structured_cdsb_sparse_v1_lifted_t
structured_cdsb_sparse_v1_lift(
    const mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf_jepa_mae_vicreg_config_t &config) {
  structured_cdsb_v1_validate_encoded(encoded, config);
  const auto plan = structured_cdsb_v1_build_plan(encoded.metadata);
  const auto device = encoded.embeddings.device();
  const auto index_options =
      torch::TensorOptions().dtype(torch::kInt64).device(device);

  std::vector<torch::Tensor> grouped_channels;
  std::vector<torch::Tensor> grouped_masks;
  grouped_channels.reserve(kStructuredCdsbChannelCount);
  grouped_masks.reserve(kStructuredCdsbChannelCount);
  for (int64_t channel = 0; channel < kStructuredCdsbChannelCount; ++channel) {
    const auto &source =
        plan.ordered_token_indices[static_cast<std::size_t>(channel)];
    const std::vector<int64_t> indices(source.begin(), source.end());
    const auto index = torch::tensor(indices, index_options);
    grouped_channels.push_back(encoded.embeddings.index_select(1, index));
    grouped_masks.push_back(encoded.token_mask.index_select(1, index));
  }
  const auto grouped = torch::stack(grouped_channels, 1); // [B,3,24,32]
  const auto grouped_mask =
      torch::stack(grouped_masks, 1).contiguous(); // [B,3,24]

  std::vector<torch::Tensor> cell_means;
  std::vector<torch::Tensor> cell_support;
  std::vector<torch::Tensor> cell_source_counts;
  cell_means.reserve(kStructuredCdsbCellCount);
  cell_support.reserve(kStructuredCdsbCellCount);
  cell_source_counts.reserve(kStructuredCdsbCellCount);
  for (int64_t cell = 0; cell < kStructuredCdsbCellCount; ++cell) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < kStructuredCdsbTokensPerChannel;
         ++position) {
      if (kStructuredCdsbFrozenCellIds[static_cast<std::size_t>(position)] ==
          cell) {
        positions.push_back(position);
      }
    }
    const auto index = torch::tensor(positions, index_options);
    const auto selected = grouped.index_select(2, index);
    const auto selected_mask = grouped_mask.index_select(2, index);
    const auto source_count = selected_mask.sum(2);
    const auto any_valid = source_count.gt(0);
    const auto all_valid =
        source_count.eq(static_cast<int64_t>(positions.size()));
    const auto raw_mean = selected.mean(2);
    const auto weight = selected_mask.to(selected.scalar_type()).unsqueeze(-1);
    const auto masked_mean =
        (selected * weight).sum(2) / weight.sum(2).clamp_min(1.0);
    auto mean =
        torch::where(all_valid.unsqueeze(-1), raw_mean, masked_mean);
    mean = torch::where(any_valid.unsqueeze(-1), mean,
                        torch::zeros_like(mean));
    cell_means.push_back(std::move(mean));
    cell_support.push_back(any_valid);
    cell_source_counts.push_back(source_count);
  }
  const auto cell_mean = torch::stack(cell_means, 2); // [B,3,16,32]
  const auto cell_support_mask =
      torch::stack(cell_support, 2).contiguous(); // [B,3,16]
  const auto cell_source_token_count =
      torch::stack(cell_source_counts, 2).contiguous(); // [B,3,16]

  std::vector<torch::Tensor> lifted_positions;
  std::vector<torch::Tensor> lifted_support;
  lifted_positions.reserve(kStructuredCdsbTokensPerChannel);
  lifted_support.reserve(kStructuredCdsbTokensPerChannel);
  for (const int64_t cell : kStructuredCdsbFrozenCellIds) {
    lifted_positions.push_back(cell_mean.select(2, cell));
    lifted_support.push_back(cell_support_mask.select(2, cell));
  }
  const auto position_values =
      torch::stack(lifted_positions, 2); // [B,3,24,32]
  const auto position_support =
      torch::stack(lifted_support, 2).contiguous(); // [B,3,24]

  std::vector<torch::Tensor> group_means;
  std::vector<torch::Tensor> group_support;
  group_means.reserve(kStructuredCdsbDomainScaleGroupCount);
  group_support.reserve(kStructuredCdsbDomainScaleGroupCount);
  for (int64_t group = 0; group < kStructuredCdsbDomainScaleGroupCount;
       ++group) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < kStructuredCdsbTokensPerChannel;
         ++position) {
      if (kStructuredCdsbFrozenDomainScaleGroupIds[
              static_cast<std::size_t>(position)] == group) {
        positions.push_back(position);
      }
    }
    const auto index = torch::tensor(positions, index_options);
    const auto selected = position_values.index_select(2, index);
    const auto selected_support = position_support.index_select(2, index);
    const auto weight =
        selected_support.to(selected.scalar_type()).unsqueeze(-1);
    const auto support_count = selected_support.sum(2);
    const auto supported = support_count.gt(0);
    auto mean =
        (selected * weight).sum(2) / weight.sum(2).clamp_min(1.0);
    mean = torch::where(supported.unsqueeze(-1), mean,
                        torch::zeros_like(mean));
    group_means.push_back(std::move(mean));
    group_support.push_back(supported);
  }
  const auto domain_scale_support_mask =
      torch::stack(group_support, 2).contiguous(); // [B,3,8]

  std::vector<torch::Tensor> completed_positions;
  completed_positions.reserve(kStructuredCdsbTokensPerChannel);
  for (int64_t position = 0; position < kStructuredCdsbTokensPerChannel;
       ++position) {
    const int64_t group = kStructuredCdsbFrozenDomainScaleGroupIds[
        static_cast<std::size_t>(position)];
    completed_positions.push_back(torch::where(
        position_support.select(2, position).unsqueeze(-1),
        position_values.select(2, position),
        group_means[static_cast<std::size_t>(group)]));
  }

  auto valid_mask = domain_scale_support_mask.all(2);
  valid_mask = valid_mask.logical_and(encoded.sample_valid_mask.unsqueeze(1));
  valid_mask = valid_mask.logical_and(encoded.channel_valid_mask).contiguous();
  auto complete_mask = grouped_mask.all(2);
  complete_mask =
      complete_mask.logical_and(encoded.sample_valid_mask.unsqueeze(1));
  complete_mask =
      complete_mask.logical_and(encoded.channel_valid_mask).contiguous();
  auto values =
      torch::stack(completed_positions, 2)
          .reshape({encoded.embeddings.size(0), kStructuredCdsbChannelCount,
                    kStructuredCdsbProjectionInputWidth})
          .contiguous();
  values = torch::where(valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  const std::vector<int64_t> expected_counts(
      kStructuredCdsbFrozenCellCounts.begin(),
      kStructuredCdsbFrozenCellCounts.end());
  auto cell_expected_token_count =
      torch::tensor(expected_counts, index_options).contiguous();
  auto repeated_support_position_count =
      position_support.sum(2).contiguous();
  return {
      .values = std::move(values),
      .valid_mask = std::move(valid_mask),
      .complete_mask = std::move(complete_mask),
      .grouped_token_mask = grouped_mask,
      .cell_support_mask = cell_support_mask,
      .cell_source_token_count = cell_source_token_count,
      .cell_expected_token_count = std::move(cell_expected_token_count),
      .domain_scale_support_mask = domain_scale_support_mask,
      .repeated_support_position_count =
          std::move(repeated_support_position_count),
  };
}

[[nodiscard]] inline channel_pool_t structured_cdsb_sparse_v1_readout(
    const mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf_jepa_mae_vicreg_config_t &config) {
  auto lifted = structured_cdsb_sparse_v1_lift(encoded, config);
  const auto fixed_projection =
      structured_cdsb_v1_projection_for(encoded.embeddings.options());
  auto values = lifted.values.matmul(fixed_projection).contiguous();

  // The complete-row branch is deliberately the literal frozen v1 readout,
  // so mixed complete/sparse batches preserve v1 bytes, not merely algebra.
  const auto complete = structured_cdsb_v1_readout(encoded, config);
  values = torch::where(lifted.complete_mask.unsqueeze(-1), complete.values,
                        values);
  values = torch::where(lifted.valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  return {.values = std::move(values),
          .valid_mask = std::move(lifted.valid_mask)};
}

inline channel_pool_t pooled_by_channel_domain(
    const torch::Tensor &embeddings, const torch::Tensor &mask,
    const mtf_token_metadata_t &metadata,
    const mtf_jepa_mae_vicreg_config_t &config, int64_t domain) {
  std::vector<torch::Tensor> pooled;
  std::vector<torch::Tensor> valid;
  pooled.reserve(static_cast<std::size_t>(config.channel_count));
  valid.reserve(static_cast<std::size_t>(config.channel_count));
  for (int64_t c = 0; c < config.channel_count; ++c) {
    auto selected = channel_domain_mask(metadata, mask, c, domain);
    pooled.push_back(masked_mean(embeddings, selected));
    valid.push_back(selected.any(/*dim=*/1));
  }
  return {.values = torch::stack(pooled, /*dim=*/1),
          .valid_mask = torch::stack(valid, /*dim=*/1)};
}

inline torch::Tensor valid_token_rows(const torch::Tensor &latents,
                                      const torch::Tensor &token_mask) {
  TORCH_CHECK(latents.dim() == 3,
              "[mtf_jepa_mae_vicreg] latent diagnostics need [B,N,D]");
  TORCH_CHECK(token_mask.dim() == 2,
              "[mtf_jepa_mae_vicreg] latent diagnostics need [B,N] mask");
  const auto rows =
      token_mask.to(torch::kBool).reshape({-1}).nonzero().reshape({-1});
  return latents.reshape({-1, latents.size(-1)}).index_select(/*dim=*/0, rows);
}

inline torch::Tensor multi_head_context_attention(
    const torch::Tensor &q, const torch::Tensor &k, const torch::Tensor &v,
    const torch::Tensor &context_mask, int64_t num_heads) {
  TORCH_CHECK(q.dim() == 3 && k.dim() == 3 && v.dim() == 3,
              "[mtf_jepa_mae_vicreg] attention tensors must be [B,N,D]");
  TORCH_CHECK(q.size(0) == k.size(0) && k.sizes() == v.sizes() &&
                  q.size(2) == k.size(2),
              "[mtf_jepa_mae_vicreg] attention tensor shape mismatch");
  TORCH_CHECK(context_mask.size(0) == k.size(0) &&
                  context_mask.size(1) == k.size(1),
              "[mtf_jepa_mae_vicreg] attention mask shape mismatch");
  const int64_t B = q.size(0);
  const int64_t Q = q.size(1);
  const int64_t K = k.size(1);
  const int64_t D = q.size(2);
  TORCH_CHECK(num_heads > 0 && D % num_heads == 0,
              "[mtf_jepa_mae_vicreg] latent_dim must divide num_heads");
  const int64_t head_dim = D / num_heads;
  auto qh =
      q.contiguous().view({B, Q, num_heads, head_dim}).permute({0, 2, 1, 3});
  auto kh =
      k.contiguous().view({B, K, num_heads, head_dim}).permute({0, 2, 1, 3});
  auto vh =
      v.contiguous().view({B, K, num_heads, head_dim}).permute({0, 2, 1, 3});
  auto scores = torch::matmul(qh, kh.transpose(-2, -1)) /
                std::sqrt(static_cast<double>(head_dim));
  const auto mask =
      context_mask.to(torch::kBool).view({B, 1, 1, K}).to(scores.device());
  const auto mask_f = mask.to(scores.dtype());
  scores = scores.masked_fill(mask.logical_not(), -1.0e9);
  auto weights = torch::softmax(scores, /*dim=*/-1) * mask_f;
  weights = weights / weights.sum(/*dim=*/-1, /*keepdim=*/true).clamp_min(1e-6);
  auto attended = torch::matmul(weights, vh);
  const auto has_context = context_mask.to(torch::kBool)
                               .any(/*dim=*/1)
                               .to(scores.dtype())
                               .view({B, 1, 1, 1})
                               .to(scores.device());
  attended = attended * has_context;
  return attended.permute({0, 2, 1, 3}).contiguous().view({B, Q, D});
}

} // namespace detail

using mtf_serving_pool_output_t = detail::channel_pool_t;

[[nodiscard]] inline mtf_serving_pool_output_t
select_mtf_serving_pool(const mtf_jepa_mae_vicreg_encode_output_t &encoded,
                        mtf_serving_pool_policy_t policy,
                        const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(encoded.embeddings.defined() && encoded.token_mask.defined(),
              "[mtf_jepa_mae_vicreg] serving pool requires encoded tokens");
  switch (policy) {
  case mtf_serving_pool_policy_t::all_tokens:
    return {.values = encoded.pooled_by_channel,
            .valid_mask = encoded.channel_valid_mask};
  case mtf_serving_pool_policy_t::time_only:
    return detail::pooled_by_channel_domain(
        encoded.embeddings, encoded.token_mask, encoded.metadata, config,
        /*domain=*/0);
  case mtf_serving_pool_policy_t::frequency_only:
    TORCH_CHECK(config.use_frequency_tokens,
                "[mtf_jepa_mae_vicreg] frequency-only serving requires "
                "frequency tokens");
    return detail::pooled_by_channel_domain(
        encoded.embeddings, encoded.token_mask, encoded.metadata, config,
        /*domain=*/1);
  case mtf_serving_pool_policy_t::domain_balanced: {
    TORCH_CHECK(config.use_frequency_tokens,
                "[mtf_jepa_mae_vicreg] domain-balanced serving requires "
                "frequency tokens");
    auto time = detail::pooled_by_channel_domain(
        encoded.embeddings, encoded.token_mask, encoded.metadata, config,
        /*domain=*/0);
    auto frequency = detail::pooled_by_channel_domain(
        encoded.embeddings, encoded.token_mask, encoded.metadata, config,
        /*domain=*/1);
    auto time_weight = time.valid_mask.to(time.values.dtype()).unsqueeze(-1);
    auto frequency_weight =
        frequency.valid_mask.to(frequency.values.dtype()).unsqueeze(-1);
    auto domain_count = time_weight + frequency_weight;
    auto values =
        (time.values * time_weight + frequency.values * frequency_weight) /
        domain_count.clamp_min(1.0);
    return {.values = std::move(values),
            .valid_mask = time.valid_mask.logical_or(frequency.valid_mask)};
  }
  case mtf_serving_pool_policy_t::structured_cdsb_v1:
    return detail::structured_cdsb_v1_readout(encoded, config);
  case mtf_serving_pool_policy_t::structured_cdsb_sparse_v1:
    return detail::structured_cdsb_sparse_v1_readout(encoded, config);
  }
  throw std::runtime_error("[mtf_jepa_mae_vicreg] unknown serving pool policy");
}

class MultiScalePatchTokenizerImpl : public torch::nn::Module {
public:
  explicit MultiScalePatchTokenizerImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    time_projection_ = register_module(
        "time_projection",
        torch::nn::Linear(2 * config_.input_width, config_.d_model));
    scale_embedding_ = register_module(
        "scale_embedding",
        torch::nn::Embedding(static_cast<int64_t>(config_.time_scales.size()),
                             config_.d_model));
    channel_embedding_ = register_module(
        "channel_embedding",
        torch::nn::Embedding(config_.channel_count, config_.d_model));
    domain_embedding_ = register_module(
        "domain_embedding", torch::nn::Embedding(2, config_.d_model));
    position_projection_ = register_module(
        "position_projection", torch::nn::Linear(4, config_.d_model));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] mtf_token_batch_t
  forward(const torch::Tensor &x,
          const torch::Tensor &feature_mask = torch::Tensor()) {
    using torch::indexing::Slice;
    const auto input = detail::canonicalize_input(x, feature_mask, config_);
    const auto &data = input.data;
    const auto &mask = input.feature_mask;
    const int64_t B = data.size(0);
    const int64_t Hx = data.size(2);
    const auto strides = detail::resolved_scale_strides(config_);

    std::vector<torch::Tensor> token_parts;
    std::vector<torch::Tensor> target_parts;
    std::vector<torch::Tensor> time_raw_parts;
    std::vector<torch::Tensor> frequency_raw_parts;
    std::vector<torch::Tensor> time_mask_parts;
    std::vector<torch::Tensor> frequency_mask_parts;
    std::vector<torch::Tensor> valid_parts;
    std::vector<int64_t> starts;
    std::vector<int64_t> widths;
    std::vector<int64_t> scale_ids;
    std::vector<int64_t> channel_ids;
    std::vector<int64_t> domain_ids;

    for (int64_t c = 0; c < config_.channel_count; ++c) {
      for (std::size_t scale_i = 0; scale_i < config_.time_scales.size();
           ++scale_i) {
        const auto windows = detail::window_plan(
            Hx, config_.time_scales[scale_i], strides[scale_i]);
        for (const auto &[start, width] : windows) {
          auto patch =
              data.index({Slice(), c, Slice(start, start + width), Slice()});
          auto valid =
              mask.index({Slice(), c, Slice(start, start + width), Slice()});
          const auto descriptor = detail::masked_patch_descriptor(patch, valid);
          const auto feature_valid = valid.any(/*dim=*/1);
          auto base = torch::gelu(time_projection_->forward(descriptor));
          auto pos =
              torch::tensor({static_cast<double>(start) /
                                 static_cast<double>(std::max<int64_t>(1, Hx)),
                             static_cast<double>(start) +
                                 0.5 * static_cast<double>(width),
                             static_cast<double>(width) /
                                 static_cast<double>(std::max<int64_t>(1, Hx)),
                             0.0},
                            torch::TensorOptions()
                                .dtype(config_.dtype)
                                .device(config_.device))
                  .view({1, 4});
          pos.index_put_({0, 1},
                         pos.index({0, 1}) /
                             static_cast<double>(std::max<int64_t>(1, Hx)));
          auto scale = torch::full({1}, static_cast<int64_t>(scale_i),
                                   torch::TensorOptions()
                                       .dtype(torch::kInt64)
                                       .device(config_.device));
          auto channel = torch::full({1}, c,
                                     torch::TensorOptions()
                                         .dtype(torch::kInt64)
                                         .device(config_.device));
          auto domain = torch::zeros({1}, torch::TensorOptions()
                                              .dtype(torch::kInt64)
                                              .device(config_.device));
          auto token = base + scale_embedding_->forward(scale) +
                       channel_embedding_->forward(channel) +
                       domain_embedding_->forward(domain) +
                       position_projection_->forward(pos);
          token_parts.push_back(token.unsqueeze(1));
          target_parts.push_back(base.unsqueeze(1));
          time_raw_parts.push_back(descriptor.unsqueeze(1));
          frequency_raw_parts.push_back(torch::zeros(
              {B, 1, config_.frequency_num_bins * config_.input_width},
              data.options()));
          time_mask_parts.push_back(
              torch::cat({feature_valid, feature_valid}, /*dim=*/1)
                  .unsqueeze(1));
          frequency_mask_parts.push_back(torch::zeros(
              {B, 1, config_.frequency_num_bins * config_.input_width},
              torch::TensorOptions()
                  .dtype(torch::kBool)
                  .device(config_.device)));
          valid_parts.push_back(
              valid.any(/*dim=*/1).any(/*dim=*/1).unsqueeze(1));
          starts.push_back(start);
          widths.push_back(width);
          scale_ids.push_back(static_cast<int64_t>(scale_i));
          channel_ids.push_back(c);
          domain_ids.push_back(0);
        }
      }
    }

    mtf_token_batch_t out{};
    out.tokens = torch::cat(token_parts, /*dim=*/1);
    out.reconstruction_targets = torch::cat(target_parts, /*dim=*/1);
    out.time_reconstruction_targets = torch::cat(time_raw_parts, /*dim=*/1);
    out.frequency_reconstruction_targets =
        torch::cat(frequency_raw_parts, /*dim=*/1);
    out.time_reconstruction_mask = torch::cat(time_mask_parts, /*dim=*/1);
    out.frequency_reconstruction_mask =
        torch::cat(frequency_mask_parts, /*dim=*/1);
    out.token_mask = torch::cat(valid_parts, /*dim=*/1);
    auto meta_opts =
        torch::TensorOptions().dtype(torch::kInt64).device(config_.device);
    out.metadata.start_index = torch::tensor(starts, meta_opts);
    out.metadata.width = torch::tensor(widths, meta_opts);
    out.metadata.scale_id = torch::tensor(scale_ids, meta_opts);
    out.metadata.channel_id = torch::tensor(channel_ids, meta_opts);
    out.metadata.domain_id = torch::tensor(domain_ids, meta_opts);
    TORCH_CHECK(out.tokens.size(0) == B,
                "[mtf_jepa_mae_vicreg] tokenizer batch mismatch");
    return out;
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  torch::nn::Linear time_projection_{nullptr};
  torch::nn::Embedding scale_embedding_{nullptr};
  torch::nn::Embedding channel_embedding_{nullptr};
  torch::nn::Embedding domain_embedding_{nullptr};
  torch::nn::Linear position_projection_{nullptr};
};

TORCH_MODULE(MultiScalePatchTokenizer);

class FrequencyTokenizerImpl : public torch::nn::Module {
public:
  explicit FrequencyTokenizerImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    frequency_projection_ = register_module(
        "frequency_projection",
        torch::nn::Linear(config_.frequency_num_bins * config_.input_width,
                          config_.d_model));
    scale_embedding_ = register_module(
        "scale_embedding",
        torch::nn::Embedding(static_cast<int64_t>(config_.time_scales.size()),
                             config_.d_model));
    channel_embedding_ = register_module(
        "channel_embedding",
        torch::nn::Embedding(config_.channel_count, config_.d_model));
    domain_embedding_ = register_module(
        "domain_embedding", torch::nn::Embedding(2, config_.d_model));
    position_projection_ = register_module(
        "position_projection", torch::nn::Linear(4, config_.d_model));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] mtf_token_batch_t
  forward(const torch::Tensor &x,
          const torch::Tensor &feature_mask = torch::Tensor()) {
    using torch::indexing::Slice;
    const auto input = detail::canonicalize_input(x, feature_mask, config_);
    const auto &data = input.data;
    const auto &mask = input.feature_mask;
    const int64_t B = data.size(0);
    const int64_t Hx = data.size(2);
    const auto strides = detail::resolved_scale_strides(config_);

    std::vector<torch::Tensor> token_parts;
    std::vector<torch::Tensor> target_parts;
    std::vector<torch::Tensor> time_raw_parts;
    std::vector<torch::Tensor> frequency_raw_parts;
    std::vector<torch::Tensor> time_mask_parts;
    std::vector<torch::Tensor> frequency_mask_parts;
    std::vector<torch::Tensor> valid_parts;
    std::vector<int64_t> starts;
    std::vector<int64_t> widths;
    std::vector<int64_t> scale_ids;
    std::vector<int64_t> channel_ids;
    std::vector<int64_t> domain_ids;

    for (int64_t c = 0; c < config_.channel_count; ++c) {
      for (std::size_t scale_i = 0; scale_i < config_.time_scales.size();
           ++scale_i) {
        const auto windows = detail::window_plan(
            Hx, config_.time_scales[scale_i], strides[scale_i]);
        for (const auto &[start, width] : windows) {
          auto patch =
              data.index({Slice(), c, Slice(start, start + width), Slice()});
          auto valid =
              mask.index({Slice(), c, Slice(start, start + width), Slice()});
          patch = torch::where(valid, patch, torch::zeros_like(patch));
          const auto feature_valid = valid.any(/*dim=*/1);
          auto descriptor = frequency_descriptor(patch, valid);
          auto base = torch::gelu(frequency_projection_->forward(descriptor));
          auto pos =
              torch::tensor({static_cast<double>(start) /
                                 static_cast<double>(std::max<int64_t>(1, Hx)),
                             static_cast<double>(start) +
                                 0.5 * static_cast<double>(width),
                             static_cast<double>(width) /
                                 static_cast<double>(std::max<int64_t>(1, Hx)),
                             1.0},
                            torch::TensorOptions()
                                .dtype(config_.dtype)
                                .device(config_.device))
                  .view({1, 4});
          pos.index_put_({0, 1},
                         pos.index({0, 1}) /
                             static_cast<double>(std::max<int64_t>(1, Hx)));
          auto scale = torch::full({1}, static_cast<int64_t>(scale_i),
                                   torch::TensorOptions()
                                       .dtype(torch::kInt64)
                                       .device(config_.device));
          auto channel = torch::full({1}, c,
                                     torch::TensorOptions()
                                         .dtype(torch::kInt64)
                                         .device(config_.device));
          auto domain = torch::ones({1}, torch::TensorOptions()
                                             .dtype(torch::kInt64)
                                             .device(config_.device));
          auto token = base + scale_embedding_->forward(scale) +
                       channel_embedding_->forward(channel) +
                       domain_embedding_->forward(domain) +
                       position_projection_->forward(pos);
          token_parts.push_back(token.unsqueeze(1));
          target_parts.push_back(base.unsqueeze(1));
          time_raw_parts.push_back(
              torch::zeros({B, 1, 2 * config_.input_width}, data.options()));
          frequency_raw_parts.push_back(descriptor.unsqueeze(1));
          time_mask_parts.push_back(torch::zeros(
              {B, 1, 2 * config_.input_width}, torch::TensorOptions()
                                                   .dtype(torch::kBool)
                                                   .device(config_.device)));
          frequency_mask_parts.push_back(
              feature_valid.unsqueeze(-1)
                  .expand({B, config_.input_width, config_.frequency_num_bins})
                  .reshape({B, 1,
                            config_.frequency_num_bins * config_.input_width}));
          valid_parts.push_back(
              valid.any(/*dim=*/1).any(/*dim=*/1).unsqueeze(1));
          starts.push_back(start);
          widths.push_back(width);
          scale_ids.push_back(static_cast<int64_t>(scale_i));
          channel_ids.push_back(c);
          domain_ids.push_back(1);
        }
      }
    }

    mtf_token_batch_t out{};
    out.tokens = torch::cat(token_parts, /*dim=*/1);
    out.reconstruction_targets = torch::cat(target_parts, /*dim=*/1);
    out.time_reconstruction_targets = torch::cat(time_raw_parts, /*dim=*/1);
    out.frequency_reconstruction_targets =
        torch::cat(frequency_raw_parts, /*dim=*/1);
    out.time_reconstruction_mask = torch::cat(time_mask_parts, /*dim=*/1);
    out.frequency_reconstruction_mask =
        torch::cat(frequency_mask_parts, /*dim=*/1);
    out.token_mask = torch::cat(valid_parts, /*dim=*/1);
    auto meta_opts =
        torch::TensorOptions().dtype(torch::kInt64).device(config_.device);
    out.metadata.start_index = torch::tensor(starts, meta_opts);
    out.metadata.width = torch::tensor(widths, meta_opts);
    out.metadata.scale_id = torch::tensor(scale_ids, meta_opts);
    out.metadata.channel_id = torch::tensor(channel_ids, meta_opts);
    out.metadata.domain_id = torch::tensor(domain_ids, meta_opts);
    TORCH_CHECK(out.tokens.size(0) == B,
                "[mtf_jepa_mae_vicreg] frequency tokenizer batch mismatch");
    return out;
  }

private:
  [[nodiscard]] torch::Tensor frequency_descriptor(const torch::Tensor &patch,
                                                   const torch::Tensor &valid) {
    const int64_t width = patch.size(1);
    const int64_t actual_bins =
        std::min<int64_t>(config_.frequency_num_bins, width / 2 + 1);
    auto valid_f = valid.to(patch.dtype());
    auto valid_count = valid_f.sum(/*dim=*/1).clamp_min(1.0); // [B,F]
    auto mean = (patch * valid_f).sum(/*dim=*/1) / valid_count;
    auto centered = torch::where(valid, patch - mean.unsqueeze(1),
                                 torch::zeros_like(patch));
    torch::Tensor hann;
    if (width <= 1) {
      hann = torch::ones({width}, patch.options());
    } else {
      constexpr double pi = 3.14159265358979323846;
      auto n = torch::arange(width, patch.options());
      hann =
          0.5 - 0.5 * torch::cos(2.0 * pi * n / static_cast<double>(width - 1));
    }
    centered = centered * hann.view({1, width, 1});
    auto denom =
        (valid_f * hann.view({1, width, 1})).sum(/*dim=*/1).clamp_min(1.0);

    auto t = torch::arange(
        width,
        torch::TensorOptions().dtype(config_.dtype).device(config_.device));
    auto k =
        torch::arange(
            actual_bins,
            torch::TensorOptions().dtype(config_.dtype).device(config_.device))
            .unsqueeze(1);
    constexpr double pi = 3.14159265358979323846;
    auto angle = 2.0 * pi * k * t.unsqueeze(0) /
                 static_cast<double>(std::max<int64_t>(1, width));
    auto cos_basis = torch::cos(angle).transpose(0, 1); // [W,K]
    auto sin_basis = torch::sin(angle).transpose(0, 1); // [W,K]
    auto by_feature = centered.transpose(1, 2);         // [B,F,W]
    auto real = torch::matmul(by_feature, cos_basis);
    auto imag = torch::matmul(by_feature, sin_basis);
    auto mag =
        torch::sqrt(real.pow(2) + imag.pow(2) + 1e-8) / denom.unsqueeze(-1);
    if (actual_bins > 0) {
      mag.index_put_({torch::indexing::Slice(), torch::indexing::Slice(), 0},
                     mean.abs());
    }
    if (config_.frequency_log_magnitude) {
      mag = torch::log1p(mag);
    }
    auto padded = torch::zeros(
        {patch.size(0), config_.input_width, config_.frequency_num_bins},
        patch.options());
    if (actual_bins > 0) {
      padded.index_put_({torch::indexing::Slice(), torch::indexing::Slice(),
                         torch::indexing::Slice(0, actual_bins)},
                        mag);
    }
    return padded.reshape(
        {patch.size(0), config_.frequency_num_bins * config_.input_width});
  }

  mtf_jepa_mae_vicreg_config_t config_{};
  torch::nn::Linear frequency_projection_{nullptr};
  torch::nn::Embedding scale_embedding_{nullptr};
  torch::nn::Embedding channel_embedding_{nullptr};
  torch::nn::Embedding domain_embedding_{nullptr};
  torch::nn::Linear position_projection_{nullptr};
};

TORCH_MODULE(FrequencyTokenizer);

inline mtf_token_batch_t concat_token_batches(const mtf_token_batch_t &a,
                                              const mtf_token_batch_t &b) {
  mtf_token_batch_t out{};
  out.tokens = torch::cat({a.tokens, b.tokens}, /*dim=*/1);
  out.reconstruction_targets =
      torch::cat({a.reconstruction_targets, b.reconstruction_targets},
                 /*dim=*/1);
  out.time_reconstruction_targets =
      torch::cat({a.time_reconstruction_targets, b.time_reconstruction_targets},
                 /*dim=*/1);
  out.frequency_reconstruction_targets = torch::cat(
      {a.frequency_reconstruction_targets, b.frequency_reconstruction_targets},
      /*dim=*/1);
  out.time_reconstruction_mask =
      torch::cat({a.time_reconstruction_mask, b.time_reconstruction_mask},
                 /*dim=*/1);
  out.frequency_reconstruction_mask = torch::cat(
      {a.frequency_reconstruction_mask, b.frequency_reconstruction_mask},
      /*dim=*/1);
  out.token_mask = torch::cat({a.token_mask, b.token_mask}, /*dim=*/1);
  out.metadata.start_index =
      torch::cat({a.metadata.start_index, b.metadata.start_index}, 0);
  out.metadata.width = torch::cat({a.metadata.width, b.metadata.width}, 0);
  out.metadata.scale_id =
      torch::cat({a.metadata.scale_id, b.metadata.scale_id}, 0);
  out.metadata.channel_id =
      torch::cat({a.metadata.channel_id, b.metadata.channel_id}, 0);
  out.metadata.domain_id =
      torch::cat({a.metadata.domain_id, b.metadata.domain_id}, 0);
  return out;
}

class TimeFrequencyViewBuilderImpl : public torch::nn::Module {
public:
  explicit TimeFrequencyViewBuilderImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    time_tokenizer_ =
        register_module("time_tokenizer", MultiScalePatchTokenizer(config_));
    frequency_tokenizer_ =
        register_module("frequency_tokenizer", FrequencyTokenizer(config_));
  }

  [[nodiscard]] mtf_token_batch_t
  forward(const torch::Tensor &x,
          const torch::Tensor &feature_mask = torch::Tensor()) {
    auto time_tokens = time_tokenizer_->forward(x, feature_mask);
    if (!config_.use_frequency_tokens) {
      return time_tokens;
    }
    auto frequency_tokens = frequency_tokenizer_->forward(x, feature_mask);
    return concat_token_batches(time_tokens, frequency_tokens);
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  MultiScalePatchTokenizer time_tokenizer_{nullptr};
  FrequencyTokenizer frequency_tokenizer_{nullptr};
};

TORCH_MODULE(TimeFrequencyViewBuilder);

class JEPAContextTargetMasker {
public:
  explicit JEPAContextTargetMasker(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
  }

  [[nodiscard]] jepa_context_target_mask_t
  create_masks(const mtf_token_batch_t &batch) const {
    auto legacy = create_legacy_masks(batch);
    if (config_.jepa_mask_policy ==
        mtf_jepa_mask_policy_t::legacy_soft_overlap) {
      return legacy;
    }
    return create_paired_target_masks(
        batch, legacy,
        config_.jepa_mask_policy ==
            mtf_jepa_mask_policy_t::support_separated_pair_v1);
  }

private:
  [[nodiscard]] jepa_context_target_mask_t
  create_legacy_masks(const mtf_token_batch_t &batch) const {
    TORCH_CHECK(batch.token_mask.defined() && batch.token_mask.dim() == 2,
                "[mtf_jepa_mae_vicreg] token_mask must be [B,N]");
    const int64_t B = batch.token_mask.size(0);
    const int64_t N = batch.token_mask.size(1);
    auto valid_cpu = batch.token_mask.to(torch::kCPU).contiguous();
    auto target_mask_cpu = torch::zeros(
        {B, N}, torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU));
    auto context_mask_cpu = torch::zeros_like(target_mask_cpu);
    auto domain_cpu = batch.metadata.domain_id.to(torch::kCPU).contiguous();
    auto channel_cpu = batch.metadata.channel_id.to(torch::kCPU).contiguous();
    auto scale_cpu = batch.metadata.scale_id.to(torch::kCPU).contiguous();
    auto start_cpu = batch.metadata.start_index.to(torch::kCPU).contiguous();
    auto width_cpu = batch.metadata.width.to(torch::kCPU).contiguous();
    auto valid_acc = valid_cpu.accessor<bool, 2>();
    auto target_acc = target_mask_cpu.accessor<bool, 2>();
    auto context_acc = context_mask_cpu.accessor<bool, 2>();
    auto domain_acc = domain_cpu.accessor<int64_t, 1>();
    auto channel_acc = channel_cpu.accessor<int64_t, 1>();
    auto scale_acc = scale_cpu.accessor<int64_t, 1>();
    auto start_acc = start_cpu.accessor<int64_t, 1>();
    auto width_acc = width_cpu.accessor<int64_t, 1>();

    int64_t total_context = 0;
    int64_t total_target = 0;
    int64_t total_valid = 0;
    int64_t total_hard_forbidden = 0;
    int64_t total_soft_forbidden = 0;
    int64_t total_relaxed_soft_forbidden = 0;
    int64_t total_reduced_targets_for_context = 0;
    int64_t total_context_starved_samples = 0;
    int64_t total_min_context_satisfied_samples = 0;

    for (int64_t b = 0; b < B; ++b) {
      std::vector<int64_t> valid_indices;
      std::vector<int64_t> time_indices;
      std::vector<int64_t> freq_indices;
      valid_indices.reserve(static_cast<std::size_t>(N));
      for (int64_t n = 0; n < N; ++n) {
        if (!valid_acc[b][n]) {
          continue;
        }
        valid_indices.push_back(n);
        if (domain_acc[n] == 0) {
          time_indices.push_back(n);
        } else {
          freq_indices.push_back(n);
        }
      }

      const int64_t valid_count = static_cast<int64_t>(valid_indices.size());
      total_valid += valid_count;
      if (valid_count == 0) {
        continue;
      }
      const int64_t min_context =
          std::max<int64_t>(1, static_cast<int64_t>(std::ceil(
                                   config_.min_context_ratio * valid_count)));
      const int64_t max_target =
          std::max<int64_t>(0, valid_count - min_context);
      std::set<int64_t> target_set;

      if (!time_indices.empty() && max_target > 0) {
        const int64_t desired_time = std::max<int64_t>(
            1, static_cast<int64_t>(std::llround(config_.mask_ratio_time *
                                                 time_indices.size())));
        const int64_t block_len = std::min<int64_t>(
            desired_time, static_cast<int64_t>(time_indices.size()));
        const int64_t max_start =
            static_cast<int64_t>(time_indices.size()) - block_len;
        int64_t block_start = 0;
        if (max_start > 0) {
          block_start =
              torch::randint(0, max_start + 1, {1},
                             torch::TensorOptions().dtype(torch::kInt64))
                  .item<int64_t>();
        }
        for (int64_t i = 0; i < block_len; ++i) {
          target_set.insert(
              time_indices[static_cast<std::size_t>(block_start + i)]);
        }
      }

      if (!freq_indices.empty() && max_target > 0 &&
          config_.mask_ratio_frequency > 0.0) {
        const int64_t desired_freq = static_cast<int64_t>(
            std::llround(config_.mask_ratio_frequency * freq_indices.size()));
        add_random_subset(freq_indices, desired_freq, target_set);
      }

      if (config_.mask_ratio_channel > 0.0 && max_target > 0) {
        for (int64_t c = 0; c < config_.channel_count; ++c) {
          const double draw = torch::rand({1}).item<double>();
          if (draw > config_.mask_ratio_channel) {
            continue;
          }
          for (const auto idx : valid_indices) {
            if (channel_acc[idx] == c) {
              target_set.insert(idx);
            }
          }
        }
      }

      if (target_set.empty() && max_target > 0 && valid_count > 1) {
        target_set.insert(valid_indices.back());
      }

      if (!target_set.empty() && max_target > 0) {
        std::vector<int64_t> seed_targets(target_set.begin(), target_set.end());
        for (const auto target_idx : seed_targets) {
          for (const auto candidate_idx : valid_indices) {
            if (candidate_idx == target_idx ||
                target_set.find(candidate_idx) != target_set.end()) {
              continue;
            }
            const bool same_window =
                channel_acc[candidate_idx] == channel_acc[target_idx] &&
                scale_acc[candidate_idx] == scale_acc[target_idx] &&
                start_acc[candidate_idx] == start_acc[target_idx] &&
                width_acc[candidate_idx] == width_acc[target_idx];
            const bool cross_domain =
                domain_acc[candidate_idx] != domain_acc[target_idx];
            const bool same_channel =
                channel_acc[candidate_idx] == channel_acc[target_idx];
            if (config_.mask_same_channel_block && same_channel &&
                static_cast<int64_t>(target_set.size()) < max_target) {
              target_set.insert(candidate_idx);
              continue;
            }
            if (config_.couple_time_frequency_masks &&
                config_.mask_same_window_across_domains && same_window &&
                cross_domain &&
                static_cast<int64_t>(target_set.size()) < max_target) {
              target_set.insert(candidate_idx);
            }
          }
        }
      }

      if (static_cast<int64_t>(target_set.size()) > max_target) {
        std::vector<int64_t> capped(target_set.begin(), target_set.end());
        target_set.clear();
        add_random_subset(capped, max_target, target_set);
      }

      std::set<int64_t> hard_forbidden_context;
      std::set<int64_t> soft_forbidden_context;
      auto recompute_forbids = [&](const std::set<int64_t> &targets,
                                   std::set<int64_t> &hard,
                                   std::set<int64_t> &soft) {
        hard.clear();
        soft.clear();
        for (const auto target_idx : targets) {
          for (const auto candidate_idx : valid_indices) {
            if (candidate_idx == target_idx ||
                targets.find(candidate_idx) != targets.end()) {
              continue;
            }
            const bool same_window =
                channel_acc[candidate_idx] == channel_acc[target_idx] &&
                scale_acc[candidate_idx] == scale_acc[target_idx] &&
                start_acc[candidate_idx] == start_acc[target_idx] &&
                width_acc[candidate_idx] == width_acc[target_idx];
            const bool cross_domain =
                domain_acc[candidate_idx] != domain_acc[target_idx];
            const bool same_channel =
                channel_acc[candidate_idx] == channel_acc[target_idx];
            if ((config_.couple_time_frequency_masks &&
                 config_.mask_same_window_across_domains && same_window &&
                 cross_domain) ||
                (config_.mask_same_channel_block && same_channel)) {
              hard.insert(candidate_idx);
              soft.erase(candidate_idx);
              continue;
            }
            const double overlap = interval_overlap_ratio(
                start_acc[target_idx], width_acc[target_idx],
                start_acc[candidate_idx], width_acc[candidate_idx]);
            if (overlap > config_.max_context_target_time_overlap &&
                hard.find(candidate_idx) == hard.end()) {
              soft.insert(candidate_idx);
            }
          }
        }
      };

      auto context_count_with = [&](const std::set<int64_t> &targets,
                                    const std::set<int64_t> &hard,
                                    const std::set<int64_t> &soft) {
        int64_t count = 0;
        for (const auto idx : valid_indices) {
          if (targets.find(idx) == targets.end() &&
              hard.find(idx) == hard.end() && soft.find(idx) == soft.end()) {
            ++count;
          }
        }
        return count;
      };

      recompute_forbids(target_set, hard_forbidden_context,
                        soft_forbidden_context);

      std::set<int64_t> empty_forbidden_context;
      while (!target_set.empty() &&
             context_count_with(target_set, hard_forbidden_context,
                                empty_forbidden_context) < min_context) {
        int64_t best_target = *target_set.begin();
        int64_t best_context_count = -1;
        int64_t best_hard_reduction = -1;
        for (const auto candidate_target : target_set) {
          auto reduced_targets = target_set;
          reduced_targets.erase(candidate_target);
          std::set<int64_t> reduced_hard;
          std::set<int64_t> reduced_soft;
          recompute_forbids(reduced_targets, reduced_hard, reduced_soft);
          const int64_t reduced_context = context_count_with(
              reduced_targets, reduced_hard, empty_forbidden_context);
          const int64_t hard_reduction =
              static_cast<int64_t>(hard_forbidden_context.size()) -
              static_cast<int64_t>(reduced_hard.size());
          if (reduced_context > best_context_count ||
              (reduced_context == best_context_count &&
               hard_reduction > best_hard_reduction)) {
            best_target = candidate_target;
            best_context_count = reduced_context;
            best_hard_reduction = hard_reduction;
          }
        }
        target_set.erase(best_target);
        ++total_reduced_targets_for_context;
        recompute_forbids(target_set, hard_forbidden_context,
                          soft_forbidden_context);
      }

      auto context_count_after_forbid = [&]() {
        return context_count_with(target_set, hard_forbidden_context,
                                  soft_forbidden_context);
      };
      while (context_count_after_forbid() < min_context &&
             !soft_forbidden_context.empty()) {
        auto it = soft_forbidden_context.end();
        --it;
        soft_forbidden_context.erase(it);
        ++total_relaxed_soft_forbidden;
      }

      for (const auto idx : target_set) {
        target_acc[b][idx] = true;
      }
      for (const auto idx : valid_indices) {
        const bool is_target = target_set.find(idx) != target_set.end();
        const bool is_forbidden =
            hard_forbidden_context.find(idx) != hard_forbidden_context.end() ||
            soft_forbidden_context.find(idx) != soft_forbidden_context.end();
        context_acc[b][idx] = !is_target && !is_forbidden;
      }
      total_target += static_cast<int64_t>(target_set.size());
      total_context += context_count_after_forbid();
      if (context_count_after_forbid() >= min_context) {
        ++total_min_context_satisfied_samples;
      } else {
        ++total_context_starved_samples;
      }
      total_hard_forbidden +=
          static_cast<int64_t>(hard_forbidden_context.size());
      total_soft_forbidden +=
          static_cast<int64_t>(soft_forbidden_context.size());
    }

    jepa_context_target_mask_t out{};
    out.context_mask = context_mask_cpu.to(batch.token_mask.device());
    out.target_mask = target_mask_cpu.to(batch.token_mask.device());
    out.valid_mask = batch.token_mask;
    const double actual = total_valid > 0 ? static_cast<double>(total_target) /
                                                static_cast<double>(total_valid)
                                          : 0.0;
    out.mask_ratio_actual =
        torch::tensor(actual, torch::TensorOptions()
                                  .dtype(torch::kFloat64)
                                  .device(batch.token_mask.device()));
    out.num_context_tokens = total_context;
    out.num_target_tokens = total_target;
    out.hard_forbidden_count = total_hard_forbidden;
    out.soft_forbidden_count = total_soft_forbidden;
    out.relaxed_soft_forbidden_count = total_relaxed_soft_forbidden;
    out.reduced_targets_for_context_count = total_reduced_targets_for_context;
    out.context_starved_sample_count = total_context_starved_samples;
    out.min_context_satisfied_count = total_min_context_satisfied_samples;
    return out;
  }

private:
  struct PairedTargetCandidate {
    int64_t time_index{0};
    int64_t frequency_index{0};
    int64_t channel{0};
    int64_t scale{0};
    int64_t start{0};
    int64_t width{0};
  };

  [[nodiscard]] jepa_context_target_mask_t create_paired_target_masks(
      const mtf_token_batch_t &batch,
      const jepa_context_target_mask_t &legacy,
      bool support_separated) const {
    const int64_t B = batch.token_mask.size(0);
    const int64_t N = batch.token_mask.size(1);
    auto valid_cpu =
        batch.token_mask.to(torch::kCPU, torch::kBool).contiguous();
    auto legacy_target_cpu =
        legacy.target_mask.to(torch::kCPU, torch::kBool).contiguous();
    auto legacy_context_cpu =
        legacy.context_mask.to(torch::kCPU, torch::kBool).contiguous();
    auto domain_cpu =
        batch.metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
    auto channel_cpu =
        batch.metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
    auto scale_cpu =
        batch.metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
    auto start_cpu =
        batch.metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
    auto width_cpu =
        batch.metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
    auto target_cpu = torch::zeros_like(valid_cpu);
    auto context_cpu = torch::zeros_like(valid_cpu);

    auto valid = valid_cpu.accessor<bool, 2>();
    auto legacy_target = legacy_target_cpu.accessor<bool, 2>();
    auto legacy_context = legacy_context_cpu.accessor<bool, 2>();
    auto target = target_cpu.accessor<bool, 2>();
    auto context = context_cpu.accessor<bool, 2>();
    auto domain = domain_cpu.accessor<int64_t, 1>();
    auto channel = channel_cpu.accessor<int64_t, 1>();
    auto scale = scale_cpu.accessor<int64_t, 1>();
    auto start = start_cpu.accessor<int64_t, 1>();
    auto width = width_cpu.accessor<int64_t, 1>();

    int64_t total_valid = 0;
    int64_t total_targets = 0;
    int64_t total_contexts = 0;
    int64_t total_hard_forbidden = 0;
    int64_t paired_target_samples = 0;
    int64_t support_separated_samples = 0;
    int64_t retained_legacy_contexts = 0;
    int64_t replaced_contexts = 0;

    for (int64_t b = 0; b < B; ++b) {
      int64_t valid_count = 0;
      for (int64_t n = 0; n < N; ++n) {
        valid_count += valid[b][n] ? 1 : 0;
      }
      total_valid += valid_count;
      if (valid_count == 0) {
        continue;
      }

      std::vector<PairedTargetCandidate> candidates;
      int64_t minimum_width = std::numeric_limits<int64_t>::max();
      for (int64_t time_index = 0; time_index < N; ++time_index) {
        if (!valid[b][time_index] || domain[time_index] != 0) {
          continue;
        }
        for (int64_t frequency_index = 0; frequency_index < N;
             ++frequency_index) {
          if (!valid[b][frequency_index] || domain[frequency_index] != 1 ||
              channel[time_index] != channel[frequency_index] ||
              scale[time_index] != scale[frequency_index] ||
              start[time_index] != start[frequency_index] ||
              width[time_index] != width[frequency_index]) {
            continue;
          }
          if (width[time_index] < minimum_width) {
            minimum_width = width[time_index];
            candidates.clear();
          }
          if (width[time_index] == minimum_width) {
            candidates.push_back({time_index, frequency_index,
                                  channel[time_index], scale[time_index],
                                  start[time_index], width[time_index]});
          }
        }
      }
      TORCH_CHECK(
          !candidates.empty(),
          "[mtf_jepa_mae_vicreg] paired-target JEPA mask has no valid "
          "time/frequency target pair");

      uint64_t salt = 0x494d41325f6d6173ULL;
      for (int64_t n = 0; n < N; ++n) {
        if (legacy_target[b][n]) {
          salt = mask_mix64(salt ^
                            (0x7461726765740000ULL + static_cast<uint64_t>(n)));
        }
        if (legacy_context[b][n]) {
          salt = mask_mix64(salt ^
                            (0x636f6e7465787400ULL + static_cast<uint64_t>(n)));
        }
      }
      const auto pair_key = [&](const PairedTargetCandidate &value) {
        return metadata_tie_key(salt, value.channel, value.scale, value.start,
                                value.width, 2);
      };
      const auto pair_less = [&](const PairedTargetCandidate &left,
                                 const PairedTargetCandidate &right) {
        const uint64_t left_key = pair_key(left);
        const uint64_t right_key = pair_key(right);
        if (left_key != right_key) {
          return left_key < right_key;
        }
        return std::tie(left.channel, left.scale, left.start, left.width) <
               std::tie(right.channel, right.scale, right.start, right.width);
      };
      const auto selected_pair =
          *std::min_element(candidates.begin(), candidates.end(), pair_less);
      target[b][selected_pair.time_index] = true;
      target[b][selected_pair.frequency_index] = true;

      const int64_t desired_context =
          std::max<int64_t>(1, static_cast<int64_t>(std::ceil(
                                   config_.min_context_ratio * valid_count)));
      std::vector<int64_t> eligible;
      eligible.reserve(static_cast<std::size_t>(valid_count));
      for (int64_t n = 0; n < N; ++n) {
        if (!valid[b][n] || n == selected_pair.time_index ||
            n == selected_pair.frequency_index) {
          continue;
        }
        const bool same_channel = channel[n] == selected_pair.channel;
        const bool overlaps =
            std::max(start[n], selected_pair.start) <
            std::min(start[n] + width[n],
                     selected_pair.start + selected_pair.width);
        if (support_separated && same_channel && overlaps) {
          ++total_hard_forbidden;
          continue;
        }
        eligible.push_back(n);
      }
      TORCH_CHECK(static_cast<int64_t>(eligible.size()) >= desired_context,
                  "[mtf_jepa_mae_vicreg] paired-target JEPA context is "
                  "geometrically infeasible for this sample");

      const auto context_key = [&](int64_t index) {
        return metadata_tie_key(salt, channel[index], scale[index],
                                start[index], width[index], domain[index]);
      };
      const auto context_less = [&](int64_t left, int64_t right) {
        const uint64_t left_key = context_key(left);
        const uint64_t right_key = context_key(right);
        if (left_key != right_key) {
          return left_key < right_key;
        }
        return std::tie(channel[left], domain[left], scale[left], start[left],
                        width[left]) < std::tie(channel[right], domain[right],
                                                scale[right], start[right],
                                                width[right]);
      };

      std::vector<int64_t> selected_context;
      selected_context.reserve(static_cast<std::size_t>(desired_context));
      for (const int64_t index : eligible) {
        if (legacy_context[b][index]) {
          selected_context.push_back(index);
        }
      }
      std::sort(selected_context.begin(), selected_context.end(), context_less);
      if (static_cast<int64_t>(selected_context.size()) > desired_context) {
        selected_context.resize(static_cast<std::size_t>(desired_context));
      }
      const int64_t retained = static_cast<int64_t>(selected_context.size());
      std::set<int64_t> selected_set(selected_context.begin(),
                                     selected_context.end());
      std::vector<int64_t> replacements;
      for (const int64_t index : eligible) {
        if (selected_set.find(index) == selected_set.end()) {
          replacements.push_back(index);
        }
      }
      std::sort(replacements.begin(), replacements.end(), context_less);
      for (const int64_t index : replacements) {
        if (static_cast<int64_t>(selected_context.size()) == desired_context) {
          break;
        }
        selected_context.push_back(index);
      }
      TORCH_CHECK(
          static_cast<int64_t>(selected_context.size()) == desired_context,
          "[mtf_jepa_mae_vicreg] paired-target JEPA context selection "
          "did not reach its exact count");
      for (const int64_t index : selected_context) {
        context[b][index] = true;
      }

      total_targets += 2;
      total_contexts += desired_context;
      ++paired_target_samples;
      support_separated_samples += support_separated ? 1 : 0;
      retained_legacy_contexts += retained;
      replaced_contexts += desired_context - retained;
    }

    jepa_context_target_mask_t out{};
    out.context_mask = context_cpu.to(batch.token_mask.device());
    out.target_mask = target_cpu.to(batch.token_mask.device());
    out.valid_mask = batch.token_mask;
    const double actual = total_valid > 0 ? static_cast<double>(total_targets) /
                                                static_cast<double>(total_valid)
                                          : 0.0;
    out.mask_ratio_actual =
        torch::tensor(actual, torch::TensorOptions()
                                  .dtype(torch::kFloat64)
                                  .device(batch.token_mask.device()));
    out.num_context_tokens = total_contexts;
    out.num_target_tokens = total_targets;
    out.hard_forbidden_count = total_hard_forbidden;
    out.soft_forbidden_count = 0;
    out.relaxed_soft_forbidden_count = 0;
    out.reduced_targets_for_context_count = 0;
    out.context_starved_sample_count = 0;
    out.min_context_satisfied_count = paired_target_samples;
    out.support_separated_sample_count = support_separated_samples;
    out.retained_legacy_context_count = retained_legacy_contexts;
    out.replaced_context_count = replaced_contexts;
    return out;
  }

  [[nodiscard]] static uint64_t mask_mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
  }

  [[nodiscard]] static uint64_t metadata_tie_key(uint64_t salt, int64_t channel,
                                                 int64_t scale, int64_t start,
                                                 int64_t width,
                                                 int64_t domain) {
    uint64_t value = salt;
    value = mask_mix64(value ^ static_cast<uint64_t>(channel + 1));
    value = mask_mix64(value ^ (static_cast<uint64_t>(scale + 1) << 8U));
    value = mask_mix64(value ^ (static_cast<uint64_t>(start + 1) << 16U));
    value = mask_mix64(value ^ (static_cast<uint64_t>(width + 1) << 32U));
    return mask_mix64(value ^ (static_cast<uint64_t>(domain + 1) << 56U));
  }

  static void add_random_subset(const std::vector<int64_t> &indices,
                                int64_t desired,
                                std::set<int64_t> &target_set) {
    if (desired <= 0 || indices.empty()) {
      return;
    }
    const int64_t take =
        std::min<int64_t>(desired, static_cast<int64_t>(indices.size()));
    auto perm = torch::randperm(static_cast<int64_t>(indices.size()),
                                torch::TensorOptions().dtype(torch::kInt64));
    auto acc = perm.accessor<int64_t, 1>();
    for (int64_t i = 0; i < take; ++i) {
      target_set.insert(indices[static_cast<std::size_t>(acc[i])]);
    }
  }

  static double interval_overlap_ratio(int64_t start_a, int64_t width_a,
                                       int64_t start_b, int64_t width_b) {
    const int64_t end_a = start_a + width_a;
    const int64_t end_b = start_b + width_b;
    const int64_t overlap = std::max<int64_t>(
        0, std::min(end_a, end_b) - std::max(start_a, start_b));
    const int64_t denom = std::max<int64_t>(1, std::min(width_a, width_b));
    return static_cast<double>(overlap) / static_cast<double>(denom);
  }

  mtf_jepa_mae_vicreg_config_t config_{};
};

class SharedTokenEncoderImpl : public torch::nn::Module {
public:
  explicit SharedTokenEncoderImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    for (int64_t i = 0; i < config_.num_encoder_layers; ++i) {
      token_layers_.push_back(
          register_module("token_layer_" + std::to_string(i),
                          torch::nn::Linear(config_.d_model, config_.d_model)));
      mix_layers_.push_back(
          register_module("mix_layer_" + std::to_string(i),
                          torch::nn::Linear(config_.d_model, config_.d_model)));
      norms_.push_back(
          register_module("norm_" + std::to_string(i),
                          torch::nn::LayerNorm(
                              torch::nn::LayerNormOptions({config_.d_model}))));
    }
    out_norm_ = register_module(
        "out_norm",
        torch::nn::LayerNorm(torch::nn::LayerNormOptions({config_.d_model})));
    latent_projection_ =
        register_module("latent_projection",
                        torch::nn::Linear(config_.d_model, config_.latent_dim));
    dropout_ = register_module("dropout", torch::nn::Dropout(config_.dropout));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] torch::Tensor forward(const torch::Tensor &tokens,
                                      const torch::Tensor &token_mask) {
    auto h = tokens.to(
        torch::TensorOptions().dtype(config_.dtype).device(config_.device));
    const auto mask = token_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config_.device));
    h = h.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
    for (std::size_t i = 0; i < token_layers_.size(); ++i) {
      const auto pooled = detail::masked_mean(h, mask);
      auto mixed = mix_layers_[i]->forward(pooled).unsqueeze(1).expand_as(h);
      auto update = token_layers_[i]->forward(norms_[i]->forward(h + mixed));
      update = torch::gelu(update);
      update = dropout_->forward(update);
      h = (h + update).masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
    }
    return latent_projection_->forward(out_norm_->forward(h))
        .masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  std::vector<torch::nn::Linear> token_layers_{};
  std::vector<torch::nn::Linear> mix_layers_{};
  std::vector<torch::nn::LayerNorm> norms_{};
  torch::nn::LayerNorm out_norm_{nullptr};
  torch::nn::Linear latent_projection_{nullptr};
  torch::nn::Dropout dropout_{nullptr};
};

TORCH_MODULE(SharedTokenEncoder);

class LatentPredictorImpl : public torch::nn::Module {
public:
  explicit LatentPredictorImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    metadata_projection_ = register_module(
        "metadata_projection", torch::nn::Linear(6, config_.latent_dim));
    q_projection_ =
        register_module("q_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    k_projection_ =
        register_module("k_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    v_projection_ =
        register_module("v_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    layers_.push_back(register_module(
        "predictor_in",
        torch::nn::Linear(config_.latent_dim, config_.predictor_hidden_dim)));
    for (int64_t i = 1; i < config_.num_predictor_layers; ++i) {
      layers_.push_back(
          register_module("predictor_hidden_" + std::to_string(i),
                          torch::nn::Linear(config_.predictor_hidden_dim,
                                            config_.predictor_hidden_dim)));
    }
    out_ = register_module(
        "predictor_out",
        torch::nn::Linear(config_.predictor_hidden_dim, config_.latent_dim));
    dropout_ = register_module("dropout", torch::nn::Dropout(config_.dropout));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] torch::Tensor forward(const torch::Tensor &context_latents,
                                      const torch::Tensor &context_mask,
                                      const mtf_token_metadata_t &metadata) {
    const auto meta = metadata_projection_->forward(
        detail::metadata_features(metadata, config_));
    auto q = q_projection_->forward(meta).unsqueeze(0).expand(
        {context_latents.size(0), meta.size(0), config_.latent_dim});
    auto k = k_projection_->forward(context_latents);
    auto v = v_projection_->forward(context_latents);
    const auto mask = context_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config_.device));
    auto attended =
        detail::multi_head_context_attention(q, k, v, mask, config_.num_heads);
    auto h = attended + q;
    for (auto &layer : layers_) {
      h = dropout_->forward(torch::gelu(layer->forward(h)));
    }
    return out_->forward(h);
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  torch::nn::Linear metadata_projection_{nullptr};
  torch::nn::Linear q_projection_{nullptr};
  torch::nn::Linear k_projection_{nullptr};
  torch::nn::Linear v_projection_{nullptr};
  std::vector<torch::nn::Linear> layers_{};
  torch::nn::Linear out_{nullptr};
  torch::nn::Dropout dropout_{nullptr};
};

TORCH_MODULE(LatentPredictor);

class MaeDecoderImpl : public torch::nn::Module {
public:
  explicit MaeDecoderImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    metadata_projection_ = register_module(
        "metadata_projection", torch::nn::Linear(6, config_.latent_dim));
    q_projection_ =
        register_module("q_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    k_projection_ =
        register_module("k_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    v_projection_ =
        register_module("v_projection", torch::nn::Linear(config_.latent_dim,
                                                          config_.latent_dim));
    layers_.push_back(register_module(
        "decoder_in",
        torch::nn::Linear(config_.latent_dim, config_.predictor_hidden_dim)));
    for (int64_t i = 1; i < config_.num_decoder_layers; ++i) {
      layers_.push_back(
          register_module("decoder_hidden_" + std::to_string(i),
                          torch::nn::Linear(config_.predictor_hidden_dim,
                                            config_.predictor_hidden_dim)));
    }
    projected_out_ = register_module(
        "decoder_projected_out",
        torch::nn::Linear(config_.predictor_hidden_dim, config_.d_model));
    time_out_ = register_module("decoder_time_out",
                                torch::nn::Linear(config_.predictor_hidden_dim,
                                                  2 * config_.input_width));
    frequency_out_ = register_module(
        "decoder_frequency_out",
        torch::nn::Linear(config_.predictor_hidden_dim,
                          config_.frequency_num_bins * config_.input_width));
    dropout_ = register_module("dropout", torch::nn::Dropout(config_.dropout));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] mae_decoder_output_t
  forward(const torch::Tensor &context_latents,
          const torch::Tensor &context_mask,
          const mtf_token_metadata_t &metadata) {
    const auto meta = metadata_projection_->forward(
        detail::metadata_features(metadata, config_));
    auto q = q_projection_->forward(meta).unsqueeze(0).expand(
        {context_latents.size(0), meta.size(0), config_.latent_dim});
    auto k = k_projection_->forward(context_latents);
    auto v = v_projection_->forward(context_latents);
    const auto mask = context_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config_.device));
    auto h =
        detail::multi_head_context_attention(q, k, v, mask, config_.num_heads) +
        q;
    for (auto &layer : layers_) {
      h = dropout_->forward(torch::gelu(layer->forward(h)));
    }
    mae_decoder_output_t out{};
    out.projected = projected_out_->forward(h);
    out.time = time_out_->forward(h);
    out.frequency = frequency_out_->forward(h);
    return out;
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  torch::nn::Linear metadata_projection_{nullptr};
  torch::nn::Linear q_projection_{nullptr};
  torch::nn::Linear k_projection_{nullptr};
  torch::nn::Linear v_projection_{nullptr};
  std::vector<torch::nn::Linear> layers_{};
  torch::nn::Linear projected_out_{nullptr};
  torch::nn::Linear time_out_{nullptr};
  torch::nn::Linear frequency_out_{nullptr};
  torch::nn::Dropout dropout_{nullptr};
};

TORCH_MODULE(MaeDecoder);

class VICRegStabilityHeadImpl : public torch::nn::Module {
public:
  explicit VICRegStabilityHeadImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)) {
    detail::validate_config(config_);
    projector_layers_.push_back(register_module(
        "projector_in",
        torch::nn::Linear(config_.latent_dim, config_.predictor_hidden_dim)));
    projector_layers_.push_back(register_module(
        "projector_hidden", torch::nn::Linear(config_.predictor_hidden_dim,
                                              config_.predictor_hidden_dim)));
    projector_out_ = register_module(
        "projector_out",
        torch::nn::Linear(config_.predictor_hidden_dim, config_.projector_dim));
    this->to(config_.device, config_.dtype);
  }

  [[nodiscard]] torch::Tensor forward(const torch::Tensor &pooled_latents) {
    TORCH_CHECK(pooled_latents.defined() && pooled_latents.dim() == 2,
                "[mtf_jepa_mae_vicreg] stability head input must be [B,D]");
    TORCH_CHECK(pooled_latents.size(1) == config_.latent_dim,
                "[mtf_jepa_mae_vicreg] stability head dim mismatch");
    auto h = pooled_latents.to(
        torch::TensorOptions().dtype(config_.dtype).device(config_.device));
    for (auto &layer : projector_layers_) {
      h = torch::gelu(layer->forward(h));
    }
    return projector_out_->forward(h).unsqueeze(1);
  }

private:
  mtf_jepa_mae_vicreg_config_t config_{};
  std::vector<torch::nn::Linear> projector_layers_{};
  torch::nn::Linear projector_out_{nullptr};
};

TORCH_MODULE(VICRegStabilityHead);

class MtfJepaMaeVicregImpl : public torch::nn::Module {
public:
  explicit MtfJepaMaeVicregImpl(mtf_jepa_mae_vicreg_config_t config)
      : config_(std::move(config)), masker_(config_) {
    detail::validate_config(config_);
    tokenizer_ =
        register_module("tokenizer", TimeFrequencyViewBuilder(config_));
    target_tokenizer_ =
        register_module("target_tokenizer", TimeFrequencyViewBuilder(config_));
    encoder_ = register_module("encoder", SharedTokenEncoder(config_));
    target_encoder_ =
        register_module("target_encoder", SharedTokenEncoder(config_));
    predictor_ = register_module("predictor", LatentPredictor(config_));
    mae_decoder_ = register_module("mae_decoder", MaeDecoder(config_));
    vicreg_stability_head_ =
        register_module("vicreg_stability_head", VICRegStabilityHead(config_));
    this->to(config_.device, config_.dtype);
    update_target_network(0.0);
    for (auto &param : target_tokenizer_->parameters()) {
      param.set_requires_grad(false);
    }
    for (auto &param : target_encoder_->parameters()) {
      param.set_requires_grad(false);
    }
  }

  [[nodiscard]] const mtf_jepa_mae_vicreg_config_t &config() const {
    return config_;
  }

  [[nodiscard]] torch::Tensor
  project_vicreg(const torch::Tensor &pooled_latents) {
    TORCH_CHECK(pooled_latents.defined() &&
                    (pooled_latents.dim() == 2 || pooled_latents.dim() == 3),
                "[mtf_jepa_mae_vicreg] VICReg projection input must be [B,D] "
                "or [B,C,D]");
    if (pooled_latents.dim() == 2) {
      return vicreg_stability_head_->forward(pooled_latents).squeeze(1);
    }
    const int64_t batch_size = pooled_latents.size(0);
    const int64_t channel_count = pooled_latents.size(1);
    auto flattened = pooled_latents.reshape(
        {batch_size * channel_count, pooled_latents.size(2)});
    return vicreg_stability_head_->forward(flattened).squeeze(1).view(
        {batch_size, channel_count, config_.projector_dim});
  }

  [[nodiscard]] mtf_token_batch_t
  tokenize(const torch::Tensor &x,
           const torch::Tensor &feature_mask = torch::Tensor()) {
    return tokenizer_->forward(x, feature_mask);
  }

  [[nodiscard]] jepa_context_target_mask_t
  create_masks(const mtf_token_batch_t &batch) const {
    return masker_.create_masks(batch);
  }

  [[nodiscard]] mtf_jepa_mae_vicreg_encode_output_t
  encode(const torch::Tensor &x,
         const torch::Tensor &feature_mask = torch::Tensor()) {
    auto batch = tokenizer_->forward(x, feature_mask);
    auto embeddings = encoder_->forward(batch.tokens, batch.token_mask);
    mtf_jepa_mae_vicreg_encode_output_t out{};
    out.embeddings = embeddings;
    out.pooled_embedding = detail::masked_mean(embeddings, batch.token_mask);
    out.pooled_by_channel = detail::pooled_by_channel(
        embeddings, batch.token_mask, batch.metadata, config_);
    out.pooled_time = detail::masked_mean(
        embeddings, detail::domain_mask(batch.metadata, batch.token_mask, 0));
    out.pooled_frequency = detail::masked_mean(
        embeddings, detail::domain_mask(batch.metadata, batch.token_mask, 1));
    out.token_mask = batch.token_mask;
    out.sample_valid_mask = batch.token_mask.any(/*dim=*/1);
    out.channel_valid_mask =
        detail::channel_valid_mask(batch.metadata, batch.token_mask, config_);
    out.metadata = batch.metadata;
    return out;
  }

  [[nodiscard]] torch::Tensor
  target_encode(const torch::Tensor &x,
                const torch::Tensor &feature_mask = torch::Tensor()) {
    auto batch = tokenizer_->forward(x, feature_mask);
    auto fallback_latents = encoder_->forward(batch.tokens, batch.token_mask);
    auto target_latents =
        compute_target_latents(x, feature_mask, batch, fallback_latents);
    if (config_.stop_gradient_target) {
      target_latents = target_latents.detach();
    }
    return target_latents;
  }

  [[nodiscard]] mtf_jepa_mae_vicreg_output_t
  forward(const torch::Tensor &x,
          const torch::Tensor &feature_mask = torch::Tensor()) {
    auto batch = tokenizer_->forward(x, feature_mask);
    auto masks = masker_.create_masks(batch);

    auto full_latents = encoder_->forward(batch.tokens, batch.token_mask);
    auto context_tokens = batch.tokens.masked_fill(
        masks.context_mask.logical_not().unsqueeze(-1), 0.0);
    auto context_latents =
        encoder_->forward(context_tokens, masks.context_mask);

    auto target_latents =
        compute_target_latents(x, feature_mask, batch, full_latents);
    if (config_.stop_gradient_target) {
      target_latents = target_latents.detach();
    }

    const auto zero = torch::zeros({}, batch.tokens.options());
    const auto check_or_sanitize_loss = [&](const torch::Tensor &value) {
      return config_.strict_finite_loss ? value : detail::finite_or_zero(value);
    };
    auto pred_latents = predictor_->forward(context_latents, masks.context_mask,
                                            batch.metadata);
    auto loss_jepa = config_.use_jepa_loss
                         ? detail::masked_mse(pred_latents, target_latents,
                                              masks.target_mask)
                         : zero;
    loss_jepa = check_or_sanitize_loss(loss_jepa);

    auto decoded = mae_decoder_->forward(context_latents, masks.context_mask,
                                         batch.metadata);
    auto loss_mae = zero;
    auto loss_mae_time = zero;
    auto loss_mae_frequency = zero;
    int64_t mae_time_active_dims = 0;
    int64_t mae_frequency_active_dims = 0;
    if (config_.use_mae_decoder) {
      if (config_.use_raw_reconstruction_targets) {
        const auto time_target_mask = masks.target_mask.logical_and(
            batch.metadata.domain_id.to(masks.target_mask.device())
                .eq(0)
                .unsqueeze(0));
        const auto frequency_target_mask = masks.target_mask.logical_and(
            batch.metadata.domain_id.to(masks.target_mask.device())
                .eq(1)
                .unsqueeze(0));
        mae_time_active_dims =
            time_target_mask.to(torch::kBool)
                .unsqueeze(-1)
                .logical_and(batch.time_reconstruction_mask.to(torch::kBool))
                .sum()
                .item<int64_t>();
        mae_frequency_active_dims =
            frequency_target_mask.to(torch::kBool)
                .unsqueeze(-1)
                .logical_and(
                    batch.frequency_reconstruction_mask.to(torch::kBool))
                .sum()
                .item<int64_t>();
        loss_mae_time = detail::masked_weighted_mse(
            decoded.time, batch.time_reconstruction_targets.detach(),
            time_target_mask, batch.time_reconstruction_mask);
        loss_mae_frequency = detail::masked_weighted_mse(
            decoded.frequency, batch.frequency_reconstruction_targets.detach(),
            frequency_target_mask, batch.frequency_reconstruction_mask);
        loss_mae = loss_mae_time + loss_mae_frequency;
      } else {
        loss_mae = detail::masked_mse(decoded.projected,
                                      batch.reconstruction_targets.detach(),
                                      masks.target_mask);
      }
    }
    loss_mae_time = check_or_sanitize_loss(loss_mae_time);
    loss_mae_frequency = check_or_sanitize_loss(loss_mae_frequency);
    loss_mae = check_or_sanitize_loss(loss_mae);

    tf_alignment_result_t tf_alignment{};
    tf_alignment.loss = zero;
    if (config_.use_tf_align_loss && config_.use_frequency_tokens) {
      tf_alignment =
          compute_pairwise_time_frequency_alignment(full_latents, batch);
    }
    auto loss_tf_align = tf_alignment.loss;
    loss_tf_align = check_or_sanitize_loss(loss_tf_align);

    torch::Tensor loss_vicreg = zero;
    torch::Tensor vicreg_sim = zero;
    torch::Tensor vicreg_var = zero;
    torch::Tensor vicreg_cov = zero;
    torch::Tensor loss_vicreg_global = zero;
    torch::Tensor loss_vicreg_channel = zero;
    torch::Tensor vicreg_global_sim = zero;
    torch::Tensor vicreg_global_var = zero;
    torch::Tensor vicreg_global_cov = zero;
    torch::Tensor vicreg_channel_sim = zero;
    torch::Tensor vicreg_channel_var = zero;
    torch::Tensor vicreg_channel_cov = zero;
    torch::Tensor vicreg_drawn_a_data{};
    torch::Tensor vicreg_drawn_a_feature_mask{};
    torch::Tensor vicreg_drawn_b_data{};
    torch::Tensor vicreg_drawn_b_feature_mask{};
    torch::Tensor vicreg_view_a_data{};
    torch::Tensor vicreg_view_a_feature_mask{};
    torch::Tensor vicreg_view_b_data{};
    torch::Tensor vicreg_view_b_feature_mask{};
    torch::Tensor vicreg_view_a_token_mask{};
    torch::Tensor vicreg_view_b_token_mask{};
    torch::Tensor vicreg_view_a_sample_valid_mask{};
    torch::Tensor vicreg_view_b_sample_valid_mask{};
    torch::Tensor vicreg_view_a_pooled_by_channel{};
    torch::Tensor vicreg_view_b_pooled_by_channel{};
    torch::Tensor vicreg_view_a_pooled_global{};
    torch::Tensor vicreg_view_b_pooled_global{};
    torch::Tensor vicreg_view_a_projected_global{};
    torch::Tensor vicreg_view_b_projected_global{};
    torch::Tensor vicreg_global_joint_mask{};
    torch::Tensor vicreg_channel_joint_mask{};
    int64_t vicreg_encoder_call_count = 0;
    int64_t vicreg_projector_call_count = 0;
    int64_t vicreg_valid_rows = 0;
    int64_t vicreg_global_valid_rows = 0;
    int64_t vicreg_channel_valid_rows = 0;
    int64_t vicreg_channel_active_groups = 0;
    int64_t vicreg_mode = (config_.use_global_vicreg ? 1 : 0) +
                          (config_.use_channel_vicreg ? 2 : 0);
    if (config_.use_vicreg_loss) {
      const auto vicreg_result = compute_vicreg_branch(x, feature_mask);
      loss_vicreg = vicreg_result.loss;
      loss_vicreg_global = vicreg_result.global_loss;
      loss_vicreg_channel = vicreg_result.channel_loss;
      vicreg_sim = vicreg_result.invariance_loss;
      vicreg_var = vicreg_result.variance_loss;
      vicreg_cov = vicreg_result.covariance_loss;
      vicreg_global_sim = vicreg_result.global_invariance_loss;
      vicreg_global_var = vicreg_result.global_variance_loss;
      vicreg_global_cov = vicreg_result.global_covariance_loss;
      vicreg_channel_sim = vicreg_result.channel_invariance_loss;
      vicreg_channel_var = vicreg_result.channel_variance_loss;
      vicreg_channel_cov = vicreg_result.channel_covariance_loss;
      vicreg_drawn_a_data = vicreg_result.drawn_a_data;
      vicreg_drawn_a_feature_mask = vicreg_result.drawn_a_feature_mask;
      vicreg_drawn_b_data = vicreg_result.drawn_b_data;
      vicreg_drawn_b_feature_mask = vicreg_result.drawn_b_feature_mask;
      vicreg_view_a_data = vicreg_result.view_a_data;
      vicreg_view_a_feature_mask = vicreg_result.view_a_feature_mask;
      vicreg_view_b_data = vicreg_result.view_b_data;
      vicreg_view_b_feature_mask = vicreg_result.view_b_feature_mask;
      vicreg_view_a_token_mask = vicreg_result.view_a_token_mask;
      vicreg_view_b_token_mask = vicreg_result.view_b_token_mask;
      vicreg_view_a_sample_valid_mask =
          vicreg_result.view_a_sample_valid_mask;
      vicreg_view_b_sample_valid_mask =
          vicreg_result.view_b_sample_valid_mask;
      vicreg_view_a_pooled_by_channel = vicreg_result.view_a_pooled_by_channel;
      vicreg_view_b_pooled_by_channel = vicreg_result.view_b_pooled_by_channel;
      vicreg_view_a_pooled_global = vicreg_result.view_a_pooled_global;
      vicreg_view_b_pooled_global = vicreg_result.view_b_pooled_global;
      vicreg_view_a_projected_global =
          vicreg_result.view_a_projected_global;
      vicreg_view_b_projected_global =
          vicreg_result.view_b_projected_global;
      vicreg_global_joint_mask = vicreg_result.global_joint_mask;
      vicreg_channel_joint_mask = vicreg_result.channel_joint_mask;
      if (config_.return_vicreg_debug_tensors) {
        vicreg_encoder_call_count = vicreg_result.encoder_call_count;
        vicreg_projector_call_count = vicreg_result.projector_call_count;
      }
      vicreg_valid_rows = vicreg_result.valid_rows;
      vicreg_global_valid_rows = vicreg_result.global_valid_rows;
      vicreg_channel_valid_rows = vicreg_result.channel_valid_rows;
      vicreg_channel_active_groups = vicreg_result.channel_active_groups;
    }
    loss_vicreg = check_or_sanitize_loss(loss_vicreg);

    auto total_loss = config_.lambda_jepa * loss_jepa +
                      config_.lambda_mae * loss_mae +
                      config_.lambda_tf_align * loss_tf_align +
                      config_.lambda_vicreg * loss_vicreg;
    if (config_.strict_finite_loss) {
      const auto all_losses_finite =
          torch::stack({loss_jepa, loss_mae_time, loss_mae_frequency, loss_mae,
                        loss_tf_align, loss_vicreg, total_loss})
              .isfinite()
              .all()
              .template item<bool>();
      TORCH_CHECK(all_losses_finite,
                  "[mtf_jepa_mae_vicreg] non-finite loss");
    } else {
      total_loss = detail::finite_or_zero(total_loss);
    }

    mtf_jepa_mae_vicreg_output_t out{};
    out.embeddings = full_latents;
    out.pooled_embedding = detail::masked_mean(full_latents, batch.token_mask);
    out.pooled_by_channel = detail::pooled_by_channel(
        full_latents, batch.token_mask, batch.metadata, config_);
    out.pooled_time = detail::masked_mean(
        full_latents, detail::domain_mask(batch.metadata, batch.token_mask, 0));
    out.pooled_frequency = detail::masked_mean(
        full_latents, detail::domain_mask(batch.metadata, batch.token_mask, 1));
    out.loss = total_loss;
    out.loss_jepa = detail::finite_or_zero(loss_jepa);
    out.loss_mae = detail::finite_or_zero(loss_mae);
    out.loss_mae_time = detail::finite_or_zero(loss_mae_time);
    out.loss_mae_frequency = detail::finite_or_zero(loss_mae_frequency);
    out.loss_tf_align = detail::finite_or_zero(loss_tf_align);
    out.loss_vicreg = detail::finite_or_zero(loss_vicreg);
    out.loss_vicreg_global = detail::finite_or_zero(loss_vicreg_global);
    out.loss_vicreg_channel = detail::finite_or_zero(loss_vicreg_channel);
    if (config_.return_vicreg_debug_tensors) {
      out.jepa_target_mask = masks.target_mask.detach();
      out.jepa_context_mask = masks.context_mask.detach();
    }
    out.vicreg_drawn_a_data = vicreg_drawn_a_data;
    out.vicreg_drawn_a_feature_mask = vicreg_drawn_a_feature_mask;
    out.vicreg_drawn_b_data = vicreg_drawn_b_data;
    out.vicreg_drawn_b_feature_mask = vicreg_drawn_b_feature_mask;
    out.vicreg_view_a_data = vicreg_view_a_data;
    out.vicreg_view_a_feature_mask = vicreg_view_a_feature_mask;
    out.vicreg_view_b_data = vicreg_view_b_data;
    out.vicreg_view_b_feature_mask = vicreg_view_b_feature_mask;
    out.vicreg_view_a_token_mask = vicreg_view_a_token_mask;
    out.vicreg_view_b_token_mask = vicreg_view_b_token_mask;
    out.vicreg_view_a_sample_valid_mask = vicreg_view_a_sample_valid_mask;
    out.vicreg_view_b_sample_valid_mask = vicreg_view_b_sample_valid_mask;
    out.vicreg_view_a_pooled_by_channel = vicreg_view_a_pooled_by_channel;
    out.vicreg_view_b_pooled_by_channel = vicreg_view_b_pooled_by_channel;
    out.vicreg_view_a_pooled_global = vicreg_view_a_pooled_global;
    out.vicreg_view_b_pooled_global = vicreg_view_b_pooled_global;
    out.vicreg_view_a_projected_global = vicreg_view_a_projected_global;
    out.vicreg_view_b_projected_global = vicreg_view_b_projected_global;
    out.vicreg_global_joint_mask = vicreg_global_joint_mask;
    out.vicreg_channel_joint_mask = vicreg_channel_joint_mask;
    out.vicreg_encoder_call_count = vicreg_encoder_call_count;
    out.vicreg_projector_call_count = vicreg_projector_call_count;
    out.sample_valid_mask = batch.token_mask.any(/*dim=*/1);
    out.channel_valid_mask =
        detail::channel_valid_mask(batch.metadata, batch.token_mask, config_);
    if (config_.return_diagnostics) {
      out.diagnostics["total_loss"] = out.loss;
      out.diagnostics["loss_jepa"] = out.loss_jepa;
      out.diagnostics["loss_mae"] = out.loss_mae;
      out.diagnostics["loss_mae_time"] = out.loss_mae_time;
      out.diagnostics["loss_mae_frequency"] = out.loss_mae_frequency;
      out.diagnostics["mae_time_active_dims"] = torch::tensor(
          static_cast<double>(mae_time_active_dims), batch.tokens.options());
      out.diagnostics["mae_frequency_active_dims"] =
          torch::tensor(static_cast<double>(mae_frequency_active_dims),
                        batch.tokens.options());
      out.diagnostics["loss_tf_align"] = out.loss_tf_align;
      out.diagnostics["loss_vicreg"] = out.loss_vicreg;
      out.diagnostics["loss_vicreg_global"] = out.loss_vicreg_global;
      out.diagnostics["loss_vicreg_channel"] = out.loss_vicreg_channel;
      out.diagnostics["mask_ratio_actual"] =
          masks.mask_ratio_actual.to(batch.tokens.device());
      out.diagnostics["num_context_tokens"] =
          torch::tensor(static_cast<double>(masks.num_context_tokens),
                        batch.tokens.options());
      out.diagnostics["num_target_tokens"] = torch::tensor(
          static_cast<double>(masks.num_target_tokens), batch.tokens.options());
      out.diagnostics["hard_forbidden_count"] =
          torch::tensor(static_cast<double>(masks.hard_forbidden_count),
                        batch.tokens.options());
      out.diagnostics["soft_forbidden_count"] =
          torch::tensor(static_cast<double>(masks.soft_forbidden_count),
                        batch.tokens.options());
      out.diagnostics["relaxed_soft_forbidden_count"] =
          torch::tensor(static_cast<double>(masks.relaxed_soft_forbidden_count),
                        batch.tokens.options());
      out.diagnostics["reduced_targets_for_context_count"] = torch::tensor(
          static_cast<double>(masks.reduced_targets_for_context_count),
          batch.tokens.options());
      out.diagnostics["context_starved_sample_count"] =
          torch::tensor(static_cast<double>(masks.context_starved_sample_count),
                        batch.tokens.options());
      out.diagnostics["min_context_satisfied_count"] =
          torch::tensor(static_cast<double>(masks.min_context_satisfied_count),
                        batch.tokens.options());
      out.diagnostics["tf_pair_count"] = torch::tensor(
          static_cast<double>(tf_alignment.pair_count), batch.tokens.options());
      out.diagnostics["tf_pair_valid_count"] =
          torch::tensor(static_cast<double>(tf_alignment.pair_valid_count),
                        batch.tokens.options());
      out.diagnostics["loss_tf_align_pairwise"] = out.loss_tf_align;
      const auto valid_latents =
          detail::valid_token_rows(full_latents, batch.token_mask);
      out.diagnostics["valid_latent_rows"] = torch::tensor(
          static_cast<double>(valid_latents.size(0)), batch.tokens.options());
      if (valid_latents.size(0) > 0) {
        out.diagnostics["latent_std"] =
            valid_latents.var(/*dim=*/0, /*unbiased=*/false)
                .clamp_min(0.0)
                .sqrt()
                .mean();
        out.diagnostics["latent_norm"] =
            valid_latents.norm(2, /*dim=*/-1).mean();
      } else {
        out.diagnostics["latent_std"] = zero;
        out.diagnostics["latent_norm"] = zero;
      }
      out.diagnostics["vicreg_sim_term"] = detail::finite_or_zero(vicreg_sim);
      out.diagnostics["vicreg_var_term"] = detail::finite_or_zero(vicreg_var);
      out.diagnostics["vicreg_cov_term"] = detail::finite_or_zero(vicreg_cov);
      out.diagnostics["vicreg_global_sim_term"] =
          detail::finite_or_zero(vicreg_global_sim);
      out.diagnostics["vicreg_global_var_term"] =
          detail::finite_or_zero(vicreg_global_var);
      out.diagnostics["vicreg_global_cov_term"] =
          detail::finite_or_zero(vicreg_global_cov);
      out.diagnostics["vicreg_channel_sim_term"] =
          detail::finite_or_zero(vicreg_channel_sim);
      out.diagnostics["vicreg_channel_var_term"] =
          detail::finite_or_zero(vicreg_channel_var);
      out.diagnostics["vicreg_channel_cov_term"] =
          detail::finite_or_zero(vicreg_channel_cov);
      out.diagnostics["vicreg_mode"] = torch::tensor(
          static_cast<double>(vicreg_mode), batch.tokens.options());
      out.diagnostics["vicreg_channel_stratified"] =
          torch::tensor(config_.stratify_channel_vicreg_by_channel ? 1.0 : 0.0,
                        batch.tokens.options());
      out.diagnostics["vicreg_valid_rows"] = torch::tensor(
          static_cast<double>(vicreg_valid_rows), batch.tokens.options());
      out.diagnostics["vicreg_global_valid_rows"] =
          torch::tensor(static_cast<double>(vicreg_global_valid_rows),
                        batch.tokens.options());
      out.diagnostics["vicreg_channel_valid_rows"] =
          torch::tensor(static_cast<double>(vicreg_channel_valid_rows),
                        batch.tokens.options());
      out.diagnostics["vicreg_channel_active_groups"] =
          torch::tensor(static_cast<double>(vicreg_channel_active_groups),
                        batch.tokens.options());
      out.diagnostics["sample_valid_count"] =
          out.sample_valid_mask.sum().to(batch.tokens.scalar_type());
      out.diagnostics["channel_valid_count"] =
          out.channel_valid_mask.sum().to(batch.tokens.scalar_type());
      out.diagnostics["target_ema_distance"] = target_ema_distance();
    }
    return out;
  }

  bool update_target_encoder(double tau = -1.0) {
    return update_target_network(tau);
  }

  bool update_target_network(double tau = -1.0) {
    if (!config_.use_target_ema && tau < 0.0) {
      return false;
    }
    if (tau < 0.0) {
      tau = config_.target_ema_tau;
    }
    TORCH_CHECK(tau >= 0.0 && tau <= 1.0,
                "[mtf_jepa_mae_vicreg] EMA tau must be in [0,1]");
    torch::NoGradGuard no_grad;
    update_ema_parameters(tokenizer_->parameters(),
                          target_tokenizer_->parameters(), tau,
                          "target tokenizer");
    update_ema_parameters(encoder_->parameters(), target_encoder_->parameters(),
                          tau, "target encoder");
    return true;
  }

  [[nodiscard]] torch::Tensor target_ema_distance() {
    auto total = torch::zeros(
        {}, torch::TensorOptions().dtype(config_.dtype).device(config_.device));
    int64_t count = 0;
    torch::NoGradGuard no_grad;
    auto accumulate = [&](std::vector<torch::Tensor> online_params,
                          std::vector<torch::Tensor> target_params) {
      TORCH_CHECK(online_params.size() == target_params.size(),
                  "[mtf_jepa_mae_vicreg] EMA distance parameter mismatch");
      for (std::size_t i = 0; i < online_params.size(); ++i) {
        total = total + (online_params[i].detach() - target_params[i].detach())
                            .pow(2)
                            .mean();
        ++count;
      }
    };
    accumulate(tokenizer_->parameters(), target_tokenizer_->parameters());
    accumulate(encoder_->parameters(), target_encoder_->parameters());
    if (count <= 0) {
      return total;
    }
    return total / static_cast<double>(count);
  }

private:
  static void update_ema_parameters(std::vector<torch::Tensor> online_params,
                                    std::vector<torch::Tensor> target_params,
                                    double tau, const char *label) {
    TORCH_CHECK(online_params.size() == target_params.size(),
                "[mtf_jepa_mae_vicreg] ", label, " parameter mismatch");
    for (std::size_t i = 0; i < online_params.size(); ++i) {
      target_params[i].mul_(tau);
      target_params[i].add_(online_params[i].detach(), 1.0 - tau);
    }
  }

  static void assert_matching_token_layout(const mtf_token_batch_t &online,
                                           const mtf_token_batch_t &target) {
    TORCH_CHECK(online.tokens.sizes() == target.tokens.sizes(),
                "[mtf_jepa_mae_vicreg] target tokenizer token shape mismatch");
    TORCH_CHECK(online.token_mask.sizes() == target.token_mask.sizes(),
                "[mtf_jepa_mae_vicreg] target tokenizer mask shape mismatch");
    const auto layout_exact =
        torch::stack({
            torch::eq(online.token_mask, target.token_mask).all(),
            torch::eq(online.metadata.start_index,
                      target.metadata.start_index)
                .all(),
            torch::eq(online.metadata.width, target.metadata.width).all(),
            torch::eq(online.metadata.scale_id, target.metadata.scale_id).all(),
            torch::eq(online.metadata.channel_id,
                      target.metadata.channel_id)
                .all(),
            torch::eq(online.metadata.domain_id, target.metadata.domain_id)
                .all()})
            .all()
            .template item<bool>();
    TORCH_CHECK(layout_exact,
                "[mtf_jepa_mae_vicreg] target tokenizer layout mismatch");
  }

  [[nodiscard]] torch::Tensor
  compute_target_latents(const torch::Tensor &x,
                         const torch::Tensor &feature_mask,
                         const mtf_token_batch_t &online_batch,
                         const torch::Tensor &fallback_latents) {
    if (!config_.use_target_ema) {
      return fallback_latents;
    }
    torch::NoGradGuard no_grad;
    const bool tokenizer_was_training = target_tokenizer_->is_training();
    const bool encoder_was_training = target_encoder_->is_training();
    target_tokenizer_->eval();
    target_encoder_->eval();
    const auto target_batch = target_tokenizer_->forward(x, feature_mask);
    assert_matching_token_layout(online_batch, target_batch);
    auto target_latents = target_encoder_->forward(target_batch.tokens.detach(),
                                                   target_batch.token_mask);
    target_tokenizer_->train(tokenizer_was_training);
    target_encoder_->train(encoder_was_training);
    return target_latents;
  }

  [[nodiscard]] tf_alignment_result_t
  compute_pairwise_time_frequency_alignment(const torch::Tensor &latents,
                                            const mtf_token_batch_t &batch) {
    auto domain_cpu = batch.metadata.domain_id.to(torch::kCPU).contiguous();
    auto channel_cpu = batch.metadata.channel_id.to(torch::kCPU).contiguous();
    auto scale_cpu = batch.metadata.scale_id.to(torch::kCPU).contiguous();
    auto start_cpu = batch.metadata.start_index.to(torch::kCPU).contiguous();
    auto width_cpu = batch.metadata.width.to(torch::kCPU).contiguous();
    auto domain = domain_cpu.accessor<int64_t, 1>();
    auto channel = channel_cpu.accessor<int64_t, 1>();
    auto scale = scale_cpu.accessor<int64_t, 1>();
    auto start = start_cpu.accessor<int64_t, 1>();
    auto width = width_cpu.accessor<int64_t, 1>();

    std::vector<int64_t> time_indices;
    std::vector<int64_t> frequency_indices;
    const int64_t N = batch.metadata.domain_id.size(0);
    for (int64_t i = 0; i < N; ++i) {
      if (domain[i] != 0) {
        continue;
      }
      for (int64_t j = 0; j < N; ++j) {
        if (domain[j] == 1 && channel[i] == channel[j] &&
            scale[i] == scale[j] && start[i] == start[j] &&
            width[i] == width[j]) {
          time_indices.push_back(i);
          frequency_indices.push_back(j);
          break;
        }
      }
    }
    tf_alignment_result_t out{};
    out.loss = torch::zeros({}, latents.options());
    out.pair_count = static_cast<int64_t>(time_indices.size());
    if (time_indices.empty()) {
      return out;
    }
    const auto idx_options =
        torch::TensorOptions().dtype(torch::kInt64).device(config_.device);
    auto time_idx = torch::tensor(time_indices, idx_options);
    auto frequency_idx = torch::tensor(frequency_indices, idx_options);
    auto time_latents = latents.index_select(/*dim=*/1, time_idx);
    auto frequency_latents = latents.index_select(/*dim=*/1, frequency_idx);
    auto time_mask = batch.token_mask.index_select(/*dim=*/1, time_idx);
    auto frequency_mask =
        batch.token_mask.index_select(/*dim=*/1, frequency_idx);
    auto pair_mask = time_mask.logical_and(frequency_mask);
    if (!pair_mask.any().template item<bool>()) {
      return out;
    }
    out.pair_valid_count = pair_mask.sum().item<int64_t>();
    auto pair_loss =
        1.0 - torch::cosine_similarity(time_latents, frequency_latents,
                                       /*dim=*/-1, /*eps=*/1e-8);
    const auto weights = pair_mask.to(pair_loss.dtype());
    out.loss = (pair_loss * weights).sum() / weights.sum().clamp_min(1.0);
    return out;
  }

  [[nodiscard]] mtf_input_t weak_augment(const torch::Tensor &x,
                                         const torch::Tensor &feature_mask) {
    return detail::apply_vicreg_weak_view_augmentation(x, feature_mask,
                                                       config_);
  }

  [[nodiscard]] vicreg_branch_loss_result_t
  compute_vicreg_branch(const torch::Tensor &x,
                        const torch::Tensor &feature_mask) {
    // Every policy consumes the two production weak-view draws first.  The
    // experiment-only pairing policy substitutes the used views only after
    // those draws, preserving the default RNG schedule and forward topology.
    const auto drawn_a = weak_augment(x, feature_mask);
    const auto drawn_b = weak_augment(x, feature_mask);
    const auto clean = detail::canonicalize_input(x, feature_mask, config_);
    mtf_input_t view_a{};
    mtf_input_t view_b{};
    switch (config_.vicreg_view_pairing_policy) {
    case mtf_vicreg_view_pairing_policy_t::independent_weak:
      view_a = drawn_a;
      view_b = drawn_b;
      break;
    case mtf_vicreg_view_pairing_policy_t::tied_weak:
      view_a = drawn_a;
      view_b = {drawn_a.data.clone(), drawn_a.feature_mask.clone()};
      break;
    case mtf_vicreg_view_pairing_policy_t::clean_identical:
      view_a = clean;
      view_b = {clean.data.clone(), clean.feature_mask.clone()};
      break;
    }
    auto encoded_a = encode(view_a.data, view_a.feature_mask);
    auto encoded_b = encode(view_b.data, view_b.feature_mask);
    auto zero = torch::zeros(
        {}, torch::TensorOptions().dtype(config_.dtype).device(config_.device));
    vicreg_branch_loss_result_t out{};
    out.encoder_call_count = 2;
    out.loss = zero;
    out.global_loss = zero;
    out.channel_loss = zero;
    out.invariance_loss = zero;
    out.variance_loss = zero;
    out.covariance_loss = zero;
    out.global_invariance_loss = zero;
    out.global_variance_loss = zero;
    out.global_covariance_loss = zero;
    out.channel_invariance_loss = zero;
    out.channel_variance_loss = zero;
    out.channel_covariance_loss = zero;
    torch::Tensor channel_joint_mask{};
    if (config_.use_channel_vicreg || config_.return_vicreg_debug_tensors) {
      channel_joint_mask =
          detail::channel_valid_mask(encoded_a.metadata, encoded_a.token_mask,
                                     config_)
              .logical_and(detail::channel_valid_mask(
                  encoded_b.metadata, encoded_b.token_mask, config_))
              .to(torch::TensorOptions()
                      .dtype(torch::kBool)
                      .device(config_.device));
    }
    if (config_.return_vicreg_debug_tensors) {
      out.drawn_a_data = drawn_a.data.detach();
      out.drawn_a_feature_mask = drawn_a.feature_mask.detach();
      out.drawn_b_data = drawn_b.data.detach();
      out.drawn_b_feature_mask = drawn_b.feature_mask.detach();
      out.view_a_data = view_a.data.detach();
      out.view_a_feature_mask = view_a.feature_mask.detach();
      out.view_b_data = view_b.data.detach();
      out.view_b_feature_mask = view_b.feature_mask.detach();
      out.view_a_token_mask = encoded_a.token_mask.detach();
      out.view_b_token_mask = encoded_b.token_mask.detach();
      out.view_a_sample_valid_mask = encoded_a.sample_valid_mask.detach();
      out.view_b_sample_valid_mask = encoded_b.sample_valid_mask.detach();
      out.view_a_pooled_by_channel = encoded_a.pooled_by_channel;
      out.view_b_pooled_by_channel = encoded_b.pooled_by_channel;
      out.channel_joint_mask = channel_joint_mask.detach();
    }
    vicreg_stability_loss_options_t opts{};
    opts.invariance_weight = config_.vicreg_sim_weight;
    opts.variance_weight = config_.vicreg_var_weight;
    opts.covariance_weight = config_.vicreg_cov_weight;
    opts.variance_floor = config_.vicreg_variance_floor;
    opts.eps = config_.vicreg_variance_epsilon;
    if (config_.use_global_vicreg) {
      auto projected_a =
          project_vicreg(encoded_a.pooled_embedding).unsqueeze(1);
      auto projected_b =
          project_vicreg(encoded_b.pooled_embedding).unsqueeze(1);
      out.projector_call_count += 2;
      auto mask = encoded_a.token_mask.any(/*dim=*/1)
                      .logical_and(encoded_b.token_mask.any(/*dim=*/1))
                      .unsqueeze(1)
                      .to(torch::TensorOptions()
                              .dtype(torch::kBool)
                              .device(config_.device));
      const auto global_result = compute_vicreg_stability_loss(
          projected_a, mask, projected_b, mask, opts);
      if (config_.return_vicreg_debug_tensors) {
        out.view_a_pooled_global = encoded_a.pooled_embedding;
        out.view_b_pooled_global = encoded_b.pooled_embedding;
        out.view_a_projected_global = projected_a;
        out.view_b_projected_global = projected_b;
        out.global_joint_mask = mask.detach();
      }
      out.global_loss = global_result.loss;
      out.global_invariance_loss = global_result.invariance_loss;
      out.global_variance_loss = global_result.variance_loss;
      out.global_covariance_loss = global_result.covariance_loss;
      out.loss = out.loss + config_.lambda_global_vicreg * global_result.loss;
      out.invariance_loss =
          out.invariance_loss +
          config_.lambda_global_vicreg * global_result.invariance_loss;
      out.variance_loss = out.variance_loss + config_.lambda_global_vicreg *
                                                  global_result.variance_loss;
      out.covariance_loss =
          out.covariance_loss +
          config_.lambda_global_vicreg * global_result.covariance_loss;
      out.global_valid_rows = global_result.valid_rows;
      out.valid_rows += global_result.valid_rows;
    }
    if (config_.use_channel_vicreg) {
      auto projected_a = project_vicreg(encoded_a.pooled_by_channel);
      auto projected_b = project_vicreg(encoded_b.pooled_by_channel);
      out.projector_call_count += 2;
      const auto channel_result =
          config_.stratify_channel_vicreg_by_channel
              ? compute_channel_stratified_vicreg_stability_loss(
                    projected_a, channel_joint_mask, projected_b,
                    channel_joint_mask, opts)
              : compute_vicreg_stability_loss(projected_a, channel_joint_mask,
                                              projected_b, channel_joint_mask,
                                              opts);
      out.channel_loss = channel_result.loss;
      out.channel_invariance_loss = channel_result.invariance_loss;
      out.channel_variance_loss = channel_result.variance_loss;
      out.channel_covariance_loss = channel_result.covariance_loss;
      out.loss = out.loss + config_.lambda_channel_vicreg * channel_result.loss;
      out.invariance_loss =
          out.invariance_loss +
          config_.lambda_channel_vicreg * channel_result.invariance_loss;
      out.variance_loss = out.variance_loss + config_.lambda_channel_vicreg *
                                                  channel_result.variance_loss;
      out.covariance_loss =
          out.covariance_loss +
          config_.lambda_channel_vicreg * channel_result.covariance_loss;
      out.channel_valid_rows = channel_result.valid_rows;
      out.channel_active_groups = channel_result.active_groups;
      out.valid_rows += channel_result.valid_rows;
    }
    return out;
  }

  mtf_jepa_mae_vicreg_config_t config_{};
  TimeFrequencyViewBuilder tokenizer_{nullptr};
  TimeFrequencyViewBuilder target_tokenizer_{nullptr};
  SharedTokenEncoder encoder_{nullptr};
  SharedTokenEncoder target_encoder_{nullptr};
  LatentPredictor predictor_{nullptr};
  MaeDecoder mae_decoder_{nullptr};
  VICRegStabilityHead vicreg_stability_head_{nullptr};
  JEPAContextTargetMasker masker_;
};

TORCH_MODULE(MtfJepaMaeVicreg);

} // namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg
