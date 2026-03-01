#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <ncursesw/ncurses.h>
#ifdef timeout
#undef timeout
#endif

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/primitives/editor.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"

#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/app/keymap.h"
#include "iinuji/iinuji_cmd/app/overlays.h"
#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/iinuji_cmd/views.h"
#include "iinuji/iinuji_cmd/views/board/app.h"
#include "iinuji/iinuji_cmd/views/data/app.h"
#include "iinuji/iinuji_cmd/views/tsiemene/app.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline int run(const char* config_folder = "/cuwacunu/src/config/") try {
  struct DlogTerminalOutputGuard {
    bool prev{true};
    ~DlogTerminalOutputGuard() {
      cuwacunu::piaabo::dlog_set_terminal_output_enabled(prev);
    }
  };
  DlogTerminalOutputGuard dlog_guard{cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  log_info("[iinuji_cmd] boot config_folder=%s\n", config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const int logs_cap_cfg =
      std::max(1, cuwacunu::piaabo::dconfig::config_space_t::get<int>(
                      "GENERAL",
                      "iinuji_logs_buffer_capacity"));
  cuwacunu::piaabo::dlog_set_buffer_capacity(static_cast<std::size_t>(logs_cap_cfg));

  cuwacunu::iinuji::NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms = -1;
  cuwacunu::iinuji::NcursesApp app(app_opts);
#if defined(NCURSES_VERSION)
  ::set_escdelay(25);
#endif

  if (has_colors()) {
    start_color();
    use_default_colors();
  }
  cuwacunu::iinuji::set_global_background("#101014");

  using namespace cuwacunu::iinuji;

  auto root = create_grid_container(
      "root",
      {len_spec::px(3), len_spec::px(2), len_spec::frac(1.0), len_spec::px(2), len_spec::px(3)},
      {len_spec::frac(1.0)},
      0,
      0,
      iinuji_layout_t{layout_mode_t::Normalized, 0, 0, 1, 1, true},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});

  auto title = create_text_box(
      "title",
      "",
      true,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#EDEDED", "#202028", true, "#6C6C75", true, false, " cuwacunu.cmd "});
  place_in_grid(title, 0, 0);
  root->add_child(title);

  auto status = create_text_box(
      "status",
      "",
      true,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#B8B8BF", "#101014", false, "#101014"});
  place_in_grid(status, 1, 0);
  root->add_child(status);

  auto workspace = create_grid_container(
      "workspace",
      {len_spec::frac(1.0)},
      {len_spec::frac(0.70), len_spec::frac(0.30)},
      1,
      1,
      iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});
  place_in_grid(workspace, 2, 0);
  root->add_child(workspace);

  auto left = create_text_box(
      "left",
      "",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#D0D0D0", "#101014", true, "#5E5E68", false, false, " view "});
  place_in_grid(left, 0, 0);
  workspace->add_child(left);

  auto right = create_text_box(
      "right",
      "",
      true,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#C8C8CE", "#101014", true, "#5E5E68", false, false, " context "});
  place_in_grid(right, 0, 1);
  workspace->add_child(right);

  auto bottom = create_text_box(
      "bottom",
      "",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#A8A8AF", "#101014", true, "#5E5E68", false, false, " message "});
  place_in_grid(bottom, 3, 0);
  root->add_child(bottom);

  auto cmdline = create_text_box(
      "cmdline",
      "cmd> ",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#E8E8E8", "#101014", true, "#5E5E68", false, false, " command "});
  cmdline->focusable = true;
  cmdline->focused = true;
  place_in_grid(cmdline, 4, 0);
  root->add_child(cmdline);

  CmdState state{};
  const auto boot_contract_hash = resolve_configured_board_contract_hash();
  state.config = load_config_view_from_config(boot_contract_hash);
  clamp_selected_tab(state);
  state.board = load_board_from_contract_hash(boot_contract_hash);
  clamp_board_navigation_state(state);
  state.data = load_data_view_from_config(&state.board);
  clamp_selected_data_channel(state);
  clamp_data_plot_mode(state);
  clamp_data_plot_x_axis(state);
  clamp_data_nav_focus(state);
  clamp_selected_training_tab(state);
  clamp_selected_training_hash(state);
  clamp_selected_tsi_tab(state);

  auto set_mouse_capture = [&](bool enabled) {
    mousemask(enabled ? ALL_MOUSE_EVENTS : 0, nullptr);
    mouseinterval(0);
    state.logs.mouse_capture = enabled;
  };
  set_mouse_capture(state.logs.mouse_capture);

  log_info("[iinuji_cmd] cuwacunu command terminal ready\n");
  log_info("[iinuji_cmd] F1 home | F2 board | F3 training | F4 tsi | F5 data | F8 logs | F9 config | type 'help' for commands\n");
  log_info("[iinuji_cmd] logs setting 'mouse capture' controls terminal select/copy mode\n");
  if (!state.config.ok) {
    log_warn("[iinuji_cmd] config tabs invalid: %s\n", state.config.error.c_str());
  } else {
    log_info("[iinuji_cmd] config tabs loaded: tabs=%zu\n", state.config.tabs.size());
  }
  if (!state.board.ok) {
    log_warn("[iinuji_cmd] board invalid: %s\n", state.board.error.c_str());
  } else {
    log_info("[iinuji_cmd] board loaded: circuits=%zu\n", state.board.board.contracts.size());
  }
  if (!state.data.ok) {
    log_warn("[iinuji_cmd] data view invalid: %s\n", state.data.error.c_str());
  } else {
    log_info("[iinuji_cmd] data view loaded: channels=%zu\n", state.data.channels.size());
  }
  log_info("[iinuji_cmd] mouse wheel=vertical scroll | Shift/Ctrl/Alt+wheel=horizontal scroll (active screen panels)\n");

  constexpr int kVScrollStep = 6;
  constexpr int kHScrollStep = 16;

  DataAppRuntime data_rt{};
  init_data_runtime(state, data_rt, true);

  auto dlog_tail_seq = []() -> std::uint64_t {
    const auto tail = cuwacunu::piaabo::dlog_snapshot(1);
    if (tail.empty()) return 0;
    return tail.back().seq;
  };

  bool dirty = true;
  int last_h = -1;
  int last_w = -1;
  std::uint64_t last_log_seq = dlog_tail_seq();
  int current_input_timeout = app_opts.input_timeout_ms;
  while (state.running) {
    init_data_runtime(state, data_rt, false);

    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H != last_h || W != last_w) {
      last_h = H;
      last_w = W;
      dirty = true;
    }

    if (state.screen == ScreenMode::Logs) {
      const std::uint64_t log_seq = dlog_tail_seq();
      if (log_seq != last_log_seq) {
        last_log_seq = log_seq;
        if (state.logs.auto_follow) jump_logs_to_bottom(state, left);
        dirty = true;
      }
    }
    if (apply_logs_pending_actions(state, left, right)) dirty = true;

    if (dirty) {
      refresh_board_editor_diagnostic(state);
      refresh_ui(state, title, status, left, right, bottom, cmdline);
      if (state.screen == ScreenMode::Logs && state.logs.auto_follow) {
        jump_logs_to_bottom(state, left);
      }
      layout_tree(root, Rect{0, 0, W, H});
      ::erase();
      render_tree(root);
      render_data_plot_overlay(state, data_rt, left, right);
      render_help_overlay(state, left, right);
      render_logs_scroll_controls(state, left);
      render_board_error_line_overlay(state, left);
      render_board_completion_overlay(state, left);
      ::refresh();
      dirty = false;
    }

    const int desired_timeout = (state.screen == ScreenMode::Logs) ? 50 : -1;
    if (desired_timeout != current_input_timeout) {
      ::wtimeout(stdscr, desired_timeout);
      current_input_timeout = desired_timeout;
    }

    const int ch = getch();
    if (ch == ERR) continue;
    if (ch == KEY_RESIZE) {
      dirty = true;
      continue;
    }
    const auto key_result = dispatch_app_key(
        state,
        ch,
        data_rt,
        left,
        right,
        kVScrollStep,
        kHScrollStep,
        set_mouse_capture);
    if (key_result.handled) {
      if (key_result.dirty) dirty = true;
      continue;
    }
  }

  return 0;
} catch (const std::exception& e) {
  endwin();
  log_err("[iinuji_cmd] exception: %s\n", e.what());
  return 1;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
