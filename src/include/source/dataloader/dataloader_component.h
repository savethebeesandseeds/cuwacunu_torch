// ./include/source/dataloader/dataloader_component.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <string>
#include <string_view>

#include "camahjucunu/dsl/instrument_signature.h"

namespace cuwacunu::source::dataloader {

inline constexpr std::string_view kCanonicalType = "tsi.source.dataloader";

[[nodiscard]] inline std::string trim_ascii_copy(std::string_view text) {
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

// Source runtime instances may still expose an instrument-qualified display
// name for logs/UI, but report ownership stays the stable canonical type token.
[[nodiscard]] inline std::string
make_display_instance_name(std::string_view instrument) {
  std::string out(kCanonicalType);
  if (!instrument.empty()) {
    out.push_back('.');
    out.append(instrument);
  }
  return out;
}

// Runtime reports keep canonical ownership in `canonical_path`; `source_label`
// only carries the configured source setting (for example a symbol).
[[nodiscard]] inline std::string
make_source_label(std::string_view instrument) {
  return trim_ascii_copy(instrument);
}

[[nodiscard]] inline std::string make_source_runtime_cursor(
    const cuwacunu::camahjucunu::instrument_signature_t &signature,
    std::string_view from_date_ddmmyyyy, std::string_view to_date_ddmmyyyy) {
  const std::string normalized_symbol = trim_ascii_copy(signature.symbol);
  const std::string normalized_record_type =
      trim_ascii_copy(signature.record_type);
  const std::string normalized_market_type =
      trim_ascii_copy(signature.market_type);
  const std::string normalized_venue = trim_ascii_copy(signature.venue);
  const std::string normalized_base_asset =
      trim_ascii_copy(signature.base_asset);
  const std::string normalized_quote_asset =
      trim_ascii_copy(signature.quote_asset);
  const std::string normalized_from = trim_ascii_copy(from_date_ddmmyyyy);
  const std::string normalized_to = trim_ascii_copy(to_date_ddmmyyyy);
  if (normalized_symbol.empty() || normalized_record_type.empty() ||
      normalized_market_type.empty() || normalized_venue.empty() ||
      normalized_base_asset.empty() || normalized_quote_asset.empty() ||
      normalized_from.empty() || normalized_to.empty()) {
    return {};
  }
  std::string out{};
  out.reserve(normalized_symbol.size() + normalized_record_type.size() +
              normalized_market_type.size() + normalized_venue.size() +
              normalized_base_asset.size() + normalized_quote_asset.size() +
              normalized_from.size() + normalized_to.size() + 7);
  out.append(normalized_symbol);
  out.push_back('|');
  out.append(normalized_record_type);
  out.push_back('|');
  out.append(normalized_market_type);
  out.push_back('|');
  out.append(normalized_venue);
  out.push_back('|');
  out.append(normalized_base_asset);
  out.push_back('|');
  out.append(normalized_quote_asset);
  out.push_back('|');
  out.append(normalized_from);
  out.push_back('|');
  out.append(normalized_to);
  return out;
}

} // namespace cuwacunu::source::dataloader
