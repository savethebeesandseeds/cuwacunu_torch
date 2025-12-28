/* iinuji_render.h */
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cwchar>

#include "iinuji/iinuji_rend.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_plot.h"

namespace cuwacunu {
namespace iinuji {

/* -------------------- Helpers -------------------- */
inline Rect inset_rect(Rect r, int l,int t,int rgt,int btm){
  return Rect{ r.x + l, r.y + t, std::max(0, r.w - (l + rgt)), std::max(0, r.h - (t + btm)) };
}

/* Compute a container's content rect (after border + padding) */
inline Rect content_rect(const iinuji_object_t& o) {
  Rect r = o.screen;
  if (o.style.border) r = inset_rect(r, 1,1,1,1);
  r = inset_rect(r, o.layout.pad_left, o.layout.pad_top, o.layout.pad_right, o.layout.pad_bottom);
  return r;
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
      int px = (int)(ch->layout.normalized ? inner.x + ch->layout.x * inner.w : inner.x + ch->layout.x);
      int py = (int)(ch->layout.normalized ? inner.y + ch->layout.y * inner.h : inner.y + ch->layout.y);
      int pw = (int)(ch->layout.normalized ? ch->layout.width  * inner.w : ch->layout.width);
      int ph = (int)(ch->layout.normalized ? ch->layout.height * inner.h : ch->layout.height);
      layout_tree(ch, Rect{px, py, pw, ph});
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

  int x=obj.screen.x, y=obj.screen.y, w=std::max(2,obj.screen.w), h=std::max(2,obj.screen.h);
  short pair = (short)get_color_pair(obj.style.border_color, obj.style.background_color);

  const wchar_t H = L'─', V = L'│', TL = L'┌', TR = L'┐', BL = L'└', BR = L'┘';
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

  auto lines = tb->wrap ? wrap_text(tb->content, std::max(1,r.w)) : std::vector<std::string>{tb->content};
  int rows = std::min((int)lines.size(), r.h);
  for (int i=0;i<rows;++i) {
    int colx = r.x;
    if (tb->align == text_align_t::Center) colx = r.x + std::max(0, (r.w - (int)lines[i].size())/2);
    else if (tb->align == text_align_t::Right) colx = r.x + std::max(0, r.w - (int)lines[i].size());
    R->putText(r.y + i, colx, lines[i], r.w, pair, obj.style.bold, obj.style.inverse);
  }
}

inline void render_buffer(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  short pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);

  auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(obj.data);
  if (!bb) return;

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  const int n = (int)bb->lines.size();
  const int max_scroll = std::max(0, n - H);

  // clamp scroll safely here (render knows visible H)
  if (bb->scroll > max_scroll) bb->scroll = max_scroll;
  if (bb->scroll < 0) bb->scroll = 0;
  if (bb->scroll == 0) bb->follow_tail = true;

  if (n == 0) return;

  if (bb->dir == buffer_dir_t::UpDown) {
    // start index into chronological lines (oldest..newest)
    const int start = std::max(0, n - H - bb->scroll);
    for (int row = 0; row < H; ++row) {
      int idx = start + row;
      if (idx < 0 || idx >= n) break;
      const std::string& line = bb->lines[(size_t)idx];

      // clip
      std::string s = ((int)line.size() > W) ? line.substr(0, (size_t)W) : line;
      R->putText(r.y + row, r.x, s, W, pair, obj.style.bold, obj.style.inverse);
    }

    // scroll hints
    if (start > 0 && W > 0) R->putGlyph(r.y, r.x + (W-1), L'↑', pair);
    if (start + H < n && W > 0) R->putGlyph(r.y + (H-1), r.x + (W-1), L'↓', pair);
  } else { // DownUp
    // top shows newest, move downward to older
    const int top = (n - 1) - bb->scroll;
    for (int row = 0; row < H; ++row) {
      int idx = top - row;
      if (idx < 0 || idx >= n) break;
      const std::string& line = bb->lines[(size_t)idx];

      std::string s = ((int)line.size() > W) ? line.substr(0, (size_t)W) : line;
      R->putText(r.y + row, r.x, s, W, pair, obj.style.bold, obj.style.inverse);
    }

    // scroll hints (meaning flips visually)
    // if scroll>0 => there exist newer lines above
    if (bb->scroll > 0 && W > 0) R->putGlyph(r.y, r.x + (W-1), L'↑', pair);
    // if bottom visible is not oldest => there exist older lines below
    if ((top - (H-1)) > 0 && W > 0) R->putGlyph(r.y + (H-1), r.x + (W-1), L'↓', pair);
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

  short bg_pair   = (short)get_color_pair(obj.style.label_color,  obj.style.background_color);
  short axes_pair = (short)get_color_pair(obj.style.border_color, obj.style.background_color);

  // Panel background
  if (auto* R = get_renderer()) {
    R->fillRect(r.y, r.x, r.h, r.w, bg_pair);
  }

  PlotOptions opt;
  opt.draw_axes = pb->opts.draw_axes;
  opt.draw_grid = pb->opts.draw_grid;
  opt.baseline0 = pb->opts.baseline0;
  opt.y_ticks = pb->opts.y_ticks; opt.x_ticks = pb->opts.x_ticks;
  opt.x_min = pb->opts.x_min; opt.x_max = pb->opts.x_max;
  opt.y_min = pb->opts.y_min; opt.y_max = pb->opts.y_max;
  opt.hard_clip = pb->opts.hard_clip;
  opt.x_log = pb->opts.x_log; opt.y_log = pb->opts.y_log;
  opt.x_log_eps = pb->opts.x_log_eps; opt.y_log_eps = pb->opts.y_log_eps;
  opt.x_label = pb->opts.x_label; opt.y_label = pb->opts.y_label;
  opt.margin_left = pb->opts.margin_left; opt.margin_right = pb->opts.margin_right;
  opt.margin_top  = pb->opts.margin_top;  opt.margin_bot   = pb->opts.margin_bot;

  // supply axes & grid colors to the plotter
  opt.axes_color_pair = axes_pair;
  opt.grid_color_pair = axes_pair;

  std::vector<Series> v; v.reserve(pb->series.size());

  // iinuji_render.h — inside render_plot, when building v[]
  for (size_t i=0; i<pb->series.size(); ++i) {
    SeriesStyle ss{};

    const plot_series_cfg_t* scp = (i < pb->series_cfg.size()) ? &pb->series_cfg[i] : nullptr;

    // Always compute a valid (non-zero) color pair
    const std::string fg = (scp && !scp->color_fg.empty()) ? scp->color_fg : "#C8C8C8";
    short cp = (short)get_color_pair(fg, obj.style.background_color);
    if (cp <= 0) cp = (short)get_color_pair("white", obj.style.background_color);
    ss.color_pair = cp;
    
    // Copy other fields only if cfg exists; otherwise defaults stand
    if (scp) {
      ss.scatter = scp->scatter;
      ss.scatter_every = scp->scatter_every;
      ss.mode = to_plot_mode(scp->mode);
      ss.fill_vertical_if_same_x = scp->fill_vertical_if_same_x;
      ss.stem_y = scp->stem_y;
      ss.envelope_enabled = scp->envelope_enabled;
      ss.envelope_source = (scp->envelope_source == envelope_source_t::OriginalSamples)
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

  render_border(*node);

  if (std::dynamic_pointer_cast<plotBox_data_t>(node->data))           render_plot(*node);
  else if (std::dynamic_pointer_cast<bufferBox_data_t>(node->data))    render_buffer(*node);
  else if (std::dynamic_pointer_cast<textBox_data_t>(node->data))      render_text(*node);
  else                                                                 render_panel(*node);

  for (auto& ch : node->children) render_tree(ch);
}

} // namespace iinuji
} // namespace cuwacunu
