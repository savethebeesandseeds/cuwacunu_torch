#pragma once

#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string ui_status_line(const CmdState& st) {
  std::ostringstream oss;
  oss << (st.screen == ScreenMode::Home ? "[F1 HOME]" : " F1 HOME ");
  oss << " ";
  oss << (st.screen == ScreenMode::Board ? "[F2 BOARD]" : " F2 BOARD ");
  oss << " ";
  oss << (st.screen == ScreenMode::Training ? "[F3 TRAIN]" : " F3 TRAIN ");
  oss << " ";
  oss << (st.screen == ScreenMode::Tsiemene ? "[F4 TSI]" : " F4 TSI ");
  oss << " ";
  oss << (st.screen == ScreenMode::Data ? "[F5 DATA]" : " F5 DATA ");
  oss << " ";
  oss << (st.screen == ScreenMode::Logs ? "[F8 LOGS]" : " F8 LOGS ");
  oss << " ";
  oss << (st.screen == ScreenMode::Config ? "[F9 CONFIG]" : " F9 CONFIG ");
  if (st.screen == ScreenMode::Board && st.board.editor_focus) {
    if (st.board.exit_prompt == BoardState::ExitPrompt::SaveDiscardCancel) {
      oss << " | board editor: save changes? Left/Right choose | Enter confirm | Esc cancel";
    } else {
      if (st.board.editor_scope == BoardEditorScope::FullInstruction) {
        oss << " | board contract editor: Esc out | Enter newline | Ctrl+S save | Ctrl+R disabled | Tab=indent";
      } else if (st.board.editor_scope == BoardEditorScope::ContractSection) {
        oss << " | board contract section editor: Esc out | Enter newline | Ctrl+S save | Ctrl+R disabled | Tab=indent";
      } else if (st.board.editor_scope == BoardEditorScope::ContractVirtual) {
        oss << " | board contract circuit editor: Esc out | Tab completion | Enter newline | Ctrl+S save | Ctrl+R validate";
      } else {
        oss << " | board editor: Esc out | Tab | Enter newline | Ctrl+S save | Ctrl+R validate";
      }
    }
  } else if (st.screen == ScreenMode::Board) {
    if (st.board.panel_focus == BoardPanelFocus::ViewOptions) {
      oss << " | board view options: Up/Down rows | Enter select | Esc context";
    } else if (st.board.panel_focus == BoardPanelFocus::ContractSections) {
      oss << " | board contract sections: Up/Down rows | Enter edit section | Esc view options";
    } else {
      oss << " | board context: Up/Down contracts | Enter view options | type command and Enter";
    }
  } else {
    oss << " | type command and Enter";
  }
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
