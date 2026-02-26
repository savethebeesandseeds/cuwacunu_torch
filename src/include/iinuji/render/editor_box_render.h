#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline int digits10_i(int v) {
  if (v < 0) v = -v;
  int d = 1;
  while (v >= 10) { v /= 10; ++d; }
  return d;
}

inline void render_editor(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  short base_pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, base_pair);

  auto ed = std::dynamic_pointer_cast<editorBox_data_t>(obj.data);
  if (!ed) return;
  ed->ensure_nonempty();

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  // Header row
  {
    std::string file = ed->path.empty() ? std::string("<new file>") : ed->path;
    std::string left;
    if (ed->dirty) left += "* ";
    if (ed->read_only) left += "[RO] ";
    left += file;

    char buf[96];
    std::snprintf(buf, sizeof(buf), "Ln %d, Col %d", ed->cursor_line + 1, ed->cursor_col + 1);
    std::string right = buf;
    if (!ed->status.empty()) {
      right += " | ";
      right += ed->status;
    }

    std::string header((size_t)W, ' ');
    for (int i=0; i<W && i<(int)left.size(); ++i) header[(size_t)i] = left[(size_t)i];
    if (!right.empty()) {
      if ((int)right.size() > W) right = right.substr((size_t)right.size() - (size_t)W);
      int rx = std::max(0, W - (int)right.size());
      for (int i=0; i<(int)right.size() && (rx+i)<W; ++i) header[(size_t)(rx+i)] = right[(size_t)i];
    }

    R->putText(r.y, r.x, header, W, base_pair, /*bold*/true, /*inverse*/false);
  }

  if (H == 1) {
    ed->last_body_h = 0;
    ed->last_lineno_w = 0;
    ed->last_text_w = 0;
    return;
  }

  // Body
  const int body_y = r.y + 1;
  const int body_h = std::max(0, H - 1);
  const int total_lines = std::max(1, (int)ed->lines.size());
  int digits = digits10_i(total_lines);

  // "%*d |" => digits + 2 columns
  int ln_w = std::min(W, digits + 2);
  if (ln_w < 3) ln_w = std::min(W, 3);
  const int text_w = std::max(0, W - ln_w);

  ed->last_body_h = body_h;
  ed->last_lineno_w = ln_w;
  ed->last_text_w = text_w;

  if (ed->top_line < 0) ed->top_line = 0;
  if (ed->top_line > total_lines - 1) ed->top_line = total_lines - 1;
  if (ed->left_col < 0) ed->left_col = 0;

  short ln_pair = (short)get_color_pair(obj.style.border_color, obj.style.background_color);
  if (ln_pair == 0) ln_pair = base_pair;

  for (int row=0; row<body_h; ++row) {
    int li = ed->top_line + row;
    if (li < 0 || li >= (int)ed->lines.size()) break;

    std::string num = std::to_string(li + 1);

    int width = digits;
    if (width < 1) width = 1;
    if (width > 32) width = 32;

    std::string prefix;
    if ((int)num.size() < width) prefix.append(width - (int)num.size(), ' ');
    prefix += num;
    prefix += " |";

    std::string gutter = prefix;
    if ((int)gutter.size() > ln_w) gutter.resize((size_t)ln_w);
    else if ((int)gutter.size() < ln_w) gutter.append((size_t)(ln_w - (int)gutter.size()), ' ');
    R->putText(body_y + row, r.x, gutter, ln_w, ln_pair);

    std::string shown;
    const std::string& line = ed->lines[(size_t)li];
    if (text_w > 0 && ed->left_col < (int)line.size()) {
      shown = line.substr((size_t)ed->left_col, (size_t)text_w);
    }

    if (shown.empty() || !ed->line_colorizer) {
      R->putText(body_y + row, r.x + ln_w, shown, text_w, base_pair);
    } else {
      std::vector<short> colors;
      ed->line_colorizer(*ed, li, line, colors, base_pair, obj.style.background_color);
      if ((int)colors.size() < (int)line.size()) colors.resize(line.size(), base_pair);

      const int vis_start = std::max(0, ed->left_col);
      const int vis_end = std::min((int)line.size(), vis_start + text_w);
      int xoff = 0;
      int i = vis_start;
      while (i < vis_end) {
        const short p = colors[(std::size_t)i];
        int j = i + 1;
        while (j < vis_end && colors[(std::size_t)j] == p) ++j;
        const int n = j - i;
        if (n > 0) {
          R->putText(
              body_y + row,
              r.x + ln_w + xoff,
              line.substr((std::size_t)i, (std::size_t)n),
              n,
              p,
              false,
              false);
          xoff += n;
        }
        i = j;
      }
    }
  }

  if (W > 0 && body_h > 0) {
    if (ed->top_line > 0) R->putGlyph(body_y, r.x + (W - 1), L'↑', base_pair);
    if (ed->top_line + body_h < (int)ed->lines.size())
      R->putGlyph(body_y + (body_h - 1), r.x + (W - 1), L'↓', base_pair);
    if (ed->left_col > 0) R->putGlyph(r.y, r.x + (W - 1), L'←', base_pair);
  }

  // caret
  if (obj.focused && obj.focusable && body_h > 0 && text_w > 0) {
    int crow = ed->cursor_line - ed->top_line;
    int ccol = ed->cursor_col - ed->left_col;
    if (crow >= 0 && crow < body_h) {
      int cx = r.x + ln_w + std::clamp(ccol, 0, std::max(0, text_w - 1));
      int cy = body_y + crow;
      if (cx >= r.x + ln_w && cx < r.x + W) {
        R->putText(cy, cx, "|", 1, base_pair, /*bold*/true, /*inverse*/true);
      }
    }
  }
}

} // namespace iinuji
} // namespace cuwacunu
