#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/iinuji_ansi.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline void render_text(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  short pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);

  auto tb = std::dynamic_pointer_cast<textBox_data_t>(obj.data);
  if (!tb) return;

  auto pair_for_emphasis = [&](text_line_emphasis_t e) -> short {
    switch (e) {
      case text_line_emphasis_t::Accent:
        return (short)get_color_pair("#C89C3A", obj.style.background_color);
      case text_line_emphasis_t::Success:
        return (short)get_color_pair("#4D7A52", obj.style.background_color);
      case text_line_emphasis_t::Fatal:
        return (short)get_color_pair("#ff0000", obj.style.background_color);
      case text_line_emphasis_t::Error:
        return (short)get_color_pair("#c61c41", obj.style.background_color);
      case text_line_emphasis_t::Warning:
        return (short)get_color_pair("#C8922C", obj.style.background_color);
      case text_line_emphasis_t::Info:
        return (short)get_color_pair("#96989a", obj.style.background_color);
      case text_line_emphasis_t::Debug:
        return (short)get_color_pair("#3F86C7", obj.style.background_color);
      case text_line_emphasis_t::None:
        return pair;
    }
    return pair;
  };
  auto emphasis_is_bold = [](text_line_emphasis_t e) {
    return e == text_line_emphasis_t::Accent ||
           e == text_line_emphasis_t::Fatal ||
           e == text_line_emphasis_t::Error ||
           e == text_line_emphasis_t::Warning;
  };

  // Focused input caret rendering:
  // We treat any focused+focusable textBox as an input line (since labels are not focusable).
  if (obj.focused && obj.focusable) {
    // Single-line input behavior
    std::string line = tb->content;
    if (auto p = line.find('\n'); p != std::string::npos) line.resize(p);

    const int H = r.h;
    const int W = r.w;
    if (H <= 0 || W <= 0) return;

    // reserve last column for caret
    const int vis_w = std::max(0, W - 1);

    const std::size_t len = line.size();
    std::size_t start = 0;
    if (vis_w > 0 && len > (std::size_t)vis_w) start = len - (std::size_t)vis_w;

    std::string shown = (vis_w > 0) ? line.substr(start, (std::size_t)vis_w) : std::string();

    // draw visible text
    if (vis_w > 0) {
      R->putText(r.y, r.x, shown, vis_w, pair, obj.style.bold, obj.style.inverse);
    }

    // caret position (after last visible char)
    std::size_t caret_off = len - start;
    if (caret_off > (std::size_t)vis_w) caret_off = (std::size_t)vis_w;
    int cx = r.x + (int)caret_off;

    // draw caret (inverse to make it obvious)
    if (cx >= r.x && cx < r.x + W) {
      R->putText(r.y, cx, "|", 1, pair, /*bold*/true, /*inverse*/true);
    }

    return; // do not fall through to multiline wrap rendering
  }

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  // Multiline support:
  // - if wrap=true  : wrap each newline-separated line
  // - if wrap=false : still split on '\n' so multiline labels render correctly
  // ANSI-aware path (for dlogs-like output)
  if (ansi::has_esc(tb->content)) {
    ansi::style_t base;
    base.fg = obj.style.label_color;
    base.bg = obj.style.background_color;
    base.bold = obj.style.bold;
    base.inverse = obj.style.inverse;
    base.dim = false;

    auto phys = split_lines_keep_empty(tb->content);
    int y = r.y;
    for (const auto& pl : phys) {
      if (y >= r.y + r.h) break;
      std::vector<ansi::row_t> rows;
      ansi::hard_wrap(pl, std::max(1, r.w), base, pair, rows);

      for (const auto& row : rows) {
        if (y >= r.y + r.h) break;
        int colx = r.x;
        if (tb->align == text_align_t::Center) colx = r.x + std::max(0, (r.w - row.len)/2);
        else if (tb->align == text_align_t::Right) colx = r.x + std::max(0, r.w - row.len);

        ansi::render_row(y, colx, std::max(0, r.w - (colx - r.x)), row, pair, obj.style.bold, obj.style.inverse);
        ++y;

        if (!tb->wrap) break; // nowrap => only render first wrapped row
      }
    }
    return;
  }

  // non-ANSI path with scrollable viewport + scrollbars
  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = W;
  int text_h = H;
  int max_line_len = 0;
  std::vector<std::string> lines;
  std::vector<text_line_emphasis_t> line_emphasis;

  auto build_text_lines = [&](int width) {
    lines.clear();
    line_emphasis.clear();
    const int safe_w = std::max(1, width);

    if (!tb->styled_lines.empty()) {
      for (const auto& line : tb->styled_lines) {
        const auto chunks = tb->wrap
            ? wrap_text(line.text, safe_w)
            : split_lines_keep_empty(line.text);
        if (chunks.empty()) {
          lines.emplace_back();
          line_emphasis.push_back(line.emphasis);
          continue;
        }
        for (const auto& chunk : chunks) {
          lines.push_back(chunk);
          line_emphasis.push_back(line.emphasis);
          if (!tb->wrap) break;
        }
      }
    } else {
      lines = tb->wrap
          ? wrap_text(tb->content, safe_w)
          : split_lines_keep_empty(tb->content);
      line_emphasis.assign(lines.size(), text_line_emphasis_t::None);
    }
  };

  // Resolve scrollbar reservations (vertical + horizontal) with a few stable iterations.
  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0) return;

    build_text_lines(text_w);

    max_line_len = 0;
    for (const auto& ln : lines) max_line_len = std::max(max_line_len, (int)ln.size());

    const bool need_h = (!tb->wrap && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, H - reserve_h_new);
    const bool need_v = ((int)lines.size() > text_h_if);
    const int reserve_v_new = need_v ? 1 : 0;

    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v) break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, W - reserve_v);
  text_h = std::max(0, H - reserve_h);
  if (text_w <= 0 || text_h <= 0) return;

  build_text_lines(text_w);

  max_line_len = 0;
  for (const auto& ln : lines) max_line_len = std::max(max_line_len, (int)ln.size());

  const int max_scroll_y = std::max(0, (int)lines.size() - text_h);
  const int max_scroll_x = tb->wrap ? 0 : std::max(0, max_line_len - text_w);
  tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
  tb->scroll_x = std::clamp(tb->scroll_x, 0, max_scroll_x);

  for (int row = 0; row < text_h; ++row) {
    const int li = tb->scroll_y + row;
    if (li < 0 || li >= (int)lines.size()) break;

    const std::string& line = lines[(std::size_t)li];
    const auto emphasis = (li < (int)line_emphasis.size())
        ? line_emphasis[(std::size_t)li]
        : text_line_emphasis_t::None;
    auto line_pair = pair_for_emphasis(emphasis);
    if (line_pair == 0) line_pair = pair;
    const bool line_bold = emphasis_is_bold(emphasis);
    const int visible_len = (int)line.size();
    int colx = r.x;
    if (tb->scroll_x == 0 && reserve_v == 0) {
      if (tb->align == text_align_t::Center) colx = r.x + std::max(0, (text_w - visible_len) / 2);
      else if (tb->align == text_align_t::Right) colx = r.x + std::max(0, text_w - visible_len);
    }

    if (!tb->wrap) {
      const int skip = tb->scroll_x;
      if (skip >= (int)line.size()) continue;
      const int take = std::min(text_w, (int)line.size() - skip);
      if (take <= 0) continue;
      R->putText(
          r.y + row,
          colx,
          line.substr((std::size_t)skip, (std::size_t)take),
          take,
          line_pair,
          (obj.style.bold || line_bold),
          obj.style.inverse);
    } else {
      const int take = std::min(text_w, (int)line.size());
      if (take <= 0) continue;
      R->putText(
          r.y + row,
          colx,
          line.substr(0, (std::size_t)take),
          take,
          line_pair,
          (obj.style.bold || line_bold),
          obj.style.inverse);
    }
  }

  short bar_pair = (short)get_color_pair(obj.style.border_color, obj.style.background_color);
  if (bar_pair == 0) bar_pair = pair;

  // Vertical scrollbar
  if (reserve_v > 0 && text_h > 0) {
    const int bar_x = r.x + text_w;
    for (int i = 0; i < text_h; ++i) {
      R->putGlyph(r.y + i, bar_x, L'│', bar_pair);
    }

    const int total_rows = std::max(1, (int)lines.size());
    int thumb_h = (int)std::lround((double)text_h * (double)text_h / (double)total_rows);
    thumb_h = std::clamp(thumb_h, 1, text_h);
    const int thumb_span = std::max(0, text_h - thumb_h);
    const int thumb_y =
        (max_scroll_y > 0) ? (int)std::lround((double)tb->scroll_y * (double)thumb_span / (double)max_scroll_y) : 0;

    for (int i = 0; i < thumb_h; ++i) {
      R->putGlyph(r.y + thumb_y + i, bar_x, L'█', bar_pair);
    }
  }

  // Horizontal scrollbar
  if (reserve_h > 0 && text_w > 0) {
    const int bar_y = r.y + text_h;
    for (int i = 0; i < text_w; ++i) {
      R->putGlyph(bar_y, r.x + i, L'─', bar_pair);
    }

    const int total_cols = std::max(1, max_line_len);
    int thumb_w = (int)std::lround((double)text_w * (double)text_w / (double)total_cols);
    thumb_w = std::clamp(thumb_w, 1, text_w);
    const int thumb_span = std::max(0, text_w - thumb_w);
    const int thumb_x =
        (max_scroll_x > 0) ? (int)std::lround((double)tb->scroll_x * (double)thumb_span / (double)max_scroll_x) : 0;

    for (int i = 0; i < thumb_w; ++i) {
      R->putGlyph(bar_y, r.x + thumb_x + i, L'█', bar_pair);
    }

    if (reserve_v > 0) {
      R->putGlyph(bar_y, r.x + text_w, L'┘', bar_pair);
    }
  }
}

} // namespace iinuji
} // namespace cuwacunu
