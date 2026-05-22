/* primitives/plot_box.h */
#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

enum class plot_mode_t { Line, Scatter, Stairs, Stem };
enum class envelope_source_t { OriginalSamples, SegmentPath };

struct plot_series_cfg_t {
  std::string color_fg;
  short color_pair{-1};

  plot_mode_t mode{plot_mode_t::Line};
  bool scatter{false};
  int scatter_every{1};

  bool fill_vertical_if_same_x{true};
  double stem_y{std::numeric_limits<double>::quiet_NaN()};

  bool envelope_enabled{false};
  envelope_source_t envelope_source{envelope_source_t::OriginalSamples};
  int envelope_min_count{2};
  int envelope_min_height{2};
  bool envelope_draw_base{true};
};

struct plot_box_options_t {
  bool draw_axes{true};
  bool draw_grid{true};
  bool baseline0{true};
  int y_ticks{5};
  int x_ticks{6};
  double x_min{std::numeric_limits<double>::quiet_NaN()};
  double x_max{std::numeric_limits<double>::quiet_NaN()};
  double y_min{std::numeric_limits<double>::quiet_NaN()};
  double y_max{std::numeric_limits<double>::quiet_NaN()};
  bool hard_clip{true};
  bool x_log{false};
  bool y_log{false};
  double x_log_eps{1e-12};
  double y_log_eps{1e-12};
  std::string x_label{};
  std::string y_label{};
  int margin_left{8};
  int margin_right{2};
  int margin_top{1};
  int margin_bot{2};
};

using plotbox_opts_t = plot_box_options_t;

struct plot_box_data_t : public iinuji_data_t {
  std::vector<std::vector<std::pair<double, double>>> series;
  std::vector<plot_series_cfg_t> series_cfg;
  plot_box_options_t opts;
};

using plotBox_data_t = plot_box_data_t;

struct plot_box_create_opts_t {
  std::vector<std::vector<std::pair<double, double>>> series{};
  std::vector<plot_series_cfg_t> series_cfg{};
  plot_box_options_t plot{};
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::None};
};

inline std::shared_ptr<iinuji_object_t>
create_plot_box(const std::string &id, plot_box_create_opts_t opts = {}) {
  auto object = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style),
      primitive_role_t::PlotBox, opts.focus_mode);
  auto plot = std::make_shared<plot_box_data_t>();
  plot->series = std::move(opts.series);
  plot->series_cfg = std::move(opts.series_cfg);
  plot->opts = opts.plot;
  object->data = plot;
  return object;
}

inline std::shared_ptr<iinuji_object_t> create_plot_box(
    const std::string &id,
    const std::vector<std::vector<std::pair<double, double>>> &series,
    const std::vector<plot_series_cfg_t> &cfg, const plot_box_options_t &opts,
    const iinuji_layout_t &layout = {}, const iinuji_style_t &style = {},
    focus_mode_t focus_mode = focus_mode_t::None) {
  plot_box_create_opts_t create_opts{};
  create_opts.series = series;
  create_opts.series_cfg = cfg;
  create_opts.plot = opts;
  create_opts.layout = layout;
  create_opts.style = style;
  create_opts.focus_mode = focus_mode;
  return create_plot_box(id, std::move(create_opts));
}

} // namespace iinuji
} // namespace cuwacunu
