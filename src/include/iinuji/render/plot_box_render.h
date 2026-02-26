#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/primitives/plot.h"
#include "iinuji/render/panel_render.h"

namespace cuwacunu {
namespace iinuji {

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

} // namespace iinuji
} // namespace cuwacunu
