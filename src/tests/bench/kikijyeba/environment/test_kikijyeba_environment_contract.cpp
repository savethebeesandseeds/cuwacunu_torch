// SPDX-License-Identifier: MIT

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/environment.h"

namespace {

namespace env = cuwacunu::kikijyeba::environment;
namespace replay = cuwacunu::kikijyeba::environment::replay;
namespace protocol = cuwacunu::kikijyeba::protocol;
namespace runtime = cuwacunu::kikijyeba::runtime;
namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace dataloader = cuwacunu::ujcamei::source::retrieval::dataloader;
namespace kline = cuwacunu::ujcamei::source::registry::types;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdn_stream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace vicreg_stream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;

void check(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to read " + path.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

void close(double actual, double expected, double tol, const char *message) {
  if (std::abs(actual - expected) > tol) {
    throw std::runtime_error(std::string(message) +
                             " actual=" + std::to_string(actual) +
                             " expected=" + std::to_string(expected));
  }
}

env::episode_spec_t make_episode_spec() {
  env::episode_spec_t spec{};
  spec.episode_id = "episode_0";
  spec.protocol_id = "protocol_fixture";
  spec.runtime_run_id = "runtime_run_0";
  spec.environment_run_id = "environment_run_0";
  spec.episode_index = 0;
  spec.requested_range.anchor_index_begin = 10;
  spec.requested_range.anchor_index_end = 12;
  spec.accepted_range.anchor_index_begin = 10;
  spec.accepted_range.anchor_index_end = 12;
  spec.accepted_range.anchor_keys = {"anchor_10", "anchor_11"};
  spec.accepted_range.cursor = {
      .wave_id = "wave_fixture",
      .wave_mode = "run",
      .source_cursor_kind = "graph_anchor",
      .source_cursor_scope = "episode",
      .source_range_policy = "anchor_index",
      .source_order_policy = "sequential",
      .anchor_index_begin = 10,
      .anchor_index_end = 12,
      .batch_cursor_token = "cursor_fixture_10_12",
  };
  spec.graph_order_fingerprint = "graph_fixture";
  spec.graph_node_ids = {"BTC", "ETH", "USDT"};
  spec.risky_node_ids = {"BTC", "ETH"};
  spec.base_policy = {.accounting_numeraire_id = "USDT",
                      .settlement_asset_id = "USDT",
                      .reserve_asset_id = "USDT",
                      .projection_reference_node_id = "USDT"};
  spec.initial_equity_base = 1000.0;
  spec.constraints.min_base_reserve_weight = 0.20;
  spec.world_mode = env::world_mode_t::historical_replay;
  spec.requested_wave.wave_id = "wave_fixture";
  spec.requested_wave.mode_text = "run";
  spec.requested_wave.source_cursor_kind = "graph_anchor";
  spec.requested_wave.source_cursor_scope = "episode";
  spec.requested_wave.source_range_policy = "anchor_index";
  spec.requested_wave.source_order_policy = "sequential";
  spec.requested_wave.anchor_index_begin = 10;
  spec.requested_wave.anchor_index_end = 12;
  return spec;
}

env::action_t
make_action(env::portfolio::timestamp_ms_t decision_timestamp_ms = 0) {
  env::action_t action{};
  action.policy_id = "equal_weight_fixture";
  action.policy_kind = env::policy_kind_t::baseline;
  action.decision_timestamp_ms = decision_timestamp_ms;
  action.node_ids = {"BTC", "ETH"};
  action.base_reserve_node_id = "USDT";
  action.target_weights = torch::tensor(
      {0.25, 0.25}, torch::TensorOptions().dtype(torch::kFloat64));
  action.target_base_reserve_weight = 0.50;
  return action;
}

env::portfolio::PortfolioState make_portfolio_state(double equity,
                                                    double drawdown) {
  env::portfolio::PortfolioState state{};
  state.timestamp_ms = 1000;
  state.accounting_node_id = "USDT";
  state.reserve_node_id = "USDT";
  state.current_weights = torch::tensor(
      {0.20, 0.20}, torch::TensorOptions().dtype(torch::kFloat64));
  state.current_units =
      torch::tensor({2.0, 4.0}, torch::TensorOptions().dtype(torch::kFloat64));
  state.base_reserve_weight = 0.60;
  state.equity_value_base = equity;
  state.drawdown = drawdown;
  return state;
}

void set_valid_projection(env::transition_t &transition) {
  transition.info.projection_validation.available = true;
  transition.info.projection_validation.valid = true;
  transition.info.projection_validation.signed_bias = 0.0;
  transition.info.projection_validation.mae = 0.0;
  transition.info.projection_validation.rmse = 0.0;
  transition.info.projection_validation.directional_accuracy = 1.0;
  transition.info.projection_validation.correlation = 1.0;
  transition.info.projection_validation.interval_coverage = 1.0;
}

env::execution::spot_edge_market_state_t make_edge_market_state() {
  graph::market_graph_t graph{};
  graph.node_ids = {"BTC", "ETH", "USDT"};
  graph.edge_ids = {"BTCUSDT", "ETHUSDT"};
  graph.base_index = {0, 1};
  graph.quote_index = {2, 2};
  graph.validate();

  env::execution::spot_edge_market_state_t market{};
  market.graph = std::move(graph);
  market.edge_mid_price = torch::tensor(
      {100.0, 50.0}, torch::TensorOptions().dtype(torch::kFloat64));
  market.edge_fee_rate = torch::full({2}, 0.001, torch::kFloat64);
  market.edge_spread_rate = torch::full({2}, 0.0005, torch::kFloat64);
  market.edge_slippage_rate = torch::full({2}, 0.0002, torch::kFloat64);
  market.edge_tradable_mask = torch::ones({2}, torch::kBool);
  env::execution::validate_spot_edge_market_state(market);
  return market;
}

env::observation_t make_observation(std::int64_t anchor_index, double equity) {
  env::observation_t observation{};
  observation.anchor_key = "anchor_" + std::to_string(anchor_index);
  observation.timestamp_ms = 1000 + anchor_index;
  observation.observation_anchor_index = anchor_index;
  observation.next_realization_anchor_index = anchor_index + 1;
  observation.knowledge_timestamp_ms = 1000 + anchor_index;
  observation.realization_available_after_timestamp_ms = 1001 + anchor_index;
  observation.portfolio_state = make_portfolio_state(equity, /*drawdown=*/0.0);
  observation.market_state.timestamp_ms = observation.timestamp_ms;
  observation.market_state.tradable_mask = torch::ones({2}, torch::kBool);
  observation.market_state.executable_mid = torch::tensor(
      {100.0, 50.0}, torch::TensorOptions().dtype(torch::kFloat64));
  observation.edge_market_state = make_edge_market_state();
  return observation;
}

class fixture_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    step_index_ = 0;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &action) override {
    env::validate_episode_action(action, spec_);

    const double equity_before =
        step_index_ == 0 ? spec_.initial_equity_base : 1010.0;
    const double equity_after = step_index_ == 0 ? 1010.0 : 1005.0;

    env::transition_t transition{};
    transition.done = step_index_ >= 1;
    transition.info.anchor_index =
        spec_.accepted_range.anchor_index_begin + step_index_;
    transition.info.anchor_key =
        "anchor_" + std::to_string(transition.info.anchor_index);
    transition.info.portfolio_before =
        make_portfolio_state(equity_before, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(equity_after, step_index_ == 0 ? 0.0 : 0.01);
    transition.info.turnover = 0.10;
    transition.info.transaction_cost_base = 0.50;
    transition.info.realized_log_growth =
        std::log(equity_after / equity_before);
    transition.info.realized_arithmetic_return =
        (equity_after / equity_before) - 1.0;
    transition.info.projection_validation.available = true;
    transition.info.projection_validation.valid = true;
    transition.info.projection_validation.mae = step_index_ == 0 ? 0.01 : 0.02;
    transition.info.projection_validation.rmse =
        transition.info.projection_validation.mae;
    transition.info.projection_validation.signed_bias =
        step_index_ == 0 ? 0.001 : -0.001;
    transition.info.projection_validation.directional_accuracy = 1.0;
    transition.info.projection_validation.interval_coverage = 1.0;
    if (step_index_ == 0) {
      transition.info.warnings.push_back("fixture.step_warning");
    }
    transition.reward = env::compute_reward(
        equity_before, equity_after, transition.info.portfolio_after.drawdown,
        transition.info.transaction_cost_base, transition.info.turnover, false);
    transition.next_observation = make_observation(
        spec_.accepted_range.anchor_index_begin + step_index_ + 1,
        equity_after);

    ++step_index_;
    return transition;
  }

private:
  env::episode_spec_t spec_{};
  std::int64_t step_index_{0};
};

class fixture_policy_t final : public env::policy_adapter_iface_t {
public:
  std::string policy_id() const override { return "equal_weight_fixture"; }

  env::policy_kind_t policy_kind() const override {
    return env::policy_kind_t::baseline;
  }

  env::action_t act(const env::observation_t &observation) override {
    return make_action(observation.knowledge_timestamp_ms);
  }
};

class drifting_action_policy_t final : public env::policy_adapter_iface_t {
public:
  std::string policy_id() const override { return "equal_weight_fixture"; }

  env::policy_kind_t policy_kind() const override {
    return env::policy_kind_t::baseline;
  }

  env::action_t act(const env::observation_t &observation) override {
    auto action = make_action(observation.knowledge_timestamp_ms);
    action.policy_id = "drifting_action_policy";
    return action;
  }
};

class bad_reset_observation_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    auto observation = make_observation(spec.accepted_range.anchor_index_begin,
                                        spec.initial_equity_base);
    observation.anchor_key = "wrong_anchor";
    return observation;
  }

  env::transition_t step(const env::action_t &) override {
    return env::transition_t{};
  }
};

class bad_next_realization_observation_world_t final
    : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    auto observation = make_observation(spec.accepted_range.anchor_index_begin,
                                        spec.initial_equity_base);
    observation.next_realization_anchor_index =
        observation.observation_anchor_index + 2;
    return observation;
  }

  env::transition_t step(const env::action_t &) override {
    return env::transition_t{};
  }
};

class bad_observation_timestamp_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    auto observation = make_observation(spec.accepted_range.anchor_index_begin,
                                        spec.initial_equity_base);
    observation.timestamp_ms = observation.knowledge_timestamp_ms + 1;
    return observation;
  }

  env::transition_t step(const env::action_t &) override {
    return env::transition_t{};
  }
};

class bad_observation_market_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    auto observation = make_observation(spec.accepted_range.anchor_index_begin,
                                        spec.initial_equity_base);
    observation.market_state.executable_mid = torch::tensor(
        {-1.0, 50.0}, torch::TensorOptions().dtype(torch::kFloat64));
    return observation;
  }

  env::transition_t step(const env::action_t &) override {
    return env::transition_t{};
  }
};

class bad_observation_edge_market_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    auto observation = make_observation(spec.accepted_range.anchor_index_begin,
                                        spec.initial_equity_base);
    graph::market_graph_t missing_reserve_graph{};
    missing_reserve_graph.node_ids = {"BTC", "ETH", "USD"};
    missing_reserve_graph.edge_ids = {"BTCUSD", "ETHUSD"};
    missing_reserve_graph.base_index = {0, 1};
    missing_reserve_graph.quote_index = {2, 2};
    missing_reserve_graph.validate();
    observation.edge_market_state.graph = std::move(missing_reserve_graph);
    observation.edge_market_state.edge_mid_price = torch::tensor(
        {100.0, 50.0}, torch::TensorOptions().dtype(torch::kFloat64));
    return observation;
  }

  env::transition_t step(const env::action_t &) override {
    return env::transition_t{};
  }
};

class bad_transition_rebalance_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    transition.info.projection_validation.available = true;
    transition.info.projection_validation.valid = true;
    transition.info.rebalance_plan_source = "fixture.bad_transition_plan";
    transition.info.rebalance_plan.timestamp_ms =
        1000 + spec_.accepted_range.anchor_index_begin;
    transition.info.rebalance_plan.valid = true;
    transition.info.rebalance_plan.node_ids = {"BTC", "ETH"};
    transition.info.rebalance_plan.base_reserve_node_id = "USDT";
    transition.info.rebalance_plan.requested_turnover_weight = 0.10;
    transition.info.rebalance_plan.routed_turnover_weight = 0.20;
    transition.info.rebalance_plan.requested_notional_base = 100.0;
    transition.info.rebalance_plan.routed_notional_base = 200.0;
    transition.info.rebalance_plan.estimated_transaction_cost_base = 0.0;
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class bad_transition_fill_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    transition.info.projection_validation.available = true;
    transition.info.projection_validation.valid = true;
    transition.info.fills.push_back({
        .timestamp_ms = 1010,
        .node_id = "BTC",
        .edge_id = "BTCUSDT",
        .side = env::accounting::fill_side_t::buy_asset,
        .quantity_asset = 1.0,
        .gross_notional_base = 100.0,
        .fee_base = -1.0,
    });
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class bad_transition_fill_timestamp_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    set_valid_projection(transition);
    transition.info.fills.push_back({
        .timestamp_ms = 2000 + spec_.accepted_range.anchor_index_begin,
        .node_id = "BTC",
        .edge_id = "BTCUSDT",
        .side = env::accounting::fill_side_t::buy_asset,
        .quantity_asset = 1.0,
        .gross_notional_base = 100.0,
        .fee_base = 0.0,
    });
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class bad_reward_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    set_valid_projection(transition);
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    transition.reward.total = std::numeric_limits<double>::quiet_NaN();
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class mismatched_reward_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    const double equity_before = spec_.initial_equity_base;
    const double equity_after = spec_.initial_equity_base * 1.01;
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(equity_before, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(equity_after, /*drawdown=*/0.0);
    transition.info.realized_log_growth =
        std::log(equity_after / equity_before);
    transition.info.realized_arithmetic_return =
        (equity_after / equity_before) - 1.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    set_valid_projection(transition);
    transition.reward = env::compute_reward(
        equity_before, equity_before,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class mismatched_growth_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    const double equity_before = spec_.initial_equity_base;
    const double equity_after = spec_.initial_equity_base * 1.01;
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(equity_before, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(equity_after, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    set_valid_projection(transition);
    transition.reward = env::compute_reward(
        equity_before, equity_after,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class missing_projection_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class early_done_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    set_valid_projection(transition);
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class bad_projection_metric_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base, /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    transition.info.projection_validation.available = true;
    transition.info.projection_validation.valid = true;
    transition.info.projection_validation.mae = 0.02;
    transition.info.projection_validation.rmse = 0.01;
    transition.info.projection_validation.signed_bias = 0.0;
    transition.info.projection_validation.directional_accuracy = 1.0;
    transition.info.projection_validation.correlation = 0.0;
    transition.info.projection_validation.interval_coverage = 1.0;
    transition.reward = env::compute_reward(
        spec_.initial_equity_base, spec_.initial_equity_base,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

class mismatched_equity_world_t final : public env::world_iface_t {
public:
  env::observation_t reset(const env::episode_spec_t &spec) override {
    spec_ = spec;
    return make_observation(spec.accepted_range.anchor_index_begin,
                            spec.initial_equity_base);
  }

  env::transition_t step(const env::action_t &) override {
    env::transition_t transition{};
    transition.done = true;
    transition.info.anchor_key =
        "anchor_" + std::to_string(spec_.accepted_range.anchor_index_begin);
    transition.info.anchor_index = spec_.accepted_range.anchor_index_begin;
    transition.info.portfolio_before =
        make_portfolio_state(spec_.initial_equity_base + 1.0,
                             /*drawdown=*/0.0);
    transition.info.portfolio_after =
        make_portfolio_state(spec_.initial_equity_base + 1.0,
                             /*drawdown=*/0.0);
    transition.info.realized_log_growth = 0.0;
    transition.info.realized_arithmetic_return = 0.0;
    transition.info.transaction_cost_base = 0.0;
    transition.info.turnover = 0.0;
    transition.reward = env::compute_reward(
        spec_.initial_equity_base + 1.0, spec_.initial_equity_base + 1.0,
        /*drawdown=*/0.0, /*transaction_cost_base=*/0.0, /*turnover=*/0.0,
        /*invalid_action=*/false);
    return transition;
  }

private:
  env::episode_spec_t spec_{};
};

void test_environment_contract() {
  const auto replay_environment_bnf = read_text(
      "/cuwacunu/src/config/grammar/kikijyeba.environment.replay.dsl.bnf");
  check(replay_environment_bnf.find("REPLAY_ENVIRONMENT") !=
                std::string::npos &&
            replay_environment_bnf.find("REQUIRE_NO_FUTURE_LEAKAGE") !=
                std::string::npos &&
            replay_environment_bnf.find("SOURCE_ORDER_POLICY") !=
                std::string::npos &&
            replay_environment_bnf.find("ACTION_TIME_POLICY") !=
                std::string::npos &&
            replay_environment_bnf.find("ACTION_SCHEMA_ID") !=
                std::string::npos &&
            replay_environment_bnf.find("REALIZED_RETURN_TRUTH") !=
                std::string::npos &&
            replay_environment_bnf.find("ACTION_POLICY_IDENTITY") !=
                std::string::npos &&
            replay_environment_bnf.find("STEP_ARTIFACT_IDENTITY") !=
                std::string::npos &&
            replay_environment_bnf.find("EXPERIMENT_REPORT_COUNT_POLICY") !=
                std::string::npos &&
            replay_environment_bnf.find("LIVE_CAPITAL_ALLOWED") !=
                std::string::npos,
        "environment replay BNF preserves safety contract");
  const auto global_config = read_text("/cuwacunu/src/config/.config");
  check(global_config.find("kikijyeba_environment_replay_dsl_bnf_path") !=
                std::string::npos &&
            global_config.find("kikijyeba_environment_replay_dsl_path") !=
                std::string::npos,
        "environment replay DSL paths are bound in global config");
  const auto replay_environment_dsl =
      read_text("/cuwacunu/src/config/kikijyeba.environment.replay.dsl");
  const auto replay_spec =
      env::decode_replay_environment_spec_from_dsl(replay_environment_dsl);
  check(replay_spec.version_token == "kikijyeba.environment.replay.v1",
        "environment replay DSL decodes version");
  check(replay_spec.range_source == "ujcamei_component_stream_cursor" &&
            replay_spec.observation_time_law == "time_t_only" &&
            replay_spec.source_order_policy == "sequential" &&
            replay_spec.realization_key_policy == "shared_key_per_frame" &&
            replay_spec.action_kind ==
                "target_node_weights_with_base_reserve" &&
            replay_spec.action_schema_id ==
                "kikijyeba.environment.action.target_weights.v1" &&
            replay_spec.action_time_policy ==
                "decision_timestamp_after_knowledge_before_realization" &&
            replay_spec.graph_node_universe_policy ==
                "episode_spec_graph_node_ids" &&
            replay_spec.realized_return_truth ==
                "direct_edge_realized_return_truth_v1",
        "environment replay DSL preserves cursor/time/action semantics");
  check(replay_spec.experiment_task_identity == "bundle_policy_task_indices",
        "environment replay DSL preserves experiment task identity semantics");
  check(replay_spec.action_policy_identity ==
                "policy_adapter_must_match_action" &&
            replay_spec.step_artifact_identity == "episode_run_policy_cursor",
        "environment replay DSL preserves policy and step artifact identity "
        "semantics");
  check(replay_spec.experiment_report_count_policy == "counts_match_evidence",
        "environment replay DSL preserves report-count evidence semantics");
  check(replay_spec.default_max_parallel_jobs > 0,
        "environment replay DSL declares a positive default parallelism bound");
  bool rejected_live_capital_replay = false;
  auto unsafe_replay_dsl = replay_environment_dsl;
  const auto live_capital_pos =
      unsafe_replay_dsl.find("LIVE_CAPITAL_ALLOWED = false");
  check(live_capital_pos != std::string::npos,
        "environment replay DSL contains live-capital guard");
  unsafe_replay_dsl.replace(live_capital_pos,
                            std::string("LIVE_CAPITAL_ALLOWED = false").size(),
                            "LIVE_CAPITAL_ALLOWED = true");
  try {
    (void)env::decode_replay_environment_spec_from_dsl(unsafe_replay_dsl);
  } catch (const std::exception &) {
    rejected_live_capital_replay = true;
  }
  check(rejected_live_capital_replay,
        "environment replay DSL rejects live-capital authority");

  bool rejected_unbounded_parallelism = false;
  auto unbounded_parallelism_dsl = replay_environment_dsl;
  const auto parallelism_pos =
      unbounded_parallelism_dsl.find("DEFAULT_MAX_PARALLEL_JOBS = 1");
  check(parallelism_pos != std::string::npos,
        "replay DSL fixture contains DEFAULT_MAX_PARALLEL_JOBS position");
  unbounded_parallelism_dsl.replace(
      parallelism_pos, std::string("DEFAULT_MAX_PARALLEL_JOBS = 1").size(),
      "DEFAULT_MAX_PARALLEL_JOBS = 0");
  try {
    (void)env::decode_replay_environment_spec_from_dsl(
        unbounded_parallelism_dsl);
  } catch (const std::exception &) {
    rejected_unbounded_parallelism = true;
  }
  check(rejected_unbounded_parallelism,
        "environment replay DSL rejects an unbounded default parallelism");

  bool rejected_bad_action_schema_dsl = false;
  auto bad_action_schema_dsl = replay_environment_dsl;
  const auto action_schema_pos = bad_action_schema_dsl.find(
      "ACTION_SCHEMA_ID = kikijyeba.environment.action.target_weights.v1");
  check(action_schema_pos != std::string::npos,
        "replay DSL fixture contains ACTION_SCHEMA_ID position");
  bad_action_schema_dsl.replace(
      action_schema_pos,
      std::string("ACTION_SCHEMA_ID = "
                  "kikijyeba.environment.action.target_weights.v1")
          .size(),
      "ACTION_SCHEMA_ID = fixture.unknown_action.v1");
  try {
    (void)env::decode_replay_environment_spec_from_dsl(bad_action_schema_dsl);
  } catch (const std::exception &) {
    rejected_bad_action_schema_dsl = true;
  }
  check(rejected_bad_action_schema_dsl,
        "environment replay DSL rejects an unexpected action schema");

  bool rejected_bad_realized_truth_dsl = false;
  auto bad_realized_truth_dsl = replay_environment_dsl;
  const auto realized_truth_pos = bad_realized_truth_dsl.find(
      "REALIZED_RETURN_TRUTH = direct_edge_realized_return_truth_v1");
  check(realized_truth_pos != std::string::npos,
        "replay DSL fixture contains REALIZED_RETURN_TRUTH position");
  bad_realized_truth_dsl.replace(
      realized_truth_pos,
      std::string("REALIZED_RETURN_TRUTH = "
                  "direct_edge_realized_return_truth_v1")
          .size(),
      "REALIZED_RETURN_TRUTH = fixture_unknown_truth");
  try {
    (void)env::decode_replay_environment_spec_from_dsl(bad_realized_truth_dsl);
  } catch (const std::exception &) {
    rejected_bad_realized_truth_dsl = true;
  }
  check(rejected_bad_realized_truth_dsl,
        "environment replay DSL rejects unexpected realized-return truth ids");

  env::runtime_job_replay_driver_options_t replay_driver_contract_options{};
  env::replay_driver_detail::validate_driver_replay_environment_contract(
      replay_spec, replay_driver_contract_options);
  bool rejected_disabled_projection_validation = false;
  replay_driver_contract_options.world_options.require_projection_validation =
      false;
  try {
    env::replay_driver_detail::validate_driver_replay_environment_contract(
        replay_spec, replay_driver_contract_options);
  } catch (const std::exception &) {
    rejected_disabled_projection_validation = true;
  }
  check(rejected_disabled_projection_validation,
        "runtime replay driver honors projection-validation contract");
  replay_driver_contract_options.world_options.require_projection_validation =
      true;
  replay_driver_contract_options.artifact_options
      .require_artifact_time_not_after_observation = false;
  bool rejected_future_artifact_policy = false;
  try {
    env::replay_driver_detail::validate_driver_replay_environment_contract(
        replay_spec, replay_driver_contract_options);
  } catch (const std::exception &) {
    rejected_future_artifact_policy = true;
  }
  check(rejected_future_artifact_policy,
        "runtime replay driver honors no-future-leakage contract");

  const auto spec = make_episode_spec();
  env::validate_episode_spec(spec);
  check(spec.accepted_range.cursor.batch_cursor_token == "cursor_fixture_10_12",
        "environment carries resolved cursor token");

  auto non_replay_mode_spec = spec;
  non_replay_mode_spec.world_mode = static_cast<env::world_mode_t>(99);
  bool rejected_non_replay_world_mode = false;
  try {
    env::validate_episode_spec(non_replay_mode_spec);
  } catch (const std::exception &) {
    rejected_non_replay_world_mode = true;
  }
  check(rejected_non_replay_world_mode,
        "environment EpisodeSpec rejects non-replay world modes in V1");

  auto split_base_spec = spec;
  split_base_spec.base_policy.projection_reference_node_id = "USD";
  bool rejected_split_base_policy = false;
  try {
    env::validate_episode_spec(split_base_spec);
  } catch (const std::exception &) {
    rejected_split_base_policy = true;
  }
  check(rejected_split_base_policy,
        "environment V1 rejects split base-policy nodes");

  auto missing_reserve_node_spec = spec;
  missing_reserve_node_spec.graph_node_ids = {"BTC", "ETH"};
  bool rejected_missing_reserve_node = false;
  try {
    env::validate_episode_spec(missing_reserve_node_spec);
  } catch (const std::exception &) {
    rejected_missing_reserve_node = true;
  }
  check(rejected_missing_reserve_node,
        "environment rejects base reserve outside graph_node_ids");

  auto bad_cursor_spec = spec;
  bad_cursor_spec.accepted_range.cursor.anchor_index_end = 13;
  bool rejected_cursor_range_mismatch = false;
  try {
    env::validate_episode_spec(bad_cursor_spec);
  } catch (const std::exception &) {
    rejected_cursor_range_mismatch = true;
  }
  check(rejected_cursor_range_mismatch,
        "environment rejects accepted cursor/requested range mismatch");

  auto shuffled_order_spec = spec;
  shuffled_order_spec.requested_wave.source_order_policy = "random";
  shuffled_order_spec.accepted_range.cursor.source_order_policy = "random";
  bool rejected_nonsequential_source_order = false;
  try {
    env::validate_episode_spec(shuffled_order_spec);
  } catch (const std::exception &) {
    rejected_nonsequential_source_order = true;
  }
  check(rejected_nonsequential_source_order,
        "environment replay V1 rejects non-sequential source order policy");

  auto bad_accepted_spec = spec;
  bad_accepted_spec.accepted_range.anchor_index_begin = 9;
  bad_accepted_spec.accepted_range.anchor_index_end = 12;
  bad_accepted_spec.accepted_range.anchor_keys = {"anchor_9", "anchor_10",
                                                  "anchor_11"};
  bool rejected_accepted_range_outside_request = false;
  try {
    env::validate_episode_spec(bad_accepted_spec);
  } catch (const std::exception &) {
    rejected_accepted_range_outside_request = true;
  }
  check(rejected_accepted_range_outside_request,
        "environment rejects accepted range outside requested range");

  const auto action = make_action();
  env::validate_action(action, spec.risky_node_ids);
  env::validate_episode_action(action, spec);

  auto bad_action_schema = action;
  bad_action_schema.action_schema_id = "fixture.unknown_action.v1";
  bool rejected_bad_action_schema = false;
  try {
    env::validate_action(bad_action_schema, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_bad_action_schema = true;
  }
  check(rejected_bad_action_schema,
        "environment rejects unexpected action schema ids");

  auto bad_action = action;
  bad_action.target_base_reserve_weight = 0.40;
  bool rejected_bad_budget = false;
  try {
    env::validate_action(bad_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_bad_budget = true;
  }
  check(rejected_bad_budget, "environment rejects action budget mismatch");

  auto bad_reserve_action = action;
  bad_reserve_action.base_reserve_node_id = "BUSD";
  bool rejected_wrong_reserve = false;
  try {
    env::validate_episode_action(bad_reserve_action, spec);
  } catch (const std::exception &) {
    rejected_wrong_reserve = true;
  }
  check(rejected_wrong_reserve,
        "environment rejects actions for the wrong episode reserve node");

  auto bad_risk_evidence_action = action;
  bad_risk_evidence_action.risk_gate.reasons.push_back("fixture.reason");
  bool rejected_unflagged_risk_evidence = false;
  try {
    env::validate_action(bad_risk_evidence_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_unflagged_risk_evidence = true;
  }
  check(rejected_unflagged_risk_evidence,
        "environment rejects unflagged action risk-gate evidence");

  auto invalid_rebalance_action = action;
  invalid_rebalance_action.rebalance_plan_available = true;
  invalid_rebalance_action.rebalance_plan_source = "fixture.invalid_plan";
  invalid_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  invalid_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  invalid_rebalance_action.rebalance_plan.requested_turnover_weight = 0.10;
  invalid_rebalance_action.rebalance_plan.routed_turnover_weight = 0.05;
  invalid_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  invalid_rebalance_action.rebalance_plan.routed_notional_base = 50.0;
  invalid_rebalance_action.rebalance_plan.estimated_transaction_cost_base =
      0.25;
  bool rejected_invalid_rebalance_evidence = false;
  try {
    env::validate_action(invalid_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_invalid_rebalance_evidence = true;
  }
  check(rejected_invalid_rebalance_evidence,
        "environment rejects unavailable-validity action rebalance evidence");

  auto untimestamped_rebalance_action = action;
  untimestamped_rebalance_action.rebalance_plan_available = true;
  untimestamped_rebalance_action.rebalance_plan_source =
      "fixture.untimestamped_plan";
  untimestamped_rebalance_action.rebalance_plan.valid = true;
  untimestamped_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  untimestamped_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  untimestamped_rebalance_action.rebalance_plan.requested_turnover_weight =
      0.10;
  untimestamped_rebalance_action.rebalance_plan.routed_turnover_weight = 0.05;
  untimestamped_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  untimestamped_rebalance_action.rebalance_plan.routed_notional_base = 50.0;
  untimestamped_rebalance_action.rebalance_plan
      .estimated_transaction_cost_base = 0.25;
  bool rejected_untimestamped_rebalance_evidence = false;
  try {
    env::validate_action(untimestamped_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_untimestamped_rebalance_evidence = true;
  }
  check(rejected_untimestamped_rebalance_evidence,
        "environment rejects untimestamped action rebalance evidence");

  auto bad_rebalance_action = action;
  bad_rebalance_action.rebalance_plan_available = true;
  bad_rebalance_action.rebalance_plan_source = "fixture.bad_plan";
  bad_rebalance_action.rebalance_plan.node_ids = {"ETH", "BTC"};
  bad_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  bad_rebalance_action.rebalance_plan.valid = true;
  bad_rebalance_action.rebalance_plan.timestamp_ms = 1;
  bool rejected_bad_rebalance_evidence = false;
  try {
    env::validate_action(bad_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_bad_rebalance_evidence = true;
  }
  check(rejected_bad_rebalance_evidence,
        "environment rejects mismatched action rebalance evidence");

  auto bad_routed_rebalance_action = action;
  bad_routed_rebalance_action.rebalance_plan_available = true;
  bad_routed_rebalance_action.rebalance_plan_source = "fixture.bad_route";
  bad_routed_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  bad_routed_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  bad_routed_rebalance_action.rebalance_plan.valid = true;
  bad_routed_rebalance_action.rebalance_plan.timestamp_ms = 1;
  bad_routed_rebalance_action.rebalance_plan.requested_turnover_weight = 0.10;
  bad_routed_rebalance_action.rebalance_plan.routed_turnover_weight = 0.20;
  bad_routed_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  bad_routed_rebalance_action.rebalance_plan.routed_notional_base = 200.0;
  bad_routed_rebalance_action.rebalance_plan.estimated_transaction_cost_base =
      0.0;
  bool rejected_routed_gt_requested = false;
  try {
    env::validate_action(bad_routed_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_routed_gt_requested = true;
  }
  check(rejected_routed_gt_requested,
        "environment rejects rebalance evidence routed above requested");

  auto summary_rebalance_action = action;
  summary_rebalance_action.rebalance_plan_available = true;
  summary_rebalance_action.rebalance_plan_source = "fixture.summary_plan";
  summary_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  summary_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  summary_rebalance_action.rebalance_plan.valid = true;
  summary_rebalance_action.rebalance_plan.timestamp_ms = 1;
  summary_rebalance_action.rebalance_plan.requested_turnover_weight = 0.10;
  summary_rebalance_action.rebalance_plan.routed_turnover_weight = 0.05;
  summary_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  summary_rebalance_action.rebalance_plan.routed_notional_base = 50.0;
  summary_rebalance_action.rebalance_plan.estimated_transaction_cost_base =
      0.25;
  env::validate_action(summary_rebalance_action, spec.risky_node_ids);

  auto bad_skipped_rebalance_action = action;
  bad_skipped_rebalance_action.rebalance_plan_available = true;
  bad_skipped_rebalance_action.rebalance_plan_source = "fixture.bad_skipped";
  bad_skipped_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  bad_skipped_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  bad_skipped_rebalance_action.rebalance_plan.valid = true;
  bad_skipped_rebalance_action.rebalance_plan.timestamp_ms = 1;
  bad_skipped_rebalance_action.rebalance_plan.requested_turnover_weight = 0.10;
  bad_skipped_rebalance_action.rebalance_plan.routed_turnover_weight = 0.0;
  bad_skipped_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  bad_skipped_rebalance_action.rebalance_plan.routed_notional_base = 0.0;
  bad_skipped_rebalance_action.rebalance_plan.estimated_transaction_cost_base =
      0.0;
  bad_skipped_rebalance_action.rebalance_plan.skipped.push_back({
      .node_id = "BTC",
      .delta_weight = 0.10,
      .requested_notional_base = 100.0,
  });
  bool rejected_empty_skipped_reason = false;
  try {
    env::validate_action(bad_skipped_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_empty_skipped_reason = true;
  }
  check(rejected_empty_skipped_reason,
        "environment rejects skipped rebalance evidence without reason");

  auto bad_aggregate_rebalance_action = action;
  bad_aggregate_rebalance_action.rebalance_plan_available = true;
  bad_aggregate_rebalance_action.rebalance_plan_source =
      "fixture.bad_aggregate";
  bad_aggregate_rebalance_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  bad_aggregate_rebalance_action.rebalance_plan.base_reserve_node_id = "USDT";
  bad_aggregate_rebalance_action.rebalance_plan.valid = true;
  bad_aggregate_rebalance_action.rebalance_plan.timestamp_ms = 1;
  bad_aggregate_rebalance_action.rebalance_plan.requested_turnover_weight =
      0.10;
  bad_aggregate_rebalance_action.rebalance_plan.routed_turnover_weight = 0.05;
  bad_aggregate_rebalance_action.rebalance_plan.requested_notional_base = 100.0;
  bad_aggregate_rebalance_action.rebalance_plan.routed_notional_base = 50.0;
  bad_aggregate_rebalance_action.rebalance_plan
      .estimated_transaction_cost_base = 0.0;
  bad_aggregate_rebalance_action.rebalance_plan.orders.push_back({
      .node_id = "BTC",
      .edge_id = "BTCUSDT",
      .edge_base_node_id = "BTC",
      .edge_quote_node_id = "USDT",
      .delta_weight = 0.05,
      .requested_notional_base = 50.0,
      .routed_notional_base = 50.0,
      .estimated_cost_base = 0.0,
  });
  bool rejected_bad_plan_aggregate = false;
  try {
    env::validate_action(bad_aggregate_rebalance_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_bad_plan_aggregate = true;
  }
  check(rejected_bad_plan_aggregate,
        "environment rejects rebalance evidence with inconsistent aggregates");

  auto bad_report_action = action;
  bad_report_action.decision_report_available = true;
  bool rejected_empty_decision_report = false;
  try {
    env::validate_action(bad_report_action, spec.risky_node_ids);
  } catch (const std::exception &) {
    rejected_empty_decision_report = true;
  }
  check(rejected_empty_decision_report,
        "environment rejects empty available decision reports");

  auto reward = env::compute_reward(/*equity_before_base=*/100.0,
                                    /*equity_after_base=*/110.0,
                                    /*drawdown=*/0.05,
                                    /*transaction_cost_base=*/1.0,
                                    /*turnover=*/0.20,
                                    /*invalid_action=*/false,
                                    {.lambda_drawdown = 0.5,
                                     .lambda_transaction_cost = 1.0,
                                     .lambda_turnover = 0.1});
  close(reward.log_growth_reward, std::log(1.10), 1e-12,
        "environment reward log growth");
  check(reward.total < reward.log_growth_reward,
        "environment reward subtracts penalties");

  auto bad_reward_options = env::reward_options_t{};
  bad_reward_options.lambda_transaction_cost = -1.0;
  bool rejected_negative_reward_penalty = false;
  try {
    (void)env::compute_reward(/*equity_before_base=*/100.0,
                              /*equity_after_base=*/110.0,
                              /*drawdown=*/0.0,
                              /*transaction_cost_base=*/1.0,
                              /*turnover=*/0.0,
                              /*invalid_action=*/false, bad_reward_options);
  } catch (const std::exception &) {
    rejected_negative_reward_penalty = true;
  }
  check(rejected_negative_reward_penalty,
        "environment rejects negative reward penalty coefficients");

  auto bad_observation = env::observation_t{};
  bad_observation.anchor_key = "anchor_10";
  bad_observation.observation_anchor_index = 10;
  bad_observation.next_realization_anchor_index = 10;
  bad_observation.knowledge_timestamp_ms = 1010;
  bad_observation.realization_available_after_timestamp_ms = 1011;
  bool rejected_bad_time_boundary = false;
  try {
    env::validate_observation_time_boundary(bad_observation);
  } catch (const std::exception &) {
    rejected_bad_time_boundary = true;
  }
  check(rejected_bad_time_boundary,
        "environment rejects observation time leakage");

  const auto time_boundary_observation = make_observation(10, 1000.0);
  auto early_decision_action =
      make_action(time_boundary_observation.knowledge_timestamp_ms - 1);
  bool rejected_early_action_decision_timestamp = false;
  try {
    env::validate_action_time_boundary(early_decision_action,
                                       time_boundary_observation);
  } catch (const std::exception &) {
    rejected_early_action_decision_timestamp = true;
  }
  check(rejected_early_action_decision_timestamp,
        "environment rejects action decision timestamps before policy "
        "knowledge");

  auto reveal_boundary_decision_action = make_action(
      time_boundary_observation.realization_available_after_timestamp_ms);
  bool rejected_reveal_boundary_action_decision_timestamp = false;
  try {
    env::validate_action_time_boundary(reveal_boundary_decision_action,
                                       time_boundary_observation);
  } catch (const std::exception &) {
    rejected_reveal_boundary_action_decision_timestamp = true;
  }
  check(rejected_reveal_boundary_action_decision_timestamp,
        "environment rejects action decisions at the realization reveal "
        "boundary");

  auto bad_transition = env::transition_t{};
  bad_transition.info.anchor_key = time_boundary_observation.anchor_key;
  bad_transition.info.anchor_index =
      time_boundary_observation.observation_anchor_index;
  bad_transition.next_observation = make_observation(12, 1000.0);
  bool rejected_transition_anchor_skip = false;
  try {
    env::validate_transition_time_boundary(time_boundary_observation,
                                           bad_transition);
  } catch (const std::exception &) {
    rejected_transition_anchor_skip = true;
  }
  check(rejected_transition_anchor_skip,
        "environment rejects transition that skips realization anchor");

  fixture_world_t world{};
  fixture_policy_t policy{};
  auto report = env::run_episode(world, policy, spec);
  check(report.episode_artifact_schema_id ==
            std::string(env::kReplayEpisodeArtifactSchema),
        "environment report is replay audit");
  check(report.transition_count == 2, "environment runs two transitions");
  check(report.step_reports.size() == 2,
        "environment records one compact step report per transition");
  check(report.step_reports[0].step_artifact_schema_id ==
            std::string(env::kReplayStepArtifactSchema),
        "environment step report is replay audit");
  check(report.step_reports[0].episode_id == "episode_0" &&
            report.step_reports[0].protocol_id == "protocol_fixture" &&
            report.step_reports[0].runtime_run_id == "runtime_run_0" &&
            report.step_reports[0].environment_run_id == "environment_run_0" &&
            report.step_reports[0].policy_id == "equal_weight_fixture" &&
            report.step_reports[0].policy_kind == env::policy_kind_t::baseline,
        "environment step report preserves run and policy identity");
  check(report.step_reports[0].anchor_key == "anchor_10" &&
            report.step_reports[0].observation_anchor_index == 10 &&
            report.step_reports[0].next_realization_anchor_index == 11 &&
            report.step_reports[0].action_decision_timestamp_ms ==
                report.step_reports[0].knowledge_timestamp_ms,
        "environment step report preserves observation and action time law");
  check(report.step_reports[0].accepted_cursor_wave_id == "wave_fixture" &&
            report.step_reports[0].accepted_cursor_kind == "graph_anchor" &&
            report.step_reports[0].accepted_cursor_scope == "episode" &&
            report.step_reports[0].accepted_batch_cursor_token ==
                "cursor_fixture_10_12" &&
            report.step_reports[0].accepted_anchor_index_begin == 10 &&
            report.step_reports[0].accepted_anchor_index_end == 12 &&
            report.step_reports[0].accepted_cursor_offset == 0,
        "environment step report preserves accepted cursor identity");
  check(report.step_reports[1].accepted_cursor_offset == 1,
        "environment step report preserves accepted cursor offset");
  check(report.step_reports[0].target_base_reserve_node_id == "USDT" &&
            report.step_reports[0].target_risky_node_weights.find("BTC:") !=
                std::string::npos,
        "environment step report preserves compact action target");
  check(std::isfinite(report.step_reports[0].reward_total),
        "environment step report reward is finite");
  check(report.step_reports[0].warning_count == 1 &&
            report.step_reports[0].warnings == "fixture.step_warning" &&
            report.step_reports[0].failure_count == 0 &&
            report.step_reports[0].failures == "none",
        "environment step report preserves warning/failure tokens");

  fixture_world_t policy_drift_world{};
  drifting_action_policy_t drifting_policy{};
  bool rejected_action_policy_drift = false;
  try {
    (void)env::run_episode(policy_drift_world, drifting_policy, spec);
  } catch (const std::exception &) {
    rejected_action_policy_drift = true;
  }
  check(rejected_action_policy_drift,
        "episode runner rejects action policy identity drift");

  bad_reset_observation_world_t bad_reset_observation_world{};
  bool rejected_bad_reset_observation = false;
  try {
    (void)env::run_episode(bad_reset_observation_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_reset_observation = true;
  }
  check(rejected_bad_reset_observation,
        "episode runner rejects reset observations outside accepted cursor "
        "evidence");

  bad_next_realization_observation_world_t bad_next_realization_world{};
  bool rejected_bad_next_realization = false;
  try {
    (void)env::run_episode(bad_next_realization_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_next_realization = true;
  }
  check(rejected_bad_next_realization,
        "episode runner rejects policy observations that skip the immediate "
        "next realization anchor");

  bad_observation_timestamp_world_t bad_observation_timestamp_world{};
  bool rejected_bad_observation_timestamp = false;
  try {
    (void)env::run_episode(bad_observation_timestamp_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_observation_timestamp = true;
  }
  check(rejected_bad_observation_timestamp,
        "episode runner rejects policy observations timestamped after "
        "knowledge time");

  bad_observation_market_world_t bad_observation_market_world{};
  bool rejected_bad_observation_market = false;
  try {
    (void)env::run_episode(bad_observation_market_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_observation_market = true;
  }
  check(rejected_bad_observation_market,
        "episode runner rejects malformed policy-observation market state");

  bad_observation_edge_market_world_t bad_observation_edge_market_world{};
  bool rejected_bad_observation_edge_market = false;
  try {
    (void)env::run_episode(bad_observation_edge_market_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_observation_edge_market = true;
  }
  check(rejected_bad_observation_edge_market,
        "episode runner rejects policy-observation edge markets outside the "
        "episode graph universe");

  bad_transition_rebalance_world_t bad_transition_rebalance_world{};
  bool rejected_bad_transition_rebalance = false;
  try {
    (void)env::run_episode(bad_transition_rebalance_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_transition_rebalance = true;
  }
  check(rejected_bad_transition_rebalance,
        "episode runner rejects malformed transition rebalance evidence");

  bad_transition_fill_world_t bad_transition_fill_world{};
  bool rejected_bad_transition_fill = false;
  try {
    (void)env::run_episode(bad_transition_fill_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_transition_fill = true;
  }
  check(rejected_bad_transition_fill,
        "episode runner rejects malformed transition fill evidence");

  bad_transition_fill_timestamp_world_t bad_transition_fill_timestamp_world{};
  bool rejected_bad_transition_fill_timestamp = false;
  try {
    (void)env::run_episode(bad_transition_fill_timestamp_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_transition_fill_timestamp = true;
  }
  check(rejected_bad_transition_fill_timestamp,
        "episode runner rejects transition fills outside the execution time "
        "window");

  bad_reward_world_t bad_reward_world{};
  bool rejected_bad_reward_transition = false;
  try {
    (void)env::run_episode(bad_reward_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_reward_transition = true;
  }
  check(rejected_bad_reward_transition,
        "episode runner rejects non-finite transition reward evidence");

  mismatched_reward_world_t mismatched_reward_world{};
  bool rejected_mismatched_reward_transition = false;
  try {
    (void)env::run_episode(mismatched_reward_world, policy, spec);
  } catch (const std::exception &) {
    rejected_mismatched_reward_transition = true;
  }
  check(rejected_mismatched_reward_transition,
        "episode runner rejects reward evidence that does not match "
        "post-execution portfolio evidence");

  mismatched_growth_world_t mismatched_growth_world{};
  bool rejected_mismatched_growth_transition = false;
  try {
    (void)env::run_episode(mismatched_growth_world, policy, spec);
  } catch (const std::exception &) {
    rejected_mismatched_growth_transition = true;
  }
  check(rejected_mismatched_growth_transition,
        "episode runner rejects realized growth evidence that does not match "
        "portfolio equity");

  missing_projection_world_t missing_projection_world{};
  bool rejected_missing_projection_transition = false;
  try {
    (void)env::run_episode(missing_projection_world, policy, spec);
  } catch (const std::exception &) {
    rejected_missing_projection_transition = true;
  }
  check(rejected_missing_projection_transition,
        "episode runner rejects missing required projection validation "
        "evidence");

  early_done_world_t early_done_world{};
  bool rejected_early_done_transition = false;
  try {
    (void)env::run_episode(early_done_world, policy, spec);
  } catch (const std::exception &) {
    rejected_early_done_transition = true;
  }
  check(rejected_early_done_transition,
        "episode runner rejects early done before accepted range completion");

  bad_projection_metric_world_t bad_projection_metric_world{};
  bool rejected_bad_projection_metric = false;
  try {
    (void)env::run_episode(bad_projection_metric_world, policy, spec);
  } catch (const std::exception &) {
    rejected_bad_projection_metric = true;
  }
  check(rejected_bad_projection_metric,
        "episode runner rejects semantically inconsistent projection "
        "validation metrics");

  mismatched_equity_world_t mismatched_equity_world{};
  bool rejected_mismatched_equity_transition = false;
  try {
    (void)env::run_episode(mismatched_equity_world, policy, spec);
  } catch (const std::exception &) {
    rejected_mismatched_equity_transition = true;
  }
  check(rejected_mismatched_equity_transition,
        "episode runner rejects transition portfolio/equity mismatch");

  check(report.runtime_run_id == "runtime_run_0",
        "environment report runtime run id");
  check(report.environment_run_id == "environment_run_0",
        "environment report environment run id");
  check(report.graph_node_ids == "BTC,ETH,USDT" &&
            report.risky_node_ids == "BTC,ETH" &&
            report.base_reserve_node_id == "USDT",
        "environment report preserves graph-node universe evidence");
  check(report.accepted_cursor_wave_id == "wave_fixture" &&
            report.requested_anchor_index_begin == 10 &&
            report.requested_anchor_index_end == 12 &&
            report.requested_source_key_begin == -1 &&
            report.requested_source_key_end == -1 &&
            report.accepted_cursor_kind == "graph_anchor" &&
            report.accepted_cursor_scope == "episode" &&
            report.accepted_batch_cursor_token == "cursor_fixture_10_12" &&
            report.accepted_anchor_index_begin == 10 &&
            report.accepted_anchor_index_end == 12 &&
            report.accepted_anchor_keys == "anchor_10,anchor_11",
        "environment report preserves accepted cursor range evidence");
  check(report.policy_kind == env::policy_kind_t::baseline,
        "environment report policy kind");
  close(report.projection_mae, 0.015, 1e-12,
        "environment averages projection MAE");
  check(std::isfinite(report.projection_rmse) &&
            report.projection_rmse + 1e-12 >= report.projection_mae,
        "environment averages projection RMSE");
  check(std::isfinite(report.projection_correlation) &&
            report.projection_correlation >= -1.0 - 1e-12 &&
            report.projection_correlation <= 1.0 + 1e-12,
        "environment averages projection correlation");
  close(report.projection_directional_accuracy, 1.0, 1e-12,
        "environment averages projection direction");
  check(report.total_turnover > 0.0, "environment accumulates turnover");
  check(report.total_transaction_cost_base > 0.0,
        "environment accumulates costs");
  check(report.final_equity_base == 1005.0, "environment report final equity");
}

env::baseline_policy_config_t make_baseline_config(std::string policy_id) {
  return env::baseline_policy_config_t{
      .policy_id = std::move(policy_id),
      .node_ids = {"BTC", "ETH"},
      .base_reserve_node_id = "USDT",
  };
}

env::baseline_policy_config_t
make_baseline_config_for_bundle(std::string policy_id,
                                const replay::replay_episode_bundle_t &bundle) {
  return env::baseline_policy_config_t{
      .policy_id = std::move(policy_id),
      .node_ids = bundle.spec.risky_node_ids,
      .base_reserve_node_id = bundle.spec.base_policy.reserve_asset_id,
  };
}

env::belief::AllocationBelief
make_environment_allocation_belief(std::string anchor_key,
                                   env::portfolio::timestamp_ms_t timestamp) {
  env::belief::AllocationBelief belief{};
  belief.anchor_key = std::move(anchor_key);
  belief.timestamp_ms = timestamp;
  belief.graph_order_fingerprint = "graph_fixture";
  belief.graph_node_ids = {"BTC", "ETH", "USDT"};
  belief.graph_node_axis = env::belief::make_graph_node_axis_binding(
      belief.graph_order_fingerprint, belief.graph_node_ids,
      "environment_fixture_graph_node_axis");
  const auto semantics = env::observer::make_kline_feature_semantics();
  belief.source_feature_semantics_id = semantics.semantics_id;
  belief.source_feature_semantics_fingerprint = semantics.fingerprint;
  belief.node_ids = {"BTC", "ETH"};
  belief.node_graph_indices = {0, 1};
  belief.base_policy = {.accounting_numeraire_id = "USDT",
                        .settlement_asset_id = "USDT",
                        .reserve_asset_id = "USDT",
                        .projection_reference_node_id = "USDT"};
  belief.projection_reference_graph_index = 2;
  belief.valid_mask = torch::ones({2}, torch::kBool);
  belief.tradable_mask = torch::ones({2}, torch::kBool);
  belief.expected_log_return = torch::tensor(
      {0.015, 0.006}, torch::TensorOptions().dtype(torch::kFloat64));
  belief.expected_arithmetic_return = torch::tensor(
      {0.016, 0.006}, torch::TensorOptions().dtype(torch::kFloat64));
  belief.marginal_variance = torch::tensor(
      {0.0004, 0.0002}, torch::TensorOptions().dtype(torch::kFloat64));
  belief.marginal_volatility = belief.marginal_variance.sqrt();
  belief.scenarios =
      torch::tensor({{0.020, 0.004},
                     {0.015, 0.006},
                     {-0.006, 0.001},
                     {0.030, 0.010},
                     {0.010, -0.002}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  belief.covariance =
      torch::tensor({{0.0004, 0.00004}, {0.00004, 0.0002}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  belief.correlation =
      torch::tensor({{1.0, 0.20}, {0.20, 1.0}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  belief.confidence = torch::tensor({0.90, 0.80}, torch::kFloat64);
  belief.linear_cost = torch::full({2}, 0.0001, torch::kFloat64);
  belief.quadratic_impact = torch::zeros({2}, torch::kFloat64);
  belief.capacity_weight_limit = torch::full({2}, 0.60, torch::kFloat64);
  belief.liquidity_score = torch::ones({2}, torch::kFloat64);
  belief.projection_validation_required = false;
  belief.projection_validated = false;
  belief.live_capital_allowed = false;
  belief.valid = true;
  env::belief::validate_allocation_belief_contract(belief);
  return belief;
}

env::belief::AllocationBelief
make_source_allocation_belief(std::string anchor_key,
                              env::portfolio::timestamp_ms_t timestamp,
                              const graph::market_graph_t &graph) {
  auto belief =
      make_environment_allocation_belief(std::move(anchor_key), timestamp);
  belief.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  belief.graph_node_ids = graph.node_ids;
  belief.graph_node_axis = env::belief::make_graph_node_axis_binding(
      belief.graph_order_fingerprint, belief.graph_node_ids,
      "source_fixture_graph_node_axis");
  env::belief::validate_allocation_belief_contract(belief);
  return belief;
}

env::belief::NodeLiftPotentialBelief
make_source_nodelift_potential_belief(std::string anchor_key,
                                      env::portfolio::timestamp_ms_t timestamp,
                                      const graph::market_graph_t &graph) {
  env::belief::NodeLiftPotentialBelief belief{};
  belief.anchor_key = std::move(anchor_key);
  belief.timestamp_ms = timestamp;
  belief.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  belief.graph_node_ids = graph.node_ids;
  belief.graph_node_axis = env::belief::make_graph_node_axis_binding(
      belief.graph_order_fingerprint, belief.graph_node_ids,
      "source_fixture_graph_node_axis");
  const auto semantics = env::observer::make_kline_feature_semantics();
  belief.source_feature_semantics_id = semantics.semantics_id;
  belief.source_feature_semantics_fingerprint = semantics.fingerprint;
  belief.log_weight =
      torch::zeros({static_cast<std::int64_t>(graph.node_ids.size()), 9, 1},
                   torch::TensorOptions().dtype(torch::kFloat64));
  belief.mu =
      torch::zeros({static_cast<std::int64_t>(graph.node_ids.size()), 9, 1},
                   torch::TensorOptions().dtype(torch::kFloat64));
  belief.sigma =
      torch::full({static_cast<std::int64_t>(graph.node_ids.size()), 9, 1},
                  0.01, torch::TensorOptions().dtype(torch::kFloat64));
  belief.active_mask = torch::ones(
      {static_cast<std::int64_t>(graph.node_ids.size()), 9}, torch::kBool);
  belief.valid = true;
  return belief;
}

replay::replay_frame_t make_replay_frame(std::int64_t anchor_index,
                                         torch::Tensor realized_log_return);

void test_baseline_policies() {
  const auto observation = make_observation(10, 1000.0);

  env::base_reserve_policy_t reserve_only(
      make_baseline_config("base_reserve_only"));
  auto reserve_action = reserve_only.act(observation);
  env::validate_action(reserve_action, {"BTC", "ETH"});
  close(reserve_action.target_weights.sum().item<double>(), 0.0, 1e-12,
        "base reserve policy holds no risky weight");
  close(reserve_action.target_base_reserve_weight, 1.0, 1e-12,
        "base reserve policy holds reserve node");

  env::equal_weight_policy_t equal_weight(make_baseline_config("equal_weight"),
                                          0.20);
  auto equal_action = equal_weight.act(observation);
  env::validate_action(equal_action, {"BTC", "ETH"});
  close(equal_action.target_weights.index({0}).item<double>(), 0.40, 1e-12,
        "equal weight policy first risky asset");
  close(equal_action.target_weights.index({1}).item<double>(), 0.40, 1e-12,
        "equal weight policy second risky asset");
  close(equal_action.target_base_reserve_weight, 0.20, 1e-12,
        "equal weight policy reserve weight");

  env::fixed_weight_policy_t fixed_weight(
      make_baseline_config("fixed_weight"),
      torch::tensor({0.10, 0.30}, torch::kFloat64),
      /*target_base_reserve_weight=*/0.60);
  auto fixed_action = fixed_weight.act(observation);
  close(fixed_action.target_weights.index({1}).item<double>(), 0.30, 1e-12,
        "fixed weight policy preserves configured weights");

  env::current_weight_policy_t current_weight(
      make_baseline_config("current_weight"));
  auto current_action = current_weight.act(observation);
  env::validate_action(current_action, {"BTC", "ETH"});
  close(current_action.target_weights.index({0}).item<double>(), 0.20, 1e-12,
        "current weight policy first risky asset");
  close(current_action.target_weights.index({1}).item<double>(), 0.20, 1e-12,
        "current weight policy second risky asset");
  close(current_action.target_base_reserve_weight, 0.60, 1e-12,
        "current weight policy reserve weight");
}

void test_spot_distributional_utility_policy_adapter() {
  auto spec = make_episode_spec();
  spec.require_projection_validation = false;
  std::vector<replay::replay_frame_t> frames;
  frames.push_back(make_replay_frame(
      10, torch::log(torch::tensor({1.04, 0.99}, torch::kFloat64))));
  frames.push_back(make_replay_frame(
      11, torch::log(torch::tensor({0.98, 1.02}, torch::kFloat64))));
  for (auto &frame : frames) {
    frame.observation.allocation_belief = make_environment_allocation_belief(
        frame.observation.anchor_key, frame.observation.timestamp_ms);
  }

  replay::replay_world_options_t options{};
  options.require_projection_validation = false;
  replay::replay_world_t world(frames, options);

  env::spot_distributional_utility_policy_config_t config{};
  config.constraints.min_base_reserve_weight = 0.20;
  config.constraints.max_weight = torch::full({2}, 0.60, torch::kFloat64);
  config.constraints.max_turnover_l1 = 0.80;
  config.constraints.lambda_cvar = 0.10;
  config.constraints.lambda_concentration = 0.01;
  config.solver_options.iterations = 24;
  config.solver_options.learning_rate = 0.05;
  env::spot_distributional_utility_policy_t policy(config);

  auto report = env::run_episode(world, policy, spec);
  check(report.policy_id ==
            std::string(env::kSpotDistributionalUtilityPolicyId),
        "allocation policy uses Kikijyeba policy identity");
  check(report.method_id ==
            std::string(env::kSpotDistributionalUtilityMethodId),
        "allocation policy reports Wikimyei method identity");
  check(report.policy_kind == env::policy_kind_t::deterministic_allocator,
        "allocation policy reports deterministic allocator kind");
  check(report.transition_count == 2,
        "allocation policy runs through replay episode");
  check(policy.last_target().valid, "allocation policy stores valid target");
  check(
      policy.last_decision_step_available(),
      "allocation policy uses decision-step evidence when edge market exists");
  check(report.step_reports[0].risk_gate_evaluated &&
            report.step_reports[0].method_id ==
                std::string(env::kSpotDistributionalUtilityMethodId) &&
            report.step_reports[0].decision_report_text_bytes > 0 &&
            report.step_reports[0].rebalance_plan_source ==
                std::string(
                    cuwacunu::cajtucu::execution::kCajtucuPaperBackendIdV1) &&
            report.step_reports[0].rebalance_plan_enforced &&
            report.step_reports[0].execution_model ==
                std::string(
                    cuwacunu::cajtucu::execution::kCajtucuPaperBackendIdV1) &&
            report.step_reports[0].cajtucu_execution_trace_available,
        "allocation replay step preserves decision-step evidence while "
        "executing through Cajtucu");
  check(policy.last_target().base_reserve_node_id == "USDT",
        "allocation policy target uses graph reserve node");
}

replay::replay_frame_t make_replay_frame(std::int64_t anchor_index,
                                         torch::Tensor realized_log_return) {
  replay::replay_frame_t frame{};
  frame.observation = make_observation(anchor_index, 1000.0);
  frame.realized_log_return = realized_log_return.to(torch::kFloat64);
  frame.projected_log_return_scenarios = torch::stack(
      {frame.realized_log_return - 0.001, frame.realized_log_return,
       frame.realized_log_return + 0.001},
      /*dim=*/0);
  frame.nodelift_residual_energy =
      torch::full({2, 9}, 0.01, torch::TensorOptions().dtype(torch::kFloat64));
  return frame;
}

void test_replay_world() {
  const auto spec = make_episode_spec();
  std::vector<replay::replay_frame_t> frames;
  frames.push_back(make_replay_frame(
      10, torch::log(torch::tensor({1.04, 0.99}, torch::kFloat64))));
  frames.push_back(make_replay_frame(
      11, torch::log(torch::tensor({0.98, 1.02}, torch::kFloat64))));

  replay::replay_world_options_t options{};
  options.linear_transaction_cost_rate = 0.001;
  options.reward_options.lambda_transaction_cost = 1.0;
  replay::replay_world_t world(frames, options);

  auto disabled_projection_options = options;
  disabled_projection_options.require_projection_validation = false;
  replay::replay_world_t disabled_projection_world(frames,
                                                   disabled_projection_options);
  bool rejected_disabled_projection_world = false;
  try {
    (void)disabled_projection_world.reset(spec);
  } catch (const std::exception &) {
    rejected_disabled_projection_world = true;
  }
  check(rejected_disabled_projection_world,
        "replay world rejects disabled projection validation when EpisodeSpec "
        "requires it");

  auto bad_reward_options = options;
  bad_reward_options.reward_options.lambda_turnover = -1.0;
  replay::replay_world_t bad_reward_options_world(frames, bad_reward_options);
  bool rejected_bad_reward_options_world = false;
  try {
    (void)bad_reward_options_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_reward_options_world = true;
  }
  check(rejected_bad_reward_options_world,
        "replay world rejects negative reward penalty coefficients");

  auto bad_world_options = options;
  bad_world_options.realized_return_consistency_tolerance =
      std::numeric_limits<double>::quiet_NaN();
  replay::replay_world_t bad_options_world(frames, bad_world_options);
  bool rejected_bad_world_options = false;
  try {
    (void)bad_options_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_world_options = true;
  }
  check(rejected_bad_world_options,
        "replay world rejects non-finite replay option tolerances");

  auto bad_next_frames = frames;
  bad_next_frames.back().observation.next_realization_anchor_index =
      spec.accepted_range.anchor_index_end + 1;
  replay::replay_world_t bad_next_world(bad_next_frames, options);
  bool rejected_bad_frame_next_realization = false;
  try {
    (void)bad_next_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_frame_next_realization = true;
  }
  check(rejected_bad_frame_next_realization,
        "replay world rejects frames that skip the immediate next realization "
        "anchor");

  auto observation = world.reset(spec);
  check(observation.anchor_key == "anchor_10", "replay reset first anchor");
  env::validate_observation_time_boundary(observation);

  auto first = world.step(make_action());
  check(!first.done, "replay first step not done");
  check(first.next_observation.anchor_key == "anchor_11",
        "replay first step advances anchor");
  check(first.next_observation.portfolio_state.equity_value_base ==
            first.info.portfolio_after.equity_value_base,
        "replay next observation carries updated portfolio state");
  check(first.info.projection_validation.available,
        "replay computes projection validation");
  check(first.info.residual_quality.available,
        "replay computes residual quality");
  check(first.info.rebalance_plan.valid, "replay creates rebalance plan");
  check(!first.info.fills.empty(), "replay creates simulated fills");
  check(first.info.transaction_cost_base > 0.0,
        "replay computes transaction cost");
  check(first.info.cajtucu_execution_trace_available,
        "replay exposes Cajtucu execution trace");
  close(
      first.info.execution_trace.ledger_before.units.index({0}).item<double>(),
      observation.portfolio_state.current_units.index({0}).item<double>(),
      1.0e-9, "replay Cajtucu ledger preserves BTC units");
  close(
      first.info.execution_trace.ledger_before.units.index({1}).item<double>(),
      observation.portfolio_state.current_units.index({1}).item<double>(),
      1.0e-9, "replay Cajtucu ledger preserves ETH units");

  auto delayed_knowledge_frames = frames;
  for (auto &frame : delayed_knowledge_frames) {
    frame.observation.timestamp_ms =
        frame.observation.knowledge_timestamp_ms - 1;
    frame.observation.market_state.timestamp_ms =
        frame.observation.timestamp_ms;
  }
  replay::replay_world_t delayed_knowledge_world(delayed_knowledge_frames,
                                                 options);
  auto delayed_observation = delayed_knowledge_world.reset(spec);
  check(delayed_observation.timestamp_ms <
            delayed_observation.knowledge_timestamp_ms,
        "replay permits observations whose event timestamp precedes policy "
        "knowledge");
  auto delayed_step = delayed_knowledge_world.step(make_action());
  check(delayed_step.info.target.timestamp_ms ==
            delayed_observation.knowledge_timestamp_ms,
        "replay stamps target decisions at the policy knowledge boundary");
  check(!delayed_step.info.fills.empty(),
        "delayed-knowledge replay still emits simulated fills");
  for (const auto &fill : delayed_step.info.fills) {
    check(fill.timestamp_ms == delayed_observation.knowledge_timestamp_ms,
          "replay stamps fills at the policy knowledge boundary");
  }

  auto bad_policy_plan_action = make_action();
  bad_policy_plan_action.rebalance_plan_available = true;
  bad_policy_plan_action.rebalance_plan_source = "fixture.bad_turnover_plan";
  bad_policy_plan_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  bad_policy_plan_action.rebalance_plan.base_reserve_node_id = "USDT";
  bad_policy_plan_action.rebalance_plan.timestamp_ms =
      frames[0].observation.knowledge_timestamp_ms;
  bad_policy_plan_action.rebalance_plan.valid = true;
  bad_policy_plan_action.rebalance_plan.requested_turnover_weight = 0.90;
  bad_policy_plan_action.rebalance_plan.routed_turnover_weight = 0.90;
  bad_policy_plan_action.rebalance_plan.requested_notional_base = 900.0;
  bad_policy_plan_action.rebalance_plan.routed_notional_base = 900.0;
  bad_policy_plan_action.rebalance_plan.estimated_transaction_cost_base = 0.0;
  replay::replay_world_t bad_policy_plan_world(frames, options);
  (void)bad_policy_plan_world.reset(spec);
  bool rejected_bad_policy_plan = false;
  try {
    (void)bad_policy_plan_world.step(bad_policy_plan_action);
  } catch (const std::exception &) {
    rejected_bad_policy_plan = true;
  }
  check(rejected_bad_policy_plan,
        "replay rejects policy rebalance evidence inconsistent with action "
        "turnover");

  auto untimestamped_policy_plan_action = make_action();
  untimestamped_policy_plan_action.rebalance_plan_available = true;
  untimestamped_policy_plan_action.rebalance_plan_source =
      "fixture.untimestamped_plan";
  untimestamped_policy_plan_action.rebalance_plan.node_ids = {"BTC", "ETH"};
  untimestamped_policy_plan_action.rebalance_plan.base_reserve_node_id = "USDT";
  untimestamped_policy_plan_action.rebalance_plan.valid = true;
  untimestamped_policy_plan_action.rebalance_plan.requested_turnover_weight =
      0.10;
  untimestamped_policy_plan_action.rebalance_plan.routed_turnover_weight = 0.10;
  untimestamped_policy_plan_action.rebalance_plan.requested_notional_base =
      100.0;
  untimestamped_policy_plan_action.rebalance_plan.routed_notional_base = 100.0;
  untimestamped_policy_plan_action.rebalance_plan
      .estimated_transaction_cost_base = 0.0;
  replay::replay_world_t untimestamped_policy_plan_world(frames, options);
  (void)untimestamped_policy_plan_world.reset(spec);
  bool rejected_untimestamped_policy_plan = false;
  try {
    (void)untimestamped_policy_plan_world.step(
        untimestamped_policy_plan_action);
  } catch (const std::exception &) {
    rejected_untimestamped_policy_plan = true;
  }
  check(rejected_untimestamped_policy_plan,
        "replay rejects action-provided rebalance evidence without a "
        "decision-time timestamp");

  auto high_cost_options = options;
  high_cost_options.linear_transaction_cost_rate = 20.0;
  replay::replay_world_t high_cost_world(frames, high_cost_options);
  (void)high_cost_world.reset(spec);
  auto high_cost = high_cost_world.step(make_action());
  check(high_cost.info.invalid_action,
        "replay marks underfunded transaction costs as invalid action");
  check(high_cost.reward.invalid_action_penalty > 0.0,
        "replay returns invalid-action penalty instead of throwing");
  check(high_cost.info.portfolio_after.base_reserve_weight >= 0.0,
        "replay keeps reserve weight nonnegative after underfunded costs");

  replay::replay_world_t episode_world(frames, options);
  fixture_policy_t policy{};
  auto report = env::run_episode(episode_world, policy, spec);
  check(report.transition_count == 2, "replay episode runs all frames");
  check(report.final_equity_base > 0.0, "replay final equity positive");
  check(report.total_transaction_cost_base > 0.0,
        "replay report accumulates transaction cost");
  check(report.projection_mae <= 0.001 + 1e-12,
        "replay report aggregates projection validation");
  check(std::isfinite(report.projection_rmse) &&
            std::isfinite(report.projection_correlation),
        "replay report aggregates projection RMSE and correlation");
  check(report.step_reports[0].rebalance_plan_valid &&
            report.step_reports[0].rebalance_plan_enforced &&
            report.step_reports[0].execution_model ==
                std::string(
                    cuwacunu::cajtucu::execution::kCajtucuPaperBackendIdV1) &&
            report.step_reports[0].cajtucu_execution_trace_available &&
            report.step_reports[0].fill_count == 2 &&
            report.step_reports[0].fill_gross_notional_base > 0.0 &&
            report.step_reports[0].residual_quality_available,
        "replay step report preserves execution and residual evidence");
  check(!report.step_reports[0].risk_gate_evaluated,
        "replay step report distinguishes unevaluated risk gate");

  auto bad_frames = frames;
  bad_frames[0].observation.anchor_key = "wrong_anchor";
  replay::replay_world_t bad_world(bad_frames, options);
  bool rejected_bad_cursor_frame = false;
  try {
    (void)bad_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_cursor_frame = true;
  }
  check(rejected_bad_cursor_frame,
        "replay rejects frame outside accepted cursor evidence");

  auto wrong_reserve_frames = frames;
  wrong_reserve_frames[0].observation.portfolio_state.reserve_node_id = "USD";
  replay::replay_world_t wrong_reserve_world(wrong_reserve_frames, options);
  bool rejected_wrong_reserve_state = false;
  try {
    (void)wrong_reserve_world.reset(spec);
  } catch (const std::exception &) {
    rejected_wrong_reserve_state = true;
  }
  check(rejected_wrong_reserve_state,
        "replay rejects frame portfolio state with wrong reserve node");

  auto future_market_frames = frames;
  future_market_frames[0].observation.market_state.timestamp_ms =
      frames[0].observation.knowledge_timestamp_ms + 1;
  replay::replay_world_t future_market_world(future_market_frames, options);
  bool rejected_future_market_state = false;
  try {
    (void)future_market_world.reset(spec);
  } catch (const std::exception &) {
    rejected_future_market_state = true;
  }
  check(rejected_future_market_state,
        "replay rejects future-dated market state");

  auto malformed_market_frames = frames;
  malformed_market_frames[0].observation.market_state.executable_mid =
      torch::tensor({100.0}, torch::TensorOptions().dtype(torch::kFloat64));
  replay::replay_world_t malformed_market_world(malformed_market_frames,
                                                options);
  bool rejected_malformed_market_state = false;
  try {
    (void)malformed_market_world.reset(spec);
  } catch (const std::exception &) {
    rejected_malformed_market_state = true;
  }
  check(rejected_malformed_market_state,
        "replay rejects malformed frame market state");

  auto malformed_edge_market_frames = frames;
  malformed_edge_market_frames[0].observation.edge_market_state.edge_mid_price =
      torch::tensor({100.0}, torch::TensorOptions().dtype(torch::kFloat64));
  replay::replay_world_t malformed_edge_market_world(
      malformed_edge_market_frames, options);
  bool rejected_malformed_edge_market_state = false;
  try {
    (void)malformed_edge_market_world.reset(spec);
  } catch (const std::exception &) {
    rejected_malformed_edge_market_state = true;
  }
  check(rejected_malformed_edge_market_state,
        "replay rejects malformed frame edge market state");

  auto bad_projection_frames = frames;
  bad_projection_frames[0].projected_log_return_scenarios =
      torch::tensor({{0.0, std::numeric_limits<double>::quiet_NaN()}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  replay::replay_world_t bad_projection_world(bad_projection_frames, options);
  bool rejected_bad_projection_tensor = false;
  try {
    (void)bad_projection_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_projection_tensor = true;
  }
  check(rejected_bad_projection_tensor,
        "replay rejects non-finite projection scenario tensors");

  auto bad_projection_mask_frames = frames;
  bad_projection_mask_frames[0].active_projection_mask =
      torch::zeros({2}, torch::kBool);
  replay::replay_world_t bad_projection_mask_world(bad_projection_mask_frames,
                                                   options);
  bool rejected_bad_projection_mask = false;
  try {
    (void)bad_projection_mask_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_projection_mask = true;
  }
  check(rejected_bad_projection_mask,
        "replay rejects projection masks without active assets");

  auto bad_residual_frames = frames;
  bad_residual_frames[0].nodelift_residual_mask =
      torch::ones({3}, torch::kBool);
  replay::replay_world_t bad_residual_world(bad_residual_frames, options);
  bool rejected_bad_residual_tensor = false;
  try {
    (void)bad_residual_world.reset(spec);
  } catch (const std::exception &) {
    rejected_bad_residual_tensor = true;
  }
  check(rejected_bad_residual_tensor,
        "replay rejects malformed residual diagnostic tensors");

  auto future_allocation_belief_frames = frames;
  future_allocation_belief_frames[0].observation.allocation_belief =
      make_environment_allocation_belief(
          frames[0].observation.anchor_key,
          frames[0].observation.knowledge_timestamp_ms + 1);
  replay::replay_world_t future_allocation_belief_world(
      future_allocation_belief_frames, options);
  bool rejected_future_allocation_belief = false;
  try {
    (void)future_allocation_belief_world.reset(spec);
  } catch (const std::exception &) {
    rejected_future_allocation_belief = true;
  }
  check(rejected_future_allocation_belief,
        "replay rejects future-dated AllocationBelief in frame observation");

  auto wrong_node_allocation_belief_frames = frames;
  auto wrong_node_belief = make_environment_allocation_belief(
      frames[0].observation.anchor_key, frames[0].observation.timestamp_ms);
  wrong_node_belief.node_ids = {"ETH", "BTC"};
  wrong_node_allocation_belief_frames[0].observation.allocation_belief =
      std::move(wrong_node_belief);
  replay::replay_world_t wrong_node_allocation_belief_world(
      wrong_node_allocation_belief_frames, options);
  bool rejected_wrong_node_allocation_belief = false;
  try {
    (void)wrong_node_allocation_belief_world.reset(spec);
  } catch (const std::exception &) {
    rejected_wrong_node_allocation_belief = true;
  }
  check(rejected_wrong_node_allocation_belief,
        "replay rejects AllocationBelief with wrong risky node order");

  auto future_nodelift_belief_frames = frames;
  env::belief::NodeLiftPotentialBelief future_nodelift_belief{};
  future_nodelift_belief.anchor_key = frames[0].observation.anchor_key;
  future_nodelift_belief.timestamp_ms =
      frames[0].observation.knowledge_timestamp_ms + 1;
  future_nodelift_belief.graph_order_fingerprint = spec.graph_order_fingerprint;
  future_nodelift_belief.graph_node_ids = spec.graph_node_ids;
  future_nodelift_belief_frames[0].observation.nodelift_potential_belief =
      future_nodelift_belief;
  replay::replay_world_t future_nodelift_belief_world(
      future_nodelift_belief_frames, options);
  bool rejected_future_nodelift_belief = false;
  try {
    (void)future_nodelift_belief_world.reset(spec);
  } catch (const std::exception &) {
    rejected_future_nodelift_belief = true;
  }
  check(rejected_future_nodelift_belief,
        "replay rejects future-dated NodeLiftPotentialBelief");

  auto skipped_sequence_frames = frames;
  skipped_sequence_frames[0].observation.next_realization_anchor_index = 12;
  replay::replay_world_t skipped_sequence_world(skipped_sequence_frames,
                                                options);
  bool rejected_skipped_frame_sequence = false;
  try {
    (void)skipped_sequence_world.reset(spec);
  } catch (const std::exception &) {
    rejected_skipped_frame_sequence = true;
  }
  check(rejected_skipped_frame_sequence,
        "replay rejects frame sequence that skips realization anchor");

  auto leaking_sequence_frames = frames;
  leaking_sequence_frames[1].observation.knowledge_timestamp_ms =
      frames[0].observation.realization_available_after_timestamp_ms - 1;
  replay::replay_world_t leaking_sequence_world(leaking_sequence_frames,
                                                options);
  bool rejected_leaking_frame_sequence = false;
  try {
    (void)leaking_sequence_world.reset(spec);
  } catch (const std::exception &) {
    rejected_leaking_frame_sequence = true;
  }
  check(rejected_leaking_frame_sequence,
        "replay rejects frame sequence with early next knowledge timestamp");

  auto impossible_return_frames = frames;
  impossible_return_frames[0].realized_arithmetic_return =
      torch::tensor({-1.10, 0.0}, torch::kFloat64);
  replay::replay_world_t impossible_return_world(impossible_return_frames,
                                                 options);
  bool rejected_impossible_arithmetic_return = false;
  try {
    (void)impossible_return_world.reset(spec);
  } catch (const std::exception &) {
    rejected_impossible_arithmetic_return = true;
  }
  check(rejected_impossible_arithmetic_return,
        "replay rejects impossible realized arithmetic return");

  auto inconsistent_return_frames = frames;
  inconsistent_return_frames[0].realized_arithmetic_return =
      torch::tensor({0.50, 0.50}, torch::kFloat64);
  replay::replay_world_t inconsistent_return_world(inconsistent_return_frames,
                                                   options);
  bool rejected_inconsistent_realized_returns = false;
  try {
    (void)inconsistent_return_world.reset(spec);
  } catch (const std::exception &) {
    rejected_inconsistent_realized_returns = true;
  }
  check(rejected_inconsistent_realized_returns,
        "replay rejects inconsistent realized return units");
}

graph::market_graph_t make_source_graph() {
  graph::market_graph_t graph{};
  graph.node_ids = {"BTC", "ETH", "USDT"};
  graph.edge_ids = {"BTCUSDT", "USDTETH"};
  graph.base_index = {0, 2};
  graph.quote_index = {2, 1};
  graph.validate();
  return graph;
}

dataloader::graph_anchor_cursor_t<std::int64_t>
make_source_cursor(const graph::market_graph_t &graph) {
  return dataloader::make_graph_anchor_cursor<std::int64_t>(
      graph.computed_graph_order_fingerprint(), /*begin_anchor_index=*/10,
      /*end_anchor_index=*/12, /*requested_batch_size=*/2,
      /*anchor_keys=*/{1000, 2000}, /*anchor_indices=*/{10, 11});
}

protocol::component_stream_wave_t make_source_wave() {
  protocol::component_stream_wave_t wave{};
  wave.wave_id = "wave_source_fixture";
  wave.mode_text = "run";
  wave.source_cursor_kind = "graph_anchor";
  wave.source_cursor_scope = "episode";
  wave.source_range_policy = "anchor_index";
  wave.source_order_policy = "sequential";
  wave.anchor_index_begin = 10;
  wave.anchor_index_end = 12;
  return wave;
}

protocol::component_stream_wave_t make_source_key_wave() {
  auto wave = make_source_wave();
  wave.wave_id = "wave_source_key_fixture";
  wave.source_range_policy = "source_key";
  wave.anchor_index_begin.reset();
  wave.anchor_index_end.reset();
  wave.source_key_begin = 1000;
  wave.source_key_end = 3000;
  return wave;
}

env::episode_spec_t make_source_episode_spec(
    const graph::market_graph_t &graph,
    const dataloader::graph_anchor_cursor_t<std::int64_t> &cursor) {
  auto spec = make_episode_spec();
  spec.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  spec.graph_node_ids = graph.node_ids;
  spec.risky_node_ids = {"BTC", "ETH"};
  spec.base_policy = {.accounting_numeraire_id = "USDT",
                      .settlement_asset_id = "USDT",
                      .reserve_asset_id = "USDT",
                      .projection_reference_node_id = "USDT"};
  const auto wave = make_source_wave();
  return replay::bind_episode_spec_to_graph_cursor(spec, wave, cursor);
}

dataloader::graph_anchor_edge_batch_t<std::int64_t> make_source_edge_batch(
    const graph::market_graph_t &graph,
    const dataloader::graph_anchor_cursor_t<std::int64_t> &cursor) {
  dataloader::graph_anchor_edge_batch_t<std::int64_t> batch{};
  batch.edge_ids = {"BTCUSDT", "USDTETH"};
  batch.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  batch.cursor = cursor;
  batch.anchor_keys = torch::tensor({1000, 2000}, torch::kInt64);
  batch.edge_present = torch::ones({2, 2}, torch::kBool);
  batch.future_features =
      torch::zeros({2, 2, 2, 1, kline::kKlineFeatureWidth},
                   torch::TensorOptions().dtype(torch::kFloat64));
  batch.future_mask = torch::ones({2, 2, 2, 1}, torch::kBool);
  batch.future_keys = torch::zeros({2, 2, 2, 1}, torch::kInt64);

  const auto close_coord =
      kline::kline_feature_index(kline::kline_feature_e::close);
  for (std::int64_t e = 0; e < 2; ++e) {
    for (std::int64_t c = 0; c < 2; ++c) {
      batch.future_keys.index_put_({0, e, c, 0}, 1010);
      batch.future_keys.index_put_({1, e, c, 0}, 2010);
    }
  }

  // BTC/USDT is forward asset/base, so the channel mean is used directly.
  batch.future_features.index_put_({0, 0, 0, 0, close_coord}, 0.01);
  batch.future_features.index_put_({0, 0, 1, 0, close_coord}, 0.03);
  batch.future_features.index_put_({1, 0, 0, 0, close_coord}, -0.02);
  batch.future_features.index_put_({1, 0, 1, 0, close_coord}, 0.00);

  // USDT/ETH is reverse base/asset, so ETH/USDT must flip the sign.
  batch.future_features.index_put_({0, 1, 0, 0, close_coord}, -0.05);
  batch.future_features.index_put_({0, 1, 1, 0, close_coord}, -0.03);
  batch.future_features.index_put_({1, 1, 0, 0, close_coord}, 0.02);
  batch.future_features.index_put_({1, 1, 1, 0, close_coord}, 0.04);

  return batch;
}

mdn::MdnOut make_source_mdn_out() {
  const std::int64_t B = 2;
  const std::int64_t N = 3;
  const std::int64_t C = 3;
  const std::int64_t Df = kline::kKlineFeatureWidth;
  const std::int64_t K = 1;
  auto opts = torch::TensorOptions().dtype(torch::kFloat64);

  mdn::MdnOut out{};
  out.log_pi = torch::zeros({B, N, C, Df, K}, opts);
  out.mu = torch::zeros({B, N, C, Df, K}, opts);
  out.sigma = torch::full({B, N, C, Df, K}, 0.0005, opts);

  const auto close_coord =
      kline::kline_feature_index(kline::kline_feature_e::close);
  const double close_potential[B][N] = {
      {0.02, 0.04, 0.00},
      {-0.01, -0.03, 0.00},
  };
  for (std::int64_t b = 0; b < B; ++b) {
    for (std::int64_t n = 0; n < N; ++n) {
      for (std::int64_t c = 0; c < C; ++c) {
        out.mu.index_put_({b, n, c, close_coord, 0}, close_potential[b][n]);
        out.mu.index_put_(
            {b, n, c,
             kline::kline_feature_index(kline::kline_feature_e::volume), 0},
            1.0 + 0.1 * n);
        out.mu.index_put_(
            {b, n, c,
             kline::kline_feature_index(kline::kline_feature_e::quote_volume),
             0},
            1.2 + 0.1 * n);
        out.mu.index_put_(
            {b, n, c,
             kline::kline_feature_index(kline::kline_feature_e::trades), 0},
            0.8 + 0.1 * n);
      }
    }
  }
  return out;
}

mdn_stream::channel_mdn_input_batch_t<std::int64_t> make_source_mdn_input_batch(
    const graph::market_graph_t &graph,
    const dataloader::graph_anchor_cursor_t<std::int64_t> &cursor) {
  const auto B = static_cast<std::int64_t>(cursor.anchor_count());
  const auto N = static_cast<std::int64_t>(graph.node_ids.size());
  const std::int64_t C = 3;
  const std::int64_t Df = kline::kKlineFeatureWidth;
  const std::int64_t De = 4;

  mdn_stream::channel_mdn_input_batch_t<std::int64_t> batch{};
  batch.context = torch::zeros({B, N, C, De},
                               torch::TensorOptions().dtype(torch::kFloat64));
  batch.context_mask = torch::ones({B, N, C}, torch::kBool);
  batch.future = torch::zeros({B, N, C, Df},
                              torch::TensorOptions().dtype(torch::kFloat64));
  batch.future_mask = torch::ones({B, N, C, Df}, torch::kBool);

  std::vector<std::int64_t> node_index_values;
  std::vector<std::int64_t> anchor_index_values;
  node_index_values.reserve(static_cast<std::size_t>(B * N));
  anchor_index_values.reserve(static_cast<std::size_t>(B * N));
  for (std::int64_t b = 0; b < B; ++b) {
    const auto anchor_index = static_cast<std::int64_t>(
        cursor.anchor_indices[static_cast<std::size_t>(b)]);
    for (std::int64_t n = 0; n < N; ++n) {
      node_index_values.push_back(n);
      anchor_index_values.push_back(anchor_index);
    }
  }
  batch.node_index =
      torch::tensor(node_index_values, torch::kInt64).view({B, N});
  batch.anchor_index =
      torch::tensor(anchor_index_values, torch::kInt64).view({B, N});
  batch.anchor_keys = torch::tensor(cursor.anchor_keys, torch::kInt64);
  batch.node_ids = graph.node_ids;
  batch.edge_ids = graph.edge_ids;
  batch.target_coords.reserve(static_cast<std::size_t>(Df));
  for (std::int64_t coord = 0; coord < Df; ++coord) {
    batch.target_coords.push_back(coord);
  }
  batch.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  batch.cursor = cursor;
  return batch;
}

std::vector<replay::replay_observation_artifacts_t>
make_source_observation_artifacts(const replay::replay_episode_bundle_t &bundle,
                                  const graph::market_graph_t &graph) {
  std::vector<replay::replay_observation_artifacts_t> artifacts;
  artifacts.reserve(bundle.frames.size());
  for (const auto &frame : bundle.frames) {
    replay::replay_observation_artifacts_t artifact{};
    artifact.anchor_key = frame.observation.anchor_key;
    artifact.observation_anchor_index =
        frame.observation.observation_anchor_index;
    artifact.artifact_timestamp_ms = frame.observation.knowledge_timestamp_ms;
    artifact.mdn_artifact = env::artifact_ref_t{
        .artifact_id = "mdn_artifact_" + frame.observation.anchor_key,
        .artifact_path = "/tmp/cuwacunu/mdn/" + frame.observation.anchor_key,
        .artifact_digest = "digest_" + frame.observation.anchor_key,
        .schema_id = "wikimyei.inference.expected_value.mdn.v1",
    };
    artifact.allocation_belief = make_source_allocation_belief(
        frame.observation.anchor_key, frame.observation.timestamp_ms, graph);
    artifact.projected_log_return_scenarios = torch::stack(
        {frame.realized_log_return - 0.001, frame.realized_log_return,
         frame.realized_log_return + 0.001},
        /*dim=*/0);
    artifact.active_projection_mask =
        torch::ones({frame.realized_log_return.size(0)}, torch::kBool);
    artifact.nodelift_residual_energy =
        torch::full({frame.realized_log_return.size(0), 9}, 0.01,
                    torch::TensorOptions().dtype(torch::kFloat64));
    artifacts.push_back(std::move(artifact));
  }
  return artifacts;
}

replay::forecast::marginal_forecast_artifact_t
make_source_forecast_artifact(const replay::replay_frame_t &frame,
                              const graph::market_graph_t &graph) {
  auto belief = make_source_allocation_belief(
      frame.observation.anchor_key, frame.observation.timestamp_ms, graph);
  replay::forecast::marginal_forecast_artifact_t artifact{};
  artifact.identity.anchor_key = belief.anchor_key;
  artifact.identity.timestamp_ms = belief.timestamp_ms;
  artifact.identity.horizon = belief.horizon;
  artifact.identity.graph_order_fingerprint = belief.graph_order_fingerprint;
  artifact.identity.graph_node_axis = belief.graph_node_axis;
  artifact.identity.source_feature_semantics_id =
      belief.source_feature_semantics_id;
  artifact.identity.source_feature_semantics_fingerprint =
      belief.source_feature_semantics_fingerprint;
  artifact.identity.node_ids_fingerprint =
      replay::forecast::node_ids_fingerprint(belief.node_ids);
  artifact.identity.model_version = "environment_fixture_forecast";
  artifact.identity.graph_node_ids = belief.graph_node_ids;
  artifact.identity.node_ids = belief.node_ids;
  artifact.identity.node_graph_indices = belief.node_graph_indices;
  artifact.identity.base_policy = belief.base_policy;
  artifact.identity.projection_reference_graph_index =
      belief.projection_reference_graph_index.value_or(-1);
  artifact.identity.return_origin = "base_relative_nodelift_projection";

  const auto A = static_cast<std::int64_t>(belief.node_ids.size());
  artifact.log_weight = torch::zeros({A, 1}, torch::kFloat64);
  artifact.mu = belief.expected_log_return.unsqueeze(1).to(torch::kFloat64);
  artifact.sigma = torch::full({A, 1}, 0.01, torch::kFloat64);
  artifact.active_mask = torch::ones({A}, torch::kBool);
  artifact.expected_log_return = belief.expected_log_return.to(torch::kFloat64);
  artifact.expected_arithmetic_return =
      belief.expected_arithmetic_return.to(torch::kFloat64);
  artifact.scenarios = belief.scenarios.to(torch::kFloat64);
  artifact.covariance = belief.covariance.to(torch::kFloat64);
  artifact.correlation = belief.correlation.to(torch::kFloat64);
  artifact.confidence = belief.confidence.to(torch::kFloat64);
  replay::forecast::validate_forecast_artifact(artifact,
                                               /*require_scenarios=*/true);
  return artifact;
}

std::vector<replay::replay_observation_artifacts_t>
write_and_load_source_observation_artifact_index(
    const replay::replay_episode_bundle_t &bundle,
    const graph::market_graph_t &graph, const std::filesystem::path &root) {
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "forecast");
  std::vector<replay::replay_observation_artifact_index_entry_t> entries;
  entries.reserve(bundle.frames.size());
  for (const auto &frame : bundle.frames) {
    const auto filename = frame.observation.anchor_key + ".forecast.pt";
    const auto artifact_path = root / "forecast" / filename;
    replay::forecast::save_forecast_artifact(
        make_source_forecast_artifact(frame, graph), artifact_path);
    entries.push_back({
        .anchor_key = frame.observation.anchor_key,
        .observation_anchor_index = frame.observation.observation_anchor_index,
        .artifact_timestamp_ms = frame.observation.knowledge_timestamp_ms,
        .forecast_artifact_path = std::filesystem::path("forecast") / filename,
        .mdn_artifact =
            env::artifact_ref_t{
                .artifact_id = "mdn_artifact_" + frame.observation.anchor_key,
                .artifact_path = "mdn/" + frame.observation.anchor_key,
                .artifact_digest = "digest_" + frame.observation.anchor_key,
                .schema_id = "wikimyei.inference.expected_value.mdn.v1",
            },
    });
  }
  const auto index_path = root / "replay_observation_artifacts.index";
  replay::write_replay_observation_artifact_index(index_path, entries);
  return replay::load_replay_observation_artifacts_from_index(index_path);
}

env::replay_policy_factory_t make_sdu_policy_factory() {
  return {
      .policy_id = env::kSpotDistributionalUtilityPolicyId,
      .policy_kind = env::policy_kind_t::deterministic_allocator,
      .make_policy =
          [](const replay::replay_episode_bundle_t &) {
            env::spot_distributional_utility_policy_config_t config{};
            config.constraints.min_base_reserve_weight = 0.20;
            config.constraints.max_weight =
                torch::full({2}, 0.60, torch::kFloat64);
            config.constraints.max_turnover_l1 = 0.80;
            config.constraints.lambda_cvar = 0.10;
            config.constraints.lambda_concentration = 0.01;
            config.solver_options.iterations = 24;
            config.solver_options.learning_rate = 0.05;
            return std::make_unique<env::spot_distributional_utility_policy_t>(
                config);
          },
  };
}

runtime::wave_plan_t make_runtime_wave_plan(
    const dataloader::graph_anchor_cursor_t<std::int64_t> &cursor) {
  runtime::wave_plan_t plan{};
  plan.wave_id = "wave_source_fixture";
  plan.target_component_family_id = "kikijyeba.environment.replay.v1";
  plan.action = "run";
  plan.mode_text = "run";
  plan.source_cursor_kind = "graph_anchor";
  plan.source_cursor_scope = "episode";
  plan.source_range_policy = "anchor_index";
  plan.source_order_policy = "sequential";
  plan.requested_anchor_index_begin = cursor.begin_anchor_index;
  plan.requested_anchor_index_end = cursor.end_anchor_index;
  plan.resolved_anchor_index_begin = cursor.begin_anchor_index;
  plan.resolved_anchor_index_end = cursor.end_anchor_index;
  plan.accepted_anchor_count = cursor.anchor_count();
  plan.candidate_anchor_count = cursor.anchor_count();
  plan.accepted_anchor_fraction = 1.0;
  plan.source_cursor_token = "runtime_graph_anchor_cursor_report_fixture";
  plan.first_anchor_key = std::to_string(cursor.anchor_keys.front());
  plan.last_anchor_key = std::to_string(cursor.anchor_keys.back());
  return plan;
}

runtime::job_manifest_t
make_runtime_manifest(const graph::market_graph_t &graph,
                      const runtime::wave_plan_t &plan) {
  runtime::job_manifest_t manifest{};
  manifest.job_id = "runtime_job_fixture";
  manifest.protocol_id = "protocol_fixture";
  manifest.component_family_id = "kikijyeba.environment.replay";
  manifest.wave_id = plan.wave_id;
  manifest.wave_mode = plan.mode_text;
  manifest.source_range_policy = plan.source_range_policy;
  manifest.source_order_policy = plan.source_order_policy;
  manifest.resolved_anchor_index_begin = plan.resolved_anchor_index_begin;
  manifest.resolved_anchor_index_end = plan.resolved_anchor_index_end;
  manifest.accepted_anchor_count = plan.accepted_anchor_count;
  manifest.candidate_anchor_count = plan.candidate_anchor_count;
  manifest.accepted_anchor_fraction = plan.accepted_anchor_fraction;
  manifest.source_cursor_token = plan.source_cursor_token;
  manifest.first_anchor_key = plan.first_anchor_key;
  manifest.last_anchor_key = plan.last_anchor_key;
  manifest.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  manifest.node_ids = graph.node_ids;
  manifest.edge_ids = graph.edge_ids;
  return manifest;
}

void test_replay_source_graph_anchor_binding() {
  const auto graph = make_source_graph();
  const auto cursor = make_source_cursor(graph);
  auto spec = make_source_episode_spec(graph, cursor);
  const auto batch = make_source_edge_batch(graph, cursor);

  env::validate_episode_spec(spec);
  check(spec.accepted_range.anchor_keys[0] == "1000",
        "replay source stringifies accepted anchor keys");
  check(!spec.accepted_range.cursor.batch_cursor_token.empty(),
        "replay source carries component-stream cursor token");

  auto nonmonotonic_cursor = cursor;
  nonmonotonic_cursor.anchor_keys = {2000, 1000};
  bool rejected_nonmonotonic_cursor = false;
  try {
    (void)replay::bind_episode_spec_to_graph_cursor(
        make_source_episode_spec(graph, cursor), make_source_wave(),
        nonmonotonic_cursor);
  } catch (const std::exception &) {
    rejected_nonmonotonic_cursor = true;
  }
  check(rejected_nonmonotonic_cursor,
        "replay source rejects nonmonotonic accepted anchor keys");

  auto source_key_base_spec = make_episode_spec();
  source_key_base_spec.graph_order_fingerprint =
      graph.computed_graph_order_fingerprint();
  source_key_base_spec.graph_node_ids = graph.node_ids;
  source_key_base_spec.risky_node_ids = {"BTC", "ETH"};
  source_key_base_spec.base_policy = {.accounting_numeraire_id = "USDT",
                                      .settlement_asset_id = "USDT",
                                      .reserve_asset_id = "USDT",
                                      .projection_reference_node_id = "USDT"};
  source_key_base_spec.require_projection_validation = false;
  const auto source_key_spec = replay::bind_episode_spec_to_graph_cursor(
      source_key_base_spec, make_source_key_wave(), cursor);
  env::validate_episode_spec(source_key_spec);
  check(!source_key_spec.requested_range.anchor_index_begin.has_value() &&
            !source_key_spec.requested_range.anchor_index_end.has_value() &&
            source_key_spec.requested_range.source_key_begin == 1000 &&
            source_key_spec.requested_range.source_key_end == 3000 &&
            source_key_spec.accepted_range.anchor_index_begin == 10 &&
            source_key_spec.accepted_range.anchor_index_end == 12 &&
            source_key_spec.accepted_range.cursor.source_range_policy ==
                "source_key",
        "replay source preserves source-key request separately from accepted "
        "anchor cursor");

  const auto realized =
      replay::realized_log_returns_from_graph_anchor_edge_batch(batch, graph,
                                                                spec);
  close(realized.index({0, 0}).item<double>(), 0.02, 1e-12,
        "replay source forward direct edge return");
  close(realized.index({0, 1}).item<double>(), 0.04, 1e-12,
        "replay source reverse direct edge return");
  close(realized.index({1, 0}).item<double>(), -0.01, 1e-12,
        "replay source second anchor forward return");
  close(realized.index({1, 1}).item<double>(), -0.03, 1e-12,
        "replay source second anchor reverse return");

  auto mismatched_anchor_tensor_batch = batch;
  mismatched_anchor_tensor_batch.anchor_keys =
      torch::tensor({1000, 3000}, torch::kInt64);
  bool rejected_mismatched_anchor_tensor = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        mismatched_anchor_tensor_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_mismatched_anchor_tensor = true;
  }
  check(rejected_mismatched_anchor_tensor,
        "replay source rejects edge-batch anchor tensor drift");

  auto float_anchor_tensor_batch = batch;
  float_anchor_tensor_batch.anchor_keys =
      torch::tensor({1000.0, 2000.0}, torch::kFloat64);
  bool rejected_float_anchor_tensor = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        float_anchor_tensor_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_float_anchor_tensor = true;
  }
  check(rejected_float_anchor_tensor,
        "replay source rejects non-integral anchor key tensors");

  auto mismatched_cursor_token_batch = batch;
  mismatched_cursor_token_batch.cursor.begin_anchor_index = 9;
  bool rejected_mismatched_cursor_token = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        mismatched_cursor_token_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_mismatched_cursor_token = true;
  }
  check(rejected_mismatched_cursor_token,
        "replay source rejects edge-batch cursor identity drift");

  auto missing_direct_edge_batch = batch;
  missing_direct_edge_batch.edge_present = batch.edge_present.clone();
  missing_direct_edge_batch.edge_present.index_put_({0, 0}, false);
  bool rejected_absent_direct_edge = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        missing_direct_edge_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_absent_direct_edge = true;
  }
  check(rejected_absent_direct_edge,
        "replay source rejects absent direct projection edges");

  auto float_edge_present_batch = batch;
  float_edge_present_batch.edge_present =
      batch.edge_present.to(torch::kFloat64);
  bool rejected_float_edge_present = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        float_edge_present_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_float_edge_present = true;
  }
  check(rejected_float_edge_present,
        "replay source rejects non-bool edge-present tensors");

  auto mixed_realization_key_batch = batch;
  mixed_realization_key_batch.future_keys = batch.future_keys.clone();
  mixed_realization_key_batch.future_keys.index_put_({0, 1, 0, 0}, 1011);
  bool rejected_mixed_realization_keys = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        mixed_realization_key_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_mixed_realization_keys = true;
  }
  check(rejected_mixed_realization_keys,
        "replay source rejects mixed realization keys inside one frame");

  replay::replay_frame_build_options_t selected_realization_channel_options{};
  selected_realization_channel_options.realization_channel_index = 1;
  const auto selected_channel_realized =
      replay::realized_log_returns_from_graph_anchor_edge_batch(
          mixed_realization_key_batch, graph, spec,
          selected_realization_channel_options);
  close(selected_channel_realized.index({0, 0}).item<double>(), 0.03, 1e-12,
        "replay source can select a realization channel for direct returns");
  close(selected_channel_realized.index({0, 1}).item<double>(), 0.03, 1e-12,
        "replay source selected channel preserves reverse-edge sign");

  auto float_future_key_batch = batch;
  float_future_key_batch.future_keys = batch.future_keys.to(torch::kFloat64);
  bool rejected_float_future_keys = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        float_future_key_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_float_future_keys = true;
  }
  check(rejected_float_future_keys,
        "replay source rejects non-integral future key tensors");

  auto float_future_mask_batch = batch;
  float_future_mask_batch.future_mask = batch.future_mask.to(torch::kFloat64);
  bool rejected_float_future_mask = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        float_future_mask_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_float_future_mask = true;
  }
  check(rejected_float_future_mask,
        "replay source rejects non-bool future masks");

  auto int_future_features_batch = batch;
  int_future_features_batch.future_features =
      batch.future_features.to(torch::kInt64);
  bool rejected_int_future_features = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        int_future_features_batch, graph, spec);
  } catch (const std::exception &) {
    rejected_int_future_features = true;
  }
  check(rejected_int_future_features,
        "replay source rejects non-floating future feature tensors");

  const auto frames = replay::make_replay_frames_from_graph_anchor_edge_batch(
      batch, graph, spec);
  check(frames.size() == 2, "replay source builds one frame per anchor");
  check(frames[0].observation.anchor_key == "1000",
        "replay source frame anchor key");
  check(frames[0].observation.timestamp_ms == 1000,
        "replay source frame knowledge timestamp");
  check(frames[0].observation.realization_available_after_timestamp_ms == 1010,
        "replay source frame realization timestamp");
  env::validate_observation_time_boundary(frames[0].observation);

  replay::replay_world_options_t options{};
  options.require_projection_validation = false;
  options.paper_execution_options.allow_synthetic_direct_edges = true;
  auto projection_optional_spec = spec;
  projection_optional_spec.require_projection_validation = false;
  replay::replay_world_t world(frames, options);
  auto observation = world.reset(projection_optional_spec);
  check(observation.anchor_key == "1000",
        "replay source frames reset in replay world");
  auto first = world.step(make_action());
  check(!first.done, "replay source first transition not done");
  check(first.info.realized_log_growth != 0.0,
        "replay source transition computes realized growth");

  auto source_key_bundle =
      replay::make_replay_episode_bundle_from_graph_anchor_edge_batch(
          source_key_base_spec, make_source_key_wave(), cursor, batch, graph,
          replay::replay_frame_build_options_t{}, options);
  auto source_key_world = replay::spawn_replay_world(source_key_bundle);
  env::base_reserve_policy_t source_key_policy(
      make_baseline_config_for_bundle("source_key_reserve", source_key_bundle));
  auto source_key_report = env::run_episode(
      *source_key_world, source_key_policy, source_key_bundle.spec);
  check(source_key_report.requested_anchor_index_begin == -1 &&
            source_key_report.requested_anchor_index_end == -1 &&
            source_key_report.requested_source_key_begin == 1000 &&
            source_key_report.requested_source_key_end == 3000 &&
            source_key_report.accepted_anchor_index_begin == 10 &&
            source_key_report.accepted_anchor_index_end == 12 &&
            source_key_report.accepted_source_range_policy == "source_key",
        "replay report keeps source-key request and accepted cursor evidence "
        "separate");

  auto source_key_plan = make_runtime_wave_plan(cursor);
  source_key_plan.source_range_policy = "source_key";
  source_key_plan.requested_anchor_index_begin.reset();
  source_key_plan.requested_anchor_index_end.reset();
  source_key_plan.requested_source_key_begin = 1000;
  source_key_plan.requested_source_key_end = 3000;
  auto batch_plan = replay::wave_plan_for_batch_cursor(source_key_plan, cursor);
  check(!batch_plan.requested_anchor_index_begin.has_value() &&
            !batch_plan.requested_anchor_index_end.has_value() &&
            batch_plan.requested_source_key_begin == 1000 &&
            batch_plan.requested_source_key_end == 3000 &&
            batch_plan.resolved_anchor_index_begin == 10 &&
            batch_plan.resolved_anchor_index_end == 12,
        "runtime replay batch cursor preserves source-key requested range");

  auto source_key_manifest = make_runtime_manifest(graph, source_key_plan);
  source_key_manifest.source_range_policy = "source_key";
  source_key_manifest.requested_source_key_begin = "1000";
  source_key_manifest.requested_source_key_end = "3000";
  const auto manifest_plan =
      replay::wave_plan_from_runtime_job_manifest(source_key_manifest);
  check(!manifest_plan.requested_anchor_index_begin.has_value() &&
            !manifest_plan.requested_anchor_index_end.has_value() &&
            manifest_plan.requested_source_key_begin == 1000 &&
            manifest_plan.requested_source_key_end == 3000 &&
            manifest_plan.resolved_anchor_index_begin == 10 &&
            manifest_plan.resolved_anchor_index_end == 12,
        "runtime replay manifest reader preserves source-key request");
  const auto manifest_wave =
      replay::component_stream_wave_from_runtime_wave_plan(manifest_plan);
  const auto manifest_source_key_spec =
      replay::bind_episode_spec_to_graph_cursor(source_key_base_spec,
                                                manifest_wave, cursor);
  env::validate_episode_spec(manifest_source_key_spec);
  check(!manifest_source_key_spec.requested_range.anchor_index_begin
                .has_value() &&
            manifest_source_key_spec.requested_range.source_key_begin == 1000 &&
            manifest_source_key_spec.accepted_range.anchor_index_begin == 10,
        "runtime replay manifest wave preserves source-key request through "
        "EpisodeSpec binding");

  auto plan = replay::make_direct_edge_projection_plan(graph, spec);
  check(plan.edge_ids[0] == "BTCUSDT",
        "replay source projection records forward edge");
  check(plan.edge_ids[1] == "USDTETH",
        "replay source projection records reverse edge");
  close(plan.signs.index({0}).item<double>(), 1.0, 1e-12,
        "replay source projection forward sign");
  close(plan.signs.index({1}).item<double>(), -1.0, 1e-12,
        "replay source projection reverse sign");

  replay::replay_world_options_t bundle_world_options{};
  bundle_world_options.require_projection_validation = false;
  bundle_world_options.paper_execution_options.allow_synthetic_direct_edges =
      true;
  auto projection_optional_bundle_spec = spec;
  projection_optional_bundle_spec.require_projection_validation = false;
  auto bundle = replay::make_replay_episode_bundle_from_graph_anchor_edge_batch(
      projection_optional_bundle_spec, make_source_wave(), cursor, batch, graph,
      replay::replay_frame_build_options_t{}, bundle_world_options);
  check(bundle.frame_count() == 2,
        "replay source bundle carries expected frame count");
  check(bundle.spec.realized_return_projection.truth_id ==
                "direct_edge_realized_return_truth_v1" &&
            bundle.spec.realized_return_projection.reference_node_id ==
                "USDT" &&
            bundle.spec.realized_return_projection.edge_ids[0] == "BTCUSDT" &&
            bundle.spec.realized_return_projection.edge_ids[1] == "USDTETH",
        "replay source bundle records realized-return truth and projection "
        "edges in EpisodeSpec");
  close(bundle.spec.realized_return_projection.signs.index({0}).item<double>(),
        1.0, 1e-12,
        "replay source bundle records forward realized-return sign");
  close(bundle.spec.realized_return_projection.signs.index({1}).item<double>(),
        -1.0, 1e-12,
        "replay source bundle records reverse realized-return sign");

  auto bad_projection_spec = bundle.spec;
  bad_projection_spec.realized_return_projection.edge_ids.pop_back();
  bool rejected_bad_projection_spec = false;
  try {
    env::validate_episode_spec(bad_projection_spec);
  } catch (const std::exception &) {
    rejected_bad_projection_spec = true;
  }
  check(rejected_bad_projection_spec,
        "environment rejects malformed realized-return projection evidence in "
        "EpisodeSpec");

  auto bad_projection_truth_spec = bundle.spec;
  bad_projection_truth_spec.realized_return_projection.truth_id =
      "fixture_unknown_truth";
  bool rejected_bad_projection_truth_spec = false;
  try {
    env::validate_episode_spec(bad_projection_truth_spec);
  } catch (const std::exception &) {
    rejected_bad_projection_truth_spec = true;
  }
  check(rejected_bad_projection_truth_spec,
        "environment rejects unknown realized-return truth ids in EpisodeSpec");

  auto bad_frame_cursor_bundle = bundle;
  bad_frame_cursor_bundle.frames[0].observation.anchor_key = "9999";
  bool rejected_bad_frame_cursor_bundle = false;
  try {
    replay::validate_replay_episode_bundle(bad_frame_cursor_bundle);
  } catch (const std::exception &) {
    rejected_bad_frame_cursor_bundle = true;
  }
  check(rejected_bad_frame_cursor_bundle,
        "replay bundle rejects frame cursor identity drift");

  auto bad_frame_sequence_bundle = bundle;
  bad_frame_sequence_bundle.frames[1].observation.knowledge_timestamp_ms =
      bad_frame_sequence_bundle.frames[0]
          .observation.realization_available_after_timestamp_ms -
      1;
  bool rejected_bad_frame_sequence_bundle = false;
  try {
    replay::validate_replay_episode_bundle(bad_frame_sequence_bundle);
  } catch (const std::exception &) {
    rejected_bad_frame_sequence_bundle = true;
  }
  check(rejected_bad_frame_sequence_bundle,
        "replay bundle rejects frame sequence time leakage");

  auto bad_frame_return_bundle = bundle;
  bad_frame_return_bundle.frames[0].realized_log_return =
      torch::tensor({0.01}, torch::kFloat64);
  bool rejected_bad_frame_return_bundle = false;
  try {
    replay::validate_replay_episode_bundle(bad_frame_return_bundle);
  } catch (const std::exception &) {
    rejected_bad_frame_return_bundle = true;
  }
  check(rejected_bad_frame_return_bundle,
        "replay bundle rejects malformed frame realized returns");

  auto integer_return_bundle = bundle;
  integer_return_bundle.frames[0].realized_log_return =
      torch::tensor({0, 0}, torch::kInt64);
  bool rejected_integer_return_bundle = false;
  try {
    replay::validate_replay_episode_bundle(integer_return_bundle);
  } catch (const std::exception &) {
    rejected_integer_return_bundle = true;
  }
  check(rejected_integer_return_bundle,
        "replay bundle rejects non-floating realized returns");

  auto mask_without_projection_bundle = bundle;
  mask_without_projection_bundle.frames[0].active_projection_mask =
      torch::ones({2}, torch::kBool);
  bool rejected_mask_without_projection_bundle = false;
  try {
    replay::validate_replay_episode_bundle(mask_without_projection_bundle);
  } catch (const std::exception &) {
    rejected_mask_without_projection_bundle = true;
  }
  check(rejected_mask_without_projection_bundle,
        "replay bundle rejects projection masks without scenarios");

  auto mismatched_projection_bundle = bundle;
  mismatched_projection_bundle.spec.require_projection_validation = true;
  bool rejected_projection_bundle_mismatch = false;
  try {
    replay::validate_replay_episode_bundle(mismatched_projection_bundle);
  } catch (const std::exception &) {
    rejected_projection_bundle_mismatch = true;
  }
  check(rejected_projection_bundle_mismatch,
        "replay bundle rejects disabled projection validation when "
        "EpisodeSpec requires it");

  auto missing_projection_payload_bundle = bundle;
  missing_projection_payload_bundle.spec.require_projection_validation = true;
  missing_projection_payload_bundle.world_options
      .require_projection_validation = true;
  replay::validate_replay_episode_bundle(missing_projection_payload_bundle);
  bool rejected_missing_projection_payload_ready_bundle = false;
  try {
    replay::validate_replay_episode_bundle_ready_for_world(
        missing_projection_payload_bundle);
  } catch (const std::exception &) {
    rejected_missing_projection_payload_ready_bundle = true;
  }
  check(rejected_missing_projection_payload_ready_bundle,
        "replay ready-for-world validation rejects missing projection "
        "scenarios");
  bool rejected_missing_projection_payload_spawn = false;
  try {
    (void)replay::spawn_replay_world(missing_projection_payload_bundle);
  } catch (const std::exception &) {
    rejected_missing_projection_payload_spawn = true;
  }
  check(rejected_missing_projection_payload_spawn,
        "replay world spawn rejects projection-required bundles without "
        "projection scenarios");

  auto incomplete_policy_constructed = std::make_shared<bool>(false);
  std::vector<env::replay_policy_factory_t> incomplete_policy_factories;
  incomplete_policy_factories.push_back({
      .policy_id = "incomplete_bundle_policy_fixture",
      .policy_kind = env::policy_kind_t::baseline,
      .make_policy =
          [incomplete_policy_constructed](
              const replay::replay_episode_bundle_t &task_bundle) {
            *incomplete_policy_constructed = true;
            return std::make_unique<env::base_reserve_policy_t>(
                make_baseline_config_for_bundle(
                    "incomplete_bundle_policy_fixture", task_bundle));
          },
  });
  env::replay_experiment_options_t incomplete_experiment_options{};
  incomplete_experiment_options.continue_on_failure = true;
  auto incomplete_experiment = env::run_replay_experiment(
      "incomplete_projection_payload_experiment",
      std::vector{missing_projection_payload_bundle},
      incomplete_policy_factories, incomplete_experiment_options);
  check(incomplete_experiment.attempted_count == 1 &&
            incomplete_experiment.completed_count == 0 &&
            incomplete_experiment.failures.size() == 1 &&
            !*incomplete_policy_constructed,
        "experiment runner rejects not-ready replay bundles before policy "
        "construction");

  auto bad_world_options_bundle = bundle;
  bad_world_options_bundle.world_options.linear_transaction_cost_rate = -0.01;
  bool rejected_bad_bundle_world_options = false;
  try {
    replay::validate_replay_episode_bundle(bad_world_options_bundle);
  } catch (const std::exception &) {
    rejected_bad_bundle_world_options = true;
  }
  check(rejected_bad_bundle_world_options,
        "replay bundle rejects malformed replay world options");

  auto bad_frame_options_bundle = bundle;
  bad_frame_options_bundle.frame_options.future_horizon_index = -1;
  bool rejected_bad_frame_options = false;
  try {
    replay::validate_replay_episode_bundle(bad_frame_options_bundle);
  } catch (const std::exception &) {
    rejected_bad_frame_options = true;
  }
  check(rejected_bad_frame_options,
        "replay bundle rejects malformed frame build options");

  auto bad_projection_sign_bundle = bundle;
  bad_projection_sign_bundle.realized_return_projection.signs =
      torch::tensor({0.0, -1.0}, torch::kFloat64);
  bool rejected_bad_projection_sign = false;
  try {
    replay::validate_replay_episode_bundle(bad_projection_sign_bundle);
  } catch (const std::exception &) {
    rejected_bad_projection_sign = true;
  }
  check(rejected_bad_projection_sign,
        "replay bundle rejects invalid projection orientation signs");

  auto empty_projection_edge_bundle = bundle;
  empty_projection_edge_bundle.realized_return_projection.edge_ids[0].clear();
  bool rejected_empty_projection_edge = false;
  try {
    replay::validate_replay_episode_bundle(empty_projection_edge_bundle);
  } catch (const std::exception &) {
    rejected_empty_projection_edge = true;
  }
  check(rejected_empty_projection_edge,
        "replay bundle rejects empty projection edge ids");

  auto duplicate_projection_edge_bundle = bundle;
  duplicate_projection_edge_bundle.realized_return_projection.edge_ids[1] =
      duplicate_projection_edge_bundle.realized_return_projection.edge_ids[0];
  bool rejected_duplicate_projection_edge = false;
  try {
    replay::validate_replay_episode_bundle(duplicate_projection_edge_bundle);
  } catch (const std::exception &) {
    rejected_duplicate_projection_edge = true;
  }
  check(rejected_duplicate_projection_edge,
        "replay bundle rejects duplicate projection edge ids");

  close(bundle.realized_return_projection.signs.index({1}).item<double>(), -1.0,
        1e-12, "replay source bundle carries reverse sign");
  auto spawned_world = replay::spawn_replay_world(bundle);
  env::equal_weight_policy_t policy(make_baseline_config("equal_weight"), 0.50);
  auto report = env::run_episode(*spawned_world, policy, bundle.spec);
  check(report.transition_count == 2,
        "replay source spawned world runs through episode");
  check(report.realized_return_truth_id ==
                "direct_edge_realized_return_truth_v1" &&
            report.realized_return_projection_reference_node_id == "USDT" &&
            report.realized_return_projection_edge_ids == "BTCUSDT,USDTETH" &&
            report.realized_return_projection_signs == "BTC:1,ETH:-1",
        "replay report exposes realized-return truth and projection edge "
        "evidence");

  auto enriched_bundle = bundle;
  enriched_bundle.world_options.require_projection_validation = true;
  auto artifacts = make_source_observation_artifacts(enriched_bundle, graph);
  artifacts[0].nodelift_potential_belief =
      make_source_nodelift_potential_belief(
          enriched_bundle.frames[0].observation.anchor_key,
          enriched_bundle.frames[0].observation.timestamp_ms, graph);
  auto artifact_edge_market = make_edge_market_state();
  artifact_edge_market.edge_mid_price = torch::tensor(
      {101.0, 51.0}, torch::TensorOptions().dtype(torch::kFloat64));
  env::execution::validate_spot_edge_market_state(artifact_edge_market);
  artifacts[0].edge_market_state = artifact_edge_market;
  replay::keyed_replay_observation_artifact_source_t artifact_source(
      "source_artifact_fixture", artifacts);
  replay::enrich_replay_episode_bundle_with_artifacts(
      enriched_bundle, artifact_source, {.require_artifacts_per_frame = true});
  check(enriched_bundle.frames[0].observation.allocation_belief.has_value(),
        "replay artifact source attaches AllocationBelief");
  check(enriched_bundle.frames[0]
            .observation.nodelift_potential_belief.has_value(),
        "replay artifact source attaches NodeLiftPotentialBelief");
  check(enriched_bundle.frames[0].observation.mdn_artifact.has_value(),
        "replay artifact source attaches MDN artifact identity");
  check(enriched_bundle.frames[0].projected_log_return_scenarios.defined(),
        "replay artifact source attaches projected return scenarios");
  close(enriched_bundle.frames[0]
            .observation.edge_market_state.edge_mid_price.index({0})
            .item<double>(),
        101.0, 1e-12,
        "replay artifact source attaches validated edge market state");

  auto bad_artifacts = make_source_observation_artifacts(bundle, graph);
  bad_artifacts[0].artifact_timestamp_ms =
      bundle.frames[0].observation.knowledge_timestamp_ms + 1;
  replay::keyed_replay_observation_artifact_source_t bad_artifact_source(
      "bad_source_artifact_fixture", bad_artifacts);
  auto bad_enriched_bundle = bundle;
  bool rejected_future_artifact = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_enriched_bundle, bad_artifact_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_future_artifact = true;
  }
  check(rejected_future_artifact,
        "replay artifact source rejects future-dated artifacts");

  auto bad_nodelift_anchor_artifacts =
      make_source_observation_artifacts(bundle, graph);
  bad_nodelift_anchor_artifacts[0].nodelift_potential_belief =
      make_source_nodelift_potential_belief(
          bundle.frames[0].observation.anchor_key + "_wrong",
          bundle.frames[0].observation.timestamp_ms, graph);
  replay::keyed_replay_observation_artifact_source_t bad_nodelift_anchor_source(
      "bad_nodelift_anchor_fixture", bad_nodelift_anchor_artifacts);
  auto bad_nodelift_anchor_bundle = bundle;
  bool rejected_bad_nodelift_anchor = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_nodelift_anchor_bundle, bad_nodelift_anchor_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_nodelift_anchor = true;
  }
  check(rejected_bad_nodelift_anchor,
        "replay artifact source rejects wrong-anchor NodeLiftPotentialBelief");

  auto bad_nodelift_graph_artifacts =
      make_source_observation_artifacts(bundle, graph);
  auto bad_nodelift_graph_belief = make_source_nodelift_potential_belief(
      bundle.frames[0].observation.anchor_key,
      bundle.frames[0].observation.timestamp_ms, graph);
  bad_nodelift_graph_belief.graph_node_ids = {"ETH", "BTC", "USDT"};
  bad_nodelift_graph_artifacts[0].nodelift_potential_belief =
      std::move(bad_nodelift_graph_belief);
  replay::keyed_replay_observation_artifact_source_t bad_nodelift_graph_source(
      "bad_nodelift_graph_fixture", bad_nodelift_graph_artifacts);
  auto bad_nodelift_graph_bundle = bundle;
  bool rejected_bad_nodelift_graph = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_nodelift_graph_bundle, bad_nodelift_graph_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_nodelift_graph = true;
  }
  check(rejected_bad_nodelift_graph,
        "replay artifact source rejects wrong-graph NodeLiftPotentialBelief");

  auto bad_nodelift_axis_artifacts =
      make_source_observation_artifacts(bundle, graph);
  auto bad_nodelift_axis_belief = make_source_nodelift_potential_belief(
      bundle.frames[0].observation.anchor_key,
      bundle.frames[0].observation.timestamp_ms, graph);
  bad_nodelift_axis_belief.graph_node_axis.node_ids = {"BTC", "USDT", "ETH"};
  bad_nodelift_axis_artifacts[0].nodelift_potential_belief =
      std::move(bad_nodelift_axis_belief);
  replay::keyed_replay_observation_artifact_source_t bad_nodelift_axis_source(
      "bad_nodelift_axis_fixture", bad_nodelift_axis_artifacts);
  auto bad_nodelift_axis_bundle = bundle;
  bool rejected_bad_nodelift_axis = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_nodelift_axis_bundle, bad_nodelift_axis_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_nodelift_axis = true;
  }
  check(rejected_bad_nodelift_axis,
        "replay artifact source rejects wrong-dock NodeLiftPotentialBelief");

  auto bad_projection_mask_artifacts =
      make_source_observation_artifacts(bundle, graph);
  bad_projection_mask_artifacts[0].active_projection_mask =
      torch::ones({2}, torch::kFloat64);
  replay::keyed_replay_observation_artifact_source_t bad_projection_mask_source(
      "bad_projection_mask_fixture", bad_projection_mask_artifacts);
  auto bad_projection_mask_bundle = bundle;
  bool rejected_bad_projection_mask = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_projection_mask_bundle, bad_projection_mask_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_projection_mask = true;
  }
  check(rejected_bad_projection_mask,
        "replay artifact source rejects non-bool projection masks");

  auto int_projection_scenario_artifacts =
      make_source_observation_artifacts(bundle, graph);
  int_projection_scenario_artifacts[0].projected_log_return_scenarios =
      int_projection_scenario_artifacts[0].projected_log_return_scenarios.to(
          torch::kInt64);
  replay::keyed_replay_observation_artifact_source_t
      int_projection_scenario_source("int_projection_scenario_fixture",
                                     int_projection_scenario_artifacts);
  auto int_projection_scenario_bundle = bundle;
  bool rejected_int_projection_scenarios = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        int_projection_scenario_bundle, int_projection_scenario_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_int_projection_scenarios = true;
  }
  check(rejected_int_projection_scenarios,
        "replay artifact source rejects non-floating projection scenarios");

  auto inactive_projection_mask_artifacts =
      make_source_observation_artifacts(bundle, graph);
  inactive_projection_mask_artifacts[0].active_projection_mask =
      torch::zeros({2}, torch::kBool);
  replay::keyed_replay_observation_artifact_source_t
      inactive_projection_mask_source("inactive_projection_mask_fixture",
                                      inactive_projection_mask_artifacts);
  auto inactive_projection_mask_bundle = bundle;
  bool rejected_inactive_projection_mask = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        inactive_projection_mask_bundle, inactive_projection_mask_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_inactive_projection_mask = true;
  }
  check(rejected_inactive_projection_mask,
        "replay artifact source rejects inactive projection masks");

  auto missing_residual_energy_artifacts =
      make_source_observation_artifacts(bundle, graph);
  missing_residual_energy_artifacts[0].nodelift_residual_energy =
      torch::Tensor{};
  missing_residual_energy_artifacts[0].nodelift_residual_mask =
      torch::ones({2, 9}, torch::kBool);
  replay::keyed_replay_observation_artifact_source_t
      missing_residual_energy_source("missing_residual_energy_fixture",
                                     missing_residual_energy_artifacts);
  auto missing_residual_energy_bundle = bundle;
  bool rejected_missing_residual_energy = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        missing_residual_energy_bundle, missing_residual_energy_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_missing_residual_energy = true;
  }
  check(rejected_missing_residual_energy,
        "replay artifact source rejects residual masks without energy");

  auto int_residual_energy_artifacts =
      make_source_observation_artifacts(bundle, graph);
  int_residual_energy_artifacts[0].nodelift_residual_energy =
      int_residual_energy_artifacts[0].nodelift_residual_energy.to(
          torch::kInt64);
  replay::keyed_replay_observation_artifact_source_t int_residual_energy_source(
      "int_residual_energy_fixture", int_residual_energy_artifacts);
  auto int_residual_energy_bundle = bundle;
  bool rejected_int_residual_energy = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        int_residual_energy_bundle, int_residual_energy_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_int_residual_energy = true;
  }
  check(rejected_int_residual_energy,
        "replay artifact source rejects non-floating residual energy");

  auto bad_residual_mask_artifacts =
      make_source_observation_artifacts(bundle, graph);
  bad_residual_mask_artifacts[0].nodelift_residual_mask =
      torch::ones({2, 9}, torch::kFloat64);
  replay::keyed_replay_observation_artifact_source_t bad_residual_mask_source(
      "bad_residual_mask_fixture", bad_residual_mask_artifacts);
  auto bad_residual_mask_bundle = bundle;
  bool rejected_bad_residual_mask = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_residual_mask_bundle, bad_residual_mask_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_residual_mask = true;
  }
  check(rejected_bad_residual_mask,
        "replay artifact source rejects non-bool residual masks");

  auto future_market_artifacts =
      make_source_observation_artifacts(bundle, graph);
  future_market_artifacts[0].market_state =
      bundle.frames[0].observation.market_state;
  future_market_artifacts[0].market_state->timestamp_ms =
      bundle.frames[0].observation.knowledge_timestamp_ms + 1;
  replay::keyed_replay_observation_artifact_source_t future_market_source(
      "future_market_artifact_fixture", future_market_artifacts);
  auto future_market_bundle = bundle;
  bool rejected_future_market_artifact = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        future_market_bundle, future_market_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_future_market_artifact = true;
  }
  check(rejected_future_market_artifact,
        "replay artifact source rejects future-dated market state");

  auto bad_edge_artifacts = make_source_observation_artifacts(bundle, graph);
  bad_edge_artifacts[0].edge_market_state = make_edge_market_state();
  bad_edge_artifacts[0].edge_market_state->edge_mid_price =
      torch::tensor({101.0}, torch::TensorOptions().dtype(torch::kFloat64));
  replay::keyed_replay_observation_artifact_source_t bad_edge_artifact_source(
      "bad_edge_market_artifact_fixture", bad_edge_artifacts);
  auto bad_edge_enriched_bundle = bundle;
  bool rejected_bad_edge_market = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        bad_edge_enriched_bundle, bad_edge_artifact_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_bad_edge_market = true;
  }
  check(rejected_bad_edge_market,
        "replay artifact source rejects malformed edge market state");

  auto missing_reserve_edge_artifacts =
      make_source_observation_artifacts(bundle, graph);
  graph::market_graph_t missing_reserve_graph{};
  missing_reserve_graph.node_ids = {"BTC", "ETH", "USD"};
  missing_reserve_graph.edge_ids = {"BTCUSD", "ETHUSD"};
  missing_reserve_graph.base_index = {0, 1};
  missing_reserve_graph.quote_index = {2, 2};
  missing_reserve_graph.validate();
  env::execution::spot_edge_market_state_t missing_reserve_market{};
  missing_reserve_market.graph = std::move(missing_reserve_graph);
  missing_reserve_market.edge_mid_price = torch::tensor(
      {100.0, 50.0}, torch::TensorOptions().dtype(torch::kFloat64));
  env::execution::validate_spot_edge_market_state(missing_reserve_market);
  missing_reserve_edge_artifacts[0].edge_market_state =
      std::move(missing_reserve_market);
  replay::keyed_replay_observation_artifact_source_t
      missing_reserve_edge_artifact_source(
          "missing_reserve_edge_market_artifact_fixture",
          missing_reserve_edge_artifacts);
  auto missing_reserve_edge_enriched_bundle = bundle;
  bool rejected_missing_reserve_edge_market = false;
  try {
    replay::enrich_replay_episode_bundle_with_artifacts(
        missing_reserve_edge_enriched_bundle,
        missing_reserve_edge_artifact_source,
        {.require_artifacts_per_frame = true});
  } catch (const std::exception &) {
    rejected_missing_reserve_edge_market = true;
  }
  check(
      rejected_missing_reserve_edge_market,
      "replay artifact source rejects edge market state without base reserve");

  const auto bad_artifact_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_bad_artifact_index_fixture";
  std::filesystem::remove_all(bad_artifact_index_root);
  std::filesystem::create_directories(bad_artifact_index_root);
  bool rejected_empty_artifact_index = false;
  try {
    replay::write_replay_observation_artifact_index(
        bad_artifact_index_root / "empty.index", {});
  } catch (const std::exception &) {
    rejected_empty_artifact_index = true;
  }
  check(rejected_empty_artifact_index,
        "replay artifact index rejects empty entry sets");

  const auto invalid_timestamp_write_path =
      bad_artifact_index_root / "invalid_timestamp_write.index";
  bool rejected_invalid_timestamp_artifact_index = false;
  try {
    replay::write_replay_observation_artifact_index(
        invalid_timestamp_write_path,
        {replay::replay_observation_artifact_index_entry_t{
            .anchor_key = "anchor_10",
            .observation_anchor_index = std::nullopt,
            .artifact_timestamp_ms = 0,
            .forecast_artifact_path = "a.forecast.pt"}});
  } catch (const std::exception &) {
    rejected_invalid_timestamp_artifact_index = true;
  }
  check(rejected_invalid_timestamp_artifact_index,
        "replay artifact index rejects nonpositive artifact timestamps");
  check(!std::filesystem::exists(invalid_timestamp_write_path),
        "replay artifact index validates invalid entries before writing");

  const auto parent_path_index_path =
      bad_artifact_index_root / "parent_path.index";
  bool rejected_parent_path_artifact_index = false;
  try {
    replay::write_replay_observation_artifact_index(
        parent_path_index_path,
        {replay::replay_observation_artifact_index_entry_t{
            .anchor_key = "anchor_10",
            .observation_anchor_index = std::nullopt,
            .artifact_timestamp_ms = 10,
            .forecast_artifact_path =
                std::filesystem::path("..") / "escape.forecast.pt"}});
  } catch (const std::exception &) {
    rejected_parent_path_artifact_index = true;
  }
  check(rejected_parent_path_artifact_index,
        "replay artifact index rejects parent-directory forecast paths");
  check(!std::filesystem::exists(parent_path_index_path),
        "replay artifact index validates parent paths before writing");

  const auto parent_path_read_index_path =
      bad_artifact_index_root / "parent_path_read.index";
  {
    std::ofstream parent_path_read_index(parent_path_read_index_path,
                                         std::ios::trunc);
    parent_path_read_index << "schema="
                           << replay::kReplayObservationArtifactIndexSchema
                           << "\n";
    parent_path_read_index
        << "entry|anchor_key=anchor_10|artifact_timestamp_ms=10"
        << "|forecast_artifact_path=../escape.forecast.pt\n";
  }
  bool rejected_parent_path_read_artifact_index = false;
  try {
    (void)replay::read_replay_observation_artifact_index(
        parent_path_read_index_path);
  } catch (const std::exception &) {
    rejected_parent_path_read_artifact_index = true;
  }
  check(rejected_parent_path_read_artifact_index,
        "replay artifact index rejects persisted parent-directory forecast "
        "paths");

  const auto parent_mdn_path_index_path =
      bad_artifact_index_root / "parent_mdn_path.index";
  bool rejected_parent_mdn_path_artifact_index = false;
  try {
    replay::write_replay_observation_artifact_index(
        parent_mdn_path_index_path,
        {replay::replay_observation_artifact_index_entry_t{
            .anchor_key = "anchor_10",
            .observation_anchor_index = std::nullopt,
            .artifact_timestamp_ms = 10,
            .forecast_artifact_path = "a.forecast.pt",
            .mdn_artifact =
                env::artifact_ref_t{
                    .artifact_id = "mdn_escape",
                    .artifact_path = "../escape_mdn.pt",
                    .artifact_digest = "digest_escape",
                    .schema_id = "wikimyei.inference.expected_value.mdn.v1",
                },
        }});
  } catch (const std::exception &) {
    rejected_parent_mdn_path_artifact_index = true;
  }
  check(rejected_parent_mdn_path_artifact_index,
        "replay artifact index rejects parent-directory MDN artifact refs");
  check(!std::filesystem::exists(parent_mdn_path_index_path),
        "replay artifact index validates MDN artifact refs before writing");

  const auto parent_mdn_path_read_index_path =
      bad_artifact_index_root / "parent_mdn_path_read.index";
  {
    std::ofstream parent_mdn_path_read_index(parent_mdn_path_read_index_path,
                                             std::ios::trunc);
    parent_mdn_path_read_index
        << "schema=" << replay::kReplayObservationArtifactIndexSchema << "\n";
    parent_mdn_path_read_index
        << "entry|anchor_key=anchor_10|artifact_timestamp_ms=10"
        << "|forecast_artifact_path=a.forecast.pt"
        << "|mdn_artifact_path=../escape_mdn.pt\n";
  }
  bool rejected_parent_mdn_path_read_artifact_index = false;
  try {
    (void)replay::read_replay_observation_artifact_index(
        parent_mdn_path_read_index_path);
  } catch (const std::exception &) {
    rejected_parent_mdn_path_read_artifact_index = true;
  }
  check(rejected_parent_mdn_path_read_artifact_index,
        "replay artifact index rejects persisted parent-directory MDN "
        "artifact refs");

  const auto duplicate_index_path = bad_artifact_index_root / "duplicate.index";
  {
    std::ofstream duplicate_index(duplicate_index_path, std::ios::trunc);
    duplicate_index << "schema="
                    << replay::kReplayObservationArtifactIndexSchema << "\n";
    duplicate_index << "entry|anchor_key=anchor_10|artifact_timestamp_ms=10"
                    << "|forecast_artifact_path=a.forecast.pt\n";
    duplicate_index << "entry|anchor_key=anchor_10|artifact_timestamp_ms=10"
                    << "|forecast_artifact_path=b.forecast.pt\n";
  }
  bool rejected_duplicate_artifact_index = false;
  try {
    (void)replay::read_replay_observation_artifact_index(duplicate_index_path);
  } catch (const std::exception &) {
    rejected_duplicate_artifact_index = true;
  }
  check(rejected_duplicate_artifact_index,
        "replay artifact index rejects duplicate anchor entries");

  const auto bad_anchor_index_path =
      bad_artifact_index_root / "bad_anchor_index.index";
  {
    std::ofstream bad_anchor_index(bad_anchor_index_path, std::ios::trunc);
    bad_anchor_index << "schema="
                     << replay::kReplayObservationArtifactIndexSchema << "\n";
    bad_anchor_index << "entry|anchor_key=anchor_11|artifact_timestamp_ms=10"
                     << "|forecast_artifact_path=a.forecast.pt"
                     << "|observation_anchor_index=-1\n";
  }
  bool rejected_bad_anchor_index = false;
  try {
    (void)replay::read_replay_observation_artifact_index(bad_anchor_index_path);
  } catch (const std::exception &) {
    rejected_bad_anchor_index = true;
  }
  check(rejected_bad_anchor_index,
        "replay artifact index rejects negative observation anchor indices");

  const auto malformed_timestamp_path =
      bad_artifact_index_root / "malformed_timestamp.index";
  {
    std::ofstream malformed_timestamp(malformed_timestamp_path,
                                      std::ios::trunc);
    malformed_timestamp << "schema="
                        << replay::kReplayObservationArtifactIndexSchema
                        << "\n";
    malformed_timestamp
        << "entry|anchor_key=anchor_12|artifact_timestamp_ms=not-a-time"
        << "|forecast_artifact_path=a.forecast.pt\n";
  }
  bool rejected_malformed_timestamp = false;
  try {
    (void)replay::read_replay_observation_artifact_index(
        malformed_timestamp_path);
  } catch (const std::exception &) {
    rejected_malformed_timestamp = true;
  }
  check(rejected_malformed_timestamp,
        "replay artifact index rejects malformed artifact timestamps");

  const auto forecast_identity_root =
      bad_artifact_index_root / "forecast_identity";
  std::filesystem::create_directories(forecast_identity_root / "forecast");
  const auto forecast_identity_path =
      forecast_identity_root / "forecast" / "anchor_1000.forecast.pt";
  replay::forecast::save_forecast_artifact(
      make_source_forecast_artifact(bundle.frames[0], graph),
      forecast_identity_path);

  const auto wrong_forecast_anchor_index_path =
      forecast_identity_root / "wrong_forecast_anchor.index";
  replay::write_replay_observation_artifact_index(
      wrong_forecast_anchor_index_path,
      {replay::replay_observation_artifact_index_entry_t{
          .anchor_key = "wrong_anchor",
          .observation_anchor_index =
              bundle.frames[0].observation.observation_anchor_index,
          .artifact_timestamp_ms =
              bundle.frames[0].observation.knowledge_timestamp_ms,
          .forecast_artifact_path =
              std::filesystem::path("forecast") / "anchor_1000.forecast.pt"}});
  bool rejected_forecast_anchor_mismatch = false;
  try {
    (void)replay::load_replay_observation_artifacts_from_index(
        wrong_forecast_anchor_index_path);
  } catch (const std::exception &) {
    rejected_forecast_anchor_mismatch = true;
  }
  check(rejected_forecast_anchor_mismatch,
        "replay artifact index rejects forecast identity anchor drift");

  const auto wrong_forecast_timestamp_index_path =
      forecast_identity_root / "wrong_forecast_timestamp.index";
  replay::write_replay_observation_artifact_index(
      wrong_forecast_timestamp_index_path,
      {replay::replay_observation_artifact_index_entry_t{
          .anchor_key = bundle.frames[0].observation.anchor_key,
          .observation_anchor_index =
              bundle.frames[0].observation.observation_anchor_index,
          .artifact_timestamp_ms =
              bundle.frames[0].observation.knowledge_timestamp_ms + 1,
          .forecast_artifact_path =
              std::filesystem::path("forecast") / "anchor_1000.forecast.pt"}});
  bool rejected_forecast_timestamp_mismatch = false;
  try {
    (void)replay::load_replay_observation_artifacts_from_index(
        wrong_forecast_timestamp_index_path);
  } catch (const std::exception &) {
    rejected_forecast_timestamp_mismatch = true;
  }
  check(rejected_forecast_timestamp_mismatch,
        "replay artifact index rejects forecast identity timestamp drift");

  auto enriched_world = replay::spawn_replay_world(enriched_bundle);
  env::spot_distributional_utility_policy_config_t allocation_config{};
  allocation_config.constraints.min_base_reserve_weight = 0.20;
  allocation_config.constraints.max_weight =
      torch::full({2}, 0.60, torch::kFloat64);
  allocation_config.constraints.max_turnover_l1 = 0.80;
  allocation_config.constraints.lambda_cvar = 0.10;
  allocation_config.constraints.lambda_concentration = 0.01;
  allocation_config.solver_options.iterations = 24;
  allocation_config.solver_options.learning_rate = 0.05;
  env::spot_distributional_utility_policy_t allocation_policy(
      allocation_config);
  auto allocation_report = env::run_episode(*enriched_world, allocation_policy,
                                            enriched_bundle.spec);
  check(allocation_report.policy_kind ==
            env::policy_kind_t::deterministic_allocator,
        "enriched replay runs deterministic Wikimyei allocation policy");
  check(allocation_report.transition_count == 2,
        "enriched replay allocation policy runs all frames");

  std::vector<env::replay_policy_factory_t> policy_factories;
  policy_factories.push_back({
      .policy_id = "base_reserve_only",
      .policy_kind = env::policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            return std::make_unique<env::base_reserve_policy_t>(
                make_baseline_config_for_bundle("base_reserve_only", bundle));
          },
  });
  policy_factories.push_back({
      .policy_id = "equal_weight",
      .policy_kind = env::policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            return std::make_unique<env::equal_weight_policy_t>(
                make_baseline_config_for_bundle("equal_weight", bundle),
                /*base_reserve_weight=*/0.50);
          },
  });
  env::replay_experiment_options_t experiment_options{};
  experiment_options.max_parallel_jobs = 2;
  auto experiment = env::run_replay_experiment(
      "source_bundle_baseline_compare", std::vector{enriched_bundle},
      policy_factories, experiment_options);
  check(experiment.experiment_artifact_schema_id ==
            std::string(env::kReplayExperimentArtifactSchema),
        "experiment runner emits replay audit experiment schema");
  check(experiment.attempted_count == 2,
        "experiment runner attempts bundle/policy product");
  check(experiment.requested_max_parallel_jobs == 2 &&
            experiment.resolved_parallelism == 2,
        "experiment runner records requested and resolved parallelism");
  check(experiment.completed_count == 2,
        "experiment runner completes both baseline policies");
  check(experiment.episode_reports.size() == 2,
        "experiment runner records one report per policy");
  check(std::isfinite(experiment.mean_total_reward()),
        "experiment runner computes mean total reward");
  check(std::isfinite(experiment.mean_max_drawdown()) &&
            std::isfinite(experiment.mean_total_turnover()) &&
            std::isfinite(experiment.mean_total_transaction_cost_base()),
        "experiment runner computes aggregate risk and cost means");
  check(std::isfinite(experiment.mean_projection_mae()),
        "experiment runner computes mean projection MAE");
  check(std::isfinite(experiment.mean_projection_rmse()) &&
            std::isfinite(experiment.mean_projection_correlation()),
        "experiment runner computes mean projection RMSE and correlation");
  check(std::isfinite(experiment.mean_zero_return_baseline_mae()) &&
            std::isfinite(experiment.mean_zero_return_baseline_rmse()) &&
            std::isfinite(experiment.mean_model_skill_vs_zero_mae()) &&
            std::isfinite(experiment.mean_model_skill_vs_zero_rmse()),
        "experiment runner computes zero-return baseline and skill metrics");
  check(!experiment.episode_reports[0].per_node_projection_mae.empty() &&
            !experiment.episode_reports[0]
                 .per_node_zero_return_baseline_mae.empty() &&
            !experiment.episode_reports[0]
                 .per_node_model_skill_vs_zero_mae.empty(),
        "experiment runner computes per-node projection diagnostics");
  check(experiment.episode_reports[0].policy_id == "base_reserve_only",
        "experiment runner preserves first policy id");
  check(experiment.episode_reports[1].policy_id == "equal_weight",
        "experiment runner preserves second policy id");
  check(experiment.episode_reports[0].experiment_task_index == 0 &&
            experiment.episode_reports[0].experiment_bundle_index == 0 &&
            experiment.episode_reports[0].experiment_policy_index == 0 &&
            experiment.episode_reports[1].experiment_task_index == 1 &&
            experiment.episode_reports[1].experiment_bundle_index == 0 &&
            experiment.episode_reports[1].experiment_policy_index == 1,
        "experiment runner preserves explicit bundle/policy task identity");
  check(experiment.policy_summaries.size() == 2,
        "experiment runner emits one summary per policy");
  check(experiment.policy_summaries[0].policy_summary_schema_id ==
            std::string(env::kReplayPolicyComparisonArtifactSchema),
        "experiment runner emits replay audit policy summary schema");
  check(experiment.policy_summaries[0].policy_id == "base_reserve_only",
        "experiment runner summary preserves first policy id");
  check(experiment.policy_summaries[0].attempted_count == 1,
        "experiment runner summary records first policy attempts");
  check(experiment.policy_summaries[0].completed_count == 1,
        "experiment runner summary records first policy completions");
  check(experiment.policy_summaries[1].policy_id == "equal_weight",
        "experiment runner summary preserves second policy id");
  check(std::isfinite(experiment.policy_summaries[1].mean_final_equity_base),
        "experiment runner summary computes final equity mean");
  check(std::isfinite(experiment.policy_summaries[1].mean_projection_mae),
        "experiment runner summary computes projection MAE mean");
  check(std::isfinite(experiment.policy_summaries[1].mean_projection_rmse) &&
            std::isfinite(
                experiment.policy_summaries[1].mean_projection_correlation),
        "experiment runner summary computes projection RMSE and correlation "
        "means");
  check(std::isfinite(
            experiment.policy_summaries[1].mean_zero_return_baseline_mae) &&
            std::isfinite(
                experiment.policy_summaries[1].mean_model_skill_vs_zero_mae),
        "experiment runner summary computes zero-return baseline skill means");
  check(experiment.runtime_run_id == bundle.spec.runtime_run_id,
        "experiment runner binds common runtime run id");
  check(experiment.environment_run_id == bundle.spec.environment_run_id,
        "experiment runner binds common environment run id");

  const auto experience_trace = env::output::make_experience_trace(experiment);
  env::output::validate_experience_trace(experience_trace);
  check(experience_trace.trace_schema_id ==
            std::string(env::output::kExperienceTraceSchema),
        "experience trace emits Cajtucu-ready schema");
  check(experience_trace.future_consumer ==
            std::string(env::output::kFutureCajtucuConsumer),
        "experience trace declares future Cajtucu consumer");
  check(experience_trace.episodes.size() == experiment.episode_reports.size(),
        "experience trace preserves episode reports");
  check(experience_trace.policy_comparisons.size() ==
            experiment.policy_summaries.size(),
        "experience trace preserves policy comparisons");
  check(!experience_trace.episodes.empty() &&
            !experience_trace.episodes[0].transitions.empty(),
        "experience trace preserves transition records");
  check(experience_trace.episodes[0].transitions[0].time_law_clean,
        "experience trace records clean time-law evidence");
  check(experience_trace.episodes[0]
            .transitions[0]
            .projection_validation_available,
        "experience trace records projection-validation availability");
  check(experience_trace.episodes[0].transitions[0].execution_model ==
            experiment.episode_reports[0].step_reports[0].execution_model,
        "experience trace preserves execution model");
  check(experience_trace.episodes[0].transitions[0].reward_total ==
            experiment.episode_reports[0].step_reports[0].reward_total,
        "experience trace preserves decomposed reward total");
  close(experience_trace.episodes[0].transitions[0].projection_rmse,
        experiment.episode_reports[0].step_reports[0].projection_rmse, 1e-12,
        "experience trace preserves projection RMSE");
  close(experience_trace.episodes[0].transitions[0].projection_correlation,
        experiment.episode_reports[0].step_reports[0].projection_correlation,
        1e-12, "experience trace preserves projection correlation");

  const auto experience_trace_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_experience_trace.report";
  env::output::write_experience_trace_report(experience_trace,
                                             experience_trace_path);
  const auto experience_trace_text = read_text(experience_trace_path);
  check(experience_trace_text.find(
            "schema=kikijyeba.environment.output.experience_trace.v1") !=
            std::string::npos,
        "experience trace writer records schema");
  check(experience_trace_text.find(
            "episode_0_transition_0_time_law_clean=true") != std::string::npos,
        "experience trace writer records time-law evidence");
  check(experience_trace_text.find("episode_0_transition_0_projection_rmse=") !=
                std::string::npos &&
            experience_trace_text.find(
                "episode_0_transition_0_projection_correlation=") !=
                std::string::npos,
        "experience trace writer records projection RMSE and correlation");
  const auto loaded_experience_trace =
      env::output::read_experience_trace_report(experience_trace_path);
  env::output::validate_experience_trace(loaded_experience_trace);
  check(loaded_experience_trace.trace_schema_id ==
            experience_trace.trace_schema_id,
        "experience trace reader preserves schema");
  check(loaded_experience_trace.episodes.size() ==
            experience_trace.episodes.size(),
        "experience trace reader preserves episode count");
  check(loaded_experience_trace.policy_comparisons.size() ==
            experience_trace.policy_comparisons.size(),
        "experience trace reader preserves policy comparison count");
  check(loaded_experience_trace.episodes[0].transitions[0].anchor_key ==
            experience_trace.episodes[0].transitions[0].anchor_key,
        "experience trace reader preserves transition anchor");
  close(loaded_experience_trace.episodes[0].transitions[0].reward_total,
        experience_trace.episodes[0].transitions[0].reward_total, 1e-12,
        "experience trace reader preserves reward total");
  close(loaded_experience_trace.episodes[0].transitions[0].projection_rmse,
        experience_trace.episodes[0].transitions[0].projection_rmse, 1e-12,
        "experience trace reader preserves projection RMSE");
  close(
      loaded_experience_trace.episodes[0].transitions[0].projection_correlation,
      experience_trace.episodes[0].transitions[0].projection_correlation, 1e-12,
      "experience trace reader preserves projection correlation");

  auto bad_experience_trace = experience_trace;
  bad_experience_trace.episodes[0].transitions[0].time_law_clean = false;
  bool rejected_bad_experience_trace = false;
  try {
    env::output::validate_experience_trace(bad_experience_trace);
  } catch (const std::exception &) {
    rejected_bad_experience_trace = true;
  }
  check(rejected_bad_experience_trace,
        "experience trace rejects broken time-law evidence");

  auto failing_policy_factories = policy_factories;
  failing_policy_factories.push_back({
      .policy_id = "null_policy_fixture",
      .policy_kind = env::policy_kind_t::external,
      .make_policy =
          [](const replay::replay_episode_bundle_t &) {
            return std::unique_ptr<env::policy_adapter_iface_t>{};
          },
  });
  auto failure_experiment_options = experiment_options;
  failure_experiment_options.continue_on_failure = true;
  auto failure_experiment = env::run_replay_experiment(
      "source_bundle_failure_compare", std::vector{enriched_bundle},
      failing_policy_factories, failure_experiment_options);
  check(failure_experiment.attempted_count == 3 &&
            failure_experiment.completed_count == 2 &&
            failure_experiment.failures.size() == 1,
        "experiment runner preserves successful reports while recording "
        "policy failures");
  check(failure_experiment.policy_summaries.size() == 3 &&
            failure_experiment.policy_summaries[2].policy_id ==
                "null_policy_fixture" &&
            failure_experiment.policy_summaries[2].attempted_count == 1 &&
            failure_experiment.policy_summaries[2].completed_count == 0 &&
            failure_experiment.policy_summaries[2].failed_count == 1,
        "experiment runner policy summary records continue-on-failure "
        "attempts");

  std::vector<env::replay_policy_factory_t> drift_policy_factories;
  drift_policy_factories.push_back({
      .policy_id = "declared_policy_fixture",
      .policy_kind = env::policy_kind_t::baseline,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            return std::make_unique<env::base_reserve_policy_t>(
                make_baseline_config_for_bundle("actual_policy_fixture",
                                                bundle));
          },
  });
  auto drift_experiment_options = experiment_options;
  drift_experiment_options.continue_on_failure = true;
  auto drift_experiment = env::run_replay_experiment(
      "source_bundle_policy_id_drift", std::vector{enriched_bundle},
      drift_policy_factories, drift_experiment_options);
  check(drift_experiment.attempted_count == 1 &&
            drift_experiment.completed_count == 0 &&
            drift_experiment.failures.size() == 1 &&
            drift_experiment.policy_summaries.size() == 1 &&
            drift_experiment.policy_summaries[0].policy_id ==
                "declared_policy_fixture" &&
            drift_experiment.policy_summaries[0].failed_count == 1 &&
            drift_experiment.failures[0].find("actual_policy_fixture") !=
                std::string::npos,
        "experiment runner rejects policy adapter id drift before execution");

  std::vector<env::replay_policy_factory_t> kind_drift_policy_factories;
  kind_drift_policy_factories.push_back({
      .policy_id = "kind_drift_fixture",
      .policy_kind = env::policy_kind_t::deterministic_allocator,
      .make_policy =
          [](const replay::replay_episode_bundle_t &bundle) {
            return std::make_unique<env::base_reserve_policy_t>(
                make_baseline_config_for_bundle("kind_drift_fixture", bundle));
          },
  });
  auto kind_drift_experiment = env::run_replay_experiment(
      "source_bundle_policy_kind_drift", std::vector{enriched_bundle},
      kind_drift_policy_factories, drift_experiment_options);
  check(kind_drift_experiment.attempted_count == 1 &&
            kind_drift_experiment.completed_count == 0 &&
            kind_drift_experiment.failures.size() == 1 &&
            kind_drift_experiment.policy_summaries.size() == 1 &&
            kind_drift_experiment.policy_summaries[0].policy_id ==
                "kind_drift_fixture" &&
            kind_drift_experiment.policy_summaries[0].policy_kind ==
                env::policy_kind_t::deterministic_allocator &&
            kind_drift_experiment.policy_summaries[0].failed_count == 1 &&
            kind_drift_experiment.failures[0].find("baseline") !=
                std::string::npos &&
            kind_drift_experiment.failures[0].find("deterministic_allocator") !=
                std::string::npos,
        "experiment runner rejects policy adapter kind drift before execution");

  const auto replay_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_replay_experiment.report";
  const auto replay_config_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_config.fixture";
  {
    std::ofstream config_fixture(replay_config_path, std::ios::trunc);
    config_fixture << "[fixture]\n"
                   << "replay_environment_contract = true\n";
  }
  env::replay_driver_detail::write_replay_experiment_report(
      experiment, replay_report_path, replay_config_path.string(),
      /*replay_bundle_count=*/1);
  const auto default_experience_trace_sidecar_path =
      env::replay_driver_detail::default_experience_trace_path_for_report(
          replay_report_path);
  check(default_experience_trace_sidecar_path.filename().string() ==
            "cuwacunu_environment_contract_replay_experiment.experience_trace."
            "report",
        "runtime replay driver derives deterministic experience trace sidecar "
        "path");
  const auto replay_report_text = read_text(replay_report_path);
  check(
      replay_report_text.find("runtime_run_id=" + bundle.spec.runtime_run_id) !=
          std::string::npos,
      "replay experiment report writes runtime run id");
  check(replay_report_text.find("environment_run_id=" +
                                bundle.spec.environment_run_id) !=
            std::string::npos,
        "replay experiment report writes environment run id");
  check(replay_report_text.find("config_path=" + replay_config_path.string()) !=
                std::string::npos &&
            replay_report_text.find("config_path_content_digest=") !=
                std::string::npos,
        "replay experiment report writes config path identity");
  check(replay_report_text.find("artifact_evidence=true") !=
                std::string::npos &&
            replay_report_text.find("visibility_only=true") !=
                std::string::npos &&
            replay_report_text.find("replay_executor=false") !=
                std::string::npos &&
            replay_report_text.find("market_readiness_authority=false") !=
                std::string::npos,
        "replay experiment report writes explicit audit-only authority flags");
  check(replay_report_text.find("replay_environment_version="
                                "kikijyeba.environment.replay.v1") !=
                std::string::npos &&
            replay_report_text.find("replay_environment_world_mode="
                                    "historical_replay") != std::string::npos &&
            replay_report_text.find("replay_environment_policy_surface="
                                    "policy_adapter") != std::string::npos &&
            replay_report_text.find("replay_environment_source_order_policy="
                                    "sequential") != std::string::npos &&
            replay_report_text.find("replay_environment_action_policy_identity="
                                    "policy_adapter_must_match_action") !=
                std::string::npos &&
            replay_report_text.find(
                "replay_environment_graph_node_universe_policy="
                "episode_spec_graph_node_ids") != std::string::npos &&
            replay_report_text.find("replay_environment_realization_key_policy="
                                    "shared_key_per_frame") !=
                std::string::npos &&
            replay_report_text.find("replay_environment_action_time_policy="
                                    "decision_timestamp_after_knowledge_before_"
                                    "realization") != std::string::npos &&
            replay_report_text.find(
                "replay_environment_experiment_task_identity="
                "bundle_policy_task_indices") != std::string::npos &&
            replay_report_text.find(
                "replay_environment_experiment_run_identity="
                "single_runtime_environment_run") != std::string::npos &&
            replay_report_text.find("replay_environment_step_artifact_identity="
                                    "episode_run_policy_cursor") !=
                std::string::npos &&
            replay_report_text.find(
                "replay_environment_experiment_report_count_policy="
                "counts_match_evidence") != std::string::npos &&
            replay_report_text.find("replay_environment_action_schema_id="
                                    "kikijyeba.environment.action."
                                    "target_weights.v1") != std::string::npos &&
            replay_report_text.find(
                "replay_environment_default_max_parallel_jobs=1") !=
                std::string::npos &&
            replay_report_text.find(
                "replay_environment_require_no_future_leakage=1") !=
                std::string::npos,
        "replay experiment report writes replay environment contract identity");
  check(replay_report_text.find("mean_projection_mae=") != std::string::npos &&
            replay_report_text.find("mean_projection_rmse=") !=
                std::string::npos &&
            replay_report_text.find("policy_1_mean_projection_mae=") !=
                std::string::npos &&
            replay_report_text.find("policy_1_mean_projection_rmse=") !=
                std::string::npos &&
            replay_report_text.find("mean_projection_correlation=") !=
                std::string::npos &&
            replay_report_text.find("mean_projection_directional_accuracy=") !=
                std::string::npos,
        "replay experiment report writes aggregate projection metrics");
  check(replay_report_text.find("time_law_observation_step_count=4") !=
                std::string::npos &&
            replay_report_text.find("time_law_expected_step_count=4") !=
                std::string::npos &&
            replay_report_text.find("time_law_action_step_count=4") !=
                std::string::npos &&
            replay_report_text.find("time_law_execution_step_count=4") !=
                std::string::npos &&
            replay_report_text.find(
                "time_law_realization_after_action_count=4") !=
                std::string::npos &&
            replay_report_text.find(
                "time_law_future_observation_violation_count=0") !=
                std::string::npos &&
            replay_report_text.find("mixed_future_realization_key_count=0") !=
                std::string::npos &&
            replay_report_text.find("projection_validation_step_count=4") !=
                std::string::npos,
        "replay experiment report writes aggregate time-law and projection "
        "step evidence for Lattice replay artifact readiness");

  auto missing_action_report = experiment;
  missing_action_report.episode_reports[0]
      .step_reports[0]
      .action_schema_id.clear();
  const auto missing_action_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_missing_action_counter.report";
  bool rejected_missing_action_counter = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        missing_action_report, missing_action_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_missing_action_counter = true;
  }
  check(rejected_missing_action_counter,
        "replay experiment report rejects missing required action counter "
        "evidence");

  auto bad_action_schema_report = experiment;
  bad_action_schema_report.episode_reports[0].step_reports[0].action_schema_id =
      "fixture.unknown_action.v1";
  const auto bad_action_schema_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_action_schema_counter.report";
  bool rejected_bad_action_schema_counter = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_action_schema_report, bad_action_schema_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_action_schema_counter = true;
  }
  check(rejected_bad_action_schema_counter,
        "replay experiment report rejects unexpected action schema counter "
        "evidence");

  auto missing_action_timestamp_report = experiment;
  missing_action_timestamp_report.episode_reports[0]
      .step_reports[0]
      .action_decision_timestamp_ms = 0;
  const auto missing_action_timestamp_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_missing_action_timestamp_counter.report";
  bool rejected_missing_action_timestamp_counter = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        missing_action_timestamp_report, missing_action_timestamp_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_missing_action_timestamp_counter = true;
  }
  check(rejected_missing_action_timestamp_counter,
        "replay experiment report rejects action counters without a decision "
        "timestamp after knowledge and before realization availability");

  auto missing_projection_report = experiment;
  missing_projection_report.episode_reports[0].step_reports[0].projection_mae =
      std::numeric_limits<double>::quiet_NaN();
  const auto missing_projection_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_missing_projection_counter.report";
  bool rejected_missing_projection_counter = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        missing_projection_report, missing_projection_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_missing_projection_counter = true;
  }
  check(rejected_missing_projection_counter,
        "replay experiment report rejects missing required projection counter "
        "evidence");

  check(replay_report_text.find("failed_count=0") != std::string::npos,
        "replay experiment report writes aggregate failed count");
  check(
      replay_report_text.find("top_level_metric_scope=") != std::string::npos &&
          replay_report_text.find("policy_metric_scope=") != std::string::npos,
      "replay experiment report labels aggregate metric scopes");
  check(replay_report_text.find("mean_max_drawdown=") != std::string::npos &&
            replay_report_text.find("mean_total_turnover=") !=
                std::string::npos &&
            replay_report_text.find("mean_total_transaction_cost_base=") !=
                std::string::npos,
        "replay experiment report writes aggregate risk and cost metrics");
  check(replay_report_text.find("mean_zero_return_baseline_mae=") !=
                std::string::npos &&
            replay_report_text.find("mean_model_skill_vs_zero_mae=") !=
                std::string::npos,
        "replay experiment report writes aggregate projection baseline skill");
  check(replay_report_text.find("policy_0_schema=") != std::string::npos &&
            replay_report_text.find("policy_1_schema=") != std::string::npos,
        "replay experiment report writes policy-comparison artifact schemas");
  check(replay_report_text.find("policy_0_mean_zero_return_baseline_mae=") !=
                std::string::npos &&
            replay_report_text.find("policy_0_mean_model_skill_vs_zero_mae=") !=
                std::string::npos,
        "replay experiment report writes policy projection baseline skill");
  check(replay_report_text.find("episode_0_per_node_projection_mae=") !=
                std::string::npos &&
            replay_report_text.find(
                "episode_0_per_node_model_skill_vs_zero_mae=") !=
                std::string::npos,
        "replay experiment report writes per-node projection diagnostics");
  check(
      replay_report_text.find("episode_0_step_0_runtime_run_id=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_policy_id=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_method_id=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_accepted_batch_cursor_token=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_accepted_cursor_offset=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_action_decision_timestamp_ms=") !=
              std::string::npos,
      "replay experiment report writes self-identifying step evidence");
  check(replay_report_text.find("episode_0_step_0_zero_return_baseline_mae=") !=
                std::string::npos &&
            replay_report_text.find(
                "episode_0_step_0_model_skill_vs_zero_mae=") !=
                std::string::npos,
        "replay experiment report writes step projection baseline skill");

  const auto runtime_experiment_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_runtime_experiments.index";
  env::write_runtime_replay_experiment_index_at(
      runtime_experiment_index_path,
      {env::runtime_replay_experiment_index_entry_t{
          .experiment_id = experiment.experiment_id,
          .runtime_run_id = experiment.runtime_run_id,
          .environment_run_id = experiment.environment_run_id,
          .policy_count = policy_factories.size(),
          .replay_bundle_count = 1,
          .attempted_count = experiment.attempted_count,
          .completed_count = experiment.completed_count,
          .report_path = replay_report_path,
          .report_digest =
              env::replay_driver_detail::replay_report_digest_for_path(
                  replay_report_path),
      }});
  const auto runtime_experiment_index =
      env::read_runtime_replay_experiment_index_at(
          runtime_experiment_index_path);
  check(runtime_experiment_index.size() == 1 &&
            runtime_experiment_index[0].runtime_run_id ==
                experiment.runtime_run_id &&
            runtime_experiment_index[0].attempted_count ==
                experiment.attempted_count &&
            runtime_experiment_index[0].completed_count ==
                experiment.completed_count &&
            !runtime_experiment_index[0].report_digest.empty(),
        "runtime replay experiment index preserves consistent counts");

  const auto empty_runtime_experiment_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_empty_runtime_experiments.index";
  bool rejected_empty_runtime_experiment_index = false;
  try {
    env::write_runtime_replay_experiment_index_at(
        empty_runtime_experiment_index_path, {});
  } catch (const std::exception &) {
    rejected_empty_runtime_experiment_index = true;
  }
  check(rejected_empty_runtime_experiment_index,
        "runtime replay experiment index rejects empty entry sets before "
        "writing");
  check(!std::filesystem::exists(empty_runtime_experiment_index_path),
        "runtime replay experiment index validates empty entry sets before "
        "writing");

  const auto persisted_empty_runtime_experiment_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_persisted_empty_runtime_experiments.index";
  {
    std::ofstream empty_runtime_experiment_index(
        persisted_empty_runtime_experiment_index_path, std::ios::trunc);
    empty_runtime_experiment_index
        << "schema=" << env::kRuntimeReplayExperimentIndexSchema << "\n";
    empty_runtime_experiment_index << "entry_count=0\n";
  }
  bool rejected_persisted_empty_runtime_experiment_index = false;
  try {
    (void)env::read_runtime_replay_experiment_index_at(
        persisted_empty_runtime_experiment_index_path);
  } catch (const std::exception &) {
    rejected_persisted_empty_runtime_experiment_index = true;
  }
  check(rejected_persisted_empty_runtime_experiment_index,
        "runtime replay experiment index rejects persisted empty entry sets");

  bool rejected_empty_runtime_experiment_index_path = false;
  try {
    (void)env::read_runtime_replay_experiment_index_at({});
  } catch (const std::exception &) {
    rejected_empty_runtime_experiment_index_path = true;
  }
  check(rejected_empty_runtime_experiment_index_path,
        "runtime replay experiment index rejects empty index paths on read");

  bool rejected_empty_runtime_experiment_job_dir_read = false;
  try {
    (void)env::read_runtime_replay_experiment_index({});
  } catch (const std::exception &) {
    rejected_empty_runtime_experiment_job_dir_read = true;
  }
  check(rejected_empty_runtime_experiment_job_dir_read,
        "runtime replay experiment index rejects empty job dirs on read");

  bool rejected_empty_runtime_experiment_job_dir_write = false;
  try {
    env::write_runtime_replay_experiment_index(
        {}, {env::runtime_replay_experiment_index_entry_t{
                .experiment_id = experiment.experiment_id,
                .runtime_run_id = experiment.runtime_run_id,
                .environment_run_id = experiment.environment_run_id,
                .policy_count = policy_factories.size(),
                .replay_bundle_count = 1,
                .attempted_count = experiment.attempted_count,
                .completed_count = experiment.completed_count,
                .report_path = replay_report_path,
                .report_digest =
                    env::replay_driver_detail::replay_report_digest_for_path(
                        replay_report_path),
            }});
  } catch (const std::exception &) {
    rejected_empty_runtime_experiment_job_dir_write = true;
  }
  check(rejected_empty_runtime_experiment_job_dir_write,
        "runtime replay experiment index rejects empty job dirs on write");

  const auto legacy_runtime_experiment_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_legacy_runtime_experiments.index";
  {
    std::ofstream legacy_index(legacy_runtime_experiment_index_path,
                               std::ios::trunc);
    legacy_index << "schema=" << env::kRuntimeReplayExperimentIndexSchema
                 << "\n"
                 << "entry_count=1\n"
                 << "entry_0_experiment_id=" << experiment.experiment_id << "\n"
                 << "entry_0_environment_run_id="
                 << experiment.environment_run_id << "\n"
                 << "entry_0_policy_count=" << policy_factories.size() << "\n"
                 << "entry_0_replay_bundle_count=1\n"
                 << "entry_0_attempted_count=" << experiment.attempted_count
                 << "\n"
                 << "entry_0_completed_count=" << experiment.completed_count
                 << "\n"
                 << "entry_0_report_path=" << replay_report_path.string()
                 << "\n";
  }
  const auto legacy_runtime_experiment_index =
      env::read_runtime_replay_experiment_index_at(
          legacy_runtime_experiment_index_path);
  check(legacy_runtime_experiment_index.size() == 1 &&
            legacy_runtime_experiment_index[0].runtime_run_id ==
                experiment.runtime_run_id,
        "runtime replay experiment index reads legacy entries by report body");

  const auto bad_runtime_experiment_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_runtime_experiments.index";
  bool rejected_bad_runtime_experiment_index = false;
  try {
    env::write_runtime_replay_experiment_index_at(
        bad_runtime_experiment_index_path,
        {env::runtime_replay_experiment_index_entry_t{
            .experiment_id = experiment.experiment_id,
            .runtime_run_id = experiment.runtime_run_id,
            .environment_run_id = experiment.environment_run_id,
            .policy_count = policy_factories.size(),
            .replay_bundle_count = 1,
            .attempted_count = experiment.attempted_count + 1,
            .completed_count = experiment.completed_count,
            .report_path = replay_report_path,
            .report_digest =
                env::replay_driver_detail::replay_report_digest_for_path(
                    replay_report_path),
        }});
  } catch (const std::exception &) {
    rejected_bad_runtime_experiment_index = true;
  }
  check(rejected_bad_runtime_experiment_index,
        "runtime replay experiment index rejects inconsistent counts");

  const auto bad_runtime_experiment_parent_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_runtime_experiment_parent";
  const auto bad_runtime_experiment_parent_index_dir =
      bad_runtime_experiment_parent_root / "index";
  std::filesystem::remove_all(bad_runtime_experiment_parent_root);
  std::filesystem::create_directories(bad_runtime_experiment_parent_index_dir);
  const auto bad_runtime_experiment_parent_write_path =
      bad_runtime_experiment_parent_index_dir / "parent_report_write.index";
  bool rejected_parent_runtime_experiment_report_write = false;
  try {
    env::write_runtime_replay_experiment_index_at(
        bad_runtime_experiment_parent_write_path,
        {env::runtime_replay_experiment_index_entry_t{
            .experiment_id = experiment.experiment_id,
            .runtime_run_id = experiment.runtime_run_id,
            .environment_run_id = experiment.environment_run_id,
            .policy_count = policy_factories.size(),
            .replay_bundle_count = 1,
            .attempted_count = experiment.attempted_count,
            .completed_count = experiment.completed_count,
            .report_path = std::filesystem::path("..") / "escape.report",
            .report_digest = "escape_report_digest",
        }});
  } catch (const std::exception &) {
    rejected_parent_runtime_experiment_report_write = true;
  }
  check(rejected_parent_runtime_experiment_report_write,
        "runtime replay experiment index rejects parent-directory report paths "
        "before writing");
  check(!std::filesystem::exists(bad_runtime_experiment_parent_write_path),
        "runtime replay experiment index validates report paths before "
        "writing");

  const auto bad_runtime_experiment_parent_read_path =
      bad_runtime_experiment_parent_index_dir / "parent_report_read.index";
  {
    std::ofstream bad_parent_report_index(
        bad_runtime_experiment_parent_read_path, std::ios::trunc);
    bad_parent_report_index
        << "schema=" << env::kRuntimeReplayExperimentIndexSchema << "\n";
    bad_parent_report_index << "entry_count=1\n";
    bad_parent_report_index
        << "entry_0_experiment_id=" << experiment.experiment_id << "\n";
    bad_parent_report_index
        << "entry_0_runtime_run_id=" << experiment.runtime_run_id << "\n";
    bad_parent_report_index
        << "entry_0_environment_run_id=" << experiment.environment_run_id
        << "\n";
    bad_parent_report_index
        << "entry_0_policy_count=" << policy_factories.size() << "\n";
    bad_parent_report_index << "entry_0_replay_bundle_count=1\n";
    bad_parent_report_index
        << "entry_0_attempted_count=" << experiment.attempted_count << "\n";
    bad_parent_report_index
        << "entry_0_completed_count=" << experiment.completed_count << "\n";
    bad_parent_report_index << "entry_0_report_path=../escape.report\n";
    bad_parent_report_index << "entry_0_report_digest=escape_report_digest\n";
  }
  bool rejected_parent_runtime_experiment_report_read = false;
  try {
    (void)env::read_runtime_replay_experiment_index_at(
        bad_runtime_experiment_parent_read_path);
  } catch (const std::exception &) {
    rejected_parent_runtime_experiment_report_read = true;
  }
  check(rejected_parent_runtime_experiment_report_read,
        "runtime replay experiment index rejects persisted parent-directory "
        "report paths");

  const auto missing_runtime_report_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_missing_runtime_report.index";
  bool rejected_missing_runtime_report = false;
  try {
    env::write_runtime_replay_experiment_index_at(
        missing_runtime_report_index_path,
        {env::runtime_replay_experiment_index_entry_t{
            .experiment_id = experiment.experiment_id,
            .runtime_run_id = experiment.runtime_run_id,
            .environment_run_id = experiment.environment_run_id,
            .policy_count = policy_factories.size(),
            .replay_bundle_count = 1,
            .attempted_count = experiment.attempted_count,
            .completed_count = experiment.completed_count,
            .report_path = std::filesystem::temp_directory_path() /
                           "cuwacunu_environment_contract_missing.report",
            .report_digest = "missing_report_digest_fixture",
        }});
  } catch (const std::exception &) {
    rejected_missing_runtime_report = true;
  }
  check(rejected_missing_runtime_report,
        "runtime replay experiment index rejects missing report paths");

  const auto bad_report_binding_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_report_binding.index";
  bool rejected_bad_report_binding = false;
  try {
    const auto mismatched_policy_count = policy_factories.size() + 1;
    env::write_runtime_replay_experiment_index_at(
        bad_report_binding_index_path,
        {env::runtime_replay_experiment_index_entry_t{
            .experiment_id = experiment.experiment_id,
            .runtime_run_id = experiment.runtime_run_id,
            .environment_run_id = experiment.environment_run_id,
            .policy_count = mismatched_policy_count,
            .replay_bundle_count = 1,
            .attempted_count = mismatched_policy_count,
            .completed_count = experiment.completed_count,
            .report_path = replay_report_path,
            .report_digest =
                env::replay_driver_detail::replay_report_digest_for_path(
                    replay_report_path),
        }});
  } catch (const std::exception &) {
    rejected_bad_report_binding = true;
  }
  check(rejected_bad_report_binding,
        "runtime replay experiment index rejects report binding drift");

  const auto bad_report_digest_index_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_report_digest.index";
  bool rejected_bad_report_digest = false;
  try {
    env::write_runtime_replay_experiment_index_at(
        bad_report_digest_index_path,
        {env::runtime_replay_experiment_index_entry_t{
            .experiment_id = experiment.experiment_id,
            .runtime_run_id = experiment.runtime_run_id,
            .environment_run_id = experiment.environment_run_id,
            .policy_count = policy_factories.size(),
            .replay_bundle_count = 1,
            .attempted_count = experiment.attempted_count,
            .completed_count = experiment.completed_count,
            .report_path = replay_report_path,
            .report_digest = "mutated_report_digest",
        }});
  } catch (const std::exception &) {
    rejected_bad_report_digest = true;
  }
  check(rejected_bad_report_digest,
        "runtime replay experiment index rejects report digest drift");

  auto bad_step_identity_experiment = experiment;
  bad_step_identity_experiment.episode_reports[0].step_reports[0].policy_id =
      "mutated_policy_id";
  const auto bad_step_identity_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_step_identity.report";
  bool rejected_bad_step_identity_report = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_step_identity_experiment, bad_step_identity_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_step_identity_report = true;
  }
  check(rejected_bad_step_identity_report,
        "replay experiment report rejects step identity drift");

  auto bad_step_cursor_experiment = experiment;
  auto &bad_step_cursor =
      bad_step_cursor_experiment.episode_reports[0].step_reports[0];
  bad_step_cursor.anchor_key = "anchor_11";
  bad_step_cursor.observation_anchor_index = 11;
  bad_step_cursor.next_realization_anchor_index = 12;
  bad_step_cursor.accepted_cursor_offset = 1;
  const auto bad_step_cursor_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_step_cursor.report";
  bool rejected_bad_step_cursor_report = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_step_cursor_experiment, bad_step_cursor_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_step_cursor_report = true;
  }
  check(rejected_bad_step_cursor_report,
        "replay experiment report rejects out-of-order step cursor evidence");

  auto bad_anchor_key_experiment = experiment;
  bad_anchor_key_experiment.episode_reports[0].accepted_anchor_keys =
      "mutated_anchor_10,anchor_11";
  const auto bad_anchor_key_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_anchor_key.report";
  bool rejected_bad_anchor_key_report = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_anchor_key_experiment, bad_anchor_key_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_anchor_key_report = true;
  }
  check(rejected_bad_anchor_key_report,
        "replay experiment report rejects accepted anchor-key drift");

  auto bad_runtime_identity_experiment = experiment;
  bad_runtime_identity_experiment.episode_reports[0].runtime_run_id =
      "mutated_runtime_run";
  for (auto &step :
       bad_runtime_identity_experiment.episode_reports[0].step_reports) {
    step.runtime_run_id = "mutated_runtime_run";
  }
  const auto bad_runtime_identity_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_runtime_identity.report";
  bool rejected_bad_runtime_identity = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_runtime_identity_experiment, bad_runtime_identity_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_runtime_identity = true;
  }
  check(rejected_bad_runtime_identity,
        "replay experiment report rejects runtime identity drift");

  auto bad_environment_identity_experiment = experiment;
  bad_environment_identity_experiment.episode_reports[0].environment_run_id =
      "mutated_environment_run";
  for (auto &step :
       bad_environment_identity_experiment.episode_reports[0].step_reports) {
    step.environment_run_id = "mutated_environment_run";
  }
  const auto bad_environment_identity_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_environment_identity.report";
  bool rejected_bad_environment_identity = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_environment_identity_experiment,
        bad_environment_identity_report_path, replay_config_path.string(),
        /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_environment_identity = true;
  }
  check(rejected_bad_environment_identity,
        "replay experiment report rejects environment identity drift");

  auto bad_count_experiment = experiment;
  ++bad_count_experiment.attempted_count;
  const auto bad_count_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_experiment_count.report";
  bool rejected_bad_experiment_count = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_count_experiment, bad_count_report_path,
        replay_config_path.string(),
        /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_experiment_count = true;
  }
  check(rejected_bad_experiment_count,
        "replay experiment report rejects inconsistent aggregate counts");

  const auto bad_bundle_count_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_bundle_count.report";
  bool rejected_bad_bundle_count = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        experiment, bad_bundle_count_report_path, replay_config_path.string(),
        /*replay_bundle_count=*/2);
  } catch (const std::exception &) {
    rejected_bad_bundle_count = true;
  }
  check(rejected_bad_bundle_count,
        "replay experiment report rejects inconsistent bundle count");

  const auto missing_config_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_missing_config.report";
  bool rejected_missing_config_path = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        experiment, missing_config_report_path, "",
        /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_missing_config_path = true;
  }
  check(rejected_missing_config_path,
        "replay experiment report rejects missing config path");

  auto bad_task_identity_experiment = experiment;
  bad_task_identity_experiment.episode_reports[0].experiment_policy_index = 1;
  const auto bad_task_identity_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_task_identity.report";
  bool rejected_bad_task_identity = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_task_identity_experiment, bad_task_identity_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_task_identity = true;
  }
  check(rejected_bad_task_identity,
        "replay experiment report rejects invalid bundle/policy task identity");

  const auto failure_replay_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_replay_failure_experiment.report";
  env::replay_driver_detail::write_replay_experiment_report(
      failure_experiment, failure_replay_report_path,
      replay_config_path.string(),
      /*replay_bundle_count=*/1);
  const auto failure_replay_report_text = read_text(failure_replay_report_path);
  check(
      failure_replay_report_text.find("failed_count=1") != std::string::npos &&
          failure_replay_report_text.find("failure_count=1") !=
              std::string::npos &&
          failure_replay_report_text.find("failure_0=") != std::string::npos &&
          failure_replay_report_text.find("null_policy_fixture") !=
              std::string::npos &&
          failure_replay_report_text.find("policy_2_failed_count=1") !=
              std::string::npos,
      "replay experiment report writes continue-on-failure evidence");

  auto bad_failure_task_experiment = failure_experiment;
  bad_failure_task_experiment.failures[0] =
      "task=99|bundle=0|policy_index=2|policy=null_policy_fixture|"
      "policy_kind=external|mutated";
  const auto bad_failure_task_report_path =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_contract_bad_failure_task.report";
  bool rejected_bad_failure_task = false;
  try {
    env::replay_driver_detail::write_replay_experiment_report(
        bad_failure_task_experiment, bad_failure_task_report_path,
        replay_config_path.string(), /*replay_bundle_count=*/1);
  } catch (const std::exception &) {
    rejected_bad_failure_task = true;
  }
  check(rejected_bad_failure_task,
        "replay experiment report rejects invalid failed task identity");

  check(replay_report_text.find("experiment_requested_max_parallel_jobs=2") !=
                std::string::npos &&
            replay_report_text.find("experiment_resolved_parallelism=2") !=
                std::string::npos,
        "replay experiment report writes requested and resolved parallelism");
  check(
      replay_report_text.find("episode_0_step_count=2") != std::string::npos &&
          replay_report_text.find("episode_0_graph_node_ids=BTC,ETH,USDT") !=
              std::string::npos &&
          replay_report_text.find("episode_0_base_reserve_node_id=USDT") !=
              std::string::npos &&
          replay_report_text.find("episode_0_requested_anchor_index_begin=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_experiment_task_index=0") !=
              std::string::npos &&
          replay_report_text.find("episode_0_experiment_bundle_index=0") !=
              std::string::npos &&
          replay_report_text.find("episode_0_experiment_policy_index=0") !=
              std::string::npos &&
          replay_report_text.find("episode_0_schema=") != std::string::npos &&
          replay_report_text.find("episode_0_warning_count=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_warnings=") != std::string::npos &&
          replay_report_text.find("episode_0_failure_count=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_failures=") != std::string::npos &&
          replay_report_text.find("episode_0_requested_source_key_begin=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_accepted_cursor_kind="
                                  "graph_anchor") != std::string::npos &&
          replay_report_text.find("episode_0_accepted_batch_cursor_token=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_accepted_anchor_index_begin=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_accepted_anchor_keys=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_schema=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_reward_total=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_next_realization_anchor_index=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_rebalance_plan_valid=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_rebalance_plan_enforced=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_execution_model=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_risk_gate_evaluated=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_fill_count=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_execution_trace_available=true") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_backend_id=cajtucu.execution.paper."
              "v1") != std::string::npos &&
          replay_report_text.find("episode_0_step_0_cajtucu_trace_id=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_market_source_id=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_synthetic_direct_edges=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_ledger_intent_equity_mismatch=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_cajtucu_order_count=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_cajtucu_total_transaction_cost_base=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_residual_quality_available=") !=
              std::string::npos &&
          replay_report_text.find(
              "episode_0_step_0_decision_report_schema_id=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_warnings=") !=
              std::string::npos &&
          replay_report_text.find("episode_0_step_0_failures=") !=
              std::string::npos,
      "replay experiment report writes compact per-step evidence");

  bool rejected_empty_preloaded_source = false;
  try {
    env::preloaded_replay_bundle_source_t empty_source(
        "empty_preloaded_source_fixture", {});
  } catch (const std::exception &) {
    rejected_empty_preloaded_source = true;
  }
  check(rejected_empty_preloaded_source,
        "preloaded replay bundle source rejects empty bundle sets");

  env::preloaded_replay_bundle_source_t source("preloaded_source_fixture",
                                               std::vector{bundle});
  auto collected = env::collect_replay_bundles(source, /*max_count=*/1);
  check(collected.size() == 1, "bundle source collect returns one bundle");
  check(!source.next_bundle().has_value(),
        "bundle source cursor exhausts after collect");
  source.reset();
  env::replay_experiment_options_t source_experiment_options{};
  source_experiment_options.max_parallel_jobs = 2;
  auto source_experiment =
      env::run_replay_experiment("source_stream_baseline_compare", source,
                                 policy_factories, source_experiment_options);
  check(source_experiment.attempted_count == 2,
        "source experiment attempts bundle/policy product");
  check(source_experiment.requested_max_parallel_jobs == 2 &&
            source_experiment.resolved_parallelism == 2,
        "source experiment records requested and resolved parallelism");
  check(source_experiment.completed_count == 2,
        "source experiment completes both baseline policies");
  check(std::isfinite(source_experiment.mean_final_equity_base()),
        "source experiment computes mean final equity");
  check(source_experiment.policy_summaries.size() == 2,
        "source experiment emits policy summaries");

  auto projection_required_bundle_world_options = bundle_world_options;
  projection_required_bundle_world_options.require_projection_validation = true;
  env::graph_anchor_replay_bundle_record_t<std::int64_t> graph_anchor_record{};
  graph_anchor_record.base_spec = spec;
  graph_anchor_record.wave = make_source_wave();
  graph_anchor_record.cursor = cursor;
  graph_anchor_record.batch = batch;
  graph_anchor_record.world_options = projection_required_bundle_world_options;
  graph_anchor_record.observation_artifacts =
      make_source_observation_artifacts(bundle, graph);
  graph_anchor_record.require_observation_artifacts = true;
  env::graph_anchor_replay_bundle_source_t<std::int64_t> graph_anchor_source(
      "graph_anchor_source_fixture", graph, std::vector{graph_anchor_record});
  auto graph_anchor_bundle = graph_anchor_source.next_bundle();
  check(graph_anchor_bundle.has_value(),
        "graph-anchor source materializes replay bundle");
  check(graph_anchor_bundle->frame_count() == 2,
        "graph-anchor source materialized frame count");
  check(!graph_anchor_source.next_bundle().has_value(),
        "graph-anchor source cursor exhausts");
  graph_anchor_source.reset();
  auto graph_anchor_experiment = env::run_replay_experiment(
      "graph_anchor_stream_baseline_compare", graph_anchor_source,
      policy_factories, source_experiment_options);
  check(graph_anchor_experiment.attempted_count == 2,
        "graph-anchor source experiment attempts bundle/policy product");
  check(graph_anchor_experiment.completed_count == 2,
        "graph-anchor source experiment completes both baseline policies");
  check(graph_anchor_experiment.policy_summaries[1].attempted_count == 1,
        "graph-anchor source summary records equal-weight attempt");

  graph_anchor_source.reset();
  auto graph_anchor_allocation_experiment = env::run_replay_experiment(
      "graph_anchor_stream_allocation_compare", graph_anchor_source,
      std::vector{make_sdu_policy_factory()}, source_experiment_options);
  check(graph_anchor_allocation_experiment.completed_count == 1,
        "graph-anchor source runs deterministic allocation policy with "
        "attached artifacts");

  replay::runtime_graph_anchor_replay_record_t<std::int64_t> runtime_record{};
  runtime_record.wave_plan = make_runtime_wave_plan(cursor);
  runtime_record.manifest =
      make_runtime_manifest(graph, runtime_record.wave_plan);
  runtime_record.base_spec = spec;
  runtime_record.base_spec.runtime_run_id = runtime_record.manifest.job_id;
  runtime_record.cursor = cursor;
  runtime_record.batch = batch;
  runtime_record.world_options = projection_required_bundle_world_options;

  const auto first_pulse_cursor =
      dataloader::make_graph_anchor_cursor<std::int64_t>(
          graph.computed_graph_order_fingerprint(), /*begin_anchor_index=*/10,
          /*end_anchor_index=*/11, /*requested_batch_size=*/1,
          /*anchor_keys=*/{1000}, /*anchor_indices=*/{10});
  auto first_pulse_record = runtime_record;
  first_pulse_record.cursor = first_pulse_cursor;
  first_pulse_record.wave_plan = replay::wave_plan_for_batch_cursor(
      runtime_record.wave_plan, first_pulse_cursor);
  first_pulse_record.batch.graph_order_fingerprint =
      graph.computed_graph_order_fingerprint();
  replay::validate_runtime_graph_anchor_replay_record(first_pulse_record,
                                                      graph);

  const auto last_pulse_cursor =
      dataloader::make_graph_anchor_cursor<std::int64_t>(
          graph.computed_graph_order_fingerprint(), /*begin_anchor_index=*/11,
          /*end_anchor_index=*/12, /*requested_batch_size=*/1,
          /*anchor_keys=*/{2000}, /*anchor_indices=*/{11});
  auto last_pulse_record = runtime_record;
  last_pulse_record.cursor = last_pulse_cursor;
  last_pulse_record.wave_plan = replay::wave_plan_for_batch_cursor(
      runtime_record.wave_plan, last_pulse_cursor);
  last_pulse_record.batch.graph_order_fingerprint =
      graph.computed_graph_order_fingerprint();
  replay::validate_runtime_graph_anchor_replay_record(last_pulse_record, graph);

  auto bad_first_manifest_record = first_pulse_record;
  bad_first_manifest_record.manifest.first_anchor_key = "999";
  bool rejected_bad_manifest_first_key = false;
  try {
    replay::validate_runtime_graph_anchor_replay_record(
        bad_first_manifest_record, graph);
  } catch (const std::exception &) {
    rejected_bad_manifest_first_key = true;
  }
  check(rejected_bad_manifest_first_key,
        "runtime replay source checks manifest first key only on first pulse");

  auto bad_last_manifest_record = last_pulse_record;
  bad_last_manifest_record.manifest.last_anchor_key = "3000";
  bool rejected_bad_manifest_last_key = false;
  try {
    replay::validate_runtime_graph_anchor_replay_record(
        bad_last_manifest_record, graph);
  } catch (const std::exception &) {
    rejected_bad_manifest_last_key = true;
  }
  check(rejected_bad_manifest_last_key,
        "runtime replay source checks manifest last key only on last pulse");

  const auto artifact_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_kikijyeba_environment_artifact_index_fixture";
  runtime_record.observation_artifacts =
      write_and_load_source_observation_artifact_index(bundle, graph,
                                                       artifact_index_root);
  runtime_record.require_observation_artifacts = true;

  const auto runtime_fixture_dir =
      std::filesystem::temp_directory_path() /
      "cuwacunu_kikijyeba_environment_runtime_fixture";
  std::filesystem::remove_all(runtime_fixture_dir);
  std::filesystem::create_directories(runtime_fixture_dir);
  runtime::write_job_manifest_file(runtime_fixture_dir / "job.manifest",
                                   runtime_record.manifest);
  auto runtime_evidence =
      replay::read_runtime_replay_job_evidence(runtime_fixture_dir);
  check(runtime_evidence.manifest.job_id == "runtime_job_fixture",
        "runtime replay source reads job manifest");
  check(runtime_evidence.wave_plan.source_cursor_kind == "graph_anchor",
        "runtime replay source reconstructs source cursor kind");
  auto runtime_base_spec = spec;
  runtime_base_spec.runtime_run_id.clear();
  runtime_base_spec.environment_run_id.clear();
  auto runtime_build_options =
      replay::make_runtime_allocation_belief_builder_options(
          graph, env::belief::belief_observer_spec_t{},
          replay::runtime_allocation_belief_observer_options_t{
              .base_reserve_node_id = "USDT",
              .risky_node_ids = spec.risky_node_ids,
              .require_direct_asset_base_edges = false,
              .default_linear_cost = 0.001,
              .default_capacity_weight_limit = 0.50,
          });
  check(runtime_build_options.base_policy.reserve_asset_id == "USDT",
        "runtime replay observer derives graph-node base reserve policy");
  check(runtime_build_options.node_graph_indices == std::vector<int64_t>{0, 1},
        "runtime replay observer maps risky nodes to graph indices");
  runtime_build_options.projection_options.coupling_options.sample_count = 32;
  runtime_build_options.projection_options.coupling_options
      .quantile_bisection_steps = 24;
  auto runtime_forecast_records =
      replay::make_runtime_replay_forecast_records_from_mdn_batch(
          make_source_mdn_out(), make_source_mdn_input_batch(graph, cursor),
          runtime_build_options,
          env::artifact_ref_t{
              .artifact_id = "mdn_artifact_batch",
              .artifact_path = "mdn/batch",
              .artifact_digest = "digest_mdn_batch",
              .schema_id = "wikimyei.inference.expected_value.mdn.v1",
          },
          "environment_fixture_model_v1", "", "identity_norm");
  check(runtime_forecast_records.size() == bundle.frames.size(),
        "runtime replay source builds one forecast record per MDN anchor");
  check(runtime_forecast_records[0].forecast_artifact.identity.anchor_key ==
            "1000",
        "runtime replay source preserves MDN anchor key");
  check(runtime_forecast_records[0].observation_anchor_index.has_value(),
        "runtime replay source derives observation anchor index from cursor");
  vicreg_stream::channel_representation_batch_t<std::int64_t>
      representation_replay_batch{};
  representation_replay_batch.future_edge_features = batch.future_features;
  representation_replay_batch.future_edge_mask = batch.future_mask;
  representation_replay_batch.future_keys = batch.future_keys;
  representation_replay_batch.anchor_keys = batch.anchor_keys;
  representation_replay_batch.edge_ids = batch.edge_ids;
  representation_replay_batch.graph_order_fingerprint =
      batch.graph_order_fingerprint;
  representation_replay_batch.cursor = batch.cursor;
  const auto runtime_edge_batch =
      replay::make_runtime_replay_edge_batch_from_channel_representation(
          representation_replay_batch);
  check(runtime_edge_batch.future_keys.defined(),
        "runtime replay source derives future keys from representation batch");
  check(runtime_edge_batch.cursor.anchor_count() == cursor.anchor_count(),
        "runtime replay source derives cursor from representation batch");
  const auto empty_runtime_forecast_write_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_empty_runtime_forecast_write_fixture";
  std::filesystem::remove_all(empty_runtime_forecast_write_root);
  const auto empty_runtime_forecast_artifact_dir =
      empty_runtime_forecast_write_root / "artifacts";
  const auto empty_runtime_forecast_index_path =
      empty_runtime_forecast_artifact_dir / "runtime_replay_artifacts.index";
  const auto empty_runtime_forecast_paths =
      replay::default_runtime_replay_artifact_paths_for_dir(
          empty_runtime_forecast_artifact_dir);
  bool rejected_empty_runtime_forecast_write = false;
  try {
    (void)replay::write_runtime_graph_anchor_replay_artifacts_to_paths(
        empty_runtime_forecast_index_path, empty_runtime_forecast_paths,
        runtime_edge_batch, {});
  } catch (const std::exception &) {
    rejected_empty_runtime_forecast_write = true;
  }
  check(rejected_empty_runtime_forecast_write,
        "runtime replay artifact writer rejects empty forecast records before "
        "writing");
  check(!std::filesystem::exists(
            empty_runtime_forecast_paths.graph_anchor_edge_batch_artifact_path),
        "runtime replay artifact writer does not write graph batches for empty "
        "forecast records");
  check(!std::filesystem::exists(empty_runtime_forecast_index_path),
        "runtime replay artifact writer does not write indexes for empty "
        "forecast records");
  const auto bad_runtime_forecast_write_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_bad_runtime_forecast_write_fixture";
  std::filesystem::remove_all(bad_runtime_forecast_write_root);
  auto bad_runtime_forecast_records = runtime_forecast_records;
  bad_runtime_forecast_records[0].forecast_artifact_path =
      std::filesystem::path("..") / "escape.forecast.pt";
  const auto bad_runtime_forecast_artifact_dir =
      bad_runtime_forecast_write_root / "artifacts";
  const auto bad_runtime_forecast_index_path =
      bad_runtime_forecast_artifact_dir / "runtime_replay_artifacts.index";
  const auto bad_runtime_forecast_escape_path =
      bad_runtime_forecast_write_root / "escape.forecast.pt";
  bool rejected_bad_runtime_forecast_write = false;
  try {
    (void)replay::write_runtime_graph_anchor_replay_artifacts_to_paths(
        bad_runtime_forecast_index_path,
        replay::default_runtime_replay_artifact_paths_for_dir(
            bad_runtime_forecast_artifact_dir),
        runtime_edge_batch, bad_runtime_forecast_records);
  } catch (const std::exception &) {
    rejected_bad_runtime_forecast_write = true;
  }
  check(rejected_bad_runtime_forecast_write,
        "runtime replay artifact writer rejects parent-directory forecast "
        "paths before writing");
  check(!std::filesystem::exists(bad_runtime_forecast_escape_path),
        "runtime replay artifact writer does not create escaped forecast "
        "artifacts");
  check(!std::filesystem::exists(bad_runtime_forecast_index_path),
        "runtime replay artifact writer validates forecast paths before "
        "writing indexes");
  const auto runtime_artifact_paths =
      replay::write_runtime_graph_anchor_replay_artifacts_from_mdn_batch(
          runtime_fixture_dir, make_source_mdn_out(),
          make_source_mdn_input_batch(graph, cursor),
          representation_replay_batch, runtime_build_options,
          env::artifact_ref_t{
              .artifact_id = "mdn_artifact_batch",
              .artifact_path = "mdn/batch",
              .artifact_digest = "digest_mdn_batch",
              .schema_id = "wikimyei.inference.expected_value.mdn.v1",
          },
          "environment_fixture_model_v1", "", "identity_norm");
  const auto runtime_pulse_artifact_paths = replay::
      write_runtime_graph_anchor_replay_artifacts_from_mdn_batch_for_pulse(
          runtime_fixture_dir, /*wave_pulse_index=*/7, make_source_mdn_out(),
          make_source_mdn_input_batch(graph, cursor),
          representation_replay_batch, runtime_build_options,
          env::artifact_ref_t{
              .artifact_id = "mdn_artifact_pulse",
              .artifact_path = "mdn/pulse",
              .artifact_digest = "digest_mdn_pulse",
              .schema_id = "wikimyei.inference.expected_value.mdn.v1",
          },
          "environment_fixture_model_v1", "", "identity_norm");
  check(!runtime_pulse_artifact_paths.observation_artifact_index_path.empty(),
        "runtime replay pulse writer returns observation artifact path");
  const auto runtime_batch_index =
      replay::read_runtime_replay_batch_index(runtime_fixture_dir);
  check(runtime_batch_index.size() == 1,
        "runtime replay pulse writer appends batch index entry");
  check(runtime_batch_index[0].wave_pulse_index == 7,
        "runtime replay batch index preserves wave pulse index");
  const auto empty_runtime_batch_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_empty_runtime_batch_index_fixture";
  std::filesystem::remove_all(empty_runtime_batch_index_root);
  bool rejected_empty_runtime_batch_index_write = false;
  try {
    replay::write_runtime_replay_batch_index(empty_runtime_batch_index_root,
                                             {});
  } catch (const std::exception &) {
    rejected_empty_runtime_batch_index_write = true;
  }
  check(rejected_empty_runtime_batch_index_write,
        "runtime replay batch index rejects empty entry sets before writing");
  check(!std::filesystem::exists(replay::runtime_replay_batch_index_path(
            empty_runtime_batch_index_root)),
        "runtime replay batch index validates empty entry sets before writing");

  const auto persisted_empty_runtime_batch_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_persisted_empty_runtime_batch_index_fixture";
  std::filesystem::remove_all(persisted_empty_runtime_batch_index_root);
  const auto persisted_empty_runtime_batch_index_path =
      replay::runtime_replay_batch_index_path(
          persisted_empty_runtime_batch_index_root);
  std::filesystem::create_directories(
      persisted_empty_runtime_batch_index_path.parent_path());
  {
    std::ofstream persisted_empty_runtime_batch_index(
        persisted_empty_runtime_batch_index_path, std::ios::trunc);
    persisted_empty_runtime_batch_index
        << "schema=" << replay::kRuntimeReplayBatchIndexSchema << "\n";
    persisted_empty_runtime_batch_index << "entry_count=0\n";
  }
  bool rejected_persisted_empty_runtime_batch_index = false;
  try {
    (void)replay::read_runtime_replay_batch_index(
        persisted_empty_runtime_batch_index_root);
  } catch (const std::exception &) {
    rejected_persisted_empty_runtime_batch_index = true;
  }
  check(rejected_persisted_empty_runtime_batch_index,
        "runtime replay batch index rejects persisted empty entry sets");

  bool rejected_empty_runtime_batch_job_dir_read = false;
  try {
    (void)replay::read_runtime_replay_batch_index({});
  } catch (const std::exception &) {
    rejected_empty_runtime_batch_job_dir_read = true;
  }
  check(rejected_empty_runtime_batch_job_dir_read,
        "runtime replay batch index rejects empty job dirs on read");

  const auto bad_runtime_batch_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_bad_runtime_batch_index_fixture";
  std::filesystem::remove_all(bad_runtime_batch_index_root);
  std::filesystem::create_directories(bad_runtime_batch_index_root);
  const auto bad_runtime_batch_index_path =
      replay::runtime_replay_batch_index_path(bad_runtime_batch_index_root);
  bool rejected_bad_runtime_batch_index_write = false;
  try {
    replay::write_runtime_replay_batch_index(
        bad_runtime_batch_index_root,
        {replay::runtime_replay_batch_index_entry_t{
            .wave_pulse_index = 0,
            .begin_anchor_index = 0,
            .end_anchor_index = 1,
            .anchor_count = 1,
            .batch_cursor_token = "bad_cursor",
            .artifact_path_index_path =
                std::filesystem::path("..") / "escape_runtime_paths.index"}});
  } catch (const std::exception &) {
    rejected_bad_runtime_batch_index_write = true;
  }
  check(rejected_bad_runtime_batch_index_write,
        "runtime replay batch index rejects parent-directory artifact path "
        "index pointers before writing");
  check(!std::filesystem::exists(bad_runtime_batch_index_path),
        "runtime replay batch index validates artifact path index pointers "
        "before writing");
  std::filesystem::create_directories(
      bad_runtime_batch_index_path.parent_path());
  {
    std::ofstream bad_runtime_batch_index(bad_runtime_batch_index_path,
                                          std::ios::trunc);
    bad_runtime_batch_index
        << "schema=" << replay::kRuntimeReplayBatchIndexSchema << "\n";
    bad_runtime_batch_index << "entry_count=1\n";
    bad_runtime_batch_index << "entry_0_wave_pulse_index=0\n";
    bad_runtime_batch_index << "entry_0_begin_anchor_index=0\n";
    bad_runtime_batch_index << "entry_0_end_anchor_index=1\n";
    bad_runtime_batch_index << "entry_0_anchor_count=1\n";
    bad_runtime_batch_index << "entry_0_batch_cursor_token=bad_cursor\n";
    bad_runtime_batch_index
        << "entry_0_artifact_path_index_path=../escape_runtime_paths.index\n";
  }
  bool rejected_bad_runtime_batch_index_read = false;
  try {
    (void)replay::read_runtime_replay_batch_index(bad_runtime_batch_index_root);
  } catch (const std::exception &) {
    rejected_bad_runtime_batch_index_read = true;
  }
  check(rejected_bad_runtime_batch_index_read,
        "runtime replay batch index rejects persisted parent-directory "
        "artifact path index pointers");
  const auto indexed_artifact_paths =
      replay::read_runtime_replay_artifact_path_index(runtime_fixture_dir);
  check(!indexed_artifact_paths.graph_anchor_edge_batch_artifact_path.empty(),
        "runtime replay source reads graph-anchor artifact path index");
  check(indexed_artifact_paths.observation_artifact_index_path ==
            runtime_artifact_paths.observation_artifact_index_path,
        "runtime replay source indexes observation artifact path");
  const auto bad_runtime_path_index_root =
      std::filesystem::temp_directory_path() /
      "cuwacunu_environment_bad_runtime_path_index_fixture";
  std::filesystem::remove_all(bad_runtime_path_index_root);
  std::filesystem::create_directories(bad_runtime_path_index_root);
  bool rejected_empty_runtime_path_index_path = false;
  try {
    replay::write_runtime_replay_artifact_path_index_at({},
                                                        indexed_artifact_paths);
  } catch (const std::exception &) {
    rejected_empty_runtime_path_index_path = true;
  }
  check(rejected_empty_runtime_path_index_path,
        "runtime replay artifact path index rejects empty index paths");
  bool rejected_empty_runtime_path_index_read_path = false;
  try {
    (void)replay::read_runtime_replay_artifact_path_index_at({});
  } catch (const std::exception &) {
    rejected_empty_runtime_path_index_read_path = true;
  }
  check(rejected_empty_runtime_path_index_read_path,
        "runtime replay artifact path index rejects empty index paths on read");
  bool rejected_empty_runtime_path_index_job_dir_read = false;
  try {
    (void)replay::read_runtime_replay_artifact_path_index({});
  } catch (const std::exception &) {
    rejected_empty_runtime_path_index_job_dir_read = true;
  }
  check(rejected_empty_runtime_path_index_job_dir_read,
        "runtime replay artifact path index rejects empty job dirs on read");
  bool rejected_empty_runtime_path_index_job_dir_write = false;
  try {
    replay::write_runtime_replay_artifact_path_index({},
                                                     indexed_artifact_paths);
  } catch (const std::exception &) {
    rejected_empty_runtime_path_index_job_dir_write = true;
  }
  check(rejected_empty_runtime_path_index_job_dir_write,
        "runtime replay artifact path index rejects empty job dirs on write");
  const auto bad_runtime_path_write_index =
      bad_runtime_path_index_root / "bad_runtime_path_write.index";
  bool rejected_bad_runtime_path_write = false;
  try {
    replay::write_runtime_replay_artifact_path_index_at(
        bad_runtime_path_write_index,
        replay::runtime_replay_artifact_paths_t{
            .graph_anchor_edge_batch_artifact_path =
                std::filesystem::path("..") / "escape_edge_batch.pt",
            .observation_artifact_index_path = "observations.index"});
  } catch (const std::exception &) {
    rejected_bad_runtime_path_write = true;
  }
  check(rejected_bad_runtime_path_write,
        "runtime replay artifact path index rejects parent-directory graph "
        "batch paths before writing");
  check(!std::filesystem::exists(bad_runtime_path_write_index),
        "runtime replay artifact path index validates graph batch paths before "
        "writing");
  const auto bad_runtime_path_read_index =
      bad_runtime_path_index_root / "bad_runtime_path_read.index";
  {
    std::ofstream bad_runtime_path_index(bad_runtime_path_read_index,
                                         std::ios::trunc);
    bad_runtime_path_index << "schema="
                           << replay::kRuntimeReplayArtifactPathIndexSchema
                           << "\n";
    bad_runtime_path_index
        << "graph_anchor_edge_batch_artifact_path=edge_batch.pt\n";
    bad_runtime_path_index
        << "observation_artifact_index_path=../observations.index\n";
  }
  bool rejected_bad_runtime_path_read = false;
  try {
    (void)replay::read_runtime_replay_artifact_path_index_at(
        bad_runtime_path_read_index);
  } catch (const std::exception &) {
    rejected_bad_runtime_path_read = true;
  }
  check(rejected_bad_runtime_path_read,
        "runtime replay artifact path index rejects persisted "
        "parent-directory observation index paths");
  auto loaded_batch =
      replay::load_graph_anchor_edge_batch_artifact<std::int64_t>(
          indexed_artifact_paths.graph_anchor_edge_batch_artifact_path);
  check(loaded_batch.cursor.anchor_count() == cursor.anchor_count(),
        "runtime replay source loads graph-anchor edge batch artifact");
  auto runtime_record_from_file =
      replay::make_runtime_graph_anchor_replay_record_from_job_dir<
          std::int64_t>(runtime_fixture_dir, runtime_base_spec,
                        replay::replay_frame_build_options_t{},
                        projection_required_bundle_world_options,
                        replay::replay_observation_artifact_options_t{},
                        /*require_observation_artifacts=*/true);
  check(runtime_record_from_file.cursor.anchor_count() == cursor.anchor_count(),
        "runtime replay job dir still supports single-batch path index");
  auto runtime_records_from_pulses =
      replay::make_runtime_graph_anchor_replay_records_from_job_dir<
          std::int64_t>(runtime_fixture_dir, runtime_base_spec,
                        replay::replay_frame_build_options_t{},
                        projection_required_bundle_world_options,
                        replay::replay_observation_artifact_options_t{},
                        /*require_observation_artifacts=*/true);
  check(runtime_records_from_pulses.size() == 1,
        "runtime replay job dir expands pulse batch index into records");

  replay::runtime_graph_anchor_replay_bundle_source_t<std::int64_t>
      runtime_source("runtime_graph_anchor_source_fixture", graph,
                     std::move(runtime_records_from_pulses));
  auto runtime_bundle = runtime_source.next_bundle();
  check(runtime_bundle.has_value(),
        "runtime replay source materializes replay bundle");
  check(runtime_bundle->spec.runtime_run_id == "runtime_job_fixture",
        "runtime replay source carries manifest job id");
  check(runtime_bundle->frames[0].observation.allocation_belief.has_value(),
        "runtime replay source attaches allocation artifact");
  runtime_source.reset();
  auto runtime_experiment = env::run_replay_experiment(
      "runtime_stream_allocation_compare", runtime_source,
      std::vector{make_sdu_policy_factory()}, source_experiment_options);
  check(runtime_experiment.completed_count == 1,
        "runtime replay source runs deterministic allocation policy");

  auto runtime_source_from_job_dir =
      replay::make_runtime_graph_anchor_replay_bundle_source_from_job_dir<
          std::int64_t>("runtime_graph_anchor_job_dir_source_fixture",
                        runtime_fixture_dir, graph, runtime_base_spec,
                        replay::replay_frame_build_options_t{},
                        projection_required_bundle_world_options,
                        replay::replay_observation_artifact_options_t{},
                        /*require_observation_artifacts=*/true);
  auto job_dir_runtime_experiment = env::run_replay_experiment(
      "runtime_job_dir_allocation_compare", runtime_source_from_job_dir,
      std::vector{make_sdu_policy_factory()}, source_experiment_options);
  check(job_dir_runtime_experiment.completed_count == 1,
        "runtime replay job-dir source runs deterministic allocation policy");

  auto bad_runtime_record = runtime_record;
  bad_runtime_record.manifest.graph_order_fingerprint = "wrong_graph";
  bool rejected_bad_runtime_graph = false;
  try {
    replay::runtime_graph_anchor_replay_bundle_source_t<std::int64_t>
        bad_runtime_source("bad_runtime_graph_source_fixture", graph,
                           std::vector{bad_runtime_record});
    (void)bad_runtime_source.next_bundle();
  } catch (const std::exception &) {
    rejected_bad_runtime_graph = true;
  }
  check(rejected_bad_runtime_graph,
        "runtime replay source rejects manifest graph mismatch");

  auto bad_spec = spec;
  bad_spec.risky_node_ids = {"SOL"};
  bool rejected_missing_direct_edge = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        batch, graph, bad_spec);
  } catch (const std::exception &) {
    rejected_missing_direct_edge = true;
  }
  check(rejected_missing_direct_edge,
        "replay source rejects missing direct asset/base edge");

  auto bad_fingerprint_spec = spec;
  bad_fingerprint_spec.graph_order_fingerprint = "wrong_graph";
  bool rejected_bad_graph_fingerprint = false;
  try {
    (void)replay::realized_log_returns_from_graph_anchor_edge_batch(
        batch, graph, bad_fingerprint_spec);
  } catch (const std::exception &) {
    rejected_bad_graph_fingerprint = true;
  }
  check(rejected_bad_graph_fingerprint,
        "replay source rejects graph fingerprint mismatch");
}

} // namespace

int main() {
  try {
    test_environment_contract();
    test_baseline_policies();
    test_spot_distributional_utility_policy_adapter();
    test_replay_world();
    test_replay_source_graph_anchor_binding();
    std::cout << "kikijyeba environment contract tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
