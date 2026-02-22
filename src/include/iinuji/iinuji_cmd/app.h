#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
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
#include "iinuji/iinuji_cmd/views/data/app.h"

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
#ifndef BUTTON1_CLICKED
#define BUTTON1_CLICKED 0
#endif
#ifndef BUTTON1_PRESSED
#define BUTTON1_PRESSED 0
#endif
#ifndef BUTTON1_DOUBLE_CLICKED
#define BUTTON1_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON1_TRIPLE_CLICKED
#define BUTTON1_TRIPLE_CLICKED 0
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
  DlogTerminalOutputGuard dlog_guard{cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  log_info("[iinuji_cmd] boot config_folder=%s\n", config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const int logs_cap_cfg =
      std::max(1, cuwacunu::piaabo::dconfig::config_space_t::get<int>(
                      "GENERAL",
                      "iinuji_logs_buffer_capacity",
                      6000));
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
      {len_spec::px(3), len_spec::px(2), len_spec::frac(1.0), len_spec::px(3)},
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

  auto cmdline = create_text_box(
      "cmdline",
      "cmd> ",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#E8E8E8", "#101014", true, "#5E5E68", false, false, " command "});
  cmdline->focusable = true;
  cmdline->focused = true;
  place_in_grid(cmdline, 3, 0);
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

  auto set_mouse_capture = [&](bool enabled) {
    mousemask(enabled ? ALL_MOUSE_EVENTS : 0, nullptr);
    mouseinterval(0);
    state.logs.mouse_capture = enabled;
  };
  set_mouse_capture(state.logs.mouse_capture);

  log_info("[iinuji_cmd] cuwacunu command terminal ready\n");
  log_info("[iinuji_cmd] F1 home | F2 board | F4 tsi | F5 data | F8 logs | F9 config | type 'help' for commands\n");
  log_info("[iinuji_cmd] logs setting 'mouse capture' controls terminal select/copy mode\n");
  if (!state.config.ok) {
    log_warn("[iinuji_cmd] config tabs invalid: %s\n", state.config.error.c_str());
  } else {
    log_info("[iinuji_cmd] config tabs loaded: tabs=%zu\n", state.config.tabs.size());
  }
  if (!state.board.ok) {
    log_warn("[iinuji_cmd] board invalid: %s\n", state.board.error.c_str());
  } else {
    log_info("[iinuji_cmd] board loaded: circuits=%zu\n", state.board.board.circuits.size());
  }
  if (!state.data.ok) {
    log_warn("[iinuji_cmd] data view invalid: %s\n", state.data.error.c_str());
  } else {
    log_info("[iinuji_cmd] data view loaded: channels=%zu\n", state.data.channels.size());
  }
  log_info("[iinuji_cmd] mouse wheel=vertical scroll | Shift/Ctrl/Alt+wheel=horizontal scroll (active screen panels)\n");

  constexpr int kVScrollStep = 3;
  constexpr int kHScrollStep = 10;

  auto scroll_text_box = [&](const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box, int dy, int dx) {
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
    if (!tb) return;
    tb->scroll_by(dy, dx);
  };

  auto scroll_active_screen = [&](int dy, int dx) {
    if (dy == 0 && dx == 0) return;
    if (state.screen == ScreenMode::Logs && dy != 0) {
      state.logs.auto_follow = false;
    }
    scroll_text_box(left, dy, dx);
    scroll_text_box(right, dy, dx);
  };

  auto jump_logs_to_top = [&]() {
    if (state.screen != ScreenMode::Logs) return;
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(left);
    if (!tb) return;
    tb->scroll_y = 0;
    tb->scroll_x = 0;
    state.logs.auto_follow = false;
  };

  auto jump_logs_to_bottom = [&]() {
    if (state.screen != ScreenMode::Logs) return;
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(left);
    if (!tb) return;
    tb->scroll_y = std::numeric_limits<int>::max();
    state.logs.auto_follow = true;
  };

  auto scroll_help_overlay = [&](int dy, int dx) {
    if (!state.help_view) return;
    if (dy != 0) {
      const int y = state.help_scroll_y + dy;
      state.help_scroll_y = std::max(0, y);
    }
    if (dx != 0) {
      const int x = state.help_scroll_x + dx;
      state.help_scroll_x = std::max(0, x);
    }
  };

  auto dispatch_canonical_internal_call = [&](std::string_view canonical_call) {
    IinujiPathHandlers handlers{state};
    auto push_ignore = [](const std::string&) {};
    auto append_ignore = [](const std::string&, const std::string&, const std::string&) {};
    auto push_err = [&](const std::string& msg) {
      log_err("[iinuji_cmd.internal] %s\n", msg.c_str());
    };
    return handlers.dispatch_canonical_text(
        std::string(canonical_call),
        push_ignore,
        push_ignore,
        push_err,
        append_ignore);
  };

  auto dispatch_logs_setting_adjust = [&](bool forward) {
    if (state.screen != ScreenMode::Logs) return false;
    switch (state.logs.selected_setting) {
      case 0: {
        constexpr std::array<std::string_view, 5> kLevelCalls = {
            canonical_paths::kLogsSettingsLevelDebug,
            canonical_paths::kLogsSettingsLevelInfo,
            canonical_paths::kLogsSettingsLevelWarning,
            canonical_paths::kLogsSettingsLevelError,
            canonical_paths::kLogsSettingsLevelFatal,
        };
        std::size_t idx = static_cast<std::size_t>(state.logs.level_filter);
        idx = forward ? ((idx + 1u) % kLevelCalls.size()) : ((idx + kLevelCalls.size() - 1u) % kLevelCalls.size());
        return dispatch_canonical_internal_call(kLevelCalls[idx]);
      }
      case 1:
        return dispatch_canonical_internal_call(canonical_paths::kLogsSettingsDateToggle);
      case 2:
        return dispatch_canonical_internal_call(canonical_paths::kLogsSettingsThreadToggle);
      case 3:
        return dispatch_canonical_internal_call(canonical_paths::kLogsSettingsColorToggle);
      case 4:
        return dispatch_canonical_internal_call(canonical_paths::kLogsSettingsFollowToggle);
      case 5:
        return dispatch_canonical_internal_call(canonical_paths::kLogsSettingsMouseCaptureToggle);
      default:
        return false;
    }
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

  auto merged_workspace_area = [&]() -> std::optional<Rect> {
    Rect area{};
    bool have = false;
    for (const auto& box : {left, right}) {
      const Rect r = content_rect(*box);
      if (r.w <= 0 || r.h <= 0) continue;
      if (!have) {
        area = r;
        have = true;
      } else {
        area = merge_rects(area, r);
      }
    }
    if (!have) return std::nullopt;
    if (area.w < 20 || area.h < 8) return std::nullopt;
    return area;
  };

  auto close_corner_hit = [](const Rect& area, int mx, int my) {
    const int close_x0 = area.x + std::max(0, area.w - 4);
    const int close_x1 = close_x0 + 2;  // "[x]"
    const int close_y = area.y;
    return my == close_y && mx >= close_x0 && mx <= close_x1;
  };

  auto logs_scroll_control_area = [&]() -> std::optional<Rect> {
    if (state.screen != ScreenMode::Logs || state.help_view) return std::nullopt;
    const Rect r = content_rect(*left);
    if (r.w < 4 || r.h < 3) return std::nullopt;
    return r;
  };

  auto logs_jump_top_hit = [&](int mx, int my) {
    const auto area = logs_scroll_control_area();
    if (!area.has_value()) return false;
    const int x0 = area->x + std::max(0, area->w - 3);
    const int x1 = x0 + 2;
    const int y = area->y;
    return my == y && mx >= x0 && mx <= x1;
  };

  auto logs_jump_bottom_hit = [&](int mx, int my) {
    const auto area = logs_scroll_control_area();
    if (!area.has_value()) return false;
    const int x0 = area->x + std::max(0, area->w - 3);
    const int x1 = x0 + 2;
    const int y = area->y + area->h - 1;
    return my == y && mx >= x0 && mx <= x1;
  };

  auto help_overlay_close_hit = [&](int mx, int my) {
    if (!state.help_view) return false;
    const auto area = merged_workspace_area();
    if (!area.has_value()) return false;
    return close_corner_hit(*area, mx, my);
  };

  auto render_help_overlay = [&]() {
    if (!state.help_view) return;
    const auto area_opt = merged_workspace_area();
    if (!area_opt.has_value()) return;
    auto* R = get_renderer();
    if (!R) return;

    const Rect area = *area_opt;
    const short bg_pair = static_cast<short>(get_color_pair("#E8EDF5", "#11151C"));
    const short hint_pair = static_cast<short>(get_color_pair("#FFD26E", "#11151C"));

    R->fillRect(area.y, area.x, area.h, area.w, bg_pair);
    {
      constexpr const char* kClose = "[x]";
      const int close_x = area.x + std::max(0, area.w - 4);
      R->putText(area.y, close_x, kClose, 3, hint_pair, true, false);
    }

    const int inner_x = area.x + 1;
    const int inner_y = area.y + 1;
    const int inner_w = std::max(0, area.w - 2);
    const int inner_h = std::max(0, area.h - 2);
    if (inner_w <= 0 || inner_h <= 0) return;

    std::vector<std::string> lines;
    lines.reserve(8 + canonical_paths::help_entries().size() * 2 + canonical_paths::alias_entries().size());
    lines.push_back(std::string("\x1f") + "HELP OVERLAY (auto-generated)");
    lines.push_back("Esc or click [x] to close");
    lines.push_back("Arrow keys and PageUp/PageDown scroll. Home=start End=tail.");
    lines.push_back("");
    lines.push_back(std::string("\x1f") + "Canonical Commands");
    for (const auto& entry : canonical_paths::help_entries()) {
      lines.push_back(std::string("\x1c") + "  " + std::string(entry.first));
      if (!entry.second.empty()) lines.push_back("    " + std::string(entry.second));
    }
    lines.push_back("");
    lines.push_back(std::string("\x1f") + "Aliases (direct shorthand)");
    for (const auto& alias : canonical_paths::alias_entries()) {
      lines.push_back(std::string("\x1d") + "  " + std::string(alias.first) + " -> " + std::string(alias.second));
    }
    lines.push_back("");
    lines.push_back("Primitive translation disabled. Use canonical paths or aliases.");

    std::ostringstream text;
    for (const auto& line : lines) text << line << "\n";

    auto overlay = create_text_box(
        "__help_overlay__",
        text.str(),
        false,
        text_align_t::Left,
        iinuji_layout_t{},
        iinuji_style_t{"#C6D3E2", "#11151C", false, "#5E5E68"});
    overlay->screen = Rect{inner_x, inner_y, inner_w, inner_h};
    auto tb = as<cuwacunu::iinuji::textBox_data_t>(overlay);
    if (!tb) return;
    tb->scroll_y = std::max(0, state.help_scroll_y);
    tb->scroll_x = std::max(0, state.help_scroll_x);
    render_text(*overlay);
    state.help_scroll_y = tb->scroll_y;
    state.help_scroll_x = tb->scroll_x;
  };

  auto render_logs_scroll_controls = [&]() {
    const auto area = logs_scroll_control_area();
    if (!area.has_value()) return;
    auto* R = get_renderer();
    if (!R) return;
    const short pair = static_cast<short>(get_color_pair("#FFD26E", "#101014"));
    const int x = area->x + std::max(0, area->w - 3);
    R->putText(area->y, x, "[^]", 3, pair, true, false);
    R->putText(area->y + area->h - 1, x, "[v]", 3, pair, true, false);
  };

  auto apply_logs_pending_actions = [&]() {
    if (state.logs.pending_scroll_y == 0 &&
        state.logs.pending_scroll_x == 0 &&
        !state.logs.pending_jump_home &&
        !state.logs.pending_jump_end) {
      return;
    }
    if (state.screen != ScreenMode::Logs) {
      state.logs.pending_scroll_y = 0;
      state.logs.pending_scroll_x = 0;
      state.logs.pending_jump_home = false;
      state.logs.pending_jump_end = false;
      return;
    }

    auto tb = as<cuwacunu::iinuji::textBox_data_t>(left);
    if (state.logs.pending_jump_home) {
      if (tb) {
        tb->scroll_y = 0;
        tb->scroll_x = 0;
      }
      state.logs.auto_follow = false;
    } else if (state.logs.pending_jump_end) {
      if (tb) tb->scroll_y = std::numeric_limits<int>::max();
      state.logs.auto_follow = true;
    }
    if (state.logs.pending_scroll_y != 0 || state.logs.pending_scroll_x != 0) {
      if (state.logs.pending_scroll_y != 0) state.logs.auto_follow = false;
      scroll_text_box(left, state.logs.pending_scroll_y, state.logs.pending_scroll_x);
      scroll_text_box(right, state.logs.pending_scroll_y, state.logs.pending_scroll_x);
    }
    state.logs.pending_scroll_y = 0;
    state.logs.pending_scroll_x = 0;
    state.logs.pending_jump_home = false;
    state.logs.pending_jump_end = false;
    dirty = true;
  };

  auto handle_help_overlay_key = [&](int ch) {
    if (!state.help_view) return false;
    switch (ch) {
      case KEY_UP:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollUp);
        return true;
      case KEY_DOWN:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollDown);
        return true;
      case KEY_LEFT:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollLeft);
        return true;
      case KEY_RIGHT:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollRight);
        return true;
      case KEY_PPAGE:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollPageUp);
        return true;
      case KEY_NPAGE:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollPageDown);
        return true;
      case KEY_HOME:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollHome);
        return true;
      case KEY_END:
        dispatch_canonical_internal_call(canonical_paths::kHelpScrollEnd);
        return true;
      default:
        return false;
    }
  };

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
        if (state.logs.auto_follow) jump_logs_to_bottom();
        dirty = true;
      }
    }
    apply_logs_pending_actions();

    if (dirty) {
      refresh_ui(state, title, status, left, right, cmdline);
      if (state.screen == ScreenMode::Logs && state.logs.auto_follow) {
        jump_logs_to_bottom();
      }
      layout_tree(root, Rect{0, 0, W, H});
      ::erase();
      render_tree(root);
      render_data_plot_overlay(state, data_rt, left, right);
      render_help_overlay();
      render_logs_scroll_controls();
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
      if (!state.logs.mouse_capture) continue;
      MEVENT me{};
      if (getmouse(&me) != OK) continue;

      const bool left_click = (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED |
                                            BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED)) != 0;
      if (left_click) {
        if (help_overlay_close_hit(me.x, me.y)) {
          dispatch_canonical_internal_call(canonical_paths::kHelpClose);
          log_info("[iinuji_cmd] help overlay closed (mouse)\n");
          dirty = true;
          continue;
        }
        if (data_plot_overlay_close_hit(state, left, right, me.x, me.y)) {
          dispatch_canonical_internal_call(canonical_paths::kDataPlotOff);
          log_info("[iinuji_cmd] data plot overlay closed (mouse)\n");
          dirty = true;
          continue;
        }
      }
      const bool mod_shift = (me.bstate & BUTTON_SHIFT) != 0;
      const bool mod_ctrl = (me.bstate & BUTTON_CTRL) != 0;
      const bool mod_alt = (me.bstate & BUTTON_ALT) != 0;
      bool horizontal_mod = (mod_shift || mod_ctrl || mod_alt);

      int dy = 0;
      int dx = 0;
      bool wheel = false;

      if (state.help_view) {
        if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED | BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
          wheel = true;
          if (horizontal_mod) dx -= kHScrollStep;
          else dy -= kVScrollStep;
        }
        if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED | BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
          wheel = true;
          if (horizontal_mod) dx += kHScrollStep;
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
          scroll_help_overlay(dy, dx);
          dirty = true;
        }
        continue;
      }

      if (left_click) {
        if (logs_jump_top_hit(me.x, me.y)) {
          dispatch_canonical_internal_call(canonical_paths::kLogsScrollHome);
          dirty = true;
          continue;
        }
        if (logs_jump_bottom_hit(me.x, me.y)) {
          dispatch_canonical_internal_call(canonical_paths::kLogsScrollEnd);
          dirty = true;
          continue;
        }
      }

      const auto l_caps = panel_scroll_caps(left);
      const auto r_caps = panel_scroll_caps(right);
      const bool any_v = l_caps.v || r_caps.v;
      const bool any_h = l_caps.h || r_caps.h;

      bool horizontal_auto = horizontal_mod || (!any_v && any_h);
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

    if (ch == 27 && state.help_view) {
      dispatch_canonical_internal_call(canonical_paths::kHelpClose);
      log_info("[iinuji_cmd] help overlay closed (esc)\n");
      dirty = true;
      continue;
    }
    if (handle_help_overlay_key(ch)) {
      dirty = true;
      continue;
    }
    const bool is_cmdline_key =
        (ch == '\n' || ch == '\r' || ch == KEY_ENTER) ||
        (ch == 21) ||  // Ctrl+U
        (ch == KEY_BACKSPACE || ch == 127 || ch == 8) ||
        (ch >= 32 && ch <= 126);
    if (state.help_view && !is_cmdline_key) continue;

    if (ch == KEY_F(1)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenHome);
      log_info("[iinuji_cmd] screen=home\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(2)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenBoard);
      log_info("[iinuji_cmd] screen=board\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(4)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenTsi);
      log_info("[iinuji_cmd] screen=tsi\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(5)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenData);
      log_info("[iinuji_cmd] screen=data\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(8)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenLogs);
      log_info("[iinuji_cmd] screen=logs\n");
      dirty = true;
      continue;
    }
    if (ch == KEY_F(9)) {
      dispatch_canonical_internal_call(canonical_paths::kScreenConfig);
      log_info("[iinuji_cmd] screen=config\n");
      dirty = true;
      continue;
    }

    if (state.screen == ScreenMode::Board && ch == KEY_DOWN) {
      dispatch_canonical_internal_call(canonical_paths::kBoardSelectNext);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Board && ch == KEY_UP) {
      dispatch_canonical_internal_call(canonical_paths::kBoardSelectPrev);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Tsiemene && ch == KEY_DOWN) {
      dispatch_canonical_internal_call(canonical_paths::kTsiTabNext);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Tsiemene && ch == KEY_UP) {
      dispatch_canonical_internal_call(canonical_paths::kTsiTabPrev);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Config && ch == KEY_DOWN) {
      dispatch_canonical_internal_call(canonical_paths::kConfigTabNext);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Config && ch == KEY_UP) {
      dispatch_canonical_internal_call(canonical_paths::kConfigTabPrev);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && ch == KEY_UP) {
      dispatch_canonical_internal_call(canonical_paths::kLogsSettingsSelectPrev);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && ch == KEY_DOWN) {
      dispatch_canonical_internal_call(canonical_paths::kLogsSettingsSelectNext);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
      const bool forward = (ch == KEY_RIGHT);
      if (dispatch_logs_setting_adjust(forward)) {
        set_mouse_capture(state.logs.mouse_capture);
        dirty = true;
        continue;
      }
    }

    if (state.screen == ScreenMode::Logs && ch == KEY_HOME) {
      dispatch_canonical_internal_call(canonical_paths::kLogsScrollHome);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && ch == KEY_END) {
      dispatch_canonical_internal_call(canonical_paths::kLogsScrollEnd);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && ch == KEY_PPAGE) {
      dispatch_canonical_internal_call(canonical_paths::kLogsScrollPageUp);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Logs && ch == KEY_NPAGE) {
      dispatch_canonical_internal_call(canonical_paths::kLogsScrollPageDown);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == 27 && state.data.plot_view) {
      dispatch_canonical_internal_call(canonical_paths::kDataPlotOff);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == KEY_UP) {
      dispatch_canonical_internal_call(canonical_paths::kDataFocusPrev);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == KEY_DOWN) {
      dispatch_canonical_internal_call(canonical_paths::kDataFocusNext);
      dirty = true;
      continue;
    }
    if (state.screen == ScreenMode::Data && state.cmdline.empty() && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
      const bool forward = (ch == KEY_RIGHT);
      switch (state.data.nav_focus) {
        case DataNavFocus::Channel:
          dispatch_canonical_internal_call(forward ? canonical_paths::kDataChNext : canonical_paths::kDataChPrev);
          break;
        case DataNavFocus::Sample:
          dispatch_canonical_internal_call(forward ? canonical_paths::kDataSampleNext : canonical_paths::kDataSamplePrev);
          break;
        case DataNavFocus::Dim:
          dispatch_canonical_internal_call(forward ? canonical_paths::kDataDimNext : canonical_paths::kDataDimPrev);
          break;
        case DataNavFocus::PlotMode: {
          const auto next_mode = forward
              ? next_data_plot_mode(state.data.plot_mode)
              : prev_data_plot_mode(state.data.plot_mode);
          switch (next_mode) {
            case DataPlotMode::SeqLength:
              dispatch_canonical_internal_call(canonical_paths::kDataPlotModeSeq);
              break;
            case DataPlotMode::FutureSeqLength:
              dispatch_canonical_internal_call(canonical_paths::kDataPlotModeFuture);
              break;
            case DataPlotMode::ChannelWeight:
              dispatch_canonical_internal_call(canonical_paths::kDataPlotModeWeight);
              break;
            case DataPlotMode::NormWindow:
              dispatch_canonical_internal_call(canonical_paths::kDataPlotModeNorm);
              break;
            case DataPlotMode::FileBytes:
              dispatch_canonical_internal_call(canonical_paths::kDataPlotModeBytes);
              break;
          }
        } break;
        case DataNavFocus::XAxis:
          dispatch_canonical_internal_call(canonical_paths::kDataAxisToggle);
          break;
        case DataNavFocus::Mask:
          dispatch_canonical_internal_call(forward ? canonical_paths::kDataMaskOn : canonical_paths::kDataMaskOff);
          break;
      }
      dirty = true;
      continue;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      const std::string cmd = state.cmdline;
      state.cmdline.clear();
      if (state.help_view && !cmd.empty()) state.help_view = false;
      run_command(state, cmd, nullptr);
      set_mouse_capture(state.logs.mouse_capture);
      IinujiStateFlow{state}.normalize_after_command();
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
