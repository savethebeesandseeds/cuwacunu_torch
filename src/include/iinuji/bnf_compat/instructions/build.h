#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"

#include "iinuji/bnf_compat/instructions/diag.h"
#include "iinuji/bnf_compat/instructions/data.h"
#include "iinuji/bnf_compat/instructions/options.h"
#include "iinuji/bnf_compat/instructions/helpers.h"
#include "iinuji/bnf_compat/instructions/form.h"
#include "iinuji/bnf_compat/instructions/validation.h"

#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

RUNTIME_WARNING("(iinuji/build.h)[] absolute layout clamps width/height with max(0, ...); 0-sized widgets may break renderers. Consider max(1, ...) for figure/panel sizes.\n");
RUNTIME_WARNING("(iinuji/build.h)[] normalized layout allows x/y/width/height outside [0,1] if coords exceed design; clamp or document expected behavior.\n");
RUNTIME_WARNING("(iinuji/build.h)[] buffer capacity should be guarded at build/runtime even if validator enforces it; prevents size_t underflow/0-cap bugs.\n");

namespace cuwacunu {
namespace iinuji {

/* Build result + runtime dispatch maps */
struct instructions_build_result_t {
  instructions_diag_t diag;
  std::shared_ptr<iinuji_object_t> root;

  resolved_event_map_t events_by_name;

  std::unordered_map<std::string, std::vector<std::string>> triggers_by_figure_id;
  std::unordered_map<std::string, std::vector<std::string>> figures_for_event;
  std::unordered_map<std::string, std::shared_ptr<iinuji_object_t>> figure_object_by_id;
  std::unordered_map<std::string, std::string> figure_kind_by_id;
};

inline std::shared_ptr<iinuji_object_t>
make_error_box(const std::string& id, const std::string& msg, const iinuji_layout_t& lay) {
  iinuji_style_t sty{};
  sty.border = true;
  sty.title = id;
  sty.label_color = "white";
  sty.background_color = "red";
  sty.border_color = "white";
  sty.bold = true;
  return create_text_box(id, msg, /*wrap*/true, text_align_t::Left, lay, sty);
}

inline std::shared_ptr<iinuji_object_t>
build_figure_object(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                    const cuwacunu::camahjucunu::iinuji_panel_t& P,
                    const cuwacunu::camahjucunu::iinuji_figure_t& F,
                    const std::string& figure_id,
                    const resolved_event_map_t& evmap,
                    const IInstructionsData& data,
                    const instructions_build_opts_t& bopt,
                    const instructions_validate_opts_t& vopt)
{
  const std::string fg = pick_color(F.text_color, P.text_color, sc.text_color, "white");
  const std::string bg = pick_color(F.back_color, P.back_color, sc.back_color, "<empty>");
  const std::string ln = pick_color(F.line_color, P.line_color, sc.line_color, "gray");

  // Layout
  iinuji_layout_t lay{};
  constexpr double U = 100.0;
  lay.x      = std::clamp(F.coords.x / U, 0.0, 1.0);
  lay.y      = std::clamp(F.coords.y / U, 0.0, 1.0);
  lay.width  = std::clamp(F.shape.x / U, 0.0, 1.0);
  lay.height = std::clamp(F.shape.y / U, 0.0, 1.0);

  // Style
  iinuji_style_t sty{};
  sty.border = F.border;
  sty.title  = (F.title_on && !is_unset_token(F.title)) ? F.title : "";
  sty.label_color = fg;
  sty.background_color = bg;
  sty.border_color = ln;

  auto find_initial_binding = [&](bind_kind_e want, const std::string& want_ev_kind)->const resolved_binding_t* {
    for (const auto& trig : F.triggers) {
      if (is_unset_token(trig)) continue;
      auto it = evmap.find(trig);
      if (it == evmap.end()) continue;
      const auto& E = it->second;
      if (vopt.enforce_event_kind_by_figure && E.kind_raw != want_ev_kind) continue;
      if (auto* b = first_binding_of_kind(E, want)) return b;
    }
    return nullptr;
  };

  const bind_kind_e want_bind = required_bind_kind_for_figure(F.kind_raw);
  const std::string want_ev_kind = required_event_kind_for_figure(F.kind_raw);
  const resolved_binding_t* src = find_initial_binding(want_bind, want_ev_kind);

  if (F.kind_raw == "_label") {
    std::string content = F.value;
    if (src && src->ref.kind == data_kind_e::Str) {
      std::string tmp;
      if (data.get_str(src->ref.index, tmp)) content = tmp;
    }
    return create_text_box(figure_id, content, /*wrap*/true, text_align_t::Left, lay, sty);
  }

  if (F.kind_raw == "_input_box") {
    std::string content = F.value;
    if (src && src->ref.kind == data_kind_e::Str) {
      std::string tmp;
      if (data.get_str(src->ref.index, tmp)) content = tmp;
    }
    return create_text_box(figure_id, content, /*wrap*/true, text_align_t::Left, lay, sty);
  }

  if (F.kind_raw == "_buffer") {
    // capacity from spec, but never allow 0/negative to reach bufferBox_data_t
    constexpr std::size_t kDefaultCap = 1000;

    std::size_t cap = kDefaultCap;
    if (F.capacity > 0) cap = (std::size_t)F.capacity;

    buffer_dir_t dir = buffer_dir_t::UpDown;
    std::string t;
    if (!is_unset_token(F.type_raw)) t = to_lower(F.type_raw);
    if (t == "downup") dir = buffer_dir_t::DownUp;

    auto o = create_object(figure_id, true, lay, sty);
    o->data = std::make_shared<bufferBox_data_t>(cap, dir);

    return o;
  }

  if (F.kind_raw == "_horizontal_plot") {
    std::vector<std::pair<double,double>> pts;
    if (src && src->ref.kind == data_kind_e::Vec) {
      (void)data.get_vec(src->ref.index, pts);
    }

    std::vector<std::vector<std::pair<double,double>>> series;
    series.push_back(std::move(pts));

    std::vector<plot_series_cfg_t> cfg;
    plot_series_cfg_t c{};
    c.color_fg = ln;
    c.mode = parse_plot_mode(F.type_raw);
    cfg.push_back(c);

    plotbox_opts_t pbopt{};
    pbopt.draw_grid = true;
    pbopt.draw_axes = true;

    auto o = create_plot_box(figure_id, series, cfg, pbopt, lay, sty);

    if (bopt.plot_legend_overlay && F.legend_on && !is_unset_token(F.legend)) {
      iinuji_layout_t l2{};
      l2.mode = layout_mode_t::Absolute;
      l2.normalized = false;
      l2.x = 1; l2.y = 0;
      l2.width  = (int)std::max<size_t>(1, std::min<size_t>(F.legend.size()+2, 50));
      l2.height = 1;

      iinuji_style_t s2 = sty;
      s2.border = false;
      s2.title.clear();

      auto legend = create_text_box(join_path(figure_id, "legend"), F.legend, /*wrap*/false,
                                    text_align_t::Left, l2, s2);
      o->add_child(legend);
    }

    return o;
  }

  if (bopt.render_placeholders_on_error) {
    return make_error_box(figure_id, "unsupported figure kind: " + F.kind_raw, lay);
  }
  return nullptr;
}

inline instructions_build_result_t
build_ui_for_screen(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& inst,
                    std::size_t screen_index,
                    const IInstructionsData& data,
                    int term_cols,
                    int term_rows,
                    const instructions_build_opts_t& bopt = {},
                    const instructions_validate_opts_t& vopt = {})
{
  instructions_build_result_t out;

  if (screen_index >= inst.screens.size()) {
    out.diag.err("build_ui_for_screen: screen_index out of range");
    return out;
  }
  const auto& sc = inst.screens[screen_index];

  auto contract = validate_and_compile_screen(sc, screen_index, vopt);
  out.diag.merge(contract.diag);
  out.events_by_name = contract.events;

  out.diag.merge(validate_data_access(out.events_by_name, data));
  if (!out.diag.ok()) return out;

  // Root is the terminal-sized container in CELLS (absolute)
  auto root = create_object(sanitize_id(sc.name));
  root->layout.mode = layout_mode_t::Absolute;
  root->layout.normalized = false;
  root->layout.x = 0; root->layout.y = 0;
  root->layout.width  = std::max(0, term_cols);
  root->layout.height = std::max(0, term_rows);

  root->style.border = sc.border;
  root->style.title  = (!is_unset_token(sc.name) ? sc.name : std::string("screen"));
  root->style.label_color = pick_color("<empty>", "<empty>", sc.text_color, "white");
  root->style.background_color = pick_color("<empty>", "<empty>", sc.back_color, "<empty>");
  root->style.border_color = pick_color("<empty>", "<empty>", sc.line_color, "gray");

  std::vector<std::pair<int,const cuwacunu::camahjucunu::iinuji_panel_t*>> panels;
  panels.reserve(sc.panels.size());
  for (const auto& P : sc.panels) panels.push_back({P.z_index, &P});
  std::stable_sort(panels.begin(), panels.end(),
                   [](auto& a, auto& b){ return a.first < b.first; });

  for (size_t pi=0; pi<panels.size(); ++pi) {
    const auto& P = *panels[pi].second;
    const std::string panel_id = mk_panel_id(sc.name, (int)pi);

    iinuji_layout_t lay{};
    constexpr double U = 100.0;

    // panel percent-of-screen [0..100] -> normalized [0..1]
    lay.mode = layout_mode_t::Normalized;
    lay.normalized = true;
    lay.x      = std::clamp(P.coords.x / U, 0.0, 1.0);
    lay.y      = std::clamp(P.coords.y / U, 0.0, 1.0);
    lay.width  = std::clamp(P.shape.x / U, 0.0, 1.0);
    lay.height = std::clamp(P.shape.y / U, 0.0, 1.0);

    iinuji_style_t sty{};
    sty.border = P.border;
    sty.title = (P.title_on && !is_unset_token(P.title)) ? P.title : "";
    sty.label_color = pick_color("<empty>", P.text_color, sc.text_color, "white");
    sty.background_color = pick_color("<empty>", P.back_color, sc.back_color, "<empty>");
    sty.border_color = pick_color("<empty>", P.line_color, sc.line_color, "gray");

    auto pobj = create_object(panel_id, true, lay, sty);
    pobj->z_index = P.z_index;

    for (size_t fi=0; fi<P.figures.size(); ++fi) {
      const auto& F = P.figures[fi];
      const std::string fig_id = mk_figure_id(sc.name, (int)pi, (int)fi, F.kind_raw);

      out.triggers_by_figure_id[fig_id] = F.triggers;
      out.figure_kind_by_id[fig_id] = F.kind_raw;

      for (const auto& trig : F.triggers) {
        if (!is_unset_token(trig)) out.figures_for_event[trig].push_back(fig_id);
      }

      // IMPORTANT: always normalized now (percent units)
      auto fobj = build_figure_object(sc, P, F, fig_id,
                                      out.events_by_name, data,
                                      bopt, vopt);

      if (fobj) {
        out.figure_object_by_id[fig_id] = fobj;
        pobj->add_child(fobj);
      } else if (bopt.render_placeholders_on_error) {
        iinuji_layout_t el{};
        el.mode = layout_mode_t::Absolute;
        el.normalized = false;
        el.x = 0; el.y = 0; el.width = 30; el.height = 3;
        pobj->add_child(make_error_box(fig_id, "figure build failed", el));
      }
    }

    root->add_child(pobj);
  }

  out.root = root;
  return out;
}

} // namespace iinuji
} // namespace cuwacunu
