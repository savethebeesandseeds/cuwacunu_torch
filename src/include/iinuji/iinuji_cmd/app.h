#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ncursesw/ncurses.h>
#ifdef timeout
#undef timeout
#endif

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iitepi/config_space_t.h"
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

inline int run(const char* global_config_path = DEFAULT_GLOBAL_CONFIG_PATH) try {
  struct DlogTerminalOutputGuard {
    bool prev{true};
    ~DlogTerminalOutputGuard() {
      cuwacunu::piaabo::dlog_set_terminal_output_enabled(prev);
    }
  };
  DlogTerminalOutputGuard dlog_guard{cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  log_info("[iinuji_cmd] boot global_config_path=%s\n", global_config_path);
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
  cuwacunu::iitepi::config_space_t::update_config();
  const int logs_cap_cfg =
      std::max(1, cuwacunu::iitepi::config_space_t::get<int>(
                      "GUI",
                      "iinuji_logs_buffer_capacity"));
  const bool logs_show_date_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_show_date",
          std::optional<bool>{true});
  const bool logs_show_thread_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_show_thread",
          std::optional<bool>{true});
  const bool logs_show_metadata_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_show_metadata",
          std::optional<bool>{true});
  const bool logs_show_color_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_show_color",
          std::optional<bool>{true});
  const bool logs_auto_follow_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_auto_follow",
          std::optional<bool>{true});
  const bool logs_mouse_capture_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI",
          "iinuji_logs_mouse_capture",
          std::optional<bool>{true});
  const std::string logs_metadata_filter_cfg =
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "GUI",
          "iinuji_logs_metadata_filter",
          std::optional<std::string>{"any"});
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
  state.logs.show_date = logs_show_date_cfg;
  state.logs.show_thread = logs_show_thread_cfg;
  state.logs.show_metadata = logs_show_metadata_cfg;
  state.logs.show_color = logs_show_color_cfg;
  state.logs.auto_follow = logs_auto_follow_cfg;
  state.logs.mouse_capture = logs_mouse_capture_cfg;
  {
    const std::string token = to_lower_copy(trim_copy(logs_metadata_filter_cfg));
    if (token == "any") {
      state.logs.metadata_filter = LogsMetadataFilter::Any;
    } else if (token == "meta+" || token == "meta" || token == "any_meta" ||
               token == "with_any_metadata") {
      state.logs.metadata_filter = LogsMetadataFilter::WithAnyMetadata;
    } else if (token == "fn+" || token == "function" ||
               token == "with_function") {
      state.logs.metadata_filter = LogsMetadataFilter::WithFunction;
    } else if (token == "path+" || token == "path" ||
               token == "with_path") {
      state.logs.metadata_filter = LogsMetadataFilter::WithPath;
    } else if (token == "callsite+" || token == "callsite" ||
               token == "with_callsite") {
      state.logs.metadata_filter = LogsMetadataFilter::WithCallsite;
    } else {
      state.logs.metadata_filter = LogsMetadataFilter::Any;
      log_warn(
          "[iinuji_cmd] invalid GUI.iinuji_logs_metadata_filter=%s (using any)\n",
          logs_metadata_filter_cfg.c_str());
    }
  }
#if !DLOGS_ENABLE_METADATA
  state.logs.show_metadata = false;
  state.logs.metadata_filter = LogsMetadataFilter::Any;
#endif

  auto set_mouse_capture = [&](bool enabled) {
    mousemask(enabled ? ALL_MOUSE_EVENTS : 0, nullptr);
    mouseinterval(0);
    state.logs.mouse_capture = enabled;
  };

  enum class BootStage : std::uint8_t {
    ResolveContract = 0,
    LoadConfig = 1,
    LoadBoard = 2,
    LoadData = 3,
    Ready = 4,
  };

  BootStage boot_stage{BootStage::ResolveContract};
  bool boot_stage_needs_paint = true;
  std::string boot_contract_hash{};

  auto boot_phase_label = [&]() -> std::string {
    switch (boot_stage) {
      case BootStage::ResolveContract:
        return "board contract";
      case BootStage::LoadConfig:
        return "config tabs";
      case BootStage::LoadBoard:
        return "board state";
      case BootStage::LoadData:
        return "data view metadata";
      case BootStage::Ready:
        return "complete";
    }
    return "runtime state";
  };

  auto boot_marker = [&](BootStage step) -> const char* {
    if (static_cast<int>(step) < static_cast<int>(boot_stage)) return "[done] ";
    if (step == boot_stage) return "[work] ";
    return "[todo] ";
  };

  auto render_boot_ui = [&]() {
    set_text_box(title, "cuwacunu.cmd - boot", true);
    set_text_box(status, "loading " + boot_phase_label(), true);
    set_text_box(
        left,
        std::string("Initializing command terminal...\n\n") +
            boot_marker(BootStage::ResolveContract) + "resolving board contract\n" +
            boot_marker(BootStage::LoadConfig) + "loading config tabs\n" +
            boot_marker(BootStage::LoadBoard) + "loading board state\n" +
            boot_marker(BootStage::LoadData) + "loading data view metadata\n\n"
            "The Home screen will appear when bootstrap completes.",
        true);
    set_text_box(
        right,
        std::string("bootstrap\n") +
            "TERM-bound ncurses session is active.\n\n"
            "current step: " + boot_phase_label() + "\n"
            "using_dev_tty=" + (app.using_dev_tty() ? std::string("true")
                                                     : std::string("false")),
        true);
    set_text_box(bottom, "", false);
    set_text_box(cmdline, "boot> loading...", false);
    cmdline->focused = false;
    right->focused = false;
  };

  auto finish_boot = [&]() {
    clamp_selected_training_tab(state);
    clamp_selected_training_hash(state);
    clamp_selected_tsi_tab(state);
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
      log_info("[iinuji_cmd] board loaded: circuits=%zu\n", state.board.board.circuits.size());
    }
    if (!state.data.ok) {
      log_warn("[iinuji_cmd] data view invalid: %s\n", state.data.error.c_str());
    } else {
      log_info("[iinuji_cmd] data view loaded: channels=%zu\n", state.data.channels.size());
    }
    log_info("[iinuji_cmd] mouse wheel=vertical scroll | Shift/Ctrl/Alt+wheel=horizontal scroll (active screen panels)\n");
  };

  constexpr int kVScrollStep = 6;
  constexpr int kHScrollStep = 16;

  DataAppRuntime data_rt{};

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
    const bool boot_active = (boot_stage != BootStage::Ready);

    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H != last_h || W != last_w) {
      last_h = H;
      last_w = W;
      dirty = true;
    }

    if (boot_active && boot_stage_needs_paint) {
      dirty = true;
    }

    if (dirty) {
      if (boot_active) {
        render_boot_ui();
      } else {
        refresh_board_editor_diagnostic(state);
        refresh_ui(state, title, status, left, right, bottom, cmdline);
        if (state.screen == ScreenMode::Logs && state.logs.auto_follow) {
          jump_logs_to_bottom(state, left);
        }
      }
      layout_tree(root, Rect{0, 0, W, H});
      ::erase();
      render_tree(root);
      if (!boot_active) {
        render_data_plot_overlay(state, data_rt, left, right);
        render_help_overlay(state, left, right);
        render_logs_scroll_controls(state, left);
        render_board_error_line_overlay(state, left);
        render_board_completion_overlay(state, left);
      }
      ::refresh();
      dirty = false;
    }

    if (boot_active) {
      if (boot_stage_needs_paint) {
        boot_stage_needs_paint = false;
        continue;
      }

      switch (boot_stage) {
        case BootStage::ResolveContract:
          boot_contract_hash = resolve_configured_board_contract_hash();
          boot_stage = BootStage::LoadConfig;
          break;
        case BootStage::LoadConfig:
          state.config = load_config_view_from_config(boot_contract_hash);
          clamp_selected_tab(state);
          boot_stage = BootStage::LoadBoard;
          break;
        case BootStage::LoadBoard:
          state.board = load_board_from_contract_hash(boot_contract_hash);
          clamp_board_navigation_state(state);
          boot_stage = BootStage::LoadData;
          break;
        case BootStage::LoadData:
          state.data = load_data_view_from_config(&state.board);
          clamp_selected_data_channel(state);
          clamp_data_plot_mode(state);
          clamp_data_plot_x_axis(state);
          clamp_data_nav_focus(state);
          finish_boot();
          boot_stage = BootStage::Ready;
          break;
        case BootStage::Ready:
          break;
      }

      dirty = true;
      boot_stage_needs_paint = (boot_stage != BootStage::Ready);
      continue;
    }

    if (data_runtime_needed(state)) {
      init_data_runtime(state, data_rt, false);
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
