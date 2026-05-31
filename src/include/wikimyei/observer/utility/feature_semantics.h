// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::observer {

namespace registry = cuwacunu::ujcamei::source::registry::types;
namespace assembly = cuwacunu::wikimyei::assembly;

inline constexpr std::string_view kKlineFeatureSemanticsId =
    "ujcamei.source.registry.types.kline_feature_registry.v1";
inline constexpr std::string_view kGraphOrderKlineCoordinateSpace =
    "graph_order.kline.v1";
inline constexpr std::string_view
    kChannelNodeFutureDistributionCoordinateSpace =
        "graph_order.channel_node_future_distribution.v1";

struct feature_descriptor_t {
  std::string name{};
  std::int64_t coord{-1};
  std::string group{};
  std::string normalization_rule{};
};

struct feature_semantics_t {
  std::string semantics_id{};
  std::string coordinate_space{};
  std::int64_t width{0};
  std::vector<feature_descriptor_t> descriptors{};
  std::string fingerprint{};
};

namespace detail {

inline constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnvPrime = 1099511628211ull;

inline void mix_hash_byte(std::uint64_t &hash, unsigned char byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= kFnvPrime;
}

inline void mix_hash_string(std::uint64_t &hash, std::string_view value) {
  for (const unsigned char c : value) {
    mix_hash_byte(hash, c);
  }
  mix_hash_byte(hash, 0xffu);
}

[[nodiscard]] inline std::string hash_hex(std::uint64_t hash) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

} // namespace detail

[[nodiscard]] constexpr std::int64_t
kline_coord(registry::kline_feature_e feature) {
  return registry::kline_feature_index(feature);
}

[[nodiscard]] inline std::string canonical_kline_feature_semantics_text() {
  std::ostringstream out;
  out << "semantics_id=" << kKlineFeatureSemanticsId << "\n";
  out << "width=" << registry::kKlineFeatureWidth << "\n";
  for (const auto &entry : registry::kKlineFeatureRegistry) {
    out << "feature=" << entry.coord << "|" << entry.name << "|"
        << registry::kline_feature_group_name(entry.group) << "|"
        << registry::kline_normalization_rule_name(entry.normalization_rule)
        << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string kline_feature_semantics_fingerprint() {
  std::uint64_t hash = detail::kFnvOffsetBasis;
  detail::mix_hash_string(hash, "cuwacunu.wikimyei.feature_semantics.v1");
  detail::mix_hash_string(hash, canonical_kline_feature_semantics_text());
  return detail::hash_hex(hash);
}

[[nodiscard]] inline feature_semantics_t
make_kline_feature_semantics(std::string coordinate_space =
                                 std::string(kGraphOrderKlineCoordinateSpace)) {
  feature_semantics_t out{};
  out.semantics_id = std::string(kKlineFeatureSemanticsId);
  out.coordinate_space = std::move(coordinate_space);
  out.width = registry::kKlineFeatureWidth;
  out.fingerprint = kline_feature_semantics_fingerprint();
  out.descriptors.reserve(registry::kKlineFeatureRegistry.size());
  for (const auto &entry : registry::kKlineFeatureRegistry) {
    out.descriptors.push_back(
        {.name = std::string(entry.name),
         .coord = entry.coord,
         .group = std::string(registry::kline_feature_group_name(entry.group)),
         .normalization_rule =
             std::string(registry::kline_normalization_rule_name(
                 entry.normalization_rule))});
  }
  return out;
}

[[nodiscard]] inline bool
dock_has_kline_feature_semantics(const assembly::dock_t &dock) {
  return dock.coordinate_space == kGraphOrderKlineCoordinateSpace ||
         dock.coordinate_space == kChannelNodeFutureDistributionCoordinateSpace;
}

[[nodiscard]] inline std::optional<feature_semantics_t>
detect_feature_semantics_from_dock(const assembly::dock_t &dock) {
  if (!dock_has_kline_feature_semantics(dock)) {
    return std::nullopt;
  }
  return make_kline_feature_semantics(dock.coordinate_space);
}

[[nodiscard]] inline feature_semantics_t
require_feature_semantics_from_dock(const assembly::dock_t &dock) {
  auto detected = detect_feature_semantics_from_dock(dock);
  if (!detected.has_value()) {
    throw std::runtime_error(
        "[feature_semantics] unsupported dock coordinate_space: " +
        dock.coordinate_space);
  }
  return *detected;
}

} // namespace cuwacunu::wikimyei::observer
