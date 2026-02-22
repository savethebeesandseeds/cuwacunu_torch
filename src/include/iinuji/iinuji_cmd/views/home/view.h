#pragma once

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiHomeView {
  const CmdState& st;

  std::string left() const {
    std::ostringstream oss;
    oss << "CUWACUNU command terminal\n\n";
    oss << "focus: command-first workflow\n";
    oss << "screens:\n";
    oss << "  F1 home\n";
    oss << "  F2 tsi board\n";
    oss << "  F4 tsiemene\n";
    oss << "  F5 data\n";
    oss << "  F8 logs\n";
    oss << "  F9 config\n\n";

    oss << "board status:\n";
    if (!st.board.ok) {
      oss << "  invalid instruction: " << st.board.error << "\n";
    } else {
      oss << "  circuits=" << st.board.board.circuits.size() << "\n";
      for (std::size_t i = 0; i < st.board.board.circuits.size(); ++i) {
        oss << "  [" << (i + 1) << "] " << st.board.board.circuits[i].name << "\n";
      }
    }
    oss << "\nconfig tabs: " << st.config.tabs.size() << "\n";
    oss << "dlogs buffered: " << cuwacunu::piaabo::dlog_buffer_size()
        << "/" << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
    return oss.str();
  }

  static std::string right() {
    std::ostringstream oss;
    oss << "commands\n";
    oss << "  help\n";
    oss << "  iinuji.help() | iinuji.quit() | iinuji.exit()\n";
    oss << "  iinuji.screen.home() | .board() | .logs() | .tsi() | .data() | .config()\n";
    oss << "  iinuji.show() | .home() | .board() | .logs() | .tsi() | .data() | .config()\n";
    oss << "  iinuji.state.reload.board() | .data() | iinuji.config.reload()\n";
    oss << "  iinuji.board.list() | .select.next() | .select.prev() | .select.index.n1()\n";
    oss << "  iinuji.tsi.tabs() | .tab.next() | .tab.prev() | .tab.index.n1() | .tab.id.<token>()\n";
    oss << "  iinuji.data.reload() | .channels() | .ch.next()/prev()/index.n1()\n";
    oss << "  iinuji.data.plot.on()/off()/toggle() | .plot.mode.seq()/future()/weight()/norm()/bytes()\n";
    oss << "  iinuji.data.axis.idx()/key()/toggle() | .mask.on()/off()/toggle()\n";
    oss << "  iinuji.data.sample.next()/prev()/random()/rand()/index.n1()\n";
    oss << "  iinuji.data.dim.next()/prev()/index.n1()/id.<token>()\n";
    oss << "  iinuji.logs.clear()\n";
    oss << "  aliases: home/f1, board/f2, tsi/f4, data/f5, logs/f8, config/f9, help, quit/exit\n";
    oss << "  primitive translation: disabled\n";
    oss << "  quit\n";
    oss << "\nmouse\n";
    oss << "  wheel        : vertical scroll (both panels)\n";
    oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll (both panels)\n";
    return oss.str();
  }
};

inline std::string make_home_left(const CmdState& st) {
  return IinujiHomeView{st}.left();
}

inline std::string make_home_right() {
  return IinujiHomeView::right();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
