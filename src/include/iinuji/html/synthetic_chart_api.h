// SPDX-License-Identifier: MIT
#pragma once

#include "piaabo/digest/sha256.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cuwacunu::iinuji::html {

inline constexpr std::size_t kSyntheticRowCount = 1200U;
inline constexpr std::size_t kSyntheticAcceptedAnchorCount = 1170U;
inline constexpr std::size_t kSyntheticMaximumInputLength = 30U;
inline constexpr std::size_t kSyntheticServedAnchorEndExclusive = 1088U;

inline constexpr std::string_view kSyntheticBenchmarkV1Id =
    "synthetic_continuous_graph_v1";
inline constexpr std::string_view kSyntheticBenchmarkV2Id =
    "synthetic_continuous_graph_v2";
inline constexpr std::size_t kSyntheticV2AcceptedAnchorCount = 4096U;
inline constexpr std::size_t kSyntheticV2ServedAnchorEndExclusive = 3264U;
inline constexpr std::size_t kSyntheticV2FinalHoldoutBegin = 3328U;
inline constexpr std::size_t kSyntheticV2FinalHoldoutEndExclusive = 4096U;

class synthetic_chart_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

namespace synthetic_chart_detail {

inline constexpr std::string_view kBenchmarkRelativeRoot =
    "src/config/benchmarks/synthetic_continuous_graph_v1";
inline constexpr std::string_view kManifestRelativePath =
    "src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/"
    "synthetic_periodic_chart_manifest.v1.report";
inline constexpr std::string_view kChannelsRelativePath =
    "src/config/benchmarks/synthetic_continuous_graph_v1/"
    "ujcamei.source.retrieval.channels.dsl";
inline constexpr std::string_view kSplitsRelativePath =
    "src/config/benchmarks/synthetic_continuous_graph_v1/"
    "ujcamei.source.splits.dsl";

struct interval_spec_t {
  std::string_view id;
  std::size_t input_length;
  std::size_t step_days;
};

struct instrument_spec_t {
  std::string_view id;
  std::size_t fundamental_period_anchors;
};

inline constexpr std::array<interval_spec_t, 3> kIntervals{{
    {"1d", 30U, 1U},
    {"3d", 10U, 3U},
    {"1w", 4U, 7U},
}};

inline constexpr std::array<instrument_spec_t, 3> kInstruments{{
    {"SYNALPHASYNUSD", 12U},
    {"SYNBETASYNUSD", 10U},
    {"SYNGAMMASYNUSD", 8U},
}};

[[noreturn]] inline void fail(const std::string &message) {
  throw synthetic_chart_error("iinuji synthetic chart: " + message);
}

inline void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] inline std::string trim(std::string_view value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1U));
}

[[nodiscard]] inline bool path_is_within(const std::filesystem::path &root,
                                         const std::filesystem::path &path) {
  auto root_it = root.begin();
  auto path_it = path.begin();
  for (; root_it != root.end(); ++root_it, ++path_it) {
    if (path_it == path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::filesystem::path
canonical_repo_root(const std::filesystem::path &candidate) {
  std::error_code error;
  const auto root = std::filesystem::canonical(candidate, error);
  require(!error && std::filesystem::is_directory(root),
          "repo root is not a readable directory: " + candidate.string());
  const auto manifest = root / std::filesystem::path(kManifestRelativePath);
  const auto channels = root / std::filesystem::path(kChannelsRelativePath);
  const auto splits = root / std::filesystem::path(kSplitsRelativePath);
  require(std::filesystem::is_regular_file(manifest) &&
              std::filesystem::is_regular_file(channels) &&
              std::filesystem::is_regular_file(splits),
          "repo root does not contain the canonical synthetic benchmark");
  return root;
}

[[nodiscard]] inline std::filesystem::path
fixed_repo_file(const std::filesystem::path &root,
                const std::filesystem::path &relative) {
  require(!relative.empty() && relative.is_relative(),
          "internal source path must be relative");
  std::error_code error;
  const auto path = std::filesystem::canonical(root / relative, error);
  require(!error && std::filesystem::is_regular_file(path),
          "required source is not a regular file: " +
              relative.generic_string());
  require(path_is_within(root, path),
          "required source resolves outside the repository: " +
              relative.generic_string());
  return path;
}

[[nodiscard]] inline std::string
read_file(const std::filesystem::path &root,
          const std::filesystem::path &relative, std::uintmax_t maximum_bytes) {
  const auto path = fixed_repo_file(root, relative);
  std::error_code error;
  const auto size = std::filesystem::file_size(path, error);
  require(!error && size <= maximum_bytes,
          "required source is larger than its fixed bound: " +
              relative.generic_string());
  std::ifstream input(path, std::ios::binary);
  require(input.good(),
          "cannot open required source: " + relative.generic_string());
  std::string contents(static_cast<std::size_t>(size), '\0');
  if (!contents.empty()) {
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  }
  require(input.good() || input.eof(),
          "failed while reading required source: " + relative.generic_string());
  require(static_cast<std::size_t>(input.gcount()) == contents.size(),
          "short read from required source: " + relative.generic_string());
  return contents;
}

inline void require_no_symlink_components(const std::filesystem::path &root,
                                           const std::filesystem::path &path) {
  require(path_is_within(root, path),
          "internal source path escapes its fixed root");
  auto cursor = root;
  auto relative = path.lexically_relative(root);
  require(!relative.empty() && relative.is_relative(),
          "internal source path is not below its fixed root");
  for (const auto &component : relative) {
    cursor /= component;
    std::error_code error;
    const auto status = std::filesystem::symlink_status(cursor, error);
    require(!error && !std::filesystem::is_symlink(status),
            "required source traverses a symbolic link: " + cursor.string());
  }
}

[[nodiscard]] inline std::string read_file_without_symlinks(
    const std::filesystem::path &root,
    const std::filesystem::path &relative, std::uintmax_t maximum_bytes) {
  require(!relative.empty() && relative.is_relative(),
          "internal source path must be relative");
  const auto lexical_root = root.lexically_normal();
  const auto lexical_path = (lexical_root / relative).lexically_normal();
  require_no_symlink_components(lexical_root, lexical_path);
  std::error_code error;
  const auto canonical_root = std::filesystem::canonical(lexical_root, error);
  require(!error && std::filesystem::is_directory(canonical_root),
          "fixed source root is not a readable directory: " + root.string());
  const auto canonical_path = std::filesystem::canonical(lexical_path, error);
  require(!error && std::filesystem::is_regular_file(canonical_path) &&
              path_is_within(canonical_root, canonical_path),
          "required source is not a regular file below its fixed root: " +
              relative.generic_string());
  const auto size = std::filesystem::file_size(canonical_path, error);
  require(!error && size <= maximum_bytes,
          "required source is larger than its fixed bound: " +
              relative.generic_string());
  std::ifstream input(canonical_path, std::ios::binary);
  require(input.good(),
          "cannot open required source: " + relative.generic_string());
  std::string contents(static_cast<std::size_t>(size), '\0');
  if (!contents.empty()) {
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  }
  require(input.good() || input.eof(),
          "failed while reading required source: " +
              relative.generic_string());
  require(static_cast<std::size_t>(input.gcount()) == contents.size(),
          "short read from required source: " + relative.generic_string());
  return contents;
}

[[nodiscard]] inline std::uint64_t parse_u64(std::string_view text,
                                             std::string_view label) {
  require(!text.empty(), std::string(label) + " is empty");
  std::uint64_t value = 0U;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value, 10);
  require(result.ec == std::errc{} && result.ptr == text.data() + text.size(),
          std::string(label) + " is not an unsigned integer");
  return value;
}

[[nodiscard]] inline std::int64_t parse_i64(std::string_view text,
                                            std::string_view label) {
  require(!text.empty(), std::string(label) + " is empty");
  std::int64_t value = 0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value, 10);
  require(result.ec == std::errc{} && result.ptr == text.data() + text.size(),
          std::string(label) + " is not an integer");
  return value;
}

[[nodiscard]] inline double parse_finite_double(std::string_view text,
                                                std::string_view label) {
  require(!text.empty(), std::string(label) + " is empty");
  const std::string owned(text);
  char *end = nullptr;
  errno = 0;
  const double value = std::strtod(owned.c_str(), &end);
  require(errno != ERANGE && end == owned.c_str() + owned.size() &&
              std::isfinite(value),
          std::string(label) + " is not a finite number");
  return value;
}

[[nodiscard]] inline std::vector<std::string> split(std::string_view line,
                                                    char delimiter) {
  std::vector<std::string> result;
  std::size_t begin = 0U;
  while (true) {
    const auto end = line.find(delimiter, begin);
    result.emplace_back(line.substr(begin, end - begin));
    if (end == std::string_view::npos) {
      return result;
    }
    begin = end + 1U;
  }
}

[[nodiscard]] inline std::string json_escape(std::string_view value) {
  std::ostringstream output;
  for (const unsigned char c : value) {
    switch (c) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (c < 0x20U) {
        output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
               << static_cast<unsigned int>(c) << std::dec;
      } else {
        output << static_cast<char>(c);
      }
    }
  }
  return output.str();
}

[[nodiscard]] inline std::string json_number(double value) {
  require(std::isfinite(value), "attempted to serialize a non-finite number");
  if (value == 0.0) {
    return "0";
  }
  std::ostringstream output;
  output << std::setprecision(std::numeric_limits<double>::max_digits10)
         << value;
  return output.str();
}

[[nodiscard]] inline const interval_spec_t *find_interval(std::string_view id) {
  const auto found =
      std::find_if(kIntervals.begin(), kIntervals.end(),
                   [id](const auto &candidate) { return candidate.id == id; });
  return found == kIntervals.end() ? nullptr : &*found;
}

[[nodiscard]] inline const instrument_spec_t *
find_instrument(std::string_view id) {
  const auto found =
      std::find_if(kInstruments.begin(), kInstruments.end(),
                   [id](const auto &candidate) { return candidate.id == id; });
  return found == kInstruments.end() ? nullptr : &*found;
}

struct kline_row_t {
  std::int64_t open_time_ms{};
  double open{};
  double high{};
  double low{};
  double close{};
  double volume{};
  std::int64_t close_time_ms{};
};

struct chart_data_t {
  std::string relative_path;
  std::string sha256;
  std::vector<kline_row_t> rows;
};

struct chart_summary_t {
  double minimum_close{std::numeric_limits<double>::infinity()};
  double maximum_close{-std::numeric_limits<double>::infinity()};
  double close_sum{};
  double minimum_log_return{std::numeric_limits<double>::infinity()};
  double maximum_log_return{-std::numeric_limits<double>::infinity()};
  double log_return_sum{};
  std::size_t positive_log_return_count{};
  std::size_t negative_log_return_count{};
  std::size_t zero_log_return_count{};
};

[[nodiscard]] inline std::map<std::string, std::string>
parse_manifest(const std::string &contents) {
  std::map<std::string, std::string> values;
  std::istringstream input(contents);
  std::string line;
  std::size_t line_number = 0U;
  while (std::getline(input, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (trim(line).empty()) {
      continue;
    }
    const auto separator = line.find('=');
    require(separator != std::string::npos,
            "manifest line " + std::to_string(line_number) + " lacks '='");
    const auto key = trim(std::string_view(line).substr(0U, separator));
    const auto value = trim(std::string_view(line).substr(separator + 1U));
    require(!key.empty() && !value.empty(),
            "manifest line " + std::to_string(line_number) + " is incomplete");
    require(values.emplace(key, value).second,
            "manifest contains duplicate key: " + key);
  }
  return values;
}

[[nodiscard]] inline const std::string &
required_value(const std::map<std::string, std::string> &values,
               const std::string &key) {
  const auto found = values.find(key);
  require(found != values.end(), "manifest lacks required key: " + key);
  return found->second;
}

inline void
require_manifest_value(const std::map<std::string, std::string> &values,
                       const std::string &key, std::string_view expected) {
  require(required_value(values, key) == expected,
          "manifest " + key + " does not match the canonical contract");
}

inline void
validate_manifest(const std::map<std::string, std::string> &manifest) {
  require_manifest_value(manifest, "schema_id",
                         "synthetic_periodic_chart_manifest.v1");
  require_manifest_value(manifest, "benchmark_id",
                         "synthetic_continuous_graph_v1");
  require_manifest_value(manifest, "chart_family",
                         "cycle_rich_zero_drift_log_price.v1");
  require_manifest_value(manifest, "row_count", "1200");
  require_manifest_value(manifest, "input_length", "30");
  require_manifest_value(manifest, "accepted_anchor_count", "1170");
  require_manifest_value(manifest, "train_anchor_begin", "0");
  require_manifest_value(manifest, "train_anchor_end_exclusive", "730");
  require_manifest_value(manifest, "embargo_gap_anchors", "30");
  require_manifest_value(manifest, "eval_anchor_begin", "760");
  require_manifest_value(manifest, "eval_anchor_end_exclusive", "1088");
  require_manifest_value(manifest, "test_anchor_begin", "1088");
  require_manifest_value(manifest, "test_anchor_end_exclusive", "1170");
  require_manifest_value(manifest, "source_split_policy",
                         "train_to_73_117_eval_65_93_test_93_100_embargo_gap");
  require_manifest_value(
      manifest, "instrument.SYNALPHASYNUSD.fundamental_period_anchors", "12");
  require_manifest_value(
      manifest, "instrument.SYNBETASYNUSD.fundamental_period_anchors", "10");
  require_manifest_value(
      manifest, "instrument.SYNGAMMASYNUSD.fundamental_period_anchors", "8");
  require_manifest_value(manifest, "max_fundamental_period_anchors", "12");
  require_manifest_value(manifest, "periodic_chart_sufficient_cycles", "true");
  require_manifest_value(manifest, "chart_drift_model", "none");
  const auto &digest = required_value(manifest, "source_function_digest");
  require(cuwacunu::piaabo::digest::is_sha256_hex(digest),
          "manifest source_function_digest is not canonical lowercase SHA-256");
}

[[nodiscard]] inline std::map<std::string, std::map<std::string, std::string>>
parse_split_blocks(const std::string &contents) {
  std::map<std::string, std::map<std::string, std::string>> result;
  constexpr std::string_view needle = "UJCAMEI_SOURCE_SPLIT {";
  std::size_t cursor = 0U;
  while ((cursor = contents.find(needle, cursor)) != std::string::npos) {
    const auto body_begin = cursor + needle.size();
    const auto body_end = contents.find("};", body_begin);
    require(body_end != std::string::npos, "unterminated split block");
    std::map<std::string, std::string> fields;
    std::size_t assignment_begin = body_begin;
    while (assignment_begin < body_end) {
      const auto semicolon = contents.find(';', assignment_begin);
      if (semicolon == std::string::npos || semicolon > body_end) {
        break;
      }
      const auto assignment = trim(std::string_view(contents).substr(
          assignment_begin, semicolon - assignment_begin));
      assignment_begin = semicolon + 1U;
      if (assignment.empty()) {
        continue;
      }
      const auto equals = assignment.find('=');
      require(equals != std::string::npos, "malformed split assignment");
      const auto key = trim(std::string_view(assignment).substr(0U, equals));
      const auto value = trim(std::string_view(assignment).substr(equals + 1U));
      require(fields.emplace(key, value).second,
              "duplicate split field: " + key);
    }
    const auto id = fields.find("SPLIT_ID");
    require(id != fields.end(), "split block lacks SPLIT_ID");
    require(result.emplace(id->second, std::move(fields)).second,
            "duplicate split block: " + id->second);
    cursor = body_end + 2U;
  }
  return result;
}

inline void validate_split_block(
    const std::map<std::string, std::map<std::string, std::string>> &blocks,
    const std::string &id, std::string_view role, std::string_view begin,
    std::string_view end) {
  const auto found = blocks.find(id);
  require(found != blocks.end(), "split DSL lacks block: " + id);
  const auto &fields = found->second;
  const auto field = [&fields,
                      &id](const std::string &key) -> const std::string & {
    const auto item = fields.find(key);
    require(item != fields.end(), "split " + id + " lacks field " + key);
    return item->second;
  };
  require(field("ROLE") == role && field("SELECTOR") == "fraction_range" &&
              field("BEGIN_FRACTION") == begin && field("END_FRACTION") == end,
          "split " + id + " does not match the canonical fractions/role");
}

[[nodiscard]] inline std::map<std::string, interval_spec_t>
parse_and_validate_channels(const std::string &contents) {
  std::map<std::string, interval_spec_t> active;
  std::istringstream input(contents);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line.front() != '|') {
      continue;
    }
    auto fields = split(line, '|');
    std::vector<std::string> compact;
    for (const auto &field : fields) {
      const auto value = trim(field);
      if (!value.empty()) {
        compact.push_back(value);
      }
    }
    if (compact.size() != 7U || compact[0] == "interval" ||
        compact[1] != "true") {
      continue;
    }
    const auto *spec = find_interval(compact[0]);
    require(spec != nullptr, "channels DSL activates an unknown interval");
    require(compact[2] == "kline" &&
                parse_u64(compact[3], "channel input_length") ==
                    spec->input_length &&
                parse_u64(compact[4], "channel future_length") == 1U &&
                parse_finite_double(compact[5], "channel weight") == 1.0 &&
                compact[6] == "log_returns",
            "active channel does not match the fixed synthetic contract: " +
                compact[0]);
    require(active.emplace(compact[0], *spec).second,
            "channels DSL contains duplicate active interval: " + compact[0]);
  }
  require(active.size() == kIntervals.size(),
          "channels DSL must contain exactly the three fixed active intervals");
  return active;
}

[[nodiscard]] inline std::string csv_relative_path(std::string_view instrument,
                                                   std::string_view interval) {
  return std::string(kBenchmarkRelativeRoot) + "/data/raw/" +
         std::string(instrument) + "/" + std::string(interval) + "/" +
         std::string(instrument) + "-" + std::string(interval) +
         "-all-years.csv";
}

[[nodiscard]] inline chart_data_t
parse_chart_csv(const std::filesystem::path &root, std::string_view instrument,
                const interval_spec_t &interval) {
  chart_data_t chart;
  chart.relative_path = csv_relative_path(instrument, interval.id);
  const auto contents =
      read_file(root, chart.relative_path, 4U * 1024U * 1024U);
  chart.sha256 = cuwacunu::piaabo::digest::sha256_hex(contents);

  std::istringstream input(contents);
  std::string line;
  std::size_t line_number = 0U;
  const std::int64_t step_ms =
      static_cast<std::int64_t>(interval.step_days) * 86400000LL;
  while (std::getline(input, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    require(!line.empty(), chart.relative_path + " contains an empty CSV row");
    const auto fields = split(line, ',');
    require(fields.size() == 12U, chart.relative_path + " row " +
                                      std::to_string(line_number) +
                                      " does not have exactly 12 fields");

    kline_row_t row;
    row.open_time_ms = parse_i64(fields[0], "CSV open_time_ms");
    row.open = parse_finite_double(fields[1], "CSV open");
    row.high = parse_finite_double(fields[2], "CSV high");
    row.low = parse_finite_double(fields[3], "CSV low");
    row.close = parse_finite_double(fields[4], "CSV close");
    row.volume = parse_finite_double(fields[5], "CSV volume");
    row.close_time_ms = parse_i64(fields[6], "CSV close_time_ms");
    const double quote_volume =
        parse_finite_double(fields[7], "CSV quote_asset_volume");
    const auto trade_count = parse_u64(fields[8], "CSV number_of_trades");
    const double taker_base =
        parse_finite_double(fields[9], "CSV taker_buy_base_volume");
    const double taker_quote =
        parse_finite_double(fields[10], "CSV taker_buy_quote_volume");
    const auto ignored = parse_u64(fields[11], "CSV ignored field");

    require(row.open > 0.0 && row.high > 0.0 && row.low > 0.0 &&
                row.close > 0.0 && row.volume >= 0.0 && quote_volume >= 0.0 &&
                taker_base >= 0.0 && taker_quote >= 0.0 && trade_count > 0U &&
                ignored == 0U,
            chart.relative_path + " row " + std::to_string(line_number) +
                " violates positive-price/nonnegative-volume constraints");
    require(row.high >= std::max(row.open, row.close) &&
                row.low <= std::min(row.open, row.close) && row.low <= row.high,
            chart.relative_path + " row " + std::to_string(line_number) +
                " violates OHLC ordering");
    require(row.close_time_ms == row.open_time_ms + step_ms - 1LL,
            chart.relative_path + " row " + std::to_string(line_number) +
                " has an invalid interval duration");
    if (!chart.rows.empty()) {
      const auto &previous = chart.rows.back();
      require(row.open_time_ms == previous.open_time_ms + step_ms &&
                  row.close_time_ms > previous.close_time_ms,
              chart.relative_path + " timestamps are not strictly ordered");
      require(std::abs(row.open - previous.close) <= 1e-12,
              chart.relative_path + " is not OHLC-continuous by row");
    }
    chart.rows.push_back(row);
  }
  require(chart.rows.size() == kSyntheticRowCount,
          chart.relative_path + " must contain exactly 1200 ordered rows");
  return chart;
}

inline constexpr std::string_view kV2BenchmarkRelativeRoot =
    "src/config/benchmarks/synthetic_continuous_graph_v2";
inline constexpr std::string_view kV2ManifestRelativePath =
    "src/config/benchmarks/synthetic_continuous_graph_v2/artifacts/"
    "synthetic_quasiperiodic_chart_manifest.v2.report";
inline constexpr std::string_view kV2DevelopmentPrefixRelativeRoot =
    "src/config/benchmarks/synthetic_continuous_graph_v2/"
    "data/development_prefix";

struct v2_interval_spec_t {
  std::string_view id;
  std::size_t input_length;
  std::size_t step_days;
  std::size_t development_row_count;
};

struct v2_instrument_spec_t {
  std::string_view id;
  double dominant_period_anchors;
};

inline constexpr std::array<v2_interval_spec_t, 3> kV2Intervals{{
    {"1d", 30U, 1U, 3294U},
    {"3d", 10U, 3U, 1098U},
    {"1w", 4U, 7U, 471U},
}};

inline constexpr std::array<v2_instrument_spec_t, 3> kV2Instruments{{
    {"SYN2ALPHASYN2USD", 24.041630560342618},
    {"SYN2BETASYN2USD", 32.908965343808667},
    {"SYN2GAMMASYN2USD", 51.429563482495169},
}};

[[nodiscard]] inline const v2_interval_spec_t *
find_v2_interval(std::string_view id) {
  const auto found = std::find_if(
      kV2Intervals.begin(), kV2Intervals.end(),
      [id](const auto &candidate) { return candidate.id == id; });
  return found == kV2Intervals.end() ? nullptr : &*found;
}

[[nodiscard]] inline const v2_instrument_spec_t *
find_v2_instrument(std::string_view id) {
  const auto found = std::find_if(
      kV2Instruments.begin(), kV2Instruments.end(),
      [id](const auto &candidate) { return candidate.id == id; });
  return found == kV2Instruments.end() ? nullptr : &*found;
}

[[nodiscard]] inline std::string
v2_csv_relative_path(std::string_view instrument, std::string_view interval) {
  return std::string(kV2DevelopmentPrefixRelativeRoot) + "/" +
         std::string(instrument) + "/" + std::string(interval) + "/" +
         std::string(instrument) + "-" + std::string(interval) +
         "-all-years.csv";
}

[[nodiscard]] inline chart_data_t parse_v2_development_chart_csv(
    const std::filesystem::path &root, std::string_view instrument,
    const v2_interval_spec_t &interval) {
  chart_data_t chart;
  chart.relative_path = v2_csv_relative_path(instrument, interval.id);
  const auto contents = read_file_without_symlinks(
      root, chart.relative_path, 16U * 1024U * 1024U);
  chart.sha256 = cuwacunu::piaabo::digest::sha256_hex(contents);

  std::istringstream input(contents);
  std::string line;
  std::size_t line_number = 0U;
  const std::int64_t step_ms =
      static_cast<std::int64_t>(interval.step_days) * 86400000LL;
  while (std::getline(input, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    require(!line.empty(), chart.relative_path + " contains an empty CSV row");
    const auto fields = split(line, ',');
    require(fields.size() == 12U,
            chart.relative_path + " row " + std::to_string(line_number) +
                " does not have exactly 12 fields");

    kline_row_t row;
    row.open_time_ms = parse_i64(fields[0], "CSV open_time_ms");
    row.open = parse_finite_double(fields[1], "CSV open");
    row.high = parse_finite_double(fields[2], "CSV high");
    row.low = parse_finite_double(fields[3], "CSV low");
    row.close = parse_finite_double(fields[4], "CSV close");
    row.volume = parse_finite_double(fields[5], "CSV volume");
    row.close_time_ms = parse_i64(fields[6], "CSV close_time_ms");
    const double quote_volume =
        parse_finite_double(fields[7], "CSV quote_asset_volume");
    const auto trade_count = parse_u64(fields[8], "CSV number_of_trades");
    const double taker_base =
        parse_finite_double(fields[9], "CSV taker_buy_base_volume");
    const double taker_quote =
        parse_finite_double(fields[10], "CSV taker_buy_quote_volume");
    const auto ignored = parse_u64(fields[11], "CSV ignored field");

    require(row.open > 0.0 && row.high > 0.0 && row.low > 0.0 &&
                row.close > 0.0 && row.volume >= 0.0 && quote_volume >= 0.0 &&
                taker_base >= 0.0 && taker_quote >= 0.0 && trade_count > 0U &&
                ignored == 0U,
            chart.relative_path + " row " + std::to_string(line_number) +
                " violates positive-price/nonnegative-volume constraints");
    require(row.high >= std::max(row.open, row.close) &&
                row.low <= std::min(row.open, row.close) && row.low <= row.high,
            chart.relative_path + " row " + std::to_string(line_number) +
                " violates OHLC ordering");
    require(row.close_time_ms == row.open_time_ms + step_ms - 1LL,
            chart.relative_path + " row " + std::to_string(line_number) +
                " has an invalid interval duration");
    if (!chart.rows.empty()) {
      const auto &previous = chart.rows.back();
      require(row.open_time_ms == previous.open_time_ms + step_ms &&
                  row.close_time_ms > previous.close_time_ms,
              chart.relative_path + " timestamps are not strictly ordered");
      require(std::abs(row.open - previous.close) <= 1e-12,
              chart.relative_path + " is not OHLC-continuous by row");
    }
    chart.rows.push_back(row);
  }
  require(chart.rows.size() == interval.development_row_count,
          chart.relative_path + " must contain exactly " +
              std::to_string(interval.development_row_count) +
              " ordered development-prefix rows");
  return chart;
}

} // namespace synthetic_chart_detail

class synthetic_chart_repository_t {
public:
  explicit synthetic_chart_repository_t(std::filesystem::path repo_root)
      : repo_root_(synthetic_chart_detail::canonical_repo_root(repo_root)) {
    load_and_validate();
  }

  [[nodiscard]] const std::filesystem::path &repo_root() const noexcept {
    return repo_root_;
  }

  [[nodiscard]] static bool instrument_allowed(std::string_view instrument) {
    return synthetic_chart_detail::find_instrument(instrument) != nullptr;
  }

  [[nodiscard]] static bool interval_allowed(std::string_view interval) {
    return synthetic_chart_detail::find_interval(interval) != nullptr;
  }

  [[nodiscard]] const std::string &catalog_json() const noexcept {
    return catalog_json_;
  }

  [[nodiscard]] const std::string &chart_json(std::string_view instrument,
                                              std::string_view interval) const {
    const auto found =
        chart_json_.find({std::string(instrument), std::string(interval)});
    if (found == chart_json_.end()) {
      throw std::out_of_range("chart is not in the fixed synthetic allowlist");
    }
    return found->second;
  }

  [[nodiscard]] std::size_t
  served_point_count(std::string_view instrument,
                     std::string_view interval) const {
    const auto found =
        charts_.find({std::string(instrument), std::string(interval)});
    if (found == charts_.end()) {
      throw std::out_of_range("chart is not in the fixed synthetic allowlist");
    }
    return std::min(kSyntheticServedAnchorEndExclusive,
                    found->second.rows.size() - kSyntheticMaximumInputLength);
  }

  [[nodiscard]] std::size_t maximum_served_raw_row() const noexcept {
    return kSyntheticMaximumInputLength + kSyntheticServedAnchorEndExclusive -
           1U;
  }

private:
  using chart_key_t = std::pair<std::string, std::string>;

  void load_and_validate() {
    using namespace synthetic_chart_detail;
    const auto manifest_contents = read_file(
        repo_root_, std::filesystem::path(kManifestRelativePath), 128U * 1024U);
    const auto channels_contents = read_file(
        repo_root_, std::filesystem::path(kChannelsRelativePath), 128U * 1024U);
    const auto splits_contents = read_file(
        repo_root_, std::filesystem::path(kSplitsRelativePath), 128U * 1024U);

    manifest_ = parse_manifest(manifest_contents);
    validate_manifest(manifest_);
    const auto channels = parse_and_validate_channels(channels_contents);
    require(channels.size() == kIntervals.size(),
            "channel validation did not retain the fixed interval set");

    const auto split_blocks = parse_split_blocks(splits_contents);
    require(split_blocks.size() == 5U,
            "split DSL must contain exactly five canonical split blocks");
    validate_split_block(split_blocks, "train_core", "train", "0/1", "73/117");
    validate_split_block(split_blocks, "acceptance_smoke", "train", "1/100",
                         "2/100");
    validate_split_block(split_blocks, "validation_holdout", "validation",
                         "80/100", "92/100");
    validate_split_block(split_blocks, "certified_replay_expansion_eval",
                         "validation", "65/100", "93/100");
    validate_split_block(split_blocks, "test_holdout", "test", "93/100",
                         "100/100");

    manifest_sha256_ = cuwacunu::piaabo::digest::sha256_hex(manifest_contents);
    channels_sha256_ = cuwacunu::piaabo::digest::sha256_hex(channels_contents);
    splits_sha256_ = cuwacunu::piaabo::digest::sha256_hex(splits_contents);

    for (const auto &instrument : kInstruments) {
      for (const auto &interval : kIntervals) {
        charts_.emplace(
            chart_key_t{std::string(instrument.id), std::string(interval.id)},
            parse_chart_csv(repo_root_, instrument.id, interval));
      }
    }
    validate_cross_interval_ohlc();
    build_chart_json_documents();
    build_catalog_json();
  }

  void validate_cross_interval_ohlc() {
    using namespace synthetic_chart_detail;
    for (const auto &instrument : kInstruments) {
      double maximum = 0.0;
      const auto &reference = charts_.at(
          {std::string(instrument.id), std::string(kIntervals[0].id)});
      for (std::size_t interval_index = 1U; interval_index < kIntervals.size();
           ++interval_index) {
        const auto &candidate =
            charts_.at({std::string(instrument.id),
                        std::string(kIntervals[interval_index].id)});
        require(candidate.rows.size() == reference.rows.size(),
                "cross-interval row counts differ for " +
                    std::string(instrument.id));
        for (std::size_t row = 0U; row < reference.rows.size(); ++row) {
          const std::array<double, 4> lhs{
              reference.rows[row].open, reference.rows[row].high,
              reference.rows[row].low, reference.rows[row].close};
          const std::array<double, 4> rhs{
              candidate.rows[row].open, candidate.rows[row].high,
              candidate.rows[row].low, candidate.rows[row].close};
          for (std::size_t field = 0U; field < lhs.size(); ++field) {
            maximum = std::max(maximum, std::abs(lhs[field] - rhs[field]));
          }
        }
      }
      cross_interval_maximum_delta_.emplace(std::string(instrument.id),
                                            maximum);
    }
  }

  void build_chart_json_documents() {
    using namespace synthetic_chart_detail;
    for (const auto &instrument : kInstruments) {
      for (const auto &interval : kIntervals) {
        const chart_key_t key{std::string(instrument.id),
                              std::string(interval.id)};
        const auto &chart = charts_.at(key);
        chart_summary_t summary;
        std::ostringstream points;
        points << '[';
        for (std::size_t anchor = 0U;
             anchor < kSyntheticServedAnchorEndExclusive; ++anchor) {
          const std::size_t row_index = kSyntheticMaximumInputLength + anchor;
          const auto &row = chart.rows[row_index];
          const double log_return =
              std::log(row.close / chart.rows[row_index - 1U].close);
          require(std::isfinite(log_return),
                  "computed close log return is not finite");
          summary.minimum_close = std::min(summary.minimum_close, row.close);
          summary.maximum_close = std::max(summary.maximum_close, row.close);
          summary.close_sum += row.close;
          summary.minimum_log_return =
              std::min(summary.minimum_log_return, log_return);
          summary.maximum_log_return =
              std::max(summary.maximum_log_return, log_return);
          summary.log_return_sum += log_return;
          if (log_return > 0.0) {
            ++summary.positive_log_return_count;
          } else if (log_return < 0.0) {
            ++summary.negative_log_return_count;
          } else {
            ++summary.zero_log_return_count;
          }
          if (anchor != 0U) {
            points << ',';
          }
          points << '[' << anchor << ',' << row.open_time_ms << ','
                 << row.close_time_ms << ',' << json_number(row.open) << ','
                 << json_number(row.high) << ',' << json_number(row.low) << ','
                 << json_number(row.close) << ',' << json_number(row.volume)
                 << ',' << json_number(log_return) << ']';
        }
        points << ']';

        const double count =
            static_cast<double>(kSyntheticServedAnchorEndExclusive);
        std::ostringstream output;
        output << "{\"schema_id\":\"iinuji.synthetic_chart.v1\""
               << ",\"benchmark_id\":\"synthetic_continuous_graph_v1\""
               << ",\"instrument\":\"" << instrument.id << "\""
               << ",\"interval\":\"" << interval.id << "\""
               << ",\"fundamental_period_anchors\":"
               << instrument.fundamental_period_anchors
               << ",\"input_length\":" << interval.input_length
               << ",\"step_days\":" << interval.step_days
               << ",\"anchor_begin\":0"
               << ",\"anchor_end_exclusive\":"
               << kSyntheticServedAnchorEndExclusive
               << ",\"raw_row_begin\":" << kSyntheticMaximumInputLength
               << ",\"raw_row_end_exclusive\":"
               << (kSyntheticMaximumInputLength +
                   kSyntheticServedAnchorEndExclusive)
               << ",\"test_holdout_served\":false"
               << ",\"point_fields\":[\"anchor\",\"open_time_ms\","
                  "\"close_time_ms\",\"open\",\"high\",\"low\","
                  "\"close\",\"volume\",\"log_return\"]"
               << ",\"summary\":{\"point_count\":"
               << kSyntheticServedAnchorEndExclusive
               << ",\"min_close\":" << json_number(summary.minimum_close)
               << ",\"max_close\":" << json_number(summary.maximum_close)
               << ",\"mean_close\":" << json_number(summary.close_sum / count)
               << ",\"min_log_return\":"
               << json_number(summary.minimum_log_return)
               << ",\"max_log_return\":"
               << json_number(summary.maximum_log_return)
               << ",\"mean_log_return\":"
               << json_number(summary.log_return_sum / count)
               << ",\"positive_log_return_count\":"
               << summary.positive_log_return_count
               << ",\"negative_log_return_count\":"
               << summary.negative_log_return_count
               << ",\"zero_log_return_count\":" << summary.zero_log_return_count
               << '}' << ",\"source\":{\"csv_path\":\""
               << json_escape(chart.relative_path) << "\",\"csv_sha256\":\""
               << chart.sha256 << "\",\"row_count\":" << chart.rows.size()
               << "},\"points\":" << points.str() << '}';
        chart_json_.emplace(key, output.str());
      }
    }
  }

  void build_catalog_json() {
    using namespace synthetic_chart_detail;
    std::ostringstream output;
    output << "{\"schema_id\":\"iinuji.synthetic_chart_catalog.v1\""
           << ",\"benchmark_id\":\"synthetic_continuous_graph_v1\""
           << ",\"row_count\":" << kSyntheticRowCount
           << ",\"accepted_anchor_count\":" << kSyntheticAcceptedAnchorCount
           << ",\"max_input_length\":" << kSyntheticMaximumInputLength
           << ",\"served_anchor_end_exclusive\":"
           << kSyntheticServedAnchorEndExclusive
           << ",\"test_holdout_served\":false"
           << ",\"source\":{\"chart_family\":\""
           << json_escape(required_value(manifest_, "chart_family"))
           << "\",\"chart_drift_model\":\""
           << json_escape(required_value(manifest_, "chart_drift_model"))
           << "\",\"source_function_digest\":\""
           << required_value(manifest_, "source_function_digest")
           << "\",\"manifest_path\":\"" << kManifestRelativePath
           << "\",\"manifest_sha256\":\"" << manifest_sha256_
           << "\",\"channels_path\":\"" << kChannelsRelativePath
           << "\",\"channels_sha256\":\"" << channels_sha256_
           << "\",\"splits_path\":\"" << kSplitsRelativePath
           << "\",\"splits_sha256\":\"" << splits_sha256_ << "\"}"
           << ",\"splits\":["
           << "{\"id\":\"train\",\"begin\":0,\"end_exclusive\":730,"
              "\"served\":true},"
           << "{\"id\":\"embargo\",\"begin\":730,"
              "\"end_exclusive\":760,\"served\":true},"
           << "{\"id\":\"evaluation\",\"begin\":760,"
              "\"end_exclusive\":1088,\"served\":true},"
           << "{\"id\":\"test_holdout\",\"begin\":1088,"
              "\"end_exclusive\":1170,\"served\":false}]"
           << ",\"intervals\":[";
    for (std::size_t index = 0U; index < kIntervals.size(); ++index) {
      if (index != 0U) {
        output << ',';
      }
      const auto &interval = kIntervals[index];
      output << "{\"id\":\"" << interval.id
             << "\",\"input_length\":" << interval.input_length
             << ",\"step_days\":" << interval.step_days << '}';
    }
    output << "],\"instruments\":[";
    for (std::size_t instrument_index = 0U;
         instrument_index < kInstruments.size(); ++instrument_index) {
      if (instrument_index != 0U) {
        output << ',';
      }
      const auto &instrument = kInstruments[instrument_index];
      const double maximum =
          cross_interval_maximum_delta_.at(std::string(instrument.id));
      output << "{\"id\":\"" << instrument.id
             << "\",\"fundamental_period_anchors\":"
             << instrument.fundamental_period_anchors
             << ",\"cross_interval_ohlc\":{\"compared_row_count\":"
             << kSyntheticRowCount
             << ",\"maximum_absolute_delta\":" << json_number(maximum)
             << ",\"identical_by_row\":" << (maximum == 0.0 ? "true" : "false")
             << "},\"charts\":[";
      for (std::size_t interval_index = 0U; interval_index < kIntervals.size();
           ++interval_index) {
        if (interval_index != 0U) {
          output << ',';
        }
        const auto &interval = kIntervals[interval_index];
        const auto &chart =
            charts_.at({std::string(instrument.id), std::string(interval.id)});
        output << "{\"interval\":\"" << interval.id << "\",\"csv_path\":\""
               << json_escape(chart.relative_path) << "\",\"csv_sha256\":\""
               << chart.sha256 << "\"}";
      }
      output << "]}";
    }
    output << "],\"validation\":{\"csv_file_count\":9,"
              "\"rows_finite\":true,\"timestamps_ordered\":true,"
              "\"ohlc_valid\":true,\"holdout_truncated\":true}}";
    catalog_json_ = output.str();
  }

  std::filesystem::path repo_root_;
  std::map<std::string, std::string> manifest_;
  std::string manifest_sha256_;
  std::string channels_sha256_;
  std::string splits_sha256_;
  std::map<chart_key_t, synthetic_chart_detail::chart_data_t> charts_;
  std::map<std::string, double> cross_interval_maximum_delta_;
  std::map<chart_key_t, std::string> chart_json_;
  std::string catalog_json_;
};

class synthetic_chart_v2_repository_t {
public:
  explicit synthetic_chart_v2_repository_t(std::filesystem::path repo_root)
      : repo_root_(synthetic_chart_detail::canonical_repo_root(repo_root)) {
    load_and_validate();
  }

  [[nodiscard]] const std::filesystem::path &repo_root() const noexcept {
    return repo_root_;
  }

  [[nodiscard]] static bool instrument_allowed(std::string_view instrument) {
    return synthetic_chart_detail::find_v2_instrument(instrument) != nullptr;
  }

  [[nodiscard]] static bool interval_allowed(std::string_view interval) {
    return synthetic_chart_detail::find_v2_interval(interval) != nullptr;
  }

  [[nodiscard]] const std::string &catalog_json() const noexcept {
    return catalog_json_;
  }

  [[nodiscard]] const std::string &chart_json(std::string_view instrument,
                                              std::string_view interval) const {
    const auto found =
        chart_json_.find({std::string(instrument), std::string(interval)});
    if (found == chart_json_.end()) {
      throw std::out_of_range("chart is not in the fixed v2 allowlist");
    }
    return found->second;
  }

  [[nodiscard]] std::size_t
  served_point_count(std::string_view instrument,
                     std::string_view interval) const {
    const auto found =
        charts_.find({std::string(instrument), std::string(interval)});
    const auto *spec = synthetic_chart_detail::find_v2_interval(interval);
    if (found == charts_.end() || spec == nullptr) {
      throw std::out_of_range("chart is not in the fixed v2 allowlist");
    }
    return found->second.rows.size() - spec->input_length;
  }

  [[nodiscard]] std::size_t
  maximum_served_raw_row(std::string_view instrument,
                         std::string_view interval) const {
    const auto found =
        charts_.find({std::string(instrument), std::string(interval)});
    if (found == charts_.end()) {
      throw std::out_of_range("chart is not in the fixed v2 allowlist");
    }
    return found->second.rows.size() - 1U;
  }

private:
  using chart_key_t = std::pair<std::string, std::string>;

  void validate_manifest_contract() const {
    using namespace synthetic_chart_detail;
    require_manifest_value(manifest_, "schema_id",
                           "synthetic_quasiperiodic_chart_manifest.v2");
    require_manifest_value(manifest_, "benchmark_id",
                           kSyntheticBenchmarkV2Id);
    require_manifest_value(manifest_, "chart_family",
                           "seeded_quasiperiodic_causal_ohlcv.v2");
    require_manifest_value(
        manifest_, "seed_commitment_sha256",
        "7d22bae27c203eeec9fb2147d63d1cbe55d9de056db7c048f74e4798849ee227");
    require_manifest_value(manifest_, "stochastic_innovations", "false");
    require_manifest_value(manifest_, "master_day_count", "4137");
    require_manifest_value(manifest_, "accepted_anchor_count", "4096");
    require_manifest_value(manifest_, "anchor_zero_master_day_index", "29");
    require_manifest_value(manifest_, "max_visible_anchor_exclusive", "3264");
    require_manifest_value(manifest_, "max_visible_anchor_master_day_index",
                           "3292");
    require_manifest_value(manifest_,
                           "development_prefix_includes_one_step_future",
                           "true");
    require_manifest_value(manifest_, "development_prefix_derivation",
                           "exact_leading_rows_of_full_raw");
    require_manifest_value(manifest_, "train_anchor_begin", "0");
    require_manifest_value(manifest_, "train_anchor_end_exclusive", "2496");
    require_manifest_value(manifest_, "train_validation_embargo_begin",
                           "2496");
    require_manifest_value(manifest_, "train_validation_embargo_end_exclusive",
                           "2560");
    require_manifest_value(manifest_, "validation_anchor_begin", "2560");
    require_manifest_value(manifest_, "validation_anchor_end_exclusive",
                           "2816");
    require_manifest_value(manifest_, "validation_certified_embargo_begin",
                           "2816");
    require_manifest_value(manifest_,
                           "validation_certified_embargo_end_exclusive",
                           "2880");
    require_manifest_value(manifest_, "certified_eval_anchor_begin", "2880");
    require_manifest_value(manifest_, "certified_eval_anchor_end_exclusive",
                           "3264");
    require_manifest_value(manifest_, "certified_final_embargo_begin", "3264");
    require_manifest_value(manifest_,
                           "certified_final_embargo_end_exclusive", "3328");
    require_manifest_value(manifest_, "final_holdout_anchor_begin", "3328");
    require_manifest_value(manifest_, "final_holdout_anchor_end_exclusive",
                           "4096");
    require_manifest_value(manifest_, "full_raw_contains_final_holdout",
                           "true");
    require_manifest_value(manifest_,
                           "development_prefix_contains_final_holdout",
                           "false");
    require_manifest_value(manifest_, "generation_complete", "true");
    require(cuwacunu::piaabo::digest::is_sha256_hex(
                required_value(manifest_, "source_process_digest")),
            "v2 source_process_digest is not canonical lowercase SHA-256");

    for (std::size_t index = 0U; index < kV2Instruments.size(); ++index) {
      require_manifest_value(
          manifest_, "instrument." + std::to_string(index) + ".id",
          kV2Instruments[index].id);
    }
    for (const auto &interval : kV2Intervals) {
      const std::string prefix = "interval." + std::string(interval.id) + ".";
      require_manifest_value(manifest_, prefix + "days",
                             std::to_string(interval.step_days));
      require_manifest_value(manifest_, prefix + "input_length",
                             std::to_string(interval.input_length));
      require_manifest_value(manifest_, prefix + "future_length", "1");
      require_manifest_value(
          manifest_, prefix + "development_prefix_row_count",
          std::to_string(interval.development_row_count));
    }
  }

  void load_and_validate() {
    using namespace synthetic_chart_detail;
    const auto manifest_contents = read_file_without_symlinks(
        repo_root_, std::filesystem::path(kV2ManifestRelativePath),
        256U * 1024U);
    manifest_ = parse_manifest(manifest_contents);
    validate_manifest_contract();
    manifest_sha256_ = cuwacunu::piaabo::digest::sha256_hex(manifest_contents);

    for (const auto &instrument : kV2Instruments) {
      for (const auto &interval : kV2Intervals) {
        auto chart =
            parse_v2_development_chart_csv(repo_root_, instrument.id, interval);
        const std::string manifest_prefix =
            "csv.development_prefix." + std::string(instrument.id) + "." +
            std::string(interval.id) + ".";
        const std::string expected_benchmark_relative =
            "data/development_prefix/" + std::string(instrument.id) + "/" +
            std::string(interval.id) + "/" + std::string(instrument.id) +
            "-" + std::string(interval.id) + "-all-years.csv";
        require_manifest_value(manifest_, manifest_prefix + "relative_path",
                               expected_benchmark_relative);
        require_manifest_value(manifest_, manifest_prefix + "sha256",
                               chart.sha256);
        charts_.emplace(
            chart_key_t{std::string(instrument.id), std::string(interval.id)},
            std::move(chart));
      }
    }
    build_chart_json_documents();
    build_catalog_json();
  }

  void build_chart_json_documents() {
    using namespace synthetic_chart_detail;
    for (const auto &instrument : kV2Instruments) {
      for (const auto &interval : kV2Intervals) {
        const chart_key_t key{std::string(instrument.id),
                              std::string(interval.id)};
        const auto &chart = charts_.at(key);
        chart_summary_t summary;
        std::ostringstream points;
        points << '[';
        const std::size_t point_count =
            chart.rows.size() - interval.input_length;
        for (std::size_t local_anchor = 0U; local_anchor < point_count;
             ++local_anchor) {
          const std::size_t row_index = interval.input_length + local_anchor;
          const std::size_t graph_anchor = local_anchor * interval.step_days;
          require(graph_anchor < kSyntheticV2ServedAnchorEndExclusive,
                  "v2 chart point crossed the development anchor boundary");
          const auto &row = chart.rows[row_index];
          const double log_return =
              std::log(row.close / chart.rows[row_index - 1U].close);
          require(std::isfinite(log_return),
                  "computed v2 close log return is not finite");
          summary.minimum_close = std::min(summary.minimum_close, row.close);
          summary.maximum_close = std::max(summary.maximum_close, row.close);
          summary.close_sum += row.close;
          summary.minimum_log_return =
              std::min(summary.minimum_log_return, log_return);
          summary.maximum_log_return =
              std::max(summary.maximum_log_return, log_return);
          summary.log_return_sum += log_return;
          if (log_return > 0.0) {
            ++summary.positive_log_return_count;
          } else if (log_return < 0.0) {
            ++summary.negative_log_return_count;
          } else {
            ++summary.zero_log_return_count;
          }
          if (local_anchor != 0U) {
            points << ',';
          }
          points << '[' << graph_anchor << ',' << row.open_time_ms << ','
                 << row.close_time_ms << ',' << json_number(row.open) << ','
                 << json_number(row.high) << ',' << json_number(row.low) << ','
                 << json_number(row.close) << ',' << json_number(row.volume)
                 << ',' << json_number(log_return) << ']';
        }
        points << ']';

        const double count = static_cast<double>(point_count);
        std::ostringstream output;
        output << "{\"schema_id\":\"iinuji.synthetic_chart.v2\""
               << ",\"benchmark_id\":\"" << kSyntheticBenchmarkV2Id << "\""
               << ",\"data_scope\":\"development_prefix_only\""
               << ",\"instrument\":\"" << instrument.id << "\""
               << ",\"interval\":\"" << interval.id << "\""
               << ",\"fundamental_period_anchors\":"
               << json_number(instrument.dominant_period_anchors)
               << ",\"input_length\":" << interval.input_length
               << ",\"step_days\":" << interval.step_days
               << ",\"anchor_begin\":0"
               << ",\"anchor_end_exclusive\":"
               << kSyntheticV2ServedAnchorEndExclusive
               << ",\"last_point_anchor\":"
               << ((point_count - 1U) * interval.step_days)
               << ",\"raw_row_begin\":" << interval.input_length
               << ",\"raw_row_end_exclusive\":" << chart.rows.size()
               << ",\"test_holdout_served\":false"
               << ",\"raw_source_served\":false"
               << ",\"point_fields\":[\"anchor\",\"open_time_ms\","
                  "\"close_time_ms\",\"open\",\"high\",\"low\","
                  "\"close\",\"volume\",\"log_return\"]"
               << ",\"summary\":{\"point_count\":" << point_count
               << ",\"min_close\":" << json_number(summary.minimum_close)
               << ",\"max_close\":" << json_number(summary.maximum_close)
               << ",\"mean_close\":"
               << json_number(summary.close_sum / count)
               << ",\"min_log_return\":"
               << json_number(summary.minimum_log_return)
               << ",\"max_log_return\":"
               << json_number(summary.maximum_log_return)
               << ",\"mean_log_return\":"
               << json_number(summary.log_return_sum / count)
               << ",\"positive_log_return_count\":"
               << summary.positive_log_return_count
               << ",\"negative_log_return_count\":"
               << summary.negative_log_return_count
               << ",\"zero_log_return_count\":"
               << summary.zero_log_return_count << '}'
               << ",\"source\":{\"scope\":\"development_prefix\","
                  "\"csv_path\":\""
               << json_escape(chart.relative_path) << "\",\"csv_sha256\":\""
               << chart.sha256 << "\",\"row_count\":" << chart.rows.size()
               << "},\"points\":" << points.str() << '}';
        chart_json_.emplace(key, output.str());
      }
    }
  }

  void build_catalog_json() {
    using namespace synthetic_chart_detail;
    std::ostringstream output;
    output << "{\"schema_id\":\"iinuji.synthetic_chart_catalog.v2\""
           << ",\"benchmark_id\":\"" << kSyntheticBenchmarkV2Id << "\""
           << ",\"display_name\":\"V2 causal aggregate process\""
           << ",\"data_scope\":\"development_prefix_only\""
           << ",\"master_day_count\":4137"
           << ",\"accepted_anchor_count\":"
           << kSyntheticV2AcceptedAnchorCount
           << ",\"max_input_length\":30"
           << ",\"served_anchor_end_exclusive\":"
           << kSyntheticV2ServedAnchorEndExclusive
           << ",\"test_holdout_served\":false"
           << ",\"raw_source_served\":false"
           << ",\"development_prefix_row_counts\":{\"1d\":3294,"
              "\"3d\":1098,\"1w\":471}"
           << ",\"source\":{\"chart_family\":\""
           << json_escape(required_value(manifest_, "chart_family"))
           << "\",\"chart_drift_model\":\"none\""
           << ",\"source_function_digest\":\""
           << required_value(manifest_, "source_process_digest")
           << "\",\"seed_commitment_sha256\":\""
           << required_value(manifest_, "seed_commitment_sha256")
           << "\",\"manifest_path\":\"" << kV2ManifestRelativePath
           << "\",\"manifest_sha256\":\"" << manifest_sha256_ << "\"}"
           << ",\"splits\":["
           << "{\"id\":\"train\",\"begin\":0,\"end_exclusive\":2496,"
              "\"served\":true},"
           << "{\"id\":\"purge_1\",\"begin\":2496,"
              "\"end_exclusive\":2560,\"served\":true},"
           << "{\"id\":\"validation\",\"begin\":2560,"
              "\"end_exclusive\":2816,\"served\":true},"
           << "{\"id\":\"purge_2\",\"begin\":2816,"
              "\"end_exclusive\":2880,\"served\":true},"
           << "{\"id\":\"certified_development\",\"begin\":2880,"
              "\"end_exclusive\":3264,\"served\":true},"
           << "{\"id\":\"purge_3\",\"begin\":3264,"
              "\"end_exclusive\":3328,\"served\":false},"
           << "{\"id\":\"test_holdout\",\"begin\":3328,"
              "\"end_exclusive\":4096,\"served\":false}]"
           << ",\"intervals\":[";
    for (std::size_t index = 0U; index < kV2Intervals.size(); ++index) {
      if (index != 0U) {
        output << ',';
      }
      const auto &interval = kV2Intervals[index];
      output << "{\"id\":\"" << interval.id
             << "\",\"input_length\":" << interval.input_length
             << ",\"step_days\":" << interval.step_days
             << ",\"development_prefix_row_count\":"
             << interval.development_row_count << '}';
    }
    output << "],\"instruments\":[";
    for (std::size_t instrument_index = 0U;
         instrument_index < kV2Instruments.size(); ++instrument_index) {
      if (instrument_index != 0U) {
        output << ',';
      }
      const auto &instrument = kV2Instruments[instrument_index];
      output << "{\"id\":\"" << instrument.id
             << "\",\"fundamental_period_anchors\":"
             << json_number(instrument.dominant_period_anchors)
             << ",\"cross_interval_ohlc\":{\"identical_by_row\":false,"
                "\"aggregation\":\"causal_nonoverlapping\"},\"charts\":[";
      for (std::size_t interval_index = 0U;
           interval_index < kV2Intervals.size(); ++interval_index) {
        if (interval_index != 0U) {
          output << ',';
        }
        const auto &interval = kV2Intervals[interval_index];
        const auto &chart = charts_.at(
            {std::string(instrument.id), std::string(interval.id)});
        output << "{\"interval\":\"" << interval.id
               << "\",\"csv_path\":\"" << json_escape(chart.relative_path)
               << "\",\"csv_sha256\":\"" << chart.sha256
               << "\",\"row_count\":" << chart.rows.size() << '}';
      }
      output << "]}";
    }
    output << "],\"validation\":{\"csv_file_count\":9,"
              "\"rows_finite\":true,\"timestamps_ordered\":true,"
              "\"ohlc_valid\":true,\"causal_aggregates\":true,"
              "\"development_prefix_only\":true,"
              "\"holdout_truncated\":true}}";
    catalog_json_ = output.str();
  }

  std::filesystem::path repo_root_;
  std::map<std::string, std::string> manifest_;
  std::string manifest_sha256_;
  std::map<chart_key_t, synthetic_chart_detail::chart_data_t> charts_;
  std::map<chart_key_t, std::string> chart_json_;
  std::string catalog_json_;
};

[[nodiscard]] inline std::filesystem::path discover_iinuji_repo_root(
    const std::filesystem::path &start = std::filesystem::current_path()) {
  std::error_code error;
  auto cursor = std::filesystem::absolute(start, error);
  synthetic_chart_detail::require(!error,
                                  "cannot make repo-root search path absolute");
  if (std::filesystem::is_regular_file(cursor)) {
    cursor = cursor.parent_path();
  }
  cursor = cursor.lexically_normal();
  while (!cursor.empty()) {
    const auto manifest =
        cursor /
        std::filesystem::path(synthetic_chart_detail::kManifestRelativePath);
    const auto channels =
        cursor /
        std::filesystem::path(synthetic_chart_detail::kChannelsRelativePath);
    const auto splits =
        cursor /
        std::filesystem::path(synthetic_chart_detail::kSplitsRelativePath);
    if (std::filesystem::is_regular_file(manifest) &&
        std::filesystem::is_regular_file(channels) &&
        std::filesystem::is_regular_file(splits)) {
      return synthetic_chart_detail::canonical_repo_root(cursor);
    }
    const auto parent = cursor.parent_path();
    if (parent == cursor || parent.empty()) {
      break;
    }
    cursor = parent;
  }
  synthetic_chart_detail::fail("could not discover repo root from " +
                               start.string() +
                               "; pass --repo-root explicitly");
}

} // namespace cuwacunu::iinuji::html
