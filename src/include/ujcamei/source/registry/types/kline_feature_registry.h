// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cuwacunu::ujcamei::source::registry::types {

inline constexpr std::int64_t kKlineFeatureWidth = 9;

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

enum class kline_feature_group_e : std::uint8_t {
  price,
  activity,
};

enum class kline_normalization_rule_e : std::uint8_t {
  log_return_to_previous_close,
  log1p,
};

struct kline_feature_descriptor_t {
  kline_feature_e feature;
  std::string_view name;
  std::int64_t coord;
  kline_feature_group_e group;
  kline_normalization_rule_e normalization_rule;
};

inline constexpr std::array<kline_feature_descriptor_t,
                            static_cast<std::size_t>(kKlineFeatureWidth)>
    kKlineFeatureRegistry{{
        {kline_feature_e::open, "open", 0, kline_feature_group_e::price,
         kline_normalization_rule_e::log_return_to_previous_close},
        {kline_feature_e::high, "high", 1, kline_feature_group_e::price,
         kline_normalization_rule_e::log_return_to_previous_close},
        {kline_feature_e::low, "low", 2, kline_feature_group_e::price,
         kline_normalization_rule_e::log_return_to_previous_close},
        {kline_feature_e::close, "close", 3, kline_feature_group_e::price,
         kline_normalization_rule_e::log_return_to_previous_close},
        {kline_feature_e::volume, "volume", 4, kline_feature_group_e::activity,
         kline_normalization_rule_e::log1p},
        {kline_feature_e::quote_volume, "quote_volume", 5,
         kline_feature_group_e::activity, kline_normalization_rule_e::log1p},
        {kline_feature_e::trades, "trades", 6, kline_feature_group_e::activity,
         kline_normalization_rule_e::log1p},
        {kline_feature_e::taker_buy_base, "taker_buy_base", 7,
         kline_feature_group_e::activity, kline_normalization_rule_e::log1p},
        {kline_feature_e::taker_buy_quote, "taker_buy_quote", 8,
         kline_feature_group_e::activity, kline_normalization_rule_e::log1p},
    }};

[[nodiscard]] constexpr std::int64_t
kline_feature_index(kline_feature_e feature) {
  return static_cast<std::int64_t>(feature);
}

[[nodiscard]] constexpr bool kline_feature_registry_is_complete() {
  std::array<bool, static_cast<std::size_t>(kKlineFeatureWidth)> seen{};
  for (const auto &entry : kKlineFeatureRegistry) {
    if (entry.coord < 0 || entry.coord >= kKlineFeatureWidth) {
      return false;
    }
    const auto coord = static_cast<std::size_t>(entry.coord);
    if (seen[coord]) {
      return false;
    }
    seen[coord] = true;
    if (entry.coord != kline_feature_index(entry.feature)) {
      return false;
    }
  }
  for (const bool present : seen) {
    if (!present) {
      return false;
    }
  }
  return true;
}

static_assert(kline_feature_registry_is_complete(),
              "kline feature registry must cover every coord exactly once");

[[nodiscard]] constexpr std::size_t
kline_feature_count(kline_feature_group_e group) {
  std::size_t count = 0;
  for (const auto &entry : kKlineFeatureRegistry) {
    if (entry.group == group) {
      ++count;
    }
  }
  return count;
}

inline constexpr std::size_t kKlinePriceFeatureCount =
    kline_feature_count(kline_feature_group_e::price);
inline constexpr std::size_t kKlineActivityFeatureCount =
    kline_feature_count(kline_feature_group_e::activity);
inline constexpr std::int64_t kKlinePriceFeatureWidth =
    static_cast<std::int64_t>(kKlinePriceFeatureCount);
inline constexpr std::int64_t kKlineActivityFeatureWidth =
    static_cast<std::int64_t>(kKlineActivityFeatureCount);

template <std::size_t N>
[[nodiscard]] constexpr std::array<std::int64_t, N>
kline_feature_coords_for_group(kline_feature_group_e group) {
  std::array<std::int64_t, N> out{};
  std::size_t index = 0;
  for (const auto &entry : kKlineFeatureRegistry) {
    if (entry.group != group) {
      continue;
    }
    if (index < N) {
      out[index] = entry.coord;
    }
    ++index;
  }
  return out;
}

inline constexpr std::array<std::int64_t, kKlinePriceFeatureCount>
    kKlinePriceFeatureCoords =
        kline_feature_coords_for_group<kKlinePriceFeatureCount>(
            kline_feature_group_e::price);

inline constexpr std::array<std::int64_t, kKlineActivityFeatureCount>
    kKlineActivityFeatureCoords =
        kline_feature_coords_for_group<kKlineActivityFeatureCount>(
            kline_feature_group_e::activity);

[[nodiscard]] constexpr const kline_feature_descriptor_t *
find_kline_feature_descriptor(kline_feature_e feature) {
  for (const auto &entry : kKlineFeatureRegistry) {
    if (entry.feature == feature) {
      return &entry;
    }
  }
  return nullptr;
}

[[nodiscard]] constexpr const kline_feature_descriptor_t *
find_kline_feature_descriptor_at(std::int64_t coord) {
  for (const auto &entry : kKlineFeatureRegistry) {
    if (entry.coord == coord) {
      return &entry;
    }
  }
  return nullptr;
}

[[nodiscard]] constexpr std::string_view
kline_feature_name(kline_feature_e feature) {
  if (const auto *entry = find_kline_feature_descriptor(feature)) {
    return entry->name;
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view
kline_feature_group_name(kline_feature_group_e group) {
  switch (group) {
  case kline_feature_group_e::price:
    return "price";
  case kline_feature_group_e::activity:
    return "activity";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view
kline_normalization_rule_name(kline_normalization_rule_e rule) {
  switch (rule) {
  case kline_normalization_rule_e::log_return_to_previous_close:
    return "log_return_to_previous_close";
  case kline_normalization_rule_e::log1p:
    return "log1p";
  }
  return "unknown";
}

} // namespace cuwacunu::ujcamei::source::registry::types
