#pragma once

#include <cctype>
#include <string>

#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
namespace command_aliases {

struct AliasLookupResult {
  bool matched{false};
  std::string canonical{};

  static AliasLookupResult no_match() {
    return {};
  }
  static AliasLookupResult matched_path(std::string path) {
    AliasLookupResult out{};
    out.matched = true;
    out.canonical = std::move(path);
    return out;
  }
};

[[nodiscard]] inline std::string trim_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string normalize_alias_key(std::string s) {
  s = trim_copy(std::move(s));
  std::string out;
  out.reserve(s.size());
  bool last_space = false;
  for (char c : s) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!last_space) out.push_back(' ');
      last_space = true;
      continue;
    }
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    last_space = false;
  }
  return trim_copy(std::move(out));
}

[[nodiscard]] inline AliasLookupResult resolve(const std::string& raw) {
  const std::string normalized = normalize_alias_key(raw);
  if (normalized.empty()) return AliasLookupResult::no_match();
  if (normalized.rfind("iinuji.", 0) == 0 || normalized.rfind("tsi.", 0) == 0) {
    return AliasLookupResult::no_match();
  }

  const auto it = canonical_paths::alias_map().find(normalized);
  if (it == canonical_paths::alias_map().end()) {
    return AliasLookupResult::no_match();
  }
  return AliasLookupResult::matched_path(std::string(canonical_paths::to_text(it->second)));
}

}  // namespace command_aliases
}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
