#pragma once

#include <cstddef>
#include <string>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct TsiViewEnterActionResult {
  bool handled{false};
  std::string canonical_call{};
};

inline TsiViewEnterActionResult handle_tsi_view_enter_action(CmdState& st) {
  TsiViewEnterActionResult out{};
  if (st.screen != ScreenMode::Tsiemene) return out;
  if (st.tsiemene.panel_focus != TsiPanelFocus::View) return out;

  out.handled = true;
  if (!tsi_selected_tab_supports_form_rows(st)) return out;

  const std::size_t row_count = tsi_form_row_count_for_selected_tab(st);
  if (row_count == 0) return out;
  const std::size_t row = (st.tsiemene.view_cursor < row_count) ? st.tsiemene.view_cursor : 0;

  switch (static_cast<TsiSourceDataloaderFormRow>(row)) {
    case TsiSourceDataloaderFormRow::Create:
      out.canonical_call = std::string(canonical_paths::kTsiDataloaderCreate);
      return out;
    case TsiSourceDataloaderFormRow::SelectPrev:
      select_prev_tsi_source_dataloader(st);
      return out;
    case TsiSourceDataloaderFormRow::SelectNext:
      select_next_tsi_source_dataloader(st);
      return out;
    case TsiSourceDataloaderFormRow::EditSelected:
      out.canonical_call = std::string(canonical_paths::kTsiDataloaderEdit);
      return out;
    case TsiSourceDataloaderFormRow::DeleteSelected:
      out.canonical_call = std::string(canonical_paths::kTsiDataloaderDelete);
      return out;
    case TsiSourceDataloaderFormRow::DslInstruments:
    case TsiSourceDataloaderFormRow::DslInputs:
    case TsiSourceDataloaderFormRow::StoreRoot:
      return out;
  }
  return out;
}

inline bool step_tsi_view_cursor(CmdState& st, int delta) {
  const std::size_t n = tsi_form_row_count_for_selected_tab(st);
  if (n == 0) return false;
  const std::size_t cur = (st.tsiemene.view_cursor < n) ? st.tsiemene.view_cursor : 0;
  const std::size_t next = (delta >= 0)
      ? ((cur + 1) % n)
      : ((cur + n - 1) % n);
  st.tsiemene.view_cursor = next;
  return true;
}

inline bool handle_tsi_key(CmdState& st, int ch, bool cmdline_empty) {
  if (st.screen != ScreenMode::Tsiemene) return false;

  if (ch == KEY_ENTER || ch == '\n' || ch == '\r') {
    if (!cmdline_empty) return false;
    if (st.tsiemene.panel_focus == TsiPanelFocus::Context) {
      st.tsiemene.panel_focus = TsiPanelFocus::View;
      clamp_tsi_navigation_state(st);
    }
    return true;
  }

  if (ch == 27) {
    if (!cmdline_empty) return false;
    if (st.tsiemene.panel_focus == TsiPanelFocus::View) {
      st.tsiemene.panel_focus = TsiPanelFocus::Context;
      return true;
    }
    return false;
  }

  if (ch == KEY_UP) {
    if (st.tsiemene.panel_focus == TsiPanelFocus::Context) {
      select_prev_tsi_tab(st);
      return true;
    }
    return step_tsi_view_cursor(st, -1);
  }

  if (ch == KEY_DOWN) {
    if (st.tsiemene.panel_focus == TsiPanelFocus::Context) {
      select_next_tsi_tab(st);
      return true;
    }
    return step_tsi_view_cursor(st, +1);
  }

  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
