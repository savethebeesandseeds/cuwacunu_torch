#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"

namespace cuwacunu {
namespace iinuji {

inline bool is_unset_token(const std::string& s) { return s.empty() || s == "<empty>"; }

inline std::string sanitize_id(std::string s) {
  if (s.empty()) return "unnamed";
  for (char& c : s) {
    if (!(std::isalnum((unsigned char)c) || c=='_' || c=='-' || c=='.')) c = '_';
  }
  return s;
}

inline std::string join_path(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
  return a + "." + b;
}

/* Match your BNF:
   <name_ident> ::= <alpha> { <alpha> | <digit> | "_" | "-" | "." }
*/
inline bool is_ident(const std::string& s) {
  if (s.empty()) return false;

  auto is_first = [](unsigned char c){
    return std::isalpha(c);
  };
  auto is_tail = [](unsigned char c){
    return std::isalnum(c) || c=='_' || c=='-' || c=='.';
  };

  if (!is_first((unsigned char)s[0])) return false;
  for (size_t i = 1; i < s.size(); ++i) {
    if (!is_tail((unsigned char)s[i])) return false;
  }
  return true;
}

inline bool is_hex_color(const std::string& s) {
  if (s.size() != 7 || s[0] != '#') return false;
  auto hex = [](unsigned char c){
    return std::isdigit(c) || (c>='a'&&c<='f') || (c>='A'&&c<='F');
  };
  for (size_t i=1;i<7;++i) if (!hex((unsigned char)s[i])) return false;
  return true;
}

inline bool is_named_color_token(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (!(std::isalnum((unsigned char)c) || c=='_' || c=='-')) return false;
  }
  return true;
}

inline bool is_valid_color_token(const std::string& s) {
  if (is_unset_token(s)) return true;
  if (is_hex_color(s)) return true;
  return is_named_color_token(s);
}

inline plot_mode_t parse_plot_mode(const std::string& type_raw) {
  const std::string t = to_lower(type_raw);
  if (t == "scatter") return plot_mode_t::Scatter;
  if (t == "stairs")  return plot_mode_t::Stairs;
  if (t == "stem")    return plot_mode_t::Stem;
  return plot_mode_t::Line;
}

inline bool is_valid_plot_type(const std::string& type_raw) {
  if (is_unset_token(type_raw)) return true;
  const std::string t = to_lower(type_raw);
  return (t=="line" || t=="scatter" || t=="stairs" || t=="stem");
}

inline std::string pick_color(const std::string& fig,
                              const std::string& pan,
                              const std::string& scr,
                              const std::string& fallback) {
  if (!is_unset_token(fig)) return fig;
  if (!is_unset_token(pan)) return pan;
  if (!is_unset_token(scr)) return scr;
  return fallback;
}

inline std::string mk_panel_id(const std::string& screen_name, int panel_index) {
  std::ostringstream oss;
  oss << sanitize_id(screen_name) << ".panel" << panel_index;
  return oss.str();
}

inline std::string mk_figure_id(const std::string& screen_name, int panel_index, int figure_index,
                                const std::string& kind_raw) {
  std::ostringstream oss;
  oss << sanitize_id(screen_name) << ".panel" << panel_index << ".fig" << figure_index;
  if (!is_unset_token(kind_raw)) oss << "." << sanitize_id(kind_raw);
  return oss.str();
}

} // namespace iinuji
} // namespace cuwacunu
