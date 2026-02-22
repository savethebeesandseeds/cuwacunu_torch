#pragma once

#include "iinuji/iinuji_cmd/views/board/view.h"
#include "iinuji/iinuji_cmd/views/config/view.h"
#include "iinuji/iinuji_cmd/views/home/view.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/tsiemene/view.h"
#include "iinuji/iinuji_cmd/views/data/view.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
struct IinujiUi {
  const CmdState& st;

  std::string status_line() const {
    std::ostringstream oss;
    oss << (st.screen == ScreenMode::Home ? "[F1 HOME]" : " F1 HOME ");
    oss << " ";
    oss << (st.screen == ScreenMode::Board ? "[F2 BOARD]" : " F2 BOARD ");
    oss << " ";
    oss << (st.screen == ScreenMode::Tsiemene ? "[F4 TSI]" : " F4 TSI ");
    oss << " ";
    oss << (st.screen == ScreenMode::Data ? "[F5 DATA]" : " F5 DATA ");
    oss << " ";
    oss << (st.screen == ScreenMode::Logs ? "[F8 LOGS]" : " F8 LOGS ");
    oss << " ";
    oss << (st.screen == ScreenMode::Config ? "[F9 CONFIG]" : " F9 CONFIG ");
    oss << " | type command and Enter";
    return oss.str();
  }

  void refresh(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& title,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& status,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
               const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& cmdline) const {
    if (st.screen == ScreenMode::Home) {
      const IinujiHomeView home{st};
      set_text_box(title, "cuwacunu.cmd - home", true);
      set_text_box(left, home.left(), true);
      set_text_box(right, home.right(), true);
    } else if (st.screen == ScreenMode::Board) {
      set_text_box(title, "cuwacunu.cmd - tsi board", true);
      set_text_box(left, make_board_left(st), false);
      set_text_box(right, make_board_right(st), true);
    } else if (st.screen == ScreenMode::Logs) {
      set_text_box(title, "cuwacunu.cmd - logs", true);
      const auto snap = cuwacunu::piaabo::dlog_snapshot();
      set_text_box(left, make_logs_left(st.logs, snap), false);
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

    set_text_box(status, status_line(), true);
    set_text_box(cmdline, "cmd> " + st.cmdline, false);
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
                       const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& cmdline) {
  IinujiUi{st}.refresh(title, status, left, right, cmdline);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
