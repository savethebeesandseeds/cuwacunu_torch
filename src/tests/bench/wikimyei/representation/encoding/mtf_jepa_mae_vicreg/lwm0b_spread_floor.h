#pragma once

#include <c10/core/impl/LocalDispatchKeySet.h>
#include <torch/torch.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <string>

namespace lwm0b {

struct SpreadFloorReference {
  torch::Tensor raw_variance;
  torch::Tensor temporal_variance;
  torch::Tensor directions;
};

inline void check_spread_floor_shape(const torch::Tensor &states) {
  TORCH_CHECK(states.dim() == 4 && states.size(0) > 1 && states.size(1) == 3 &&
                  states.size(2) == 3 && states.size(3) == 32,
              "LWM-0b spread floor requires [B>1,3,3,32]");
}

// Variance is over B alone, separately at each [W,C,M], with divisor B.
inline torch::Tensor projected_population_variance(torch::Tensor states,
                                                   torch::Tensor directions,
                                                   bool temporal_centered) {
  if (temporal_centered)
    states = states - states.mean(1, true);
  const auto projected = torch::matmul(states, directions);
  return (projected - projected.mean(0, true)).square().mean(0);
}

// The caller supplies the fit256 calibration states and the fixed projection
// bank. This function does not sample directions or fit any Gaussian shape.
inline SpreadFloorReference fit_spread_floor(torch::Tensor reference_states,
                                             torch::Tensor directions) {
  check_spread_floor_shape(reference_states);
  TORCH_CHECK(directions.dim() == 2 && directions.size(0) == 32 &&
                  directions.size(1) == 1024 &&
                  directions.device() == reference_states.device(),
              "LWM-0b directions must be [32,1024] on the states device");
  torch::NoGradGuard no_grad;
  const c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKeySet{
      c10::DispatchKey::AutocastCPU, c10::DispatchKey::AutocastCUDA});
  reference_states = reference_states.detach().to(torch::kFloat32);
  SpreadFloorReference result;
  result.directions = directions.detach().to(torch::kFloat32).clone();
  TORCH_CHECK(torch::isfinite(result.directions).all().item<bool>(),
              "LWM-0b directions must be finite");
  result.raw_variance =
      projected_population_variance(reference_states, result.directions, false)
          .detach()
          .clone();
  result.temporal_variance =
      projected_population_variance(reference_states, result.directions, true)
          .detach()
          .clone();
  for (const auto &variance : {result.raw_variance, result.temporal_variance}) {
    TORCH_CHECK(torch::isfinite(variance).all().item<bool>() &&
                    variance.gt(1e-10).all().item<bool>(),
                "LWM-0b reference variances must be finite and > 1e-10");
  }
  return result;
}

inline torch::Tensor spread_floor_branch(torch::Tensor states,
                                         const SpreadFloorReference &reference,
                                         bool temporal_centered) {
  check_spread_floor_shape(states);
  const auto &reference_variance =
      temporal_centered ? reference.temporal_variance : reference.raw_variance;
  TORCH_CHECK(
      reference.directions.dim() == 2 && reference.directions.size(0) == 32 &&
          reference.directions.size(1) == 1024 &&
          reference.directions.device() == states.device() &&
          reference_variance.dim() == 3 && reference_variance.size(0) == 3 &&
          reference_variance.size(1) == 3 &&
          reference_variance.size(2) == 1024 &&
          reference_variance.device() == states.device(),
      "LWM-0b reference must match the states shape and device");
  const c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKeySet{
      c10::DispatchKey::AutocastCPU, c10::DispatchKey::AutocastCUDA});
  // Casting states preserves their autograd graph. Both fitted variances and
  // directions remain frozen even if a caller changes requires_grad later.
  const auto variance = projected_population_variance(
      states.to(torch::kFloat32),
      reference.directions.detach().to(torch::kFloat32), temporal_centered);
  const auto ratio = variance / reference_variance.detach().to(torch::kFloat32);
  return torch::relu(0.5 - torch::sqrt(ratio + 1e-6)).square().mean() / 0.25;
}

inline torch::Tensor spread_floor(torch::Tensor states,
                                  const SpreadFloorReference &reference) {
  return 0.5 * (spread_floor_branch(states, reference, false) +
                spread_floor_branch(states, reference, true));
}

struct SpreadFloorFixtureMeasurement {
  std::string name;
  double raw_loss = 0.0;
  double temporal_loss = 0.0;
  double total_loss = 0.0;
  double raw_gradient_norm = 0.0;
  double temporal_gradient_norm = 0.0;
  double gradient_norm = 0.0;
  double raw_energy = 0.0;
  double temporal_energy = 0.0;
  double raw_energy_negative_gradient_derivative = 0.0;
  double temporal_energy_negative_gradient_derivative = 0.0;
  double step_norm = 0.0;
  double raw_loss_after_step = 0.0;
  double temporal_loss_after_step = 0.0;
  double total_loss_after_step = 0.0;
  double raw_energy_after_step = 0.0;
  double temporal_energy_after_step = 0.0;
  double batch_reversal_error = 0.0;
  bool finite = false;
};

struct SpreadFloorFixtures {
  std::string root;
  bool pass = false;
  SpreadFloorFixtureMeasurement healthy;
  SpreadFloorFixtureMeasurement low_scale;
  SpreadFloorFixtureMeasurement static_states;
  SpreadFloorFixtureMeasurement near_static;
  SpreadFloorFixtureMeasurement rank_one;
  SpreadFloorFixtureMeasurement near_rank_one;
  SpreadFloorFixtureMeasurement collapsed;
  double rank_one_energy_match_error = 0.0;
  double near_rank_one_energy_match_error = 0.0;
  double batch_reversal_error = 0.0;
};

inline torch::Tensor spread_floor_energy_residual(torch::Tensor states,
                                                  bool temporal_centered) {
  if (temporal_centered)
    states = states - states.mean(1, true);
  return states - states.mean(0, true);
}

// Mean over B,W,C of the total feature energy, after batch centering.
inline double spread_floor_energy(torch::Tensor states,
                                  bool temporal_centered) {
  return spread_floor_energy_residual(states, temporal_centered)
      .square()
      .sum(-1)
      .mean()
      .item<double>();
}

inline SpreadFloorFixtureMeasurement
measure_spread_floor_fixture(const std::string &name, torch::Tensor fixture,
                             const SpreadFloorReference &reference) {
  torch::AutoGradMode enable_grad(true);
  const c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKeySet{
      c10::DispatchKey::AutocastCPU, c10::DispatchKey::AutocastCUDA});
  auto states = fixture.detach().to(torch::kFloat32).clone();
  states.set_requires_grad(true);
  const auto raw = spread_floor_branch(states, reference, false);
  const auto temporal = spread_floor_branch(states, reference, true);
  const auto total = 0.5 * (raw + temporal);
  const auto raw_gradient =
      torch::autograd::grad({raw}, {states}, {}, true).at(0);
  const auto temporal_gradient =
      torch::autograd::grad({temporal}, {states}, {}, true).at(0);
  const auto gradient = torch::autograd::grad({total}, {states}).at(0);
  torch::NoGradGuard no_grad;
  SpreadFloorFixtureMeasurement result;
  result.name = name;
  result.raw_loss = raw.item<double>();
  result.temporal_loss = temporal.item<double>();
  result.total_loss = total.item<double>();
  result.raw_gradient_norm = raw_gradient.norm().item<double>();
  result.temporal_gradient_norm = temporal_gradient.norm().item<double>();
  result.gradient_norm = gradient.norm().item<double>();
  result.raw_energy = spread_floor_energy(states, false);
  result.temporal_energy = spread_floor_energy(states, true);
  const double energy_divisor =
      states.size(0) * states.size(1) * states.size(2);
  result.raw_energy_negative_gradient_derivative =
      -2.0 *
      (spread_floor_energy_residual(states, false) * gradient)
          .sum()
          .item<double>() /
      energy_divisor;
  result.temporal_energy_negative_gradient_derivative =
      -2.0 *
      (spread_floor_energy_residual(states, true) * gradient)
          .sum()
          .item<double>() /
      energy_divisor;
  auto stepped = states.detach();
  if (std::isfinite(result.gradient_norm) && result.gradient_norm > 0.0) {
    result.step_norm = 0.001 * states.norm().item<double>();
    stepped = stepped - gradient * (result.step_norm / result.gradient_norm);
  }
  result.raw_loss_after_step =
      spread_floor_branch(stepped, reference, false).item<double>();
  result.temporal_loss_after_step =
      spread_floor_branch(stepped, reference, true).item<double>();
  result.total_loss_after_step =
      0.5 * (result.raw_loss_after_step + result.temporal_loss_after_step);
  result.raw_energy_after_step = spread_floor_energy(stepped, false);
  result.temporal_energy_after_step = spread_floor_energy(stepped, true);
  const auto reversed = states.detach().flip({0});
  const double reversed_raw =
      spread_floor_branch(reversed, reference, false).item<double>();
  const double reversed_temporal =
      spread_floor_branch(reversed, reference, true).item<double>();
  result.batch_reversal_error = std::max(
      {std::abs(result.raw_loss - reversed_raw),
       std::abs(result.temporal_loss - reversed_temporal),
       std::abs(result.total_loss - 0.5 * (reversed_raw + reversed_temporal))});
  const std::array<double, 18> values{
      result.raw_loss,
      result.temporal_loss,
      result.total_loss,
      result.raw_gradient_norm,
      result.temporal_gradient_norm,
      result.gradient_norm,
      result.raw_energy,
      result.temporal_energy,
      result.raw_energy_negative_gradient_derivative,
      result.temporal_energy_negative_gradient_derivative,
      result.step_norm,
      result.raw_loss_after_step,
      result.temporal_loss_after_step,
      result.total_loss_after_step,
      result.raw_energy_after_step,
      result.temporal_energy_after_step,
      reversed_raw,
      reversed_temporal};
  result.finite = std::all_of(values.begin(), values.end(), [](double value) {
    return std::isfinite(value);
  });
  return result;
}

inline void log_spread_floor_fixture(std::ostream &log, const std::string &root,
                                     const SpreadFloorFixtureMeasurement &m) {
  const std::string key = root + ".fixture." + m.name + '.';
  const std::array<std::pair<const char *, double>, 17> values{
      {{"raw_loss", m.raw_loss},
       {"temporal_loss", m.temporal_loss},
       {"total_loss", m.total_loss},
       {"raw_gradient_norm", m.raw_gradient_norm},
       {"temporal_gradient_norm", m.temporal_gradient_norm},
       {"gradient_norm", m.gradient_norm},
       {"raw_energy", m.raw_energy},
       {"temporal_energy", m.temporal_energy},
       {"raw_energy_negative_gradient_derivative",
        m.raw_energy_negative_gradient_derivative},
       {"temporal_energy_negative_gradient_derivative",
        m.temporal_energy_negative_gradient_derivative},
       {"step_norm", m.step_norm},
       {"raw_loss_after_step", m.raw_loss_after_step},
       {"temporal_loss_after_step", m.temporal_loss_after_step},
       {"total_loss_after_step", m.total_loss_after_step},
       {"raw_energy_after_step", m.raw_energy_after_step},
       {"temporal_energy_after_step", m.temporal_energy_after_step},
       {"batch_reversal_error", m.batch_reversal_error}}};
  for (const auto &[name, value] : values)
    log << key << name << '=' << value << '\n';
  log << key << "finite=" << m.finite << '\n';
}

// No RNG and no calibration retuning: healthy is exactly the first 64 supplied
// reference states. Positive rank-one penalty is not a rank-restoration claim.
inline SpreadFloorFixtures
check_spread_floor_fixtures(torch::Tensor reference_states,
                            const SpreadFloorReference &reference,
                            const std::string &root, std::ostream &log) {
  check_spread_floor_shape(reference_states);
  TORCH_CHECK(reference_states.size(0) >= 64,
              "LWM-0b fixtures require at least 64 reference states");
  torch::NoGradGuard no_grad;
  const c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKeySet{
      c10::DispatchKey::AutocastCPU, c10::DispatchKey::AutocastCUDA});
  const auto healthy =
      reference_states.narrow(0, 0, 64).detach().to(torch::kFloat32).clone();
  const auto temporal_mean = healthy.mean(1, true);
  const auto batch_mean = healthy.mean(0, true);
  const auto centered = healthy - batch_mean;
  const auto target_energy = centered.square().mean(0, true).sum(-1, true);
  const auto scalar = centered.narrow(3, 0, 1);
  const auto scalar_energy = scalar.square().mean(0, true);
  TORCH_CHECK(torch::isfinite(scalar_energy).all().item<bool>() &&
                  scalar_energy.gt(1e-10).all().item<bool>(),
              "LWM-0b fixed feature-0 rank fixture has degenerate energy");
  const auto feature_direction =
      torch::ones({1, 1, 1, 32}, healthy.options().requires_grad(false)) /
      std::sqrt(32.0);
  const auto rank_centered =
      scalar * (target_energy / scalar_energy).sqrt() * feature_direction;
  const auto rank_one = batch_mean + rank_centered;
  // Fixed near-rank-one fixture: add 0.01 times the centered reference, then
  // restore the original per-W/C total centered feature energy.
  auto near_rank_centered = rank_centered + 0.01 * centered;
  near_rank_centered = near_rank_centered - near_rank_centered.mean(0, true);
  near_rank_centered =
      near_rank_centered *
      (target_energy / near_rank_centered.square().mean(0, true).sum(-1, true))
          .sqrt();
  const auto near_rank_one = batch_mean + near_rank_centered;
  SpreadFloorFixtures result;
  result.root = root;
  result.healthy = measure_spread_floor_fixture("healthy", healthy, reference);
  result.low_scale =
      measure_spread_floor_fixture("low_scale", 0.1 * healthy, reference);
  result.static_states = measure_spread_floor_fixture(
      "static", temporal_mean.expand_as(healthy), reference);
  result.near_static = measure_spread_floor_fixture(
      "near_static", temporal_mean + 0.1 * (healthy - temporal_mean),
      reference);
  result.rank_one =
      measure_spread_floor_fixture("rank_one", rank_one, reference);
  result.near_rank_one =
      measure_spread_floor_fixture("near_rank_one", near_rank_one, reference);
  result.collapsed = measure_spread_floor_fixture(
      "collapsed", torch::zeros_like(healthy), reference);
  const auto energy_match_error = [&](const torch::Tensor &states) {
    const auto energy =
        (states - states.mean(0, true)).square().mean(0, true).sum(-1, true);
    return ((energy - target_energy).abs() / target_energy)
        .max()
        .item<double>();
  };
  result.rank_one_energy_match_error = energy_match_error(rank_one);
  result.near_rank_one_energy_match_error = energy_match_error(near_rank_one);
  const auto old_precision = log.precision();
  log << std::setprecision(17);
  bool all_finite = true;
  for (const auto *measurement :
       {&result.healthy, &result.low_scale, &result.static_states,
        &result.near_static, &result.rank_one, &result.near_rank_one,
        &result.collapsed}) {
    log_spread_floor_fixture(log, root, *measurement);
    all_finite = all_finite && measurement->finite;
    result.batch_reversal_error = std::max(result.batch_reversal_error,
                                           measurement->batch_reversal_error);
  }
  // Exact zero collapse has a finite positive penalty and exactly zero
  // derivative. This objective therefore gives no escape guarantee there.
  result.pass =
      all_finite && result.healthy.total_loss <= 1e-8 &&
      result.healthy.gradient_norm <= 1e-8 &&
      result.low_scale.total_loss > 0.1 &&
      result.low_scale.gradient_norm > 0.0 &&
      result.low_scale.raw_energy_negative_gradient_derivative > 0.0 &&
      result.low_scale.total_loss_after_step < result.low_scale.total_loss &&
      result.static_states.temporal_loss > 0.9 &&
      result.near_static.temporal_loss > 0.1 &&
      result.near_static.temporal_energy_negative_gradient_derivative > 0.0 &&
      result.near_static.total_loss_after_step <
          result.near_static.total_loss &&
      result.rank_one.total_loss > 1e-5 &&
      result.near_rank_one.total_loss > 1e-5 &&
      result.collapsed.total_loss > 0.0 &&
      result.collapsed.raw_gradient_norm == 0.0 &&
      result.collapsed.temporal_gradient_norm == 0.0 &&
      result.collapsed.gradient_norm == 0.0 &&
      std::isfinite(result.rank_one_energy_match_error) &&
      std::isfinite(result.near_rank_one_energy_match_error) &&
      result.rank_one_energy_match_error <= 1e-5 &&
      result.near_rank_one_energy_match_error <= 1e-5 &&
      result.batch_reversal_error <= 1e-5;
  log << root << ".fixture_pass=" << result.pass << '\n'
      << root
      << ".rank_one_energy_match_error=" << result.rank_one_energy_match_error
      << '\n'
      << root << ".near_rank_one_energy_match_error="
      << result.near_rank_one_energy_match_error << '\n'
      << root << ".batch_reversal_error=" << result.batch_reversal_error << '\n'
      << root << ".exact_collapse_escape_guarantee=false\n"
      << root << ".rank_restoration_claim=false\n";
  log.precision(old_precision);
  return result;
}

} // namespace lwm0b
