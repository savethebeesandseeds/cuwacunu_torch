#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"

namespace cuwacunu {
namespace iinuji {
namespace primitives {

[[nodiscard]] inline std::string editor_join_lines(const std::vector<std::string>& lines) {
  if (lines.empty()) return {};
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    oss << lines[i];
    if (i + 1 < lines.size()) oss << '\n';
  }
  return oss.str();
}

inline void editor_set_text(editorBox_data_t& ed, const std::string& text) {
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

[[nodiscard]] inline std::string editor_text(const editorBox_data_t& ed) {
  return editor_join_lines(ed.lines);
}

inline void editor_ensure_cursor_visible(editorBox_data_t& ed);

[[nodiscard]] inline editorBox_data_t::history_frame_t
editor_capture_history_frame(const editorBox_data_t& ed) {
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

[[nodiscard]] inline bool editor_history_frame_same_state(
    const editorBox_data_t::history_frame_t& lhs,
    const editorBox_data_t::history_frame_t& rhs) {
  return lhs.lines == rhs.lines &&
         lhs.cursor_line == rhs.cursor_line &&
         lhs.cursor_col == rhs.cursor_col &&
         lhs.preferred_col == rhs.preferred_col &&
         lhs.top_line == rhs.top_line &&
         lhs.left_col == rhs.left_col &&
         lhs.dirty == rhs.dirty;
}

[[nodiscard]] inline bool editor_history_frame_same_edit_surface(
    const editorBox_data_t::history_frame_t& lhs,
    const editorBox_data_t::history_frame_t& rhs) {
  return lhs.lines == rhs.lines &&
         lhs.cursor_line == rhs.cursor_line &&
         lhs.cursor_col == rhs.cursor_col &&
         lhs.preferred_col == rhs.preferred_col &&
         lhs.top_line == rhs.top_line &&
         lhs.left_col == rhs.left_col;
}

inline void editor_trim_history(editorBox_data_t& ed) {
  if (ed.history_limit == 0) {
    ed.undo_stack.clear();
    ed.redo_stack.clear();
    return;
  }
  if (ed.undo_stack.size() > ed.history_limit) {
    const std::size_t extra = ed.undo_stack.size() - ed.history_limit;
    ed.undo_stack.erase(ed.undo_stack.begin(),
                        ed.undo_stack.begin() + static_cast<std::ptrdiff_t>(extra));
  }
  if (ed.redo_stack.size() > ed.history_limit) {
    const std::size_t extra = ed.redo_stack.size() - ed.history_limit;
    ed.redo_stack.erase(ed.redo_stack.begin(),
                        ed.redo_stack.begin() + static_cast<std::ptrdiff_t>(extra));
  }
}

inline void editor_push_undo_frame(editorBox_data_t& ed,
                                   const editorBox_data_t::history_frame_t& frame) {
  if (!ed.history_enabled || ed.history_limit == 0) return;
  if (!ed.undo_stack.empty() && editor_history_frame_same_state(ed.undo_stack.back(), frame)) return;
  ed.undo_stack.push_back(frame);
  editor_trim_history(ed);
}

inline void editor_history_break_merge_chain(editorBox_data_t& ed) {
  ed.history_merge_kind = editorBox_data_t::history_merge_kind_t::None;
  ed.history_merge_cursor_line = -1;
  ed.history_merge_cursor_col = -1;
  ed.history_merge_line_count = static_cast<int>(ed.lines.size());
}

[[nodiscard]] inline bool editor_history_can_merge(
    const editorBox_data_t& ed,
    const editorBox_data_t::history_frame_t& before,
    editorBox_data_t::history_merge_kind_t merge_kind) {
  if (!ed.history_enabled || ed.history_limit == 0) return false;
  if (merge_kind == editorBox_data_t::history_merge_kind_t::None) return false;
  if (ed.undo_stack.empty()) return false;
  return ed.history_merge_kind == merge_kind &&
         ed.history_merge_cursor_line == before.cursor_line &&
         ed.history_merge_cursor_col == before.cursor_col &&
         ed.history_merge_line_count == static_cast<int>(before.lines.size());
}

inline void editor_history_note_merge(editorBox_data_t& ed,
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
inline bool editor_apply_mutation(
    editorBox_data_t& ed,
    Fn&& mutation,
    editorBox_data_t::history_merge_kind_t merge_kind =
        editorBox_data_t::history_merge_kind_t::None) {
  ed.ensure_nonempty();
  const auto before = editor_capture_history_frame(ed);
  mutation();
  const auto after = editor_capture_history_frame(ed);
  if (editor_history_frame_same_edit_surface(before, after)) return false;
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

struct editor_key_result_t {
  bool handled{false};
  bool content_changed{false};
  bool viewport_changed{false};
  bool saved{false};
  bool loaded{false};
  bool undo{false};
  bool redo{false};
  bool close_requested{false};
  std::string status{};
};

struct editor_delimiter_match_t {
  int anchor_line{0};
  int anchor_col{0};
  int match_line{0};
  int match_col{0};
};

[[nodiscard]] inline int editor_tab_width(const editorBox_data_t& ed) {
  return std::max(1, ed.tab_width);
}

[[nodiscard]] inline int editor_visual_advance(const editorBox_data_t& ed, char ch, int visual_col) {
  if (ch == '\t') {
    const int tw = editor_tab_width(ed);
    const int rem = visual_col % tw;
    return (rem == 0) ? tw : (tw - rem);
  }
  return 1;
}

[[nodiscard]] inline int editor_visual_column_for_raw_column(const editorBox_data_t& ed,
                                                             std::string_view line,
                                                             int raw_col) {
  raw_col = std::clamp(raw_col, 0, static_cast<int>(line.size()));
  int visual_col = 0;
  for (int i = 0; i < raw_col; ++i) {
    visual_col += editor_visual_advance(ed, line[static_cast<std::size_t>(i)], visual_col);
  }
  return visual_col;
}

[[nodiscard]] inline int editor_raw_column_for_visual_column(const editorBox_data_t& ed,
                                                             std::string_view line,
                                                             int target_visual_col) {
  target_visual_col = std::max(0, target_visual_col);
  int visual_col = 0;
  int raw_col = 0;
  while (raw_col < static_cast<int>(line.size()) && visual_col < target_visual_col) {
    visual_col += editor_visual_advance(ed, line[static_cast<std::size_t>(raw_col)], visual_col);
    ++raw_col;
  }
  return raw_col;
}

inline void editor_ensure_cursor_visible(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int body_h = std::max(1, ed.last_body_h);
  const int text_w = std::max(1, ed.last_text_w);

  if (ed.cursor_line < ed.top_line) ed.top_line = ed.cursor_line;
  if (ed.cursor_line >= ed.top_line + body_h) ed.top_line = ed.cursor_line - body_h + 1;

  const std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor_visual_col = editor_visual_column_for_raw_column(ed, line, ed.cursor_col);
  const int left_visual_col = std::max(0, ed.left_col);

  if (cursor_visual_col < left_visual_col) {
    ed.left_col = cursor_visual_col;
  } else if (cursor_visual_col >= left_visual_col + text_w) {
    ed.left_col = cursor_visual_col - text_w + 1;
  }
  if (ed.top_line < 0) ed.top_line = 0;
  if (ed.left_col < 0) ed.left_col = 0;
}

inline void editor_restore_history_frame(editorBox_data_t& ed,
                                         const editorBox_data_t::history_frame_t& frame) {
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

inline void editor_move_left(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.cursor_col > 0) {
    --ed.cursor_col;
  } else if (ed.cursor_line > 0) {
    --ed.cursor_line;
    ed.cursor_col = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  }
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_right(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
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

inline void editor_move_up(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0) ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line > 0) --ed.cursor_line;
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_down(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0) ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line + 1 < static_cast<int>(ed.lines.size())) ++ed.cursor_line;
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_home(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  ed.cursor_col = 0;
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_end(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  ed.cursor_col = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_up(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line = std::max(0, ed.cursor_line - delta);
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_down(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line = std::min(static_cast<int>(ed.lines.size()) - 1, ed.cursor_line + delta);
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_history_break_merge_chain(ed);
  editor_ensure_cursor_visible(ed);
}

inline void editor_insert_char(editorBox_data_t& ed, char ch) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        line.insert(line.begin() + ed.cursor_col, ch);
        ++ed.cursor_col;
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::InsertChar);
}

inline void editor_insert_text(editorBox_data_t& ed, std::string_view text) {
  if (text.empty()) return;

  std::string normalized;
  normalized.reserve(text.size());
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
      normalized.push_back('\n');
      continue;
    }
    normalized.push_back(ch);
  }
  if (normalized.empty()) return;

  (void)editor_apply_mutation(ed, [&]() {
    const std::size_t line_idx = static_cast<std::size_t>(ed.cursor_line);
    std::string& line = ed.lines[line_idx];
    if (normalized.find('\n') == std::string::npos) {
      line.insert(static_cast<std::size_t>(ed.cursor_col), normalized);
      ed.cursor_col += static_cast<int>(normalized.size());
    } else {
      const std::string right = line.substr(static_cast<std::size_t>(ed.cursor_col));
      auto inserted = split_lines_keep_empty(normalized);
      if (inserted.empty()) inserted.emplace_back();

      line.erase(static_cast<std::size_t>(ed.cursor_col));
      line += inserted.front();

      if (inserted.size() == 1) {
        line += right;
        ed.cursor_col += static_cast<int>(inserted.front().size());
      } else {
        std::vector<std::string> tail(inserted.begin() + 1, inserted.end());
        tail.back() += right;
        ed.lines.insert(ed.lines.begin() + ed.cursor_line + 1, tail.begin(), tail.end());
        ed.cursor_line += static_cast<int>(tail.size());
        ed.cursor_col = static_cast<int>(inserted.back().size());
      }
    }
    ed.preferred_col = ed.cursor_col;
  });
}

inline void editor_insert_newline(editorBox_data_t& ed) {
  editor_insert_text(ed, "\n");
}

[[nodiscard]] inline int editor_spaces_to_next_tab_stop(const editorBox_data_t& ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size())) {
    return editor_tab_width(ed);
  }
  const std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int visual_col = editor_visual_column_for_raw_column(ed, line, ed.cursor_col);
  const int tw = editor_tab_width(ed);
  const int rem = visual_col % tw;
  return (rem == 0) ? tw : (tw - rem);
}

inline void editor_insert_tab(editorBox_data_t& ed) {
  editor_insert_text(ed, std::string(static_cast<std::size_t>(editor_spaces_to_next_tab_stop(ed)), ' '));
}

[[nodiscard]] inline bool editor_can_undo(const editorBox_data_t& ed) {
  return ed.history_enabled && !ed.undo_stack.empty();
}

[[nodiscard]] inline bool editor_can_redo(const editorBox_data_t& ed) {
  return ed.history_enabled && !ed.redo_stack.empty();
}

[[nodiscard]] inline bool editor_undo(editorBox_data_t& ed) {
  if (!editor_can_undo(ed)) return false;
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

[[nodiscard]] inline bool editor_redo(editorBox_data_t& ed) {
  if (!editor_can_redo(ed)) return false;
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

inline void editor_backspace(editorBox_data_t& ed) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        if (ed.cursor_col > 0) {
          std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
          line.erase(line.begin() + (ed.cursor_col - 1));
          --ed.cursor_col;
          ed.preferred_col = ed.cursor_col;
          return;
        }
        if (ed.cursor_line == 0) return;
        const int prev_line = ed.cursor_line - 1;
        std::string& dst = ed.lines[static_cast<std::size_t>(prev_line)];
        std::string& src = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        const int join_col = static_cast<int>(dst.size());
        dst += src;
        ed.lines.erase(ed.lines.begin() + ed.cursor_line);
        ed.cursor_line = prev_line;
        ed.cursor_col = join_col;
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::BackspaceChar);
}

inline void editor_delete(editorBox_data_t& ed) {
  (void)editor_apply_mutation(
      ed,
      [&]() {
        std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
        if (ed.cursor_col < static_cast<int>(line.size())) {
          line.erase(line.begin() + ed.cursor_col);
          ed.preferred_col = ed.cursor_col;
          return;
        }
        if (ed.cursor_line + 1 >= static_cast<int>(ed.lines.size())) return;
        line += ed.lines[static_cast<std::size_t>(ed.cursor_line + 1)];
        ed.lines.erase(ed.lines.begin() + ed.cursor_line + 1);
        ed.preferred_col = ed.cursor_col;
      },
      editorBox_data_t::history_merge_kind_t::DeleteChar);
}

inline void editor_delete_to_eol(editorBox_data_t& ed) {
  (void)editor_apply_mutation(ed, [&]() {
    std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    if (ed.cursor_col >= static_cast<int>(line.size())) return;
    line.erase(static_cast<std::size_t>(ed.cursor_col));
    ed.preferred_col = ed.cursor_col;
  });
}

[[nodiscard]] inline bool editor_is_word_char(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_' || ch == '.' || ch == '@' || ch == ':' || ch == '-';
}

inline void editor_delete_prev_word(editorBox_data_t& ed) {
  (void)editor_apply_mutation(ed, [&]() {
    if (ed.cursor_col == 0) {
      if (ed.cursor_line == 0) return;
      const int prev_line = ed.cursor_line - 1;
      std::string& dst = ed.lines[static_cast<std::size_t>(prev_line)];
      std::string& src = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
      const int join_col = static_cast<int>(dst.size());
      dst += src;
      ed.lines.erase(ed.lines.begin() + ed.cursor_line);
      ed.cursor_line = prev_line;
      ed.cursor_col = join_col;
      ed.preferred_col = ed.cursor_col;
      return;
    }
    std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    int pos = ed.cursor_col;
    while (pos > 0 && std::isspace(static_cast<unsigned char>(line[static_cast<std::size_t>(pos - 1)]))) --pos;
    while (pos > 0 && editor_is_word_char(line[static_cast<std::size_t>(pos - 1)])) --pos;
    while (pos > 0 &&
           !std::isspace(static_cast<unsigned char>(line[static_cast<std::size_t>(pos - 1)])) &&
           !editor_is_word_char(line[static_cast<std::size_t>(pos - 1)])) {
      --pos;
    }
    if (pos >= ed.cursor_col) return;
    line.erase(static_cast<std::size_t>(pos), static_cast<std::size_t>(ed.cursor_col - pos));
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
    case '(': return ')';
    case '[': return ']';
    case '{': return '}';
    case ')': return '(';
    case ']': return '[';
    case '}': return '{';
    default: return '\0';
  }
}

[[nodiscard]] inline std::optional<std::pair<int, int>> editor_delimiter_anchor_at_cursor(
    const editorBox_data_t& ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size())) return std::nullopt;
  const std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor = std::clamp(ed.cursor_col, 0, static_cast<int>(line.size()));
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
editor_matching_delimiter_at_cursor(const editorBox_data_t& ed) {
  const auto anchor = editor_delimiter_anchor_at_cursor(ed);
  if (!anchor.has_value()) return std::nullopt;

  const int anchor_line = anchor->first;
  const int anchor_col = anchor->second;
  const char anchor_char =
      ed.lines[static_cast<std::size_t>(anchor_line)][static_cast<std::size_t>(anchor_col)];
  const char match_char = editor_matching_delimiter_char(anchor_char);
  if (match_char == '\0') return std::nullopt;

  if (editor_is_open_delimiter(anchor_char)) {
    int depth = 0;
    for (int line_index = anchor_line; line_index < static_cast<int>(ed.lines.size()); ++line_index) {
      const std::string& line = ed.lines[static_cast<std::size_t>(line_index)];
      int col = 0;
      if (line_index == anchor_line) col = anchor_col + 1;
      for (; col < static_cast<int>(line.size()); ++col) {
        const char ch = line[static_cast<std::size_t>(col)];
        if (ch == anchor_char) {
          ++depth;
        } else if (ch == match_char) {
          if (depth == 0) {
            return editor_delimiter_match_t{anchor_line, anchor_col, line_index, col};
          }
          --depth;
        }
      }
    }
    return std::nullopt;
  }

  int depth = 0;
  for (int line_index = anchor_line; line_index >= 0; --line_index) {
    const std::string& line = ed.lines[static_cast<std::size_t>(line_index)];
    int col = static_cast<int>(line.size()) - 1;
    if (line_index == anchor_line) col = anchor_col - 1;
    for (; col >= 0; --col) {
      const char ch = line[static_cast<std::size_t>(col)];
      if (ch == anchor_char) {
        ++depth;
      } else if (ch == match_char) {
        if (depth == 0) {
          return editor_delimiter_match_t{anchor_line, anchor_col, line_index, col};
        }
        --depth;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline bool editor_load_file(editorBox_data_t& ed,
                                           std::string_view path_in = {},
                                           std::string* error = nullptr) {
  if (error) error->clear();
  const std::string path = path_in.empty() ? ed.path : std::string(path_in);
  if (path.empty()) {
    if (error) *error = "editor path is empty";
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file: " + path;
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

[[nodiscard]] inline bool editor_save_file(editorBox_data_t& ed,
                                           std::string_view path_in = {},
                                           std::string* error = nullptr) {
  if (error) error->clear();
  const std::string path = path_in.empty() ? ed.path : std::string(path_in);
  if (path.empty()) {
    if (error) *error = "editor path is empty";
    return false;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot write file: " + path;
    return false;
  }
  out << editor_text(ed);
  if (!out.good()) {
    if (error) *error = "write failed: " + path;
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
editor_token_prefix_at_cursor(const editorBox_data_t& ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size())) return std::nullopt;
  const std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor = std::clamp(ed.cursor_col, 0, static_cast<int>(line.size()));
  int start = cursor;
  while (start > 0 && editor_is_word_char(line[static_cast<std::size_t>(start - 1)])) --start;
  if (start == cursor) return std::nullopt;
  return std::make_pair(start, line.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(cursor - start)));
}

[[nodiscard]] inline std::string longest_common_prefix(const std::vector<std::string_view>& values) {
  if (values.empty()) return {};
  std::string out(values[0]);
  for (std::size_t i = 1; i < values.size() && !out.empty(); ++i) {
    const std::string_view v = values[i];
    std::size_t j = 0;
    const std::size_t n = std::min(out.size(), v.size());
    while (j < n && out[j] == v[j]) ++j;
    out.resize(j);
  }
  return out;
}

[[nodiscard]] inline editor_completion_t editor_complete(editorBox_data_t& ed,
                                                         std::span<const std::string> candidates) {
  editor_completion_t out{};
  ed.ensure_nonempty();
  const auto pref = editor_token_prefix_at_cursor(ed);
  if (!pref.has_value()) {
    out.status = "no completion prefix";
    return out;
  }
  const int start = pref->first;
  const std::string& prefix = pref->second;

  std::vector<std::string_view> matches;
  matches.reserve(candidates.size());
  for (const auto& cand : candidates) {
    if (cand.size() < prefix.size()) continue;
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
    std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    line.replace(static_cast<std::size_t>(start), prefix.size(), replacement);
    ed.cursor_col = start + static_cast<int>(replacement.size());
    ed.preferred_col = ed.cursor_col;
  });
  out.status = (matches.size() == 1) ? "completion accepted" : (std::to_string(matches.size()) + " matches");
  return out;
}

[[nodiscard]] inline editor_key_result_t
editor_handle_key(editorBox_data_t& ed,
                  int ch,
                  const editor_keymap_options_t& opts = {}) {
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

  auto handle_close_request = [&](std::string_view prompt_key, bool via_escape) {
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
    set_status("unsaved changes, press " + std::string(prompt_key) + " again to discard");
  };

  const bool is_close_key =
      opts.allow_close &&
      (ch == 17 || (opts.escape_requests_close && ch == 27));
  const bool is_save_key = opts.allow_save && ch == 19;
  if (!is_close_key && !is_save_key) {
    ed.close_armed = false;
    ed.close_armed_via_escape = false;
  }

  switch (ch) {
    case KEY_UP:
    case 16:
      editor_move_up(ed);
      out.handled = true;
      break;
    case KEY_DOWN:
    case 14:
      editor_move_down(ed);
      out.handled = true;
      break;
    case KEY_LEFT:
    case 2:
      editor_move_left(ed);
      out.handled = true;
      break;
    case KEY_RIGHT:
    case 6:
      editor_move_right(ed);
      out.handled = true;
      break;
    case KEY_HOME:
    case 1:
      editor_move_home(ed);
      out.handled = true;
      break;
    case KEY_END:
    case 5:
      editor_move_end(ed);
      out.handled = true;
      break;
    case KEY_PPAGE:
      editor_page_up(ed);
      out.handled = true;
      break;
    case KEY_NPAGE:
      editor_page_down(ed);
      out.handled = true;
      break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_backspace(ed);
        out.handled = true;
      }
      break;
    case KEY_DC:
    case 4:
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_delete(ed);
        out.handled = true;
      }
      break;
    case KEY_ENTER:
    case '\n':
    case '\r':
      if (!opts.allow_newline) break;
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_insert_newline(ed);
        out.handled = true;
      }
      break;
    case '\t':
      if (!opts.allow_tab_indent) break;
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_insert_tab(ed);
        out.handled = true;
      }
      break;
    case 11:
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_delete_to_eol(ed);
        out.handled = true;
      }
      break;
    case 23:
      if (ed.read_only) {
        refuse_read_only();
      } else {
        editor_delete_prev_word(ed);
        out.handled = true;
      }
      break;
    case 18:
      if (!opts.allow_reload) break;
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
    case 19:
      if (!opts.allow_save) break;
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
    case 25:
      if (!opts.allow_history) break;
      out.handled = true;
      if (editor_redo(ed)) {
        out.redo = true;
        set_status("redo");
      } else {
        set_status("redo unavailable");
      }
      break;
    case 26:
      if (!opts.allow_history) break;
      out.handled = true;
      if (editor_undo(ed)) {
        out.undo = true;
        set_status("undo");
      } else {
        set_status("undo unavailable");
      }
      break;
    case 17:
      if (opts.allow_close) handle_close_request("Ctrl+Q", false);
      break;
    case 27:
      if (opts.allow_close && opts.escape_requests_close) {
        handle_close_request("Esc", true);
      }
      break;
    default:
      if (opts.allow_printable_insert && ch >= 32 && ch <= 126) {
        if (ed.read_only) {
          refuse_read_only();
        } else {
          editor_insert_char(ed, static_cast<char>(ch));
          out.handled = true;
        }
      }
      break;
  }

  const auto after = editor_capture_history_frame(ed);
  out.content_changed = before.lines != after.lines || before.dirty != after.dirty;
  out.viewport_changed =
      before.cursor_line != after.cursor_line ||
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
editor_handle_key(const std::shared_ptr<iinuji_object_t>& obj,
                  int ch,
                  const editor_keymap_options_t& opts = {}) {
  if (!obj) return {};
  auto ed = std::dynamic_pointer_cast<editorBox_data_t>(obj->data);
  if (!ed) return {};
  return editor_handle_key(*ed, ch, opts);
}

inline std::shared_ptr<iinuji_object_t> create_editor_box(
    const std::string& id,
    std::string path = {},
    std::string text = {},
    bool read_only = false,
    const iinuji_layout_t& layout = {},
    const iinuji_style_t& style = {}) {
  auto obj = create_object(id, true, layout, style);
  obj->focusable = true;
  auto ed = std::make_shared<editorBox_data_t>(std::move(path));
  ed->read_only = read_only;
  editor_set_text(*ed, text);
  ed->dirty = false;
  obj->data = ed;
  return obj;
}

}  // namespace primitives
}  // namespace iinuji
}  // namespace cuwacunu
