// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/spot_rebalance_router.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    accounting::portfolio_ledger {

namespace execution = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::execution::spot_rebalance_router;
namespace portfolio = cuwacunu::wikimyei::policy::portfolio;

enum class fill_side_t {
  buy_asset,
  sell_asset,
};

[[nodiscard]] inline const char *fill_side_name(fill_side_t side) {
  switch (side) {
  case fill_side_t::buy_asset:
    return "buy_asset";
  case fill_side_t::sell_asset:
    return "sell_asset";
  }
  return "unknown";
}

struct executed_fill_t {
  portfolio::timestamp_ms_t timestamp_ms{0};
  std::string node_id{};
  std::string edge_id{};
  fill_side_t side{fill_side_t::buy_asset};

  double quantity_asset{0.0};
  double gross_notional_base{0.0};
  double fee_base{0.0};
};

struct portfolio_ledger_options_t {
  bool allow_negative_base_reserve{false};
  double dust_quantity{1.0e-12};
};

struct portfolio_ledger_t {
  portfolio::timestamp_ms_t timestamp_ms{0};
  portfolio::node_id_t base_reserve_node_id{};
  std::vector<portfolio::node_id_t> node_ids{};

  torch::Tensor units{};             // [A]
  torch::Tensor average_cost_base{}; // [A], base per unit including buy fees
  torch::Tensor realized_pnl_base{}; // [A]

  double base_reserve_units{0.0};
  double cumulative_fees_base{0.0};
  std::int64_t fill_count{0};
};

namespace detail {

inline void require_vector_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined()) {
    TORCH_CHECK(!required, "[portfolio_ledger] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A, "[portfolio_ledger] ",
              name, " must be [A]");
}

[[nodiscard]] inline std::int64_t
node_index_or_throw(const std::vector<portfolio::node_id_t> &node_ids,
                    const std::string &node_id) {
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(node_ids.size());
       ++i) {
    if (node_ids[static_cast<std::size_t>(i)] == node_id) {
      return i;
    }
  }
  throw std::out_of_range("[portfolio_ledger] unknown node_id: " + node_id);
}

[[nodiscard]] inline torch::Tensor
vector_or_zeros(const torch::Tensor &tensor, std::int64_t A,
                torch::TensorOptions options) {
  if (tensor.defined()) {
    require_vector_shape(tensor, A, "vector", true);
    return tensor.to(options);
  }
  return torch::zeros({A}, options);
}

} // namespace detail

inline void validate_ledger(const portfolio_ledger_t &ledger) {
  TORCH_CHECK(!ledger.base_reserve_node_id.empty(),
              "[portfolio_ledger] base_reserve_node_id required");
  const auto A = static_cast<std::int64_t>(ledger.node_ids.size());
  detail::require_vector_shape(ledger.units, A, "units", true);
  detail::require_vector_shape(ledger.average_cost_base, A, "average_cost_base",
                               true);
  detail::require_vector_shape(ledger.realized_pnl_base, A, "realized_pnl_base",
                               true);
  TORCH_CHECK(torch::isfinite(ledger.units).all().item<bool>(),
              "[portfolio_ledger] units contains non-finite values");
  TORCH_CHECK(
      torch::isfinite(ledger.average_cost_base).all().item<bool>(),
      "[portfolio_ledger] average_cost_base contains non-finite values");
  TORCH_CHECK(
      torch::isfinite(ledger.realized_pnl_base).all().item<bool>(),
      "[portfolio_ledger] realized_pnl_base contains non-finite values");
  TORCH_CHECK(std::isfinite(ledger.base_reserve_units),
              "[portfolio_ledger] base_reserve_units must be finite");
  TORCH_CHECK(std::isfinite(ledger.cumulative_fees_base),
              "[portfolio_ledger] cumulative_fees_base must be finite");
}

[[nodiscard]] inline portfolio_ledger_t
make_ledger(portfolio::timestamp_ms_t timestamp_ms,
            portfolio::node_id_t base_reserve_node_id,
            std::vector<portfolio::node_id_t> node_ids,
            double base_reserve_units, torch::Tensor units = torch::Tensor(),
            torch::Tensor average_cost_base = torch::Tensor(),
            torch::Tensor realized_pnl_base = torch::Tensor()) {
  TORCH_CHECK(!base_reserve_node_id.empty(),
              "[portfolio_ledger] base_reserve_node_id required");
  const auto A = static_cast<std::int64_t>(node_ids.size());
  auto options = torch::TensorOptions().dtype(torch::kFloat64);
  portfolio_ledger_t out{};
  out.timestamp_ms = timestamp_ms;
  out.base_reserve_node_id = std::move(base_reserve_node_id);
  out.node_ids = std::move(node_ids);
  out.units = detail::vector_or_zeros(units, A, options);
  out.average_cost_base =
      detail::vector_or_zeros(average_cost_base, A, options);
  out.realized_pnl_base =
      detail::vector_or_zeros(realized_pnl_base, A, options);
  out.base_reserve_units = base_reserve_units;
  validate_ledger(out);
  return out;
}

inline void apply_fill(portfolio_ledger_t &ledger, const executed_fill_t &fill,
                       const portfolio_ledger_options_t &options = {}) {
  validate_ledger(ledger);
  TORCH_CHECK(!fill.node_id.empty(),
              "[portfolio_ledger] fill node_id required");
  TORCH_CHECK(fill.quantity_asset > 0.0 && std::isfinite(fill.quantity_asset),
              "[portfolio_ledger] fill quantity_asset must be positive");
  TORCH_CHECK(fill.gross_notional_base >= 0.0 &&
                  std::isfinite(fill.gross_notional_base),
              "[portfolio_ledger] fill gross_notional_base invalid");
  TORCH_CHECK(fill.fee_base >= 0.0 && std::isfinite(fill.fee_base),
              "[portfolio_ledger] fill fee_base invalid");

  const auto i = detail::node_index_or_throw(ledger.node_ids, fill.node_id);
  auto units = ledger.units.to(torch::kFloat64).clone();
  auto avg = ledger.average_cost_base.to(torch::kFloat64).clone();
  auto realized = ledger.realized_pnl_base.to(torch::kFloat64).clone();

  const double old_units = units.index({i}).item<double>();
  const double old_avg = avg.index({i}).item<double>();
  if (fill.side == fill_side_t::buy_asset) {
    const double base_reserve_out = fill.gross_notional_base + fill.fee_base;
    if (!options.allow_negative_base_reserve &&
        ledger.base_reserve_units + 1.0e-9 < base_reserve_out) {
      throw std::runtime_error(
          "[portfolio_ledger] insufficient base reserve for buy");
    }
    const double new_units = old_units + fill.quantity_asset;
    const double new_cost =
        old_units * old_avg + fill.gross_notional_base + fill.fee_base;
    units.index_put_({i}, new_units);
    avg.index_put_({i}, new_units > options.dust_quantity ? new_cost / new_units
                                                          : 0.0);
    ledger.base_reserve_units -= base_reserve_out;
  } else {
    if (old_units + options.dust_quantity < fill.quantity_asset) {
      throw std::runtime_error(
          "[portfolio_ledger] insufficient units for sell");
    }
    const double sell_quantity = std::min(old_units, fill.quantity_asset);
    const double proceeds = fill.gross_notional_base - fill.fee_base;
    const double pnl = proceeds - old_avg * sell_quantity;
    const double new_units = old_units - sell_quantity;
    units.index_put_({i}, std::max(0.0, new_units));
    if (new_units <= options.dust_quantity) {
      avg.index_put_({i}, 0.0);
    }
    realized.index_put_({i}, realized.index({i}).item<double>() + pnl);
    ledger.base_reserve_units += proceeds;
  }

  ledger.timestamp_ms = std::max(ledger.timestamp_ms, fill.timestamp_ms);
  ledger.units = std::move(units);
  ledger.average_cost_base = std::move(avg);
  ledger.realized_pnl_base = std::move(realized);
  ledger.cumulative_fees_base += fill.fee_base;
  ++ledger.fill_count;
  validate_ledger(ledger);
}

[[nodiscard]] inline executed_fill_t fill_from_routed_order_intent(
    const execution::routed_order_intent_t &intent, double asset_price_base,
    double executed_fraction = 1.0, double fee_base = -1.0,
    portfolio::timestamp_ms_t timestamp_ms = 0) {
  TORCH_CHECK(asset_price_base > 0.0 && std::isfinite(asset_price_base),
              "[portfolio_ledger] asset_price_base must be positive");
  TORCH_CHECK(executed_fraction >= 0.0 && executed_fraction <= 1.0,
              "[portfolio_ledger] executed_fraction must be in [0,1]");
  const bool asset_is_edge_base = intent.edge_base_node_id == intent.node_id;
  const bool asset_is_edge_quote = intent.edge_quote_node_id == intent.node_id;
  TORCH_CHECK(asset_is_edge_base || asset_is_edge_quote,
              "[portfolio_ledger] intent node_id is not an edge endpoint");
  const bool buy_asset =
      (asset_is_edge_base &&
       intent.side == execution::order_side_t::buy_edge_base) ||
      (asset_is_edge_quote &&
       intent.side == execution::order_side_t::sell_edge_base);
  const double notional = intent.routed_notional_base * executed_fraction;
  executed_fill_t fill{};
  fill.timestamp_ms = timestamp_ms;
  fill.node_id = intent.node_id;
  fill.edge_id = intent.edge_id;
  fill.side = buy_asset ? fill_side_t::buy_asset : fill_side_t::sell_asset;
  fill.quantity_asset = notional / asset_price_base;
  fill.gross_notional_base = notional;
  fill.fee_base = fee_base >= 0.0
                      ? fee_base
                      : intent.estimated_cost_base * executed_fraction;
  return fill;
}

[[nodiscard]] inline portfolio::PortfolioState
mark_to_market(const portfolio_ledger_t &ledger,
               const torch::Tensor &asset_price_base,
               portfolio::timestamp_ms_t timestamp_ms) {
  validate_ledger(ledger);
  const auto A = static_cast<std::int64_t>(ledger.node_ids.size());
  detail::require_vector_shape(asset_price_base, A, "asset_price_base", true);
  auto options = torch::TensorOptions().dtype(torch::kFloat64);
  auto prices = asset_price_base.to(options);
  TORCH_CHECK((prices > 0.0).all().item<bool>(),
              "[portfolio_ledger] asset prices must be positive");
  auto units = ledger.units.to(options);
  auto values = units * prices;
  const double risky_value = values.sum().item<double>();
  const double equity = ledger.base_reserve_units + risky_value;
  TORCH_CHECK(equity > 0.0, "[portfolio_ledger] equity must be positive");
  auto weights = values / equity;
  auto unrealized = values - units * ledger.average_cost_base.to(options);

  portfolio::PortfolioState out{};
  out.timestamp_ms = timestamp_ms;
  out.accounting_node_id = ledger.base_reserve_node_id;
  out.reserve_node_id = ledger.base_reserve_node_id;
  out.current_weights = std::move(weights);
  out.current_units = units.clone();
  out.base_reserve_weight = ledger.base_reserve_units / equity;
  out.equity_value_base = equity;
  out.unrealized_pnl = std::move(unrealized);
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::accounting::portfolio_ledger
