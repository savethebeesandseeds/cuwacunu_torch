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
  std::string accounting_numeraire_node_id{};
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
}

[[nodiscard]] inline action_t
make_baseline_action(const baseline_policy_config_t &config,
                     torch::Tensor target_weights,
                     portfolio::timestamp_ms_t decision_timestamp_ms = 0) {
  validate_baseline_policy_config(config);
  action_t action{};
  action.policy_id = config.policy_id;
  action.policy_kind = policy_kind_t::baseline;
  action.decision_timestamp_ms = decision_timestamp_ms;
  action.node_ids = config.node_ids;
  action.target_weights = target_weights.to(torch::kFloat64).contiguous();
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

class numeraire_only_policy_t final : public policy_adapter_iface_t {
public:
  explicit numeraire_only_policy_t(baseline_policy_config_t config)
      : config_(std::move(config)) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
    if (detail::blank(config_.accounting_numeraire_node_id)) {
      throw std::runtime_error(
          "[numeraire_only_policy] accounting_numeraire_node_id is required");
    }
    if (!std::count(config_.node_ids.begin(), config_.node_ids.end(),
                    config_.accounting_numeraire_node_id)) {
      throw std::runtime_error(
          "[numeraire_only_policy] accounting numeraire must be in node_ids");
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
    auto weights = torch::zeros({A}, torch::TensorOptions().dtype(torch::kFloat64));
    const auto it = std::find(config_.node_ids.begin(), config_.node_ids.end(),
                              config_.accounting_numeraire_node_id);
    weights.index_put_({static_cast<std::int64_t>(it - config_.node_ids.begin())},
                       1.0);
    return baseline_policy_detail::make_baseline_action(
        config_, weights, observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
};

class equal_weight_policy_t final : public policy_adapter_iface_t {
public:
  explicit equal_weight_policy_t(baseline_policy_config_t config)
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
    auto weights = torch::full({A}, 1.0 / static_cast<double>(A),
                               torch::TensorOptions().dtype(torch::kFloat64));
    return baseline_policy_detail::make_baseline_action(
        config_, weights, observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
};

class fixed_weight_policy_t final : public policy_adapter_iface_t {
public:
  fixed_weight_policy_t(baseline_policy_config_t config,
                        torch::Tensor target_weights)
      : config_(std::move(config)),
        target_weights_(target_weights.to(torch::kFloat64).contiguous()) {
    baseline_policy_detail::validate_baseline_policy_config(config_);
    (void)baseline_policy_detail::make_baseline_action(
        config_, target_weights_);
  }

  [[nodiscard]] std::string policy_id() const override {
    return config_.policy_id;
  }

  [[nodiscard]] policy_kind_t policy_kind() const override {
    return policy_kind_t::baseline;
  }

  [[nodiscard]] action_t act(const observation_t &observation) override {
    return baseline_policy_detail::make_baseline_action(
        config_, target_weights_, observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
  torch::Tensor target_weights_{};
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
    const double total = weights.sum().item<double>();
    if (total <= 0.0 || !std::isfinite(total)) {
      weights = torch::full({A}, 1.0 / static_cast<double>(A),
                            torch::TensorOptions().dtype(torch::kFloat64));
    } else if (std::abs(total - 1.0) > 1.0e-8) {
      weights = weights / total;
    }
    return baseline_policy_detail::make_baseline_action(
        config_, weights, observation.knowledge_timestamp_ms);
  }

private:
  baseline_policy_config_t config_{};
};

} // namespace cuwacunu::kikijyeba::environment
