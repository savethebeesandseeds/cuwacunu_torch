// ./include/hero/lattice_hero/source_runtime_projection.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"

namespace cuwacunu {
namespace hero {
namespace wave {

struct source_runtime_channel_state_t {
  std::string interval{};
  bool active{false};
};

struct source_runtime_projection_input_t {
  std::string symbol{};
  std::string from_date_ddmmyyyy{};
  std::string to_date_ddmmyyyy{};
  double domain_from_ms{0.0};
  double domain_to_ms{0.0};  // Exclusive.
  std::vector<source_runtime_channel_state_t> channel_states{};
};

struct source_runtime_projection_fragment_t {
  std::vector<std::pair<std::string, double>> projection_num{};
  std::vector<std::pair<std::string, std::string>> projection_txt{};
  std::string projection_lls{};
};

namespace source_runtime_projection_detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] inline bool parse_i64_strict(std::string_view text,
                                           std::int64_t* out) {
  if (!out) return false;
  const std::string t = trim_ascii(text);
  if (t.empty()) return false;
  std::int64_t value = 0;
  const auto* b = t.data();
  const auto* e = t.data() + t.size();
  const auto r = std::from_chars(b, e, value);
  if (r.ec != std::errc{} || r.ptr != e) return false;
  *out = value;
  return true;
}

[[nodiscard]] inline constexpr bool is_leap_year(int y) noexcept {
  return ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
}

[[nodiscard]] inline constexpr int days_in_month(int y, int m) noexcept {
  constexpr int kDays[12] = {31, 28, 31, 30, 31, 30,
                             31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 0;
  if (m == 2 && is_leap_year(y)) return 29;
  return kDays[m - 1];
}

[[nodiscard]] inline constexpr std::int64_t days_from_civil_utc(
    int y, unsigned m, unsigned d) noexcept {
  y -= (m <= 2) ? 1 : 0;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned mp = (m > 2) ? (m - 3) : (m + 9);
  const unsigned doy = (153 * mp + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<std::int64_t>(era) * 146097 +
         static_cast<std::int64_t>(doe) - 719468;
}

[[nodiscard]] inline bool parse_day_start_utc_ms(
    std::string_view ddmmyyyy,
    std::int64_t* out_ms,
    std::string* error) {
  if (error) error->clear();
  if (!out_ms) {
    if (error) *error = "day-start output pointer is null";
    return false;
  }
  *out_ms = 0;

  const std::string value = trim_ascii(ddmmyyyy);
  const std::size_t p1 = value.find('.');
  if (p1 == std::string::npos) {
    if (error) *error = "date must be DD.MM.YYYY";
    return false;
  }
  const std::size_t p2 = value.find('.', p1 + 1);
  if (p2 == std::string::npos) {
    if (error) *error = "date must be DD.MM.YYYY";
    return false;
  }

  std::int64_t d = 0;
  std::int64_t m = 0;
  std::int64_t y = 0;
  if (!parse_i64_strict(value.substr(0, p1), &d) ||
      !parse_i64_strict(value.substr(p1 + 1, p2 - p1 - 1), &m) ||
      !parse_i64_strict(value.substr(p2 + 1), &y)) {
    if (error) *error = "date contains non-numeric parts";
    return false;
  }
  if (y < 1970 || m < 1 || m > 12) {
    if (error) *error = "date out of supported range";
    return false;
  }
  const int dim = days_in_month(static_cast<int>(y), static_cast<int>(m));
  if (d < 1 || d > dim) {
    if (error) *error = "day is out of range for month";
    return false;
  }

  constexpr std::int64_t kMsPerDay = 24LL * 60LL * 60LL * 1000LL;
  const std::int64_t day_index = days_from_civil_utc(
      static_cast<int>(y),
      static_cast<unsigned>(m),
      static_cast<unsigned>(d));
  if (day_index < 0) {
    if (error) *error = "date before unix epoch is unsupported";
    return false;
  }
  *out_ms = day_index * kMsPerDay;
  return true;
}

[[nodiscard]] inline std::string sanitize_interval_token(std::string token) {
  token = trim_ascii(token);
  if (token.empty()) return {};
  for (char& c : token) {
    const bool ok = (std::isalnum(static_cast<unsigned char>(c)) != 0) ||
                    c == '_' || c == '.' || c == '-';
    if (!ok) c = '_';
  }
  return token;
}

[[nodiscard]] inline double clamp_ratio_01(double v) {
  if (!std::isfinite(v)) return 0.0;
  if (v < 0.0) return 0.0;
  if (v > 1.0) return 1.0;
  return v;
}

[[nodiscard]] inline std::string format_double(double v) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10) << v;
  return oss.str();
}

}  // namespace source_runtime_projection_detail

[[nodiscard]] inline bool build_source_runtime_projection_fragment(
    const source_runtime_projection_input_t& input,
    source_runtime_projection_fragment_t* out,
    std::string* error) {
  using namespace source_runtime_projection_detail;
  if (error) error->clear();
  if (!out) {
    if (error) *error = "projection fragment output pointer is null";
    return false;
  }
  *out = source_runtime_projection_fragment_t{};

  const std::string symbol = trim_ascii(input.symbol);
  if (symbol.empty()) {
    if (error) *error = "missing source symbol";
    return false;
  }

  std::int64_t from_day_ms = 0;
  std::string date_error;
  if (!parse_day_start_utc_ms(input.from_date_ddmmyyyy, &from_day_ms,
                              &date_error)) {
    if (error) *error = "invalid FROM date: " + date_error;
    return false;
  }
  std::int64_t to_day_ms = 0;
  if (!parse_day_start_utc_ms(input.to_date_ddmmyyyy, &to_day_ms,
                              &date_error)) {
    if (error) *error = "invalid TO date: " + date_error;
    return false;
  }
  if (from_day_ms > to_day_ms) {
    if (error) *error = "SOURCE.RUNTIME.FROM must be <= TO";
    return false;
  }

  constexpr double kMsPerDay = 24.0 * 60.0 * 60.0 * 1000.0;
  const double request_from_ms = static_cast<double>(from_day_ms);
  const double request_to_ms =
      static_cast<double>(to_day_ms) + kMsPerDay;  // Half-open [from,to).

  if (!std::isfinite(input.domain_from_ms) || !std::isfinite(input.domain_to_ms)) {
    if (error) *error = "source domain bounds are non-finite";
    return false;
  }
  const double domain_from_ms = input.domain_from_ms;
  const double domain_to_ms = input.domain_to_ms;
  if (!(domain_to_ms > domain_from_ms)) {
    if (error) *error = "source domain span must be > 0";
    return false;
  }

  const double domain_span_ms = domain_to_ms - domain_from_ms;
  const bool clipped_left = (request_from_ms < domain_from_ms);
  const bool clipped_right = (request_to_ms > domain_to_ms);

  const double request_from_ratio =
      clamp_ratio_01((request_from_ms - domain_from_ms) / domain_span_ms);
  const double request_to_ratio =
      clamp_ratio_01((request_to_ms - domain_from_ms) / domain_span_ms);
  const double request_span_ratio =
      clamp_ratio_01((request_to_ms - request_from_ms) / domain_span_ms);

  const double effective_from_ms =
      std::clamp(request_from_ms, domain_from_ms, domain_to_ms);
  const double effective_to_ms =
      std::clamp(request_to_ms, domain_from_ms, domain_to_ms);
  const double covered_ms =
      std::max(0.0, effective_to_ms - effective_from_ms);
  const double coverage_ratio = clamp_ratio_01(covered_ms / domain_span_ms);

  std::map<std::string, bool> interval_active{};
  for (const auto& ch : input.channel_states) {
    const std::string interval = sanitize_interval_token(ch.interval);
    if (interval.empty()) continue;
    const auto it = interval_active.find(interval);
    if (it == interval_active.end()) {
      interval_active.emplace(interval, ch.active);
    } else {
      it->second = (it->second || ch.active);
    }
  }
  if (interval_active.empty()) {
    if (error) *error = "no channel interval states resolved for projection";
    return false;
  }

  std::size_t active_count = 0;
  for (const auto& [_, is_active] : interval_active) {
    if (is_active) ++active_count;
  }
  const std::size_t total_count = interval_active.size();
  const double active_ratio =
      (total_count == 0)
          ? 0.0
          : (static_cast<double>(active_count) / static_cast<double>(total_count));

  out->projection_num = {
      {"source.runtime.request.from_ratio", request_from_ratio},
      {"source.runtime.request.to_ratio", request_to_ratio},
      {"source.runtime.request.span_ratio", request_span_ratio},
      {"source.runtime.effective.coverage_ratio", coverage_ratio},
      {"source.runtime.flags.clipped_left", clipped_left ? 1.0 : 0.0},
      {"source.runtime.flags.clipped_right", clipped_right ? 1.0 : 0.0},
      {"source.channels.active_count", static_cast<double>(active_count)},
      {"source.channels.total_count", static_cast<double>(total_count)},
      {"source.channels.active_ratio", active_ratio},
  };

  for (const auto& [interval, is_active] : interval_active) {
    out->projection_num.emplace_back(
        "source.channel." + interval + ".active", is_active ? 1.0 : 0.0);
  }

  out->projection_txt.emplace_back("source.runtime.symbol", symbol);
  out->projection_txt.emplace_back("source.runtime.range_basis",
                                   "effective_intersection");
  out->projection_txt.emplace_back(
      "source.runtime.interval_semantics", "half_open_utc_day");

  std::sort(out->projection_num.begin(), out->projection_num.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  std::sort(out->projection_txt.begin(), out->projection_txt.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  std::ostringstream projection;
  projection << "# source.runtime.projection.v2\n";
  projection << "source.runtime.projection.schema=wave.source.runtime.projection.v2\n";

  projection << "\n# section.projection_num\n";
  for (const auto& [k, v] : out->projection_num) {
    projection << k << "=" << format_double(v) << "\n";
  }

  projection << "\n# section.projection_txt\n";
  for (const auto& [k, v] : out->projection_txt) {
    projection << k << "=" << v << "\n";
  }

  projection << "\n# section.audit\n";
  projection << "source.runtime.debug.domain_from_ms=" << format_double(domain_from_ms)
             << "\n";
  projection << "source.runtime.debug.domain_to_ms=" << format_double(domain_to_ms)
             << "\n";
  projection << "source.runtime.debug.domain_span_ms=" << format_double(domain_span_ms)
             << "\n";
  projection << "source.runtime.debug.request_from_ms=" << format_double(request_from_ms)
             << "\n";
  projection << "source.runtime.debug.request_to_ms=" << format_double(request_to_ms)
             << "\n";
  projection << "source.runtime.debug.request_span_ms="
             << format_double(std::max(0.0, request_to_ms - request_from_ms)) << "\n";
  projection << "source.runtime.debug.effective_from_ms="
             << format_double(effective_from_ms) << "\n";
  projection << "source.runtime.debug.effective_to_ms="
             << format_double(effective_to_ms) << "\n";
  projection << "source.runtime.debug.covered_ms=" << format_double(covered_ms)
             << "\n";
  projection << "source.runtime.debug.flags.clipped_left="
        << (clipped_left ? "1" : "0") << "\n";
  projection << "source.runtime.debug.flags.clipped_right="
        << (clipped_right ? "1" : "0") << "\n";

  out->projection_lls =
      cuwacunu::camahjucunu::dsl::convert_latent_lineage_state_payload_to_lattice_state(
          projection.str());
  return true;
}

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
