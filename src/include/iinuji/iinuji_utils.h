/* iinuji_utils.h */
#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace cuwacunu {
namespace iinuji {

inline bool is_unset_color_token(const std::string &s) {
  return s.empty() || s == "<empty>";
}

inline int clamp255(int v) { return std::clamp(v, 0, 255); }

inline int hex_digit_to_int(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'a' && ch <= 'f')
    return 10 + (ch - 'a');
  if (ch >= 'A' && ch <= 'F')
    return 10 + (ch - 'A');
  return -1;
}

inline bool parse_hex_byte_chars(char hi, char lo, int &out) {
  const int h = hex_digit_to_int(hi);
  const int l = hex_digit_to_int(lo);
  if (h < 0 || l < 0)
    return false;
  out = (h << 4) | l;
  return true;
}

inline bool parse_hex_rgb8(const std::string &s, int &r, int &g, int &b) {
  if (s.size() != 7 || s[0] != '#')
    return false;
  int rr = 0;
  int gg = 0;
  int bb = 0;
  if (!parse_hex_byte_chars(s[1], s[2], rr))
    return false;
  if (!parse_hex_byte_chars(s[3], s[4], gg))
    return false;
  if (!parse_hex_byte_chars(s[5], s[6], bb))
    return false;
  r = rr;
  g = gg;
  b = bb;
  return true;
}

inline std::string rgb8_to_hex(int r, int g, int b) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", clamp255(r), clamp255(g),
                clamp255(b));
  return std::string(buf);
}

inline std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

inline std::vector<std::string> split_lines_keep_empty(const std::string &s) {
  std::vector<std::string> out;

  std::size_t i = 0;
  while (i <= s.size()) {
    std::size_t j = s.find('\n', i);
    if (j == std::string::npos)
      j = s.size();
    std::string line = s.substr(i, j - i);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    out.push_back(std::move(line));
    if (j == s.size())
      break;
    i = j + 1;
  }
  return out;
}

inline std::vector<std::string> wrap_text(const std::string &s, int width) {
  std::vector<std::string> out;
  if (width <= 0) {
    out.emplace_back();
    return out;
  }

  auto phys = split_lines_keep_empty(s);
  for (const auto &line : phys) {
    if (line.empty()) {
      out.emplace_back();
      continue;
    }

    std::size_t i = 0;
    while (i < line.size()) {
      std::size_t end =
          std::min(i + static_cast<std::size_t>(width), line.size());
      std::size_t brk = end;
      if (end < line.size()) {
        std::size_t j = line.rfind(' ', end);
        if (j != std::string::npos && j >= i)
          brk = j;
      }
      if (brk == i)
        brk = end;
      out.emplace_back(line.substr(i, brk - i));
      if (brk < line.size() && line[brk] == ' ')
        i = brk + 1;
      else
        i = brk;
    }
  }

  return out;
}

} // namespace iinuji
} // namespace cuwacunu
