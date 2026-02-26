#pragma once

#include <algorithm>
#include <cstdint>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

namespace board_contract_dsl_key_highlight {
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY) inline constexpr const char ID[] = KEY;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
}  // namespace board_contract_dsl_key_highlight

enum class board_contract_section_kind_t : std::uint8_t {
  None = 0,
  Circuit,
  ObservationSources,
  ObservationChannels,
  JkimyeiSpecs,
  Other,
};

inline std::string lower_ascii_copy_board_editor(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool is_board_instruction_path(const std::string& path) {
  const std::string p = lower_ascii_copy_board_editor(path);
  return p.find("tsiemene_circuit.dsl") != std::string::npos ||
         p.find("board.dsl") != std::string::npos;
}

inline bool is_token_char_board_editor(char c) {
  const unsigned char uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) || c == '_' || c == '.' || c == '-';
}

inline std::string_view trim_ascii_view_board_editor(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
  return s;
}

inline bool starts_with_ascii_board_editor(std::string_view s, std::string_view p) {
  return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

inline std::string_view parse_segment_marker_key_board_editor(std::string_view line,
                                                              bool begin_marker) {
  line = trim_ascii_view_board_editor(line);
  const std::string_view token = begin_marker ? "BEGIN " : "END ";
  if (!starts_with_ascii_board_editor(line, token)) return {};
  line.remove_prefix(token.size());
  line = trim_ascii_view_board_editor(line);
  return line;
}

inline board_contract_section_kind_t section_kind_from_key_board_editor(std::string_view key) {
  if (key == board_contract_dsl_key_highlight::ContractCircuit) {
    return board_contract_section_kind_t::Circuit;
  }
  if (key == board_contract_dsl_key_highlight::ContractObservationSources) {
    return board_contract_section_kind_t::ObservationSources;
  }
  if (key == board_contract_dsl_key_highlight::ContractObservationChannels) {
    return board_contract_section_kind_t::ObservationChannels;
  }
  if (key == board_contract_dsl_key_highlight::ContractJkimyeiSpecs) {
    return board_contract_section_kind_t::JkimyeiSpecs;
  }
  if (key.empty()) return board_contract_section_kind_t::None;
  return board_contract_section_kind_t::Other;
}

inline board_contract_section_kind_t active_section_for_line_board_editor(
    const cuwacunu::iinuji::editorBox_data_t& ed,
    int line_index) {
  if (line_index < 0 || ed.lines.empty()) return board_contract_section_kind_t::None;
  const int cap = std::min(line_index, static_cast<int>(ed.lines.size()) - 1);
  board_contract_section_kind_t active = board_contract_section_kind_t::None;
  for (int i = 0; i <= cap; ++i) {
    const std::string_view line = ed.lines[static_cast<std::size_t>(i)];
    const std::string_view begin_key = parse_segment_marker_key_board_editor(line, true);
    if (!begin_key.empty()) {
      active = section_kind_from_key_board_editor(begin_key);
      continue;
    }
    const std::string_view end_key = parse_segment_marker_key_board_editor(line, false);
    if (end_key.empty()) continue;
    const board_contract_section_kind_t end_kind = section_kind_from_key_board_editor(end_key);
    if (end_kind == active) active = board_contract_section_kind_t::None;
  }
  return active;
}

inline board_contract_section_kind_t forced_section_kind_from_editor_path_board_editor(
    std::string_view path) {
  const std::string_view marker = "#section:";
  const std::size_t at = path.rfind(marker);
  if (at == std::string_view::npos) return board_contract_section_kind_t::None;
  std::string_view key = path.substr(at + marker.size());
  const std::size_t hash = key.find('#');
  if (hash != std::string_view::npos) key = key.substr(0, hash);
  return section_kind_from_key_board_editor(trim_ascii_view_board_editor(key));
}

inline void paint_span_board_editor(std::vector<short>& colors, int b, int e, short pair) {
  if (pair == 0) return;
  const int n = static_cast<int>(colors.size());
  if (n <= 0) return;
  b = std::clamp(b, 0, n);
  e = std::clamp(e, 0, n);
  if (b >= e) return;
  for (int i = b; i < e; ++i) colors[(std::size_t)i] = pair;
}

inline void paint_word_board_editor(const std::string& line,
                                    std::vector<short>& out_colors,
                                    std::string_view word,
                                    short pair) {
  if (word.empty() || pair == 0 || line.empty()) return;
  for (std::size_t pos = 0; pos + word.size() <= line.size();) {
    pos = line.find(word, pos);
    if (pos == std::string::npos) break;
    const bool left_ok = (pos == 0) || !is_token_char_board_editor(line[pos - 1]);
    const std::size_t end = pos + word.size();
    const bool right_ok = (end >= line.size()) || !is_token_char_board_editor(line[end]);
    if (left_ok && right_ok) {
      paint_span_board_editor(out_colors, static_cast<int>(pos), static_cast<int>(end), pair);
    }
    pos = end;
  }
}

inline void paint_numeric_literals_board_editor(const std::string& line,
                                                std::vector<short>& out_colors,
                                                short pair) {
  if (pair == 0) return;
  for (std::size_t i = 0; i < line.size();) {
    if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
      ++i;
      continue;
    }
    std::size_t b = i;
    std::size_t e = i + 1;
    while (e < line.size()) {
      const char c = line[e];
      if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
        ++e;
        continue;
      }
      break;
    }
    paint_span_board_editor(out_colors, static_cast<int>(b), static_cast<int>(e), pair);
    i = e;
  }
}

inline void paint_key_value_pairs_board_editor(const std::string& line,
                                               std::vector<short>& out_colors,
                                               short key_pair,
                                               short value_pair) {
  if (line.empty()) return;
  for (std::size_t eq = 0; eq < line.size();) {
    eq = line.find('=', eq);
    if (eq == std::string::npos) break;

    std::size_t kb = eq;
    while (kb > 0) {
      const char c = line[kb - 1];
      if (is_token_char_board_editor(c)) {
        --kb;
        continue;
      }
      break;
    }
    if (kb < eq) {
      paint_span_board_editor(
          out_colors,
          static_cast<int>(kb),
          static_cast<int>(eq),
          key_pair);
    }

    std::size_t vb = eq + 1;
    while (vb < line.size() && std::isspace(static_cast<unsigned char>(line[vb]))) ++vb;
    std::size_t ve = vb;
    while (ve < line.size()) {
      const char c = line[ve];
      if (c == ',' || c == '|') break;
      ++ve;
    }
    while (ve > vb && std::isspace(static_cast<unsigned char>(line[ve - 1]))) --ve;
    if (vb < ve) {
      paint_span_board_editor(
          out_colors,
          static_cast<int>(vb),
          static_cast<int>(ve),
          value_pair);
    }

    eq = eq + 1;
  }
}

inline void colorize_circuit_dsl_line_board_editor(const std::string& line,
                                                   std::vector<short>& out_colors,
                                                   short var_pair,
                                                   short comp_pair,
                                                   short dir_pair,
                                                   short kind_pair) {
  auto paint = [&](int b, int e, short pair) {
    paint_span_board_editor(out_colors, b, e, pair);
  };

  if (const std::size_t eq = line.find('='); eq != std::string::npos) {
    std::size_t b = 0;
    while (b < eq && std::isspace(static_cast<unsigned char>(line[b]))) ++b;
    std::size_t e = b;
    while (e < eq && is_token_char_board_editor(line[e])) ++e;
    if (e > b && !(e - b >= 4 && line.compare(b, 4, "tsi.") == 0)) {
      paint(static_cast<int>(b), static_cast<int>(e), var_pair);
    }
  }

  if (const std::size_t arrow = line.find("->"); arrow != std::string::npos) {
    auto mark_hop_alias = [&](std::size_t s0, std::size_t s1) {
      std::size_t b = s0;
      while (b < s1 && std::isspace(static_cast<unsigned char>(line[b]))) ++b;
      std::size_t e = b;
      while (e < s1 && is_token_char_board_editor(line[e])) ++e;
      if (e > b && !(e - b >= 4 && line.compare(b, 4, "tsi.") == 0)) {
        paint(static_cast<int>(b), static_cast<int>(e), var_pair);
      }
    };
    mark_hop_alias(0, arrow);
    mark_hop_alias(arrow + 2, line.size());
  }

  for (std::size_t pos = 0; pos < line.size();) {
    pos = line.find("tsi.", pos);
    if (pos == std::string::npos) break;
    const bool boundary_left = (pos == 0) || !is_token_char_board_editor(line[pos - 1]);
    if (!boundary_left) {
      ++pos;
      continue;
    }
    std::size_t end = pos + 4;
    while (end < line.size() && is_token_char_board_editor(line[end])) ++end;
    paint(static_cast<int>(pos), static_cast<int>(end), comp_pair);
    pos = end;
  }

  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] != '@') continue;
    std::size_t j = i + 1;
    while (j < line.size() && is_token_char_board_editor(line[j])) ++j;
    if (j > i + 1) paint(static_cast<int>(i), static_cast<int>(j), dir_pair);
  }

  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] != ':') continue;
    std::size_t j = i + 1;
    while (j < line.size() && is_token_char_board_editor(line[j])) ++j;
    if (j > i + 1) paint(static_cast<int>(i), static_cast<int>(j), kind_pair);
  }
}

inline void colorize_tabular_dsl_line_board_editor(const std::string& line,
                                                   std::vector<short>& out_colors,
                                                   short section_pair,
                                                   short kw_pair,
                                                   short var_pair,
                                                   short kind_pair,
                                                   short bool_true_pair,
                                                   short bool_false_pair,
                                                   short path_pair) {
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '|' || c == '/' || c == '\\') {
      paint_span_board_editor(out_colors, static_cast<int>(i), static_cast<int>(i + 1), section_pair);
    }
  }

  const std::string_view trimmed = trim_ascii_view_board_editor(line);
  if (starts_with_ascii_board_editor(trimmed, "[") && trimmed.find(']') != std::string_view::npos) {
    const std::size_t b = line.find('[');
    const std::size_t e = line.find(']');
    if (b != std::string::npos && e != std::string::npos && e > b) {
      paint_span_board_editor(out_colors, static_cast<int>(b), static_cast<int>(e + 1), kw_pair);
    }
  }

  paint_word_board_editor(line, out_colors, "true", bool_true_pair);
  paint_word_board_editor(line, out_colors, "false", bool_false_pair);
  paint_key_value_pairs_board_editor(line, out_colors, var_pair, kind_pair);
  paint_numeric_literals_board_editor(line, out_colors, kind_pair);

  for (std::size_t pos = 0; pos < line.size();) {
    pos = line.find('/', pos);
    if (pos == std::string::npos) break;
    std::size_t end = pos + 1;
    while (end < line.size()) {
      const char c = line[end];
      if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ',' || c == ')') break;
      ++end;
    }
    if (end > pos + 2) {
      paint_span_board_editor(out_colors, static_cast<int>(pos), static_cast<int>(end), path_pair);
    }
    pos = end;
  }
}

inline void board_instruction_colorize_line(const cuwacunu::iinuji::editorBox_data_t& ed,
                                            int line_index,
                                            const std::string& line,
                                            std::vector<short>& out_colors,
                                            short base_pair,
                                            const std::string& bg_color) {
  out_colors.assign(line.size(), base_pair);
  if (line.empty()) return;

  short var_pair = static_cast<short>(get_color_pair("#89B4FA", bg_color));
  short comp_pair = static_cast<short>(get_color_pair("#E3C779", bg_color));
  short dir_pair = static_cast<short>(get_color_pair("#C994F3", bg_color));
  short kind_pair = static_cast<short>(get_color_pair("#7FD4C6", bg_color));
  short kw_pair = static_cast<short>(get_color_pair("#A8D8A0", bg_color));
  short section_pair = static_cast<short>(get_color_pair("#F2B880", bg_color));
  short comment_pair = static_cast<short>(get_color_pair("#8C95A6", bg_color));
  short bool_true_pair = static_cast<short>(get_color_pair("#76C893", bg_color));
  short bool_false_pair = static_cast<short>(get_color_pair("#E57A7A", bg_color));
  short path_pair = static_cast<short>(get_color_pair("#C9A66B", bg_color));
  if (var_pair == 0) var_pair = base_pair;
  if (comp_pair == 0) comp_pair = base_pair;
  if (dir_pair == 0) dir_pair = base_pair;
  if (kind_pair == 0) kind_pair = base_pair;
  if (kw_pair == 0) kw_pair = base_pair;
  if (section_pair == 0) section_pair = base_pair;
  if (comment_pair == 0) comment_pair = base_pair;
  if (bool_true_pair == 0) bool_true_pair = base_pair;
  if (bool_false_pair == 0) bool_false_pair = base_pair;
  if (path_pair == 0) path_pair = base_pair;

  auto paint = [&](int b, int e, short pair) {
    paint_span_board_editor(out_colors, b, e, pair);
  };

  std::size_t non_ws = 0;
  while (non_ws < line.size() && std::isspace(static_cast<unsigned char>(line[non_ws]))) ++non_ws;
  if (non_ws >= line.size()) return;
  if (line[non_ws] == '#') {
    paint(static_cast<int>(non_ws), static_cast<int>(line.size()), comment_pair);
    return;
  }
  const std::string_view trimmed = trim_ascii_view_board_editor(line);
  if (starts_with_ascii_board_editor(trimmed, "/*") ||
      starts_with_ascii_board_editor(trimmed, "*/") ||
      starts_with_ascii_board_editor(trimmed, "*")) {
    paint(static_cast<int>(non_ws), static_cast<int>(line.size()), comment_pair);
    return;
  }
  if (line.compare(non_ws, 6, "BEGIN ") == 0 || line.compare(non_ws, 4, "END ") == 0) {
    const bool is_begin = (line.compare(non_ws, 6, "BEGIN ") == 0);
    const std::size_t kw_len = is_begin ? 5u : 3u;
    const std::size_t key_start = non_ws + kw_len + 1u;
    paint(static_cast<int>(non_ws), static_cast<int>(non_ws + kw_len), kw_pair);
    if (key_start < line.size()) {
      paint(static_cast<int>(key_start), static_cast<int>(line.size()), section_pair);
    }
    return;
  }

  board_contract_section_kind_t section_kind = active_section_for_line_board_editor(ed, line_index);
  if (section_kind == board_contract_section_kind_t::None) {
    section_kind = forced_section_kind_from_editor_path_board_editor(ed.path);
  }
  if (section_kind == board_contract_section_kind_t::Circuit) {
    colorize_circuit_dsl_line_board_editor(
        line,
        out_colors,
        var_pair,
        comp_pair,
        dir_pair,
        kind_pair);
    return;
  }

  colorize_tabular_dsl_line_board_editor(
      line,
      out_colors,
      section_pair,
      kw_pair,
      var_pair,
      kind_pair,
      bool_true_pair,
      bool_false_pair,
      path_pair);
}

inline void configure_board_editor_highlighting(cuwacunu::iinuji::editorBox_data_t& ed) {
  if (!is_board_instruction_path(ed.path)) {
    ed.line_colorizer = {};
    return;
  }
  ed.line_colorizer =
      [](const cuwacunu::iinuji::editorBox_data_t& editor,
         int line_index,
         const std::string& line,
         std::vector<short>& out_colors,
         short base_pair,
         const std::string& bg_color) {
        board_instruction_colorize_line(editor, line_index, line, out_colors, base_pair, bg_color);
      };
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
