#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/lattice/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_lattice_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Lattice) return false;
  if (ch == 'r' || ch == 'R') {
    log_dbg("[iinuji:lattice] refresh_key mode=%s\n",
            lattice_refresh_mode_name(LatticeRefreshMode::SyncStore));
    queue_lattice_refresh(st, LatticeRefreshMode::SyncStore,
                          "Syncing hero.lattice and hero.hashimyei catalogs...");
    return true;
  }
  if (lattice_is_navigator_focus(st)) {
    if (ch == KEY_UP || ch == 'k') return lattice_select_prev_section(st);
    if (ch == KEY_DOWN || ch == 'j') return lattice_select_next_section(st);
    if (ch == KEY_HOME) return lattice_set_section(st, LatticeSection::Overview);
    if (ch == KEY_END) return lattice_set_section(st, LatticeSection::Lattice);
    if (ch == KEY_RIGHT || ch == 'l' || ch == ']' || ch == '\n' ||
        ch == '\r' || ch == KEY_ENTER) {
      if (st.lattice.selected_section == LatticeSection::Components &&
          st.lattice.component_rows.empty()) {
        queue_lattice_refresh(st, LatticeRefreshMode::SyncStore,
                              "Refreshing hero.hashimyei and hero.lattice catalogs...");
      }
      return lattice_focus_worklist(st);
    }
    return false;
  }

  if (ch == 27) {
    if (st.lattice.selected_section == LatticeSection::Lattice &&
        st.lattice.mode != LatticeMode::Roots) {
      return lattice_exit_mode_to_roots(st);
    }
    return lattice_focus_navigator(st);
  }

  if (st.lattice.selected_section == LatticeSection::Overview) {
    if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
      return lattice_focus_navigator(st);
    }
    return false;
  }

  if (st.lattice.selected_section == LatticeSection::Components) {
    if (st.lattice.component_rows.empty()) {
      if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
        return lattice_focus_navigator(st);
      }
      if (ch == KEY_RIGHT || ch == 'l' || ch == ']' || ch == '\n' ||
          ch == '\r' || ch == KEY_ENTER) {
        queue_lattice_refresh(st, LatticeRefreshMode::SyncStore,
                              "Refreshing hero.hashimyei and hero.lattice catalogs...");
        return true;
      }
      return false;
    }
    if (ch == KEY_UP || ch == 'k') return select_prev_lattice_component_row(st);
    if (ch == KEY_DOWN || ch == 'j') {
      return select_next_lattice_component_row(st);
    }
    if (ch == KEY_HOME) return select_first_lattice_component_row(st);
    if (ch == KEY_END) return select_last_lattice_component_row(st);
    if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
      return select_lattice_component_parent_row(st) || lattice_focus_navigator(st);
    }
    if (ch == KEY_RIGHT || ch == 'l' || ch == ']' || ch == '\n' ||
        ch == '\r' || ch == KEY_ENTER) {
      return select_lattice_component_child_row(st);
    }
    return false;
  }

  if (st.lattice.mode == LatticeMode::Roots) {
    if (ch == KEY_UP || ch == 'k') return select_prev_lattice_mode_root(st);
    if (ch == KEY_DOWN || ch == 'j') return select_next_lattice_mode_root(st);
    if (ch == KEY_HOME) {
      if (st.lattice.selected_mode_row == 0) return false;
      st.lattice.selected_mode_row = 0;
      return true;
    }
    if (ch == KEY_END) {
      if (st.lattice.selected_mode_row == 1) return false;
      st.lattice.selected_mode_row = 1;
      return true;
    }
    if (ch == KEY_RIGHT || ch == 'l' || ch == ']' || ch == '\n' ||
        ch == '\r' || ch == KEY_ENTER) {
      return lattice_enter_selected_mode(st);
    }
    if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
      return lattice_focus_navigator(st);
    }
    return false;
  }

  if (st.lattice.mode == LatticeMode::Views) {
    if (ch == KEY_UP || ch == 'k') return lattice_select_prev_view(st);
    if (ch == KEY_DOWN || ch == 'j') return lattice_select_next_view(st);
    if (ch == KEY_HOME) return lattice_select_first_view(st);
    if (ch == KEY_END) return lattice_select_last_view(st);
    if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
      return lattice_exit_mode_to_roots(st);
    }
    return false;
  }

  if (ch == KEY_UP || ch == 'k') return select_prev_lattice_row(st);
  if (ch == KEY_DOWN || ch == 'j') return select_next_lattice_row(st);
  if (ch == KEY_HOME) return select_first_lattice_row(st);
  if (ch == KEY_END) return select_last_lattice_row(st);
  if (ch == KEY_LEFT || ch == 'h' || ch == '[') {
    return select_lattice_parent_row(st) || lattice_exit_mode_to_roots(st);
  }
  if (ch == KEY_RIGHT || ch == 'l' || ch == ']' || ch == '\n' ||
      ch == '\r' || ch == KEY_ENTER) {
    return select_lattice_child_row(st);
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
