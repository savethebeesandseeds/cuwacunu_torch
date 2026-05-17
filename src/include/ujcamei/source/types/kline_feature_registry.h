// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace cuwacunu::ujcamei::source::types {

inline constexpr std::int64_t kKlineFeatureWidth = 9;
inline constexpr std::int64_t kKlinePriceFeatureWidth = 4;
inline constexpr std::int64_t kKlineActivityFeatureWidth = 5;

enum class kline_feature_e : std::int64_t {
  open = 0,
  high = 1,
  low = 2,
  close = 3,
  volume = 4,
  quote_volume = 5,
  trades = 6,
  taker_buy_base = 7,
  taker_buy_quote = 8,
};

inline constexpr std::array<std::int64_t, kKlinePriceFeatureWidth>
    kKlinePriceFeatureCoords{0, 1, 2, 3};

inline constexpr std::array<std::int64_t, kKlineActivityFeatureWidth>
    kKlineActivityFeatureCoords{4, 5, 6, 7, 8};

[[nodiscard]] constexpr std::int64_t
kline_feature_index(kline_feature_e feature) {
  return static_cast<std::int64_t>(feature);
}

[[nodiscard]] constexpr std::string_view
kline_feature_name(kline_feature_e feature) {
  switch (feature) {
  case kline_feature_e::open:
    return "open";
  case kline_feature_e::high:
    return "high";
  case kline_feature_e::low:
    return "low";
  case kline_feature_e::close:
    return "close";
  case kline_feature_e::volume:
    return "volume";
  case kline_feature_e::quote_volume:
    return "quote_volume";
  case kline_feature_e::trades:
    return "trades";
  case kline_feature_e::taker_buy_base:
    return "taker_buy_base";
  case kline_feature_e::taker_buy_quote:
    return "taker_buy_quote";
  }
  return "unknown";
}

} // namespace cuwacunu::ujcamei::source::types
