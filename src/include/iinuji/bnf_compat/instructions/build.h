#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <fstream>

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
  
  // Global bottom section (DSL-defined via footer template)
  std::shared_ptr<iinuji_object_t> bottom;      // container at reserved rows
  std::shared_ptr<iinuji_object_t> menu_bar;    // status label (kept name for compatibility)
  std::shared_ptr<iinuji_object_t> terminal_input;
  std::string terminal_input_id;

  resolved_event_map_t events_by_name;

  std::unordered_map<std::string, std::vector<std::string>> triggers_by_figure_id;
  std::unordered_map<std::string, std::vector<std::string>> figures_for_event;
  std::unordered_map<std::string, std::shared_ptr<iinuji_object_t>> figure_object_by_id;
  std::unordered_map<std::string, std::string> figure_kind_by_id;

  // Tab navigation (stable order: panels by z_index, then figure declaration order)
  std::vector<std::string> focus_order;
  std::size_t focus_index = (std::size_t)-1;
};

// Focus policy:
// Only interactive widgets participate in focus navigation.
inline bool is_focusable_figure_kind(const std::string& kind_raw)
{
  return (kind_raw == "_input_box" || kind_raw == "_horizontal_plot" || kind_raw == "_text_editor");
}

inline void apply_focus_policy(const std::shared_ptr<iinuji_object_t>& o,
                               const std::string& kind_raw)
{
  if (!o) return;
  // NOTE: assumes iinuji_object_t exposes a "focusable" boolean (or similar).
  // If your field is named differently, change it here once.
  o->focusable = is_focusable_figure_kind(kind_raw);
}

inline std::shared_ptr<iinuji_object_t>
make_error_box(const std::string& id, const std::string& msg, const iinuji_layout_t& lay) {
  iinuji_style_t sty{};
  sty.border = true;
  sty.title = id;
  sty.label_color = "white";
  sty.background_color = "red";
  sty.border_color = "white";
  sty.bold = true;
  auto o = create_text_box(id, msg, /*wrap*/true, text_align_t::Left, lay, sty);
  if (o) o->focusable = false;
  return o;
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
      if (vopt.enforce_event_kind_by_figure && E.kind_raw != want_ev_kind && F.kind_raw != "_text_editor") continue;
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
    bool wrap = true;
    if (!is_unset_token(F.type_raw)) {
      std::string tt = to_lower(F.type_raw);
      if (tt == "nowrap") wrap = false;
    }

    auto o = create_text_box(figure_id, content, /*wrap*/wrap, text_align_t::Left, lay, sty);
    apply_focus_policy(o, F.kind_raw); // label -> not focusable
    return o;
  }

  if (F.kind_raw == "_input_box") {
    std::string content = F.value;
    if (src && src->ref.kind == data_kind_e::Str) {
      std::string tmp;
      if (data.get_str(src->ref.index, tmp)) content = tmp;
    }
    auto o = create_text_box(figure_id, content, /*wrap*/false, text_align_t::Left, lay, sty);
    apply_focus_policy(o, F.kind_raw); // focusable = true
    return o;
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
    if (o) o->focusable = false;

    return o;
  }

  if (F.kind_raw == "_text_editor") {
    std::string path = F.value;
    if (src && src->ref.kind == data_kind_e::Str) {
      std::string tmp;
      if (data.get_str(src->ref.index, tmp)) path = tmp;
    }

    auto o = create_object(figure_id, true, lay, sty);
    auto ed = std::make_shared<editorBox_data_t>(path);

    if (!is_unset_token(F.type_raw)) {
      std::string tt = to_lower(F.type_raw);
      if (tt == "readonly" || tt == "ro") ed->read_only = true;
    }

    if (!path.empty()) {
      std::ifstream in(path);
      if (in) {
        ed->lines.clear();
        std::string line;
        while (std::getline(in, line)) {
          if (!line.empty() && line.back() == '\r') line.pop_back();
          ed->lines.push_back(std::move(line));
        }
        if (ed->lines.empty()) ed->lines.emplace_back();
        ed->dirty = false;
        ed->status.clear();
      } else {
        ed->status = "open failed";
        ed->dirty = false;
      }
    }

    o->data = ed;
    apply_focus_policy(o, F.kind_raw);
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
    apply_focus_policy(o, F.kind_raw); // focusable = true

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
      if (legend) legend->focusable = false;
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
                    const instructions_validate_opts_t& vopt = {},
                    const cuwacunu::camahjucunu::iinuji_screen_t* footer_sc = nullptr)
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

  const int full_w = std::max(0, term_cols);
  const int full_h = std::max(0, term_rows);

  const int menu_rows =
    (bopt.show_global_menu_bar ? std::clamp(bopt.global_menu_rows, 0, full_h) : 0);
  const int content_h = std::max(0, full_h - menu_rows);

  const std::string root_id = sanitize_id(sc.name);

  // Outer root spans the entire terminal. We keep it borderless so the menu bar
  // can live on the last row without fighting the screen border.
  auto root = create_object(root_id);
  root->layout.mode = layout_mode_t::Absolute;
  root->layout.normalized = false;
  root->layout.x = 0; root->layout.y = 0;
  root->layout.width  = full_w;
  root->layout.height = full_h;

  root->style.border = false;
  root->style.title.clear();
  root->style.label_color = pick_color("<empty>", "<empty>", sc.text_color, "white");
  root->style.background_color = pick_color("<empty>", "<empty>", sc.back_color, "<empty>");
  root->style.border_color = pick_color("<empty>", "<empty>", sc.line_color, "gray");

  // Content container: this is what the DSL 0..100 normalized coordinates map to.
  auto content = create_object(join_path(root_id, "content"));
  content->layout.mode = layout_mode_t::Absolute;
  content->layout.normalized = false;
  content->layout.x = 0; content->layout.y = 0;
  content->layout.width  = full_w;
  content->layout.height = content_h;

  content->style.border = sc.border;
  content->style.title  = (!is_unset_token(sc.name) ? sc.name : std::string("screen"));
  content->style.label_color = pick_color("<empty>", "<empty>", sc.text_color, "white");
  content->style.background_color = pick_color("<empty>", "<empty>", sc.back_color, "<empty>");
  content->style.border_color = pick_color("<empty>", "<empty>", sc.line_color, "gray");

  root->add_child(content);

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
        apply_focus_policy(fobj, F.kind_raw);
        out.figure_object_by_id[fig_id] = fobj;
        if (fobj->focusable) out.focus_order.push_back(fig_id);
        pobj->add_child(fobj);
      } else if (bopt.render_placeholders_on_error) {
        iinuji_layout_t el{};
        el.mode = layout_mode_t::Absolute;
        el.normalized = false;
        el.x = 0; el.y = 0; el.width = 30; el.height = 3;
        pobj->add_child(make_error_box(fig_id, "figure build failed", el));
      }
    }

    content->add_child(pobj);
  }

  // Global bottom section (reserved rows at the bottom).
  // If footer_sc is provided, we build it from DSL as panels/figures.
  // Otherwise we fall back to the legacy single text line.
  if (menu_rows > 0 && full_w > 0 && bopt.show_global_menu_bar) {

    iinuji_layout_t bl{};
    bl.mode = layout_mode_t::Absolute;
    bl.normalized = false;
    bl.x = 0; bl.y = content_h;
    bl.width  = full_w;
    bl.height = menu_rows;

    // base colors: prefer footer screen colors, fall back to active screen
    const std::string base_fg =
      footer_sc ? pick_color("<empty>", "<empty>", footer_sc->text_color,
                             pick_color("<empty>", "<empty>", sc.text_color, "white"))
                : pick_color("<empty>", "<empty>", sc.text_color, "white");

    const std::string base_bg =
      footer_sc ? pick_color("<empty>", "<empty>", footer_sc->back_color,
                             pick_color("<empty>", "<empty>", sc.back_color, "<empty>"))
                : pick_color("<empty>", "<empty>", sc.back_color, "<empty>");

    const std::string base_ln =
      footer_sc ? pick_color("<empty>", "<empty>", footer_sc->line_color,
                             pick_color("<empty>", "<empty>", sc.line_color, "gray"))
                : pick_color("<empty>", "<empty>", sc.line_color, "gray");

    iinuji_style_t bs{};
    bs.border = false;
    bs.title.clear();
    bs.label_color = base_fg;
    bs.background_color = base_bg;
    bs.border_color = base_ln;

    out.bottom = create_object(join_path(root_id, "bottom"), true, bl, bs);
    if (out.bottom) out.bottom->focusable = false;
    root->add_child(out.bottom);

    // --- DSL footer path ---
    if (footer_sc && out.bottom && !footer_sc->panels.empty()) {

      // stable base so footer ids are unique per screen root:
      const std::string footer_base = join_path(root_id, "bottom");

      // Sort footer panels by z_index like normal panels
      std::vector<std::pair<int,const cuwacunu::camahjucunu::iinuji_panel_t*>> fp;
      fp.reserve(footer_sc->panels.size());
      for (const auto& P : footer_sc->panels) fp.push_back({P.z_index, &P});
      std::stable_sort(fp.begin(), fp.end(),
                       [](auto& a, auto& b){ return a.first < b.first; });

      for (size_t pi=0; pi<fp.size(); ++pi) {
        const auto& P = *fp[pi].second;
        const std::string panel_id = mk_panel_id(footer_base, (int)pi);

        iinuji_layout_t lay{};
        constexpr double U = 100.0;
        lay.mode = layout_mode_t::Normalized;
        lay.normalized = true;
        lay.x      = std::clamp(P.coords.x / U, 0.0, 1.0);
        lay.y      = std::clamp(P.coords.y / U, 0.0, 1.0);
        lay.width  = std::clamp(P.shape.x / U, 0.0, 1.0);
        lay.height = std::clamp(P.shape.y / U, 0.0, 1.0);

        iinuji_style_t sty{};
        sty.border = P.border;
        sty.title = (P.title_on && !is_unset_token(P.title)) ? P.title : "";
        sty.label_color = pick_color("<empty>", P.text_color, footer_sc->text_color, base_fg);
        sty.background_color = pick_color("<empty>", P.back_color, footer_sc->back_color, base_bg);
        sty.border_color = pick_color("<empty>", P.line_color, footer_sc->line_color, base_ln);

        auto pobj = create_object(panel_id, true, lay, sty);
        pobj->z_index = P.z_index;

        for (size_t fi=0; fi<P.figures.size(); ++fi) {
          const auto& F = P.figures[fi];
          const std::string fig_id = mk_figure_id(footer_base, (int)pi, (int)fi, F.kind_raw);

          // keep mappings (useful for input handling)
          out.triggers_by_figure_id[fig_id] = F.triggers;
          out.figure_kind_by_id[fig_id] = F.kind_raw;

          // NOTE: footer figures don't participate in figures_for_event unless you
          // intentionally wire them to events present in the main screen event map.
          for (const auto& trig : F.triggers) {
            if (!is_unset_token(trig)) out.figures_for_event[trig].push_back(fig_id);
          }

          auto fobj = build_figure_object(*footer_sc, P, F, fig_id,
                                          out.events_by_name, data,
                                          bopt, vopt);

          if (fobj) {
            apply_focus_policy(fobj, F.kind_raw);
            out.figure_object_by_id[fig_id] = fobj;
            if (fobj->focusable) out.focus_order.push_back(fig_id);

            // First footer label becomes menu/status target
            if (!out.menu_bar && F.kind_raw == "_label") {
              out.menu_bar = fobj;
            }
            // First footer input becomes terminal
            if (!out.terminal_input && F.kind_raw == "_input_box") {
              out.terminal_input = fobj;
              out.terminal_input_id = fig_id;
            }

            pobj->add_child(fobj);
          }
        }

        out.bottom->add_child(pobj);
      }

    } else {
      // --- Legacy fallback: status label + terminal input (ALWAYS) ---

      auto build_menu_text = [&]()->std::string {
        std::ostringstream oss;
        oss << "F+N: switch screens";
        std::string s = oss.str();
        if ((int)s.size() > full_w) {
          if (full_w >= 3) s = s.substr(0, (size_t)full_w - 3) + "...";
          else s = s.substr(0, (size_t)full_w);
        }
        return s;
      };

      // split: 65% status / 35% input (same as your DSL footer)
      int split = (int)std::lround(full_w * 0.65);
      split = std::clamp(split, 0, full_w);

      // Status label (left)
      iinuji_layout_t ml{};
      ml.mode = layout_mode_t::Absolute;
      ml.normalized = false;
      ml.x = 0; ml.y = 0;
      ml.width  = std::max(0, split);
      ml.height = menu_rows;

      out.menu_bar = create_text_box(join_path(root_id, "bottom.status"),
                                    build_menu_text(),
                                    /*wrap*/false, text_align_t::Left, ml, bs);
      if (out.menu_bar) out.menu_bar->focusable = false;
      if (out.bottom && out.menu_bar) out.bottom->add_child(out.menu_bar);

      // Terminal input (right)
      iinuji_style_t isty = bs;
      isty.background_color = "#202020";
      isty.label_color = "white";

      iinuji_layout_t il{};
      il.mode = layout_mode_t::Absolute;
      il.normalized = false;
      il.x = split; il.y = 0;
      il.width  = std::max(1, full_w - split);
      il.height = menu_rows;

      out.terminal_input = create_text_box(join_path(root_id, "bottom.input"),
                                          "",
                                          /*wrap*/false, text_align_t::Left, il, isty);

      apply_focus_policy(out.terminal_input, "_input_box");
      if (out.terminal_input) {
        out.terminal_input_id = out.terminal_input->id;

        // register in maps so runtime key handler sees it
        out.figure_object_by_id[out.terminal_input_id] = out.terminal_input;
        out.figure_kind_by_id[out.terminal_input_id] = "_input_box";
        out.triggers_by_figure_id[out.terminal_input_id] = {};

        if (out.terminal_input->focusable) out.focus_order.push_back(out.terminal_input_id);

        if (out.bottom) out.bottom->add_child(out.terminal_input);
      }
    }
  }

  // Default focus: first focusable figure (if any)
  if (!out.focus_order.empty()) {
    std::size_t want = 0;
    if (!out.terminal_input_id.empty()) {
      auto itw = std::find(out.focus_order.begin(), out.focus_order.end(), out.terminal_input_id);
      if (itw != out.focus_order.end()) want = (std::size_t)std::distance(out.focus_order.begin(), itw);
    }
    out.focus_index = want;
    auto it = out.figure_object_by_id.find(out.focus_order[out.focus_index]);
    if (it != out.figure_object_by_id.end() && it->second) {
      it->second->focused = true;
    }
  }


  out.root = root;
  return out;
}

} // namespace iinuji
} // namespace cuwacunu
