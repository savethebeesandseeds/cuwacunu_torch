#pragma once

#include <algorithm>
#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string ui_bottom_line(const CmdState& st) {
  if (st.screen != ScreenMode::Board) return {};

  auto shorten = [](std::string s, std::size_t n = 220u) {
    if (s.size() <= n) return s;
    s.resize(n > 3u ? (n - 3u) : n);
    if (n > 3u) s += "...";
    return s;
  };

  if (st.board.editor_focus &&
      st.board.exit_prompt == BoardState::ExitPrompt::SaveDiscardCancel) {
    const int idx = std::clamp(st.board.exit_prompt_index, 0, 2);
    auto opt = [&](const char* label, bool selected) {
      if (!selected) return std::string("[  ") + label + "  ]";
      return std::string("[> ") + label + " <]";
    };
    std::ostringstream oss;
    oss << "save changes: ";
    oss << opt("Save", idx == 0) << " ";
    oss << opt("Discard", idx == 1) << " ";
    oss << opt("Cancel", idx == 2);
    oss << " | Left/Right move | Enter select | Esc cancel";
    return oss.str();
  }

  if (st.board.editor_focus &&
      st.board.diagnostic_active &&
      !st.board.diagnostic_message.empty()) {
    std::ostringstream oss;
    oss << "error L";
    if (st.board.diagnostic_line >= 0) {
      oss << (st.board.diagnostic_line + 1);
    } else {
      oss << "?";
    }
    if (st.board.diagnostic_col >= 0) {
      oss << ":C" << (st.board.diagnostic_col + 1);
    }
    oss << ": " << shorten(st.board.diagnostic_message);
    return oss.str();
  }

  if (!st.board.ok && !st.board.error.empty()) {
    return "board invalid: " + shorten(st.board.error);
  }

  if (st.board.editor_focus && st.board.editor && !st.board.editor->status.empty()) {
    return "editor: " + shorten(st.board.editor->status, 180u);
  }

  if (st.board.panel_focus == BoardPanelFocus::ViewOptions) {
    return "View options: Up/Down select | Enter open | Esc context | options: Diagram / Contract Text (edit)";
  }
  if (st.board.panel_focus == BoardPanelFocus::ContractSections) {
    return "Contract sections: Up/Down select | Enter edit selected section | Esc view options";
  }
  return "Context: Up/Down select contract | Enter view options | selected display on left panel";
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
