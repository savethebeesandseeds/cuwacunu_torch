#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string_view trim_board_line_view(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
  return s;
}

inline int board_find_circuit_header_line(const cuwacunu::iinuji::editorBox_data_t& ed, int circuit_index) {
  if (circuit_index < 0) return -1;
  int seen = 0;
  for (int i = 0; i < static_cast<int>(ed.lines.size()); ++i) {
    const std::string_view line = trim_board_line_view(ed.lines[static_cast<std::size_t>(i)]);
    if (line.empty()) continue;
    if (line.find("= {") == std::string_view::npos && line.find("={") == std::string_view::npos) continue;
    if (seen == circuit_index) return i;
    ++seen;
  }
  return -1;
}

inline std::vector<std::string> board_declared_aliases(const cuwacunu::iinuji::editorBox_data_t& ed) {
  std::vector<std::string> aliases;
  aliases.reserve(ed.lines.size());
  for (const auto& raw : ed.lines) {
    std::string_view line = trim_board_line_view(raw);
    if (line.empty()) continue;
    if (line.find("->") != std::string_view::npos) continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    std::string alias = std::string(trim_board_line_view(line.substr(0, eq)));
    if (!alias.empty()) aliases.push_back(std::move(alias));
  }
  std::sort(aliases.begin(), aliases.end());
  aliases.erase(std::unique(aliases.begin(), aliases.end()), aliases.end());
  return aliases;
}

inline int guess_board_error_line(const cuwacunu::iinuji::editorBox_data_t& ed,
                                  std::string_view error_text,
                                  int selected_circuit_index) {
  if (ed.lines.empty()) return 0;

  const std::size_t circuit_at = error_text.find("circuit[");
  if (circuit_at != std::string_view::npos) {
    std::size_t p = circuit_at + 8;
    int idx = 0;
    bool has_digit = false;
    while (p < error_text.size() && std::isdigit(static_cast<unsigned char>(error_text[p]))) {
      has_digit = true;
      idx = idx * 10 + static_cast<int>(error_text[p] - '0');
      ++p;
    }
    if (has_digit) {
      const int line = board_find_circuit_header_line(ed, idx);
      if (line >= 0) return line;
    }
  }

  const auto aliases = board_declared_aliases(ed);
  std::vector<std::string> hits;
  hits.reserve(aliases.size());
  for (const auto& alias : aliases) {
    if (error_text.find(alias) != std::string_view::npos) hits.push_back(alias);
  }

  auto find_line_with = [&](const std::vector<std::string>& needles,
                            bool require_hop,
                            bool require_decl) -> int {
    for (int i = 0; i < static_cast<int>(ed.lines.size()); ++i) {
      const std::string& line = ed.lines[static_cast<std::size_t>(i)];
      if (require_hop && line.find("->") == std::string::npos) continue;
      if (require_decl && line.find('=') == std::string::npos) continue;
      bool ok = true;
      for (const auto& n : needles) {
        if (line.find(n) == std::string::npos) {
          ok = false;
          break;
        }
      }
      if (ok) return i;
    }
    return -1;
  };

  if (hits.size() >= 2u) {
    const int hop_line = find_line_with({hits[0], hits[1]}, true, false);
    if (hop_line >= 0) return hop_line;
  }
  if (!hits.empty()) {
    if (error_text.find("hop") != std::string_view::npos) {
      const int hop_line = find_line_with({hits[0]}, true, false);
      if (hop_line >= 0) return hop_line;
    }
    const int decl_line = find_line_with({hits[0]}, false, true);
    if (decl_line >= 0) return decl_line;
    const int any_line = find_line_with({hits[0]}, false, false);
    if (any_line >= 0) return any_line;
  }

  const int selected_header = board_find_circuit_header_line(ed, selected_circuit_index);
  if (selected_header >= 0) return selected_header;
  return std::clamp(ed.cursor_line, 0, static_cast<int>(ed.lines.size()) - 1);
}

inline std::pair<int, int> parse_board_error_line_col(std::string_view error_text) {
  auto parse_after = [&](std::string_view key) -> int {
    const std::size_t at = error_text.find(key);
    if (at == std::string_view::npos) return -1;
    std::size_t p = at + key.size();
    while (p < error_text.size() && !std::isdigit(static_cast<unsigned char>(error_text[p]))) ++p;
    if (p >= error_text.size()) return -1;
    int value = 0;
    bool any = false;
    while (p < error_text.size() && std::isdigit(static_cast<unsigned char>(error_text[p]))) {
      any = true;
      value = value * 10 + static_cast<int>(error_text[p] - '0');
      ++p;
    }
    return any ? value : -1;
  };

  int line = parse_after("line ");
  if (line < 0) line = parse_after("line:");
  int col = parse_after("column ");
  if (col < 0) col = parse_after("column:");
  return {line, col};
}

inline void refresh_board_editor_diagnostic(CmdState& st) {
  st.board.diagnostic_active = false;
  st.board.diagnostic_line = -1;
  st.board.diagnostic_col = -1;
  st.board.diagnostic_message.clear();

  if (st.screen != ScreenMode::Board || !st.board.editor_focus || !st.board.editor) return;
  if (st.board.editor_scope == BoardEditorScope::FullInstruction ||
      st.board.editor_scope == BoardEditorScope::ContractSection) {
    return;
  }
  const auto& ed = *st.board.editor;
  if (ed.lines.empty()) return;

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t parsed{};
  std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved{};
  std::string error;
  const std::string text = cuwacunu::iinuji::primitives::editor_text(ed);
  if (decode_board_instruction_text(text, &parsed, &resolved, &error)) return;

  st.board.diagnostic_active = true;
  const auto [line_from_error, col_from_error] = parse_board_error_line_col(error);
  if (line_from_error > 0) st.board.diagnostic_line = line_from_error - 1;
  else st.board.diagnostic_line = guess_board_error_line(ed, error, static_cast<int>(st.board.selected_circuit));
  if (col_from_error > 0) st.board.diagnostic_col = col_from_error - 1;

  for (char& ch : error) {
    if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
  }
  while (error.find("  ") != std::string::npos) {
    error.erase(error.find("  "), 1u);
  }
  if (error.size() > 140u) {
    error.resize(137u);
    error += "...";
  }
  st.board.diagnostic_message = std::move(error);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
