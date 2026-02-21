/* iinuji_render.h */
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cwchar>
#include <cstdio>

#include "iinuji/iinuji_rend.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_plot.h"
#include "iinuji/iinuji_ansi.h"

namespace cuwacunu {
namespace iinuji {

/* -------------------- Helpers -------------------- */
inline Rect inset_rect(Rect r, int l,int t,int rgt,int btm){
  return Rect{ r.x + l, r.y + t, std::max(0, r.w - (l + rgt)), std::max(0, r.h - (t + btm)) };
}

/* Compute a container's content rect (after border + padding) */
inline Rect content_rect(const iinuji_object_t& o) {
  Rect r = o.screen;
  // Reserve a 1-cell "frame" when either:
  //  - border is enabled, or
  //  - the object is focused + focusable (focus frame even if border hidden)
  bool want_frame = (o.style.border || (o.focused && o.focusable));

  // If the widget is too small, don't inset (otherwise content becomes 0x0).
  if (want_frame && r.w >= 3 && r.h >= 3) {
    r = inset_rect(r, 1,1,1,1);
  }
  r = inset_rect(r, o.layout.pad_left, o.layout.pad_top, o.layout.pad_right, o.layout.pad_bottom);
  return r;
}

inline void render_focus_frame_bg(const iinuji_object_t& obj) {
  // Only for focused, focusable, borderless widgets: draw a background-only frame (no lines).
  if (!(obj.focused && obj.focusable)) return;
  if (obj.style.border) return;
  
  auto* R = get_renderer();
  if (!R) return;
  
  const Rect s = obj.screen;
  const int w = s.w, h = s.h;
  if (w <= 0 || h <= 0) return;
  if (w <  3 || h <  3) return;

  constexpr double kFocusDarken = 0.8; // 20% darker
  const std::string bg = focus_darken_bg_token(obj.style.background_color, kFocusDarken);

  // Frame uses (darkened) border color as fg if available; otherwise label color.
  std::string fg = obj.style.border_color;
  if (is_unset_color_token(fg)) fg = obj.style.label_color;
  fg = focus_darken_fg_token(fg, kFocusDarken);

  short pair = (short)get_color_pair(fg, bg);
  if (pair == 0) pair = (short)get_color_pair("white", bg);

  // top
  R->fillRect(s.y, s.x, 1, w, pair);
  // bottom
  if (h > 1) R->fillRect(s.y + h - 1, s.x, 1, w, pair);
  // sides
  if (h > 2) {
    R->fillRect(s.y + 1, s.x, h - 2, 1, pair);
    if (w > 1) R->fillRect(s.y + 1, s.x + w - 1, h - 2, 1, pair);
  }
}

/* -------------------- Track resolution (grid) -------------------- */
inline std::vector<int> resolve_tracks(const std::vector<len_spec>& defs, int total_px, int gap, int pad_a, int pad_b) {
  int n = (int)defs.size();
  std::vector<int> out(n,0);
  if (n==0 || total_px <= 0) return out;
  int available = total_px - pad_a - pad_b - (n>0 ? gap*(n-1) : 0);
  if (available < 0) available = 0;

  int fixed = 0; double frac_sum = 0.0;
  for (auto& d : defs) {
    if (d.u == unit_t::Px) fixed += (int)std::max(0.0, d.v);
    else frac_sum += std::max(0.0, d.v);
  }
  int rem = std::max(0, available - fixed);
  for (int i=0;i<n;++i) {
    if (defs[i].u == unit_t::Px) out[i] = (int)std::max(0.0, defs[i].v);
    else out[i] = (frac_sum > 0 ? (int)std::round(rem * (defs[i].v/frac_sum)) : 0);
  }
  int sum=0; for (int v:out) sum+=v;
  int diff = available - sum;
  for (int i=0; i<n && diff!=0; ++i) { int d = diff>0?1:-1; out[i]+=d; diff-=d; }
  return out;
}

/* -------------------- Layout engine -------------------- */
inline void layout_tree(const std::shared_ptr<iinuji_object_t>& node, const Rect& rect) {
  if (!node || !node->visible) return;
  node->screen = rect;

  Rect inner = content_rect(*node);

  if (node->grid) {
    auto rows = resolve_tracks(node->grid->rows, inner.h, node->grid->gap_row,
                               node->grid->pad_top, node->grid->pad_bottom);
    auto cols = resolve_tracks(node->grid->cols, inner.w, node->grid->gap_col,
                               node->grid->pad_left, node->grid->pad_right);

    std::vector<int> row_y(rows.size(),0), col_x(cols.size(),0);
    int y = inner.y + node->grid->pad_top;
    for (size_t r=0;r<rows.size();++r){ row_y[r]=y; y += rows[r] + (int)(r+1<rows.size()?node->grid->gap_row:0); }
    int x = inner.x + node->grid->pad_left;
    for (size_t c=0;c<cols.size();++c){ col_x[c]=x; x += cols[c] + (int)(c+1<cols.size()?node->grid->gap_col:0); }

    for (auto& ch : node->children) {
      if (!ch->visible) continue;
      if (ch->layout.mode == layout_mode_t::GridCell) {
        int r = std::clamp(ch->layout.grid_row, 0, (int)rows.size()-1);
        int c = std::clamp(ch->layout.grid_col, 0, (int)cols.size()-1);
        int rs = std::max(1, ch->layout.grid_row_span);
        int cs = std::max(1, ch->layout.grid_col_span);
        int r_last = std::clamp(r+rs-1, r, (int)rows.size()-1);
        int c_last = std::clamp(c+cs-1, c, (int)cols.size()-1);

        int cx = col_x[c];
        int cy = row_y[r];
        int cw = (col_x[c_last] - cx) + cols[c_last] + node->grid->gap_col*(c_last-c);
        int chh= (row_y[r_last] - cy) + rows[r_last] + node->grid->gap_row*(r_last-r);

        layout_tree(ch, Rect{cx, cy, cw, chh});
      }
    }
  }

  Rect freeR = inner;
  for (auto& ch : node->children) {
    if (!ch->visible) continue;
    if (ch->layout.mode != layout_mode_t::Dock) continue;

    switch (ch->layout.dock) {
      case dock_t::Top: {
        int h = (ch->layout.dock_size.u==unit_t::Px)
                ? (int)ch->layout.dock_size.v
                : (int)std::round(freeR.h * ch->layout.dock_size.v);
        layout_tree(ch, Rect{freeR.x, freeR.y, freeR.w, std::max(0,h)});
        freeR.y += h; freeR.h -= h; break;
      }
      case dock_t::Bottom: {
        int h = (ch->layout.dock_size.u==unit_t::Px)
                ? (int)ch->layout.dock_size.v
                : (int)std::round(freeR.h * ch->layout.dock_size.v);
        layout_tree(ch, Rect{freeR.x, freeR.y + freeR.h - std::max(0,h), freeR.w, std::max(0,h)});
        freeR.h -= h; break;
      }
      case dock_t::Left: {
        int w = (ch->layout.dock_size.u==unit_t::Px)
                ? (int)ch->layout.dock_size.v
                : (int)std::round(freeR.w * ch->layout.dock_size.v);
        layout_tree(ch, Rect{freeR.x, freeR.y, std::max(0,w), freeR.h});
        freeR.x += w; freeR.w -= w; break;
      }
      case dock_t::Right: {
        int w = (ch->layout.dock_size.u==unit_t::Px)
                ? (int)ch->layout.dock_size.v
                : (int)std::round(freeR.w * ch->layout.dock_size.v);
        layout_tree(ch, Rect{freeR.x + freeR.w - std::max(0,w), freeR.y, std::max(0,w), freeR.h});
        freeR.w -= w; break;
      }
      case dock_t::Fill:
      case dock_t::None:
      default: break;
    }
  }
  for (auto& ch : node->children) {
    if (!ch->visible) continue;
    if (ch->layout.mode == layout_mode_t::Dock && ch->layout.dock == dock_t::Fill) {
      layout_tree(ch, freeR);
    }
  }

  for (auto& ch : node->children) {
    if (!ch->visible) continue;
    if (ch->layout.mode == layout_mode_t::Absolute || ch->layout.mode == layout_mode_t::Normalized) {
      if (ch->layout.normalized) {
        int x0 = inner.x + (int)std::lround(ch->layout.x * inner.w);
        int y0 = inner.y + (int)std::lround(ch->layout.y * inner.h);
        int x1 = inner.x + (int)std::lround((ch->layout.x + ch->layout.width) * inner.w);
        int y1 = inner.y + (int)std::lround((ch->layout.y + ch->layout.height) * inner.h);
        layout_tree(ch, Rect{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)});
      } else {
        int px = inner.x + (int)ch->layout.x;
        int py = inner.y + (int)ch->layout.y;
        int pw = (int)ch->layout.width;
        int ph = (int)ch->layout.height;
        layout_tree(ch, Rect{px, py, pw, ph});
      }
    }
  }
}

/* -------------------- Picking (topmost) -------------------- */
inline bool pt_in_rect(const Rect& r, int x, int y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

inline std::shared_ptr<iinuji_object_t> pick_topmost(const std::shared_ptr<iinuji_object_t>& node, int x, int y) {
  if (!node || !node->visible) return nullptr;
  if (!pt_in_rect(node->screen, x, y)) return nullptr;
  std::shared_ptr<iinuji_object_t> best = node;
  for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
    auto& ch = *it;
    if (!ch->visible) continue;
    auto got = pick_topmost(ch, x, y);
    if (got) {
      if (!best || got->z_index >= best->z_index) best = got;
    }
  }
  return best;
}

/* -------------------- Rendering helpers via IRend -------------------- */
inline void render_border(const iinuji_object_t& obj) {
  if (!obj.style.border) return;
  auto* R = get_renderer();
  if (!R) return;

  const int x = obj.screen.x, y = obj.screen.y;
  const int w = obj.screen.w, h = obj.screen.h;
  if (w <= 0 || h <= 0) return;

  constexpr double kFocusDarken = 0.8; // 20% darker

  std::string fg = obj.style.border_color;
  std::string bg = obj.style.background_color;

  if (obj.focused && obj.focusable) {
    if (is_unset_color_token(fg)) fg = obj.style.label_color;
    fg = focus_darken_fg_token(fg, kFocusDarken);
    // If bg is terminal-default (<empty>), keep it (can't darken reliably).
    if (!is_unset_color_token(bg)) bg = darken_color_token(bg, kFocusDarken);
  }

  short pair = (short)get_color_pair(fg, bg);

  const wchar_t H = L'─', V = L'│', TL = L'┌', TR = L'┐', BL = L'└', BR = L'┘';
  if (w == 1 || h == 1) {
    // Degenerate: just paint something so focus doesn't crash tiny rects.
    R->fillRect(y, x, h, w, pair);
    return;
  }

  for (int c=1;c<w-1;++c){ R->putGlyph(y, x+c, H, pair); R->putGlyph(y+h-1, x+c, H, pair); }
  for (int r=1;r<h-1;++r){ R->putGlyph(y+r, x, V, pair); R->putGlyph(y+r, x+w-1, V, pair); }
  R->putGlyph(y, x, TL, pair); R->putGlyph(y, x+w-1, TR, pair);
  R->putGlyph(y+h-1, x, BL, pair); R->putGlyph(y+h-1, x+w-1, BR, pair);

  if (!obj.style.title.empty() && w>4) {
    int available = w - 4;
    std::string t = obj.style.title.substr(0, (size_t)available);
    R->putText(y, x+2, t, available, pair);
  }
}

inline void render_panel(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;
  short pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);
}

inline void render_text(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  short pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);

  auto tb = std::dynamic_pointer_cast<textBox_data_t>(obj.data);
  if (!tb) return;

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

  // Resolve scrollbar reservations (vertical + horizontal) with a few stable iterations.
  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0) return;

    lines = tb->wrap
      ? wrap_text(tb->content, std::max(1, text_w))
      : split_lines_keep_empty(tb->content);

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

  lines = tb->wrap
    ? wrap_text(tb->content, std::max(1, text_w))
    : split_lines_keep_empty(tb->content);

  max_line_len = 0;
  for (const auto& ln : lines) max_line_len = std::max(max_line_len, (int)ln.size());

  const int max_scroll_y = std::max(0, (int)lines.size() - text_h);
  const int max_scroll_x = tb->wrap ? 0 : std::max(0, max_line_len - text_w);
  tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
  tb->scroll_x = std::clamp(tb->scroll_x, 0, max_scroll_x);

  for (int row = 0; row < text_h; ++row) {
    const int li = tb->scroll_y + row;
    if (li < 0 || li >= (int)lines.size()) break;

    std::string line = lines[(std::size_t)li];
    bool selected_line = false;
    if (!line.empty() && line.front() == '\x1f') {
      selected_line = true;
      line.erase(line.begin());
    }
    if (!tb->wrap && tb->scroll_x > 0) {
      if (tb->scroll_x >= (int)line.size()) line.clear();
      else line = line.substr((std::size_t)tb->scroll_x, (std::size_t)text_w);
    }

    int colx = r.x;
    // Alignment is only meaningful when not horizontally scrolled and no side bar.
    if (tb->scroll_x == 0 && reserve_v == 0) {
      if (tb->align == text_align_t::Center) colx = r.x + std::max(0, (text_w - (int)line.size()) / 2);
      else if (tb->align == text_align_t::Right) colx = r.x + std::max(0, text_w - (int)line.size());
    }

    short line_pair = pair;
    bool line_bold = obj.style.bold;
    if (selected_line) {
      line_pair = (short)get_color_pair("#FFD26E", obj.style.background_color);
      line_bold = true;
    }
    R->putText(r.y + row, colx, line, text_w, line_pair, line_bold, obj.style.inverse);
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

    char nbuf[64];
    std::string num = std::to_string(li + 1);

    // clamp digits too, so you can’t allocate an absurd amount of padding if something goes wrong
    int width = digits;
    if (width < 1) width = 1;
    if (width > 32) width = 32;

    std::string prefix;
    if ((int)num.size() < width) prefix.append(width - (int)num.size(), ' ');
    prefix += num;
    prefix += " |";

    std::string gutter = nbuf;
    if ((int)gutter.size() > ln_w) gutter.resize((size_t)ln_w);
    else if ((int)gutter.size() < ln_w) gutter.append((size_t)(ln_w - (int)gutter.size()), ' ');
    R->putText(body_y + row, r.x, gutter, ln_w, ln_pair);

    std::string shown;
    const std::string& line = ed->lines[(size_t)li];
    if (text_w > 0 && ed->left_col < (int)line.size()) {
      shown = line.substr((size_t)ed->left_col, (size_t)text_w);
    }
    R->putText(body_y + row, r.x + ln_w, shown, text_w, base_pair);
  }

  // scroll hints
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

inline void render_buffer(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  // Base/background fill uses the widget default colors.
  short base_pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, base_pair);

  auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(obj.data);
  if (!bb) return;

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  const int n = (int)bb->lines.size();
  if (n == 0) return;

  // feed width hint back into the model
  bb->wrap_width_last = W;

  // Color selection:
  //   - if L.color present => use it (EVENT __color)
  //   - else => obj.style.label_color (FIGURE __text_color)
  auto line_pair_for = [&](const buffer_line_t& L) -> short {
    const std::string& fg = (!L.color.empty()) ? L.color : obj.style.label_color;
    short cp = (short)get_color_pair(fg, obj.style.background_color);
    return (cp > 0 ? cp : base_pair);
  };

  struct vis_row_t { std::vector<ansi::row_t> rows; }; // each entry is exactly one visible row (rows.size()==1)
  std::vector<vis_row_t> vis;
  vis.reserve((size_t)n * 2);

  auto push_wrapped = [&](const buffer_line_t& L){
    std::string prefix;
    if (!L.label.empty()) prefix = "[" + L.label + "] ";
    const int prefix_len = (int)prefix.size();
    const int avail = std::max(1, W - prefix_len);

    short base_pair_line = line_pair_for(L);

    // ANSI base style for this buffer line:
    ansi::style_t base;
    base.fg = (!L.color.empty()) ? L.color : obj.style.label_color;
    base.bg = obj.style.background_color;
    base.bold = obj.style.bold;
    base.inverse = obj.style.inverse;
    base.dim = false;

    // Wrap the payload with ANSI awareness (hard wrap)
    std::vector<ansi::row_t> payload_rows;
    ansi::hard_wrap(L.text, avail, base, base_pair_line, payload_rows);
    if (payload_rows.empty()) payload_rows.push_back(ansi::row_t{});

    for (size_t i=0;i<payload_rows.size();++i) {
      ansi::row_t full;

      // prefix / indentation uses the line base color pair
      if (i == 0) {
        if (!prefix.empty()) ansi::append_plain(full, prefix, base_pair_line, obj.style.bold, obj.style.inverse);
      } else {
        if (prefix_len > 0) ansi::append_plain(full, std::string((size_t)prefix_len, ' '), base_pair_line, obj.style.bold, obj.style.inverse);
      }

      // append payload segments
      for (const auto& seg : payload_rows[i].segs) {
        ansi::row_t tmp;
        tmp.segs.push_back(seg);
        tmp.len = (int)seg.text.size();
        // merge: render_row() is segment-based so we can just push segs
        if (!full.segs.empty()) {
          auto& last = full.segs.back();
          if (last.pair == seg.pair && last.bold == seg.bold && last.inverse == seg.inverse) {
            last.text += seg.text;
          } else {
            full.segs.push_back(seg);
          }
        } else {
          full.segs.push_back(seg);
        }
      }

      // Clamp visible len (best-effort; segment lengths are ASCII)
      int len = 0;
      for (const auto& s : full.segs) len += (int)s.text.size();
      full.len = len;

      vis_row_t vr;
      vr.rows.push_back(std::move(full)); // exactly one row
      vis.push_back(std::move(vr));
    }
  };

  if (bb->dir == buffer_dir_t::UpDown) {
    for (const auto& L : bb->lines) push_wrapped(L); // oldest..newest
  } else {
    for (int i=(int)bb->lines.size()-1; i>=0; --i) push_wrapped(bb->lines[(size_t)i]); // newest..oldest
  }

  const int total = (int)vis.size();
  if (total <= 0) return;

  const int max_scroll = std::max(0, total - H);
  if (bb->scroll < 0) bb->scroll = 0;
  if (bb->scroll > max_scroll) bb->scroll = max_scroll;
  bb->follow_tail = (bb->scroll == 0);

  int start = 0;
  if (bb->dir == buffer_dir_t::UpDown) start = std::max(0, total - H - bb->scroll);
  else                                start = bb->scroll;

  for (int row=0; row<H; ++row) {
    int idx = start + row;
    if (idx < 0 || idx >= total) break;
    const auto& one = vis[(size_t)idx].rows;
    if (!one.empty()) {
      ansi::render_row(r.y + row, r.x, W, one[0], base_pair, obj.style.bold, obj.style.inverse);
    }
  }

  if (W > 0) {
    if (start > 0)        R->putGlyph(r.y,        r.x + (W-1), L'↑', base_pair);
    if (start + H < total) R->putGlyph(r.y + (H-1), r.x + (W-1), L'↓', base_pair);
  }
}

/* Map plot_mode_t -> PlotMode */
inline PlotMode to_plot_mode(plot_mode_t m) {
  using PM = PlotMode;
  switch (m) {
    case plot_mode_t::Scatter: return PM::Scatter;
    case plot_mode_t::Stairs:  return PM::Stairs;
    case plot_mode_t::Stem:    return PM::Stem;
    case plot_mode_t::Line:
    default:                   return PM::Line;
  }
}

/* used to render plot */
inline void render_plot(const iinuji_object_t& obj) {
  auto pb = std::dynamic_pointer_cast<plotBox_data_t>(obj.data);
  if (!pb) { render_panel(obj); return; }

  Rect r = content_rect(obj);

  const std::string bg = obj.style.background_color;     // "<empty>" => terminal default
  const std::string fg = obj.style.label_color;
  const std::string ln = obj.style.border_color;

  // Background fill pair: fg doesn't really matter for spaces, but keep it consistent
  short bg_pair   = (short)get_color_pair(fg, bg);
  short axes_pair = (short)get_color_pair(fg, bg);
  short grid_pair = (short)get_color_pair(ln, bg);

  // Fill the panel background first
  if (auto* R = get_renderer()) {
    R->fillRect(r.y, r.x, r.h, r.w, bg_pair);
  }

  PlotOptions opt;
  opt.draw_axes = pb->opts.draw_axes;
  opt.draw_grid = pb->opts.draw_grid;
  opt.baseline0 = pb->opts.baseline0;
  opt.y_ticks = pb->opts.y_ticks;
  opt.x_ticks = pb->opts.x_ticks;
  opt.x_min = pb->opts.x_min; opt.x_max = pb->opts.x_max;
  opt.y_min = pb->opts.y_min; opt.y_max = pb->opts.y_max;
  opt.hard_clip = pb->opts.hard_clip;
  opt.x_log = pb->opts.x_log; opt.y_log = pb->opts.y_log;
  opt.x_log_eps = pb->opts.x_log_eps; opt.y_log_eps = pb->opts.y_log_eps;
  opt.x_label = pb->opts.x_label; opt.y_label = pb->opts.y_label;
  opt.margin_left = pb->opts.margin_left; opt.margin_right = pb->opts.margin_right;
  opt.margin_top  = pb->opts.margin_top;  opt.margin_bot   = pb->opts.margin_bot;

  // IMPORTANT: give the plotter the background pair to use for empty cells
  opt.bg_color_pair   = bg_pair;
  opt.axes_color_pair = axes_pair;
  opt.grid_color_pair = grid_pair;

  std::vector<Series> v;
  v.reserve(pb->series.size());

  for (size_t i = 0; i < pb->series.size(); ++i) {
    SeriesStyle ss{};

    const plot_series_cfg_t* scp = (i < pb->series_cfg.size()) ? &pb->series_cfg[i] : nullptr;

    // IMPORTANT: treat "<empty>" as unset; don't use it as a fg color
    std::string sfg = "#C8C8C8";
    if (scp && !scp->color_fg.empty() && scp->color_fg != "<empty>") sfg = scp->color_fg;

    // Respect explicit prebuilt color pair if provided.
    if (scp && scp->color_pair > 0) ss.color_pair = scp->color_pair;
    else ss.color_pair = (short)get_color_pair(sfg, bg);

    if (scp) {
      ss.scatter = scp->scatter;
      ss.scatter_every = scp->scatter_every;
      ss.mode = to_plot_mode(scp->mode);
      ss.fill_vertical_if_same_x = scp->fill_vertical_if_same_x;
      ss.stem_y = scp->stem_y;
      ss.envelope_enabled = scp->envelope_enabled;
      ss.envelope_source =
        (scp->envelope_source == envelope_source_t::OriginalSamples)
          ? SeriesStyle::EnvelopeSource::OriginalSamples
          : SeriesStyle::EnvelopeSource::SegmentPath;
      ss.envelope_min_count  = scp->envelope_min_count;
      ss.envelope_min_height = scp->envelope_min_height;
      ss.envelope_draw_base  = scp->envelope_draw_base;
    }

    v.push_back(Series{ &pb->series[i], ss });
  }

  plot_braille_multi(v, r.x, r.y, r.w, r.h, opt);
}

/* Render whole tree (after layout_tree) */
inline void render_tree(const std::shared_ptr<iinuji_object_t>& node) {
  if (!node || !node->visible) return;

  // Focus frame (borderless focus highlight) must be drawn before content fill,
  // and content_rect() already reserves the 1-cell frame while focused.
  render_focus_frame_bg(*node);

  render_border(*node);

  if (std::dynamic_pointer_cast<plotBox_data_t>(node->data))            render_plot(*node);
  else if (std::dynamic_pointer_cast<bufferBox_data_t>(node->data))     render_buffer(*node);
  else if (std::dynamic_pointer_cast<editorBox_data_t>(node->data))     render_editor(*node);
  else if (std::dynamic_pointer_cast<textBox_data_t>(node->data))       render_text(*node);
  else                                                                  render_panel(*node);

  for (auto& ch : node->children) render_tree(ch);
}

} // namespace iinuji
} // namespace cuwacunu
