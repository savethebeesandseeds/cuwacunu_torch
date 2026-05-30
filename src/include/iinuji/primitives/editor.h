#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <functional>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iinuji/iinuji_color.h"
#include "iinuji/iinuji_keys.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

struct editor_box_data_t : public iinuji_data_t {
  struct history_frame_t {
    std::vector<std::string> lines{};
    int cursor_line{0};
    int cursor_col{0};
    int preferred_col{-1};
    int top_line{0};
    int left_col{0};
    bool dirty{false};
  };

  enum class history_merge_kind_t {
    None = 0,
    InsertChar,
    BackspaceChar,
    DeleteChar,
  };

  using line_colorizer_t = std::function<void(
      const editor_box_data_t &editor, int line_index, const std::string &line,
      std::vector<short> &out_colors, short base_pair,
      const std::string &background_color)>;

  std::string path;
  std::vector<std::string> lines;
  bool dirty{false};
  bool read_only{false};
  bool close_armed{false};
  bool close_armed_via_escape{false};

  int cursor_line{0};
  int cursor_col{0};
  int preferred_col{-1};
  int top_line{0};
  int left_col{0};

  int last_body_h{0};
  int last_lineno_w{0};
  int last_text_w{0};

  int tab_width{2};
  bool show_header{true};
  bool show_footer{true};
  bool show_line_numbers{true};
  bool show_tabs{true};
  bool highlight_current_line{true};
  bool highlight_matching_delimiter{true};
  std::string status;
  line_colorizer_t line_colorizer{};
  bool history_enabled{true};
  std::size_t history_limit{128};
  std::vector<history_frame_t> undo_stack{};
  std::vector<history_frame_t> redo_stack{};
  history_merge_kind_t history_merge_kind{history_merge_kind_t::None};
  int history_merge_cursor_line{-1};
  int history_merge_cursor_col{-1};
  int history_merge_line_count{0};

  explicit editor_box_data_t(std::string p = "") : path(std::move(p)) {
    lines.emplace_back();
  }

  void ensure_nonempty() {
    if (lines.empty())
      lines.emplace_back();
    cursor_line =
        std::clamp(cursor_line, 0, static_cast<int>(lines.size()) - 1);
    cursor_col = std::clamp(
        cursor_col, 0,
        static_cast<int>(lines[static_cast<std::size_t>(cursor_line)].size()));
    if (top_line < 0)
      top_line = 0;
    if (left_col < 0)
      left_col = 0;
  }
};

using editorBox_data_t = editor_box_data_t;

struct editor_box_opts_t {
  std::string path{};
  std::string text{};
  bool read_only{false};
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::Input};
};

namespace primitives {

enum class editor_syntax_kind_t {
  Plain = 0,
  Assignment,
  Bnf,
  Markdown,
};

inline std::string editor_syntax_lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return out;
}

inline editor_syntax_kind_t editor_syntax_kind_for_path(std::string_view path) {
  const std::string lower = editor_syntax_lower(path);
  if (lower.ends_with(".bnf"))
    return editor_syntax_kind_t::Bnf;
  if (lower.ends_with(".md"))
    return editor_syntax_kind_t::Markdown;
  if (lower.ends_with(".dsl") || lower.ends_with(".man") ||
      lower.ends_with(".jkimyei") || lower.ends_with(".net") ||
      lower.ends_with(".manifest") || lower.ends_with(".state") ||
      lower.ends_with(".config") || lower.ends_with("/.config") ||
      lower == ".config") {
    return editor_syntax_kind_t::Assignment;
  }
  return editor_syntax_kind_t::Plain;
}

inline short editor_syntax_pair(std::string_view fg, std::string_view bg,
                                short fallback) {
  const short pair =
      static_cast<short>(get_color_pair(std::string(fg), std::string(bg)));
  return pair == 0 ? fallback : pair;
}

inline void editor_syntax_apply(std::vector<short> &colors, std::size_t begin,
                                std::size_t end, short pair) {
  if (begin >= colors.size())
    return;
  end = std::min(end, colors.size());
  for (std::size_t i = begin; i < end; ++i)
    colors[i] = pair;
}

inline bool editor_syntax_ident_char(unsigned char ch) {
  return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.';
}

inline std::size_t editor_syntax_first_nonspace(std::string_view line,
                                                std::size_t end) {
  std::size_t i = 0;
  end = std::min(end, line.size());
  while (i < end && std::isspace(static_cast<unsigned char>(line[i])))
    ++i;
  return i;
}

inline std::size_t editor_syntax_find_comment(std::string_view line) {
  bool single_quote = false;
  bool double_quote = false;
  bool escaped = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if ((single_quote || double_quote) && ch == '\\') {
      escaped = true;
      continue;
    }
    if (!single_quote && ch == '"') {
      double_quote = !double_quote;
      continue;
    }
    if (!double_quote && ch == '\'') {
      single_quote = !single_quote;
      continue;
    }
    if (single_quote || double_quote)
      continue;
    if (ch == '#')
      return i;
    if (ch == '/' && i + 1u < line.size() && line[i + 1u] == '/' &&
        (i == 0u || std::isspace(static_cast<unsigned char>(line[i - 1u])))) {
      return i;
    }
  }
  return std::string_view::npos;
}

inline std::size_t editor_syntax_find_unquoted(std::string_view line,
                                               char wanted, std::size_t end) {
  bool single_quote = false;
  bool double_quote = false;
  bool escaped = false;
  end = std::min(end, line.size());
  for (std::size_t i = 0; i < end; ++i) {
    const char ch = line[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if ((single_quote || double_quote) && ch == '\\') {
      escaped = true;
      continue;
    }
    if (!single_quote && ch == '"') {
      double_quote = !double_quote;
      continue;
    }
    if (!double_quote && ch == '\'') {
      single_quote = !single_quote;
      continue;
    }
    if (!single_quote && !double_quote && ch == wanted)
      return i;
  }
  return std::string_view::npos;
}

inline bool editor_syntax_number_token(std::string_view token) {
  if (token.empty())
    return false;
  bool digit = false;
  std::size_t i = (token.front() == '-' || token.front() == '+') ? 1u : 0u;
  for (; i < token.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(token[i]);
    if (std::isdigit(ch)) {
      digit = true;
      continue;
    }
    if (token[i] == '.' || token[i] == '_')
      continue;
    return false;
  }
  return digit;
}

inline void editor_syntax_color_strings(std::string_view line, std::size_t end,
                                        std::vector<short> &colors,
                                        short pair) {
  end = std::min(end, line.size());
  for (std::size_t i = 0; i < end; ++i) {
    const char quote = line[i];
    if (quote != '"' && quote != '\'')
      continue;
    const std::size_t begin = i++;
    bool escaped = false;
    for (; i < end; ++i) {
      const char ch = line[i];
      if (escaped) {
        escaped = false;
        continue;
      }
      if (ch == '\\') {
        escaped = true;
        continue;
      }
      if (ch == quote) {
        ++i;
        break;
      }
    }
    editor_syntax_apply(colors, begin, i, pair);
    if (i > 0)
      --i;
  }
}

inline void editor_syntax_color_value_tokens(std::string_view line,
                                             std::size_t begin, std::size_t end,
                                             std::vector<short> &colors,
                                             short value_pair, short bool_pair,
                                             short number_pair,
                                             short path_pair) {
  std::size_t i = begin;
  while (i < end) {
    while (i < end && (std::isspace(static_cast<unsigned char>(line[i])) ||
                       line[i] == ','))
      ++i;
    const std::size_t token_begin = i;
    while (i < end && !std::isspace(static_cast<unsigned char>(line[i])) &&
           line[i] != ',')
      ++i;
    if (token_begin == i)
      continue;
    std::string token(line.substr(token_begin, i - token_begin));
    while (!token.empty() &&
           (token.back() == ';' || token.back() == ',' || token.back() == ')'))
      token.pop_back();
    const std::string lower = editor_syntax_lower(token);
    short pair = value_pair;
    if (lower == "true" || lower == "false" || lower == "yes" ||
        lower == "no") {
      pair = bool_pair;
    } else if (editor_syntax_number_token(token)) {
      pair = number_pair;
    } else if (token.find('/') != std::string::npos ||
               token.starts_with("./") || token.starts_with("../")) {
      pair = path_pair;
    }
    editor_syntax_apply(colors, token_begin, token_begin + token.size(), pair);
  }
}

inline void editor_colorize_assignment_line(const editor_box_data_t &, int,
                                            const std::string &line,
                                            std::vector<short> &colors,
                                            short base_pair,
                                            const std::string &bg) {
  const short key_pair = editor_syntax_pair("#8BD5CA", bg, base_pair);
  const short type_pair = editor_syntax_pair("#88C0D0", bg, base_pair);
  const short value_pair = editor_syntax_pair("#D8DEE9", bg, base_pair);
  const short string_pair = editor_syntax_pair("#A3BE8C", bg, base_pair);
  const short number_pair = editor_syntax_pair("#D08770", bg, base_pair);
  const short bool_pair = editor_syntax_pair("#B48EAD", bg, base_pair);
  const short op_pair = editor_syntax_pair("#E3C779", bg, base_pair);
  const short comment_pair = editor_syntax_pair("#6D7890", bg, base_pair);
  const short section_pair = editor_syntax_pair("#F2B880", bg, base_pair);

  const std::size_t comment = editor_syntax_find_comment(line);
  const std::size_t code_end =
      comment == std::string_view::npos ? line.size() : comment;
  const std::size_t first = editor_syntax_first_nonspace(line, code_end);

  if (first < code_end && line[first] == '[') {
    const std::size_t close = line.find(']', first + 1u);
    if (close != std::string::npos && close < code_end)
      editor_syntax_apply(colors, first, close + 1u, section_pair);
  }

  const std::size_t eq = editor_syntax_find_unquoted(line, '=', code_end);
  const std::size_t lhs_end = eq == std::string_view::npos ? code_end : eq;
  std::size_t key_end = lhs_end;
  while (key_end > first &&
         std::isspace(static_cast<unsigned char>(line[key_end - 1u])))
    --key_end;
  const std::size_t colon = editor_syntax_find_unquoted(line, ':', lhs_end);
  if (colon != std::string_view::npos && colon >= first) {
    key_end = colon;
    editor_syntax_apply(colors, colon, lhs_end, type_pair);
  }
  if (first < key_end)
    editor_syntax_apply(colors, first, key_end, key_pair);

  for (std::size_t i = first; i < code_end; ++i) {
    const char ch = line[i];
    if (ch == '=' || ch == ':' || ch == ',' || ch == '[' || ch == ']' ||
        ch == '|') {
      colors[i] = op_pair;
    }
  }

  if (eq != std::string_view::npos && eq + 1u < code_end) {
    editor_syntax_color_value_tokens(line, eq + 1u, code_end, colors,
                                     value_pair, bool_pair, number_pair,
                                     string_pair);
  }
  editor_syntax_color_strings(line, code_end, colors, string_pair);
  if (comment != std::string_view::npos)
    editor_syntax_apply(colors, comment, line.size(), comment_pair);
}

inline void editor_colorize_bnf_line(const editor_box_data_t &, int,
                                     const std::string &line,
                                     std::vector<short> &colors,
                                     short base_pair, const std::string &bg) {
  const short nonterminal_pair = editor_syntax_pair("#8BD5CA", bg, base_pair);
  const short terminal_pair = editor_syntax_pair("#A3BE8C", bg, base_pair);
  const short op_pair = editor_syntax_pair("#E3C779", bg, base_pair);
  const short comment_pair = editor_syntax_pair("#6D7890", bg, base_pair);
  const std::size_t comment = editor_syntax_find_comment(line);
  const std::size_t code_end =
      comment == std::string_view::npos ? line.size() : comment;

  for (std::size_t i = 0; i < code_end; ++i) {
    if (line[i] == '<') {
      const std::size_t close = line.find('>', i + 1u);
      if (close != std::string::npos && close < code_end) {
        editor_syntax_apply(colors, i, close + 1u, nonterminal_pair);
        i = close;
        continue;
      }
    }
    if (line[i] == ':' || line[i] == '=' || line[i] == '|' || line[i] == '[' ||
        line[i] == ']' || line[i] == '(' || line[i] == ')' || line[i] == '*' ||
        line[i] == '+' || line[i] == '?') {
      colors[i] = op_pair;
    }
  }
  editor_syntax_color_strings(line, code_end, colors, terminal_pair);
  if (comment != std::string_view::npos)
    editor_syntax_apply(colors, comment, line.size(), comment_pair);
}

inline void editor_colorize_markdown_line(
    const editor_box_data_t &editor, int line_index, const std::string &line,
    std::vector<short> &colors, short base_pair, const std::string &bg) {
  const short heading_pair = editor_syntax_pair("#F2B880", bg, base_pair);
  const short op_pair = editor_syntax_pair("#E3C779", bg, base_pair);
  const std::size_t first = editor_syntax_first_nonspace(line, line.size());
  if (first < line.size() && line[first] == '#') {
    editor_syntax_apply(colors, first, line.size(), heading_pair);
    return;
  }
  if (first + 2u < line.size() && line.substr(first, 3) == "```") {
    editor_syntax_apply(colors, first, line.size(), op_pair);
    return;
  }
  if (first + 1u < line.size() && (line[first] == '-' || line[first] == '*') &&
      std::isspace(static_cast<unsigned char>(line[first + 1u]))) {
    colors[first] = op_pair;
  }
  if (editor_syntax_find_unquoted(line, '=', line.size()) !=
      std::string_view::npos) {
    editor_colorize_assignment_line(editor, line_index, line, colors, base_pair,
                                    bg);
  }
}

inline editor_box_data_t::line_colorizer_t
editor_make_syntax_colorizer(editor_syntax_kind_t kind) {
  switch (kind) {
  case editor_syntax_kind_t::Assignment:
    return editor_colorize_assignment_line;
  case editor_syntax_kind_t::Bnf:
    return editor_colorize_bnf_line;
  case editor_syntax_kind_t::Markdown:
    return editor_colorize_markdown_line;
  case editor_syntax_kind_t::Plain:
  default:
    return {};
  }
}

inline void editor_configure_syntax(editor_box_data_t &editor,
                                    editor_syntax_kind_t kind) {
  editor.line_colorizer = editor_make_syntax_colorizer(kind);
}

inline void editor_configure_syntax_from_path(editor_box_data_t &editor) {
  editor_configure_syntax(editor, editor_syntax_kind_for_path(editor.path));
}

[[nodiscard]] inline std::string
editor_join_lines(const std::vector<std::string> &lines) {
  if (lines.empty())
    return {};
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    oss << lines[i];
    if (i + 1 < lines.size())
      oss << '\n';
  }
  return oss.str();
}

inline void editor_set_text(editorBox_data_t &ed, const std::string &text) {
  ed.lines = split_lines_keep_empty(text);
  ed.cursor_line = 0;
  ed.cursor_col = 0;
  ed.top_line = 0;
  ed.left_col = 0;
  ed.preferred_col = -1;
  ed.undo_stack.clear();
  ed.redo_stack.clear();
  ed.history_merge_kind = editorBox_data_t::history_merge_kind_t::None;
  ed.history_merge_cursor_line = -1;
  ed.history_merge_cursor_col = -1;
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
  ed.ensure_nonempty();
  ed.history_merge_line_count = static_cast<int>(ed.lines.size());
}

[[nodiscard]] inline std::string editor_text(const editorBox_data_t &ed) {
  return editor_join_lines(ed.lines);
}

inline void editor_ensure_cursor_visible(editorBox_data_t &ed);

[[nodiscard]] inline editorBox_data_t::history_frame_t
editor_capture_history_frame(const editorBox_data_t &ed) {
  editorBox_data_t::history_frame_t frame{};
  frame.lines = ed.lines;
  frame.cursor_line = ed.cursor_line;
  frame.cursor_col = ed.cursor_col;
  frame.preferred_col = ed.preferred_col;
  frame.top_line = ed.top_line;
  frame.left_col = ed.left_col;
  frame.dirty = ed.dirty;
  return frame;
}

[[nodiscard]] inline bool
editor_history_frame_same_state(const editorBox_data_t::history_frame_t &lhs,
                                const editorBox_data_t::history_frame_t &rhs) {
  return lhs.lines == rhs.lines && lhs.cursor_line == rhs.cursor_line &&
         lhs.cursor_col == rhs.cursor_col &&
         lhs.preferred_col == rhs.preferred_col &&
         lhs.top_line == rhs.top_line && lhs.left_col == rhs.left_col &&
         lhs.dirty == rhs.dirty;
}

[[nodiscard]] inline bool editor_history_frame_same_edit_surface(
    const editorBox_data_t::history_frame_t &lhs,
    const editorBox_data_t::history_frame_t &rhs) {
  return lhs.lines == rhs.lines && lhs.cursor_line == rhs.cursor_line &&
         lhs.cursor_col == rhs.cursor_col &&
         lhs.preferred_col == rhs.preferred_col &&
         lhs.top_line == rhs.top_line && lhs.left_col == rhs.left_col;
}

inline void editor_trim_history(editorBox_data_t &ed) {
  if (ed.history_limit == 0) {
    ed.undo_stack.clear();
    ed.redo_stack.clear();
    return;
  }
  if (ed.undo_stack.size() > ed.history_limit) {
    const std::size_t extra = ed.undo_stack.size() - ed.history_limit;
    ed.undo_stack.erase(ed.undo_stack.begin(),
                        ed.undo_stack.begin() +
                            static_cast<std::ptrdiff_t>(extra));
  }
  if (ed.redo_stack.size() > ed.history_limit) {
    const std::size_t extra = ed.redo_stack.size() - ed.history_limit;
    ed.redo_stack.erase(ed.redo_stack.begin(),
                        ed.redo_stack.begin() +
                            static_cast<std::ptrdiff_t>(extra));
  }
}

inline void
editor_push_undo_frame(editorBox_data_t &ed,
                       const editorBox_data_t::history_frame_t &frame) {
  if (!ed.history_enabled || ed.history_limit == 0)
    return;
  if (!ed.undo_stack.empty() &&
      editor_history_frame_same_state(ed.undo_stack.back(), frame))
    return;
  ed.undo_stack.push_back(frame);
  editor_trim_history(ed);
}

inline void editor_history_break_merge_chain(editorBox_data_t &ed) {
  ed.history_merge_kind = editorBox_data_t::history_merge_kind_t::None;
  ed.history_merge_cursor_line = -1;
  ed.history_merge_cursor_col = -1;
  ed.history_merge_line_count = static_cast<int>(ed.lines.size());
}

[[nodiscard]] inline bool
editor_history_can_merge(const editorBox_data_t &ed,
                         const editorBox_data_t::history_frame_t &before,
                         editorBox_data_t::history_merge_kind_t merge_kind) {
  if (!ed.history_enabled || ed.history_limit == 0)
    return false;
  if (merge_kind == editorBox_data_t::history_merge_kind_t::None)
    return false;
  if (ed.undo_stack.empty())
    return false;
  return ed.history_merge_kind == merge_kind &&
         ed.history_merge_cursor_line == before.cursor_line &&
         ed.history_merge_cursor_col == before.cursor_col &&
         ed.history_merge_line_count == static_cast<int>(before.lines.size());
}

inline void
editor_history_note_merge(editorBox_data_t &ed,
                          editorBox_data_t::history_merge_kind_t merge_kind) {
  if (merge_kind == editorBox_data_t::history_merge_kind_t::None ||
      !ed.history_enabled || ed.history_limit == 0) {
    editor_history_break_merge_chain(ed);
    return;
  }
  ed.history_merge_kind = merge_kind;
  ed.history_merge_cursor_line = ed.cursor_line;
  ed.history_merge_cursor_col = ed.cursor_col;
  ed.history_merge_line_count = static_cast<int>(ed.lines.size());
}

template <class Fn>
inline bool
editor_apply_mutation(editorBox_data_t &ed, Fn &&mutation,
                      editorBox_data_t::history_merge_kind_t merge_kind =
                          editorBox_data_t::history_merge_kind_t::None) {
  ed.ensure_nonempty();
  const auto before = editor_capture_history_frame(ed);
  mutation();
  const auto after = editor_capture_history_frame(ed);
  if (editor_history_frame_same_edit_surface(before, after))
    return false;
  if (!editor_history_can_merge(ed, before, merge_kind)) {
    editor_push_undo_frame(ed, before);
  }
  ed.redo_stack.clear();
  ed.dirty = true;
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
  editor_history_note_merge(ed, merge_kind);
  editor_ensure_cursor_visible(ed);
  return true;
}

struct editor_keymap_options_t {
  bool allow_save{true};
  bool allow_reload{true};
  bool allow_history{true};
  bool allow_close{true};
  bool allow_newline{true};
  bool allow_tab_indent{true};
  bool allow_printable_insert{true};
  bool escape_requests_close{false};
};

struct editor_key_result_t : public primitive_key_result_t {
  bool saved{false};
  bool loaded{false};
  bool undo{false};
  bool redo{false};
};

struct editor_delimiter_match_t {
  int anchor_line{0};
  int anchor_col{0};
  int match_line{0};
  int match_col{0};
};

[[nodiscard]] inline int editor_tab_width(const editorBox_data_t &ed) {
  return std::max(1, ed.tab_width);
}

[[nodiscard]] inline int editor_visual_advance(const editorBox_data_t &ed,
                                               char ch, int visual_col) {
  if (ch == '\t') {
    const int tw = editor_tab_width(ed);
    const int rem = visual_col % tw;
    return (rem == 0) ? tw : (tw - rem);
  }
  return 1;
}

[[nodiscard]] inline int
editor_visual_column_for_raw_column(const editorBox_data_t &ed,
                                    std::string_view line, int raw_col) {
  raw_col = std::clamp(raw_col, 0, static_cast<int>(line.size()));
  int visual_col = 0;
  for (int i = 0; i < raw_col; ++i) {
    visual_col += editor_visual_advance(ed, line[static_cast<std::size_t>(i)],
                                        visual_col);
  }
  return visual_col;
}

[[nodiscard]] inline int editor_raw_column_for_visual_column(
    const editorBox_data_t &ed, std::string_view line, int target_visual_col) {
  target_visual_col = std::max(0, target_visual_col);
  int visual_col = 0;
  int raw_col = 0;
  while (raw_col < static_cast<int>(line.size()) &&
         visual_col < target_visual_col) {
    visual_col += editor_visual_advance(
        ed, line[static_cast<std::size_t>(raw_col)], visual_col);
    ++raw_col;
  }
  return raw_col;
}

inline void editor_ensure_cursor_visible(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  const int body_h = std::max(1, ed.last_body_h);
  const int text_w = std::max(1, ed.last_text_w);

  if (ed.cursor_line < ed.top_line)
    ed.top_line = ed.cursor_line;
  if (ed.cursor_line >= ed.top_line + body_h)
    ed.top_line = ed.cursor_line - body_h + 1;

  const std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor_visual_col =
      editor_visual_column_for_raw_column(ed, line, ed.cursor_col);
  const int left_visual_col = std::max(0, ed.left_col);

  if (cursor_visual_col < left_visual_col) {
    ed.left_col = cursor_visual_col;
  } else if (cursor_visual_col >= left_visual_col + text_w) {
    ed.left_col = cursor_visual_col - text_w + 1;
  }
  if (ed.top_line < 0)
    ed.top_line = 0;
  if (ed.left_col < 0)
    ed.left_col = 0;
}

inline void
editor_restore_history_frame(editorBox_data_t &ed,
                             const editorBox_data_t::history_frame_t &frame) {
  ed.lines = frame.lines;
  ed.cursor_line = frame.cursor_line;
  ed.cursor_col = frame.cursor_col;
  ed.preferred_col = frame.preferred_col;
  ed.top_line = frame.top_line;
  ed.left_col = frame.left_col;
  ed.dirty = frame.dirty;
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
  editor_history_break_merge_chain(ed);
  ed.ensure_nonempty();
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_left(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  if (ed.cursor_col > 0) {
    --ed.cursor_col;
  } else if (ed.cursor_line > 0) {
    --ed.cursor_line;
    ed.cursor_col = static_cast<int>(
        ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  }
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_right(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  const int line_len = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  if (ed.cursor_col < line_len) {
    ++ed.cursor_col;
  } else if (ed.cursor_line + 1 < static_cast<int>(ed.lines.size())) {
    ++ed.cursor_line;
    ed.cursor_col = 0;
  }
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_up(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0)
    ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line > 0)
    --ed.cursor_line;
  const int line_len = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_down(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0)
    ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line + 1 < static_cast<int>(ed.lines.size()))
    ++ed.cursor_line;
  const int line_len = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_home(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  ed.cursor_col = 0;
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_end(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  ed.cursor_col = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_up(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line = std::max(0, ed.cursor_line - delta);
  const int line_len = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_down(editorBox_data_t &ed) {
  ed.ensure_nonempty();
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line =
      std::min(static_cast<int>(ed.lines.size()) - 1, ed.cursor_line + delta);
  const int line_len = static_cast<int>(
      ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_insert_char(editorBox_data_t &ed, char ch) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        line.insert(line.begin() + ed.cursor_col, ch);
        ++ed.cursor_col;
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::InsertChar);
}

inline void editor_insert_text(editorBox_data_t &ed, std::string_view text) {
  if (text.empty())
    return;

  std::string normalized;
  normalized.reserve(text.size());
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n')
        ++i;
      normalized.push_back('\n');
      continue;
    }
    normalized.push_back(ch);
  }
  if (normalized.empty())
    return;

  (void)editor_apply_mutation(ed, [&]() {
    const std::size_t line_idx = static_cast<std::size_t>(ed.cursor_line);
    std::string &line = ed.lines[line_idx];
    if (normalized.find('\n') == std::string::npos) {
      line.insert(static_cast<std::size_t>(ed.cursor_col), normalized);
      ed.cursor_col += static_cast<int>(normalized.size());
    } else {
      const std::string right =
          line.substr(static_cast<std::size_t>(ed.cursor_col));
      auto inserted = split_lines_keep_empty(normalized);
      if (inserted.empty())
        inserted.emplace_back();

      line.erase(static_cast<std::size_t>(ed.cursor_col));
      line += inserted.front();

      if (inserted.size() == 1) {
        line += right;
        ed.cursor_col += static_cast<int>(inserted.front().size());
      } else {
        std::vector<std::string> tail(inserted.begin() + 1, inserted.end());
        tail.back() += right;
        ed.lines.insert(ed.lines.begin() + ed.cursor_line + 1, tail.begin(),
                        tail.end());
        ed.cursor_line += static_cast<int>(tail.size());
        ed.cursor_col = static_cast<int>(inserted.back().size());
      }
    }
    ed.preferred_col = ed.cursor_col;
  });
}

inline void editor_insert_newline(editorBox_data_t &ed) {
  editor_insert_text(ed, "\n");
}

[[nodiscard]] inline int
editor_spaces_to_next_tab_stop(const editorBox_data_t &ed) {
  if (ed.cursor_line < 0 ||
      ed.cursor_line >= static_cast<int>(ed.lines.size())) {
    return editor_tab_width(ed);
  }
  const std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int visual_col =
      editor_visual_column_for_raw_column(ed, line, ed.cursor_col);
  const int tw = editor_tab_width(ed);
  const int rem = visual_col % tw;
  return (rem == 0) ? tw : (tw - rem);
}

inline void editor_insert_tab(editorBox_data_t &ed) {
  editor_insert_text(ed, std::string(static_cast<std::size_t>(
                                         editor_spaces_to_next_tab_stop(ed)),
                                     ' '));
}

[[nodiscard]] inline bool editor_can_undo(const editorBox_data_t &ed) {
  return ed.history_enabled && !ed.undo_stack.empty();
}

[[nodiscard]] inline bool editor_can_redo(const editorBox_data_t &ed) {
  return ed.history_enabled && !ed.redo_stack.empty();
}

[[nodiscard]] inline bool editor_undo(editorBox_data_t &ed) {
  if (!editor_can_undo(ed))
    return false;
  const auto current = editor_capture_history_frame(ed);
  const auto frame = ed.undo_stack.back();
  ed.undo_stack.pop_back();
  if (ed.history_enabled && ed.history_limit > 0) {
    ed.redo_stack.push_back(current);
    editor_trim_history(ed);
  }
  editor_restore_history_frame(ed, frame);
  editor_history_break_merge_chain(ed);
  ed.status = "undo";
  return true;
}

[[nodiscard]] inline bool editor_redo(editorBox_data_t &ed) {
  if (!editor_can_redo(ed))
    return false;
  const auto current = editor_capture_history_frame(ed);
  const auto frame = ed.redo_stack.back();
  ed.redo_stack.pop_back();
  if (ed.history_enabled && ed.history_limit > 0) {
    ed.undo_stack.push_back(current);
    editor_trim_history(ed);
  }
  editor_restore_history_frame(ed, frame);
  editor_history_break_merge_chain(ed);
  ed.status = "redo";
  return true;
}

inline void editor_backspace(editorBox_data_t &ed) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        if (ed.cursor_col > 0) {
          std::string &line =
              ed.lines[static_cast<std::size_t>(ed.cursor_line)];
          line.erase(line.begin() + (ed.cursor_col - 1));
          --ed.cursor_col;
          ed.preferred_col = ed.cursor_col;
          return;
        }
        if (ed.cursor_line == 0)
          return;
        const int prev_line = ed.cursor_line - 1;
        std::string &dst = ed.lines[static_cast<std::size_t>(prev_line)];
        std::string &src = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        const int join_col = static_cast<int>(dst.size());
        dst += src;
        ed.lines.erase(ed.lines.begin() + ed.cursor_line);
        ed.cursor_line = prev_line;
        ed.cursor_col = join_col;
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::BackspaceChar);
}

inline void editor_delete(editorBox_data_t &ed) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        if (ed.cursor_col < static_cast<int>(line.size())) {
          line.erase(line.begin() + ed.cursor_col);
          ed.preferred_col = ed.cursor_col;
          return;
        }
        if (ed.cursor_line + 1 >= static_cast<int>(ed.lines.size()))
          return;
        line += ed.lines[static_cast<std::size_t>(ed.cursor_line + 1)];
        ed.lines.erase(ed.lines.begin() + ed.cursor_line + 1);
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::DeleteChar);
}

inline void editor_delete_to_eol(editorBox_data_t &ed) {
  (void)editor_apply_mutation(ed, [&]() {
    std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    if (ed.cursor_col >= static_cast<int>(line.size()))
      return;
    line.erase(static_cast<std::size_t>(ed.cursor_col));
    ed.preferred_col = ed.cursor_col;
  });
}

[[nodiscard]] inline bool editor_is_word_char(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_' || ch == '.' || ch == '@' || ch == ':' ||
         ch == '-';
}

inline void editor_delete_prev_word(editorBox_data_t &ed) {
  (void)editor_apply_mutation(ed, [&]() {
    if (ed.cursor_col == 0) {
      if (ed.cursor_line == 0)
        return;
      const int prev_line = ed.cursor_line - 1;
      std::string &dst = ed.lines[static_cast<std::size_t>(prev_line)];
      std::string &src = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
      const int join_col = static_cast<int>(dst.size());
      dst += src;
      ed.lines.erase(ed.lines.begin() + ed.cursor_line);
      ed.cursor_line = prev_line;
      ed.cursor_col = join_col;
      ed.preferred_col = ed.cursor_col;
      return;
    }
    std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    int pos = ed.cursor_col;
    while (pos > 0 && std::isspace(static_cast<unsigned char>(
                          line[static_cast<std::size_t>(pos - 1)])))
      --pos;
    while (pos > 0 &&
           editor_is_word_char(line[static_cast<std::size_t>(pos - 1)]))
      --pos;
    while (pos > 0 &&
           !std::isspace(static_cast<unsigned char>(
               line[static_cast<std::size_t>(pos - 1)])) &&
           !editor_is_word_char(line[static_cast<std::size_t>(pos - 1)])) {
      --pos;
    }
    if (pos >= ed.cursor_col)
      return;
    line.erase(static_cast<std::size_t>(pos),
               static_cast<std::size_t>(ed.cursor_col - pos));
    ed.cursor_col = pos;
    ed.preferred_col = ed.cursor_col;
  });
}

[[nodiscard]] inline bool editor_is_open_delimiter(char ch) {
  return ch == '(' || ch == '[' || ch == '{';
}

[[nodiscard]] inline bool editor_is_close_delimiter(char ch) {
  return ch == ')' || ch == ']' || ch == '}';
}

[[nodiscard]] inline char editor_matching_delimiter_char(char ch) {
  switch (ch) {
  case '(':
    return ')';
  case '[':
    return ']';
  case '{':
    return '}';
  case ')':
    return '(';
  case ']':
    return '[';
  case '}':
    return '{';
  default:
    return '\0';
  }
}

[[nodiscard]] inline std::optional<std::pair<int, int>>
editor_delimiter_anchor_at_cursor(const editorBox_data_t &ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size()))
    return std::nullopt;
  const std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor =
      std::clamp(ed.cursor_col, 0, static_cast<int>(line.size()));
  if (cursor < static_cast<int>(line.size())) {
    const char ch = line[static_cast<std::size_t>(cursor)];
    if (editor_is_open_delimiter(ch) || editor_is_close_delimiter(ch)) {
      return std::make_pair(ed.cursor_line, cursor);
    }
  }
  if (cursor > 0) {
    const int left = cursor - 1;
    const char ch = line[static_cast<std::size_t>(left)];
    if (editor_is_open_delimiter(ch) || editor_is_close_delimiter(ch)) {
      return std::make_pair(ed.cursor_line, left);
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<editor_delimiter_match_t>
editor_matching_delimiter_at_cursor(const editorBox_data_t &ed) {
  const auto anchor = editor_delimiter_anchor_at_cursor(ed);
  if (!anchor.has_value())
    return std::nullopt;

  const int anchor_line = anchor->first;
  const int anchor_col = anchor->second;
  const char anchor_char = ed.lines[static_cast<std::size_t>(anchor_line)]
                                   [static_cast<std::size_t>(anchor_col)];
  const char match_char = editor_matching_delimiter_char(anchor_char);
  if (match_char == '\0')
    return std::nullopt;

  if (editor_is_open_delimiter(anchor_char)) {
    int depth = 0;
    for (int line_index = anchor_line;
         line_index < static_cast<int>(ed.lines.size()); ++line_index) {
      const std::string &line = ed.lines[static_cast<std::size_t>(line_index)];
      int col = 0;
      if (line_index == anchor_line)
        col = anchor_col + 1;
      for (; col < static_cast<int>(line.size()); ++col) {
        const char ch = line[static_cast<std::size_t>(col)];
        if (ch == anchor_char) {
          ++depth;
        } else if (ch == match_char) {
          if (depth == 0) {
            return editor_delimiter_match_t{anchor_line, anchor_col, line_index,
                                            col};
          }
          --depth;
        }
      }
    }
    return std::nullopt;
  }

  int depth = 0;
  for (int line_index = anchor_line; line_index >= 0; --line_index) {
    const std::string &line = ed.lines[static_cast<std::size_t>(line_index)];
    int col = static_cast<int>(line.size()) - 1;
    if (line_index == anchor_line)
      col = anchor_col - 1;
    for (; col >= 0; --col) {
      const char ch = line[static_cast<std::size_t>(col)];
      if (ch == anchor_char) {
        ++depth;
      } else if (ch == match_char) {
        if (depth == 0) {
          return editor_delimiter_match_t{anchor_line, anchor_col, line_index,
                                          col};
        }
        --depth;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline bool editor_load_file(editorBox_data_t &ed,
                                           std::string_view path_in = {},
                                           std::string *error = nullptr) {
  if (error)
    error->clear();
  const std::string path = path_in.empty() ? ed.path : std::string(path_in);
  if (path.empty()) {
    if (error)
      *error = "editor path is empty";
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error)
      *error = "cannot open file: " + path;
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  ed.path = path;
  editor_set_text(ed, ss.str());
  ed.dirty = false;
  editor_history_break_merge_chain(ed);
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
  ed.status = "loaded";
  return true;
}

[[nodiscard]] inline bool editor_save_file(editorBox_data_t &ed,
                                           std::string_view path_in = {},
                                           std::string *error = nullptr) {
  if (error)
    error->clear();
  const std::string path = path_in.empty() ? ed.path : std::string(path_in);
  if (path.empty()) {
    if (error)
      *error = "editor path is empty";
    return false;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error)
      *error = "cannot write file: " + path;
    return false;
  }
  out << editor_text(ed);
  if (!out.good()) {
    if (error)
      *error = "write failed: " + path;
    return false;
  }
  ed.path = path;
  ed.dirty = false;
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
  editor_history_break_merge_chain(ed);
  ed.status = "saved";
  return true;
}

struct editor_completion_t {
  bool changed{false};
  std::size_t matches{0};
  std::string status{};
};

[[nodiscard]] inline std::optional<std::pair<int, std::string>>
editor_token_prefix_at_cursor(const editorBox_data_t &ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size()))
    return std::nullopt;
  const std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor =
      std::clamp(ed.cursor_col, 0, static_cast<int>(line.size()));
  int start = cursor;
  while (start > 0 &&
         editor_is_word_char(line[static_cast<std::size_t>(start - 1)]))
    --start;
  if (start == cursor)
    return std::nullopt;
  return std::make_pair(start,
                        line.substr(static_cast<std::size_t>(start),
                                    static_cast<std::size_t>(cursor - start)));
}

[[nodiscard]] inline std::string
longest_common_prefix(const std::vector<std::string_view> &values) {
  if (values.empty())
    return {};
  std::string out(values[0]);
  for (std::size_t i = 1; i < values.size() && !out.empty(); ++i) {
    const std::string_view v = values[i];
    std::size_t j = 0;
    const std::size_t n = std::min(out.size(), v.size());
    while (j < n && out[j] == v[j])
      ++j;
    out.resize(j);
  }
  return out;
}

[[nodiscard]] inline editor_completion_t
editor_complete(editorBox_data_t &ed, std::span<const std::string> candidates) {
  editor_completion_t out{};
  ed.ensure_nonempty();
  const auto pref = editor_token_prefix_at_cursor(ed);
  if (!pref.has_value()) {
    out.status = "no completion prefix";
    return out;
  }
  const int start = pref->first;
  const std::string &prefix = pref->second;

  std::vector<std::string_view> matches;
  matches.reserve(candidates.size());
  for (const auto &cand : candidates) {
    if (cand.size() < prefix.size())
      continue;
    if (cand.compare(0, prefix.size(), prefix) == 0) {
      matches.emplace_back(cand);
    }
  }
  out.matches = matches.size();
  if (matches.empty()) {
    out.status = "no completion";
    return out;
  }

  std::string replacement;
  if (matches.size() == 1) {
    replacement = std::string(matches[0]);
  } else {
    replacement = longest_common_prefix(matches);
  }

  if (replacement.size() < prefix.size()) {
    out.status = std::to_string(matches.size()) + " matches";
    return out;
  }
  if (replacement == prefix) {
    out.status = std::to_string(matches.size()) + " matches";
    return out;
  }

  out.changed = editor_apply_mutation(ed, [&]() {
    std::string &line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    line.replace(static_cast<std::size_t>(start), prefix.size(), replacement);
    ed.cursor_col = start + static_cast<int>(replacement.size());
    ed.preferred_col = ed.cursor_col;
  });
  out.status = (matches.size() == 1)
                   ? "completion accepted"
                   : (std::to_string(matches.size()) + " matches");
  return out;
}

[[nodiscard]] inline editor_key_result_t
editor_handle_key(editorBox_data_t &ed, int ch,
                  const editor_keymap_options_t &opts = {}) {
  const auto before = editor_capture_history_frame(ed);
  editor_key_result_t out{};

  auto set_status = [&](std::string s) {
    ed.status = s;
    out.status = ed.status;
  };

  auto refuse_read_only = [&]() {
    out.handled = true;
    set_status("read only");
  };

  auto handle_close_request = [&](std::string_view prompt_key,
                                  bool via_escape) {
    out.handled = true;
    editor_history_break_merge_chain(ed);
    if (!ed.dirty) {
      ed.close_armed = false;
      ed.close_armed_via_escape = false;
      out.close_requested = true;
      set_status("close editor");
      return;
    }
    if (ed.close_armed) {
      ed.close_armed = false;
      ed.close_armed_via_escape = false;
      out.close_requested = true;
      set_status("discard changes");
      return;
    }
    ed.close_armed = true;
    ed.close_armed_via_escape = via_escape;
    set_status("unsaved changes, press " + std::string(prompt_key) +
               " again to exit");
  };

  const key_action_t action = standard_key_action(ch);
  const bool is_close_key =
      opts.allow_close &&
      (action == key_action_t::Close ||
       (opts.escape_requests_close && action == key_action_t::Cancel));
  const bool is_save_key = opts.allow_save && action == key_action_t::Save;
  if (!is_close_key && !is_save_key) {
    ed.close_armed = false;
    ed.close_armed_via_escape = false;
  }

  switch (action) {
  case key_action_t::MoveUp:
    editor_move_up(ed);
    out.handled = true;
    break;
  case key_action_t::MoveDown:
    editor_move_down(ed);
    out.handled = true;
    break;
  case key_action_t::MoveLeft:
    editor_move_left(ed);
    out.handled = true;
    break;
  case key_action_t::MoveRight:
    editor_move_right(ed);
    out.handled = true;
    break;
  case key_action_t::Home:
    editor_move_home(ed);
    out.handled = true;
    break;
  case key_action_t::End:
    editor_move_end(ed);
    out.handled = true;
    break;
  case key_action_t::PageUp:
    editor_page_up(ed);
    out.handled = true;
    break;
  case key_action_t::PageDown:
    editor_page_down(ed);
    out.handled = true;
    break;
  case key_action_t::Backspace:
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_backspace(ed);
      out.handled = true;
    }
    break;
  case key_action_t::Delete:
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_delete(ed);
      out.handled = true;
    }
    break;
  case key_action_t::Enter:
    if (!opts.allow_newline)
      break;
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_insert_newline(ed);
      out.handled = true;
    }
    break;
  case key_action_t::Tab:
    if (!opts.allow_tab_indent)
      break;
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_insert_tab(ed);
      out.handled = true;
    }
    break;
  case key_action_t::DeleteToEndOfLine:
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_delete_to_eol(ed);
      out.handled = true;
    }
    break;
  case key_action_t::DeletePreviousWord:
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_delete_prev_word(ed);
      out.handled = true;
    }
    break;
  case key_action_t::Reload:
    if (!opts.allow_reload)
      break;
    out.handled = true;
    editor_history_break_merge_chain(ed);
    {
      std::string error{};
      if (editor_load_file(ed, {}, &error)) {
        out.loaded = true;
        set_status("loaded");
      } else {
        set_status("reload failed: " + error);
      }
    }
    break;
  case key_action_t::Save:
    if (!opts.allow_save)
      break;
    out.handled = true;
    {
      const bool close_after_save = ed.close_armed;
      editor_history_break_merge_chain(ed);
      if (ed.read_only) {
        set_status("read only");
        break;
      }
      std::string error{};
      if (editor_save_file(ed, {}, &error)) {
        out.saved = true;
        if (close_after_save) {
          out.close_requested = true;
          ed.close_armed = false;
          ed.close_armed_via_escape = false;
          set_status("saved and closed");
        } else {
          set_status("saved");
        }
      } else {
        set_status("save failed: " + error);
      }
    }
    break;
  case key_action_t::Redo:
    if (!opts.allow_history)
      break;
    out.handled = true;
    if (editor_redo(ed)) {
      out.redo = true;
      set_status("redo");
    } else {
      set_status("redo unavailable");
    }
    break;
  case key_action_t::Undo:
    if (!opts.allow_history)
      break;
    out.handled = true;
    if (editor_undo(ed)) {
      out.undo = true;
      set_status("undo");
    } else {
      set_status("undo unavailable");
    }
    break;
  case key_action_t::Close:
    if (opts.allow_close)
      handle_close_request("Ctrl+Q", false);
    break;
  case key_action_t::Cancel:
    if (opts.allow_close && opts.escape_requests_close) {
      handle_close_request("Esc", true);
    }
    break;
  case key_action_t::Printable:
    if (!opts.allow_printable_insert)
      break;
    if (ed.read_only) {
      refuse_read_only();
    } else {
      editor_insert_char(ed, static_cast<char>(ch));
      out.handled = true;
    }
    break;
  case key_action_t::None:
    break;
  }

  const auto after = editor_capture_history_frame(ed);
  out.content_changed =
      before.lines != after.lines || before.dirty != after.dirty;
  out.viewport_changed = before.cursor_line != after.cursor_line ||
                         before.cursor_col != after.cursor_col ||
                         before.top_line != after.top_line ||
                         before.left_col != after.left_col;

  if (out.handled && out.status.empty() && out.content_changed) {
    set_status("editing");
  } else if (out.handled && out.status.empty()) {
    out.status = ed.status;
  }

  return out;
}

[[nodiscard]] inline editor_key_result_t
editor_handle_key(const std::shared_ptr<iinuji_object_t> &obj, int ch,
                  const editor_keymap_options_t &opts = {}) {
  if (!obj)
    return {};
  auto ed = std::dynamic_pointer_cast<editor_box_data_t>(obj->data);
  if (!ed)
    return {};
  return editor_handle_key(*ed, ch, opts);
}

inline std::shared_ptr<iinuji_object_t>
create_editor_box(const std::string &id, editor_box_opts_t opts) {
  auto obj = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style),
      primitive_role_t::EditorBox, opts.focus_mode);
  auto ed = std::make_shared<editor_box_data_t>(std::move(opts.path));
  ed->read_only = opts.read_only;
  editor_set_text(*ed, opts.text);
  editor_configure_syntax_from_path(*ed);
  ed->dirty = false;
  obj->data = ed;
  return obj;
}

inline std::shared_ptr<iinuji_object_t>
create_editor_box(const std::string &id, std::string path = {},
                  std::string text = {}, bool read_only = false,
                  const iinuji_layout_t &layout = {},
                  const iinuji_style_t &style = {}) {
  editor_box_opts_t opts{};
  opts.path = std::move(path);
  opts.text = std::move(text);
  opts.read_only = read_only;
  opts.layout = layout;
  opts.style = style;
  return create_editor_box(id, std::move(opts));
}

[[nodiscard]] inline editor_key_result_t
handle_editor_key(editor_box_data_t &editor, int key,
                  const editor_keymap_options_t &opts = {}) {
  return editor_handle_key(editor, key, opts);
}

[[nodiscard]] inline editor_key_result_t
handle_editor_key(const std::shared_ptr<iinuji_object_t> &object, int key,
                  const editor_keymap_options_t &opts = {}) {
  return editor_handle_key(object, key, opts);
}

} // namespace primitives

inline std::shared_ptr<iinuji_object_t>
create_editor_box(const std::string &id, editor_box_opts_t opts) {
  return primitives::create_editor_box(id, std::move(opts));
}

inline std::shared_ptr<iinuji_object_t>
create_editor_box(const std::string &id, std::string path = {},
                  std::string text = {}, bool read_only = false,
                  const iinuji_layout_t &layout = {},
                  const iinuji_style_t &style = {}) {
  return primitives::create_editor_box(id, std::move(path), std::move(text),
                                       read_only, layout, style);
}

} // namespace iinuji
} // namespace cuwacunu
