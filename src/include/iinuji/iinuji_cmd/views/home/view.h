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
    oss << "  F3 training\n";
    oss << "  F4 tsiemene\n";
    oss << "  F5 data\n";
    oss << "  F8 logs\n";
    oss << "  F9 config\n\n";

    oss << "board status:\n";
    if (!st.board.ok) {
      oss << "  invalid instruction: " << st.board.error << "\n";
    } else {
      oss << "  circuits=" << st.board.board.contracts.size() << "\n";
      for (std::size_t i = 0; i < st.board.board.contracts.size(); ++i) {
        oss << "  [" << (i + 1) << "] " << st.board.board.contracts[i].name << "\n";
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
    const auto call_cmds = [] {
      std::vector<std::string_view> out;
      auto append = [&](std::initializer_list<std::string_view> prefixes) {
        const auto cmds = canonical_paths::call_texts_by_prefix(prefixes);
        for (const auto cmd : cmds) {
          if (std::find(out.begin(), out.end(), cmd) == out.end()) out.push_back(cmd);
        }
      };
      append({"iinuji.help(", "iinuji.quit(", "iinuji.exit("});
      append({"iinuji.screen."});
      append({"iinuji.show."});
      append({"iinuji.refresh(", "iinuji.state.reload.", "iinuji.config.reload("});
      append({"iinuji.board."});
      append({"iinuji.training."});
      append({"iinuji.tsi."});
      append({"iinuji.data."});
      append({"iinuji.logs."});
      append({"iinuji.config."});
      return out;
    }();
    const auto pattern_cmds = [] {
      std::vector<std::string_view> out;
      auto append = [&](std::initializer_list<std::string_view> prefixes) {
        const auto cmds = canonical_paths::pattern_texts_by_prefix(prefixes);
        for (const auto cmd : cmds) {
          if (std::find(out.begin(), out.end(), cmd) == out.end()) out.push_back(cmd);
        }
      };
      append({"iinuji.board."});
      append({"iinuji.training."});
      append({"iinuji.tsi."});
      append({"iinuji.data."});
      append({"iinuji.config."});
      return out;
    }();
    for (const auto cmd : call_cmds) oss << "  " << cmd << "\n";
    for (const auto cmd : pattern_cmds) oss << "  " << cmd << "\n";
    oss << "  aliases: home/f1, board/f2, training/f3, tsi/f4, data/f5, logs/f8, config/f9, help, quit/exit\n";
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
