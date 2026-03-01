/* tsiemene_circuit_runtime_invoke.cpp */
#include "tsiemene_circuit_runtime_internal.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {

namespace tsiemene_circuit_runtime_internal {

std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

} /* namespace tsiemene_circuit_runtime_internal */

namespace {

using tsiemene_circuit_runtime_internal::trim_ascii_ws_copy;

[[nodiscard]] std::optional<std::uint64_t> parse_u64_strict_(std::string_view s) {
  std::uint64_t out = 0;
  const auto* b = s.data();
  const auto* e = s.data() + s.size();
  const auto r = std::from_chars(b, e, out);
  if (r.ec != std::errc{} || r.ptr != e) return std::nullopt;
  return out;
}

[[nodiscard]] std::optional<std::int64_t> parse_i64_strict_(std::string_view s) {
  std::int64_t out = 0;
  const auto* b = s.data();
  const auto* e = s.data() + s.size();
  const auto r = std::from_chars(b, e, out);
  if (r.ec != std::errc{} || r.ptr != e) return std::nullopt;
  return out;
}

[[nodiscard]] std::string to_lower_ascii_(std::string s) {
  for (char& c : s) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return s;
}

[[nodiscard]] constexpr bool is_leap_year_(int y) noexcept {
  return ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
}

[[nodiscard]] constexpr int days_in_month_(int y, int m) noexcept {
  constexpr int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 0;
  if (m == 2 && is_leap_year_(y)) return 29;
  return kDays[m - 1];
}

// Whole UTC days since unix epoch (1970-01-01).
[[nodiscard]] constexpr std::int64_t days_from_civil_utc_(int y,
                                                           unsigned m,
                                                           unsigned d) noexcept {
  y -= (m <= 2) ? 1 : 0;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned mp = (m > 2) ? (m - 3) : (m + 9);
  const unsigned doy = (153 * mp + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<std::int64_t>(era) * 146097 +
         static_cast<std::int64_t>(doe) - 719468;
}

[[nodiscard]] std::optional<std::int64_t>
parse_ddmmyyyy_to_unix_ms_(std::string_view ddmmyyyy, bool end_of_day) {
  const std::size_t p1 = ddmmyyyy.find('.');
  if (p1 == std::string_view::npos) return std::nullopt;
  const std::size_t p2 = ddmmyyyy.find('.', p1 + 1);
  if (p2 == std::string_view::npos) return std::nullopt;

  const auto d = parse_i64_strict_(ddmmyyyy.substr(0, p1));
  const auto m = parse_i64_strict_(ddmmyyyy.substr(p1 + 1, p2 - p1 - 1));
  const auto y = parse_i64_strict_(ddmmyyyy.substr(p2 + 1));
  if (!d || !m || !y) return std::nullopt;
  if (*y < 1970 || *m < 1 || *m > 12) return std::nullopt;

  const int dim = days_in_month_(static_cast<int>(*y), static_cast<int>(*m));
  if (*d < 1 || *d > dim) return std::nullopt;

  constexpr std::int64_t kMsPerDay = 24LL * 60LL * 60LL * 1000LL;
  const std::int64_t day_index = days_from_civil_utc_(
      static_cast<int>(*y),
      static_cast<unsigned>(*m),
      static_cast<unsigned>(*d));
  if (day_index < 0) return std::nullopt;

  const std::int64_t day_start_ms = day_index * kMsPerDay;
  return day_start_ms + (end_of_day ? (kMsPerDay - 1) : 0);
}

[[nodiscard]] std::string extract_symbol_from_command_(std::string cmd) {
  cmd = trim_ascii_ws_copy(std::move(cmd));
  if (cmd.empty()) return {};
  if (cmd.rfind("batches=", 0) == 0) return {};
  const std::size_t lb = cmd.find('[');
  if (lb == std::string::npos) return cmd;
  return trim_ascii_ws_copy(cmd.substr(0, lb));
}

bool parse_wave_envelope_(std::string payload,
                          tsiemene_wave_invoke_t* out,
                          std::string* error) {
  if (!out) return false;
  *out = tsiemene_wave_invoke_t{};

  payload = trim_ascii_ws_copy(std::move(payload));
  if (payload.empty()) {
    if (error) *error = "empty circuit invoke payload";
    return false;
  }

  constexpr std::string_view kPrefix = "wave@";
  if (payload.rfind(std::string(kPrefix), 0) != 0) {
    out->source_command = payload;
    out->source_symbol = extract_symbol_from_command_(out->source_command);
    return true;
  }

  const std::string rest = payload.substr(kPrefix.size());
  const std::size_t sep = rest.find('@');
  if (sep == std::string::npos) {
    if (error) {
      *error = "wave invoke envelope missing source command separator '@': " +
               payload;
    }
    return false;
  }

  const std::string meta = trim_ascii_ws_copy(rest.substr(0, sep));
  const std::string source_command = trim_ascii_ws_copy(rest.substr(sep + 1));
  if (source_command.empty()) {
    if (error) *error = "wave invoke envelope has empty source command";
    return false;
  }

  bool has_wave_i = false;
  bool has_from = false;
  bool has_to = false;
  std::int64_t span_from_ms = 0;
  std::int64_t span_to_ms = 0;

  std::size_t i = 0;
  while (i < meta.size()) {
    std::size_t j = i;
    while (j < meta.size() && meta[j] != ',') ++j;
    std::string item = trim_ascii_ws_copy(meta.substr(i, j - i));
    if (item.empty()) {
      if (error) *error = "empty wave invoke metadata token";
      return false;
    }

    const std::size_t c = item.find(':');
    if (c == std::string::npos) {
      if (error) {
        *error =
            "invalid wave invoke metadata token (expected key:value): " + item;
      }
      return false;
    }
    std::string key = to_lower_ascii_(trim_ascii_ws_copy(item.substr(0, c)));
    std::string val = trim_ascii_ws_copy(item.substr(c + 1));
    if (key.empty() || val.empty()) {
      if (error) {
        *error =
            "invalid empty key/value in wave invoke metadata token: " + item;
      }
      return false;
    }

    if (key == "symbol") {
      out->source_symbol = val;
    } else if (key == "epochs") {
      const auto v = parse_u64_strict_(val);
      if (!v || *v == 0) {
        if (error) {
          *error = "invalid epochs value in wave invoke metadata: " + val;
        }
        return false;
      }
      out->total_epochs = *v;
    } else if (key == "episode") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) {
          *error = "invalid episode value in wave invoke metadata: " + val;
        }
        return false;
      }
      out->episode = *v;
    } else if (key == "batch") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) *error = "invalid batch value in wave invoke metadata: " + val;
        return false;
      }
      out->batch = *v;
    } else if (key == "max_batches") {
      const auto v = parse_u64_strict_(val);
      if (!v || *v == 0) {
        if (error) {
          *error =
              "invalid max_batches value in wave invoke metadata: " + val;
        }
        return false;
      }
      out->max_batches_per_epoch = *v;
    } else if (key == "i") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) *error = "invalid i value in wave invoke metadata: " + val;
        return false;
      }
      out->wave_i = *v;
      has_wave_i = true;
    } else if (key == "from") {
      const auto v = parse_ddmmyyyy_to_unix_ms_(val, /*end_of_day=*/false);
      if (!v) {
        if (error) *error = "invalid from date (expected dd.mm.yyyy): " + val;
        return false;
      }
      span_from_ms = *v;
      has_from = true;
    } else if (key == "to") {
      const auto v = parse_ddmmyyyy_to_unix_ms_(val, /*end_of_day=*/true);
      if (!v) {
        if (error) *error = "invalid to date (expected dd.mm.yyyy): " + val;
        return false;
      }
      span_to_ms = *v;
      has_to = true;
    } else if (key == "from_ms") {
      const auto v = parse_i64_strict_(val);
      if (!v) {
        if (error) *error = "invalid from_ms value in wave invoke metadata: " + val;
        return false;
      }
      if (*v < 0) {
        if (error) *error = "from_ms must be >= 0 in wave invoke metadata: " + val;
        return false;
      }
      span_from_ms = *v;
      has_from = true;
    } else if (key == "to_ms") {
      const auto v = parse_i64_strict_(val);
      if (!v) {
        if (error) *error = "invalid to_ms value in wave invoke metadata: " + val;
        return false;
      }
      if (*v < 0) {
        if (error) *error = "to_ms must be >= 0 in wave invoke metadata: " + val;
        return false;
      }
      span_to_ms = *v;
      has_to = true;
    } else {
      if (error) *error = "unknown wave invoke metadata key: " + key;
      return false;
    }

    i = (j < meta.size()) ? (j + 1) : j;
  }

  if (has_from != has_to) {
    if (error) {
      *error =
          "wave invoke metadata requires both from/to (or from_ms/to_ms) when one is provided";
    }
    return false;
  }

  out->source_command = source_command;
  if (out->source_symbol.empty()) {
    out->source_symbol = extract_symbol_from_command_(source_command);
  }
  if (!has_wave_i) out->wave_i = out->batch;
  if (has_from && has_to) {
    out->has_time_span = true;
    out->span_begin_ms = std::min(span_from_ms, span_to_ms);
    out->span_end_ms = std::max(span_from_ms, span_to_ms);
  }
  return true;
}

} /* namespace */

std::optional<tsiemene::DirectiveId> parse_directive_ref(std::string s) {
  return tsiemene::parse_directive_id(std::move(s));
}

std::optional<tsiemene::PayloadKind> parse_kind_ref(std::string s) {
  s = tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(std::move(s));
  if (s == "tensor" || s == ":tensor") return tsiemene::PayloadKind::Tensor;
  if (s == "str" || s == ":str") return tsiemene::PayloadKind::String;
  return std::nullopt;
}

bool parse_circuit_invoke_wave(const tsiemene_circuit_decl_t& circuit,
                               tsiemene_wave_invoke_t* out,
                               std::string* error) {
  return parse_wave_envelope_(circuit.invoke_payload, out, error);
}

std::string circuit_invoke_command(const tsiemene_circuit_decl_t& circuit) {
  tsiemene_wave_invoke_t parsed{};
  std::string local_error;
  if (!parse_circuit_invoke_wave(circuit, &parsed, &local_error)) return {};
  return parsed.source_command;
}

std::string circuit_invoke_symbol(const tsiemene_circuit_decl_t& circuit) {
  tsiemene_wave_invoke_t parsed{};
  std::string local_error;
  if (!parse_circuit_invoke_wave(circuit, &parsed, &local_error)) return {};
  return parsed.source_symbol;
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */
