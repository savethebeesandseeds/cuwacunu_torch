/* iinuji_color.h */
#pragma once

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_utils.h"

namespace cuwacunu {
namespace iinuji {

inline std::map<std::string, int> color_map;

inline int get_color(const std::string &color_name, int def_r = 1000,
                     int def_g = 1000, int def_b = 1000);

inline bool rgb8_from_color_id(int id, int &r, int &g, int &b) {
  if (!has_colors())
    return false;
  if (id < 0 || id >= COLORS)
    return false;
  short rr = 0;
  short gg = 0;
  short bb = 0;
  if (color_content(static_cast<short>(id), &rr, &gg, &bb) == ERR)
    return false;
  r = static_cast<int>(std::lround(rr * 255.0 / 1000.0));
  g = static_cast<int>(std::lround(gg * 255.0 / 1000.0));
  b = static_cast<int>(std::lround(bb * 255.0 / 1000.0));
  return true;
}

inline bool rgb8_for_token(const std::string &tok, int &r, int &g, int &b) {
  if (parse_hex_rgb8(tok, r, g, b))
    return true;
  if (is_unset_color_token(tok))
    return false;
  if (!has_colors())
    return false;
  int id = get_color(tok);
  return rgb8_from_color_id(id, r, g, b);
}

inline std::string darken_color_token(const std::string &tok, double factor) {
  int r = 0;
  int g = 0;
  int b = 0;
  if (!rgb8_for_token(tok, r, g, b))
    return tok;
  r = clamp255(static_cast<int>(std::lround(r * factor)));
  g = clamp255(static_cast<int>(std::lround(g * factor)));
  b = clamp255(static_cast<int>(std::lround(b * factor)));
  return rgb8_to_hex(r, g, b);
}

inline std::string focus_darken_bg_token(const std::string &bg,
                                         double factor = 0.8) {
  if (is_unset_color_token(bg))
    return "#000000";
  return darken_color_token(bg, factor);
}

inline std::string focus_darken_fg_token(const std::string &fg,
                                         double factor = 0.8) {
  if (is_unset_color_token(fg))
    return "#505050";
  return darken_color_token(fg, factor);
}

inline bool parse_hex_rgb(const std::string &name, int &r, int &g, int &b) {
  int rr = 0;
  int gg = 0;
  int bb = 0;
  if (!parse_hex_rgb8(name, rr, gg, bb))
    return false;
  r = static_cast<int>(std::round(rr * 1000.0 / 255.0));
  g = static_cast<int>(std::round(gg * 1000.0 / 255.0));
  b = static_cast<int>(std::round(bb * 1000.0 / 255.0));
  return true;
}

inline int ansi_color_id_for_name(const std::string &name) {
  static std::map<std::string, int> base = {
      {"black", COLOR_BLACK}, {"red", COLOR_RED},
      {"green", COLOR_GREEN}, {"yellow", COLOR_YELLOW},
      {"blue", COLOR_BLUE},   {"magenta", COLOR_MAGENTA},
      {"cyan", COLOR_CYAN},   {"white", COLOR_WHITE},
      {"gray", COLOR_WHITE},
  };
  auto it = base.find(to_lower(name));
  return (it != base.end()) ? it->second : COLOR_WHITE;
}

inline int nearest_ansi_from_rgb(int r, int g, int b) {
  struct C {
    int id;
    int R;
    int G;
    int B;
  };
  C base[] = {
      {COLOR_BLACK, 0, 0, 0},      {COLOR_RED, 1000, 0, 0},
      {COLOR_GREEN, 0, 1000, 0},   {COLOR_YELLOW, 1000, 1000, 0},
      {COLOR_BLUE, 0, 0, 1000},    {COLOR_MAGENTA, 1000, 0, 1000},
      {COLOR_CYAN, 0, 1000, 1000}, {COLOR_WHITE, 1000, 1000, 1000},
  };
  int best = COLOR_WHITE;
  int bestd = INT_MAX;
  for (auto &c : base) {
    int dr = r - c.R;
    int dg = g - c.G;
    int db = b - c.B;
    int d = dr * dr + dg * dg + db * db;
    if (d < bestd) {
      bestd = d;
      best = c.id;
    }
  }
  return best;
}

inline bool curses_truecolor_ok() {
  return has_colors() && can_change_color() && COLORS > 16;
}

inline short reserve_custom_color_id() {
  static bool inited = false;
  static std::vector<short> freelist;
  if (!inited) {
    inited = true;
    const short start = (COLORS > 16) ? 16 : (COLORS > 8 ? 8 : 0);
    for (short i = start; i < COLORS; ++i)
      freelist.push_back(i);
  }
  if (freelist.empty())
    return -1;
  short id = freelist.back();
  freelist.pop_back();
  return id;
}

inline int alloc_true_color(const std::string &key, int r, int g, int b) {
  if (!has_colors())
    return COLOR_WHITE;
  if (!curses_truecolor_ok())
    return nearest_ansi_from_rgb(r, g, b);
  auto it = color_map.find(key);
  if (it != color_map.end())
    return it->second;

  short id = reserve_custom_color_id();
  if (id < 0)
    return nearest_ansi_from_rgb(r, g, b);

  init_color(id, std::clamp(r, 0, 1000), std::clamp(g, 0, 1000),
             std::clamp(b, 0, 1000));
  color_map[key] = id;
  return id;
}

inline int get_color(const std::string &color_name, int def_r, int def_g,
                     int def_b) {
  if (!has_colors())
    return COLOR_WHITE;

  if (is_unset_color_token(color_name))
    return -1;

  int r = 0;
  int g = 0;
  int b = 0;
  if (parse_hex_rgb(color_name, r, g, b))
    return alloc_true_color(color_name, r, g, b);

  auto lname = to_lower(color_name);
  auto it = color_map.find(lname);
  if (it != color_map.end())
    return it->second;

  if (can_change_color()) {
    struct C {
      const char *n;
      int r;
      int g;
      int b;
    };
    C table[] = {
        {"black", 0, 0, 0},        {"white", 1000, 1000, 1000},
        {"gray", 600, 600, 600},   {"red", 1000, 0, 0},
        {"green", 0, 1000, 0},     {"blue", 0, 0, 1000},
        {"yellow", 1000, 1000, 0}, {"magenta", 1000, 0, 1000},
        {"cyan", 0, 1000, 1000},   {"orange", 1000, 647, 0},
        {"purple", 627, 125, 941}, {"teal", 0, 502, 502},
    };
    for (auto &c : table) {
      if (lname == c.n)
        return alloc_true_color(lname, c.r, c.g, c.b);
    }
    return alloc_true_color(lname, def_r, def_g, def_b);
  }

  return ansi_color_id_for_name(lname);
}

inline int get_color_pair(const std::string &fg, const std::string &bg) {
  static std::map<std::pair<int, int>, int> cache;
  static int next_pair_id = 1;

  if (!has_colors())
    return 0;

  int fg_id = get_color(fg);
  int bg_id = get_color(bg, 0, 0, 0);

  if (fg_id == -1 && bg_id == -1)
    return 0;

  auto key = std::make_pair(fg_id, bg_id);
  const auto original_key = key;
  if (auto it = cache.find(key); it != cache.end())
    return it->second;

  auto fallback_pair = [&](int preferred_fg, int preferred_bg) -> int {
    for (const auto &entry : cache) {
      if (entry.first.second == preferred_bg)
        return entry.second;
    }
    for (const auto &entry : cache) {
      if (entry.first.first == preferred_fg)
        return entry.second;
    }
    if (!cache.empty())
      return cache.begin()->second;
    return 0;
  };

  if (next_pair_id >= COLOR_PAIRS) {
    const int fallback = fallback_pair(fg_id, bg_id);
    if (fallback != 0)
      cache[original_key] = fallback;
    return fallback;
  }

  int pid = next_pair_id++;

  if (init_pair(pid, static_cast<short>(fg_id), static_cast<short>(bg_id)) ==
      ERR) {
    int fg2 = (fg_id == -1) ? COLOR_WHITE : fg_id;
    int bg2 = (bg_id == -1) ? COLOR_BLACK : bg_id;
    if (init_pair(pid, static_cast<short>(fg2), static_cast<short>(bg2)) ==
        ERR) {
      const int fallback = fallback_pair(fg2, bg2);
      if (fallback != 0) {
        cache[original_key] = fallback;
        cache[std::make_pair(fg2, bg2)] = fallback;
      }
      return fallback;
    }
    key = std::make_pair(fg2, bg2);
  }

  cache[original_key] = pid;
  cache[key] = pid;
  return pid;
}

inline void set_global_background(const std::string &background_color) {
  if (!has_colors())
    return;
  int bg_pair = get_color_pair("white", background_color);
  bkgd(' ' | COLOR_PAIR(bg_pair));
}

} // namespace iinuji
} // namespace cuwacunu
