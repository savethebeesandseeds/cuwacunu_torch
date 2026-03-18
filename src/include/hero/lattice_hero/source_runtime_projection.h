// ./include/hero/lattice_hero/source_runtime_projection.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "tsiemene/tsi.report.h"

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

inline constexpr std::string_view kSourceRuntimeProjectionSchemaV2 =
    "wave.source.runtime.projection.v2";
inline constexpr std::string_view
    kSourceRuntimeProjectionLatestReportFilename =
        "source_runtime_projection.latest.lls";

struct source_runtime_projection_report_identity_t {
  std::string canonical_path{};
  std::string source_runtime_cursor{};
  std::string source_label{};
  std::string binding_id{};
  bool has_wave_cursor{false};
  std::uint64_t wave_cursor{0};
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

[[nodiscard]] inline std::string_view projection_reference_domain(
    std::string_view key) {
  if (key == "source.runtime.request.from_ratio" ||
      key == "source.runtime.request.to_ratio" ||
      key == "source.runtime.request.span_ratio" ||
      key == "source.runtime.effective.coverage_ratio" ||
      key == "source.runtime.flags.clipped_left" ||
      key == "source.runtime.flags.clipped_right" ||
      key == "source.channels.active_ratio" || key.ends_with(".active")) {
    return "(0,1)";
  }
  if (key == "source.channels.active_count" ||
      key == "source.channels.total_count") {
    return "(0,+inf)";
  }
  return "(-inf,+inf)";
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

  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t projection{};
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "source.runtime.projection.schema", "wave.source.runtime.projection.v2"));
  for (const auto& [k, v] : out->projection_num) {
    projection.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
            k, v, std::string(projection_reference_domain(k))));
  }
  for (const auto& [k, v] : out->projection_txt) {
    projection.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(k, v));
  }
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.domain_from_ms", domain_from_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.domain_to_ms", domain_to_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.domain_span_ms", domain_span_ms, "(0,+inf)"));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.request_from_ms", request_from_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.request_to_ms", request_to_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.request_span_ms",
          std::max(0.0, request_to_ms - request_from_ms),
          "(0,+inf)"));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.effective_from_ms", effective_from_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.effective_to_ms", effective_to_ms));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source.runtime.debug.covered_ms", covered_ms, "(0,+inf)"));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          "source.runtime.debug.flags.clipped_left", clipped_left));
  projection.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          "source.runtime.debug.flags.clipped_right", clipped_right));

  try {
    out->projection_lls =
        cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
            projection);
  } catch (const std::exception& e) {
    if (error) {
      *error = std::string("cannot emit source runtime projection .lls: ") +
               e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool
build_source_runtime_projection_runtime_report_document(
    const source_runtime_projection_fragment_t& fragment,
    const source_runtime_projection_report_identity_t& identity,
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "runtime report document output pointer is null";
    return false;
  }
  *out = cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t{};

  if (identity.canonical_path.empty()) {
    if (error) {
      *error = "source runtime projection runtime report missing canonical_path";
    }
    return false;
  }

  auto& entries = out->entries;
  entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", std::string(kSourceRuntimeProjectionSchemaV2)));
  {
    ::tsiemene::component_report_identity_t header_identity{};
    header_identity.canonical_path = identity.canonical_path;
    header_identity.binding_id = identity.binding_id;
    header_identity.source_runtime_cursor = identity.source_runtime_cursor;
    header_identity.has_wave_cursor = identity.has_wave_cursor;
    header_identity.wave_cursor = identity.wave_cursor;
    cuwacunu::piaabo::latent_lineage_state::append_runtime_report_header_entries(
        out, ::tsiemene::make_runtime_report_header(header_identity));
  }
  const std::string source_label =
      identity.source_label.empty() ? identity.canonical_path : identity.source_label;
  if (!source_label.empty()) {
    entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "source_label", source_label));
  }
  for (const auto& [k, v] : fragment.projection_num) {
    entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
            k, v,
            std::string(
                source_runtime_projection_detail::projection_reference_domain(
                    k))));
  }
  for (const auto& [k, v] : fragment.projection_txt) {
    entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            k, v));
  }
  return cuwacunu::piaabo::latent_lineage_state::validate_runtime_lls_document(
      *out, error);
}

[[nodiscard]] inline bool emit_source_runtime_projection_runtime_report(
    const source_runtime_projection_fragment_t& fragment,
    const source_runtime_projection_report_identity_t& identity,
    std::string* out_text,
    std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "runtime report text output pointer is null";
    return false;
  }
  out_text->clear();
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!build_source_runtime_projection_runtime_report_document(
          fragment, identity, &document, error)) {
    return false;
  }
  *out_text =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
          document);
  return true;
}

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
