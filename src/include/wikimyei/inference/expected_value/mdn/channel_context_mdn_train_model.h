// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
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
};

struct channel_context_mdn_train_step_result_t {
  torch::Tensor loss{};
  torch::Tensor nll{};
  torch::Tensor nll_per_channel{};
  torch::Tensor nll_per_target_feature{};
  torch::Tensor mixture_usage{};
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
  bool skipped{false};
  bool optimizer_step_applied{false};
  bool gradients_finite{true};
  double grad_norm{std::numeric_limits<double>::quiet_NaN()};
};

namespace channel_context_mdn_train_detail {

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
  return torch::isfinite(out.log_pi)
             .logical_not()
             .sum()
             .template item<int64_t>() +
         torch::isfinite(out.mu).logical_not().sum().template item<int64_t>() +
         torch::isfinite(out.sigma)
             .logical_not()
             .sum()
             .template item<int64_t>();
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
            mdn_out, clean_input.future, combined_mask, options_.nll);
    out.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::mdn::
        mdn_masked_mean_per_channel(nll_map, combined_mask);
    out.nll_per_target_feature = cuwacunu::wikimyei::inference::expected_value::
        mdn::mdn_masked_mean_per_target_feature(nll_map, combined_mask);
    out.nll =
        compute_channel_context_mdn_nll(mdn_out, clean_input, options_.nll);
    out.loss = out.nll;
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
            mdn_out, clean_input.channel.future, combined_mask, options_.nll);
    out.nll_per_channel = cuwacunu::wikimyei::inference::expected_value::mdn::
        mdn_masked_mean_per_channel(nll_map, combined_mask);
    out.nll_per_target_feature = cuwacunu::wikimyei::inference::expected_value::
        mdn::mdn_masked_mean_per_target_feature(nll_map, combined_mask);
    out.nll = compute_channel_context_plus_global_mdn_nll(mdn_out, clean_input,
                                                          options_.nll);
    out.loss = out.nll;
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
