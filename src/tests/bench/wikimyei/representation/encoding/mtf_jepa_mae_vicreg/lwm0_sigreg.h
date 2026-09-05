#pragma once

#include <c10/core/impl/LocalDispatchKeySet.h>
#include <torch/torch.h>

#include <algorithm>
#include <cmath>

namespace lwm0 {

inline constexpr double kSigregWeight = 0.09;

inline torch::Tensor make_directions(torch::Device device) {
  auto directions = torch::randn(
      {32, 1024}, torch::TensorOptions().dtype(torch::kFloat32).device(device));
  return directions / directions.square().sum(0, true).sqrt();
}

// LeWM reference-code quadrature, independently over batch at each
// time/channel. The temporal axis is W, never C. Casting preserves the encoder
// autograd graph.
inline torch::Tensor sigreg(torch::Tensor states, torch::Tensor directions,
                            bool temporal_centered) {
  TORCH_CHECK(states.dim() == 4 && states.size(0) > 1 && states.size(1) > 0 &&
                  states.size(2) > 0 && states.size(3) == 32,
              "LWM-0 SIGReg requires [B>1,W,C,32]");
  TORCH_CHECK(directions.dim() == 2 && directions.size(0) == 32 &&
                  directions.size(1) == 1024 &&
                  directions.device() == states.device(),
              "LWM-0 directions must be [32,1024] on the states device");
  const c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKeySet{
      c10::DispatchKey::AutocastCPU, c10::DispatchKey::AutocastCUDA});
  states = states.to(torch::kFloat32);
  directions = directions.to(torch::kFloat32);
  if (temporal_centered)
    states = states - states.mean(1, true);

  const auto options = states.options().requires_grad(false);
  const auto frequencies = torch::linspace(0.0, 3.0, 17, options);
  const auto phi = torch::exp(-frequencies.square() / 2.0);
  auto weights = torch::full({17}, 2.0 * (3.0 / 16.0), options);
  weights.select(0, 0).fill_(3.0 / 16.0);
  weights.select(0, 16).fill_(3.0 / 16.0);
  weights = weights * phi;

  // [B,W,C,M,K]; at the audited B=64 this phase tensor occupies about 40 MiB.
  const auto phases =
      torch::matmul(states, directions).unsqueeze(-1) * frequencies;
  const auto real = phases.cos().mean(0) - phi;
  const auto imaginary = phases.sin().mean(0);
  const auto discrepancy = real.square() + imaginary.square();
  return (discrepancy * weights).sum(-1).mean() * states.size(0);
}

struct SigregFixtures {
  bool pass = false;
  double collapsed_loss = 0.0;
  double gaussian_loss = 0.0;
  double static_raw_loss = 0.0;
  double static_centered_loss = 0.0;
  double collapsed_gradient_norm = 0.0;
  double centering_invariance_error = 0.0;
  double batch_permutation_error = 0.0;
  double channel_permutation_error = 0.0;
};

inline SigregFixtures check_sigreg_fixtures(torch::Device device,
                                            torch::Tensor directions) {
  torch::NoGradGuard no_grad;
  SigregFixtures result;
  const auto options =
      torch::TensorOptions().dtype(torch::kFloat32).device(device);
  {
    torch::AutoGradMode enable_grad(true);
    auto collapsed = torch::zeros({64, 3, 3, 32}, options.requires_grad(true));
    const auto loss = sigreg(collapsed, directions, false);
    result.collapsed_loss = loss.item<double>();
    const auto gradient = torch::autograd::grad({loss}, {collapsed}).at(0);
    result.collapsed_gradient_norm = gradient.norm().item<double>();
  }

  const auto gaussian = torch::randn({64, 3, 3, 32}, options);
  result.gaussian_loss = sigreg(gaussian, directions, false).item<double>();
  const double centered_loss =
      sigreg(gaussian, directions, true).item<double>();
  const auto static_states =
      torch::randn({64, 1, 3, 32}, options).expand({64, 3, 3, 32});
  result.static_raw_loss =
      sigreg(static_states, directions, false).item<double>();
  result.static_centered_loss =
      sigreg(static_states, directions, true).item<double>();

  const auto offset = 0.05 * torch::randn({64, 1, 3, 32}, options);
  const double shifted_loss =
      sigreg(gaussian + offset, directions, true).item<double>();
  result.centering_invariance_error = std::abs(centered_loss - shifted_loss);
  const auto batch_order = torch::randperm(64, options.dtype(torch::kLong));
  const auto channel_order =
      torch::arange(3, options.dtype(torch::kLong)).flip({0});
  const auto close = [](double a, double b) {
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a - b) <= 1e-6 + 1e-5 * std::max(std::abs(a), std::abs(b));
  };
  bool invariant = close(centered_loss, shifted_loss);
  for (const bool centered : {false, true}) {
    const double baseline = centered ? centered_loss : result.gaussian_loss;
    const double batch_loss =
        sigreg(gaussian.index_select(0, batch_order), directions, centered)
            .item<double>();
    const double channel_loss =
        sigreg(gaussian.index_select(2, channel_order), directions, centered)
            .item<double>();
    result.batch_permutation_error = std::max(result.batch_permutation_error,
                                              std::abs(baseline - batch_loss));
    result.channel_permutation_error = std::max(
        result.channel_permutation_error, std::abs(baseline - channel_loss));
    invariant = invariant && close(baseline, batch_loss) &&
                close(baseline, channel_loss);
  }
  // Exact collapse has positive loss but zero derivative: no escape guarantee.
  result.pass = invariant && std::isfinite(result.collapsed_loss) &&
                std::isfinite(result.static_raw_loss) &&
                std::isfinite(result.static_centered_loss) &&
                result.collapsed_loss > result.gaussian_loss &&
                result.collapsed_loss > centered_loss &&
                result.static_centered_loss > result.static_raw_loss &&
                std::isfinite(result.collapsed_gradient_norm) &&
                result.collapsed_gradient_norm == 0.0;
  return result;
}

} // namespace lwm0
