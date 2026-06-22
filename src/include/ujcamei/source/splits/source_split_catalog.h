// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::ujcamei::source::splits {

struct source_anchor_interval_t {
  std::int64_t begin{0};
  std::int64_t end{0};

  [[nodiscard]] std::int64_t length() const { return end - begin; }
};

enum class source_split_role_t { unknown, train, validation, test };

enum class source_split_selector_kind_t {
  anchor_index_range,
  fraction_range,
};

[[nodiscard]] inline const char *
source_split_role_name(source_split_role_t role) {
  switch (role) {
  case source_split_role_t::unknown:
    return "unknown";
  case source_split_role_t::train:
    return "train";
  case source_split_role_t::validation:
    return "validation";
  case source_split_role_t::test:
    return "test";
  }
  return "unknown";
}

[[nodiscard]] inline source_split_role_t
parse_source_split_role(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value == "train") {
    return source_split_role_t::train;
  }
  if (value == "validation") {
    return source_split_role_t::validation;
  }
  if (value == "test") {
    return source_split_role_t::test;
  }
  if (value == "unknown") {
    return source_split_role_t::unknown;
  }
  throw std::runtime_error("[source_split] unknown ROLE: " + value);
}

[[nodiscard]] inline const char *
source_split_selector_kind_name(source_split_selector_kind_t kind) {
  switch (kind) {
  case source_split_selector_kind_t::anchor_index_range:
    return "anchor_index_range";
  case source_split_selector_kind_t::fraction_range:
    return "fraction_range";
  }
  return "anchor_index_range";
}

[[nodiscard]] inline source_split_selector_kind_t
parse_source_split_selector_kind(std::string value) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  value = kv::lowercase(kv::trim(std::move(value)));
  if (value == "anchor_index_range" || value == "anchor_index") {
    return source_split_selector_kind_t::anchor_index_range;
  }
  if (value == "fraction_range" || value == "fraction") {
    return source_split_selector_kind_t::fraction_range;
  }
  throw std::runtime_error("[source_split] unknown SELECTOR: " + value);
}

struct source_fraction_t {
  std::int64_t numerator{0};
  std::int64_t denominator{1};
};

struct source_fraction_range_t {
  source_fraction_t begin{};
  source_fraction_t end{1, 1};
};

struct source_split_t {
  std::string split_id{};
  source_split_role_t role{source_split_role_t::unknown};
  source_split_selector_kind_t selector_kind{
      source_split_selector_kind_t::anchor_index_range};
  source_anchor_interval_t anchor_range{};
  source_fraction_range_t fraction_range{};
  std::int64_t min_count{1};
};

struct source_split_catalog_t {
  std::string schema{"ujcamei.source.splits.v1"};
  std::string catalog_id{};
  std::string cursor_domain{"ujcamei.graph_anchor"};
  std::vector<source_split_t> splits{};

  [[nodiscard]] const source_split_t *
  find_split(const std::string &split_id) const {
    for (const auto &split : splits) {
      if (split.split_id == split_id) {
        return &split;
      }
    }
    return nullptr;
  }
};

namespace source_split_detail {

namespace fs = std::filesystem;
namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string read_text_file_or_throw(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[source_split] unable to open file: " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::int64_t
parse_nonnegative_i64(const kv::block_t &block, const std::string &key) {
  const auto value = kv::parse_i64(kv::required(block, key));
  if (value < 0) {
    throw std::runtime_error("[source_split] " + key + " must be non-negative");
  }
  return value;
}

[[nodiscard]] inline std::int64_t parse_positive_i64(const kv::block_t &block,
                                                     const std::string &key,
                                                     std::int64_t fallback) {
  const auto raw = kv::trim(kv::optional(block, key, std::to_string(fallback)));
  const auto value = kv::parse_i64(raw);
  if (value <= 0) {
    throw std::runtime_error("[source_split] " + key + " must be positive");
  }
  return value;
}

[[nodiscard]] inline bool has_key(const kv::block_t &block,
                                  const std::string &key) {
  return block.values.find(key) != block.values.end();
}

[[nodiscard]] inline source_fraction_t parse_fraction(std::string raw,
                                                      const std::string &key) {
  raw = kv::trim(std::move(raw));
  const auto slash = raw.find('/');
  if (slash == std::string::npos ||
      raw.find('/', slash + 1) != std::string::npos) {
    throw std::runtime_error("[source_split] " + key +
                             " must be a rational fraction like 65/100");
  }
  source_fraction_t out{};
  out.numerator = kv::parse_i64(raw.substr(0, slash));
  out.denominator = kv::parse_i64(raw.substr(slash + 1));
  if (out.numerator < 0) {
    throw std::runtime_error("[source_split] " + key +
                             " numerator must be non-negative");
  }
  if (out.denominator <= 0) {
    throw std::runtime_error("[source_split] " + key +
                             " denominator must be positive");
  }
  if (out.numerator > out.denominator) {
    throw std::runtime_error("[source_split] " + key +
                             " must be less than or equal to 1");
  }
  const auto divisor = std::gcd(out.numerator, out.denominator);
  if (divisor > 1) {
    out.numerator /= divisor;
    out.denominator /= divisor;
  }
  return out;
}

[[nodiscard]] inline bool fraction_less(const source_fraction_t &lhs,
                                        const source_fraction_t &rhs) {
  const auto left = static_cast<__int128>(lhs.numerator) *
                    static_cast<__int128>(rhs.denominator);
  const auto right = static_cast<__int128>(rhs.numerator) *
                     static_cast<__int128>(lhs.denominator);
  return left < right;
}

[[nodiscard]] inline std::int64_t
fraction_boundary_floor(const source_fraction_t &fraction,
                        std::int64_t accepted_anchor_count) {
  if (accepted_anchor_count < 0) {
    throw std::runtime_error(
        "[source_split] accepted anchor count must be non-negative");
  }
  const auto product = static_cast<__int128>(accepted_anchor_count) *
                       static_cast<__int128>(fraction.numerator);
  const auto cut = product / static_cast<__int128>(fraction.denominator);
  if (cut > static_cast<__int128>(std::numeric_limits<std::int64_t>::max())) {
    throw std::runtime_error("[source_split] materialized fraction boundary "
                             "exceeds int64 range");
  }
  return static_cast<std::int64_t>(cut);
}

[[nodiscard]] inline std::string
fraction_text(const source_fraction_t &fraction) {
  return std::to_string(fraction.numerator) + "/" +
         std::to_string(fraction.denominator);
}

[[nodiscard]] inline std::string hex_u64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

inline void mix_hash_string(std::uint64_t &hash, std::string_view value) {
  constexpr std::uint64_t kFnvPrime = 1099511628211ull;
  for (const unsigned char c : value) {
    hash ^= static_cast<std::uint64_t>(c);
    hash *= kFnvPrime;
  }
}

} // namespace source_split_detail

[[nodiscard]] inline source_split_catalog_t
decode_source_split_catalog_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  source_split_catalog_t out{};
  bool saw_catalog = false;
  std::vector<std::string> split_ids;
  for (const auto &block : kv::parse_blocks(dsl_text)) {
    if (block.name == "UJCAMEI_SOURCE_SPLIT_CATALOG") {
      if (saw_catalog) {
        throw std::runtime_error(
            "[source_split] only one UJCAMEI_SOURCE_SPLIT_CATALOG block is "
            "allowed");
      }
      saw_catalog = true;
      out.catalog_id = kv::required(block, "CATALOG_ID");
      out.cursor_domain =
          kv::optional(block, "CURSOR_DOMAIN", out.cursor_domain);
      continue;
    }
    if (block.name != "UJCAMEI_SOURCE_SPLIT") {
      throw std::runtime_error("[source_split] unexpected block: " +
                               block.name);
    }
    source_split_t split{};
    split.split_id = kv::required(block, "SPLIT_ID");
    split.role = parse_source_split_role(kv::required(block, "ROLE"));
    const bool has_anchor_begin =
        source_split_detail::has_key(block, "ANCHOR_INDEX_BEGIN");
    const bool has_anchor_end =
        source_split_detail::has_key(block, "ANCHOR_INDEX_END");
    const bool has_fraction_begin =
        source_split_detail::has_key(block, "BEGIN_FRACTION");
    const bool has_fraction_end =
        source_split_detail::has_key(block, "END_FRACTION");
    if (has_anchor_begin != has_anchor_end) {
      throw std::runtime_error(
          "[source_split] ANCHOR_INDEX_BEGIN and "
          "ANCHOR_INDEX_END must be authored together for " +
          split.split_id);
    }
    if (has_fraction_begin != has_fraction_end) {
      throw std::runtime_error("[source_split] BEGIN_FRACTION and END_FRACTION "
                               "must be authored together for " +
                               split.split_id);
    }
    if (has_anchor_begin && has_fraction_begin) {
      throw std::runtime_error(
          "[source_split] split cannot mix anchor indexes and fractions: " +
          split.split_id);
    }
    const std::string selector_text = kv::optional(block, "SELECTOR", "");
    if (!kv::trim(selector_text).empty()) {
      split.selector_kind = parse_source_split_selector_kind(selector_text);
    } else if (has_fraction_begin) {
      split.selector_kind = source_split_selector_kind_t::fraction_range;
    } else {
      split.selector_kind = source_split_selector_kind_t::anchor_index_range;
    }
    split.min_count =
        source_split_detail::parse_positive_i64(block, "MIN_COUNT", 1);
    if (split.selector_kind ==
        source_split_selector_kind_t::anchor_index_range) {
      if (!has_anchor_begin) {
        throw std::runtime_error(
            "[source_split] anchor_index_range requires ANCHOR_INDEX_BEGIN and "
            "ANCHOR_INDEX_END for " +
            split.split_id);
      }
      split.anchor_range.begin = source_split_detail::parse_nonnegative_i64(
          block, "ANCHOR_INDEX_BEGIN");
      split.anchor_range.end =
          source_split_detail::parse_nonnegative_i64(block, "ANCHOR_INDEX_END");
      if (split.anchor_range.end <= split.anchor_range.begin) {
        throw std::runtime_error(
            "[source_split] ANCHOR_INDEX_END must be greater than "
            "ANCHOR_INDEX_BEGIN for " +
            split.split_id);
      }
      if (split.anchor_range.length() < split.min_count) {
        throw std::runtime_error("[source_split] materialized split is smaller "
                                 "than MIN_COUNT for " +
                                 split.split_id);
      }
    } else {
      if (!has_fraction_begin) {
        throw std::runtime_error(
            "[source_split] fraction_range requires BEGIN_FRACTION and "
            "END_FRACTION for " +
            split.split_id);
      }
      split.fraction_range.begin = source_split_detail::parse_fraction(
          kv::required(block, "BEGIN_FRACTION"), "BEGIN_FRACTION");
      split.fraction_range.end = source_split_detail::parse_fraction(
          kv::required(block, "END_FRACTION"), "END_FRACTION");
      if (!source_split_detail::fraction_less(split.fraction_range.begin,
                                              split.fraction_range.end)) {
        throw std::runtime_error("[source_split] END_FRACTION must be greater "
                                 "than BEGIN_FRACTION for " +
                                 split.split_id);
      }
    }
    if (std::find(split_ids.begin(), split_ids.end(), split.split_id) !=
        split_ids.end()) {
      throw std::runtime_error("[source_split] duplicate SPLIT_ID: " +
                               split.split_id);
    }
    split_ids.push_back(split.split_id);
    out.splits.push_back(std::move(split));
  }
  if (!saw_catalog) {
    throw std::runtime_error(
        "[source_split] UJCAMEI_SOURCE_SPLIT_CATALOG block is required");
  }
  if (out.catalog_id.empty()) {
    throw std::runtime_error("[source_split] CATALOG_ID is required");
  }
  if (out.cursor_domain != "ujcamei.graph_anchor") {
    throw std::runtime_error(
        "[source_split] v1 only supports CURSOR_DOMAIN=ujcamei.graph_anchor");
  }
  if (out.splits.empty()) {
    throw std::runtime_error(
        "[source_split] at least one UJCAMEI_SOURCE_SPLIT block is required");
  }
  return out;
}

[[nodiscard]] inline source_anchor_interval_t
materialize_source_split_range(const source_split_t &split,
                               std::int64_t accepted_anchor_count) {
  if (split.selector_kind == source_split_selector_kind_t::anchor_index_range) {
    if (accepted_anchor_count > 0 &&
        split.anchor_range.end > accepted_anchor_count) {
      throw std::runtime_error("[source_split] anchor_index_range exceeds "
                               "accepted anchor domain for " +
                               split.split_id);
    }
    return split.anchor_range;
  }
  if (accepted_anchor_count <= 0) {
    throw std::runtime_error(
        "[source_split] fraction_range requires a positive "
        "accepted anchor count for " +
        split.split_id);
  }
  source_anchor_interval_t out{};
  out.begin = source_split_detail::fraction_boundary_floor(
      split.fraction_range.begin, accepted_anchor_count);
  out.end = source_split_detail::fraction_boundary_floor(
      split.fraction_range.end, accepted_anchor_count);
  if (out.end <= out.begin) {
    throw std::runtime_error(
        "[source_split] fraction_range materialized to an empty split for " +
        split.split_id);
  }
  if (out.length() < split.min_count) {
    throw std::runtime_error(
        "[source_split] materialized split is smaller than "
        "MIN_COUNT for " +
        split.split_id);
  }
  return out;
}

[[nodiscard]] inline source_split_t
materialize_source_split(const source_split_t &split,
                         std::int64_t accepted_anchor_count) {
  source_split_t out = split;
  out.anchor_range =
      materialize_source_split_range(split, accepted_anchor_count);
  out.selector_kind = source_split_selector_kind_t::anchor_index_range;
  return out;
}

[[nodiscard]] inline source_split_catalog_t
materialize_source_split_catalog(const source_split_catalog_t &catalog,
                                 std::int64_t accepted_anchor_count) {
  source_split_catalog_t out = catalog;
  out.splits.clear();
  out.splits.reserve(catalog.splits.size());
  for (const auto &split : catalog.splits) {
    out.splits.push_back(
        materialize_source_split(split, accepted_anchor_count));
  }
  return out;
}

[[nodiscard]] inline source_split_catalog_t
load_source_split_catalog_from_file(const std::filesystem::path &path) {
  return decode_source_split_catalog_from_dsl(
      source_split_detail::read_text_file_or_throw(path));
}

[[nodiscard]] inline std::string
canonical_source_split_catalog_text(const source_split_catalog_t &catalog) {
  std::ostringstream out;
  out << "schema=" << catalog.schema << "\n";
  out << "catalog_id=" << catalog.catalog_id << "\n";
  out << "cursor_domain=" << catalog.cursor_domain << "\n";
  std::vector<source_split_t> splits = catalog.splits;
  std::sort(splits.begin(), splits.end(),
            [](const source_split_t &a, const source_split_t &b) {
              return a.split_id < b.split_id;
            });
  for (const auto &split : splits) {
    out << "split=" << split.split_id << "|"
        << source_split_role_name(split.role);
    if (split.selector_kind ==
        source_split_selector_kind_t::anchor_index_range) {
      out << "|" << split.anchor_range.begin << "|" << split.anchor_range.end;
    } else {
      out << "|" << source_split_selector_kind_name(split.selector_kind)
          << "|fraction="
          << source_split_detail::fraction_text(split.fraction_range.begin)
          << ":" << source_split_detail::fraction_text(split.fraction_range.end)
          << "|min_count=" << split.min_count;
    }
    out << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string
source_split_catalog_fingerprint(const source_split_catalog_t &catalog) {
  std::uint64_t hash = 14695981039346656037ull;
  source_split_detail::mix_hash_string(hash, "ujcamei.source.splits.v1");
  source_split_detail::mix_hash_string(
      hash, canonical_source_split_catalog_text(catalog));
  return source_split_detail::hex_u64(hash);
}

} // namespace cuwacunu::ujcamei::source::splits
