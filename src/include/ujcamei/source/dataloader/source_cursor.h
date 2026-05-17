// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ujcamei/source/instrument_signature.h"

namespace cuwacunu::ujcamei::source::dataloader {

inline constexpr std::string_view kSourceRuntimeCursorVersion =
    "ujcamei.source_runtime_cursor.v1";
inline constexpr std::string_view kGraphAnchorCursorReportVersion =
    "ujcamei.graph_anchor_cursor_report.v1";

[[nodiscard]] inline std::string trim_cursor_token(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

// Stable source-range selector. It is runtime/reporting metadata, not authored
// source schema. Legacy reports used the same idea to partition analytics and
// source projection artifacts by selected instrument + source range.
struct source_runtime_cursor_t {
  cuwacunu::ujcamei::source::instrument_signature_t source_signature{};
  std::string requested_from{};
  std::string requested_to{};
  std::string cursor_token{};

  [[nodiscard]] bool empty() const { return cursor_token.empty(); }
};

[[nodiscard]] inline std::string make_source_runtime_cursor_token(
    const cuwacunu::ujcamei::source::instrument_signature_t &signature,
    std::string_view requested_from, std::string_view requested_to) {
  const std::string symbol = trim_cursor_token(signature.symbol);
  const std::string record_type = trim_cursor_token(signature.record_type);
  const std::string market_type = trim_cursor_token(signature.market_type);
  const std::string venue = trim_cursor_token(signature.venue);
  const std::string base_asset = trim_cursor_token(signature.base_asset);
  const std::string quote_asset = trim_cursor_token(signature.quote_asset);
  const std::string from = trim_cursor_token(requested_from);
  const std::string to = trim_cursor_token(requested_to);

  if (symbol.empty() || record_type.empty() || market_type.empty() ||
      venue.empty() || base_asset.empty() || quote_asset.empty() ||
      from.empty() || to.empty()) {
    return {};
  }

  std::string out;
  out.reserve(symbol.size() + record_type.size() + market_type.size() +
              venue.size() + base_asset.size() + quote_asset.size() +
              from.size() + to.size() + 7);
  out.append(symbol);
  out.push_back('|');
  out.append(record_type);
  out.push_back('|');
  out.append(market_type);
  out.push_back('|');
  out.append(venue);
  out.push_back('|');
  out.append(base_asset);
  out.push_back('|');
  out.append(quote_asset);
  out.push_back('|');
  out.append(from);
  out.push_back('|');
  out.append(to);
  return out;
}

[[nodiscard]] inline source_runtime_cursor_t make_source_runtime_cursor(
    cuwacunu::ujcamei::source::instrument_signature_t source_signature,
    std::string requested_from, std::string requested_to) {
  source_runtime_cursor_t out{};
  out.source_signature = std::move(source_signature);
  out.requested_from = trim_cursor_token(requested_from);
  out.requested_to = trim_cursor_token(requested_to);
  out.cursor_token = make_source_runtime_cursor_token(
      out.source_signature, out.requested_from, out.requested_to);
  return out;
}

template <typename KeyT> struct graph_anchor_cursor_t {
  std::string graph_order_fingerprint{};
  std::size_t begin_anchor_index{0};
  std::size_t end_anchor_index{0};
  std::size_t requested_batch_size{0};
  std::vector<KeyT> anchor_keys{};

  [[nodiscard]] std::size_t anchor_count() const { return anchor_keys.size(); }
  [[nodiscard]] bool empty() const { return anchor_keys.empty(); }

  [[nodiscard]] std::optional<KeyT> first_anchor_key() const {
    if (anchor_keys.empty()) {
      return std::nullopt;
    }
    return anchor_keys.front();
  }

  [[nodiscard]] std::optional<KeyT> last_anchor_key() const {
    if (anchor_keys.empty()) {
      return std::nullopt;
    }
    return anchor_keys.back();
  }

  [[nodiscard]] std::string cursor_token() const {
    std::ostringstream oss;
    oss << "graph=" << graph_order_fingerprint
        << "|begin=" << begin_anchor_index << "|end=" << end_anchor_index
        << "|requested_batch=" << requested_batch_size
        << "|anchors=" << anchor_keys.size();
    if (!anchor_keys.empty()) {
      oss << "|first=" << anchor_keys.front() << "|last=" << anchor_keys.back();
    }
    return oss.str();
  }
};

template <typename KeyT> struct graph_anchor_cursor_report_t {
  std::string version{std::string(kGraphAnchorCursorReportVersion)};
  std::string graph_order_fingerprint{};
  std::vector<std::string> edge_ids{};
  std::string reference_edge_id{};

  bool require_all_edges{true};
  bool include_future{true};
  bool require_future{true};
  bool validate_anchor_fetch{true};

  std::optional<KeyT> common_left_key{std::nullopt};
  std::optional<KeyT> common_right_key{std::nullopt};
  std::optional<KeyT> reference_left_key{std::nullopt};
  std::optional<KeyT> reference_right_key{std::nullopt};
  std::optional<KeyT> reference_key_step{std::nullopt};

  std::size_t candidate_anchor_count{0};
  std::size_t accepted_anchor_count{0};
  std::size_t skipped_outside_common_range{0};
  std::size_t skipped_missing_edge_coverage{0};
  std::size_t skipped_failed_fetch_probe{0};
  std::size_t duplicate_anchor_count{0};

  std::vector<KeyT> anchor_keys{};

  [[nodiscard]] std::size_t anchor_count() const {
    return accepted_anchor_count;
  }

  [[nodiscard]] std::size_t skipped_anchor_count() const {
    return skipped_outside_common_range + skipped_missing_edge_coverage +
           skipped_failed_fetch_probe + duplicate_anchor_count;
  }

  [[nodiscard]] bool empty() const { return accepted_anchor_count == 0; }

  [[nodiscard]] std::optional<KeyT> first_anchor_key() const {
    if (anchor_keys.empty()) {
      return std::nullopt;
    }
    return anchor_keys.front();
  }

  [[nodiscard]] std::optional<KeyT> last_anchor_key() const {
    if (anchor_keys.empty()) {
      return std::nullopt;
    }
    return anchor_keys.back();
  }

  [[nodiscard]] std::string cursor_token() const {
    std::ostringstream oss;
    oss << "version=" << version << "|graph=" << graph_order_fingerprint
        << "|reference_edge=" << reference_edge_id
        << "|edges=" << edge_ids.size() << "|accepted=" << accepted_anchor_count
        << "|candidates=" << candidate_anchor_count
        << "|skipped=" << skipped_anchor_count();
    if (!anchor_keys.empty()) {
      oss << "|first=" << anchor_keys.front() << "|last=" << anchor_keys.back();
    }
    return oss.str();
  }
};

template <typename KeyT>
[[nodiscard]] graph_anchor_cursor_t<KeyT> make_graph_anchor_cursor(
    std::string graph_order_fingerprint, std::size_t begin_anchor_index,
    std::size_t end_anchor_index, std::size_t requested_batch_size,
    std::vector<KeyT> anchor_keys) {
  graph_anchor_cursor_t<KeyT> out{};
  out.graph_order_fingerprint = std::move(graph_order_fingerprint);
  out.begin_anchor_index = begin_anchor_index;
  out.end_anchor_index = end_anchor_index;
  out.requested_batch_size = requested_batch_size;
  out.anchor_keys = std::move(anchor_keys);
  return out;
}

} // namespace cuwacunu::ujcamei::source::dataloader
