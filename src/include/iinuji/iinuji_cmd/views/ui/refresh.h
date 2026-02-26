#pragma once

#include <memory>
#include <string>

#include "iinuji/iinuji_cmd/views/board/contract.section.registry.h"
#include "iinuji/iinuji_cmd/views/board/view.h"
#include "iinuji/iinuji_cmd/views/config/view.h"
#include "iinuji/iinuji_cmd/views/data/view.h"
#include "iinuji/iinuji_cmd/views/home/view.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/training/view.h"
#include "iinuji/iinuji_cmd/views/tsiemene/view.h"
#include "iinuji/iinuji_cmd/views/ui/bottom.h"
#include "iinuji/iinuji_cmd/views/ui/status.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void ui_refresh_panels(
    const CmdState& st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& title,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& status,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& bottom,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& cmdline) {
  right->focusable = false;
  right->focused = false;
  right->style.title = " context ";
  bottom->focusable = false;
  bottom->focused = false;
  bottom->style.title = " message ";
  const bool bottom_is_error =
      (st.screen == ScreenMode::Board) &&
      ((!st.board.ok && !st.board.error.empty()) ||
       (st.board.editor_focus && st.board.diagnostic_active && !st.board.diagnostic_message.empty()));
  bottom->style.label_color = bottom_is_error ? "#c38e8e" : "#A8A8AF";
  set_text_box(bottom, (st.screen == ScreenMode::Board ? ui_bottom_line(st) : std::string{}), false);
  if (st.screen == ScreenMode::Home) {
    const IinujiHomeView home{st};
    set_text_box(title, "cuwacunu.cmd - home", true);
    set_text_box(left, home.left(), true);
    set_text_box(right, home.right(), true);
  } else if (st.screen == ScreenMode::Board) {
    set_text_box(title, "cuwacunu.cmd - tsi board", true);
    if (st.board.editor && st.board.editor_focus) {
      set_editor_box(left, st.board.editor);
      left->focusable = true;
      left->focused = true;
      if (st.board.exit_prompt == BoardState::ExitPrompt::SaveDiscardCancel) {
        left->style.title = " board.contract [save prompt] ";
      } else {
        left->style.title =
            (st.board.editor_scope == BoardEditorScope::ContractVirtual)
                ? " board.contract.circuit [edit] "
                : " board.dsl [edit] ";
        if (st.board.editor_scope == BoardEditorScope::ContractSection) {
          left->style.title =
              " " + std::string(board_contract_section_key(st.board.editing_contract_section)) + " [edit] ";
        }
        if (st.board.editor_scope == BoardEditorScope::FullInstruction) {
          left->style.title = " board.contract [edit] ";
        }
      }
    } else {
      if (st.board.display_mode == BoardDisplayMode::ContractTextEdit) {
        set_text_box_styled_lines(left, make_board_left_contract_edit_styled_lines(st), false);
      } else {
        set_text_box(left, make_board_left(st), false);
      }
      left->focusable = false;
      left->focused = false;
      switch (st.board.display_mode) {
        case BoardDisplayMode::Diagram:
          left->style.title = " view [diagram] ";
          break;
        case BoardDisplayMode::ContractTextEdit:
          left->style.title = " view [contract edit] ";
          break;
      }
    }
    set_text_box_styled_lines(right, make_board_right_styled_lines(st), true);
    right->focusable = false;
    right->focused = false;
    right->style.title = " context ";
  } else if (st.screen == ScreenMode::Training) {
    set_text_box(title, "cuwacunu.cmd - training", true);
    set_text_box(left, make_training_left(st), false);
    set_text_box(right, make_training_right(st), true);
  } else if (st.screen == ScreenMode::Logs) {
    set_text_box(title, "cuwacunu.cmd - logs", true);
    const auto snap = cuwacunu::piaabo::dlog_snapshot();
    set_text_box_styled_lines(left, make_logs_left_styled_lines(st.logs, snap), false);
    set_text_box(right, make_logs_right(st.logs, snap), true);
  } else if (st.screen == ScreenMode::Tsiemene) {
    set_text_box(title, "cuwacunu.cmd - tsiemene", true);
    set_text_box(left, make_tsi_left(st), false);
    set_text_box(right, make_tsi_right(st), true);
  } else if (st.screen == ScreenMode::Data) {
    set_text_box(title, "cuwacunu.cmd - data", true);
    set_text_box(left, make_data_left(st), false);
    set_text_box(right, make_data_right(st), true);
  } else {
    set_text_box(title, "cuwacunu.cmd - config", true);
    set_text_box(left, make_config_left(st), false);
    set_text_box(right, make_config_right(st), true);
  }

  set_text_box(status, ui_status_line(st), true);
  set_text_box(cmdline, "cmd> " + st.cmdline, false);
  const bool board_editor_focus = (st.screen == ScreenMode::Board && st.board.editor_focus);
  cmdline->focused = !board_editor_focus;
  right->focused = false;
}

struct IinujiUi {
  const CmdState& st;

  std::string status_line() const {
    return ui_status_line(st);
  }

  std::string bottom_line() const {
    return ui_bottom_line(st);
  }

  void refresh(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& title,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& status,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& bottom,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& cmdline) const {
    ui_refresh_panels(st, title, status, left, right, bottom, cmdline);
  }
};

inline std::string make_status_line(const CmdState& st) {
  return IinujiUi{st}.status_line();
}

inline void refresh_ui(const CmdState& st,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& title,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& status,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& bottom,
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& cmdline) {
  IinujiUi{st}.refresh(title, status, left, right, bottom, cmdline);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
