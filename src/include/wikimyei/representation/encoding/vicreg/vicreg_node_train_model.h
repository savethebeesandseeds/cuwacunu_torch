// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/representation/encoding/vicreg/vicreg_rank4_encoder.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct vicreg_node_train_options_t {
  double invariance_weight{25.0};
  double variance_weight{25.0};
  double covariance_weight{1.0};
  double variance_floor{1.0};
  double jitter_std{0.01};
  double grad_clip_norm{0.0};
  int64_t min_valid_rows{2};
  double eps{1e-4};
  bool skip_non_finite_loss{true};
};

struct vicreg_node_train_step_result_t {
  torch::Tensor loss{};
  torch::Tensor invariance_loss{};
  torch::Tensor variance_loss{};
  torch::Tensor covariance_loss{};
  int64_t valid_projection_rows{0};
  bool skipped{false};
  bool optimizer_step_applied{false};
  bool gradients_finite{true};
  double grad_norm{std::numeric_limits<double>::quiet_NaN()};
};

namespace vicreg_node_train_detail {

[[nodiscard]] inline torch::Tensor
augment_rank4_view(const torch::Tensor &data, const torch::Tensor &mask,
                   const vicreg_node_train_options_t &options) {
  auto out = data;
  if (options.jitter_std > 0.0) {
    auto noise = torch::randn_like(data) * options.jitter_std;
    out = out + noise.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
  }
  return out.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
}

[[nodiscard]] inline torch::Tensor valid_encoded_rows(const torch::Tensor &z,
                                                      const torch::Tensor &mask) {
  TORCH_CHECK(mask.dim() == 3,
              "[vicreg_node_train_model] mask must be [B,C,H]");
  if (z.dim() == 3) {
    auto time_mask = mask.any(/*dim=*/1); // [B,H]
    TORCH_CHECK(z.size(0) == time_mask.size(0) &&
                    z.size(1) == time_mask.size(1),
                "[vicreg_node_train_model] encoded sequence shape mismatch");
    return z.masked_select(time_mask.unsqueeze(-1).expand_as(z))
        .reshape({-1, z.size(-1)});
  }
  if (z.dim() == 2) {
    auto row_mask = mask.any(/*dim=*/1).any(/*dim=*/1);
    TORCH_CHECK(z.size(0) == row_mask.size(0),
                "[vicreg_node_train_model] encoded row shape mismatch");
    auto rows = row_mask.nonzero().reshape({-1});
    return z.index_select(/*dim=*/0, rows);
  }
  TORCH_CHECK(false,
              "[vicreg_node_train_model] encoder output must be rank 2 or 3");
}

[[nodiscard]] inline torch::Tensor variance_term(const torch::Tensor &z,
                                                 double floor, double eps) {
  auto std = torch::sqrt(z.var(/*dim=*/0, /*unbiased=*/false) + eps);
  return torch::relu(floor - std).mean();
}

[[nodiscard]] inline torch::Tensor covariance_term(const torch::Tensor &z) {
  const auto rows = z.size(0);
  const auto dim = z.size(1);
  if (rows <= 1 || dim <= 1) {
    return torch::zeros({}, z.options());
  }
  auto centered = z - z.mean(/*dim=*/0, /*keepdim=*/true);
  auto cov = centered.transpose(0, 1).matmul(centered) /
             static_cast<double>(rows - 1);
  auto off_diag = cov - torch::diag(torch::diag(cov));
  return off_diag.pow(2).sum() / static_cast<double>(dim);
}

[[nodiscard]] inline bool parameters_are_finite(
    const std::vector<torch::Tensor> &params, bool inspect_grad) {
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

[[nodiscard]] inline double gradient_norm(
    const std::vector<torch::Tensor> &params) {
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

inline void clip_gradients(std::vector<torch::Tensor> &params,
                           double clip_norm, double current_norm) {
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

} // namespace vicreg_node_train_detail

class vicreg_node_train_model_t {
public:
  vicreg_node_train_model_t(
      VICReg_Rank4_Encoder encoder, double learning_rate,
      vicreg_node_train_options_t options = {})
      : encoder_(std::move(encoder)), options_(std::move(options)) {
    if (learning_rate <= 0.0) {
      throw std::runtime_error(
          "[vicreg_node_train_model] learning_rate must be positive");
    }
    params_ = encoder_->parameters();
    optimizer_ = std::make_unique<torch::optim::Adam>(
        params_, torch::optim::AdamOptions(learning_rate));
  }

  [[nodiscard]] torch::Tensor encode(const torch::Tensor &data,
                                     const torch::Tensor &mask, bool,
                                     bool detach_to_cpu) {
    encoder_->eval();
    torch::NoGradGuard no_grad;
    auto out = encoder_->forward(
        data.to(torch::TensorOptions().dtype(encoder_->dtype)
                    .device(encoder_->device)),
        mask.to(torch::TensorOptions().dtype(torch::kBool)
                    .device(encoder_->device)));
    if (detach_to_cpu) {
      out = out.detach().to(torch::kCPU);
    }
    return out;
  }

  [[nodiscard]] vicreg_node_train_step_result_t
  train_one_batch(const torch::Tensor &data, const torch::Tensor &mask, int,
                  bool) {
    TORCH_CHECK(data.defined() && data.dim() == 4,
                "[vicreg_node_train_model] data must be [B,C,H,D]");
    TORCH_CHECK(mask.defined() && mask.dim() == 3,
                "[vicreg_node_train_model] mask must be [B,C,H]");
    TORCH_CHECK(data.size(0) == mask.size(0) && data.size(1) == mask.size(1) &&
                    data.size(2) == mask.size(2),
                "[vicreg_node_train_model] data/mask shape mismatch");

    vicreg_node_train_step_result_t out{};
    auto data_d = data.detach().to(torch::TensorOptions()
                                       .dtype(encoder_->dtype)
                                       .device(encoder_->device));
    auto mask_d = mask.detach().to(torch::TensorOptions()
                                       .dtype(torch::kBool)
                                       .device(encoder_->device));
    const auto valid_time_count =
        mask_d.any(/*dim=*/1).sum().template item<int64_t>();
    if (valid_time_count < options_.min_valid_rows) {
      out.skipped = true;
      out.loss = torch::zeros({}, data_d.options());
      return out;
    }

    encoder_->train();
    optimizer_->zero_grad();
    const auto view1 =
        vicreg_node_train_detail::augment_rank4_view(data_d, mask_d, options_);
    const auto view2 =
        vicreg_node_train_detail::augment_rank4_view(data_d, mask_d, options_);
    auto z1 = vicreg_node_train_detail::valid_encoded_rows(
        encoder_->forward(view1, mask_d), mask_d);
    auto z2 = vicreg_node_train_detail::valid_encoded_rows(
        encoder_->forward(view2, mask_d), mask_d);
    out.valid_projection_rows = std::min(z1.size(0), z2.size(0));
    if (out.valid_projection_rows < options_.min_valid_rows) {
      out.skipped = true;
      out.loss = torch::zeros({}, data_d.options());
      return out;
    }
    if (z1.size(0) != out.valid_projection_rows) {
      z1 = z1.narrow(0, 0, out.valid_projection_rows);
    }
    if (z2.size(0) != out.valid_projection_rows) {
      z2 = z2.narrow(0, 0, out.valid_projection_rows);
    }

    out.invariance_loss = torch::mse_loss(z1, z2);
    out.variance_loss =
        0.5 * (vicreg_node_train_detail::variance_term(
                   z1, options_.variance_floor, options_.eps) +
               vicreg_node_train_detail::variance_term(
                   z2, options_.variance_floor, options_.eps));
    out.covariance_loss =
        0.5 * (vicreg_node_train_detail::covariance_term(z1) +
               vicreg_node_train_detail::covariance_term(z2));
    out.loss = options_.invariance_weight * out.invariance_loss +
               options_.variance_weight * out.variance_loss +
               options_.covariance_weight * out.covariance_loss;

    if (!torch::isfinite(out.loss).all().template item<bool>()) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false, "[vicreg_node_train_model] non-finite loss");
      }
      return out;
    }

    out.loss.backward();
    out.grad_norm = vicreg_node_train_detail::gradient_norm(params_);
    vicreg_node_train_detail::clip_gradients(params_, options_.grad_clip_norm,
                                             out.grad_norm);
    out.gradients_finite =
        vicreg_node_train_detail::parameters_are_finite(params_,
                                                        /*inspect_grad=*/true);
    if (!out.gradients_finite) {
      out.skipped = true;
      if (!options_.skip_non_finite_loss) {
        TORCH_CHECK(false, "[vicreg_node_train_model] non-finite gradients");
      }
      optimizer_->zero_grad();
      return out;
    }

    optimizer_->step();
    out.optimizer_step_applied = true;
    return out;
  }

  [[nodiscard]] VICReg_Rank4_Encoder &encoder() { return encoder_; }
  [[nodiscard]] const VICReg_Rank4_Encoder &encoder() const { return encoder_; }
  [[nodiscard]] torch::optim::Adam &optimizer() { return *optimizer_; }

private:
  VICReg_Rank4_Encoder encoder_{nullptr};
  vicreg_node_train_options_t options_{};
  std::vector<torch::Tensor> params_{};
  std::unique_ptr<torch::optim::Adam> optimizer_{};
};

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
