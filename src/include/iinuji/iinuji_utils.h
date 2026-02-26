/* iinuji_utils.h */
#pragma once
#include <ncursesw/ncurses.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <climits>
#include <cstdio>

namespace cuwacunu {
namespace iinuji {

inline std::map<std::string, int> color_map;

inline int get_color(const std::string& color_name, int def_r=1000, int def_g=1000, int def_b=1000);

inline bool is_unset_color_token(const std::string& s) {
  return s.empty() || s == "<empty>";
}

inline int clamp255(int v) { return std::clamp(v, 0, 255); }

inline int hex_digit_to_int(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
  if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
  return -1;
}

inline bool parse_hex_byte_chars(char hi, char lo, int& out) {
  const int h = hex_digit_to_int(hi);
  const int l = hex_digit_to_int(lo);
  if (h < 0 || l < 0) return false;
  out = (h << 4) | l;
  return true;
}

inline bool parse_hex_rgb8(const std::string& s, int& r, int& g, int& b) {
  if (s.size() != 7 || s[0] != '#') return false;
  int rr = 0, gg = 0, bb = 0;
  if (!parse_hex_byte_chars(s[1], s[2], rr)) return false;
  if (!parse_hex_byte_chars(s[3], s[4], gg)) return false;
  if (!parse_hex_byte_chars(s[5], s[6], bb)) return false;
  r = rr;
  g = gg;
  b = bb;
  return true;
}

inline std::string rgb8_to_hex(int r, int g, int b) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x",
                clamp255(r), clamp255(g), clamp255(b));
  return std::string(buf);
}

inline bool rgb8_from_color_id(int id, int& r, int& g, int& b) {
  if (!has_colors()) return false;
  if (id < 0 || id >= COLORS) return false;
  short rr=0, gg=0, bb=0;
  if (color_content((short)id, &rr, &gg, &bb) == ERR) return false;
  r = (int)std::lround(rr * 255.0 / 1000.0);
  g = (int)std::lround(gg * 255.0 / 1000.0);
  b = (int)std::lround(bb * 255.0 / 1000.0);
  return true;
}

inline bool rgb8_for_token(const std::string& tok, int& r, int& g, int& b) {
  if (parse_hex_rgb8(tok, r, g, b)) return true;
  if (is_unset_color_token(tok)) return false;
  if (!has_colors()) return false;
  int id = get_color(tok);
  return rgb8_from_color_id(id, r, g, b);
}

inline std::string darken_color_token(const std::string& tok, double factor) {
  int r=0, g=0, b=0;
  if (!rgb8_for_token(tok, r, g, b)) return tok; // fallback: unchanged
  r = clamp255((int)std::lround(r * factor));
  g = clamp255((int)std::lround(g * factor));
  b = clamp255((int)std::lround(b * factor));
  return rgb8_to_hex(r, g, b);
}

// Focus policy helpers:
inline std::string focus_darken_bg_token(const std::string& bg, double factor=0.8) {
  // If the widget uses terminal-default bg, pick a deterministic dark bg for the focus frame.
  if (is_unset_color_token(bg)) return "#000000";
  return darken_color_token(bg, factor);
}
inline std::string focus_darken_fg_token(const std::string& fg, double factor=0.8) {
  if (is_unset_color_token(fg)) return "#505050";
  return darken_color_token(fg, factor);
}

inline std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

inline bool parse_hex_rgb(const std::string& name, int& r, int& g, int& b) {
  int rr = 0, gg = 0, bb = 0;
  if (!parse_hex_rgb8(name, rr, gg, bb)) return false;
  r = static_cast<int>(std::round(rr * 1000.0 / 255.0));
  g = static_cast<int>(std::round(gg * 1000.0 / 255.0));
  b = static_cast<int>(std::round(bb * 1000.0 / 255.0));
  return true;
}

inline int ansi_color_id_for_name(const std::string& name) {
  static std::map<std::string,int> base = {
    {"black",COLOR_BLACK},{"red",COLOR_RED},{"green",COLOR_GREEN},{"yellow",COLOR_YELLOW},
    {"blue",COLOR_BLUE},{"magenta",COLOR_MAGENTA},{"cyan",COLOR_CYAN},{"white",COLOR_WHITE},
    {"gray",COLOR_WHITE}
  };
  auto it = base.find(to_lower(name));
  return (it!=base.end()) ? it->second : COLOR_WHITE;
}

inline int nearest_ansi_from_rgb(int r, int g, int b) {
  struct C { int id,R,G,B; } base[] = {
    {COLOR_BLACK,0,0,0},{COLOR_RED,1000,0,0},{COLOR_GREEN,0,1000,0},{COLOR_YELLOW,1000,1000,0},
    {COLOR_BLUE,0,0,1000},{COLOR_MAGENTA,1000,0,1000},{COLOR_CYAN,0,1000,1000},{COLOR_WHITE,1000,1000,1000}
  };
  int best = COLOR_WHITE, bestd = INT_MAX;
  for (auto &c : base) {
    int dr=r-c.R, dg=g-c.G, db=b-c.B;
    int d = dr*dr + dg*dg + db*db;
    if (d < bestd) { bestd = d; best = c.id; }
  }
  return best;
}

inline bool curses_truecolor_ok() {
  return has_colors() && can_change_color() && COLORS > 16;
}

inline short reserve_custom_color_id() {
  static bool inited=false;
  static std::vector<short> freelist;
  if (!inited) {
    inited = true;
    // Keep 0..7 reserved; start at 16 if available.
    short start = (COLORS > 16) ? 16 : (COLORS > 8 ? 8 : 0);
    for (short i=start; i < COLORS; ++i) freelist.push_back(i);
  }
  if (freelist.empty()) return -1;
  short id = freelist.back();
  freelist.pop_back();
  return id;
}

inline int alloc_true_color(const std::string& key, int r, int g, int b){
  if (!has_colors()) return COLOR_WHITE;
  if (!curses_truecolor_ok()) return nearest_ansi_from_rgb(r,g,b);
  auto it = color_map.find(key);
  if (it != color_map.end()) return it->second;

  short id = reserve_custom_color_id();
  if (id < 0) return nearest_ansi_from_rgb(r,g,b);  // graceful fallback

  // clamp and initialize
  init_color(id, std::clamp(r,0,1000), std::clamp(g,0,1000), std::clamp(b,0,1000));
  color_map[key] = id;
  return id;
}

inline int get_color(const std::string& color_name, int def_r, int def_g, int def_b) {
  if (!has_colors()) return COLOR_WHITE;

  // IMPORTANT: "<empty>" means "use terminal default color"
  if (is_unset_color_token(color_name)) return -1;

  int r=0,g=0,b=0;
  if (parse_hex_rgb(color_name, r,g,b)) return alloc_true_color(color_name, r,g,b);

  auto lname = to_lower(color_name);
  auto it = color_map.find(lname);
  if (it != color_map.end()) return it->second;

  if (can_change_color()) {
    struct C { const char* n; int r,g,b; } table[] = {
      {"black",0,0,0},{"white",1000,1000,1000},{"gray",600,600,600},
      {"red",1000,0,0},{"green",0,1000,0},{"blue",0,0,1000},
      {"yellow",1000,1000,0},{"magenta",1000,0,1000},{"cyan",0,1000,1000},
      {"orange",1000,647,0},{"purple",627,125,941},{"teal",0,502,502}
    };
    for (auto& c : table) if (lname==c.n) return alloc_true_color(lname, c.r,c.g,c.b);
    return alloc_true_color(lname, def_r,def_g,def_b);
  } else {
    return ansi_color_id_for_name(lname);
  }
}

inline int get_color_pair(const std::string& fg, const std::string& bg) {
  static std::map<std::pair<int,int>, int> cache;
  static int next_pair_id = 1;

  if (!has_colors()) return 0;

  int fg_id = get_color(fg);
  int bg_id = get_color(bg, 0,0,0);

  if (fg_id == -1 && bg_id == -1) return 0;

  auto key = std::make_pair(fg_id, bg_id);
  if (auto it = cache.find(key); it != cache.end()) return it->second;

  // If we run out of pairs, degrade safely to default instead of overwriting the last pair.
  if (next_pair_id >= COLOR_PAIRS) return 0;

  int pid = next_pair_id++;

  // init_pair can fail (notably if -1 used without use_default_colors support).
  if (init_pair(pid, (short)fg_id, (short)bg_id) == ERR) {
    // Fallback: force concrete defaults instead of leaving the pair undefined.
    int fg2 = (fg_id == -1) ? COLOR_WHITE : fg_id;
    int bg2 = (bg_id == -1) ? COLOR_BLACK : bg_id;
    (void)init_pair(pid, (short)fg2, (short)bg2);
    key = std::make_pair(fg2, bg2);
  }

  cache[key] = pid;
  return pid;
}

inline void set_global_background(const std::string& background_color) {
  if (!has_colors()) return;
  int bg_pair = get_color_pair("white", background_color);
  bkgd(' ' | COLOR_PAIR(bg_pair));
}

// Very simple word wrap (safe at EoS)
inline std::vector<std::string> split_lines_keep_empty(const std::string& s) {
  std::vector<std::string> out;
  // Keep behavior consistent: empty string => one empty line
  std::size_t i = 0;
  while (i <= s.size()) {
    std::size_t j = s.find('\n', i);
    if (j == std::string::npos) j = s.size();
    std::string line = s.substr(i, j - i);
    // tolerate CRLF inputs
    if (!line.empty() && line.back() == '\r') line.pop_back();
    out.push_back(std::move(line));
    if (j == s.size()) break;
    i = j + 1;
  }
  return out;
}

inline std::vector<std::string> wrap_text(const std::string& s, int width) {
  std::vector<std::string> out;
  if (width <= 0) { out.emplace_back(); return out; }
  // Multiline aware: split by '\n' first, wrap each physical line.
  auto phys = split_lines_keep_empty(s);
  for (const auto& line : phys) {
    // Preserve empty lines
    if (line.empty()) { out.emplace_back(); continue; }

    std::size_t i = 0;
    while (i < line.size()) {
      std::size_t end = std::min(i + (std::size_t)width, line.size());
      std::size_t brk = end;
      if (end < line.size()) {
        std::size_t j = line.rfind(' ', end);
        if (j != std::string::npos && j >= i) brk = j;
      }
      if (brk == i) brk = end;
      out.emplace_back(line.substr(i, brk - i));
      if (brk < line.size() && line[brk] == ' ') i = brk + 1;
      else i = brk;
    }
  }

  return out;
}

} // namespace iinuji
} // namespace cuwacunu
