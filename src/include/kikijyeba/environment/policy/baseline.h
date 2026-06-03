// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/control/interfaces.h"

namespace cuwacunu::kikijyeba::environment {

struct baseline_policy_config_t {
  std::string policy_id{};
  std::vector<std::string> node_ids{};
  std::string base_reserve_node_id{};
};

namespace baseline_policy_detail {

inline void
validate_baseline_policy_config(const baseline_policy_config_t &config) {
  if (detail::blank(config.policy_id)) {
    throw std::runtime_error("[baseline_policy] policy_id is required");
  }
  if (config.node_ids.empty()) {
    throw std::runtime_error("[baseline_policy] node_ids are required");
  }
  if (detail::blank(config.base_reserve_node_id)) {
    throw std::runtime_error(
        "[baseline_policy] base_reserve_node_id is required");
  }
}

[[nodiscard]] inline action_t
make_baseline_action(const baseline_policy_config_t &config,
                     torch::Tensor target_weights,
                     double target_base_reserve_weight,
                     portfolio::timestamp_ms_t decision_timestamp_ms = 0) {
  validate_baseline_policy_config(config);
  action_t action{};
  action.policy_id = config.policy_id;
  action.policy_kind = policy_kind_t::baseline;
  action.decision_timestamp_ms = decision_timestamp_ms;
  action.node_ids = config.node_ids;
  action.base_reserve_node_id = config.base_reserve_node_id;
  action.target_weights = target_weights.to(torch::kFloat64).contiguous();
  action.target_base_reserve_weight = target_base_reserve_weight;
  validate_action(action, config.node_ids);
  return action;
}

[[nodiscard]] inline torch::Tensor
current_weights_or_zero(const observation_t &observation, std::int64_t A) {
  if (!observation.portfolio_state.current_weights.defined()) {
    return torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  auto weights =
      observation.portfolio_state.current_weights.to(torch::kFloat64);
  TORCH_CHECK(weights.dim() == 1 && weights.size(0) == A,
              "[baseline_policy] current_weights must be [A]");
  TORCH_CHECK(torch::isfinite(weights).all().template item<bool>(),
              "[baseline_policy] current_weights contain non-finite values");
  return weights.clamp_min(0.0);
}

} // namespace baseline_policy_detail

class base_reserve_policy_t final : public policy_adapter_iface_t {
public:
  explicit base_reserve_policy_t(baseline_policy_config_t config)
      : config_(std::move(config)) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::baseline;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    const auto A = static_cast<std::int64_t>(config_.node_ids.size());
    return baseline_policy_detail::make_baseline_action(
        config_,
        torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64)), 1.0,
        observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
};

class equal_weight_policy_t final : public policy_adapter_iface_t {
public:
  explicit equal_weight_policy_t(baseline_policy_config_t config,
                                 double base_reserve_weight = 0.0)
      : config_(std::move(config)), base_reserve_weight_(base_reserve_weight) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
    if (!std::isfinite(base_reserve_weight_) || base_reserve_weight_ < 0.0 ||
        base_reserve_weight_ > 1.0) {
      throw std::runtime_error(
          "[equal_weight_policy] base_reserve_weight must be in [0,1]");
    }
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::baseline;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    const auto A = static_cast<std::int64_t>(config_.node_ids.size());
    const double risky_budget = 1.0 - base_reserve_weight_;
    auto weights = torch::full({A}, risky_budget / static_cast<double>(A),
                               torch::TensorOptions().dtype(torch::kFloat64));
    return baseline_policy_detail::make_baseline_action(
        config_, weights, base_reserve_weight_,
        observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
  double base_reserve_weight_{0.0};
};

class fixed_weight_policy_t final : public policy_adapter_iface_t {
public:
  fixed_weight_policy_t(baseline_policy_config_t config,
                        torch::Tensor target_weights,
                        double target_base_reserve_weight)
      : config_(std::move(config)),
        target_weights_(target_weights.to(torch::kFloat64).contiguous()),
        target_base_reserve_weight_(target_base_reserve_weight) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
    (void)baseline_policy_detail::make_baseline_action(
        config_, target_weights_, target_base_reserve_weight_);
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::baseline;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    return baseline_policy_detail::make_baseline_action(
        config_, target_weights_, target_base_reserve_weight_,
        observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
  torch::Tensor target_weights_{};
  double target_base_reserve_weight_{1.0};
};

class current_weight_policy_t final : public policy_adapter_iface_t {
public:
  explicit current_weight_policy_t(baseline_policy_config_t config)
      : config_(std::move(config)) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::baseline;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    const auto A = static_cast<std::int64_t>(config_.node_ids.size());
    auto weights =
        baseline_policy_detail::current_weights_or_zero(observation, A);
    double reserve = observation.portfolio_state.base_reserve_weight;
    if (!std::isfinite(reserve) || reserve < 0.0) {
      reserve = 1.0 - weights.sum().item<double>();
    }
    reserve = std::max(0.0, reserve);
    const double total = weights.sum().item<double>() + reserve;
    if (total <= 0.0 || !std::isfinite(total)) {
      weights =
          torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
      reserve = 1.0;
    } else if (std::abs(total - 1.0) > 1.0e-8) {
      weights = weights / total;
      reserve = reserve / total;
    }
    return baseline_policy_detail::make_baseline_action(
        config_, weights, reserve, observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
};

} // namespace cuwacunu::kikijyeba::environment
