#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/view.styles.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void put_canvas_char(std::vector<std::string>& canvas,
                            std::vector<std::vector<circuit_draw_style_t>>& styles,
                            int x,
                            int y,
                            char ch,
                            circuit_draw_style_t style = circuit_draw_style_t::Default) {
  if (y < 0 || y >= (int)canvas.size()) return;
  if (x < 0 || x >= (int)canvas[(std::size_t)y].size()) return;

  char& cell = canvas[(std::size_t)y][(std::size_t)x];
  auto& style_cell = styles[(std::size_t)y][(std::size_t)x];
  if (cell == ' ' || cell == ch) {
    cell = ch;
    style_cell = merge_draw_style(style_cell, style);
    return;
  }

  const bool old_h = (cell == '-' || cell == '>' || cell == '<');
  const bool old_v = (cell == '|' || cell == '^' || cell == 'v');
  const bool new_h = (ch == '-' || ch == '>' || ch == '<');
  const bool new_v = (ch == '|' || ch == '^' || ch == 'v');
  if ((old_h && new_v) || (old_v && new_h) || cell == '+' || ch == '+') {
    cell = '+';
    style_cell = merge_draw_style(style_cell, style);
    return;
  }
  if (ch == '>' || ch == '<' || ch == '^' || ch == 'v') {
    cell = ch;
    style_cell = merge_draw_style(style_cell, style);
  }
}

inline void draw_hline(std::vector<std::string>& canvas,
                       std::vector<std::vector<circuit_draw_style_t>>& styles,
                       int x0,
                       int x1,
                       int y,
                       char ch = '-',
                       circuit_draw_style_t style = circuit_draw_style_t::Default) {
  if (x0 > x1) std::swap(x0, x1);
  for (int x = x0; x <= x1; ++x) put_canvas_char(canvas, styles, x, y, ch, style);
}

inline void draw_vline(std::vector<std::string>& canvas,
                       std::vector<std::vector<circuit_draw_style_t>>& styles,
                       int x,
                       int y0,
                       int y1,
                       char ch = '|',
                       circuit_draw_style_t style = circuit_draw_style_t::Default) {
  if (y0 > y1) std::swap(y0, y1);
  for (int y = y0; y <= y1; ++y) put_canvas_char(canvas, styles, x, y, ch, style);
}

inline void draw_text(std::vector<std::string>& canvas,
                      std::vector<std::vector<circuit_draw_style_t>>& styles,
                      int x,
                      int y,
                      const std::string& text,
                      circuit_draw_style_t style = circuit_draw_style_t::Default) {
  if (y < 0 || y >= (int)canvas.size()) return;
  if (x < 0 || x >= (int)canvas[(std::size_t)y].size()) return;
  std::string& row = canvas[(std::size_t)y];
  for (int i = 0; i < (int)text.size() && (x + i) < (int)row.size(); ++i) {
    row[(std::size_t)(x + i)] = text[(std::size_t)i];
    styles[(std::size_t)y][(std::size_t)(x + i)] =
        merge_draw_style(styles[(std::size_t)y][(std::size_t)(x + i)], style);
  }
}

inline void draw_box(std::vector<std::string>& canvas,
                     std::vector<std::vector<circuit_draw_style_t>>& styles,
                     int x,
                     int y,
                     int w,
                     const std::string& line1,
                     const std::string& line2,
                     circuit_draw_style_t border_style,
                     circuit_draw_style_t line1_style,
                     circuit_draw_style_t line2_style) {
  if (w < 4) return;
  draw_hline(canvas, styles, x, x + w - 1, y, '-', border_style);
  draw_hline(canvas, styles, x, x + w - 1, y + 3, '-', border_style);
  draw_vline(canvas, styles, x, y, y + 3, '|', border_style);
  draw_vline(canvas, styles, x + w - 1, y, y + 3, '|', border_style);
  put_canvas_char(canvas, styles, x, y, '+', border_style);
  put_canvas_char(canvas, styles, x + w - 1, y, '+', border_style);
  put_canvas_char(canvas, styles, x, y + 3, '+', border_style);
  put_canvas_char(canvas, styles, x + w - 1, y + 3, '+', border_style);

  draw_text(canvas, styles, x + 1, y + 1, trim_to_width(line1, w - 2), line1_style);
  draw_text(canvas, styles, x + 1, y + 2, trim_to_width(line2, w - 2), line2_style);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
