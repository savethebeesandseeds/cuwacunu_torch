// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

struct channel_context_mdn_train_options_t {
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions nll{};
  double grad_clip_norm{0.0};
  bool skip_non_finite_loss{true};
  double edge_return_auxiliary_loss_weight{0.0};
  double edge_return_auxiliary_direction_weight{0.0};
  double edge_return_auxiliary_rank_weight{0.0};
  double edge_return_auxiliary_huber_beta{0.01};
  double edge_return_auxiliary_logit_scale{50.0};
  bool direct_edge_return_readout_enabled{false};
  double direct_edge_return_readout_loss_weight{0.0};
  double direct_edge_return_readout_direction_weight{0.0};
  double direct_edge_return_readout_rank_weight{0.0};
  double direct_edge_return_readout_huber_beta{0.01};
  double direct_edge_return_readout_logit_scale{50.0};
};

struct channel_context_mdn_train_step_result_t {
  torch::Tensor loss{};
  torch::Tensor nll{};
  torch::Tensor nll_per_channel{};
  torch::Tensor nll_per_target_feature{};
  torch::Tensor nll_per_channel_target_feature{};
  torch::Tensor valid_count_per_channel{};
  torch::Tensor valid_count_per_target_feature{};
  torch::Tensor valid_count_per_channel_target_feature{};
  torch::Tensor mixture_usage{};
  torch::Tensor edge_return_auxiliary_loss{};
  torch::Tensor edge_return_auxiliary_regression_loss{};
  torch::Tensor edge_return_auxiliary_direction_loss{};
  torch::Tensor edge_return_auxiliary_rank_loss{};
  int64_t edge_return_auxiliary_valid_count{0};
  int64_t edge_return_auxiliary_pairwise_valid_count{0};
  torch::Tensor direct_edge_return_readout_loss{};
  torch::Tensor direct_edge_return_readout_regression_loss{};
  torch::Tensor direct_edge_return_readout_direction_loss{};
  torch::Tensor direct_edge_return_readout_rank_loss{};
  int64_t direct_edge_return_readout_loss_valid_count{0};
  int64_t direct_edge_return_readout_loss_pairwise_valid_count{0};
  int64_t valid_target_count{0};
  double valid_target_fraction{0.0};
  double sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double sigma_mean_valid{std::numeric_limits<double>::quiet_NaN()};
  double sigma_min_valid{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max_valid{std::numeric_limits<double>::quiet_NaN()};
  double mixture_entropy{std::numeric_limits<double>::quiet_NaN()};
  int64_t nonfinite_output_count{0};
  int64_t ev_metric_valid_count{0};
  int64_t ev_direction_correct_count{0};
  double ev_abs_error_sum{0.0};
  double ev_squared_error_sum{0.0};
  double ev_signed_error_sum{0.0};
  torch::Tensor ev_valid_count_per_channel{};
  torch::Tensor ev_direction_correct_count_per_channel{};
  torch::Tensor ev_abs_error_sum_per_channel{};
  torch::Tensor ev_squared_error_sum_per_channel{};
  torch::Tensor ev_signed_error_sum_per_channel{};
  torch::Tensor ev_valid_count_per_target_feature{};
  torch::Tensor ev_direction_correct_count_per_target_feature{};
  torch::Tensor ev_abs_error_sum_per_target_feature{};
  torch::Tensor ev_squared_error_sum_per_target_feature{};
  torch::Tensor ev_signed_error_sum_per_target_feature{};
  torch::Tensor ev_valid_count_per_channel_target_feature{};
  torch::Tensor ev_direction_correct_count_per_channel_target_feature{};
  torch::Tensor ev_abs_error_sum_per_channel_target_feature{};
  torch::Tensor ev_squared_error_sum_per_channel_target_feature{};
  torch::Tensor ev_signed_error_sum_per_channel_target_feature{};
  torch::Tensor ev_valid_count_per_node{};
  torch::Tensor ev_direction_correct_count_per_node{};
  torch::Tensor ev_abs_error_sum_per_node{};
  torch::Tensor ev_squared_error_sum_per_node{};
  torch::Tensor ev_signed_error_sum_per_node{};
  int64_t edge_return_projection_valid_count{0};
  int64_t edge_return_projection_direction_correct_count{0};
  double edge_return_projection_abs_error_sum{0.0};
  double edge_return_projection_squared_error_sum{0.0};
  double edge_return_projection_signed_error_sum{0.0};
  double edge_return_projection_pred_sum{0.0};
  double edge_return_projection_realized_sum{0.0};
  double edge_return_projection_pred_squared_sum{0.0};
  double edge_return_projection_realized_squared_sum{0.0};
  double edge_return_projection_cross_sum{0.0};
  int64_t edge_return_projection_pairwise_rank_valid_count{0};
  int64_t edge_return_projection_pairwise_rank_correct_count{0};
  int64_t edge_return_projection_best_asset_valid_count{0};
  int64_t edge_return_projection_best_asset_correct_count{0};
  torch::Tensor edge_return_projection_valid_count_per_edge{};
  torch::Tensor edge_return_projection_direction_correct_count_per_edge{};
  torch::Tensor edge_return_projection_valid_count_per_channel{};
  torch::Tensor edge_return_projection_direction_correct_count_per_channel{};
  int64_t direct_edge_return_readout_valid_count{0};
  int64_t direct_edge_return_readout_direction_correct_count{0};
  double direct_edge_return_readout_abs_error_sum{0.0};
  double direct_edge_return_readout_squared_error_sum{0.0};
  double direct_edge_return_readout_signed_error_sum{0.0};
  double direct_edge_return_readout_pred_sum{0.0};
  double direct_edge_return_readout_realized_sum{0.0};
  double direct_edge_return_readout_pred_squared_sum{0.0};
  double direct_edge_return_readout_realized_squared_sum{0.0};
  double direct_edge_return_readout_cross_sum{0.0};
  int64_t direct_edge_return_readout_pairwise_rank_valid_count{0};
  int64_t direct_edge_return_readout_pairwise_rank_correct_count{0};
  int64_t direct_edge_return_readout_best_asset_valid_count{0};
  int64_t direct_edge_return_readout_best_asset_correct_count{0};
  torch::Tensor direct_edge_return_readout_valid_count_per_edge{};
  torch::Tensor direct_edge_return_readout_direction_correct_count_per_edge{};
  torch::Tensor direct_edge_return_readout_valid_count_per_channel{};
  torch::Tensor
      direct_edge_return_readout_direction_correct_count_per_channel{};
  bool skipped{false};
  bool optimizer_step_applied{false};
  bool gradients_finite{true};
  double grad_norm{std::numeric_limits<double>::quiet_NaN()};
};

namespace channel_context_mdn_train_detail {

struct edge_return_auxiliary_loss_t {
  torch::Tensor loss{};
  torch::Tensor regression_loss{};
  torch::Tensor direction_loss{};
  torch::Tensor rank_loss{};
  int64_t valid_count{0};
  int64_t pairwise_valid_count{0};
};

[[nodiscard]] inline channel_mdn_input_t
sanitize_train_input(const channel_mdn_input_t &input) {
  TORCH_CHECK(input.context.defined() && input.context.dim() == 4,
              "[channel_context_mdn_train_model] context must be "
              "[B,N,C,De]");
  TORCH_CHECK(input.context_mask.defined() && input.context_mask.dim() == 3,
              "[channel_context_mdn_train_model] context_mask must be "
              "[B,N,C]");
  TORCH_CHECK(input.future.defined() && input.future.dim() == 4,
              "[channel_context_mdn_train_model] future must be "
              "[B,N,C,Df]");
  TORCH_CHECK(input.future_mask.defined() && input.future_mask.dim() == 4,
              "[channel_context_mdn_train_model] future_mask must be "
              "[B,N,C,Df]");
  TORCH_CHECK(input.context.size(0) == input.context_mask.size(0) &&
                  input.context.size(1) == input.context_mask.size(1) &&
                  input.context.size(2) == input.context_mask.size(2),
              "[channel_context_mdn_train_model] context/mask shape mismatch");
  TORCH_CHECK(input.future.size(0) == input.context.size(0) &&
                  input.future.size(1) == input.context.size(1) &&
                  input.future.size(2) == input.context.size(2) &&
                  input.future.size(0) == input.future_mask.size(0) &&
                  input.future.size(1) == input.future_mask.size(1) &&
                  input.future.size(2) == input.future_mask.size(2) &&
                  input.future.size(3) == input.future_mask.size(3),
              "[channel_context_mdn_train_model] future/mask shape mismatch");

  auto context_mask =
      input.context_mask.to(torch::TensorOptions()
                                .dtype(torch::kBool)
                                .device(input.context.device()));
  auto future_mask = input.future_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(input.future.device()));

  auto context_finite_or_masked =
      torch::isfinite(input.context)
          .logical_or(context_mask.unsqueeze(-1).logical_not());
  TORCH_CHECK(context_finite_or_masked.all().template item<bool>(),
              "[channel_context_mdn_train_model] non-finite context value "
              "under true context mask");

  auto future_finite_or_masked =
      torch::isfinite(input.future).logical_or(future_mask.logical_not());
  TORCH_CHECK(future_finite_or_masked.all().template item<bool>(),
              "[channel_context_mdn_train_model] non-finite future value "
              "under true future mask");

  channel_mdn_input_t out = input;
  out.context = torch::where(context_mask.unsqueeze(-1), input.context,
                             torch::zeros_like(input.context));
  out.context_mask = context_mask;
  out.future =
      torch::where(future_mask, input.future, torch::zeros_like(input.future));
  out.future_mask = future_mask;
  return out;
}

inline void accumulate_expected_value_metrics(
    channel_context_mdn_train_step_result_t &step, const MdnOut &mdn_out,
    const torch::Tensor &future, const torch::Tensor &combined_mask) {
  TORCH_CHECK(future.defined() && future.dim() == 4,
              "[channel_context_mdn_train_model] future must be [B,N,C,Df]");
  TORCH_CHECK(combined_mask.defined() && combined_mask.dim() == 4,
              "[channel_context_mdn_train_model] combined_mask must be "
              "[B,N,C,Df]");
  auto expected = mdn_expectation(mdn_out).detach().to(
      torch::TensorOptions()
          .dtype(torch::kFloat64)
          .device(mdn_out.log_pi.device()));
  auto realized = future.detach().to(
      torch::TensorOptions().dtype(torch::kFloat64).device(expected.device()));
  TORCH_CHECK(expected.sizes() == realized.sizes(),
              "[channel_context_mdn_train_model] expected/future shape "
              "mismatch for EV metrics");
  auto valid = combined_mask
                   .to(torch::TensorOptions()
                           .dtype(torch::kBool)
                           .device(expected.device()))
                   .logical_and(torch::isfinite(expected))
                   .logical_and(torch::isfinite(realized));
  const auto valid_count = valid.sum().template item<int64_t>();
  if (valid_count <= 0) {
    return;
  }
  auto error = (expected - realized).masked_select(valid);
  const auto full_error =
      torch::where(valid, expected - realized, torch::zeros_like(expected));
  const auto valid_i64 = valid.to(torch::kInt64);
  const auto direction_correct_full =
      expected.sign().eq(realized.sign()).logical_and(valid).to(torch::kInt64);
  auto direction_correct = direction_correct_full.masked_select(valid);
  step.ev_metric_valid_count = valid_count;
  step.ev_direction_correct_count =
      direction_correct.sum().to(torch::kCPU).template item<int64_t>();
  step.ev_abs_error_sum =
      error.abs().sum().to(torch::kCPU).template item<double>();
  step.ev_squared_error_sum =
      error.pow(2).sum().to(torch::kCPU).template item<double>();
  step.ev_signed_error_sum =
      error.sum().to(torch::kCPU).template item<double>();

  const auto channel_dims = std::vector<int64_t>{0, 1, 3};
  const auto target_feature_dims = std::vector<int64_t>{0, 1, 2};
  const auto channel_target_feature_dims = std::vector<int64_t>{0, 1};
  const auto node_dims = std::vector<int64_t>{0, 2, 3};

  step.ev_valid_count_per_channel =
      valid_i64.sum(channel_dims).to(torch::kCPU).to(torch::kInt64);
  step.ev_direction_correct_count_per_channel =
      direction_correct_full.sum(channel_dims)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.ev_abs_error_sum_per_channel =
      full_error.abs().sum(channel_dims).to(torch::kCPU).to(torch::kFloat64);
  step.ev_squared_error_sum_per_channel =
      full_error.pow(2).sum(channel_dims).to(torch::kCPU).to(torch::kFloat64);
  step.ev_signed_error_sum_per_channel =
      full_error.sum(channel_dims).to(torch::kCPU).to(torch::kFloat64);

  step.ev_valid_count_per_target_feature =
      valid_i64.sum(target_feature_dims).to(torch::kCPU).to(torch::kInt64);
  step.ev_direction_correct_count_per_target_feature =
      direction_correct_full.sum(target_feature_dims)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.ev_abs_error_sum_per_target_feature = full_error.abs()
                                                 .sum(target_feature_dims)
                                                 .to(torch::kCPU)
                                                 .to(torch::kFloat64);
  step.ev_squared_error_sum_per_target_feature = full_error.pow(2)
                                                     .sum(target_feature_dims)
                                                     .to(torch::kCPU)
                                                     .to(torch::kFloat64);
  step.ev_signed_error_sum_per_target_feature =
      full_error.sum(target_feature_dims).to(torch::kCPU).to(torch::kFloat64);

  step.ev_valid_count_per_channel_target_feature =
      valid_i64.sum(channel_target_feature_dims)
          .reshape({-1})
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.ev_direction_correct_count_per_channel_target_feature =
      direction_correct_full.sum(channel_target_feature_dims)
          .reshape({-1})
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.ev_abs_error_sum_per_channel_target_feature =
      full_error.abs()
          .sum(channel_target_feature_dims)
          .reshape({-1})
          .to(torch::kCPU)
          .to(torch::kFloat64);
  step.ev_squared_error_sum_per_channel_target_feature =
      full_error.pow(2)
          .sum(channel_target_feature_dims)
          .reshape({-1})
          .to(torch::kCPU)
          .to(torch::kFloat64);
  step.ev_signed_error_sum_per_channel_target_feature =
      full_error.sum(channel_target_feature_dims)
          .reshape({-1})
          .to(torch::kCPU)
          .to(torch::kFloat64);

  step.ev_valid_count_per_node =
      valid_i64.sum(node_dims).to(torch::kCPU).to(torch::kInt64);
  step.ev_direction_correct_count_per_node =
      direction_correct_full.sum(node_dims).to(torch::kCPU).to(torch::kInt64);
  step.ev_abs_error_sum_per_node =
      full_error.abs().sum(node_dims).to(torch::kCPU).to(torch::kFloat64);
  step.ev_squared_error_sum_per_node =
      full_error.pow(2).sum(node_dims).to(torch::kCPU).to(torch::kFloat64);
  step.ev_signed_error_sum_per_node =
      full_error.sum(node_dims).to(torch::kCPU).to(torch::kFloat64);

  constexpr int64_t quote_node_index = 0;
  constexpr int64_t close_feature_index = 3;
  if (expected.size(1) <= 1 || expected.size(3) <= close_feature_index) {
    return;
  }

  const auto close_expected = expected.select(3, close_feature_index);
  const auto close_realized = realized.select(3, close_feature_index);
  const auto close_valid = valid.select(3, close_feature_index);
  const auto base_count = close_expected.size(1) - 1;
  const auto expected_quote =
      close_expected.select(1, quote_node_index).unsqueeze(1);
  const auto realized_quote =
      close_realized.select(1, quote_node_index).unsqueeze(1);
  const auto valid_quote = close_valid.select(1, quote_node_index).unsqueeze(1);
  const auto expected_base = close_expected.narrow(1, 1, base_count);
  const auto realized_base = close_realized.narrow(1, 1, base_count);
  const auto valid_base = close_valid.narrow(1, 1, base_count);

  const auto predicted_edge = expected_base - expected_quote;
  const auto realized_edge = realized_base - realized_quote;
  const auto edge_valid = valid_base.logical_and(valid_quote)
                              .logical_and(torch::isfinite(predicted_edge))
                              .logical_and(torch::isfinite(realized_edge));
  const auto edge_valid_count = edge_valid.sum().template item<int64_t>();
  if (edge_valid_count <= 0) {
    return;
  }

  const auto edge_error =
      (predicted_edge - realized_edge).masked_select(edge_valid);
  const auto edge_direction_correct_full = predicted_edge.sign()
                                               .eq(realized_edge.sign())
                                               .logical_and(edge_valid)
                                               .to(torch::kInt64);
  const auto edge_direction_correct =
      edge_direction_correct_full.masked_select(edge_valid);
  const auto predicted_selected = predicted_edge.masked_select(edge_valid);
  const auto realized_selected = realized_edge.masked_select(edge_valid);

  step.edge_return_projection_valid_count = edge_valid_count;
  step.edge_return_projection_direction_correct_count =
      edge_direction_correct.sum().to(torch::kCPU).template item<int64_t>();
  step.edge_return_projection_abs_error_sum =
      edge_error.abs().sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_squared_error_sum =
      edge_error.pow(2).sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_signed_error_sum =
      edge_error.sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_pred_sum =
      predicted_selected.sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_realized_sum =
      realized_selected.sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_pred_squared_sum =
      predicted_selected.pow(2).sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_realized_squared_sum =
      realized_selected.pow(2).sum().to(torch::kCPU).template item<double>();
  step.edge_return_projection_cross_sum =
      (predicted_selected * realized_selected)
          .sum()
          .to(torch::kCPU)
          .template item<double>();

  const auto edge_valid_i64 = edge_valid.to(torch::kInt64);
  const auto edge_dims = std::vector<int64_t>{0, 2};
  const auto channel_dims_for_edges = std::vector<int64_t>{0, 1};
  step.edge_return_projection_valid_count_per_edge =
      edge_valid_i64.sum(edge_dims).to(torch::kCPU).to(torch::kInt64);
  step.edge_return_projection_direction_correct_count_per_edge =
      edge_direction_correct_full.sum(edge_dims)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.edge_return_projection_valid_count_per_channel =
      edge_valid_i64.sum(channel_dims_for_edges)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.edge_return_projection_direction_correct_count_per_channel =
      edge_direction_correct_full.sum(channel_dims_for_edges)
          .to(torch::kCPU)
          .to(torch::kInt64);

  const auto complete_rank_valid = edge_valid.all(1);
  const auto best_valid_count =
      complete_rank_valid.sum().template item<int64_t>();
  if (best_valid_count > 0) {
    const auto predicted_best = std::get<1>(predicted_edge.max(1));
    const auto realized_best = std::get<1>(realized_edge.max(1));
    const auto best_correct = predicted_best.eq(realized_best)
                                  .logical_and(complete_rank_valid)
                                  .to(torch::kInt64)
                                  .sum()
                                  .to(torch::kCPU)
                                  .template item<int64_t>();
    step.edge_return_projection_best_asset_valid_count = best_valid_count;
    step.edge_return_projection_best_asset_correct_count = best_correct;
  }

  for (int64_t lhs = 0; lhs < base_count; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < base_count; ++rhs) {
      const auto lhs_pred = predicted_edge.select(1, lhs);
      const auto rhs_pred = predicted_edge.select(1, rhs);
      const auto lhs_real = realized_edge.select(1, lhs);
      const auto rhs_real = realized_edge.select(1, rhs);
      const auto lhs_valid = edge_valid.select(1, lhs);
      const auto rhs_valid = edge_valid.select(1, rhs);
      const auto pair_valid = lhs_valid.logical_and(rhs_valid);
      const auto pair_count = pair_valid.sum().template item<int64_t>();
      if (pair_count <= 0) {
        continue;
      }
      const auto pair_correct = (lhs_pred - rhs_pred)
                                    .sign()
                                    .eq((lhs_real - rhs_real).sign())
                                    .logical_and(pair_valid)
                                    .to(torch::kInt64)
                                    .sum()
                                    .to(torch::kCPU)
                                    .template item<int64_t>();
      step.edge_return_projection_pairwise_rank_valid_count += pair_count;
      step.edge_return_projection_pairwise_rank_correct_count += pair_correct;
    }
  }
}

inline void accumulate_direct_edge_return_readout_metrics(
    channel_context_mdn_train_step_result_t &step, const MdnOut &mdn_out,
    const torch::Tensor &future, const torch::Tensor &combined_mask) {
  if (!mdn_out.direct_edge_return.defined()) {
    return;
  }
  TORCH_CHECK(mdn_out.direct_edge_return.dim() == 3,
              "[channel_context_mdn_train_model] direct_edge_return must be "
              "[B,N-1,C]");
  TORCH_CHECK(future.defined() && future.dim() == 4,
              "[channel_context_mdn_train_model] future must be [B,N,C,Df]");
  TORCH_CHECK(combined_mask.defined() && combined_mask.dim() == 4,
              "[channel_context_mdn_train_model] combined_mask must be "
              "[B,N,C,Df]");

  constexpr int64_t quote_node_index = 0;
  constexpr int64_t close_feature_index = 3;
  if (future.size(1) <= 1 || future.size(3) <= close_feature_index) {
    return;
  }
  const auto base_count = future.size(1) - 1;
  TORCH_CHECK(mdn_out.direct_edge_return.size(0) == future.size(0) &&
                  mdn_out.direct_edge_return.size(1) == base_count &&
                  mdn_out.direct_edge_return.size(2) == future.size(2),
              "[channel_context_mdn_train_model] direct_edge_return shape "
              "must match [B,N-1,C]");

  auto predicted_edge = mdn_out.direct_edge_return.detach().to(
      torch::TensorOptions()
          .dtype(torch::kFloat64)
          .device(mdn_out.direct_edge_return.device()));
  auto realized = future.detach().to(torch::TensorOptions()
                                         .dtype(torch::kFloat64)
                                         .device(predicted_edge.device()));
  auto valid = combined_mask
                   .to(torch::TensorOptions()
                           .dtype(torch::kBool)
                           .device(predicted_edge.device()))
                   .logical_and(torch::isfinite(realized));

  const auto close_realized = realized.select(3, close_feature_index);
  const auto close_valid = valid.select(3, close_feature_index);
  const auto realized_quote =
      close_realized.select(1, quote_node_index).unsqueeze(1);
  const auto valid_quote = close_valid.select(1, quote_node_index).unsqueeze(1);
  const auto realized_base = close_realized.narrow(1, 1, base_count);
  const auto valid_base = close_valid.narrow(1, 1, base_count);

  const auto realized_edge = realized_base - realized_quote;
  const auto edge_valid = valid_base.logical_and(valid_quote)
                              .logical_and(torch::isfinite(predicted_edge))
                              .logical_and(torch::isfinite(realized_edge));
  const auto edge_valid_count = edge_valid.sum().template item<int64_t>();
  if (edge_valid_count <= 0) {
    return;
  }

  const auto edge_error =
      (predicted_edge - realized_edge).masked_select(edge_valid);
  const auto edge_direction_correct_full = predicted_edge.sign()
                                               .eq(realized_edge.sign())
                                               .logical_and(edge_valid)
                                               .to(torch::kInt64);
  const auto edge_direction_correct =
      edge_direction_correct_full.masked_select(edge_valid);
  const auto predicted_selected = predicted_edge.masked_select(edge_valid);
  const auto realized_selected = realized_edge.masked_select(edge_valid);

  step.direct_edge_return_readout_valid_count = edge_valid_count;
  step.direct_edge_return_readout_direction_correct_count =
      edge_direction_correct.sum().to(torch::kCPU).template item<int64_t>();
  step.direct_edge_return_readout_abs_error_sum =
      edge_error.abs().sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_squared_error_sum =
      edge_error.pow(2).sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_signed_error_sum =
      edge_error.sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_pred_sum =
      predicted_selected.sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_realized_sum =
      realized_selected.sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_pred_squared_sum =
      predicted_selected.pow(2).sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_realized_squared_sum =
      realized_selected.pow(2).sum().to(torch::kCPU).template item<double>();
  step.direct_edge_return_readout_cross_sum =
      (predicted_selected * realized_selected)
          .sum()
          .to(torch::kCPU)
          .template item<double>();

  const auto edge_valid_i64 = edge_valid.to(torch::kInt64);
  const auto edge_dims = std::vector<int64_t>{0, 2};
  const auto channel_dims_for_edges = std::vector<int64_t>{0, 1};
  step.direct_edge_return_readout_valid_count_per_edge =
      edge_valid_i64.sum(edge_dims).to(torch::kCPU).to(torch::kInt64);
  step.direct_edge_return_readout_direction_correct_count_per_edge =
      edge_direction_correct_full.sum(edge_dims)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.direct_edge_return_readout_valid_count_per_channel =
      edge_valid_i64.sum(channel_dims_for_edges)
          .to(torch::kCPU)
          .to(torch::kInt64);
  step.direct_edge_return_readout_direction_correct_count_per_channel =
      edge_direction_correct_full.sum(channel_dims_for_edges)
          .to(torch::kCPU)
          .to(torch::kInt64);

  const auto complete_rank_valid = edge_valid.all(1);
  const auto best_valid_count =
      complete_rank_valid.sum().template item<int64_t>();
  if (best_valid_count > 0) {
    const auto predicted_best = std::get<1>(predicted_edge.max(1));
    const auto realized_best = std::get<1>(realized_edge.max(1));
    const auto best_correct = predicted_best.eq(realized_best)
                                  .logical_and(complete_rank_valid)
                                  .to(torch::kInt64)
                                  .sum()
                                  .to(torch::kCPU)
                                  .template item<int64_t>();
    step.direct_edge_return_readout_best_asset_valid_count = best_valid_count;
    step.direct_edge_return_readout_best_asset_correct_count = best_correct;
  }

  for (int64_t lhs = 0; lhs < base_count; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < base_count; ++rhs) {
      const auto lhs_pred = predicted_edge.select(1, lhs);
      const auto rhs_pred = predicted_edge.select(1, rhs);
      const auto lhs_real = realized_edge.select(1, lhs);
      const auto rhs_real = realized_edge.select(1, rhs);
      const auto lhs_valid = edge_valid.select(1, lhs);
      const auto rhs_valid = edge_valid.select(1, rhs);
      const auto pair_valid = lhs_valid.logical_and(rhs_valid);
      const auto pair_count = pair_valid.sum().template item<int64_t>();
      if (pair_count <= 0) {
        continue;
      }
      const auto pair_correct = (lhs_pred - rhs_pred)
                                    .sign()
                                    .eq((lhs_real - rhs_real).sign())
                                    .logical_and(pair_valid)
                                    .to(torch::kInt64)
                                    .sum()
                                    .to(torch::kCPU)
                                    .template item<int64_t>();
      step.direct_edge_return_readout_pairwise_rank_valid_count += pair_count;
      step.direct_edge_return_readout_pairwise_rank_correct_count +=
          pair_correct;
    }
  }
}

[[nodiscard]] inline torch::Tensor smooth_l1_mean(const torch::Tensor &diff,
                                                  double beta) {
  TORCH_CHECK(diff.defined(), "[channel_mdn_edge_aux] diff is undefined");
  TORCH_CHECK(beta > 0.0, "[channel_mdn_edge_aux] Huber beta must be positive");
  const auto abs_diff = diff.abs();
  const auto quadratic = 0.5 * diff.pow(2) / beta;
  const auto linear = abs_diff - 0.5 * beta;
  return torch::where(abs_diff < beta, quadratic, linear).mean();
}

[[nodiscard]] inline edge_return_auxiliary_loss_t
compute_edge_return_auxiliary_loss(
    const MdnOut &mdn_out, const torch::Tensor &future,
    const torch::Tensor &combined_mask,
    const channel_context_mdn_train_options_t &options) {
  edge_return_auxiliary_loss_t out{};
  const auto zero = future.defined() ? future.new_zeros({})
                                     : torch::zeros({}, torch::kFloat32);
  out.loss = zero;
  out.regression_loss = zero;
  out.direction_loss = zero;
  out.rank_loss = zero;

  const bool any_enabled =
      options.edge_return_auxiliary_loss_weight > 0.0 ||
      options.edge_return_auxiliary_direction_weight > 0.0 ||
      options.edge_return_auxiliary_rank_weight > 0.0;
  if (!any_enabled) {
    return out;
  }
  TORCH_CHECK(options.edge_return_auxiliary_huber_beta > 0.0,
              "[channel_mdn_edge_aux] Huber beta must be positive");
  TORCH_CHECK(options.edge_return_auxiliary_logit_scale > 0.0,
              "[channel_mdn_edge_aux] logit scale must be positive");
  TORCH_CHECK(future.defined() && future.dim() == 4,
              "[channel_mdn_edge_aux] future must be [B,N,C,Df]");
  TORCH_CHECK(combined_mask.defined() && combined_mask.dim() == 4,
              "[channel_mdn_edge_aux] combined_mask must be [B,N,C,Df]");

  constexpr int64_t quote_node_index = 0;
  constexpr int64_t close_feature_index = 3;
  auto expected = mdn_expectation(mdn_out);
  if (expected.size(1) <= 1 || expected.size(3) <= close_feature_index) {
    return out;
  }
  auto realized = future.to(expected.options());
  auto valid = combined_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(expected.device()));

  const auto close_expected = expected.select(3, close_feature_index);
  const auto close_realized = realized.select(3, close_feature_index);
  const auto close_valid = valid.select(3, close_feature_index);
  const auto base_count = close_expected.size(1) - 1;
  const auto expected_quote =
      close_expected.select(1, quote_node_index).unsqueeze(1);
  const auto realized_quote =
      close_realized.select(1, quote_node_index).unsqueeze(1);
  const auto valid_quote = close_valid.select(1, quote_node_index).unsqueeze(1);
  const auto expected_base = close_expected.narrow(1, 1, base_count);
  const auto realized_base = close_realized.narrow(1, 1, base_count);
  const auto valid_base = close_valid.narrow(1, 1, base_count);

  const auto predicted_edge = expected_base - expected_quote;
  const auto realized_edge = realized_base - realized_quote;
  const auto edge_valid =
      valid_base.logical_and(valid_quote)
          .logical_and(torch::isfinite(predicted_edge.detach()))
          .logical_and(torch::isfinite(realized_edge.detach()));
  out.valid_count = edge_valid.sum().template item<int64_t>();
  if (out.valid_count <= 0) {
    return out;
  }

  const auto predicted_selected = predicted_edge.masked_select(edge_valid);
  const auto realized_selected = realized_edge.masked_select(edge_valid);
  const auto edge_diff = predicted_selected - realized_selected;
  out.regression_loss =
      smooth_l1_mean(edge_diff, options.edge_return_auxiliary_huber_beta);

  const auto realized_sign = realized_selected.detach().sign();
  const auto direction_active = realized_sign.ne(0);
  if (direction_active.sum().template item<int64_t>() > 0) {
    const auto pred_for_direction =
        predicted_selected.masked_select(direction_active);
    const auto sign_for_direction =
        realized_sign.masked_select(direction_active);
    out.direction_loss =
        torch::softplus(-(pred_for_direction * sign_for_direction *
                          options.edge_return_auxiliary_logit_scale))
            .mean();
  }

  torch::Tensor rank_loss_sum = zero;
  int64_t rank_count = 0;
  for (int64_t lhs = 0; lhs < base_count; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < base_count; ++rhs) {
      const auto lhs_pred = predicted_edge.select(1, lhs);
      const auto rhs_pred = predicted_edge.select(1, rhs);
      const auto lhs_real = realized_edge.select(1, lhs);
      const auto rhs_real = realized_edge.select(1, rhs);
      const auto lhs_valid = edge_valid.select(1, lhs);
      const auto rhs_valid = edge_valid.select(1, rhs);
      const auto pair_valid = lhs_valid.logical_and(rhs_valid);
      if (pair_valid.sum().template item<int64_t>() <= 0) {
        continue;
      }
      const auto pred_diff = (lhs_pred - rhs_pred).masked_select(pair_valid);
      const auto rank_sign =
          (lhs_real - rhs_real).detach().sign().masked_select(pair_valid);
      const auto rank_active = rank_sign.ne(0);
      const auto active_count = rank_active.sum().template item<int64_t>();
      if (active_count <= 0) {
        continue;
      }
      const auto terms =
          torch::softplus(-(pred_diff.masked_select(rank_active) *
                            rank_sign.masked_select(rank_active) *
                            options.edge_return_auxiliary_logit_scale));
      rank_loss_sum = rank_loss_sum + terms.sum();
      rank_count += active_count;
    }
  }
  out.pairwise_valid_count = rank_count;
  if (rank_count > 0) {
    out.rank_loss = rank_loss_sum / static_cast<double>(rank_count);
  }

  out.loss = zero;
  if (options.edge_return_auxiliary_loss_weight > 0.0) {
    out.loss = out.loss +
               options.edge_return_auxiliary_loss_weight * out.regression_loss;
  }
  if (options.edge_return_auxiliary_direction_weight > 0.0) {
    out.loss = out.loss + options.edge_return_auxiliary_direction_weight *
                              out.direction_loss;
  }
  if (options.edge_return_auxiliary_rank_weight > 0.0) {
    out.loss =
        out.loss + options.edge_return_auxiliary_rank_weight * out.rank_loss;
  }
  return out;
}

[[nodiscard]] inline edge_return_auxiliary_loss_t
compute_direct_edge_return_readout_loss(
    const MdnOut &mdn_out, const torch::Tensor &future,
    const torch::Tensor &combined_mask,
    const channel_context_mdn_train_options_t &options) {
  edge_return_auxiliary_loss_t out{};
  const auto zero = future.defined() ? future.new_zeros({})
                                     : torch::zeros({}, torch::kFloat32);
  out.loss = zero;
  out.regression_loss = zero;
  out.direction_loss = zero;
  out.rank_loss = zero;

  const bool any_weight_enabled =
      options.direct_edge_return_readout_loss_weight > 0.0 ||
      options.direct_edge_return_readout_direction_weight > 0.0 ||
      options.direct_edge_return_readout_rank_weight > 0.0;
  const bool any_enabled =
      options.direct_edge_return_readout_enabled || any_weight_enabled;
  if (!any_enabled) {
    return out;
  }
  TORCH_CHECK(options.direct_edge_return_readout_enabled,
              "[channel_mdn_direct_edge_readout] weights require "
              "DIRECT_EDGE_RETURN_READOUT_ENABLED=true");
  TORCH_CHECK(mdn_out.direct_edge_return.defined(),
              "[channel_mdn_direct_edge_readout] model did not emit "
              "direct_edge_return");
  TORCH_CHECK(mdn_out.direct_edge_return.dim() == 3,
              "[channel_mdn_direct_edge_readout] direct_edge_return must be "
              "[B,N-1,C]");
  TORCH_CHECK(options.direct_edge_return_readout_huber_beta > 0.0,
              "[channel_mdn_direct_edge_readout] Huber beta must be positive");
  TORCH_CHECK(options.direct_edge_return_readout_logit_scale > 0.0,
              "[channel_mdn_direct_edge_readout] logit scale must be positive");
  TORCH_CHECK(future.defined() && future.dim() == 4,
              "[channel_mdn_direct_edge_readout] future must be [B,N,C,Df]");
  TORCH_CHECK(combined_mask.defined() && combined_mask.dim() == 4,
              "[channel_mdn_direct_edge_readout] combined_mask must be "
              "[B,N,C,Df]");

  constexpr int64_t quote_node_index = 0;
  constexpr int64_t close_feature_index = 3;
  if (future.size(1) <= 1 || future.size(3) <= close_feature_index) {
    return out;
  }
  const auto base_count = future.size(1) - 1;
  TORCH_CHECK(mdn_out.direct_edge_return.size(0) == future.size(0) &&
                  mdn_out.direct_edge_return.size(1) == base_count &&
                  mdn_out.direct_edge_return.size(2) == future.size(2),
              "[channel_mdn_direct_edge_readout] direct_edge_return shape "
              "must match [B,N-1,C]");

  auto predicted_edge = mdn_out.direct_edge_return;
  auto realized = future.to(predicted_edge.options());
  auto valid = combined_mask.to(torch::TensorOptions()
                                    .dtype(torch::kBool)
                                    .device(predicted_edge.device()));
  const auto close_realized = realized.select(3, close_feature_index);
  const auto close_valid = valid.select(3, close_feature_index);
  const auto realized_quote =
      close_realized.select(1, quote_node_index).unsqueeze(1);
  const auto valid_quote = close_valid.select(1, quote_node_index).unsqueeze(1);
  const auto realized_base = close_realized.narrow(1, 1, base_count);
  const auto valid_base = close_valid.narrow(1, 1, base_count);

  const auto realized_edge = realized_base - realized_quote;
  const auto edge_valid =
      valid_base.logical_and(valid_quote)
          .logical_and(torch::isfinite(predicted_edge.detach()))
          .logical_and(torch::isfinite(realized_edge.detach()));
  out.valid_count = edge_valid.sum().template item<int64_t>();
  if (out.valid_count <= 0) {
    return out;
  }

  const auto predicted_selected = predicted_edge.masked_select(edge_valid);
  const auto realized_selected = realized_edge.masked_select(edge_valid);
  const auto edge_diff = predicted_selected - realized_selected;
  out.regression_loss =
      smooth_l1_mean(edge_diff, options.direct_edge_return_readout_huber_beta);

  const auto realized_sign = realized_selected.detach().sign();
  const auto direction_active = realized_sign.ne(0);
  if (direction_active.sum().template item<int64_t>() > 0) {
    const auto pred_for_direction =
        predicted_selected.masked_select(direction_active);
    const auto sign_for_direction =
        realized_sign.masked_select(direction_active);
    out.direction_loss =
        torch::softplus(-(pred_for_direction * sign_for_direction *
                          options.direct_edge_return_readout_logit_scale))
            .mean();
  }

  torch::Tensor rank_loss_sum = zero;
  int64_t rank_count = 0;
  for (int64_t lhs = 0; lhs < base_count; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < base_count; ++rhs) {
      const auto lhs_pred = predicted_edge.select(1, lhs);
      const auto rhs_pred = predicted_edge.select(1, rhs);
      const auto lhs_real = realized_edge.select(1, lhs);
      const auto rhs_real = realized_edge.select(1, rhs);
      const auto lhs_valid = edge_valid.select(1, lhs);
      const auto rhs_valid = edge_valid.select(1, rhs);
      const auto pair_valid = lhs_valid.logical_and(rhs_valid);
      if (pair_valid.sum().template item<int64_t>() <= 0) {
        continue;
      }
      const auto pred_diff = (lhs_pred - rhs_pred).masked_select(pair_valid);
      const auto rank_sign =
          (lhs_real - rhs_real).detach().sign().masked_select(pair_valid);
      const auto rank_active = rank_sign.ne(0);
      const auto active_count = rank_active.sum().template item<int64_t>();
      if (active_count <= 0) {
        continue;
      }
      const auto terms =
          torch::softplus(-(pred_diff.masked_select(rank_active) *
                            rank_sign.masked_select(rank_active) *
                            options.direct_edge_return_readout_logit_scale));
      rank_loss_sum = rank_loss_sum + terms.sum();
      rank_count += active_count;
    }
  }
  out.pairwise_valid_count = rank_count;
  if (rank_count > 0) {
    out.rank_loss = rank_loss_sum / static_cast<double>(rank_count);
  }

  out.loss = zero;
  if (options.direct_edge_return_readout_loss_weight > 0.0) {
    out.loss = out.loss + options.direct_edge_return_readout_loss_weight *
                              out.regression_loss;
  }
  if (options.direct_edge_return_readout_direction_weight > 0.0) {
    out.loss = out.loss + options.direct_edge_return_readout_direction_weight *
                              out.direction_loss;
  }
  if (options.direct_edge_return_readout_rank_weight > 0.0) {
    out.loss = out.loss +
               options.direct_edge_return_readout_rank_weight * out.rank_loss;
  }
  return out;
}

inline void record_edge_return_auxiliary_loss(
    channel_context_mdn_train_step_result_t &step,
    const edge_return_auxiliary_loss_t &auxiliary) {
  step.edge_return_auxiliary_loss = auxiliary.loss;
  step.edge_return_auxiliary_regression_loss = auxiliary.regression_loss;
  step.edge_return_auxiliary_direction_loss = auxiliary.direction_loss;
  step.edge_return_auxiliary_rank_loss = auxiliary.rank_loss;
  step.edge_return_auxiliary_valid_count = auxiliary.valid_count;
  step.edge_return_auxiliary_pairwise_valid_count =
      auxiliary.pairwise_valid_count;
}

inline void record_direct_edge_return_readout_loss(
    channel_context_mdn_train_step_result_t &step,
    const edge_return_auxiliary_loss_t &direct) {
  step.direct_edge_return_readout_loss = direct.loss;
  step.direct_edge_return_readout_regression_loss = direct.regression_loss;
  step.direct_edge_return_readout_direction_loss = direct.direction_loss;
  step.direct_edge_return_readout_rank_loss = direct.rank_loss;
  step.direct_edge_return_readout_loss_valid_count = direct.valid_count;
  step.direct_edge_return_readout_loss_pairwise_valid_count =
      direct.pairwise_valid_count;
}

[[nodiscard]] inline channel_mdn_plus_global_input_t
sanitize_plus_global_train_input(const channel_mdn_plus_global_input_t &input) {
  auto clean_channel = sanitize_train_input(input.channel);
  TORCH_CHECK(input.global_context.defined() && input.global_context.dim() == 3,
              "[channel_context_plus_global_mdn_train_model] global_context "
              "must be [B,N,Dg]");
  TORCH_CHECK(input.global_mask.defined() && input.global_mask.dim() == 2,
              "[channel_context_plus_global_mdn_train_model] global_mask must "
              "be [B,N]");
  TORCH_CHECK(input.global_context.size(0) == clean_channel.context.size(0) &&
                  input.global_context.size(1) ==
                      clean_channel.context.size(1) &&
                  input.global_mask.size(0) == clean_channel.context.size(0) &&
                  input.global_mask.size(1) == clean_channel.context.size(1),
              "[channel_context_plus_global_mdn_train_model] global/context "
              "shape mismatch");

  auto global_mask =
      input.global_mask.to(torch::TensorOptions()
                               .dtype(torch::kBool)
                               .device(input.global_context.device()));
  auto global_finite_or_masked =
      torch::isfinite(input.global_context)
          .logical_or(global_mask.unsqueeze(-1).logical_not());
  TORCH_CHECK(global_finite_or_masked.all().template item<bool>(),
              "[channel_context_plus_global_mdn_train_model] non-finite "
              "global context value under true global mask");

  channel_mdn_plus_global_input_t out{};
  out.channel = std::move(clean_channel);
  out.global_context =
      torch::where(global_mask.unsqueeze(-1), input.global_context,
                   torch::zeros_like(input.global_context));
  out.global_mask = global_mask.to(out.channel.context_mask.device());
  return out;
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

inline int64_t nonfinite_count(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out) {
  int64_t count =
      torch::isfinite(out.log_pi).logical_not().sum().template item<int64_t>() +
      torch::isfinite(out.mu).logical_not().sum().template item<int64_t>() +
      torch::isfinite(out.sigma).logical_not().sum().template item<int64_t>();
  if (out.direct_edge_return.defined()) {
    count += torch::isfinite(out.direct_edge_return)
                 .logical_not()
                 .sum()
                 .template item<int64_t>();
  }
  return count;
}

inline double masked_mixture_entropy(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const torch::Tensor &mask) {
  auto pi = out.log_pi.exp();
  auto entropy = -(pi * out.log_pi).sum(/*dim=*/-1);
  auto valid = mask.to(entropy.dtype());
  const auto denom = valid.sum().clamp_min(1.0);
  return ((entropy * valid).sum() / denom).to(torch::kCPU).item<double>();
}

struct masked_sigma_summary_t {
  double mean{std::numeric_limits<double>::quiet_NaN()};
  double min{std::numeric_limits<double>::quiet_NaN()};
  double max{std::numeric_limits<double>::quiet_NaN()};
};

inline masked_sigma_summary_t masked_sigma_summary(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const torch::Tensor &mask) {
  masked_sigma_summary_t summary{};
  if (!out.sigma.defined() || !mask.defined() || out.sigma.numel() == 0) {
    return summary;
  }
  auto valid = mask.to(torch::TensorOptions()
                           .dtype(torch::kBool)
                           .device(out.sigma.device()))
                   .unsqueeze(-1)
                   .expand_as(out.sigma);
  auto values = out.sigma.masked_select(valid);
  if (values.numel() == 0) {
    return summary;
  }
  summary.mean = values.mean().to(torch::kCPU).template item<double>();
  summary.min = values.min().to(torch::kCPU).template item<double>();
  summary.max = values.max().to(torch::kCPU).template item<double>();
  return summary;
}

inline torch::Tensor mixture_usage(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const torch::Tensor &mask) {
  auto valid = mask.to(out.log_pi.dtype()).unsqueeze(-1);
  auto denom = valid.sum().clamp_min(1.0);
  return (out.log_pi.exp() * valid).sum(std::vector<int64_t>{0, 1, 2, 3}) /
         denom;
}

} // namespace channel_context_mdn_train_detail

class channel_context_mdn_train_model_t {
public:
  channel_context_mdn_train_model_t(
      ChannelContextMdn model, double learning_rate,
      channel_context_mdn_train_options_t options = {})
      : model_(std::move(model)), options_(std::move(options)) {
    TORCH_CHECK(!model_.is_empty(),
                "[channel_context_mdn_train_model] model is null");
    TORCH_CHECK(learning_rate > 0.0,
                "[channel_context_mdn_train_model] learning_rate must be "
                "positive");
    params_ = model_->parameters();
    optimizer_ = std::make_unique<torch::optim::Adam>(
        params_, torch::optim::AdamOptions(learning_rate));
  }

  [[nodiscard]] cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut
  forward(const torch::Tensor &context) {
    model_->eval();
    torch::NoGradGuard no_grad;
    return model_->forward(context);
  }

  [[nodiscard]] cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut
  forward(const torch::Tensor &context, const torch::Tensor &context_mask) {
    model_->eval();
    torch::NoGradGuard no_grad;
    return model_->forward(context, context_mask);
  }

  [[nodiscard]] torch::Tensor
  direct_edge_context_features(const torch::Tensor &context,
                               const torch::Tensor &context_mask) {
    model_->eval();
    torch::NoGradGuard no_grad;
    return model_->direct_edge_context_features(context, context_mask);
  }

  [[nodiscard]] channel_context_mdn_train_step_result_t
  train_one_batch(const channel_mdn_input_t &input) {
    const auto clean_input =
        channel_context_mdn_train_detail::sanitize_train_input(input);
    auto combined_mask = combine_channel_context_and_future_mask(
        clean_input.context_mask, clean_input.future_mask);
    const auto valid_count = combined_mask.sum().template item<int64_t>();

    channel_context_mdn_train_step_result_t out{};
    out.valid_target_count = valid_count;
    out.valid_target_fraction =
        combined_mask.numel() == 0
            ? 0.0
            : static_cast<double>(valid_count) /
                  static_cast<double>(combined_mask.numel());
    if (valid_count == 0) {
      out.skipped = true;
      out.loss = torch::zeros({}, clean_input.context.options());
      out.nll = out.loss;
      return out;
    }

    model_->train();
    optimizer_->zero_grad();
    auto mdn_out =
        model_->forward(clean_input.context, clean_input.context_mask);
    out.nonfinite_output_count =
        channel_context_mdn_train_detail::nonfinite_count(mdn_out);
    auto nll_map =
        cuwacunu::wikimyei::inference::expected_value::mdn::mdn_nll_map(
            mdn_out, clean_input.future, options_.nll);
    channel_context_mdn_train_detail::accumulate_expected_value_metrics(
        out, mdn_out, clean_input.future, combined_mask);
    channel_context_mdn_train_detail::
        accumulate_direct_edge_return_readout_metrics(
            out, mdn_out, clean_input.future, combined_mask);
    out.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::mdn::
        mdn_masked_mean_per_channel(nll_map, combined_mask);
    out.nll_per_target_feature = cuwacunu::wikimyei::inference::expected_value::
        mdn::mdn_masked_mean_per_target_feature(nll_map, combined_mask);
    out.nll_per_channel_target_feature =
        cuwacunu::wikimyei::inference::expected_value::mdn::
            mdn_masked_mean_per_channel_target_feature(nll_map, combined_mask);
    out.valid_count_per_channel = cuwacunu::wikimyei::inference::
        expected_value::mdn::mdn_valid_count_per_channel(combined_mask);
    out.valid_count_per_target_feature = cuwacunu::wikimyei::inference::
        expected_value::mdn::mdn_valid_count_per_target_feature(combined_mask);
    out.valid_count_per_channel_target_feature =
        cuwacunu::wikimyei::inference::expected_value::mdn::
            mdn_valid_count_per_channel_target_feature(combined_mask);
    out.nll =
        compute_channel_context_mdn_nll(mdn_out, clean_input, options_.nll);
    const auto edge_auxiliary =
        channel_context_mdn_train_detail::compute_edge_return_auxiliary_loss(
            mdn_out, clean_input.future, combined_mask, options_);
    channel_context_mdn_train_detail::record_edge_return_auxiliary_loss(
        out, edge_auxiliary);
    const auto direct_readout = channel_context_mdn_train_detail::
        compute_direct_edge_return_readout_loss(mdn_out, clean_input.future,
                                                combined_mask, options_);
    channel_context_mdn_train_detail::record_direct_edge_return_readout_loss(
        out, direct_readout);
    out.loss = out.nll + edge_auxiliary.loss + direct_readout.loss;
    out.sigma_mean = mdn_out.sigma.mean().to(torch::kCPU).item<double>();
    out.sigma_min = mdn_out.sigma.min().to(torch::kCPU).item<double>();
    out.sigma_max = mdn_out.sigma.max().to(torch::kCPU).item<double>();
    const auto sigma_valid =
        channel_context_mdn_train_detail::masked_sigma_summary(mdn_out,
                                                               combined_mask);
    out.sigma_mean_valid = sigma_valid.mean;
    out.sigma_min_valid = sigma_valid.min;
    out.sigma_max_valid = sigma_valid.max;
    out.mixture_entropy =
        channel_context_mdn_train_detail::masked_mixture_entropy(mdn_out,
                                                                 combined_mask);
    out.mixture_usage =
        channel_context_mdn_train_detail::mixture_usage(mdn_out, combined_mask);

    if (out.nonfinite_output_count > 0 ||
        !torch::isfinite(out.loss).all().template item<bool>()) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false,
                    "[channel_context_mdn_train_model] non-finite loss/output");
      }
      return out;
    }

    out.loss.backward();
    out.grad_norm = channel_context_mdn_train_detail::gradient_norm(params_);
    channel_context_mdn_train_detail::clip_gradients(
        params_, options_.grad_clip_norm, out.grad_norm);
    out.gradients_finite =
        channel_context_mdn_train_detail::parameters_are_finite(
            params_, /*inspect_grad=*/true);
    if (!out.gradients_finite) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false,
                    "[channel_context_mdn_train_model] non-finite gradients");
      }
      optimizer_->zero_grad();
      return out;
    }

    optimizer_->step();
    out.optimizer_step_applied = true;
    return out;
  }

  [[nodiscard]] ChannelContextMdn &model() { return model_; }
  [[nodiscard]] const ChannelContextMdn &model() const { return model_; }
  [[nodiscard]] torch::optim::Adam &optimizer() { return *optimizer_; }

private:
  ChannelContextMdn model_{nullptr};
  channel_context_mdn_train_options_t options_{};
  std::vector<torch::Tensor> params_{};
  std::unique_ptr<torch::optim::Adam> optimizer_{};
};

class channel_context_plus_global_mdn_train_model_t {
public:
  channel_context_plus_global_mdn_train_model_t(
      ChannelContextPlusGlobalMdn model, double learning_rate,
      channel_context_mdn_train_options_t options = {})
      : model_(std::move(model)), options_(std::move(options)) {
    TORCH_CHECK(!model_.is_empty(),
                "[channel_context_plus_global_mdn_train_model] model is null");
    TORCH_CHECK(learning_rate > 0.0,
                "[channel_context_plus_global_mdn_train_model] learning_rate "
                "must be positive");
    params_ = model_->parameters();
    optimizer_ = std::make_unique<torch::optim::Adam>(
        params_, torch::optim::AdamOptions(learning_rate));
  }

  [[nodiscard]] cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut
  forward(const channel_mdn_plus_global_input_t &input) {
    const auto clean_input =
        channel_context_mdn_train_detail::sanitize_plus_global_train_input(
            input);
    model_->eval();
    torch::NoGradGuard no_grad;
    return model_->forward(clean_input.channel.context,
                           clean_input.channel.context_mask,
                           clean_input.global_context, clean_input.global_mask);
  }

  [[nodiscard]] channel_context_mdn_train_step_result_t
  train_one_batch(const channel_mdn_plus_global_input_t &input) {
    const auto clean_input =
        channel_context_mdn_train_detail::sanitize_plus_global_train_input(
            input);
    auto combined_mask = combine_channel_plus_global_context_and_future_mask(
        clean_input.channel.context_mask, clean_input.global_mask,
        clean_input.channel.future_mask);
    const auto valid_count = combined_mask.sum().template item<int64_t>();

    channel_context_mdn_train_step_result_t out{};
    out.valid_target_count = valid_count;
    out.valid_target_fraction =
        combined_mask.numel() == 0
            ? 0.0
            : static_cast<double>(valid_count) /
                  static_cast<double>(combined_mask.numel());
    if (valid_count == 0) {
      out.skipped = true;
      out.loss = torch::zeros({}, clean_input.channel.context.options());
      out.nll = out.loss;
      return out;
    }

    model_->train();
    optimizer_->zero_grad();
    auto mdn_out = model_->forward(
        clean_input.channel.context, clean_input.channel.context_mask,
        clean_input.global_context, clean_input.global_mask);
    out.nonfinite_output_count =
        channel_context_mdn_train_detail::nonfinite_count(mdn_out);
    auto nll_map =
        cuwacunu::wikimyei::inference::expected_value::mdn::mdn_nll_map(
            mdn_out, clean_input.channel.future, options_.nll);
    channel_context_mdn_train_detail::accumulate_expected_value_metrics(
        out, mdn_out, clean_input.channel.future, combined_mask);
    channel_context_mdn_train_detail::
        accumulate_direct_edge_return_readout_metrics(
            out, mdn_out, clean_input.channel.future, combined_mask);
    out.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::mdn::
        mdn_masked_mean_per_channel(nll_map, combined_mask);
    out.nll_per_target_feature = cuwacunu::wikimyei::inference::expected_value::
        mdn::mdn_masked_mean_per_target_feature(nll_map, combined_mask);
    out.nll_per_channel_target_feature =
        cuwacunu::wikimyei::inference::expected_value::mdn::
            mdn_masked_mean_per_channel_target_feature(nll_map, combined_mask);
    out.valid_count_per_channel = cuwacunu::wikimyei::inference::
        expected_value::mdn::mdn_valid_count_per_channel(combined_mask);
    out.valid_count_per_target_feature = cuwacunu::wikimyei::inference::
        expected_value::mdn::mdn_valid_count_per_target_feature(combined_mask);
    out.valid_count_per_channel_target_feature =
        cuwacunu::wikimyei::inference::expected_value::mdn::
            mdn_valid_count_per_channel_target_feature(combined_mask);
    out.nll = compute_channel_context_plus_global_mdn_nll(mdn_out, clean_input,
                                                          options_.nll);
    const auto edge_auxiliary =
        channel_context_mdn_train_detail::compute_edge_return_auxiliary_loss(
            mdn_out, clean_input.channel.future, combined_mask, options_);
    channel_context_mdn_train_detail::record_edge_return_auxiliary_loss(
        out, edge_auxiliary);
    const auto direct_readout = channel_context_mdn_train_detail::
        compute_direct_edge_return_readout_loss(
            mdn_out, clean_input.channel.future, combined_mask, options_);
    channel_context_mdn_train_detail::record_direct_edge_return_readout_loss(
        out, direct_readout);
    out.loss = out.nll + edge_auxiliary.loss + direct_readout.loss;
    out.sigma_mean = mdn_out.sigma.mean().to(torch::kCPU).item<double>();
    out.sigma_min = mdn_out.sigma.min().to(torch::kCPU).item<double>();
    out.sigma_max = mdn_out.sigma.max().to(torch::kCPU).item<double>();
    const auto sigma_valid =
        channel_context_mdn_train_detail::masked_sigma_summary(mdn_out,
                                                               combined_mask);
    out.sigma_mean_valid = sigma_valid.mean;
    out.sigma_min_valid = sigma_valid.min;
    out.sigma_max_valid = sigma_valid.max;
    out.mixture_entropy =
        channel_context_mdn_train_detail::masked_mixture_entropy(mdn_out,
                                                                 combined_mask);
    out.mixture_usage =
        channel_context_mdn_train_detail::mixture_usage(mdn_out, combined_mask);

    if (out.nonfinite_output_count > 0 ||
        !torch::isfinite(out.loss).all().template item<bool>()) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false,
                    "[channel_context_plus_global_mdn_train_model] non-finite "
                    "loss/output");
      }
      return out;
    }

    out.loss.backward();
    out.grad_norm = channel_context_mdn_train_detail::gradient_norm(params_);
    channel_context_mdn_train_detail::clip_gradients(
        params_, options_.grad_clip_norm, out.grad_norm);
    out.gradients_finite =
        channel_context_mdn_train_detail::parameters_are_finite(
            params_, /*inspect_grad=*/true);
    if (!out.gradients_finite) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false,
                    "[channel_context_plus_global_mdn_train_model] non-finite "
                    "gradients");
      }
      optimizer_->zero_grad();
      return out;
    }

    optimizer_->step();
    out.optimizer_step_applied = true;
    return out;
  }

  [[nodiscard]] ChannelContextPlusGlobalMdn &model() { return model_; }
  [[nodiscard]] const ChannelContextPlusGlobalMdn &model() const {
    return model_;
  }
  [[nodiscard]] torch::optim::Adam &optimizer() { return *optimizer_; }

private:
  ChannelContextPlusGlobalMdn model_{nullptr};
  channel_context_mdn_train_options_t options_{};
  std::vector<torch::Tensor> params_{};
  std::unique_ptr<torch::optim::Adam> optimizer_{};
};

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
