#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

#include "iinuji/bnf_compat/instructions/diag.h"
#include "iinuji/bnf_compat/instructions/data.h"
#include "iinuji/bnf_compat/instructions/options.h"
#include "iinuji/bnf_compat/instructions/helpers.h"
#include "iinuji/bnf_compat/instructions/form.h"

#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

RUNTIME_WARNING("(iinuji/validation.h)[] many checks re-validate grammar-level syntax (kinds/ident shape); consider slimming validation to semantic rules only.\n");
RUNTIME_WARNING("(iinuji/validation.h)[] compile_events() stores events even if some bindings invalid; consider marking event invalid if any binding fails to avoid partial-wiring surprises.\n");
RUNTIME_WARNING("(iinuji/validation.h)[] forbid_mixed_figure_kinds_per_event is strict; simplifies wiring but prevents intentional fan-out. Consider a future 'explicit binding' mode.\n");

namespace cuwacunu {
namespace iinuji {

inline bool pct_ok(double v) {
  return std::isfinite(v) && v >= 0.0 && v <= 100.0;
}

inline void validate_screen_fields(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                                   size_t si,
                                   instructions_diag_t& d)
{
  const std::string where = "screen[" + std::to_string(si) + "]";

  if (!is_unset_token(sc.kind_raw) && sc.kind_raw != "_screen")
    d.err(where + ": kind_raw='" + sc.kind_raw + "' (expected _screen)");

  if (is_unset_token(sc.name) || !is_ident(sc.name))
    d.err(where + ": invalid/missing __name '" + sc.name + "'");

  if (!is_unset_token(sc.key_raw)) {
    std::string k = to_lower(sc.key_raw);
    const bool ok =
      (k == "f0" || k == "f+0") ||                 // default screen marker
      (k.size() >= 3 && k.rfind("f+", 0) == 0);    // F+N

    if (!ok)
      d.warn(where + ": __key '" + sc.key_raw + "' not in expected form 'F+N' or 'F0'");
  }

  if (sc.tickness <= 0.0 || !std::isfinite(sc.tickness))
    d.err(where + ": __tickness must be > 0");

  if (!is_valid_color_token(sc.line_color))
    d.err(where + ": invalid __line_color '" + sc.line_color + "'");
  if (!is_valid_color_token(sc.text_color))
    d.err(where + ": invalid __text_color '" + sc.text_color + "'");
  if (!is_valid_color_token(sc.back_color))
    d.err(where + ": invalid __back_color '" + sc.back_color + "'");
}

inline void validate_panel_fields(const cuwacunu::camahjucunu::iinuji_panel_t& P,
                                  size_t si, size_t pi,
                                  instructions_diag_t& d)
{
  const std::string where = "screen[" + std::to_string(si) + "].panel[" + std::to_string(pi) + "]";

  if (!is_unset_token(P.kind_raw) && P.kind_raw != "_rectangle")
    d.err(where + ": kind_raw='" + P.kind_raw + "' (expected _rectangle)");

  if (!P.coords.set) d.err(where + ": missing __coords");
  if (!P.shape.set)  d.err(where + ": missing __shape");
  
  if (P.coords.set && (!pct_ok(P.coords.x) || !pct_ok(P.coords.y)))
    d.err(where + ": __coords must be within [0,100]");

  if (P.shape.set && (P.shape.x <= 0.0 || P.shape.y <= 0.0))
    d.err(where + ": __shape must be > 0");

  if (P.shape.set && (!pct_ok(P.shape.x) || !pct_ok(P.shape.y)))
    d.err(where + ": __shape must be within (0,100]");

  if (P.coords.set && P.shape.set) {
    if (P.coords.x + P.shape.x > 100.0 + 1e-9)
      d.err(where + ": __coords.x + __shape.x must be <= 100");
    if (P.coords.y + P.shape.y > 100.0 + 1e-9)
      d.err(where + ": __coords.y + __shape.y must be <= 100");
  }

  if (P.shape.set && (P.shape.x <= 0 || P.shape.y <= 0))
    d.err(where + ": __shape must be > 0");

  if (P.tickness <= 0.0 || !std::isfinite(P.tickness))
    d.err(where + ": __tickness must be > 0");

  if (P.title_on && is_unset_token(P.title))
    d.err(where + ": __title true but title string is empty");

  if (!is_valid_color_token(P.line_color))
    d.err(where + ": invalid __line_color '" + P.line_color + "'");
  if (!is_valid_color_token(P.text_color))
    d.err(where + ": invalid __text_color '" + P.text_color + "'");
  if (!is_valid_color_token(P.back_color))
    d.err(where + ": invalid __back_color '" + P.back_color + "'");
}

inline void validate_figure_fields(const cuwacunu::camahjucunu::iinuji_figure_t& F,
                                   size_t si, size_t pi, size_t fi,
                                   instructions_diag_t& d)
{
  const std::string where = "screen[" + std::to_string(si) + "].panel[" + std::to_string(pi)
                          + "].figure[" + std::to_string(fi) + "]";

  if (is_unset_token(F.kind_raw)) {
    d.err(where + ": missing FIGURE kind");
    return;
  }

  if (F.kind_raw != "_label" && F.kind_raw != "_horizontal_plot" && F.kind_raw != "_input_box" && F.kind_raw != "_buffer")
    d.err(where + ": unsupported FIGURE kind_raw='" + F.kind_raw + "'");

  if (!F.coords.set) d.err(where + ": missing __coords");
  if (!F.shape.set)  d.err(where + ": missing __shape");
  if (F.shape.set && (F.shape.x <= 0 || F.shape.y <= 0))
    d.err(where + ": __shape must be > 0");

  if (F.tickness <= 0.0 || !std::isfinite(F.tickness))
    d.err(where + ": __tickness must be > 0");

  if (!is_valid_color_token(F.line_color))
    d.err(where + ": invalid __line_color '" + F.line_color + "'");
  if (!is_valid_color_token(F.text_color))
    d.err(where + ": invalid __text_color '" + F.text_color + "'");
  if (!is_valid_color_token(F.back_color))
    d.err(where + ": invalid __back_color '" + F.back_color + "'");

  if (F.kind_raw == "_label" || F.kind_raw == "_input_box") {
    if (!F.has_value || is_unset_token(F.value))
      d.err(where + ": " + F.kind_raw + " requires __value");
  }

  if (F.kind_raw == "_horizontal_plot") {
    if (F.has_value && !is_unset_token(F.value))
      d.err(where + ": _horizontal_plot must NOT have __value");

    if (!is_valid_plot_type(F.type_raw))
      d.err(where + ": invalid __type '" + F.type_raw + "' (expected line/scatter/stairs/stem)");

    if (F.title_on && is_unset_token(F.title))
      d.err(where + ": __title true but title string is empty");
    if (F.legend_on && is_unset_token(F.legend))
      d.err(where + ": __legend true but legend string is empty");
  }

  if (F.kind_raw == "_buffer") {
    // must NOT have __value
    if (F.has_value && !is_unset_token(F.value))
      d.err(where + ": _buffer must NOT have __value");

    // require capacity
    // (requires decoder/struct update: F.capacity + F.has_capacity)
    if (!F.has_capacity || F.capacity <= 0)
      d.err(where + ": _buffer requires __capacity > 0");

    // type must be updown or downup (treat <empty> as unset/default)
    std::string t;
    if (!is_unset_token(F.type_raw)) t = to_lower(F.type_raw);

    if (!(t.empty() || t=="updown" || t=="downup")) {
      d.err(where + ": _buffer invalid __type '" + F.type_raw + "' (expected updown/downup)");
    }
  }
}

inline resolved_event_map_t compile_events(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                                          size_t si,
                                          const instructions_validate_opts_t& vopt,
                                          instructions_diag_t& d)
{
  resolved_event_map_t out;
  std::unordered_set<std::string> seen;

  for (size_t ei=0; ei<sc.events.size(); ++ei) {
    const auto& E = sc.events[ei];
    const std::string where = "screen[" + std::to_string(si) + "].event[" + std::to_string(ei) + "]";

    if (E.kind_raw != "_update" && E.kind_raw != "_action") {
      d.err(where + ": unsupported EVENT kind_raw='" + E.kind_raw + "'");
      continue;
    }
    if (is_unset_token(E.name) || !is_ident(E.name)) {
      d.err(where + ": invalid/missing __name '" + E.name + "'");
      continue;
    }
    if (!seen.insert(E.name).second) {
      d.err(where + ": duplicate EVENT __name '" + E.name + "'");
      continue;
    }

    if (vopt.require_event_bindings && E.bindings.empty()) {
      d.err(where + ": missing __form bindings");
      continue;
    }

    resolved_event_t re;
    re.kind_raw = E.kind_raw;
    re.name = E.name;

    // Optional EVENT metadata used by FIGURE _buffer
    if (E.has_label && !is_unset_token(E.label)) {
      if (!is_ident(E.label)) {
        d.err(where + ": invalid __label '" + E.label + "'");
      } else {
        re.has_label = true;
        re.label = E.label;
      }
    }

    if (E.has_color && !is_unset_token(E.color)) {
      if (!is_valid_color_token(E.color)) {
        d.err(where + ": invalid __color '" + E.color + "'");
      } else {
        re.has_color = true;
        re.color = E.color;
      }
    }

    for (size_t bi=0; bi<E.bindings.size(); ++bi) {
      const auto& B = E.bindings[bi];
      const std::string bwhere = where + ".binding[" + std::to_string(bi) + "]";

      if (is_unset_token(B.local_name) || !is_ident(B.local_name)) {
        d.err(bwhere + ": invalid local_name '" + B.local_name + "'");
        continue;
      }
      if (is_unset_token(B.path_name)) {
        d.err(bwhere + ": missing path_name");
        continue;
      }

      bind_kind_e bk = parse_bind_kind(B.local_name);
      if (vopt.strict_form_types && bk == bind_kind_e::Unknown) {
        d.err(bwhere + ": local_name must be one of {str,vec,num}, got '" + B.local_name + "'");
        continue;
      }
      
      data_ref_t ref = parse_data_path(B.path_name);
      if (ref.kind == data_kind_e::Invalid) {
        d.err(bwhere + ": invalid path '" + B.path_name +
              "' (expected .strN/.vecN/.numN or .sys.stdout/.sys.stderr)");
        continue;
      }
      if (ref.kind != data_kind_e::System && ref.index < 0) {
        d.err(bwhere + ": invalid indexed path '" + B.path_name +
              "' (expected .strN/.vecN/.numN)");
        continue;
      }

      if (bk != bind_kind_e::Unknown && !kind_ok(bk, ref.kind)) {
        d.err(bwhere + ": type mismatch: local '" + B.local_name + "' vs path '" + B.path_name + "'");
        continue;
      }

      re.bindings.push_back(resolved_binding_t{bk, ref});
    }

    // If all bindings were rejected as invalid, treat the event as invalid.
    if (vopt.require_event_bindings && re.bindings.empty()) {
      d.err(where + ": no valid __form bindings (all bindings were invalid)");
      continue;
    }

    out[re.name] = std::move(re);
  }

  return out;
}

inline instructions_diag_t validate_data_access(const resolved_event_map_t& evmap,
                                                const IInstructionsData& data)
{
  instructions_diag_t d;
  for (const auto& kv : evmap) {
    const auto& E = kv.second;
    for (size_t bi=0; bi<E.bindings.size(); ++bi) {
      const auto& B = E.bindings[bi];
      const std::string where = "event[" + E.name + "].binding[" + std::to_string(bi) + "]";

      switch (B.ref.kind) {
        case data_kind_e::System:
          // ok: not in data store
          break;
        case data_kind_e::Str:
          if (!data.supports_str(B.ref.index))
            d.err(where + ": refers to str" + std::to_string(B.ref.index) + " but data does not support it");
          break;
        case data_kind_e::Vec:
          if (!data.supports_vec(B.ref.index))
            d.err(where + ": refers to vec" + std::to_string(B.ref.index) + " but data does not support it");
          break;
        case data_kind_e::Num:
          if (!data.supports_num(B.ref.index))
            d.err(where + ": refers to num" + std::to_string(B.ref.index) + " but data does not support it");
          break;
        default:
          d.err(where + ": invalid data ref");
          break;
      }
    }
  }
  return d;
}

inline void cross_validate_triggers(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                                    size_t si,
                                    const resolved_event_map_t& evmap,
                                    const instructions_validate_opts_t& vopt,
                                    instructions_diag_t& d)
{
  std::unordered_map<std::string, std::unordered_set<std::string>> event_to_figkinds;

  for (size_t pi=0; pi<sc.panels.size(); ++pi) {
    const auto& P = sc.panels[pi];
    for (size_t fi=0; fi<P.figures.size(); ++fi) {
      const auto& F = P.figures[fi];
      if (is_unset_token(F.kind_raw)) continue;

      const bind_kind_e want_bind = required_bind_kind_for_figure(F.kind_raw);
      const std::string want_ev_kind = required_event_kind_for_figure(F.kind_raw);

      for (const auto& trig : F.triggers) {
        if (is_unset_token(trig)) continue;

        auto it = evmap.find(trig);
        if (it == evmap.end()) {
          if (vopt.require_trigger_event_exists)
            d.err("screen[" + std::to_string(si) + "]: FIGURE trigger '" + trig + "' has no matching EVENT");
          continue;
        }

        const auto& E = it->second;
        event_to_figkinds[trig].insert(F.kind_raw);

        if (vopt.enforce_event_kind_by_figure) {
          if (E.kind_raw != want_ev_kind) {
            d.err("screen[" + std::to_string(si) + "]: EVENT '" + trig +
                  "' kind mismatch for FIGURE '" + F.kind_raw +
                  "' (needs " + want_ev_kind + ", got " + E.kind_raw + ")");
          }
        }

        if (!first_binding_of_kind(E, want_bind)) {
          d.err("screen[" + std::to_string(si) + "]: EVENT '" + trig +
                "' missing required binding type for FIGURE '" + F.kind_raw + "'");
        }

        if (event_has_system_binding(E) && F.kind_raw != "_buffer") {
          d.err("screen[" + std::to_string(si) + "]: EVENT '" + trig +
                "' is a system stream source, only _buffer may trigger it");
        }
      }
    }
  }

  if (vopt.forbid_mixed_figure_kinds_per_event) {
    for (const auto& kv : event_to_figkinds) {
      if (kv.second.size() > 1) {
        std::string ks;
        for (const auto& k : kv.second) { if (!ks.empty()) ks += ","; ks += k; }
        d.err("screen[" + std::to_string(si) + "]: EVENT '" + kv.first +
              "' referenced by multiple FIGURE kinds {" + ks + "} (ambiguous wiring)");
      }
    }
  }
}

inline void validate_same_binding_per_figure(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                                             size_t si,
                                             const resolved_event_map_t& evmap,
                                             const instructions_validate_opts_t& vopt,
                                             instructions_diag_t& d)
{
  if (!vopt.enforce_same_binding_per_figure) return;

  for (size_t pi=0; pi<sc.panels.size(); ++pi) {
    const auto& P = sc.panels[pi];
    for (size_t fi=0; fi<P.figures.size(); ++fi) {
      const auto& F = P.figures[fi];
      if (F.kind_raw == "_buffer") continue; // buffer can be populated from multiple sources
      if (is_unset_token(F.kind_raw)) continue;

      const bind_kind_e want_bind = required_bind_kind_for_figure(F.kind_raw);
      const std::string want_ev_kind = required_event_kind_for_figure(F.kind_raw);

      bool have_ref = false;
      data_ref_t ref0;

      for (const auto& trig : F.triggers) {
        if (is_unset_token(trig)) continue;
        auto it = evmap.find(trig);
        if (it == evmap.end()) continue;

        const auto& E = it->second;
        if (vopt.enforce_event_kind_by_figure && E.kind_raw != want_ev_kind) continue;

        const resolved_binding_t* b = first_binding_of_kind(E, want_bind);
        if (!b) continue;

        if (!have_ref) {
          have_ref = true;
          ref0 = b->ref;
        } else {
          if (ref0.kind != b->ref.kind || ref0.index != b->ref.index) {
            d.err("screen[" + std::to_string(si) + "]: FIGURE triggers bind to different slots "
                  "(first " + ref0.raw + ", then " + b->ref.raw + ")");
          }
        }
      }
    }
  }
}

inline void validate_system_events(const resolved_event_map_t& evmap,
                                   size_t si,
                                   instructions_diag_t& d)
{
  for (const auto& kv : evmap) {
    const auto& E = kv.second;
    bool has_sys = false;
    for (const auto& b : E.bindings) if (b.ref.kind == data_kind_e::System) has_sys = true;
    if (!has_sys) continue;

    const std::string where = "screen[" + std::to_string(si) + "].event[" + E.name + "]";

    // system sources are update-only
    if (E.kind_raw != "_update") {
      d.err(where + ": system stream bindings are only allowed on _update events");
    }

    // keep it strict: exactly one binding, str -> .sys.*
    if (E.bindings.size() != 1) {
      d.err(where + ": system stream event must have exactly one __form binding");
      continue;
    }

    const auto& B = E.bindings[0];
    if (B.bind_kind != bind_kind_e::Str) {
      d.err(where + ": system stream binding must be local_name 'str'");
    }
    if (B.ref.kind != data_kind_e::System) {
      d.err(where + ": expected system ref (.sys.stdout/.sys.stderr)");
    }
    if (!(B.ref.sys == sys_ref_e::Stdout || B.ref.sys == sys_ref_e::Stderr)) {
      d.err(where + ": invalid system ref");
    }
  }
}

/* Validate + compile contract for a single screen */
struct screen_contract_t {
  instructions_diag_t diag;
  resolved_event_map_t events;
};

inline screen_contract_t validate_and_compile_screen(const cuwacunu::camahjucunu::iinuji_screen_t& sc,
                                                    size_t si,
                                                    const instructions_validate_opts_t& vopt = {})
{
  screen_contract_t out;

  validate_screen_fields(sc, si, out.diag);

  for (size_t pi=0; pi<sc.panels.size(); ++pi) {
    validate_panel_fields(sc.panels[pi], si, pi, out.diag);
    for (size_t fi=0; fi<sc.panels[pi].figures.size(); ++fi) {
      validate_figure_fields(sc.panels[pi].figures[fi], si, pi, fi, out.diag);
    }
  }

  out.events = compile_events(sc, si, vopt, out.diag);
  validate_system_events(out.events, si, out.diag);

  cross_validate_triggers(sc, si, out.events, vopt, out.diag);
  validate_same_binding_per_figure(sc, si, out.events, vopt, out.diag);

  return out;
}

inline instructions_diag_t validate_instruction(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& inst,
                                               const instructions_validate_opts_t& vopt = {})
{
  instructions_diag_t d;
  if (inst.screens.empty()) {
    d.err("instruction: no screens present");
    return d;
  }
  for (size_t si=0; si<inst.screens.size(); ++si) {
    auto one = validate_and_compile_screen(inst.screens[si], si, vopt);
    d.merge(one.diag);
  }
  return d;
}

} // namespace iinuji
} // namespace cuwacunu
