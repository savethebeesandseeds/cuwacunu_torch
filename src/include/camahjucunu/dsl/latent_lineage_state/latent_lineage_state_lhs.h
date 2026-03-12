// ./include/camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

inline constexpr std::string_view kLatentLineageStateFileExtension = ".dsl";

struct latent_lineage_state_lhs_t {
  std::string key{};
  std::string declared_domain{};
  std::string declared_type{};
};

[[nodiscard]] inline std::string trim_ascii_ws_copy(std::string_view text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] inline std::size_t find_top_level_colon(
    std::string_view text) {
  int square_depth = 0;
  int round_depth = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '[') {
      ++square_depth;
      continue;
    }
    if (c == ']') {
      if (square_depth > 0) --square_depth;
      continue;
    }
    if (c == '(') {
      ++round_depth;
      continue;
    }
    if (c == ')') {
      if (round_depth > 0) --round_depth;
      continue;
    }
    if (c == ':' && square_depth == 0 && round_depth == 0) {
      return i;
    }
  }
  return std::string_view::npos;
}

[[nodiscard]] inline std::size_t trailing_domain_begin_index(
    std::string_view text) {
  if (text.empty()) return std::string_view::npos;
  const char close = text.back();
  if (close != ']' && close != ')') return std::string_view::npos;
  const char open = (close == ']') ? '[' : '(';
  int depth = 0;
  for (std::size_t i = text.size(); i-- > 0;) {
    const char c = text[i];
    if (c == close) {
      ++depth;
      continue;
    }
    if (c == open) {
      --depth;
      if (depth == 0) return i;
    }
  }
  return std::string_view::npos;
}

[[nodiscard]] inline latent_lineage_state_lhs_t parse_latent_lineage_state_lhs(
    std::string_view raw_lhs) {
  latent_lineage_state_lhs_t out{};
  std::string lhs = trim_ascii_ws_copy(raw_lhs);
  if (lhs.empty()) return out;

  const std::size_t colon = find_top_level_colon(lhs);
  std::string left = lhs;
  if (colon != std::string::npos) {
    left = trim_ascii_ws_copy(lhs.substr(0, colon));
    out.declared_type = trim_ascii_ws_copy(lhs.substr(colon + 1));
  }

  const std::size_t domain_begin = trailing_domain_begin_index(left);
  if (domain_begin != std::string::npos) {
    out.key = trim_ascii_ws_copy(
        std::string_view(left).substr(0, domain_begin));
    out.declared_domain =
        trim_ascii_ws_copy(std::string_view(left).substr(domain_begin));
    if (out.key.empty()) {
      out.key = trim_ascii_ws_copy(left);
      out.declared_domain.clear();
    }
  } else {
    out.key = trim_ascii_ws_copy(left);
  }

  return out;
}

[[nodiscard]] inline std::string extract_latent_lineage_state_lhs_key(
    std::string_view lhs) {
  return parse_latent_lineage_state_lhs(lhs).key;
}

[[nodiscard]] inline std::string lowercase_ascii_copy(std::string_view text) {
  std::string out(text);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] inline bool parse_bool_ascii(std::string_view text, bool* out) {
  if (!out) return false;
  const std::string lowered = lowercase_ascii_copy(trim_ascii_ws_copy(text));
  if (lowered == "true" || lowered == "false") {
    *out = (lowered == "true");
    return true;
  }
  return false;
}

[[nodiscard]] inline bool parse_i64_ascii(std::string_view text,
                                          std::int64_t* out) {
  if (!out) return false;
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) return false;
  std::int64_t parsed = 0;
  const auto r =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (r.ec != std::errc{} || r.ptr != value.data() + value.size()) return false;
  *out = parsed;
  return true;
}

[[nodiscard]] inline bool parse_double_ascii(std::string_view text,
                                             double* out) {
  if (!out) return false;
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == nullptr || end != value.c_str() + value.size()) {
    return false;
  }
  *out = parsed;
  return true;
}

[[nodiscard]] inline std::string infer_declared_type_from_value(
    std::string_view value) {
  const std::string token = trim_ascii_ws_copy(value);
  if (token.empty()) return "str";
  bool as_bool = false;
  if (parse_bool_ascii(token, &as_bool)) return "bool";
  std::int64_t as_i64 = 0;
  if (parse_i64_ascii(token, &as_i64)) return "int";
  double as_f64 = 0.0;
  if (parse_double_ascii(token, &as_f64)) return "double";
  return "str";
}

[[nodiscard]] inline std::string default_domain_for_declared_type(
    std::string_view declared_type) {
  const std::string lowered =
      lowercase_ascii_copy(trim_ascii_ws_copy(declared_type));
  if (lowered == "bool") return {};
  if (lowered == "int" || lowered == "long" || lowered == "uint" ||
      lowered == "size_t") {
    return "(-inf,+inf)";
  }
  if (lowered == "float" || lowered == "double" || lowered == "long double") {
    return "(-inf,+inf)";
  }
  return {};
}

[[nodiscard]] inline std::string normalize_assignment_to_lattice_state(
    std::string_view lhs,
    std::string_view rhs) {
  latent_lineage_state_lhs_t parsed = parse_latent_lineage_state_lhs(lhs);
  if (parsed.key.empty()) return {};
  if (parsed.declared_type.empty()) {
    parsed.declared_type = infer_declared_type_from_value(rhs);
  }
  if (parsed.declared_domain.empty()) {
    parsed.declared_domain =
        default_domain_for_declared_type(parsed.declared_type);
  }
  std::string out = parsed.key;
  out += parsed.declared_domain;
  out += ":";
  out += parsed.declared_type;
  out += " = ";
  out += trim_ascii_ws_copy(rhs);
  return out;
}

[[nodiscard]] inline std::string convert_latent_lineage_state_payload_to_lattice_state(
    std::string_view payload) {
  if (payload.empty()) return {};
  std::string out;
  std::size_t cursor = 0;
  while (cursor <= payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();

    std::string_view line = payload.substr(cursor, line_end - cursor);
    bool had_cr = false;
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
      had_cr = true;
    }

    const std::string trimmed = trim_ascii_ws_copy(line);
    const bool is_comment = !trimmed.empty() &&
                            (trimmed.front() == '#' || trimmed.front() == ';' ||
                             (trimmed.size() >= 2 && trimmed[0] == '/' &&
                              trimmed[1] == '/'));
    if (!trimmed.empty() && !is_comment) {
      const std::size_t eq = trimmed.find('=');
      if (eq != std::string::npos && eq > 0) {
        const std::string normalized = normalize_assignment_to_lattice_state(
            trimmed.substr(0, eq), trimmed.substr(eq + 1));
        if (!normalized.empty()) {
          out.append(normalized);
        } else {
          out.append(std::string(line));
        }
      } else {
        out.append(std::string(line));
      }
    } else {
      out.append(std::string(line));
    }

    if (had_cr) out.push_back('\r');
    out.push_back('\n');
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return out;
}

}  // namespace dsl
}  // namespace camahjucunu
}  // namespace cuwacunu
