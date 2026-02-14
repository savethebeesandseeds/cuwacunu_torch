#pragma once

namespace cuwacunu {
namespace iinuji {

struct instructions_build_opts_t {
  bool auto_normalize = true;
  bool force_normalize = false;
  bool plot_legend_overlay = true;

  // Reserve N terminal rows at the bottom for a global menu/status bar.
  // The DSL 0..100 vertical range maps to the remaining rows.
  // Default: reserve 1 last row (so 100% height == "all but the last line").
  int  global_menu_rows = 1;
  bool show_global_menu_bar = true;

  // if invalid, default is: don't build anything
  bool render_placeholders_on_error = false;
};

struct instructions_validate_opts_t {
  bool strict_form_types = true;                 // local_name must be {str,vec,num}
  bool require_event_bindings = true;            // EVENT must have __form bindings
  bool require_trigger_event_exists = true;      // triggers must reference a real EVENT
  bool forbid_mixed_figure_kinds_per_event = true;
  bool enforce_event_kind_by_figure = true;

  // Important pedantry: all triggers of the same figure must bind to same slot.
  bool enforce_same_binding_per_figure = true;
};

} // namespace iinuji
} // namespace cuwacunu
