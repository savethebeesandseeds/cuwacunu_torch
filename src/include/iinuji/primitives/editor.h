#pragma once

#include <algorithm>
#include <cctype>
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
  ed.ensure_nonempty();
}

[[nodiscard]] inline std::string editor_text(const editorBox_data_t& ed) {
  return editor_join_lines(ed.lines);
}

inline void editor_ensure_cursor_visible(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int body_h = std::max(1, ed.last_body_h);
  const int text_w = std::max(1, ed.last_text_w);

  if (ed.cursor_line < ed.top_line) ed.top_line = ed.cursor_line;
  if (ed.cursor_line >= ed.top_line + body_h) ed.top_line = ed.cursor_line - body_h + 1;

  if (ed.cursor_col < ed.left_col) ed.left_col = ed.cursor_col;
  if (ed.cursor_col >= ed.left_col + text_w) ed.left_col = ed.cursor_col - text_w + 1;
  if (ed.top_line < 0) ed.top_line = 0;
  if (ed.left_col < 0) ed.left_col = 0;
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
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_up(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0) ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line > 0) --ed.cursor_line;
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_down(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.preferred_col < 0) ed.preferred_col = ed.cursor_col;
  if (ed.cursor_line + 1 < static_cast<int>(ed.lines.size())) ++ed.cursor_line;
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::clamp(ed.preferred_col, 0, line_len);
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_home(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  ed.cursor_col = 0;
  ed.preferred_col = ed.cursor_col;
  editor_ensure_cursor_visible(ed);
}

inline void editor_move_end(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  ed.cursor_col = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.preferred_col = ed.cursor_col;
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_up(editorBox_data_t& ed) {
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line = std::max(0, ed.cursor_line - delta);
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_ensure_cursor_visible(ed);
}

inline void editor_page_down(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  const int delta = std::max(1, ed.last_body_h);
  ed.cursor_line = std::min(static_cast<int>(ed.lines.size()) - 1, ed.cursor_line + delta);
  const int line_len = static_cast<int>(ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
  ed.cursor_col = std::min(ed.cursor_col, line_len);
  ed.preferred_col = ed.cursor_col;
  editor_ensure_cursor_visible(ed);
}

inline void editor_insert_char(editorBox_data_t& ed, char ch) {
  ed.ensure_nonempty();
  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  line.insert(line.begin() + ed.cursor_col, ch);
  ++ed.cursor_col;
  ed.preferred_col = ed.cursor_col;
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
}

inline void editor_insert_text(editorBox_data_t& ed, std::string_view text) {
  for (const char ch : text) editor_insert_char(ed, ch);
}

inline void editor_insert_newline(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const std::string right = line.substr(static_cast<std::size_t>(ed.cursor_col));
  line.resize(static_cast<std::size_t>(ed.cursor_col));
  ed.lines.insert(ed.lines.begin() + ed.cursor_line + 1, right);
  ++ed.cursor_line;
  ed.cursor_col = 0;
  ed.preferred_col = ed.cursor_col;
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
}

inline void editor_backspace(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.cursor_col > 0) {
    std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    line.erase(line.begin() + (ed.cursor_col - 1));
    --ed.cursor_col;
    ed.preferred_col = ed.cursor_col;
    ed.dirty = true;
    editor_ensure_cursor_visible(ed);
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
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
}

inline void editor_delete(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  if (ed.cursor_col < static_cast<int>(line.size())) {
    line.erase(line.begin() + ed.cursor_col);
    ed.dirty = true;
    editor_ensure_cursor_visible(ed);
    return;
  }
  if (ed.cursor_line + 1 >= static_cast<int>(ed.lines.size())) return;
  line += ed.lines[static_cast<std::size_t>(ed.cursor_line + 1)];
  ed.lines.erase(ed.lines.begin() + ed.cursor_line + 1);
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
}

inline void editor_delete_to_eol(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  if (ed.cursor_col >= static_cast<int>(line.size())) return;
  line.erase(static_cast<std::size_t>(ed.cursor_col));
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
}

[[nodiscard]] inline bool editor_is_word_char(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_' || ch == '.' || ch == '@' || ch == ':' || ch == '-';
}

inline void editor_delete_prev_word(editorBox_data_t& ed) {
  ed.ensure_nonempty();
  if (ed.cursor_col == 0) {
    editor_backspace(ed);
    return;
  }
  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  int pos = ed.cursor_col;
  while (pos > 0 && std::isspace(static_cast<unsigned char>(line[static_cast<std::size_t>(pos - 1)]))) --pos;
  while (pos > 0 && editor_is_word_char(line[static_cast<std::size_t>(pos - 1)])) --pos;
  if (pos >= ed.cursor_col) return;
  line.erase(static_cast<std::size_t>(pos), static_cast<std::size_t>(ed.cursor_col - pos));
  ed.cursor_col = pos;
  ed.preferred_col = ed.cursor_col;
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
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

  std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  line.replace(static_cast<std::size_t>(start), prefix.size(), replacement);
  ed.cursor_col = start + static_cast<int>(replacement.size());
  ed.preferred_col = ed.cursor_col;
  ed.dirty = true;
  editor_ensure_cursor_visible(ed);
  out.changed = true;
  out.status = (matches.size() == 1) ? "completion accepted" : (std::to_string(matches.size()) + " matches");
  return out;
}

}  // namespace primitives
}  // namespace iinuji
}  // namespace cuwacunu
