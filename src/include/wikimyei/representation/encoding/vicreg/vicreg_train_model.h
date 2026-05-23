// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/representation/encoding/vicreg/channel_preserving_encoder.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_loss.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_projector.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct vicreg_train_options_t {
  vicreg_loss_options_t vicreg{};
  double jitter_std{0.01};
  double feature_dropout_prob{0.0};
  double history_dropout_prob{0.0};
  double grad_clip_norm{0.0};
  int64_t min_valid_rows{2};
  bool skip_non_finite_loss{true};
};

struct vicreg_train_step_result_t {
  torch::Tensor loss{};
  torch::Tensor invariance_loss{};
  torch::Tensor variance_loss{};
  torch::Tensor covariance_loss{};
  torch::Tensor global_loss{};
  torch::Tensor per_channel_valid_rows{};
  torch::Tensor per_channel_variance_loss{};
  torch::Tensor per_channel_covariance_loss{};
  torch::Tensor per_channel_norm_mean{};
  torch::Tensor cross_channel_similarity{};
  int64_t valid_projection_rows{0};
  int64_t view1_valid_feature_count{0};
  int64_t view2_valid_feature_count{0};
  double view1_valid_feature_fraction{std::numeric_limits<double>::quiet_NaN()};
  double view2_valid_feature_fraction{std::numeric_limits<double>::quiet_NaN()};
  double view1_feature_retention_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double view2_feature_retention_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  bool skipped{false};
  bool optimizer_step_applied{false};
  bool gradients_finite{true};
  double grad_norm{std::numeric_limits<double>::quiet_NaN()};
};

namespace vicreg_train_detail {

struct vicreg_augmented_view_t {
  torch::Tensor data{};
  torch::Tensor feature_mask{};
};

inline void
validate_augmentation_options(const vicreg_train_options_t &options) {
  TORCH_CHECK(options.jitter_std >= 0.0,
              "[vicreg_train_model] jitter_std must be >= 0");
  TORCH_CHECK(options.feature_dropout_prob >= 0.0 &&
                  options.feature_dropout_prob <= 1.0,
              "[vicreg_train_model] feature_dropout_prob must be in "
              "[0,1]");
  TORCH_CHECK(options.history_dropout_prob >= 0.0 &&
                  options.history_dropout_prob <= 1.0,
              "[vicreg_train_model] history_dropout_prob must be in "
              "[0,1]");
}

[[nodiscard]] inline vicreg_augmented_view_t
augment_channel_view(const torch::Tensor &data, const torch::Tensor &mask,
                     const vicreg_train_options_t &options) {
  validate_augmentation_options(options);
  auto mask_bool =
      channel_preserving_encoder_detail::make_feature_mask(data, mask);
  auto view_mask = mask_bool.clone();

  if (options.feature_dropout_prob > 0.0) {
    const auto keep_prob = 1.0 - options.feature_dropout_prob;
    const auto keep = torch::rand(data.sizes(), data.options()).lt(keep_prob);
    view_mask = view_mask.logical_and(keep);
  }
  if (options.history_dropout_prob > 0.0) {
    const auto keep_prob = 1.0 - options.history_dropout_prob;
    std::vector<int64_t> cell_shape;
    cell_shape.reserve(static_cast<size_t>(data.dim()));
    for (int64_t dim = 0; dim < data.dim(); ++dim) {
      cell_shape.push_back(data.size(dim));
    }
    cell_shape.back() = 1;
    const auto keep =
        torch::rand(cell_shape, data.options()).lt(keep_prob).expand_as(data);
    view_mask = view_mask.logical_and(keep);
  }

  auto out = torch::where(mask_bool, data, torch::zeros_like(data));
  if (options.jitter_std > 0.0) {
    auto noise = torch::randn_like(data) * options.jitter_std;
    out = out + torch::where(view_mask, noise, torch::zeros_like(noise));
  }
  out = torch::where(view_mask, out, torch::zeros_like(out));
  return {out, view_mask};
}

[[nodiscard]] inline int64_t
valid_feature_count(const torch::Tensor &feature_mask) {
  return feature_mask.to(torch::kBool).sum().template item<int64_t>();
}

[[nodiscard]] inline double fraction_or_nan(int64_t numerator,
                                            int64_t denominator) {
  if (denominator <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return static_cast<double>(numerator) / static_cast<double>(denominator);
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

inline vicreg_train_step_result_t
make_skipped_result(const torch::TensorOptions &options) {
  vicreg_train_step_result_t out{};
  out.skipped = true;
  out.loss = torch::zeros({}, options);
  return out;
}

} // namespace vicreg_train_detail

class vicreg_train_model_t {
public:
  vicreg_train_model_t(ChannelPreservingEncoder encoder,
                       VicregProjector projector, double learning_rate,
                       vicreg_train_options_t options = {})
      : encoder_(std::move(encoder)), projector_(std::move(projector)),
        options_(std::move(options)) {
    TORCH_CHECK(!encoder_.is_empty(), "[vicreg_train_model] encoder is null");
    TORCH_CHECK(!projector_.is_empty(),
                "[vicreg_train_model] projector is null");
    TORCH_CHECK(learning_rate > 0.0,
                "[vicreg_train_model] learning_rate must be positive");
    vicreg_train_detail::validate_augmentation_options(options_);
    params_ = encoder_->parameters();
    auto projector_params = projector_->parameters();
    params_.insert(params_.end(), projector_params.begin(),
                   projector_params.end());
    optimizer_ = std::make_unique<torch::optim::Adam>(
        params_, torch::optim::AdamOptions(learning_rate));
  }

  [[nodiscard]] channel_preserving_encoder_output_t
  encode(const torch::Tensor &data, const torch::Tensor &feature_mask,
         bool detach_to_cpu = false) {
    encoder_->eval();
    torch::NoGradGuard no_grad;
    auto out = encoder_->forward(
        data.to(torch::TensorOptions()
                    .dtype(encoder_->options().dtype)
                    .device(encoder_->options().device)),
        feature_mask.to(torch::TensorOptions()
                            .dtype(torch::kBool)
                            .device(encoder_->options().device)));
    if (detach_to_cpu) {
      out.sequence = out.sequence.detach().to(torch::kCPU);
      out.sequence_mask = out.sequence_mask.detach().to(torch::kCPU);
      out.reduced = out.reduced.detach().to(torch::kCPU);
      out.reduced_mask = out.reduced_mask.detach().to(torch::kCPU);
      out.reducer_weights = out.reducer_weights.detach().to(torch::kCPU);
    }
    return out;
  }

  [[nodiscard]] vicreg_train_step_result_t
  train_one_batch(const torch::Tensor &data,
                  const torch::Tensor &feature_mask) {
    TORCH_CHECK(data.defined() && data.dim() == 4,
                "[vicreg_train_model] data must be [M,C,Hx,Dx]");
    TORCH_CHECK(feature_mask.defined() && feature_mask.dim() == 4,
                "[vicreg_train_model] feature_mask must be "
                "[M,C,Hx,Dx]");
    TORCH_CHECK(data.sizes() == feature_mask.sizes(),
                "[vicreg_train_model] data/mask shape mismatch");

    auto data_d = data.detach().to(torch::TensorOptions()
                                       .dtype(encoder_->options().dtype)
                                       .device(encoder_->options().device));
    auto mask_d =
        feature_mask.detach().to(torch::TensorOptions()
                                     .dtype(torch::kBool)
                                     .device(encoder_->options().device));
    const auto valid_feature =
        channel_preserving_encoder_detail::make_feature_mask(data_d, mask_d);
    const auto sequence_mask =
        channel_preserving_encoder_detail::make_sequence_mask(
            valid_feature, encoder_->options());
    const auto valid_cells = sequence_mask.sum().template item<int64_t>();
    if (valid_cells < options_.min_valid_rows) {
      return vicreg_train_detail::make_skipped_result(data_d.options());
    }

    encoder_->train();
    projector_->train();
    optimizer_->zero_grad();

    const auto view1 =
        vicreg_train_detail::augment_channel_view(data_d, mask_d, options_);
    const auto view2 =
        vicreg_train_detail::augment_channel_view(data_d, mask_d, options_);
    const auto enc1 = encoder_->forward(view1.data, view1.feature_mask);
    const auto enc2 = encoder_->forward(view2.data, view2.feature_mask);
    auto p1 = projector_->forward(enc1.reduced);
    auto p2 = projector_->forward(enc2.reduced);
    auto vicreg = compute_vicreg_loss(p1, enc1.reduced_mask, p2,
                                      enc2.reduced_mask, options_.vicreg);

    vicreg_train_step_result_t out{};
    out.loss = vicreg.loss;
    out.invariance_loss = vicreg.invariance_loss;
    out.variance_loss = vicreg.variance_loss;
    out.covariance_loss = vicreg.covariance_loss;
    out.global_loss = vicreg.global_loss;
    out.per_channel_valid_rows = vicreg.per_channel_valid_rows;
    out.per_channel_variance_loss = vicreg.per_channel_variance_loss;
    out.per_channel_covariance_loss = vicreg.per_channel_covariance_loss;
    out.per_channel_norm_mean = vicreg.per_channel_norm_mean;
    out.cross_channel_similarity = vicreg.cross_channel_similarity;
    out.valid_projection_rows = vicreg.valid_rows;
    const auto original_valid_features =
        vicreg_train_detail::valid_feature_count(valid_feature);
    const auto total_features = mask_d.numel();
    out.view1_valid_feature_count =
        vicreg_train_detail::valid_feature_count(view1.feature_mask);
    out.view2_valid_feature_count =
        vicreg_train_detail::valid_feature_count(view2.feature_mask);
    out.view1_valid_feature_fraction = vicreg_train_detail::fraction_or_nan(
        out.view1_valid_feature_count, total_features);
    out.view2_valid_feature_fraction = vicreg_train_detail::fraction_or_nan(
        out.view2_valid_feature_count, total_features);
    out.view1_feature_retention_fraction = vicreg_train_detail::fraction_or_nan(
        out.view1_valid_feature_count, original_valid_features);
    out.view2_feature_retention_fraction = vicreg_train_detail::fraction_or_nan(
        out.view2_valid_feature_count, original_valid_features);
    if (out.valid_projection_rows < options_.min_valid_rows) {
      out.skipped = true;
      out.loss = torch::zeros({}, data_d.options());
      return out;
    }
    if (!torch::isfinite(out.loss).all().template item<bool>()) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false, "[vicreg_train_model] non-finite loss");
      }
      return out;
    }

    out.loss.backward();
    out.grad_norm = vicreg_train_detail::gradient_norm(params_);
    vicreg_train_detail::clip_gradients(params_, options_.grad_clip_norm,
                                        out.grad_norm);
    out.gradients_finite = vicreg_train_detail::parameters_are_finite(
        params_, /*inspect_grad=*/true);
    if (!out.gradients_finite) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false, "[vicreg_train_model] non-finite gradients");
      }
      optimizer_->zero_grad();
      return out;
    }

    optimizer_->step();
    out.optimizer_step_applied = true;
    return out;
  }

  [[nodiscard]] ChannelPreservingEncoder &encoder() { return encoder_; }
  [[nodiscard]] const ChannelPreservingEncoder &encoder() const {
    return encoder_;
  }
  [[nodiscard]] VicregProjector &projector() { return projector_; }
  [[nodiscard]] const VicregProjector &projector() const { return projector_; }
  [[nodiscard]] torch::optim::Adam &optimizer() { return *optimizer_; }

private:
  ChannelPreservingEncoder encoder_{nullptr};
  VicregProjector projector_{nullptr};
  vicreg_train_options_t options_{};
  std::vector<torch::Tensor> params_{};
  std::unique_ptr<torch::optim::Adam> optimizer_{};
};

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
