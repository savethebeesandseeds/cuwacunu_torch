#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "iinuji/iinuji_cmd/views/board/completion.h"
#include "iinuji/iinuji_cmd/views/board/diagnostic.h"
#include "iinuji/iinuji_cmd/views/board/editor.h"
#include "iinuji/iinuji_cmd/views/board/overlay.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool open_board_view_option(CmdState& st) {
  clamp_board_view_option(st);
  st.board.display_mode = board_display_mode_for_option(st.board.selected_view_option);
  if (st.board.display_mode == BoardDisplayMode::ContractTextEdit) {
    st.board.panel_focus = BoardPanelFocus::ContractSections;
    clamp_board_contract_section(st);
    st.board.editor_focus = false;
    st.board.editor_scope = BoardEditorScope::None;
    st.board.exit_prompt = BoardState::ExitPrompt::None;
    st.board.exit_prompt_index = 0;
    clear_board_completion(st);
    return true;
  }
  st.board.panel_focus = BoardPanelFocus::ViewOptions;
  st.board.editor_focus = false;
  st.board.editor_scope = BoardEditorScope::None;
  st.board.exit_prompt = BoardState::ExitPrompt::None;
  st.board.exit_prompt_index = 0;
  clear_board_completion(st);
  return true;
}

inline bool handle_board_navigation_key(CmdState& st, int ch, bool cmdline_empty) {
  if (st.screen != ScreenMode::Board) return false;
  if (st.board.editor_focus) return false;

  if ((ch == KEY_ENTER || ch == '\n' || ch == '\r') && cmdline_empty) {
    if (st.board.panel_focus == BoardPanelFocus::Context) {
      st.board.panel_focus = BoardPanelFocus::ViewOptions;
      clamp_board_view_option(st);
      return true;
    }
    if (st.board.panel_focus == BoardPanelFocus::ViewOptions) {
      return open_board_view_option(st);
    }
    if (st.board.panel_focus == BoardPanelFocus::ContractSections) {
      return enter_selected_contract_section_editor(st);
    }
    return false;
  }
  if (ch == 27 && cmdline_empty) {
    switch (st.board.panel_focus) {
      case BoardPanelFocus::ContractSections:
        st.board.panel_focus = BoardPanelFocus::ViewOptions;
        return true;
      case BoardPanelFocus::ViewOptions:
        st.board.panel_focus = BoardPanelFocus::Context;
        return true;
      case BoardPanelFocus::Context:
        return false;
    }
  }

  if (ch == KEY_DOWN) {
    switch (st.board.panel_focus) {
      case BoardPanelFocus::ViewOptions:
        return select_next_board_view_option(st);
      case BoardPanelFocus::ContractSections:
        return select_next_board_contract_section(st);
      case BoardPanelFocus::Context:
        return select_next_board_circuit(st);
    }
  }
  if (ch == KEY_UP) {
    switch (st.board.panel_focus) {
      case BoardPanelFocus::ViewOptions:
        return select_prev_board_view_option(st);
      case BoardPanelFocus::ContractSections:
        return select_prev_board_contract_section(st);
      case BoardPanelFocus::Context:
        return select_prev_board_circuit(st);
    }
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
