#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_cmd/views/board/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void render_board_completion_overlay(
    const CmdState& st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left) {
  if (st.screen != ScreenMode::Board) return;
  if (!st.board.editor_focus || !st.board.editor) return;
  if (!st.board.completion_active || st.board.completion_items.empty()) return;

  auto* R = get_renderer();
  if (!R) return;

  const Rect r = content_rect(*left);
  const auto& ed = *st.board.editor;
  const int body_y = r.y + 1;
  const int body_h = std::max(0, r.h - 1);
  if (body_h <= 0 || r.w <= 0) return;

  const int crow = ed.cursor_line - ed.top_line;
  const int ccol = ed.cursor_col - ed.left_col;
  int anchor_y = body_y + std::max(0, crow);
  int anchor_x = r.x + std::max(0, ed.last_lineno_w) + std::max(0, ccol);

  const int n_total = static_cast<int>(st.board.completion_items.size());
  const int n_show = std::min(6, n_total);
  int max_item_w = 12;
  for (const auto& item : st.board.completion_items) {
    max_item_w = std::max(max_item_w, static_cast<int>(item.size()) + 4);
  }
  int popup_w = std::min(std::max(8, max_item_w), std::max(8, r.w));

  if (anchor_x + popup_w > r.x + r.w) anchor_x = std::max(r.x, r.x + r.w - popup_w);
  if (anchor_y + n_show > body_y + body_h) anchor_y = std::max(body_y, body_y + body_h - n_show);

  const int selected = std::clamp(static_cast<int>(st.board.completion_index), 0, n_total - 1);
  const int page_start = (selected / n_show) * n_show;
  const short pair = static_cast<short>(get_color_pair("#88888f", left->style.background_color));
  const short sel_pair = static_cast<short>(get_color_pair("#b8b8c0", left->style.background_color));

  for (int row = 0; row < n_show; ++row) {
    const int idx = page_start + row;
    if (idx >= n_total) break;
    const bool is_sel = (idx == selected);
    std::string line = (is_sel ? "(o) " : "( ) ");
    line += st.board.completion_items[static_cast<std::size_t>(idx)];
    if (static_cast<int>(line.size()) > popup_w) line.resize(static_cast<std::size_t>(popup_w));
    if (static_cast<int>(line.size()) < popup_w) line.append(static_cast<std::size_t>(popup_w - line.size()), ' ');
    R->putText(anchor_y + row, anchor_x, line, popup_w, is_sel ? sel_pair : pair, false, false);
  }
}

inline void render_board_error_line_overlay(
    const CmdState& st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left) {
  if (st.screen != ScreenMode::Board) return;
  if (!st.board.editor_focus || !st.board.editor) return;
  if (!st.board.diagnostic_active) return;

  auto* R = get_renderer();
  if (!R) return;

  const Rect r = content_rect(*left);
  const auto& ed = *st.board.editor;
  const int body_y = r.y + 1;
  const int body_h = std::max(0, r.h - 1);
  if (body_h <= 0 || r.w <= 0) return;

  int line_index = st.board.diagnostic_line;
  if (line_index < 0 || line_index >= static_cast<int>(ed.lines.size())) {
    line_index = std::clamp(ed.cursor_line, 0, static_cast<int>(ed.lines.size()) - 1);
  }
  const int row = line_index - ed.top_line;
  if (row < 0 || row >= body_h) return;

  const int total_lines = std::max(1, static_cast<int>(ed.lines.size()));
  int digits = digits10_i(total_lines);
  int ln_w = std::min(r.w, digits + 2);
  if (ln_w < 3) ln_w = std::min(r.w, 3);
  if (ln_w <= 0) return;

  const std::string num = std::to_string(line_index + 1);
  int width = digits;
  if (width < 1) width = 1;
  if (width > 32) width = 32;
  std::string prefix;
  if (static_cast<int>(num.size()) < width) prefix.append(width - static_cast<int>(num.size()), ' ');
  prefix += num;
  prefix += " |";

  std::string gutter = prefix;
  if (static_cast<int>(gutter.size()) > ln_w) gutter.resize(static_cast<std::size_t>(ln_w));
  else if (static_cast<int>(gutter.size()) < ln_w) {
    gutter.append(static_cast<std::size_t>(ln_w - gutter.size()), ' ');
  }
  if (!gutter.empty()) gutter[0] = '!';

  const short pair = static_cast<short>(get_color_pair("#c38e8e", left->style.background_color));
  R->putText(body_y + row, r.x, gutter, ln_w, pair, true, false);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
