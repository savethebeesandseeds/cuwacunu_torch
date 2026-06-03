// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

#include <torch/torch.h>

#include "wikimyei/observer/utility/channel_consensus.h"
#include "wikimyei/observer/utility/feature_semantics.h"

namespace cuwacunu::wikimyei::observer {

struct flow_liquidity_options_t {
  std::int64_t volume_coord{kline_coord(registry::kline_feature_e::volume)};
  std::int64_t quote_volume_coord{
      kline_coord(registry::kline_feature_e::quote_volume)};
  std::int64_t trades_coord{kline_coord(registry::kline_feature_e::trades)};
  std::int64_t taker_buy_base_coord{
      kline_coord(registry::kline_feature_e::taker_buy_base)};
  std::int64_t taker_buy_quote_coord{
      kline_coord(registry::kline_feature_e::taker_buy_quote)};
  bool neutral_when_missing{false};
  double neutral_liquidity_score{1.0};
  double neutral_capacity_weight_limit{1.0};
};

struct flow_liquidity_t {
  torch::Tensor activity_score{};        // [B,N], [0,1]
  torch::Tensor flow_pressure{};         // [B,N], roughly [-1,1]
  torch::Tensor liquidity_score{};       // [B,N], [0,1]
  torch::Tensor capacity_weight_limit{}; // [B,N], [0,1]
};

inline void require_coord(const channel_consensus_t &consensus,
                          std::int64_t coord, const char *name) {
  TORCH_CHECK(coord >= 0 && coord < consensus.combined_mean.size(2),
              "[flow_liquidity] ", name, " out of range");
}

[[nodiscard]] inline bool has_coord(const channel_consensus_t &consensus,
                                    std::int64_t coord) {
  return coord >= 0 && coord < consensus.combined_mean.size(2);
}

[[nodiscard]] inline flow_liquidity_t
neutral_flow_liquidity(const channel_consensus_t &consensus,
                       const flow_liquidity_options_t &options) {
  const auto B = consensus.combined_mean.size(0);
  const auto N = consensus.combined_mean.size(1);
  auto tensor_options = consensus.combined_mean.options();
  auto liquidity_score =
      torch::full({B, N}, options.neutral_liquidity_score, tensor_options)
          .clamp(0.0, 1.0);
  auto capacity_weight_limit =
      torch::full({B, N}, options.neutral_capacity_weight_limit, tensor_options)
          .clamp(0.0, 1.0);
  auto activity_score = liquidity_score.clone();
  auto flow_pressure = torch::zeros({B, N}, tensor_options);

  if (consensus.active_mask.defined()) {
    auto active = consensus.active_mask.any(/*dim=*/2);
    activity_score = activity_score.masked_fill(active.logical_not(), 0.0);
    flow_pressure = flow_pressure.masked_fill(active.logical_not(), 0.0);
    liquidity_score = liquidity_score.masked_fill(active.logical_not(), 0.0);
    capacity_weight_limit =
        capacity_weight_limit.masked_fill(active.logical_not(), 0.0);
  }

  flow_liquidity_t result{};
  result.activity_score = std::move(activity_score);
  result.flow_pressure = std::move(flow_pressure);
  result.liquidity_score = std::move(liquidity_score);
  result.capacity_weight_limit = std::move(capacity_weight_limit);
  return result;
}

[[nodiscard]] inline flow_liquidity_t
compute_flow_liquidity(const channel_consensus_t &consensus,
                       const flow_liquidity_options_t &options = {}) {
  TORCH_CHECK(consensus.combined_mean.defined() &&
                  consensus.combined_mean.dim() == 3,
              "[flow_liquidity] combined_mean must be [B,N,Df]");
  if (options.neutral_when_missing &&
      (!has_coord(consensus, options.volume_coord) ||
       !has_coord(consensus, options.quote_volume_coord) ||
       !has_coord(consensus, options.trades_coord) ||
       !has_coord(consensus, options.taker_buy_base_coord) ||
       !has_coord(consensus, options.taker_buy_quote_coord))) {
    return neutral_flow_liquidity(consensus, options);
  }
  require_coord(consensus, options.volume_coord, "volume_coord");
  require_coord(consensus, options.quote_volume_coord, "quote_volume_coord");
  require_coord(consensus, options.trades_coord, "trades_coord");
  require_coord(consensus, options.taker_buy_base_coord,
                "taker_buy_base_coord");
  require_coord(consensus, options.taker_buy_quote_coord,
                "taker_buy_quote_coord");

  auto volume = consensus.combined_mean.select(2, options.volume_coord);
  auto quote = consensus.combined_mean.select(2, options.quote_volume_coord);
  auto trades = consensus.combined_mean.select(2, options.trades_coord);
  auto taker_base =
      consensus.combined_mean.select(2, options.taker_buy_base_coord);
  auto taker_quote =
      consensus.combined_mean.select(2, options.taker_buy_quote_coord);

  auto activity_score = torch::sigmoid((volume + quote + trades) / 3.0);
  auto denom = (volume.abs() + quote.abs()).clamp_min(1.0e-6);
  auto flow_pressure = ((taker_base + taker_quote) / denom).tanh();
  auto liquidity_score = activity_score.clamp(0.0, 1.0);
  auto capacity_weight_limit = (0.02 + 0.18 * liquidity_score).clamp(0.0, 1.0);

  if (consensus.active_mask.defined()) {
    auto active = consensus.active_mask.any(/*dim=*/2);
    activity_score = activity_score.masked_fill(active.logical_not(), 0.0);
    flow_pressure = flow_pressure.masked_fill(active.logical_not(), 0.0);
    liquidity_score = liquidity_score.masked_fill(active.logical_not(), 0.0);
    capacity_weight_limit =
        capacity_weight_limit.masked_fill(active.logical_not(), 0.0);
  }

  flow_liquidity_t result{};
  result.activity_score = std::move(activity_score);
  result.flow_pressure = std::move(flow_pressure);
  result.liquidity_score = std::move(liquidity_score);
  result.capacity_weight_limit = std::move(capacity_weight_limit);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
