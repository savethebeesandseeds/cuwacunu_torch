#pragma once

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef timeout
#undef timeout
#endif
#include <torch/torch.h>
#include <ncursesw/ncurses.h>

#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/types/types_data.h"
#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"
#include "piaabo/dconfig.h"

#include "iinuji/iinuji_cmd/views/data/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;
using Dataset_t = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
using ObsSample_t = cuwacunu::camahjucunu::data::observation_sample_t;

struct DataAppRuntime {
  bool ready{false};
  bool sample_ready{false};
  std::string error{};
  std::string signature{};
  Dataset_t dataset{};
  ObsSample_t sample{};
  std::size_t sample_index{0};
  std::size_t sample_count{0};
  std::size_t C{0};
  std::size_t T{0};
  std::size_t D{0};
  std::mt19937 rng{std::random_device{}()};
};

inline Rect merge_rects(const Rect& a, const Rect& b) {
  const int x0 = std::min(a.x, b.x);
  const int y0 = std::min(a.y, b.y);
  const int x1 = std::max(a.x + a.w, b.x + b.w);
  const int y1 = std::max(a.y + a.h, b.y + b.h);
  return Rect{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

inline std::optional<Rect> data_plot_overlay_area(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  if (state.screen != ScreenMode::Data || !state.data.plot_view) return std::nullopt;

  Rect area{};
  bool have_area = false;
  for (const auto& box : {left, right}) {
    const Rect r = content_rect(*box);
    if (r.w <= 0 || r.h <= 0) continue;
    if (!have_area) {
      area = r;
      have_area = true;
    } else {
      area = merge_rects(area, r);
    }
  }
  if (!have_area || area.w < 20 || area.h < 10) return std::nullopt;
  return area;
}

inline bool data_plot_overlay_close_hit(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
    int mouse_x,
    int mouse_y) {
  const auto area = data_plot_overlay_area(state, left, right);
  if (!area.has_value()) return false;
  const int close_x0 = area->x + std::max(0, area->w - 4);
  const int close_x1 = close_x0 + 2;  // "[x]"
  const int close_y = area->y;
  return mouse_y == close_y && mouse_x >= close_x0 && mouse_x <= close_x1;
}

inline torch::Tensor to_cpu_float_contig(const torch::Tensor& t) {
  if (!t.defined()) return torch::Tensor();
  return t.to(torch::kCPU).contiguous().to(torch::kFloat32);
}

inline torch::Tensor as_ctd(const torch::Tensor& t) {
  if (!t.defined()) return torch::Tensor();
  torch::Tensor x = t;
  if (x.dim() == 4) {
    if (x.size(0) <= 0) return torch::Tensor();
    x = x[0];
  }
  if (x.dim() == 2) x = x.unsqueeze(0);
  if (x.dim() != 3) return torch::Tensor();
  return to_cpu_float_contig(x);
}

inline torch::Tensor as_ct(const torch::Tensor& t) {
  if (!t.defined()) return torch::Tensor();
  torch::Tensor x = t;
  if (x.dim() == 3) {
    if (x.size(0) <= 0) return torch::Tensor();
    x = x[0];
  }
  if (x.dim() == 1) x = x.unsqueeze(0);
  if (x.dim() != 2) return torch::Tensor();
  return to_cpu_float_contig(x);
}

inline torch::Tensor as_ct_double(const torch::Tensor& t) {
  if (!t.defined()) return torch::Tensor();
  torch::Tensor x = t;
  if (x.dim() == 3) {
    if (x.size(0) <= 0) return torch::Tensor();
    x = x[0];
  }
  if (x.dim() == 1) x = x.unsqueeze(0);
  if (x.dim() != 2) return torch::Tensor();
  return x.to(torch::kCPU).contiguous().to(torch::kFloat64);
}

inline std::string data_plot_overlay_commands_hint() {
  std::ostringstream oss;
  oss << "cmds: " << canonical_paths::kDataPlotOn
      << " | " << canonical_paths::kDataPlotOff
      << " | " << canonical_paths::kDataPlotToggle
      << " | " << canonical_paths::kDataPlotModeSeq
      << " | " << canonical_paths::kDataPlotModeFuture
      << " | " << canonical_paths::kDataPlotModeWeight
      << " | " << canonical_paths::kDataPlotModeNorm
      << " | " << canonical_paths::kDataPlotModeBytes
      << " | " << canonical_paths::kDataAxisIdx
      << " | " << canonical_paths::kDataAxisKey
      << " | " << canonical_paths::kDataAxisToggle;
  return oss.str();
}

inline void sync_data_tensor_state(CmdState& state, const DataAppRuntime& rt) {
  state.data.plot_tensor_ready = (rt.ready && rt.sample_ready);
  state.data.plot_tensor_error = rt.error;
  state.data.plot_sample_count = rt.sample_count;
  state.data.plot_sample_index = rt.sample_index;
  state.data.plot_C = rt.C;
  state.data.plot_T = rt.T;
  state.data.plot_D = rt.D;
  clamp_data_plot_sample_index(state);
  clamp_data_plot_feature_dim(state);
}

inline std::string data_runtime_signature(const CmdState& state) {
  return state.data.focus_instrument + "|" + state.data.raw_instruction;
}

inline bool load_data_sample(CmdState& state, DataAppRuntime& rt, std::size_t idx) {
  if (!rt.ready || rt.sample_count == 0) {
    rt.sample_ready = false;
    rt.error = "dataset is not ready";
    rt.C = rt.T = rt.D = 0;
    sync_data_tensor_state(state, rt);
    return false;
  }
  idx = std::min(idx, rt.sample_count - 1);
  try {
    rt.sample = rt.dataset.get(idx);
    torch::Tensor x = as_ctd(rt.sample.features);
    if (!x.defined()) {
      rt.sample_ready = false;
      rt.error = "sample features are not [C,T,D]";
      rt.C = rt.T = rt.D = 0;
      sync_data_tensor_state(state, rt);
      return false;
    }
    rt.sample_ready = true;
    rt.error.clear();
    rt.sample_index = idx;
    rt.C = static_cast<std::size_t>(x.size(0));
    rt.T = static_cast<std::size_t>(x.size(1));
    rt.D = static_cast<std::size_t>(x.size(2));
    if (state.data.selected_channel >= rt.C && rt.C > 0) {
      state.data.selected_channel = 0;
    }
    if (state.data.plot_feature_dim >= rt.D && rt.D > 0) {
      state.data.plot_feature_dim = 0;
    }
    sync_data_tensor_state(state, rt);
    return true;
  } catch (const std::exception& e) {
    rt.sample_ready = false;
    rt.error = std::string("sample load failed: ") + e.what();
  } catch (...) {
    rt.sample_ready = false;
    rt.error = "sample load failed";
  }
  rt.C = rt.T = rt.D = 0;
  sync_data_tensor_state(state, rt);
  return false;
}

inline void init_data_runtime(CmdState& state, DataAppRuntime& rt, bool force) {
  if (!state.data.ok) {
    rt.ready = false;
    rt.sample_ready = false;
    rt.error = state.data.error.empty() ? "data view is invalid" : state.data.error;
    rt.sample_count = 0;
    rt.sample_index = 0;
    rt.C = rt.T = rt.D = 0;
    sync_data_tensor_state(state, rt);
    return;
  }

  const std::string sig = data_runtime_signature(state);
  if (!force && rt.ready && rt.signature == sig) {
    clamp_data_plot_sample_index(state);
    if (!rt.sample_ready || state.data.plot_sample_index != rt.sample_index) {
      load_data_sample(state, rt, state.data.plot_sample_index);
    } else {
      sync_data_tensor_state(state, rt);
    }
    return;
  }

  rt = DataAppRuntime{};
  rt.signature = sig;
  try {
    if (state.board.contract_hash.empty()) {
      rt.ready = false;
      rt.sample_ready = false;
      rt.error = "board contract hash is unavailable";
      rt.sample_count = 0;
      rt.sample_index = 0;
      rt.C = rt.T = rt.D = 0;
      sync_data_tensor_state(state, rt);
      return;
    }
    const std::string contract_hash = state.board.contract_hash;
    auto obs = cuwacunu::camahjucunu::decode_observation_spec_from_contract(
        contract_hash);
    std::string instrument = state.data.focus_instrument;
    if (instrument.empty() && !obs.source_forms.empty()) {
      instrument = obs.source_forms.front().instrument;
    }
    if (instrument.empty()) {
      rt.ready = false;
      rt.error = "no instrument resolved for dataset";
      sync_data_tensor_state(state, rt);
      return;
    }
    const bool force_rebuild_cache =
        cuwacunu::iitepi::config_space_t::get<bool>(
            "DATA_LOADER", "dataloader_force_rebuild_cache");
    rt.dataset = cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(
        instrument, obs, force_rebuild_cache);
    auto sz = rt.dataset.size();
    rt.sample_count = sz.has_value() ? *sz : 0;
    rt.ready = (rt.sample_count > 0);
    if (!rt.ready) {
      rt.error = "dataset has no samples";
      sync_data_tensor_state(state, rt);
      return;
    }
    clamp_data_plot_sample_index(state);
    if (state.data.plot_sample_index >= rt.sample_count) state.data.plot_sample_index = 0;
    load_data_sample(state, rt, state.data.plot_sample_index);
  } catch (const std::exception& e) {
    rt.ready = false;
    rt.sample_ready = false;
    rt.error = std::string("dataset init failed: ") + e.what();
    rt.sample_count = 0;
    rt.sample_index = 0;
    rt.C = rt.T = rt.D = 0;
    sync_data_tensor_state(state, rt);
  } catch (...) {
    rt.ready = false;
    rt.sample_ready = false;
    rt.error = "dataset init failed";
    rt.sample_count = 0;
    rt.sample_index = 0;
    rt.C = rt.T = rt.D = 0;
    sync_data_tensor_state(state, rt);
  }
}

inline bool step_data_sample(CmdState& state, DataAppRuntime& rt, int delta) {
  if (rt.sample_count == 0) return false;
  const long long n = static_cast<long long>(rt.sample_count);
  const long long cur = static_cast<long long>(state.data.plot_sample_index % rt.sample_count);
  long long next = (cur + static_cast<long long>(delta)) % n;
  if (next < 0) next += n;
  state.data.plot_sample_index = static_cast<std::size_t>(next);
  return load_data_sample(state, rt, state.data.plot_sample_index);
}

inline bool random_data_sample(CmdState& state, DataAppRuntime& rt) {
  if (rt.sample_count == 0) return false;
  std::uniform_int_distribution<std::size_t> dist(0, rt.sample_count - 1);
  state.data.plot_sample_index = dist(rt.rng);
  return load_data_sample(state, rt, state.data.plot_sample_index);
}

inline bool step_data_dim(CmdState& state, int delta) {
  if (state.data.plot_D == 0) return false;
  const long long n = static_cast<long long>(state.data.plot_D);
  const long long cur = static_cast<long long>(state.data.plot_feature_dim % state.data.plot_D);
  long long next = (cur + static_cast<long long>(delta)) % n;
  if (next < 0) next += n;
  state.data.plot_feature_dim = static_cast<std::size_t>(next);
  return true;
}

inline bool step_data_focus(CmdState& state, int delta) {
  const long long n = static_cast<long long>(data_nav_focus_count());
  if (n <= 0) return false;
  const long long cur = static_cast<long long>(static_cast<std::size_t>(state.data.nav_focus));
  long long next = (cur + static_cast<long long>(delta)) % n;
  if (next < 0) next += n;
  state.data.nav_focus = static_cast<DataNavFocus>(next);
  clamp_data_nav_focus(state);
  return true;
}

inline void render_data_plot_overlay(
    const CmdState& state,
    const DataAppRuntime& rt,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  const auto area_opt = data_plot_overlay_area(state, left, right);
  if (!area_opt.has_value()) return;
  auto* R = get_renderer();
  if (!R) return;

  const Rect area = *area_opt;

  const short bg_pair = static_cast<short>(get_color_pair("#D8E3ED", "#0F1218"));
  const short title_pair = static_cast<short>(get_color_pair("#F2F8FF", "#0F1218"));
  const short text_pair = static_cast<short>(get_color_pair("#C6D5E3", "#0F1218"));
  const short grid_pair = static_cast<short>(get_color_pair("#4A5562", "#0F1218"));
  const short selected_pair = static_cast<short>(get_color_pair("#FFD26E", "#0F1218"));
  const short warn_pair = static_cast<short>(get_color_pair("#FFB96B", "#0F1218"));
  const short mask_pair = static_cast<short>(get_color_pair("#FF4D4D", "#0F1218"));

  R->fillRect(area.y, area.x, area.h, area.w, bg_pair);
  {
    constexpr const char* kClose = "[x]";
    const int close_x = area.x + std::max(0, area.w - 4);
    R->putText(area.y, close_x, kClose, 3, selected_pair, true, false);
  }

  const int inner_x = area.x + 1;
  const int inner_y = area.y + 1;
  const int inner_w = std::max(0, area.w - 2);
  const int inner_h = std::max(0, area.h - 2);
  if (inner_w < 16 || inner_h < 8) return;

  const std::size_t meta_channel_count = state.data.channels.size();
  std::size_t selected_meta_idx = 0;
  if (meta_channel_count > 0) {
    selected_meta_idx = std::min(state.data.selected_channel, meta_channel_count - 1);
  }

  std::ostringstream header;
  std::string header_feature_name = "x";
  std::string header_feature_type = "double";
  if (meta_channel_count > 0 && state.data.plot_D > 0) {
    const auto& hdr_ch = state.data.channels[selected_meta_idx];
    const std::size_t hdr_dim = std::min(state.data.plot_feature_dim, state.data.plot_D - 1);
    header_feature_name = data_feature_name_for_channel_dim(hdr_ch, hdr_dim);
    header_feature_type = data_feature_type_for_channel_dim(hdr_ch, hdr_dim);
  }
  header << "F5 DATA PLOT | mode=" << data_plot_mode_name(state.data.plot_mode)
         << " (" << data_plot_mode_token(state.data.plot_mode) << ")"
         << " x=" << data_plot_x_axis_token(state.data.plot_x_axis)
         << " | sample="
         << (state.data.plot_sample_count == 0 ? 0 : (state.data.plot_sample_index + 1))
         << "/" << state.data.plot_sample_count
         << " dim="
         << (state.data.plot_D == 0 ? 0 : (state.data.plot_feature_dim + 1))
         << "/" << state.data.plot_D
         << " [" << header_feature_name << ":" << header_feature_type << "]";
  R->putText(inner_y, inner_x, trim_to_width(header.str(), inner_w), inner_w, title_pair, true, false);

  std::ostringstream subtitle;
  subtitle << "focus="
           << (state.data.focus_instrument.empty() ? "<none>" : state.data.focus_instrument)
           << " channels=" << meta_channel_count
           << " nav=" << data_nav_focus_name(state.data.nav_focus)
           << " x-axis=" << data_plot_x_axis_name(state.data.plot_x_axis)
           << " tensor[C,T,D]=[" << state.data.plot_C << "," << state.data.plot_T << "," << state.data.plot_D << "]";
  R->putText(inner_y + 1, inner_x, trim_to_width(subtitle.str(), inner_w), inner_w, text_pair);

  constexpr int kHeaderRows = 2;
  constexpr int kFooterRows = 3;
  const int plot_x = inner_x;
  const int plot_y = inner_y + kHeaderRows;
  const int plot_w = inner_w;
  const int plot_h = inner_h - kHeaderRows - kFooterRows;

  bool plotted = false;
  std::string selected_line_text = "selected [n/a]";

  if (plot_w < 18 || plot_h < 8) {
    R->putText(plot_y, plot_x, "screen too small for plot view", plot_w, warn_pair);
  } else if ((state.data.plot_mode == DataPlotMode::SeqLength ||
              state.data.plot_mode == DataPlotMode::FutureSeqLength) &&
             rt.ready && rt.sample_ready) {
    const bool future_mode = (state.data.plot_mode == DataPlotMode::FutureSeqLength);
    torch::Tensor x = as_ctd(future_mode ? rt.sample.future_features : rt.sample.features);
    if (x.defined()) {
      torch::Tensor m = as_ct(future_mode ? rt.sample.future_mask : rt.sample.mask);
      torch::Tensor k = as_ct_double(future_mode ? rt.sample.future_keys : rt.sample.past_keys);
      const std::size_t C = static_cast<std::size_t>(x.size(0));
      const std::size_t T = static_cast<std::size_t>(x.size(1));
      const std::size_t D = static_cast<std::size_t>(x.size(2));
      const bool want_key_axis = (state.data.plot_x_axis == DataPlotXAxis::KeyValue);
      const bool keys_shape_ok =
          k.defined() &&
          k.dim() == 2 &&
          static_cast<std::size_t>(k.size(1)) == T &&
          (static_cast<std::size_t>(k.size(0)) == C || static_cast<std::size_t>(k.size(0)) == 1);
      const bool use_key_axis = want_key_axis && keys_shape_ok;
      const bool key_broadcast = use_key_axis && (static_cast<std::size_t>(k.size(0)) == 1);
      if (!m.defined() || m.size(0) != x.size(0) || m.size(1) != x.size(1)) {
        m = torch::ones({x.size(0), x.size(1)}, torch::TensorOptions().dtype(torch::kFloat32));
      }
      if (state.data.plot_feature_dim >= D && D > 0) {
        // clamped by key handlers/commands.
      }
      const std::size_t d_sel = std::min(state.data.plot_feature_dim, D > 0 ? D - 1 : 0);
      const std::size_t c_sel = (C > 0) ? std::min(state.data.selected_channel, C - 1) : 0;
      std::string d_name = "x" + std::to_string(d_sel);
      std::string d_type = "double";
      if (meta_channel_count > 0) {
        const auto& dch = state.data.channels[selected_meta_idx];
        d_name = data_feature_name_for_channel_dim(dch, d_sel);
        d_type = data_feature_type_for_channel_dim(dch, d_sel);
      }

      static const char* kPalette[] = {
          "#F94144", "#277DA1", "#efef09", "#43AA8B",
          "#577590", "#90BE6D", "#4D908E", "#F9C74F", "#b0b0b0"};
      constexpr int kPaletteN = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));

      std::vector<std::vector<std::pair<double, double>>> line_series(C);
      std::vector<std::vector<std::pair<double, double>>> missing_series(C);
      for (std::size_t c = 0; c < C; ++c) {
        auto& line = line_series[c];
        auto& miss = missing_series[c];
        line.reserve(T);
        miss.reserve(T / 4);
        float last_valid = 0.0f;
        bool have_last = false;
        for (std::size_t t = 0; t < T; ++t) {
          double xv = static_cast<double>(t);
          if (use_key_axis) {
            const std::size_t kc = key_broadcast ? 0 : c;
            const double kx = k[static_cast<long>(kc)][static_cast<long>(t)].item<double>();
            if (std::isfinite(kx)) xv = kx;
          }
          const float v = x[static_cast<long>(c)][static_cast<long>(t)][static_cast<long>(d_sel)].item<float>();
          const float mv = m[static_cast<long>(c)][static_cast<long>(t)].item<float>();
          if (mv > 0.5f) {
            line.emplace_back(xv, static_cast<double>(v));
            have_last = true;
            last_valid = v;
          } else {
            line.emplace_back(xv, std::numeric_limits<double>::quiet_NaN());
            if (state.data.plot_mask_overlay) {
              if (use_key_axis && xv <= 0.0) continue;
              const double my = static_cast<double>(have_last ? last_valid : v);
              miss.emplace_back(xv, my);
            }
          }
        }
      }

      std::vector<std::pair<double, double>> selected_line =
          (C > 0) ? line_series[c_sel] : std::vector<std::pair<double, double>>{};
      std::vector<Series> series;
      series.reserve(C * 2 + 1);
      for (std::size_t c = 0; c < C; ++c) {
        SeriesStyle line_style{};
        line_style.color_pair = static_cast<short>(get_color_pair(kPalette[c % kPaletteN], "#0F1218"));
        line_style.mode = PlotMode::Line;
        line_style.scatter = false;
        series.push_back(Series{&line_series[c], line_style});
        if (state.data.plot_mask_overlay && !missing_series[c].empty()) {
          SeriesStyle miss_style{};
          miss_style.color_pair = mask_pair;
          miss_style.mode = PlotMode::Scatter;
          miss_style.scatter = true;
          miss_style.scatter_every = 1;
          series.push_back(Series{&missing_series[c], miss_style});
        }
      }
      if (C > 0) {
        SeriesStyle selected_style{};
        selected_style.color_pair = selected_pair;
        selected_style.mode = PlotMode::Line;
        selected_style.scatter = true;
        selected_style.scatter_every = 1;
        series.push_back(Series{&selected_line, selected_style});
      }

      PlotOptions opt{};
      opt.margin_left = std::min(10, std::max(6, plot_w / 10));
      opt.margin_right = 2;
      opt.margin_top = 1;
      opt.margin_bot = 2;
      opt.draw_axes = true;
      opt.draw_grid = true;
      opt.baseline0 = true;
      opt.x_ticks = std::max(3, std::min(8, static_cast<int>(T)));
      opt.y_ticks = 5;
      if (!use_key_axis) {
        opt.x_min = 0.0;
        opt.x_max = std::max(1.0, static_cast<double>(T > 0 ? (T - 1) : 1));
      }
      opt.x_label = use_key_axis
                        ? (future_mode ? "future key_value (key_type_t)" : "sequence key_value (key_type_t)")
                        : (future_mode ? "future index (h)" : "sequence index (t)");
      opt.y_label = d_name;
      opt.bg_color_pair = bg_pair;
      opt.axes_color_pair = text_pair;
      opt.grid_color_pair = grid_pair;

      plot_braille_multi(series, plot_x, plot_y, plot_w, plot_h, opt);
      plotted = true;

      std::ostringstream sel;
      sel << "selected ch=" << (C > 0 ? (c_sel + 1) : 0) << "/" << C
          << " dim=" << (D > 0 ? (d_sel + 1) : 0) << "/" << D
          << " [" << d_name << ":" << d_type << "]"
          << " x=" << data_plot_x_axis_token(use_key_axis ? DataPlotXAxis::KeyValue : DataPlotXAxis::Index)
          << " stream=" << (future_mode ? "future" : "past")
          << " mask=" << (state.data.plot_mask_overlay ? "on" : "off");
      if (want_key_axis && !use_key_axis) sel << " (fallback=idx)";
      selected_line_text = sel.str();
    } else {
      R->putText(plot_y, plot_x, "tensor sample missing sequence data", plot_w, warn_pair);
    }
  }

  if (!plotted) {
    if (meta_channel_count == 0) {
      R->putText(plot_y, plot_x, "no active channels found in observation spec", plot_w, warn_pair);
    } else if (!data_plot_mode_is_dynamic(state.data.plot_mode)) {
      R->putText(
          plot_y,
          plot_x,
          "static mode selected (weight/norm/bytes) - values shown in view panel",
          plot_w,
          warn_pair);
      int y = plot_y + 2;
      for (std::size_t i = 0; i < meta_channel_count && y < (plot_y + plot_h); ++i, ++y) {
        const auto& ch = state.data.channels[i];
        const bool active = (i == selected_meta_idx);
        const double v = plot_value_for_channel(ch, state.data.plot_mode);
        std::ostringstream row;
        row << (active ? ">" : " ") << "[" << (i + 1) << "] "
            << ch.interval << "/" << ch.record_type
            << " value=" << format_plot_value(v, state.data.plot_mode);
        R->putText(y, plot_x, trim_to_width(row.str(), plot_w), plot_w, active ? selected_pair : text_pair);
      }
      selected_line_text =
          "static mode: no overlay plot (switch mode to seq/future to render curves)";
    } else {
      const char* reason = (!rt.ready || !rt.sample_ready)
                               ? "tensor sample not ready for seq/future plot"
                               : "unable to render seq/future plot";
      R->putText(plot_y, plot_x, reason, plot_w, warn_pair);
      if (!state.data.plot_tensor_error.empty()) {
        R->putText(plot_y + 1, plot_x, trim_to_width(state.data.plot_tensor_error, plot_w), plot_w, warn_pair);
      }
      selected_line_text = reason;
    }
  }

  const int footer_y = inner_y + inner_h - kFooterRows;
  R->putText(
      footer_y,
      inner_x,
      "keys: Up/Down select focus | Left/Right change focused state | Esc or [x] close plot | printable keys -> cmd>",
      inner_w,
      text_pair);
  R->putText(
      footer_y + 1,
      inner_x,
      trim_to_width(data_plot_overlay_commands_hint(), inner_w),
      inner_w,
      text_pair);
  R->putText(footer_y + 2, inner_x, trim_to_width(selected_line_text, inner_w), inner_w, selected_pair);
}

template <class AppendLog, class ClosePlot>
inline bool handle_data_key(CmdState& state,
                            DataAppRuntime& rt,
                            int ch,
                            AppendLog&& append_log,
                            ClosePlot&& close_plot) {
  if (state.screen != ScreenMode::Data || !state.cmdline.empty()) return false;

  if (ch == 27 && state.data.plot_view) {
    close_plot();
    append_log("data.plotview=off (esc)", "nav", "#d0d0d0");
    return true;
  }

  if (ch == KEY_UP) {
    if (step_data_focus(state, -1)) {
      append_log("data.focus=" + data_nav_focus_name(state.data.nav_focus), "nav", "#d0d0d0");
    }
    return true;
  }
  if (ch == KEY_DOWN) {
    if (step_data_focus(state, +1)) {
      append_log("data.focus=" + data_nav_focus_name(state.data.nav_focus), "nav", "#d0d0d0");
    }
    return true;
  }
  if (ch == KEY_RIGHT || ch == KEY_LEFT) {
    const int delta = (ch == KEY_RIGHT) ? +1 : -1;
    switch (state.data.nav_focus) {
      case DataNavFocus::Channel: {
        if (delta > 0) select_next_data_channel(state);
        else select_prev_data_channel(state);
        append_log("data.channel=" + std::to_string(state.data.selected_channel + 1), "nav", "#d0d0d0");
      } break;
      case DataNavFocus::Sample: {
        if (step_data_sample(state, rt, delta)) {
          append_log("data.sample=" + std::to_string(state.data.plot_sample_index + 1), "nav", "#d0d0d0");
        } else if (!state.data.plot_tensor_error.empty()) {
          append_log(state.data.plot_tensor_error, "warn", "#ffd27f");
        }
      } break;
      case DataNavFocus::Dim: {
        if (step_data_dim(state, delta)) {
          append_log("data.dim=" + std::to_string(state.data.plot_feature_dim + 1), "nav", "#d0d0d0");
        }
      } break;
      case DataNavFocus::PlotMode: {
        state.data.plot_mode =
            (delta > 0) ? next_data_plot_mode(state.data.plot_mode) : prev_data_plot_mode(state.data.plot_mode);
        clamp_data_plot_mode(state);
        append_log("data.plot=" + data_plot_mode_token(state.data.plot_mode), "nav", "#d0d0d0");
      } break;
      case DataNavFocus::XAxis: {
        state.data.plot_x_axis = next_data_plot_x_axis(state.data.plot_x_axis);
        clamp_data_plot_x_axis(state);
        append_log("data.x=" + data_plot_x_axis_token(state.data.plot_x_axis), "nav", "#d0d0d0");
      } break;
      case DataNavFocus::Mask: {
        if (delta > 0) state.data.plot_mask_overlay = true;
        else state.data.plot_mask_overlay = false;
        append_log(std::string("data.mask=") + (state.data.plot_mask_overlay ? "on" : "off"), "nav", "#d0d0d0");
      } break;
    }
    return true;
  }

  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
