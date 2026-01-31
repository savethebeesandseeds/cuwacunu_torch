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

namespace cuwacunu {
namespace iinuji {

inline std::map<std::string, int> color_map;

inline bool is_unset_color_token(const std::string& s) {
  return s.empty() || s == "<empty>";
}

inline std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

inline bool parse_hex_rgb(const std::string& name, int& r, int& g, int& b) {
  if (name.size()==7 && name[0]=='#') {
    auto hex = [&](int i){ return int(std::round(std::stoi(name.substr(i,2), nullptr, 16) * 1000.0 / 255.0)); };
    r=hex(1); g=hex(3); b=hex(5); return true;
  }
  return false;
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

inline int get_color(const std::string& color_name, int def_r=1000, int def_g=1000, int def_b=1000) {
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
inline std::vector<std::string> wrap_text(const std::string& s, int width) {
  std::vector<std::string> out;
  if (width <= 0) { out.emplace_back(); return out; }
  size_t i = 0;
  while (i < s.size()) {
    size_t end = std::min(i + (size_t)width, s.size());
    size_t brk = end;
    if (end < s.size()) {
      size_t j = s.rfind(' ', end);
      if (j != std::string::npos && j >= i) brk = j;
    }
    if (brk == i) brk = end;
    out.emplace_back(s.substr(i, brk - i));
    if (brk < s.size() && s[brk] == ' ') i = brk + 1;
    else i = brk;
  }
  return out;
}

} // namespace iinuji
} // namespace cuwacunu
