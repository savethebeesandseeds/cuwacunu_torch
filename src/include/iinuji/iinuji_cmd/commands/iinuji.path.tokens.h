#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
namespace canonical_path_tokens {

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

[[nodiscard]] inline bool is_atom_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

[[nodiscard]] inline std::string to_atom(std::string_view raw) {
  std::string out;
  out.reserve(raw.size());
  for (char c : raw) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (is_atom_char(c)) out.push_back(static_cast<char>(std::tolower(uc)));
    else out.push_back('_');
  }
  if (out.empty()) out = "empty";
  if (!std::isalpha(static_cast<unsigned char>(out.front())) && out.front() != '_') {
    out.insert(out.begin(), 'v');
    out.insert(out.begin() + 1, '_');
  }
  return out;
}

[[nodiscard]] inline std::string make_index_atom(std::size_t idx1) {
  return "n" + std::to_string(idx1);
}

[[nodiscard]] inline bool parse_index_atom(std::string_view atom, std::size_t* out) {
  if (!out || atom.empty()) return false;
  std::string_view digits = atom;
  if (!std::isdigit(static_cast<unsigned char>(digits.front()))) {
    if (digits.rfind("idx", 0) == 0) digits.remove_prefix(3);
    else if (digits.front() == 'n' || digits.front() == 'i' || digits.front() == 'v') digits.remove_prefix(1);
  }
  if (digits.empty()) return false;

  std::size_t value = 0;
  for (char c : digits) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    value = value * 10 + static_cast<std::size_t>(c - '0');
  }
  if (value == 0) return false;
  *out = value;
  return true;
}

[[nodiscard]] inline bool token_matches(std::string_view candidate, std::string_view query) {
  if (lower_ascii_copy(std::string(candidate)) == lower_ascii_copy(std::string(query))) {
    return true;
  }
  return to_atom(candidate) == to_atom(query);
}

}  // namespace canonical_path_tokens
}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
