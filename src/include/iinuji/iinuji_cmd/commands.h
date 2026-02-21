#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"

#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "iinuji/iinuji_cmd/views/config/commands.h"
#include "iinuji/iinuji_cmd/views/data/commands.h"
#include "iinuji/iinuji_cmd/views/home/commands.h"
#include "iinuji/iinuji_cmd/views/logs/commands.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void reload_board(CmdState& st) {
  st.board = load_board_from_config();
  clamp_selected_circuit(st);
  st.data = load_data_view_from_config(&st.board);
  clamp_selected_data_channel(st);
  clamp_data_plot_mode(st);
  clamp_data_plot_x_axis(st);
  clamp_data_nav_focus(st);
  clamp_data_plot_feature_dim(st);
  clamp_data_plot_sample_index(st);
}

inline void reload_data(CmdState& st) {
  st.data = load_data_view_from_config(&st.board);
  clamp_selected_data_channel(st);
  clamp_data_plot_mode(st);
  clamp_data_plot_x_axis(st);
  clamp_data_nav_focus(st);
  clamp_data_plot_feature_dim(st);
  clamp_data_plot_sample_index(st);
}

inline void reload_config_and_board(CmdState& st) {
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  st.config = load_config_view_from_config();
  clamp_selected_tab(st);
  st.board = load_board_from_config();
  clamp_selected_circuit(st);
  st.data = load_data_view_from_config(&st.board);
  clamp_selected_data_channel(st);
  clamp_data_plot_mode(st);
  clamp_data_plot_x_axis(st);
  clamp_data_nav_focus(st);
  clamp_data_plot_feature_dim(st);
  clamp_data_plot_sample_index(st);
}

inline void run_command(CmdState& st,
                        const std::string& raw,
                        const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& log_box) {
  const std::string cmd = raw;
  if (cmd.empty()) return;

  append_log(log_box, "$ " + cmd, "cmd", "#9ecfff");
  log_info("[iinuji_cmd.cmd] %s\n", cmd.c_str());

  std::istringstream iss(cmd);
  std::string a0;
  iss >> a0;
  a0 = to_lower_copy(a0);

  auto push_info = [&](const std::string& m) {
    append_log(log_box, m, "info", "#b8d8b8");
    log_info("[iinuji_cmd] %s\n", m.c_str());
  };
  auto push_warn = [&](const std::string& m) {
    append_log(log_box, m, "warn", "#ffd27f");
    log_warn("[iinuji_cmd] %s\n", m.c_str());
  };
  auto push_err = [&](const std::string& m) {
    append_log(log_box, m, "err", "#ff9ea1");
    log_err("[iinuji_cmd] %s\n", m.c_str());
  };
  auto append = [&](const std::string& text, const std::string& label, const std::string& color) {
    append_log(log_box, text, label, color);
  };

  if (a0 == "help") {
    push_info("global: help | quit | home | board | logs | tsi | data | config");
    push_info("screen: screen home|board|logs|tsi|data|config");
    push_info("reload: reload [board|data|config]");
    push_info("board: next | prev | circuit N | list | show");
    push_info("tsi: tsi | tsi tabs | tsi tab next|prev|N|<id> | show tsi");
    push_info("data: data reload | plot [on|off|toggle] | data plot [on|off|toggle|seq|future|weight|norm|bytes] | data x idx|key|toggle");
    push_info("data: data ch next|prev|N | data sample next|prev|random|N | data dim next|prev|N|<name> | data mask on|off|toggle | show data");
    push_info("config: tab next|prev|N|<id> | tabs | show config");
    push_info("logs: logs | logs clear | show logs");
    push_info("input: printable keys go to cmd>, arrows control screen state");
    push_info("mouse: wheel=vertical scroll (active screen panels), Shift/Ctrl/Alt+wheel=horizontal scroll");
    return;
  }

  if (a0 == "quit" || a0 == "exit") {
    st.running = false;
    return;
  }

  if (handle_home_command(st, a0, push_info)) return;

  if (a0 == "board" || a0 == "f2") {
    st.screen = ScreenMode::Board;
    push_info("screen=board");
    return;
  }

  if (handle_logs_command(st, a0, iss, push_info)) return;

  if (handle_tsi_command(st, a0, iss, push_info, push_warn, push_err, append)) return;

  if (handle_data_command(st, a0, iss, push_info, push_warn, push_err, append)) return;

  if (a0 == "plot") {
    std::string arg;
    iss >> arg;
    arg = to_lower_copy(arg);
    if (arg.empty() || arg == "on" || arg == "open") {
      st.screen = ScreenMode::Data;
      st.data.plot_view = true;
      push_info("data plotview=on");
      return;
    }
    if (arg == "off") {
      st.screen = ScreenMode::Data;
      st.data.plot_view = false;
      push_info("data plotview=off");
      return;
    }
    if (arg == "toggle") {
      st.screen = ScreenMode::Data;
      st.data.plot_view = !st.data.plot_view;
      push_info(std::string("data plotview=") + (st.data.plot_view ? "on" : "off"));
      return;
    }
    push_err("usage: plot [on|off|toggle]");
    return;
  }

  if (handle_config_command(st, a0, iss, push_info, push_warn, push_err, append)) return;

  if (a0 == "screen") {
    std::string arg;
    iss >> arg;
    arg = to_lower_copy(arg);
    if (arg == "home" || arg == "f1") {
      st.screen = ScreenMode::Home;
      push_info("screen=home");
      return;
    }
    if (arg == "board" || arg == "f2") {
      st.screen = ScreenMode::Board;
      push_info("screen=board");
      return;
    }
    if (arg == "logs" || arg == "f3") {
      st.screen = ScreenMode::Logs;
      push_info("screen=logs");
      return;
    }
    if (arg == "tsi" || arg == "f4") {
      st.screen = ScreenMode::Tsiemene;
      push_info("screen=tsi");
      return;
    }
    if (arg == "data" || arg == "f5") {
      st.screen = ScreenMode::Data;
      push_info("screen=data");
      return;
    }
    if (arg == "config" || arg == "f9") {
      st.screen = ScreenMode::Config;
      push_info("screen=config");
      return;
    }
    push_err("usage: screen home|board|logs|tsi|data|config");
    return;
  }

  if (a0 == "reload") {
    std::string arg;
    iss >> arg;
    arg = to_lower_copy(arg);
    if (arg.empty() || arg == "board") {
      reload_board(st);
      if (st.board.ok) push_info("board reloaded");
      else push_err("reload board failed: " + st.board.error);
      return;
    }
    if (arg == "data") {
      reload_data(st);
      if (st.data.ok) push_info("data reloaded");
      else push_err("reload data failed: " + st.data.error);
      return;
    }
    if (arg == "config" || arg == "all") {
      reload_config_and_board(st);
      if (!st.config.ok) push_err("reload config failed: " + st.config.error);
      else push_info("config reloaded: tabs=" + std::to_string(st.config.tabs.size()));
      if (!st.board.ok) push_err("board reload after config failed: " + st.board.error);
      else push_info("board reloaded");
      if (!st.data.ok) push_err("data reload after config failed: " + st.data.error);
      else push_info("data reloaded");
      return;
    }
    push_err("usage: reload [board|data|config]");
    return;
  }

  if (handle_board_nav_command(st, a0, iss, push_info, push_warn, push_err)) return;

  if (handle_board_list_command(st, a0, push_warn, push_err, append)) return;

  if (a0 == "show") {
    std::string arg;
    iss >> arg;
    arg = to_lower_copy(arg);

    if (arg == "config" || (arg.empty() && st.screen == ScreenMode::Config)) {
      handle_config_show(st, push_warn, append);
      return;
    }

    if (arg == "tsi" || (arg.empty() && st.screen == ScreenMode::Tsiemene)) {
      handle_tsi_show(st, push_warn, append);
      return;
    }

    if (arg == "data" || (arg.empty() && st.screen == ScreenMode::Data)) {
      handle_data_show(st, append);
      return;
    }

    handle_board_show(st, push_warn, push_err, append);
    return;
  }

  push_err("unknown command: " + a0);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
