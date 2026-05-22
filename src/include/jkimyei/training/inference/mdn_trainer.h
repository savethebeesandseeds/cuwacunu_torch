// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "jkimyei/api/training_spec.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::jkimyei::training::inference {

struct mdn_training_options_t {
  std::vector<int64_t> target_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
  bool skip_empty_batches{true};
  bool require_finite_context{true};
  bool require_finite_targets_under_mask{true};
  double grad_clip_norm{0.0};
  double low_target_coverage_warning_fraction{0.05};
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions
      nll_options{};
  cuwacunu::wikimyei::inference::expected_value::mdn::stream::
      mdn_adapter_options_t adapter_options{};
};

struct mdn_training_metadata_t {
  std::string target_domain{"node_future"};
  std::string activity_target_semantics{"node_feature_support_mean"};
  std::string head_policy{"per_node"};
  std::vector<int64_t> target_coords{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::string graph_order_fingerprint{};
  int64_t context_dim{0};
  int64_t target_dim{0};
  int64_t channel_count{0};
  int64_t horizon_count{0};
  int64_t node_count{0};
};

struct mdn_train_report_t {
  bool trained{false};
  bool skipped_empty{false};
  bool optimizer_step_applied{false};
  bool low_target_coverage_warning{false};

  int64_t row_count{0};
  int64_t node_count{0};
  int64_t valid_row_count{0};
  int64_t valid_target_count{0};
  int64_t skipped_node_head_count{0};
  int64_t active_node_head_count{0};
  int64_t trained_node_head_count{0};
  int64_t evaluated_node_head_count{0};
  double valid_node_target_fraction{std::numeric_limits<double>::quiet_NaN()};

  double loss{std::numeric_limits<double>::quiet_NaN()};
  double node_context_norm_mean{std::numeric_limits<double>::quiet_NaN()};
  double node_context_norm_max{std::numeric_limits<double>::quiet_NaN()};
  double mixture_entropy_mean{std::numeric_limits<double>::quiet_NaN()};
  double mixture_entropy_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};

  std::vector<int64_t> routed_row_count_by_node{};
  std::vector<int64_t> active_row_count_by_node{};
  std::vector<int64_t> trained_row_count_by_node{};
  std::vector<int64_t> valid_target_count_by_node{};
  std::vector<double> nll_by_node{};

  torch::Tensor valid_target_count_by_channel{};
  torch::Tensor valid_target_count_by_horizon{};
  torch::Tensor nll_by_channel{};
  torch::Tensor nll_by_horizon{};

  mdn_training_metadata_t metadata{};
};

namespace mdn_trainer_detail {

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;

inline torch::Tensor finite_mask_like(const torch::Tensor &values) {
  return torch::isfinite(values);
}

inline void check_finite_context(const torch::Tensor &context,
                                 const torch::Tensor &context_mask) {
  TORCH_CHECK(context.dim() == 2, "[mdn_trainer] context must be [B_node,D]");
  TORCH_CHECK(context_mask.dim() == 1 &&
                  context_mask.size(0) == context.size(0),
              "[mdn_trainer] context_mask must be [B_node]");
  auto valid = context_mask.to(torch::kBool).view({context.size(0), 1});
  TORCH_CHECK(finite_mask_like(context)
                  .logical_or(valid.logical_not())
                  .all()
                  .item<bool>(),
              "[mdn_trainer] non-finite context value under true "
              "context mask");
}

inline void check_finite_targets_under_mask(const torch::Tensor &targets,
                                            const torch::Tensor &mask) {
  TORCH_CHECK(targets.dim() == 4,
              "[mdn_trainer] targets must be [B_node,C,Hf,Df]");
  TORCH_CHECK(mask.dim() == 3 && mask.size(0) == targets.size(0) &&
                  mask.size(1) == targets.size(1) &&
                  mask.size(2) == targets.size(2),
              "[mdn_trainer] mask must be [B_node,C,Hf]");
  auto valid = mask.to(torch::kBool).unsqueeze(-1).expand_as(targets);
  TORCH_CHECK(finite_mask_like(targets)
                  .logical_or(valid.logical_not())
                  .all()
                  .item<bool>(),
              "[mdn_trainer] non-finite target value under true future "
              "node mask");
}

inline torch::Tensor valid_training_rows(const torch::Tensor &rows,
                                         const torch::Tensor &mask) {
  auto mask_for_rows = mask.index_select(/*dim=*/0, rows); // [R,C,Hf]
  auto row_valid = mask_for_rows.any(std::vector<int64_t>{1, 2});
  auto keep = torch::nonzero(row_valid).reshape({-1});
  if (keep.numel() == 0) {
    return rows.new_empty({0});
  }
  return rows.index_select(/*dim=*/0, keep);
}

inline double tensor_scalar_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .template item<double>();
}

inline void accumulate_output_stats(mdn_train_report_t &report,
                                    const mdn::MdnOut &out,
                                    const torch::Tensor &mask,
                                    double &entropy_sum, double &entropy_min,
                                    int64_t &entropy_count, double &sigma_sum,
                                    double &sigma_min, double &sigma_max,
                                    int64_t &sigma_count) {
  auto valid = mask.to(torch::kBool);
  if (!valid.any().item<bool>()) {
    return;
  }

  auto pi = out.log_pi.exp();
  auto entropy = -(pi * out.log_pi).sum(/*dim=*/3); // [B,C,Hf]
  auto entropy_valid = entropy.masked_select(valid);
  if (entropy_valid.numel() > 0) {
    entropy_sum += entropy_valid.sum().detach().to(torch::kCPU).item<double>();
    entropy_count += entropy_valid.numel();
    entropy_min =
        std::min(entropy_min,
                 entropy_valid.min().detach().to(torch::kCPU).item<double>());
  }

  auto sigma_valid = out.sigma.masked_select(valid.unsqueeze(-1).unsqueeze(-1));
  if (sigma_valid.numel() > 0) {
    sigma_sum += sigma_valid.sum().detach().to(torch::kCPU).item<double>();
    sigma_count += sigma_valid.numel();
    sigma_min = std::min(
        sigma_min, sigma_valid.min().detach().to(torch::kCPU).item<double>());
    sigma_max = std::max(
        sigma_max, sigma_valid.max().detach().to(torch::kCPU).item<double>());
  }

  report.finite_parameter_check =
      (report.finite_parameter_check != 0.0 &&
       torch::isfinite(out.log_pi).all().item<bool>() &&
       torch::isfinite(out.mu).all().item<bool>() &&
       torch::isfinite(out.sigma).all().item<bool>())
          ? 1.0
          : 0.0;
}

template <typename KeyT>
inline mdn_training_metadata_t
make_metadata(const std::vector<int64_t> &target_coords,
              const mdnstream::mdn_input_batch_t<KeyT> &batch,
              int64_t node_count) {
  mdn_training_metadata_t out{};
  out.target_coords = target_coords;
  out.node_ids = batch.node_ids;
  out.edge_ids = batch.edge_ids;
  out.graph_order_fingerprint = batch.graph_order_fingerprint;
  out.context_dim = batch.context.defined() && batch.context.dim() == 2
                        ? batch.context.size(1)
                        : 0;
  out.target_dim = static_cast<int64_t>(target_coords.size());
  out.channel_count = batch.future.defined() && batch.future.dim() == 4
                          ? batch.future.size(1)
                          : 0;
  out.horizon_count = batch.future.defined() && batch.future.dim() == 4
                          ? batch.future.size(2)
                          : 0;
  out.node_count = node_count;
  return out;
}

} // namespace mdn_trainer_detail

template <typename KeyT>
[[nodiscard]] mdn_train_report_t train_mdn_batch(
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        mdn_input_batch_t<KeyT> &batch,
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        &node_heads,
    torch::optim::Optimizer &optimizer,
    const mdn_training_options_t &options = {}) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace mdnstream =
      cuwacunu::wikimyei::inference::expected_value::mdn::stream;

  TORCH_CHECK(!node_heads.empty(), "[mdn_trainer] node_heads is empty");
  TORCH_CHECK(batch.context.defined() && batch.context.dim() == 2,
              "[mdn_trainer] context must be [B_node,D]");
  TORCH_CHECK(batch.context_mask.defined() && batch.context_mask.dim() == 1 &&
                  batch.context_mask.size(0) == batch.context.size(0),
              "[mdn_trainer] context_mask must be [B_node]");
  TORCH_CHECK(batch.node_index.defined() && batch.node_index.dim() == 1 &&
                  batch.node_index.size(0) == batch.context.size(0),
              "[mdn_trainer] node_index must be [B_node]");
  TORCH_CHECK(batch.future.defined() && batch.future.dim() == 4,
              "[mdn_trainer] future must be [B_node,C,Hf,D]");
  TORCH_CHECK(batch.future_mask.defined() && batch.future_mask.dim() == 3 &&
                  batch.future_mask.size(0) == batch.future.size(0) &&
                  batch.future_mask.size(1) == batch.future.size(1) &&
                  batch.future_mask.size(2) == batch.future.size(2),
              "[mdn_trainer] future_mask must be [B_node,C,Hf]");

  const int64_t inferred_node_count =
      batch.node_index.numel() == 0
          ? 0
          : batch.node_index.max().template item<int64_t>() + 1;
  const int64_t node_count = batch.node_ids.empty()
                                 ? inferred_node_count
                                 : static_cast<int64_t>(batch.node_ids.size());
  TORCH_CHECK(node_count > 0, "[mdn_trainer] node_count must be positive");
  TORCH_CHECK(static_cast<int64_t>(node_heads.size()) >= node_count,
              "[mdn_trainer] not enough per-node MDN heads");

  mdn_train_report_t report{};
  report.row_count = batch.context.size(0);
  report.node_count = node_count;
  report.routed_row_count_by_node.assign(node_count, 0);
  report.active_row_count_by_node.assign(node_count, 0);
  report.trained_row_count_by_node.assign(node_count, 0);
  report.valid_target_count_by_node.assign(node_count, 0);
  report.nll_by_node.assign(node_count,
                            std::numeric_limits<double>::quiet_NaN());
  report.metadata = mdn_trainer_detail::make_metadata(options.target_coords,
                                                      batch, node_count);

  if (options.require_finite_context) {
    mdn_trainer_detail::check_finite_context(batch.context, batch.context_mask);
  }

  auto mask = mdnstream::combine_mdn_context_and_future_mask(batch.context_mask,
                                                             batch.future_mask);
  if (options.require_finite_targets_under_mask) {
    mdn_trainer_detail::check_finite_targets_under_mask(batch.future, mask);
  }

  report.valid_row_count =
      mask.any(std::vector<int64_t>{1, 2}).sum().template item<int64_t>();
  report.valid_target_count = mask.sum().template item<int64_t>();
  const int64_t total_possible_targets = mask.numel();
  report.valid_node_target_fraction =
      total_possible_targets == 0
          ? std::numeric_limits<double>::quiet_NaN()
          : static_cast<double>(report.valid_target_count) /
                static_cast<double>(total_possible_targets);
  report.low_target_coverage_warning =
      std::isfinite(report.valid_node_target_fraction) &&
      report.valid_node_target_fraction <
          options.low_target_coverage_warning_fraction;

  if (report.valid_target_count == 0) {
    report.skipped_empty = options.skip_empty_batches;
    return report;
  }

  optimizer.zero_grad();

  std::vector<torch::Tensor> node_loss_sums;
  std::vector<torch::Tensor> node_valid_counts;
  node_loss_sums.reserve(node_count);
  node_valid_counts.reserve(node_count);

  auto nll_by_channel_sum =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto nll_by_channel_den =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto nll_by_horizon_sum =
      torch::zeros({batch.future.size(2)}, batch.context.options());
  auto nll_by_horizon_den =
      torch::zeros({batch.future.size(2)}, batch.context.options());
  auto valid_target_count_by_channel =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto valid_target_count_by_horizon =
      torch::zeros({batch.future.size(2)}, batch.context.options());

  double entropy_sum = 0.0;
  double entropy_min = std::numeric_limits<double>::infinity();
  int64_t entropy_count = 0;
  double sigma_sum = 0.0;
  double sigma_min = std::numeric_limits<double>::infinity();
  double sigma_max = -std::numeric_limits<double>::infinity();
  int64_t sigma_count = 0;

  for (int64_t node_slot = 0; node_slot < node_count; ++node_slot) {
    auto routed_rows = mdnstream::mdn_rows_for_node(batch, node_slot);
    report.routed_row_count_by_node[node_slot] = routed_rows.numel();
    auto rows = mdn_trainer_detail::valid_training_rows(routed_rows, mask);
    if (rows.numel() == 0) {
      ++report.skipped_node_head_count;
      continue;
    }

    auto context_n = batch.context.index_select(/*dim=*/0, rows);
    auto target_n = batch.future.index_select(/*dim=*/0, rows);
    auto mask_n = mask.index_select(/*dim=*/0, rows);
    const int64_t valid_count = mask_n.sum().template item<int64_t>();
    if (valid_count <= 0) {
      ++report.skipped_node_head_count;
      continue;
    }

    auto out = node_heads[node_slot]->forward_from_encoding(context_n);
    auto nll = mdn::mdn_nll_map(out, target_n, mask_n, options.nll_options);
    auto valid_f = mask_n.to(nll.scalar_type());
    auto loss_sum = nll.sum();
    auto valid_count_t = valid_f.sum().clamp_min(1.0);

    node_loss_sums.push_back(loss_sum);
    node_valid_counts.push_back(valid_count_t);
    report.active_row_count_by_node[node_slot] = rows.numel();
    report.trained_row_count_by_node[node_slot] = rows.numel();
    report.valid_target_count_by_node[node_slot] = valid_count;
    report.nll_by_node[node_slot] =
        mdn_trainer_detail::tensor_scalar_or_nan(loss_sum / valid_count_t);
    ++report.active_node_head_count;
    ++report.trained_node_head_count;

    nll_by_channel_sum =
        nll_by_channel_sum + nll.sum(std::vector<int64_t>{0, 2});
    nll_by_channel_den =
        nll_by_channel_den + valid_f.sum(std::vector<int64_t>{0, 2});
    nll_by_horizon_sum =
        nll_by_horizon_sum + nll.sum(std::vector<int64_t>{0, 1});
    nll_by_horizon_den =
        nll_by_horizon_den + valid_f.sum(std::vector<int64_t>{0, 1});
    valid_target_count_by_channel =
        valid_target_count_by_channel + valid_f.sum(std::vector<int64_t>{0, 2});
    valid_target_count_by_horizon =
        valid_target_count_by_horizon + valid_f.sum(std::vector<int64_t>{0, 1});

    mdn_trainer_detail::accumulate_output_stats(
        report, out, mask_n, entropy_sum, entropy_min, entropy_count, sigma_sum,
        sigma_min, sigma_max, sigma_count);
  }

  if (node_loss_sums.empty()) {
    report.skipped_empty = options.skip_empty_batches;
    return report;
  }

  auto total_loss = torch::stack(node_loss_sums).sum() /
                    torch::stack(node_valid_counts).sum().clamp_min(1.0);
  total_loss.backward();
  if (options.grad_clip_norm > 0.0) {
    std::vector<torch::Tensor> params;
    for (int64_t node_slot = 0; node_slot < node_count; ++node_slot) {
      for (auto &param : node_heads[node_slot]->parameters()) {
        params.push_back(param);
      }
    }
    torch::nn::utils::clip_grad_norm_(params, options.grad_clip_norm);
  }
  optimizer.step();

  report.trained = true;
  report.optimizer_step_applied = true;
  report.loss = mdn_trainer_detail::tensor_scalar_or_nan(total_loss);

  auto context_norm = batch.context.norm(2, /*dim=*/1);
  auto valid_context_norm =
      context_norm.masked_select(batch.context_mask.to(torch::kBool));
  if (valid_context_norm.numel() > 0) {
    report.node_context_norm_mean = valid_context_norm.mean()
                                        .detach()
                                        .to(torch::kCPU)
                                        .template item<double>();
    report.node_context_norm_max = valid_context_norm.max()
                                       .detach()
                                       .to(torch::kCPU)
                                       .template item<double>();
  }
  if (entropy_count > 0) {
    report.mixture_entropy_mean =
        entropy_sum / static_cast<double>(entropy_count);
    report.mixture_entropy_min = entropy_min;
  }
  if (sigma_count > 0) {
    report.sigma_mean = sigma_sum / static_cast<double>(sigma_count);
    report.sigma_min = sigma_min;
    report.sigma_max = sigma_max;
  }
  report.valid_target_count_by_channel = valid_target_count_by_channel.detach();
  report.valid_target_count_by_horizon = valid_target_count_by_horizon.detach();
  report.nll_by_channel =
      (nll_by_channel_sum / nll_by_channel_den.clamp_min(1.0)).detach();
  report.nll_by_horizon =
      (nll_by_horizon_sum / nll_by_horizon_den.clamp_min(1.0)).detach();
  return report;
}

template <typename KeyT>
[[nodiscard]] mdn_train_report_t evaluate_mdn_batch(
    const cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        mdn_input_batch_t<KeyT> &batch,
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        &node_heads,
    const mdn_training_options_t &options = {}) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace mdnstream =
      cuwacunu::wikimyei::inference::expected_value::mdn::stream;

  TORCH_CHECK(!node_heads.empty(), "[mdn_trainer] node_heads is empty");
  TORCH_CHECK(batch.context.defined() && batch.context.dim() == 2,
              "[mdn_trainer] context must be [B_node,D]");
  TORCH_CHECK(batch.context_mask.defined() && batch.context_mask.dim() == 1 &&
                  batch.context_mask.size(0) == batch.context.size(0),
              "[mdn_trainer] context_mask must be [B_node]");
  TORCH_CHECK(batch.node_index.defined() && batch.node_index.dim() == 1 &&
                  batch.node_index.size(0) == batch.context.size(0),
              "[mdn_trainer] node_index must be [B_node]");
  TORCH_CHECK(batch.future.defined() && batch.future.dim() == 4,
              "[mdn_trainer] future must be [B_node,C,Hf,D]");
  TORCH_CHECK(batch.future_mask.defined() && batch.future_mask.dim() == 3 &&
                  batch.future_mask.size(0) == batch.future.size(0) &&
                  batch.future_mask.size(1) == batch.future.size(1) &&
                  batch.future_mask.size(2) == batch.future.size(2),
              "[mdn_trainer] future_mask must be [B_node,C,Hf]");

  const int64_t inferred_node_count =
      batch.node_index.numel() == 0
          ? 0
          : batch.node_index.max().template item<int64_t>() + 1;
  const int64_t node_count = batch.node_ids.empty()
                                 ? inferred_node_count
                                 : static_cast<int64_t>(batch.node_ids.size());
  TORCH_CHECK(node_count > 0, "[mdn_trainer] node_count must be positive");
  TORCH_CHECK(static_cast<int64_t>(node_heads.size()) >= node_count,
              "[mdn_trainer] not enough per-node MDN heads");

  mdn_train_report_t report{};
  report.row_count = batch.context.size(0);
  report.node_count = node_count;
  report.routed_row_count_by_node.assign(node_count, 0);
  report.active_row_count_by_node.assign(node_count, 0);
  report.trained_row_count_by_node.assign(node_count, 0);
  report.valid_target_count_by_node.assign(node_count, 0);
  report.nll_by_node.assign(node_count,
                            std::numeric_limits<double>::quiet_NaN());
  report.metadata = mdn_trainer_detail::make_metadata(options.target_coords,
                                                      batch, node_count);

  if (options.require_finite_context) {
    mdn_trainer_detail::check_finite_context(batch.context, batch.context_mask);
  }
  auto mask = mdnstream::combine_mdn_context_and_future_mask(batch.context_mask,
                                                             batch.future_mask);
  if (options.require_finite_targets_under_mask) {
    mdn_trainer_detail::check_finite_targets_under_mask(batch.future, mask);
  }

  report.valid_row_count =
      mask.any(std::vector<int64_t>{1, 2}).sum().template item<int64_t>();
  report.valid_target_count = mask.sum().template item<int64_t>();
  const int64_t total_possible_targets = mask.numel();
  report.valid_node_target_fraction =
      total_possible_targets == 0
          ? std::numeric_limits<double>::quiet_NaN()
          : static_cast<double>(report.valid_target_count) /
                static_cast<double>(total_possible_targets);
  report.low_target_coverage_warning =
      std::isfinite(report.valid_node_target_fraction) &&
      report.valid_node_target_fraction <
          options.low_target_coverage_warning_fraction;
  if (report.valid_target_count == 0) {
    report.skipped_empty = options.skip_empty_batches;
    return report;
  }

  auto nll_by_channel_sum =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto nll_by_channel_den =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto nll_by_horizon_sum =
      torch::zeros({batch.future.size(2)}, batch.context.options());
  auto nll_by_horizon_den =
      torch::zeros({batch.future.size(2)}, batch.context.options());
  auto valid_target_count_by_channel =
      torch::zeros({batch.future.size(1)}, batch.context.options());
  auto valid_target_count_by_horizon =
      torch::zeros({batch.future.size(2)}, batch.context.options());

  double entropy_sum = 0.0;
  double entropy_min = std::numeric_limits<double>::infinity();
  int64_t entropy_count = 0;
  double sigma_sum = 0.0;
  double sigma_min = std::numeric_limits<double>::infinity();
  double sigma_max = -std::numeric_limits<double>::infinity();
  int64_t sigma_count = 0;
  std::vector<torch::Tensor> node_loss_sums;
  std::vector<torch::Tensor> node_valid_counts;

  torch::NoGradGuard no_grad;
  for (int64_t node_slot = 0; node_slot < node_count; ++node_slot) {
    auto routed_rows = mdnstream::mdn_rows_for_node(batch, node_slot);
    report.routed_row_count_by_node[node_slot] = routed_rows.numel();
    auto rows = mdn_trainer_detail::valid_training_rows(routed_rows, mask);
    if (rows.numel() == 0) {
      ++report.skipped_node_head_count;
      continue;
    }
    auto context_n = batch.context.index_select(/*dim=*/0, rows);
    auto target_n = batch.future.index_select(/*dim=*/0, rows);
    auto mask_n = mask.index_select(/*dim=*/0, rows);
    const int64_t valid_count = mask_n.sum().template item<int64_t>();
    if (valid_count <= 0) {
      ++report.skipped_node_head_count;
      continue;
    }
    node_heads[node_slot]->eval();
    auto out = node_heads[node_slot]->forward_from_encoding(context_n);
    auto nll = mdn::mdn_nll_map(out, target_n, mask_n, options.nll_options);
    auto valid_f = mask_n.to(nll.scalar_type());
    auto loss_sum = nll.sum();
    auto valid_count_t = valid_f.sum().clamp_min(1.0);

    node_loss_sums.push_back(loss_sum);
    node_valid_counts.push_back(valid_count_t);
    report.active_row_count_by_node[node_slot] = rows.numel();
    report.valid_target_count_by_node[node_slot] = valid_count;
    report.nll_by_node[node_slot] =
        mdn_trainer_detail::tensor_scalar_or_nan(loss_sum / valid_count_t);
    ++report.active_node_head_count;
    ++report.evaluated_node_head_count;

    nll_by_channel_sum =
        nll_by_channel_sum + nll.sum(std::vector<int64_t>{0, 2});
    nll_by_channel_den =
        nll_by_channel_den + valid_f.sum(std::vector<int64_t>{0, 2});
    nll_by_horizon_sum =
        nll_by_horizon_sum + nll.sum(std::vector<int64_t>{0, 1});
    nll_by_horizon_den =
        nll_by_horizon_den + valid_f.sum(std::vector<int64_t>{0, 1});
    valid_target_count_by_channel =
        valid_target_count_by_channel + valid_f.sum(std::vector<int64_t>{0, 2});
    valid_target_count_by_horizon =
        valid_target_count_by_horizon + valid_f.sum(std::vector<int64_t>{0, 1});

    mdn_trainer_detail::accumulate_output_stats(
        report, out, mask_n, entropy_sum, entropy_min, entropy_count, sigma_sum,
        sigma_min, sigma_max, sigma_count);
  }

  if (node_loss_sums.empty()) {
    report.skipped_empty = options.skip_empty_batches;
    return report;
  }

  auto total_loss = torch::stack(node_loss_sums).sum() /
                    torch::stack(node_valid_counts).sum().clamp_min(1.0);
  report.loss = mdn_trainer_detail::tensor_scalar_or_nan(total_loss);

  auto context_norm = batch.context.norm(2, /*dim=*/1);
  auto valid_context_norm =
      context_norm.masked_select(batch.context_mask.to(torch::kBool));
  if (valid_context_norm.numel() > 0) {
    report.node_context_norm_mean = valid_context_norm.mean()
                                        .detach()
                                        .to(torch::kCPU)
                                        .template item<double>();
    report.node_context_norm_max = valid_context_norm.max()
                                       .detach()
                                       .to(torch::kCPU)
                                       .template item<double>();
  }
  if (entropy_count > 0) {
    report.mixture_entropy_mean =
        entropy_sum / static_cast<double>(entropy_count);
    report.mixture_entropy_min = entropy_min;
  }
  if (sigma_count > 0) {
    report.sigma_mean = sigma_sum / static_cast<double>(sigma_count);
    report.sigma_min = sigma_min;
    report.sigma_max = sigma_max;
  }
  report.valid_target_count_by_channel = valid_target_count_by_channel.detach();
  report.valid_target_count_by_horizon = valid_target_count_by_horizon.detach();
  report.nll_by_channel =
      (nll_by_channel_sum / nll_by_channel_den.clamp_min(1.0)).detach();
  report.nll_by_horizon =
      (nll_by_horizon_sum / nll_by_horizon_den.clamp_min(1.0)).detach();
  return report;
}

template <typename KeyT>
[[nodiscard]] mdn_train_report_t train_mdn_batch(
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        node_representation_batch_t<KeyT> &nodes,
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        &node_heads,
    torch::optim::Optimizer &optimizer,
    const mdn_training_options_t &options = {}) {
  auto mdn_batch = cuwacunu::wikimyei::inference::expected_value::mdn::stream::
      make_mdn_input_batch(nodes, options.adapter_options);
  return train_mdn_batch(mdn_batch, node_heads, optimizer, options);
}

} // namespace cuwacunu::jkimyei::training::inference

namespace cuwacunu::jkimyei::training {

[[nodiscard]] inline inference::mdn_training_options_t
mdn_training_options_from_specs(
    const training_run_spec_t &training,
    const cuwacunu::wikimyei::inference::expected_value::mdn::mdn_spec_t &mdn) {
  validate_training_run_spec(training);
  cuwacunu::wikimyei::inference::expected_value::mdn::validate_mdn_spec(mdn);
  if (training.task != training_task_t::mdn_expected_value_inference) {
    throw std::runtime_error("[training_spec] training task is not "
                             "mdn_expected_value_inference");
  }
  inference::mdn_training_options_t out{};
  out.target_coords = mdn.target_coords;
  out.grad_clip_norm = training.grad_clip_norm;
  out.nll_options = cuwacunu::wikimyei::inference::expected_value::mdn::
      mdn_nll_options_from_spec(mdn);
  out.adapter_options = cuwacunu::wikimyei::inference::expected_value::mdn::
      mdn_adapter_options_from_spec(mdn);
  return out;
}

} // namespace cuwacunu::jkimyei::training
