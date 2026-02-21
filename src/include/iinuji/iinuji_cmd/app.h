#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"

#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/iinuji_cmd/views.h"
#include "iinuji/iinuji_cmd/views/board/app.h"
#include "iinuji/iinuji_cmd/views/config/app.h"
#include "iinuji/iinuji_cmd/views/data/app.h"
#include "iinuji/iinuji_cmd/views/tsiemene/app.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

#ifndef BUTTON_SHIFT
#define BUTTON_SHIFT 0
#endif
#ifndef BUTTON_CTRL
#define BUTTON_CTRL 0
#endif
#ifndef BUTTON_ALT
#define BUTTON_ALT 0
#endif
#ifndef BUTTON4_CLICKED
#define BUTTON4_CLICKED 0
#endif
#ifndef BUTTON5_CLICKED
#define BUTTON5_CLICKED 0
#endif
#ifndef BUTTON4_DOUBLE_CLICKED
#define BUTTON4_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON5_DOUBLE_CLICKED
#define BUTTON5_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON4_TRIPLE_CLICKED
#define BUTTON4_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON5_TRIPLE_CLICKED
#define BUTTON5_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON6_PRESSED
#define BUTTON6_PRESSED 0
#endif
#ifndef BUTTON7_PRESSED
#define BUTTON7_PRESSED 0
#endif

inline int run(const char* config_folder = "/cuwacunu/src/config/") try {
  struct DlogTerminalOutputGuard {
    bool prev{true};
    ~DlogTerminalOutputGuard() {
      cuwacunu::piaabo::dlog_set_terminal_output_enabled(prev);
    }
  };
  cuwacunu::piaabo::dlog_set_buffer_capacity(6000);
  DlogTerminalOutputGuard dlog_guard{cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  log_info("[iinuji_cmd] boot config_folder=%s\n", config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

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
  mousemask(ALL_MOUSE_EVENTS, nullptr);
  mouseinterval(0);

  using namespace cuwacunu::iinuji;

  auto root = create_grid_container(
      "root",
      {len_spec::px(3), len_spec::px(2), len_spec::frac(0.68), len_spec::frac(0.32), len_spec::px(3)},
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

  auto logs = create_buffer_box(
      "logs",
      2000,
      buffer_dir_t::UpDown,
      iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", true, "#5E5E68", false, false, " command log "});
  place_in_grid(logs, 3, 0);
  root->add_child(logs);

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
  state.config = load_config_view_from_config();
  clamp_selected_tab(state);
  state.board = load_board_from_config();
  clamp_selected_circuit(state);
  state.data = load_data_view_from_config(&state.board);
  clamp_selected_data_channel(state);
  clamp_data_plot_mode(state);
  clamp_data_plot_x_axis(state);
  clamp_data_nav_focus(state);
  clamp_selected_tsi_tab(state);

  append_log(logs, "cuwacunu command terminal ready", "boot", "#9fd8ff");
  append_log(logs, "F1 home | F2 board | F3 logs | F4 tsi | F5 data | F9 config | type 'help' for commands", "boot", "#9fd8ff");
  if (!state.config.ok) {
    append_log(logs, "config tabs invalid: " + state.config.error, "warn", "#ffd27f");
  } else {
    append_log(logs, "config tabs loaded: tabs=" + std::to_string(state.config.tabs.size()), "boot", "#9fd8ff");
  }
  if (!state.board.ok) {
    append_log(logs, "board invalid: " + state.board.error, "warn", "#ffd27f");
  } else {
    append_log(logs, "board loaded: circuits=" + std::to_string(state.board.board.circuits.size()), "boot", "#9fd8ff");
  }
  if (!state.data.ok) {
    append_log(logs, "data view invalid: " + state.data.error, "warn", "#ffd27f");
  } else {
    append_log(logs, "data view loaded: channels=" + std::to_string(state.data.channels.size()), "boot", "#9fd8ff");
  }
  append_log(logs, "mouse wheel=vertical scroll | Shift/Ctrl/Alt+wheel=horizontal scroll (active screen panels)", "boot", "#9fd8ff");

  constexpr int kVScrollStep = 3;
  constexpr int kHScrollStep = 10;

  auto scroll_text_box = [&](const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box, int dy, int dx) {
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
    if (!tb) return;
    tb->scroll_by(dy, dx);
  };

  auto scroll_active_screen = [&](int dy, int dx) {
    if (dy == 0 && dx == 0) return;
    scroll_text_box(left, dy, dx);
    scroll_text_box(right, dy, dx);
  };

  struct ScrollCaps {
    bool v{false};
    bool h{false};
  };
  auto panel_scroll_caps = [&](const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box) {
    ScrollCaps out{};
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
    if (!tb) return out;

    const Rect r = content_rect(*box);
    const int W = std::max(0, r.w);
    const int H = std::max(0, r.h);
    if (W <= 0 || H <= 0) return out;

    int reserve_v = 0;
    int reserve_h = 0;
    int text_w = W;
    int text_h = H;
    int max_line_len = 0;
    std::vector<std::string> lines;

    for (int it = 0; it < 3; ++it) {
      text_w = std::max(0, W - reserve_v);
      text_h = std::max(0, H - reserve_h);
      if (text_w <= 0 || text_h <= 0) return out;

      lines = tb->wrap
          ? wrap_text(tb->content, std::max(1, text_w))
          : split_lines_keep_empty(tb->content);
      max_line_len = 0;
      for (const auto& ln : lines) max_line_len = std::max(max_line_len, (int)ln.size());

      const bool need_h = (!tb->wrap && max_line_len > text_w);
      const int reserve_h_new = need_h ? 1 : 0;
      const int text_h_if = std::max(0, H - reserve_h_new);
      const bool need_v = ((int)lines.size() > text_h_if);
      const int reserve_v_new = need_v ? 1 : 0;

      if (reserve_h_new == reserve_h && reserve_v_new == reserve_v) break;
      reserve_h = reserve_h_new;
      reserve_v = reserve_v_new;
    }

    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0) return out;

    lines = tb->wrap
        ? wrap_text(tb->content, std::max(1, text_w))
        : split_lines_keep_empty(tb->content);
    max_line_len = 0;
    for (const auto& ln : lines) max_line_len = std::max(max_line_len, (int)ln.size());

    out.v = ((int)lines.size() > text_h);
    out.h = (!tb->wrap && max_line_len > text_w);
    return out;
  };

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
        dirty = true;
      }
    }

    if (dirty) {
      refresh_ui(state, title, status, left, right, cmdline);
      layout_tree(root, Rect{0, 0, W, H});
      ::erase();
      render_tree(root);
      render_data_plot_overlay(state, data_rt, left, right, logs);
      ::refresh();
      dirty = false;
    }

    const int desired_timeout = (state.screen == ScreenMode::Logs) ? 50 : -1;
    if (desired_timeout != current_input_timeout) {
      ::timeout(desired_timeout);
      current_input_timeout = desired_timeout;
    }

    const int ch = getch();
    if (ch == ERR) continue;
    if (ch == KEY_RESIZE) {
      dirty = true;
      continue;
    }

    if (ch == KEY_MOUSE) {
      MEVENT me{};
      if (getmouse(&me) != OK) continue;

      const bool mod_shift = (me.bstate & BUTTON_SHIFT) != 0;
      const bool mod_ctrl = (me.bstate & BUTTON_CTRL) != 0;
      const bool mod_alt = (me.bstate & BUTTON_ALT) != 0;
      bool horizontal_mod = (mod_shift || mod_ctrl || mod_alt);

      const auto l_caps = panel_scroll_caps(left);
      const auto r_caps = panel_scroll_caps(right);
      const bool any_v = l_caps.v || r_caps.v;
      const bool any_h = l_caps.h || r_caps.h;

      bool horizontal_auto = horizontal_mod || (!any_v && any_h);

      int dy = 0;
      int dx = 0;
      bool wheel = false;

      if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED | BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
        wheel = true;
        if (horizontal_auto) dx -= kHScrollStep;
        else dy -= kVScrollStep;
      }
      if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED | BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
        wheel = true;
        if (horizontal_auto) dx += kHScrollStep;
        else dy += kVScrollStep;
      }
      if (me.bstate & BUTTON6_PRESSED) {
        wheel = true;
        dx -= kHScrollStep;
      }
      if (me.bstate & BUTTON7_PRESSED) {
        wheel = true;
        dx += kHScrollStep;
      }
      if (wheel) {
        scroll_active_screen(dy, dx);
        dirty = true;
      }
      continue;
    }

    if (ch == KEY_F(1)) {
      state.screen = ScreenMode::Home;
      append_log(logs, "screen=home", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=home\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(2)) {
      state.screen = ScreenMode::Board;
      append_log(logs, "screen=board", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=board\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(3)) {
      state.screen = ScreenMode::Logs;
      append_log(logs, "screen=logs", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=logs\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(4)) {
      state.screen = ScreenMode::Tsiemene;
      append_log(logs, "screen=tsi", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=tsi\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(5)) {
      state.screen = ScreenMode::Data;
      append_log(logs, "screen=data", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=data\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(9)) {
      state.screen = ScreenMode::Config;
      append_log(logs, "screen=config", "nav", "#d0d0d0");
      log_info("[iinuji_cmd] screen=config\n");
      dirty = true;
      continue;
    }

    auto append_nav = [&](const std::string& text, const std::string& label, const std::string& color) {
      append_log(logs, text, label, color);
    };

    if (handle_data_key(state, data_rt, ch, append_nav)) {
      dirty = true;
      continue;
    }
    if (handle_board_key(state, ch)) {
      dirty = true;
      continue;
    }
    if (handle_tsi_key(state, ch)) {
      dirty = true;
      continue;
    }
    if (handle_config_key(state, ch)) {
      dirty = true;
      continue;
    }

    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      const std::string cmd = state.cmdline;
      state.cmdline.clear();
      run_command(state, cmd, logs);
      clamp_selected_circuit(state);
      clamp_selected_tsi_tab(state);
      clamp_selected_data_channel(state);
      clamp_data_plot_mode(state);
      clamp_data_plot_x_axis(state);
      clamp_data_nav_focus(state);
      clamp_data_plot_feature_dim(state);
      clamp_data_plot_sample_index(state);
      clamp_selected_tab(state);
      init_data_runtime(state, data_rt, false);
      dirty = true;
      continue;
    }

    if (ch == 21) {  // Ctrl+U
      state.cmdline.clear();
      dirty = true;
      continue;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (!state.cmdline.empty()) {
        state.cmdline.pop_back();
        dirty = true;
      }
      continue;
    }

    if (ch >= 32 && ch <= 126) {
      state.cmdline.push_back(static_cast<char>(ch));
      dirty = true;
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
