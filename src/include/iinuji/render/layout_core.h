#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/render/renderer.h"

namespace cuwacunu {
namespace iinuji {

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

} // namespace iinuji
} // namespace cuwacunu
