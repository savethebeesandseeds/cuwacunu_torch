/* primitives/plot.h */
#pragma once
#include "iinuji/render/renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "piaabo/log/dlogs.h" // Diagnostics only; rendering uses iinuji hooks.

namespace cuwacunu {
namespace iinuji {

/* ----------------------------- Braille primitives --------------------------
 */
static inline int dot_bit_index(int sub_x, int sub_y) {
  // Braille cell has 2x4 dots; mapping:
  // (0,0)=1,(1,0)=4,(0,1)=2,(1,1)=5,(0,2)=3,(1,2)=6,(0,3)=7,(1,3)=8
  static const int dot_map[4][2] = {
      {0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};
  return dot_map[sub_y][sub_x];
}

/* ------------------------------- Public API --------------------------------
 */
enum class PlotMode {
  Line,    // connect-the-dots (Bresenham)
  Scatter, // points only
  Stairs,  // horizontal then vertical at each sample
  Stem     // vertical sticks from baseline to y
};

struct PlotOptions {
  // Viewport (braille cells) and margins (in terminal columns/rows)
  int margin_left = 8; // space for y labels
  int margin_right = 2;
  int margin_top = 1;
  int margin_bot = 2; // space for x labels

  // Axes behavior
  bool draw_axes = true;
  bool draw_grid = true;
  int y_ticks = 5;
  int x_ticks = 6;
  bool draw_y_tick_labels = true;
  bool draw_y2_tick_labels = false;
  bool draw_x_tick_labels = true;
  bool baseline0 = true; // draw y=0 line if within range

  // Ranges: if NaN, auto
  double x_min = std::numeric_limits<double>::quiet_NaN();
  double x_max = std::numeric_limits<double>::quiet_NaN();
  double y_min = std::numeric_limits<double>::quiet_NaN();
  double y_max = std::numeric_limits<double>::quiet_NaN();
  double y2_min = std::numeric_limits<double>::quiet_NaN();
  double y2_max = std::numeric_limits<double>::quiet_NaN();

  // Optional log scales
  bool x_log = false;
  bool y_log = false;
  double x_log_eps = 1e-12; // added before log
  double y_log_eps = 1e-12;

  // Labels
  std::string x_label;
  std::string y_label;
  std::string y2_label;

  // Clipping safeguard
  bool hard_clip = true;

  // Colors supplied by the backend
  short axes_color_pair = 0;
  short y_axis_color_pair = 0;
  short y2_axis_color_pair = 0;
  short grid_color_pair = 0;
  short bg_color_pair =
      0; // fallback color; when set, empty plot cells are filled
};

struct SeriesStyle {
  short color_pair = 0;
  bool scatter = false;
  int scatter_every = 1;
  PlotMode mode = PlotMode::Line;

  double stem_y = std::numeric_limits<double>::quiet_NaN();
  bool fill_vertical_if_same_x = true;

  // Envelope overlay controls (applies to Line)
  enum class EnvelopeSource { OriginalSamples, SegmentPath };
  bool envelope_enabled = false;
  EnvelopeSource envelope_source = EnvelopeSource::OriginalSamples;
  int envelope_min_count = 2;
  int envelope_min_height = 2;
  bool envelope_draw_base = true;
  bool use_y2_axis = false;
};

struct Series {
  const std::vector<std::pair<double, double>> *data = nullptr;
  SeriesStyle style;
};

/* ------------------------------ Utils --------------------------------------
 */
static inline bool is_finite(double v) { return std::isfinite(v); }

static inline void cell_set_dot(std::vector<std::vector<unsigned char>> &cells,
                                int width_cells, int height_cells, int px,
                                int py) {
  if (px < 0 || py < 0)
    return;
  int cell_x = px / 2;
  int cell_y = py / 4;
  if (cell_x < 0 || cell_x >= width_cells || cell_y < 0 ||
      cell_y >= height_cells)
    return;
  int sub_x = px % 2;
  int sub_y = py % 4;
  cells[cell_y][cell_x] |= (unsigned char)dot_bit_index(sub_x, sub_y);
}

// OVERLAY color: overwrite
static inline void color_touch(std::vector<std::vector<short>> &colors,
                               int width_cells, int height_cells, int px,
                               int py, short cp) {
  if (cp <= 0)
    return;
  int cell_x = px / 2;
  int cell_y = py / 4;
  if (cell_x < 0 || cell_x >= width_cells || cell_y < 0 ||
      cell_y >= height_cells)
    return;
  colors[cell_y][cell_x] = cp;
}

static inline void draw_text_clipped(int y, int x, const std::string &s,
                                     int max_w, short color_pair = 0) {
  if (max_w <= 0)
    return;
  std::string t = (int)s.size() > max_w ? s.substr(0, max_w) : s;
  if (auto *R = get_renderer()) {
    R->putText(y, x, t, max_w, color_pair);
  }
}

/* "nice" tick step (rough) */
static inline double nice_step(double span, int target_ticks) {
  if (span <= 0 || target_ticks <= 0)
    return 1.0;
  double raw = span / target_ticks;
  double mag = std::pow(10.0, std::floor(std::log10(raw)));
  double norm = raw / mag;
  double step;
  if (norm < 1.5)
    step = 1.0;
  else if (norm < 3.0)
    step = 2.0;
  else if (norm < 7.0)
    step = 5.0;
  else
    step = 10.0;
  return step * mag;
}

static inline double safe_log10(double v, double eps) {
  if (std::isnan(v))
    return std::numeric_limits<double>::quiet_NaN();
  if (v <= -eps)
    return std::numeric_limits<double>::quiet_NaN();
  return std::log10(std::max(v, 0.0) + eps);
}

/* ----------------------- integer-line rasterization ------------------------
 */
template <class F>
static inline void rasterize_line_int(int x0, int y0, int x1, int y1,
                                      F &&plot) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int x = x0, y = y0;
  while (true) {
    plot(x, y);
    if (x == x1 && y == y1)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y += sy;
    }
  }
}

static inline void rasterize_vertical_span(
    int px, int py0, int py1, std::vector<std::vector<unsigned char>> &cells,
    std::vector<std::vector<short>> &colors, int plot_w, int plot_h, short cp) {
  if (py0 > py1)
    std::swap(py0, py1);
  for (int py = py0; py <= py1; ++py) {
    cell_set_dot(cells, plot_w, plot_h, px, py);
    color_touch(colors, plot_w, plot_h, px, py, cp);
  }
}

/* ------------------------------ Core plotter --------------------------------
 */
static void plot_braille_multi(const std::vector<Series> &series, int start_x,
                               int start_y, int width_cells, int height_cells,
                               const PlotOptions &opt = {}) {
  if (width_cells <= 0 || height_cells <= 0) {
    log_err("(iinuji_plot)[plot_braille] width/height must be > 0\n");
    return;
  }
  if (series.empty())
    return;

  // Inner plot area (in braille cells) after margins
  const int plot_x0 = start_x + opt.margin_left;
  const int plot_y0 = start_y + opt.margin_top;
  const int plot_w =
      std::max(0, width_cells - (opt.margin_left + opt.margin_right));
  const int plot_h =
      std::max(0, height_cells - (opt.margin_top + opt.margin_bot));
  if (plot_w <= 0 || plot_h <= 0)
    return;

  auto transform_x = [&](double x) -> double {
    return opt.x_log ? safe_log10(x, opt.x_log_eps) : x;
  };
  auto transform_y = [&](double y) -> double {
    return opt.y_log ? safe_log10(y, opt.y_log_eps) : y;
  };

  // Data ranges: x is shared, y and y2 are independent axes.
  double x_min = is_finite(opt.x_min)
                     ? transform_x(opt.x_min)
                     : std::numeric_limits<double>::quiet_NaN();
  double x_max = is_finite(opt.x_max)
                     ? transform_x(opt.x_max)
                     : std::numeric_limits<double>::quiet_NaN();
  double y_min = is_finite(opt.y_min)
                     ? transform_y(opt.y_min)
                     : std::numeric_limits<double>::quiet_NaN();
  double y_max = is_finite(opt.y_max)
                     ? transform_y(opt.y_max)
                     : std::numeric_limits<double>::quiet_NaN();
  double y2_min = is_finite(opt.y2_min)
                      ? transform_y(opt.y2_min)
                      : std::numeric_limits<double>::quiet_NaN();
  double y2_max = is_finite(opt.y2_max)
                      ? transform_y(opt.y2_max)
                      : std::numeric_limits<double>::quiet_NaN();

  bool auto_x_min = !is_finite(opt.x_min) || !is_finite(x_min);
  bool auto_x_max = !is_finite(opt.x_max) || !is_finite(x_max);
  bool auto_y_min = !is_finite(opt.y_min) || !is_finite(y_min);
  bool auto_y_max = !is_finite(opt.y_max) || !is_finite(y_max);
  bool auto_y2_min = !is_finite(opt.y2_min) || !is_finite(y2_min);
  bool auto_y2_max = !is_finite(opt.y2_max) || !is_finite(y2_max);

  auto acc_y_range = [](double y, double &range_min, double &range_max,
                        bool auto_min, bool auto_max) {
    if (auto_min)
      range_min = !is_finite(range_min) ? y : std::min(range_min, y);
    if (auto_max)
      range_max = !is_finite(range_max) ? y : std::max(range_max, y);
  };

  auto acc_range = [&](const SeriesStyle &style, double x, double y) {
    x = transform_x(x);
    y = transform_y(y);
    if (!is_finite(x) || !is_finite(y))
      return;

    if (auto_x_min)
      x_min = !is_finite(x_min) ? x : std::min(x_min, x);
    if (auto_x_max)
      x_max = !is_finite(x_max) ? x : std::max(x_max, x);
    if (style.use_y2_axis)
      acc_y_range(y, y2_min, y2_max, auto_y2_min, auto_y2_max);
    else
      acc_y_range(y, y_min, y_max, auto_y_min, auto_y_max);
  };

  for (const auto &s : series)
    if (s.data)
      for (const auto &p : *s.data)
        acc_range(s.style, p.first, p.second);

  if (!is_finite(x_min) || !is_finite(x_max) || !(x_max > x_min)) {
    x_min = 0.0;
    x_max = 1.0;
  }
  if (!is_finite(y_min) || !is_finite(y_max) || !(y_max > y_min)) {
    y_min = 0.0;
    y_max = 1.0;
  }
  if (!is_finite(y2_min) || !is_finite(y2_max) || !(y2_max > y2_min)) {
    y2_min = y_min;
    y2_max = y_max;
  }

  // Plot geometry uses separate data/underlay buffers so grid/baseline never
  // mutate the same braille glyph as series data.
  std::vector<std::vector<unsigned char>> underlay_cells(
      plot_h, std::vector<unsigned char>(plot_w, 0));
  std::vector<std::vector<short>> underlay_colors(
      plot_h, std::vector<short>(plot_w, 0));
  std::vector<std::vector<unsigned char>> cells(
      plot_h, std::vector<unsigned char>(plot_w, 0));
  std::vector<std::vector<short>> colors(plot_h, std::vector<short>(plot_w, 0));

  auto map_tx_to_px = [&](double tx, int &px) -> bool {
    if (!is_finite(tx))
      return false;
    double t = (tx - x_min) / (x_max - x_min);
    if (!is_finite(t))
      return false;
    if (opt.hard_clip)
      t = std::clamp(t, 0.0, 1.0);
    px = (int)std::llround(t * (plot_w * 2 - 1));
    return true;
  };

  auto map_ty_range_to_py = [&](double ty, double range_min, double range_max,
                                int &py) -> bool {
    if (!is_finite(ty))
      return false;
    double t = (ty - range_min) / (range_max - range_min);
    if (!is_finite(t))
      return false;
    if (opt.hard_clip)
      t = std::clamp(t, 0.0, 1.0);
    // invert so larger y appears *higher* on screen
    py = (int)std::llround((1.0 - t) * (plot_h * 4 - 1));
    return true;
  };

  auto map_ty_to_py = [&](double ty, int &py) -> bool {
    return map_ty_range_to_py(ty, y_min, y_max, py);
  };

  auto map_ty2_to_py = [&](double ty, int &py) -> bool {
    return map_ty_range_to_py(ty, y2_min, y2_max, py);
  };

  auto project_x = [&](double x, int &px) -> bool {
    return map_tx_to_px(transform_x(x), px);
  };

  auto project_y = [&](const SeriesStyle &style, double y, int &py) -> bool {
    const double ty = transform_y(y);
    return style.use_y2_axis ? map_ty2_to_py(ty, py) : map_ty_to_py(ty, py);
  };

  auto project_point = [&](const SeriesStyle &style, double x, double y,
                           int &px, int &py) -> bool {
    return project_x(x, px) && project_y(style, y, py);
  };

  // Optional baseline y=0
  int baseline_py = std::numeric_limits<int>::min();
  const double zero_ty = transform_y(0.0);
  const bool draw_baseline =
      opt.baseline0 && !opt.y_log && is_finite(zero_ty) && y_min < zero_ty &&
      zero_ty < y_max && map_ty_to_py(zero_ty, baseline_py);
  const short y_axis_pair =
      opt.y_axis_color_pair > 0 ? opt.y_axis_color_pair : opt.axes_color_pair;
  const short y2_axis_pair =
      opt.y2_axis_color_pair > 0 ? opt.y2_axis_color_pair : opt.axes_color_pair;

  // --------------------------------------------------------------------------
  // PREPASS: draw GRID + BASELINE into the dedicated underlay buffer
  // --------------------------------------------------------------------------
  if (opt.draw_grid) {
    // Horizontal grid (Y ticks)
    double y_span = y_max - y_min;
    double y_step = nice_step(y_span, std::max(2, opt.y_ticks));
    double y0 = std::ceil(y_min / y_step) * y_step;

    for (double yv = y0; yv <= y_max + 1e-12; yv += y_step) {
      int py = 0;
      if (!map_ty_to_py(yv, py))
        continue;
      for (int px = 0; px < plot_w * 2; ++px) {
        cell_set_dot(underlay_cells, plot_w, plot_h, px, py);
        color_touch(underlay_colors, plot_w, plot_h, px, py,
                    opt.grid_color_pair);
      }
    }

    // Vertical grid (X ticks)
    double x_span = x_max - x_min;
    double x_step = nice_step(x_span, std::max(2, opt.x_ticks));
    double x0 = std::ceil(x_min / x_step) * x_step;

    for (double xv = x0; xv <= x_max + 1e-12; xv += x_step) {
      int px = 0;
      if (!map_tx_to_px(xv, px))
        continue;
      for (int py = 0; py < plot_h * 4; ++py) {
        cell_set_dot(underlay_cells, plot_w, plot_h, px, py);
        color_touch(underlay_colors, plot_w, plot_h, px, py,
                    opt.grid_color_pair);
      }
    }
  }

  // Baseline y=0
  if (draw_baseline) {
    for (int px = 0; px < plot_w * 2; ++px) {
      cell_set_dot(underlay_cells, plot_w, plot_h, px, baseline_py);
      color_touch(underlay_colors, plot_w, plot_h, px, baseline_py,
                  opt.axes_color_pair);
    }
  }

  if (opt.draw_axes) {
    for (int py = 0; py < plot_h * 4; ++py) {
      cell_set_dot(underlay_cells, plot_w, plot_h, 0, py);
      color_touch(underlay_colors, plot_w, plot_h, 0, py, y_axis_pair);
      if (opt.draw_y2_tick_labels) {
        const int right_px = std::max(0, plot_w * 2 - 1);
        cell_set_dot(underlay_cells, plot_w, plot_h, right_px, py);
        color_touch(underlay_colors, plot_w, plot_h, right_px, py,
                    y2_axis_pair);
      }
    }
  }

  // --------------------------------------------------------------------------
  // SERIES: draw data OVERLAY (wins over grid/baseline)
  // --------------------------------------------------------------------------
  for (const auto &s : series) {
    if (!s.data || s.data->empty())
      continue;
    const auto &pts = *s.data;

    auto put_dot = [&](int px, int py) {
      if (!opt.hard_clip ||
          (px >= 0 && px < plot_w * 2 && py >= 0 && py < plot_h * 4)) {
        cell_set_dot(cells, plot_w, plot_h, px, py);
        color_touch(colors, plot_w, plot_h, px, py,
                    s.style.color_pair); // OVERLAY
      }
    };

    auto draw_segment_line = [&](double x1, double y1, double x2, double y2) {
      int px1 = 0, py1 = 0, px2 = 0, py2 = 0;
      if (!project_point(s.style, x1, y1, px1, py1) ||
          !project_point(s.style, x2, y2, px2, py2))
        return;
      rasterize_line_int(px1, py1, px2, py2,
                         [&](int qx, int qy) { put_dot(qx, qy); });
      if (s.style.fill_vertical_if_same_x && px1 == px2 &&
          std::abs(py2 - py1) > 1) {
        rasterize_vertical_span(px1, py1, py2, cells, colors, plot_w, plot_h,
                                s.style.color_pair);
      }
    };

    auto draw_scatter = [&]() {
      int every = std::max(1, s.style.scatter_every);
      for (size_t i = 0; i < pts.size(); i += (size_t)every) {
        const auto &p = pts[i];
        int px = 0, py = 0;
        if (!project_point(s.style, p.first, p.second, px, py))
          continue;
        put_dot(px, py);
      }
    };

    switch (s.style.mode) {
    case PlotMode::Scatter: {
      draw_scatter();
    } break;

    case PlotMode::Stairs: {
      for (size_t i = 1; i < pts.size(); ++i) {
        double x1 = pts[i - 1].first, y1 = pts[i - 1].second;
        double x2 = pts[i].first, y2 = pts[i].second;
        if (!is_finite(x1) || !is_finite(y1) || !is_finite(x2) ||
            !is_finite(y2))
          continue;
        draw_segment_line(x1, y1, x2, y1); // horizontal
        draw_segment_line(x2, y1, x2, y2); // vertical
      }
      if (s.style.scatter)
        draw_scatter();
    } break;

    case PlotMode::Stem: {
      // Determine stem baseline
      double by = s.style.stem_y;
      if (!is_finite(by)) {
        double zero_t = transform_y(0.0);
        by = (y_min <= zero_t && zero_t <= y_max)
                 ? 0.0
                 : (opt.y_log ? std::pow(10.0, y_min) - opt.y_log_eps : y_min);
      }
      int bpy = 0;
      if (!project_y(s.style, by, bpy))
        break;
      for (const auto &p : pts) {
        int px = 0, py = 0;
        if (!project_point(s.style, p.first, p.second, px, py))
          continue;
        rasterize_vertical_span(px, bpy, py, cells, colors, plot_w, plot_h,
                                s.style.color_pair);
      }
      if (s.style.scatter)
        draw_scatter();
    } break;

    case PlotMode::Line: {
      const bool draw_base =
          (!s.style.envelope_enabled) || s.style.envelope_draw_base;
      if (draw_base) {
        for (size_t i = 1; i < pts.size(); ++i) {
          double x1 = pts[i - 1].first, y1 = pts[i - 1].second;
          double x2 = pts[i].first, y2 = pts[i].second;
          if (!is_finite(x1) || !is_finite(y1) || !is_finite(x2) ||
              !is_finite(y2))
            continue;
          draw_segment_line(x1, y1, x2, y2);
        }
      }

      if (s.style.envelope_enabled) {
        const int XW = plot_w * 2;
        std::vector<int> bin_min(XW, std::numeric_limits<int>::max());
        std::vector<int> bin_max(XW, -std::numeric_limits<int>::max());
        std::vector<int> bin_cnt(XW, 0);

        if (s.style.envelope_source ==
            SeriesStyle::EnvelopeSource::OriginalSamples) {
          for (const auto &p : pts) {
            int px = 0, py = 0;
            if (!project_point(s.style, p.first, p.second, px, py))
              continue;
            if (px >= 0 && px < XW) {
              bin_min[px] = std::min(bin_min[px], py);
              bin_max[px] = std::max(bin_max[px], py);
              bin_cnt[px]++;
            }
          }
        } else {
          for (size_t i = 1; i < pts.size(); ++i) {
            double x1 = pts[i - 1].first, y1 = pts[i - 1].second;
            double x2 = pts[i].first, y2 = pts[i].second;
            int px1 = 0, py1 = 0, px2 = 0, py2 = 0;
            if (!project_point(s.style, x1, y1, px1, py1) ||
                !project_point(s.style, x2, y2, px2, py2))
              continue;
            rasterize_line_int(px1, py1, px2, py2, [&](int qx, int qy) {
              if (qx >= 0 && qx < XW) {
                bin_min[qx] = std::min(bin_min[qx], qy);
                bin_max[qx] = std::max(bin_max[qx], qy);
                bin_cnt[qx]++;
              }
            });
          }
        }

        const int MIN_COUNT = std::max(1, s.style.envelope_min_count);
        const int MIN_HEIGHT = std::max(0, s.style.envelope_min_height);
        for (int qx = 0; qx < XW; ++qx) {
          if (bin_cnt[qx] >= MIN_COUNT && bin_min[qx] <= bin_max[qx]) {
            if ((bin_max[qx] - bin_min[qx]) >= MIN_HEIGHT) {
              rasterize_vertical_span(qx, bin_min[qx], bin_max[qx], cells,
                                      colors, plot_w, plot_h,
                                      s.style.color_pair);
            }
          }
        }
      }

      if (s.style.scatter)
        draw_scatter();
    } break;
    }
  }

  // --------------------------------------------------------------------------
  // LABELS: draw texts only (outside plot area). NO glyphs inside the plot!
  // --------------------------------------------------------------------------
  if (opt.draw_axes) {
    if (opt.draw_y_tick_labels) {
      double y_span = y_max - y_min;
      double y_step = nice_step(y_span, std::max(2, opt.y_ticks));
      double y0 = std::ceil(y_min / y_step) * y_step;

      for (double yv = y0; yv <= y_max + 1e-12; yv += y_step) {
        int py = 0;
        if (!map_ty_to_py(yv, py))
          continue;
        int row = plot_y0 + (py / 4);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6g",
                      opt.y_log ? (std::pow(10.0, yv) - opt.y_log_eps) : yv);
        int lab_x =
            start_x + std::max(0, opt.margin_left - 1 - (int)std::strlen(buf));
        draw_text_clipped(row, lab_x, buf, opt.margin_left - 1, y_axis_pair);
      }

      if (!opt.y_label.empty()) {
        draw_text_clipped(start_y, start_x, opt.y_label, opt.margin_left,
                          y_axis_pair);
      }
    }

    if (opt.draw_y2_tick_labels) {
      double y2_span = y2_max - y2_min;
      double y2_step = nice_step(y2_span, std::max(2, opt.y_ticks));
      double y20 = std::ceil(y2_min / y2_step) * y2_step;

      for (double yv = y20; yv <= y2_max + 1e-12; yv += y2_step) {
        int py = 0;
        if (!map_ty2_to_py(yv, py))
          continue;
        int row = plot_y0 + (py / 4);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6g",
                      opt.y_log ? (std::pow(10.0, yv) - opt.y_log_eps) : yv);
        draw_text_clipped(row, plot_x0 + plot_w + 1, buf, opt.margin_right - 1,
                          y2_axis_pair);
      }

      if (!opt.y2_label.empty()) {
        draw_text_clipped(start_y, plot_x0 + plot_w + 1, opt.y2_label,
                          opt.margin_right - 1, y2_axis_pair);
      }
    }

    if (opt.draw_x_tick_labels) {
      double x_span = x_max - x_min;
      double x_step = nice_step(x_span, std::max(2, opt.x_ticks));
      double x0 = std::ceil(x_min / x_step) * x_step;

      for (double xv = x0; xv <= x_max + 1e-12; xv += x_step) {
        int px = 0;
        if (!map_tx_to_px(xv, px))
          continue;
        int col = plot_x0 + (px / 2);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6g",
                      opt.x_log ? (std::pow(10.0, xv) - opt.x_log_eps) : xv);
        int lx = col - (int)std::strlen(buf) / 2;
        draw_text_clipped(start_y + opt.margin_top + plot_h + 0, lx, buf,
                          (int)std::strlen(buf), opt.axes_color_pair);
      }

      if (!opt.x_label.empty()) {
        draw_text_clipped(start_y + opt.margin_top + plot_h + 1, plot_x0,
                          opt.x_label, plot_w, opt.axes_color_pair);
      }
    }
  }

  // --------------------------------------------------------------------------
  // BLIT: write braille cells with per-cell color
  // --------------------------------------------------------------------------
  if (auto *R = get_renderer()) {
    for (int r = 0; r < plot_h; ++r) {
      for (int c = 0; c < plot_w; ++c) {
        unsigned char bits = cells[r][c];
        short cp = colors[r][c];

        if (bits == 0) {
          bits = underlay_cells[r][c];
          cp = underlay_colors[r][c];
        }

        if (bits == 0 && cp == 0 && opt.bg_color_pair <= 0)
          continue;

        // If there are no dots, draw SPACE so the background fill stays uniform
        wchar_t ch = (bits == 0) ? L' ' : (wchar_t)(0x2800 + bits);

        if (cp == 0)
          cp = opt.bg_color_pair; // use plot background

        R->putBraille(plot_y0 + r, plot_x0 + c, ch, cp);
      }
    }
  }
}

/* ----------------------- Convenience single-series API ----------------------
 */
static void plot_braille(const std::vector<std::pair<double, double>> &points,
                         int start_x, int start_y, int width_cells,
                         int height_cells) {
  PlotOptions opt;
  Series s;
  s.data = &points;
  s.style = {};
  plot_braille_multi({s}, start_x, start_y, width_cells, height_cells, opt);
}

} // namespace iinuji
} // namespace cuwacunu
