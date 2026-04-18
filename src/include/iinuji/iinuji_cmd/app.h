#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <ncursesw/ncurses.h>
#ifdef timeout
#undef timeout
#endif

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "iinuji/primitives/animation.h"
#include "iinuji/primitives/art_text.h"
#include "iinuji/primitives/image.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"

#include "iinuji/iinuji_cmd/app/keymap.h"
#include "iinuji/iinuji_cmd/app/overlays.h"
#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/iinuji_cmd/views.h"
#include "iinuji/iinuji_cmd/views/logs/app.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::uint64_t monotonic_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline constexpr int kAnimatedHomeFrameStepMs = 33;
inline constexpr int kFarewellHoldMs = 1100;

inline int desired_input_timeout_for_screen(ScreenMode screen,
                                            bool animate_home = false) {
  if (screen == ScreenMode::Home)
    return animate_home ? kAnimatedHomeFrameStepMs : 250;
  return (screen == ScreenMode::ShellLogs) ? 50 : 250;
}

inline std::uint64_t
idle_refresh_period_ms_for_screen(ScreenMode screen,
                                  bool animate_home = false) {
  switch (screen) {
  case ScreenMode::Home:
    return animate_home ? static_cast<std::uint64_t>(kAnimatedHomeFrameStepMs)
                        : 5000u;
  case ScreenMode::Workbench:
    return 1000;
  case ScreenMode::Runtime:
    return 2000;
  case ScreenMode::Lattice:
    return 1200;
  case ScreenMode::ShellLogs:
    return 250;
  case ScreenMode::Config:
    return 2000;
  }
  return 5000;
}

inline bool refresh_visible_screen_on_idle(CmdState &state,
                                           bool animate_home = false) {
  switch (state.screen) {
  case ScreenMode::Home:
    return animate_home;
  case ScreenMode::Workbench: {
    const std::string previous_status = state.workbench.status;
    const bool previous_status_is_error = state.workbench.status_is_error;
    const std::uint64_t previous_status_expires_at_ms =
        state.workbench.status_expires_at_ms;
    (void)queue_workbench_refresh(state);
    if (!previous_status.empty() && !previous_status_is_error &&
        (previous_status_expires_at_ms == 0 ||
         workbench_status_now_ms() < previous_status_expires_at_ms)) {
      state.workbench.status = previous_status;
      state.workbench.status_is_error = false;
      state.workbench.status_expires_at_ms = previous_status_expires_at_ms;
      state.workbench.error.clear();
    }
    return false;
  }
  case ScreenMode::Runtime: {
    const std::string previous_status = state.runtime.status;
    const bool previous_status_is_error = state.runtime.status_is_error;
    (void)queue_runtime_refresh(state);
    if (!previous_status.empty() && !previous_status_is_error) {
      state.runtime.status = previous_status;
      state.runtime.status_is_error = false;
      state.runtime.error.clear();
    }
    return false;
  }
  case ScreenMode::Lattice:
    return false;
  case ScreenMode::Config:
    return false;
  case ScreenMode::ShellLogs:
    return false;
  }
  return false;
}

inline int
run(const char *global_config_path = DEFAULT_GLOBAL_CONFIG_PATH) try {
  struct DlogTerminalOutputGuard {
    bool prev{true};
    ~DlogTerminalOutputGuard() {
      cuwacunu::piaabo::dlog_set_terminal_output_enabled(prev);
    }
  };
  DlogTerminalOutputGuard dlog_guard{
      cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  log_dbg("[iinuji_cmd] boot global_config_path=%s\n", global_config_path);
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
  cuwacunu::iitepi::config_space_t::update_config();

  const int logs_cap_cfg =
      std::max(1, cuwacunu::iitepi::config_space_t::get<int>(
                      "GUI", "iinuji_logs_buffer_capacity"));
  const bool logs_show_date_cfg = cuwacunu::iitepi::config_space_t::get<bool>(
      "GUI", "iinuji_logs_show_date", std::optional<bool>{false});
  const bool logs_show_thread_cfg = cuwacunu::iitepi::config_space_t::get<bool>(
      "GUI", "iinuji_logs_show_thread", std::optional<bool>{false});
  const bool logs_show_metadata_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI", "iinuji_logs_show_metadata", std::optional<bool>{true});
  const bool logs_show_color_cfg = cuwacunu::iitepi::config_space_t::get<bool>(
      "GUI", "iinuji_logs_show_color", std::optional<bool>{true});
  const bool logs_auto_follow_cfg = cuwacunu::iitepi::config_space_t::get<bool>(
      "GUI", "iinuji_logs_auto_follow", std::optional<bool>{true});
  const bool logs_mouse_capture_cfg =
      cuwacunu::iitepi::config_space_t::get<bool>(
          "GUI", "iinuji_logs_mouse_capture", std::optional<bool>{true});
  const std::string logs_metadata_filter_cfg =
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "GUI", "iinuji_logs_metadata_filter",
          std::optional<std::string>{"any"});
  cuwacunu::piaabo::dlog_set_buffer_capacity(
      static_cast<std::size_t>(logs_cap_cfg));

  cuwacunu::iinuji::NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms =
      desired_input_timeout_for_screen(ScreenMode::Home);
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

  const std::string loading_logo_path_cfg =
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "GUI", "iinuji_loading_logo_path",
          std::optional<std::string>{
              "/cuwacunu/src/resources/waajacamaya.png"});
  const std::string closing_logo_path_cfg =
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "GUI", "iinuji_closing_logo_path",
          std::optional<std::string>{loading_logo_path_cfg});
  const std::string home_animation_path_cfg =
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "GUI", "iinuji_home_animation_path",
          std::optional<std::string>{
              "/cuwacunu/src/resources/waajacamaya.apng"});

  const home_showcase_assets_t home_showcase_assets =
      load_home_showcase_assets(loading_logo_path_cfg, home_animation_path_cfg);
  const rgba_image_t &boot_logo_image = home_showcase_assets.loading_logo_image;
  const std::string &boot_logo_error = home_showcase_assets.loading_logo_error;
  const bool boot_logo_ok = home_showcase_assets.loading_logo_ok;
  if (!boot_logo_ok) {
    log_warn("[iinuji_cmd] loading logo unavailable: %s (%s)\n",
             loading_logo_path_cfg.c_str(),
             boot_logo_error.empty() ? "decode failed"
                                     : boot_logo_error.c_str());
  }

  const bool closing_logo_matches_loading =
      closing_logo_path_cfg == loading_logo_path_cfg;
  rgba_image_t closing_logo_image{};
  std::string closing_logo_error{};
  const bool closing_logo_ok =
      closing_logo_matches_loading
          ? boot_logo_ok
          : decode_png_rgba_file(closing_logo_path_cfg, closing_logo_image,
                                 closing_logo_error);
  if (!closing_logo_ok) {
    log_warn("[iinuji_cmd] closing logo unavailable: %s (%s)%s\n",
             closing_logo_path_cfg.c_str(),
             closing_logo_error.empty() ? "decode failed"
                                        : closing_logo_error.c_str(),
             boot_logo_ok ? " | using loading logo fallback" : "");
  }
  const rgba_image_t &farewell_logo_image =
      (closing_logo_ok && !closing_logo_matches_loading) ? closing_logo_image
                                                         : boot_logo_image;
  const bool farewell_logo_ok = closing_logo_ok || boot_logo_ok;
  const bool farewell_logo_uses_loading_fallback =
      !closing_logo_ok && boot_logo_ok;

  const rgba_animation_t &home_animation = home_showcase_assets.home_animation;
  const std::string &home_animation_error =
      home_showcase_assets.home_animation_error;
  const bool home_animation_ok = home_showcase_assets.home_animation_ok;
  if (!home_animation_ok) {
    log_warn("[iinuji_cmd] home animation unavailable: %s (%s)\n",
             home_animation_path_cfg.c_str(),
             home_animation_error.empty() ? "decode failed"
                                          : home_animation_error.c_str());
  }
  const bool home_animation_is_dynamic =
      home_showcase_assets.home_animation_is_dynamic;
  std::uint64_t home_animation_origin_ms = 0u;
  const image_grayscale_options_t boot_logo_render_opt =
      make_home_showcase_render_options();
  const art_text_options_t home_wordmark_text_opt =
      make_home_showcase_wordmark_text_options();

  auto root = create_grid_container(
      "root",
      {len_spec::px(3), len_spec::px(0), len_spec::frac(1.0), len_spec::px(3),
       len_spec::px(3)},
      {len_spec::frac(1.0)}, 0, 0,
      iinuji_layout_t{layout_mode_t::Normalized, 0, 0, 1, 1, true},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});

  auto title =
      create_text_box("title", "", false, text_align_t::Left, iinuji_layout_t{},
                      iinuji_style_t{"#EDEDED", "#202028", true, "#6C6C75",
                                     true, false, " cuwacunu.cmd "});
  place_in_grid(title, 0, 0);
  root->add_child(title);

  auto status = create_text_box(
      "status", "", false, text_align_t::Left, iinuji_layout_t{},
      iinuji_style_t{"#B8B8BF", "#101014", false, "#101014"});
  status->visible = false;
  place_in_grid(status, 1, 0);
  root->add_child(status);

  auto workspace = create_grid_container(
      "workspace", {len_spec::frac(1.0)},
      {len_spec::frac(0.44), len_spec::frac(0.56)}, 1, 1, iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});
  place_in_grid(workspace, 2, 0);
  root->add_child(workspace);

  auto left = create_grid_container(
      "left", {len_spec::frac(0.34), len_spec::frac(0.66)},
      {len_spec::frac(1.0)}, 1, 0, iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#101014"});
  place_in_grid(left, 0, 0);
  workspace->add_child(left);

  auto left_main =
      create_panel("left_main", iinuji_layout_t{},
                   iinuji_style_t{"#D0D0D0", "#101014", true, "#5E5E68", false,
                                  false, " view "});
  place_in_grid(left_main, 0, 0, 2, 1);
  left->add_child(left_main);

  auto left_nav_panel =
      create_panel("left_nav_panel", iinuji_layout_t{},
                   iinuji_style_t{"#D0D0D0", "#101014", true, "#5E5E68", false,
                                  false, " navigation "});
  left_nav_panel->visible = false;
  place_in_grid(left_nav_panel, 0, 0);
  left->add_child(left_nav_panel);

  auto left_worklist_panel =
      create_panel("left_worklist_panel", iinuji_layout_t{},
                   iinuji_style_t{"#D0D0D0", "#101014", true, "#5E5E68", false,
                                  false, " worklist "});
  left_worklist_panel->visible = false;
  place_in_grid(left_worklist_panel, 1, 0);
  left->add_child(left_worklist_panel);

  auto right = create_grid_container(
      "right", {len_spec::frac(1.0), len_spec::px(0)}, {len_spec::frac(1.0)}, 1,
      0, iinuji_layout_t{},
      iinuji_style_t{"#C8C8CE", "#101014", false, "#101014"});
  place_in_grid(right, 0, 1);
  workspace->add_child(right);

  auto right_main =
      create_panel("right_main", iinuji_layout_t{},
                   iinuji_style_t{"#C8C8CE", "#101014", true, "#5E5E68", false,
                                  false, " context "});
  place_in_grid(right_main, 0, 0);
  right->add_child(right_main);

  auto right_aux =
      create_panel("right_aux", iinuji_layout_t{},
                   iinuji_style_t{"#C8C8CE", "#101014", true, "#5E5E68", false,
                                  false, " context "});
  right_aux->visible = false;
  place_in_grid(right_aux, 1, 0);
  right->add_child(right_aux);

  auto bottom = create_text_box(
      "bottom", "", false, text_align_t::Left, iinuji_layout_t{},
      iinuji_style_t{"#B3A99B", "#1E1B18", true, "#4E473E", false, false,
                     " status "});
  place_in_grid(bottom, 3, 0);
  root->add_child(bottom);

  auto cmdline = create_text_box(
      "cmdline", "cmd> ", false, text_align_t::Left, iinuji_layout_t{},
      iinuji_style_t{"#E8E8E8", "#101014", true, "#5E5E68", false, false,
                     " command "});
  cmdline->focusable = true;
  cmdline->focused = true;
  place_in_grid(cmdline, 4, 0);
  root->add_child(cmdline);

  CmdState state{};
  state.lattice.global_config_path = global_config_path;
  state.shell_logs.show_date = logs_show_date_cfg;
  state.shell_logs.show_thread = logs_show_thread_cfg;
  state.shell_logs.show_metadata = logs_show_metadata_cfg;
  state.shell_logs.show_color = logs_show_color_cfg;
  state.shell_logs.auto_follow = logs_auto_follow_cfg;
  state.shell_logs.mouse_capture = logs_mouse_capture_cfg;
  {
    const std::string token =
        to_lower_copy(trim_copy(logs_metadata_filter_cfg));
    if (token == "any") {
      state.shell_logs.metadata_filter = LogsMetadataFilter::Any;
    } else if (token == "meta+" || token == "meta" || token == "any_meta" ||
               token == "with_any_metadata") {
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithAnyMetadata;
    } else if (token == "fn+" || token == "function" ||
               token == "with_function") {
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithFunction;
    } else if (token == "path+" || token == "path" || token == "with_path") {
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithPath;
    } else if (token == "callsite+" || token == "callsite" ||
               token == "with_callsite") {
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithCallsite;
    } else {
      state.shell_logs.metadata_filter = LogsMetadataFilter::Any;
      log_warn("[iinuji_cmd] invalid GUI.iinuji_logs_metadata_filter=%s (using "
               "any)\n",
               logs_metadata_filter_cfg.c_str());
    }
  }
#if !DLOGS_ENABLE_METADATA
  state.shell_logs.show_metadata = false;
  state.shell_logs.metadata_filter = LogsMetadataFilter::Any;
#endif

  auto set_mouse_capture = [&](bool enabled) {
    mousemask(enabled ? ALL_MOUSE_EVENTS : 0, nullptr);
    mouseinterval(0);
    state.shell_logs.mouse_capture = enabled;
  };

  auto boot_pair = [&](const std::string &fg,
                       const std::string &bg = "#101014") -> short {
    return static_cast<short>(get_color_pair(fg, bg));
  };

  enum class BootStage : std::uint8_t {
    LoadHumanDefaults = 0,
    LoadMarshalDefaults = 1,
    LoadRuntimeDefaults = 2,
    LoadHumanWorkbench = 3,
    LoadConfig = 4,
    LoadRuntime = 5,
    Ready = 6,
  };

  struct BootStageDescriptor {
    BootStage stage;
    const char *label;
    const char *detail;
  };

  static constexpr std::array<BootStageDescriptor, 6> kBootStages{{
      {BootStage::LoadHumanDefaults, "Human Hero defaults",
       "resolve Human Hero DSL and load operator defaults"},
      {BootStage::LoadMarshalDefaults, "Marshal Hero defaults",
       "resolve Marshal Hero DSL and load marshal session defaults"},
      {BootStage::LoadRuntimeDefaults, "Runtime Hero defaults",
       "resolve Runtime Hero DSL and load runtime defaults"},
      {BootStage::LoadHumanWorkbench, "Operator workbench",
       "collect operator sessions, requests, and review summaries"},
      {BootStage::LoadConfig, "Config catalog",
       "load config policy and catalog visible config files"},
      {BootStage::LoadRuntime, "Runtime inventory",
       "collect runtime sessions, campaigns, jobs, and device state"},
  }};
  static constexpr std::size_t kBootStageCount = kBootStages.size();
  static constexpr std::uint64_t kBootStageEstimateMinMs = 120u;
  static constexpr std::uint64_t kBootStageEstimateMaxMs = 3200u;
  static constexpr std::array<std::uint64_t, kBootStageCount>
      kDefaultBootStageEstimateMs{{
          420u,
          360u,
          360u,
          300u,
          300u,
          460u,
      }};
  const std::string boot_profile_path =
      "/tmp/cuwacunu_iinuji_cmd_boot_profile.txt";

  BootStage boot_stage{BootStage::LoadHumanDefaults};
  bool boot_stage_needs_paint = true;
  const std::uint64_t boot_visual_start_ms = monotonic_now_ms();
  std::uint64_t boot_stage_started_ms = boot_visual_start_ms;
  std::uint64_t boot_visual_hold_until_ms = 0u;

  auto clamp_boot_stage_estimate_ms = [&](std::uint64_t value) {
    return std::clamp(value, kBootStageEstimateMinMs, kBootStageEstimateMaxMs);
  };

  auto load_boot_stage_estimate_ms =
      [&]() -> std::array<std::uint64_t, kBootStageCount> {
    auto estimates = kDefaultBootStageEstimateMs;
    std::ifstream in(boot_profile_path);
    if (!in) {
      return estimates;
    }
    for (std::size_t i = 0; i < estimates.size(); ++i) {
      std::uint64_t value = 0u;
      if (!(in >> value)) {
        return estimates;
      }
      estimates[i] = std::max(kDefaultBootStageEstimateMs[i],
                              clamp_boot_stage_estimate_ms(value));
    }
    return estimates;
  };

  auto write_boot_stage_estimate_ms =
      [&](const std::array<std::uint64_t, kBootStageCount> &estimates) {
        std::ofstream out(boot_profile_path, std::ios::trunc);
        if (!out) {
          return;
        }
        for (std::size_t i = 0; i < estimates.size(); ++i) {
          out << std::max(kDefaultBootStageEstimateMs[i],
                          clamp_boot_stage_estimate_ms(estimates[i]))
              << '\n';
        }
      };

  std::array<std::uint64_t, kBootStageCount> boot_stage_estimate_ms =
      load_boot_stage_estimate_ms();
  std::array<std::uint64_t, kBootStageCount> boot_stage_actual_ms{};

  auto boot_profile_index = [&](BootStage stage) -> std::size_t {
    return std::min<std::size_t>(static_cast<std::size_t>(stage),
                                 kBootStageCount - 1u);
  };

  auto boot_estimated_total_ms = [&]() -> std::uint64_t {
    std::uint64_t total = 0u;
    for (const auto estimate_ms : boot_stage_estimate_ms) {
      total += clamp_boot_stage_estimate_ms(estimate_ms);
    }
    return std::max<std::uint64_t>(1u, total);
  };

  auto persist_boot_stage_estimate_ms = [&]() {
    std::array<std::uint64_t, kBootStageCount> merged = boot_stage_estimate_ms;
    for (std::size_t i = 0; i < merged.size(); ++i) {
      const std::uint64_t base = clamp_boot_stage_estimate_ms(
          merged[i] > 0u ? merged[i] : kDefaultBootStageEstimateMs[i]);
      const std::uint64_t sample = clamp_boot_stage_estimate_ms(
          boot_stage_actual_ms[i] > 0u ? boot_stage_actual_ms[i] : base);
      merged[i] =
          std::max(kDefaultBootStageEstimateMs[i],
                   clamp_boot_stage_estimate_ms((base + sample * 2u) / 3u));
    }
    write_boot_stage_estimate_ms(merged);
    boot_stage_estimate_ms = merged;
  };

  auto advance_boot_stage = [&](BootStage completed_stage,
                                BootStage next_stage) -> std::uint64_t {
    const std::uint64_t finished_ms = monotonic_now_ms();
    boot_stage_actual_ms[boot_profile_index(completed_stage)] =
        std::max<std::uint64_t>(1u, finished_ms - boot_stage_started_ms);
    boot_stage_started_ms = finished_ms;
    boot_stage = next_stage;
    return finished_ms;
  };

  struct BootVisualProgress {
    double ratio{0.0};
    bool complete{false};
  };

  auto boot_visual_progress = [&](std::uint64_t now_ms) -> BootVisualProgress {
    BootVisualProgress visual{};
    const std::uint64_t total_ms = boot_estimated_total_ms();
    const std::uint64_t elapsed_ms =
        (now_ms > boot_visual_start_ms) ? (now_ms - boot_visual_start_ms) : 0u;
    const double raw_ratio =
        std::clamp(static_cast<double>(std::min(elapsed_ms, total_ms)) /
                       static_cast<double>(total_ms),
                   0.0, 1.0);

    if (boot_stage != BootStage::Ready) {
      const double completed_floor =
          std::clamp(static_cast<double>(boot_profile_index(boot_stage)) /
                         static_cast<double>(kBootStageCount),
                     0.0, 0.90);
      visual.ratio =
          std::clamp(std::max(raw_ratio, completed_floor), 0.0, 0.97);
      visual.complete = false;
      return visual;
    }

    visual.ratio = raw_ratio;
    visual.complete = (boot_visual_hold_until_ms == 0u ||
                       now_ms >= boot_visual_hold_until_ms);
    if (visual.complete) {
      visual.ratio = 1.0;
    }
    return visual;
  };

  auto boot_actual_phase_label = [&]() -> std::string {
    if (boot_stage == BootStage::Ready) {
      return "complete";
    }
    return to_lower_copy(
        std::string(kBootStages[boot_profile_index(boot_stage)].label));
  };

  auto boot_actual_phase_detail = [&]() -> std::string {
    if (boot_stage == BootStage::Ready) {
      return "startup complete";
    }
    return std::string(kBootStages[boot_profile_index(boot_stage)].detail);
  };

  auto log_boot_stage_begin = [&](BootStage stage) {
    const auto &descriptor = kBootStages[boot_profile_index(stage)];
    log_dbg("[iinuji_cmd.boot] stage=%s begin | %s\n", descriptor.label,
            descriptor.detail);
  };

  auto log_boot_stage_finish = [&](BootStage stage, bool ok,
                                   std::string summary = {}) {
    const auto &descriptor = kBootStages[boot_profile_index(stage)];
    const std::uint64_t now_ms = monotonic_now_ms();
    const auto elapsed_ms = static_cast<unsigned long long>(
        now_ms >= boot_stage_started_ms ? (now_ms - boot_stage_started_ms)
                                        : 0u);
    const std::string suffix =
        summary.empty() ? std::string{} : " | " + std::move(summary);
    if (ok) {
      log_dbg("[iinuji_cmd.boot] stage=%s ok elapsed_ms=%llu%s\n",
              descriptor.label, elapsed_ms, suffix.c_str());
    } else {
      log_err("[iinuji_cmd.boot] stage=%s failed elapsed_ms=%llu%s\n",
              descriptor.label, elapsed_ms, suffix.c_str());
    }
  };

  auto render_boot_ui = [&]() {
    left_main->visible = true;
    left_nav_panel->visible = false;
    left_worklist_panel->visible = false;
    right_main->visible = true;
    right_aux->visible = false;
    title->style.label_color = "#F1FFF4";
    title->style.background_color = "#173122";
    title->style.border_color = "#2D6A44";
    status->style.label_color = "#9ED8AE";
    status->style.background_color = "#101014";
    status->style.border_color = "#101014";
    left_main->style.border_color = "#2D6A44";
    left_main->style.label_color = "#DAF5E0";
    left_main->style.title = " waajacamaya ";
    right_main->style.border_color = "#2D6A44";
    right_main->style.label_color = "#D6EBD9";
    right_main->style.title = " ";
    bottom->style.border_color = "#2D6A44";
    bottom->style.label_color = "#B7D2C0";
    bottom->style.title = " status ";
    cmdline->style.border_color = "#2D6A44";
    cmdline->style.label_color = "#E8F7EC";

    set_text_box(title, "loading " + boot_actual_phase_label(), false);
    set_text_box(status, "", false);
    set_text_box(left_main, "", true);
    set_text_box(right_main, "", true);
    set_text_box(bottom, "waajacu.com | bootstrap in progress", false);
    set_text_box(cmdline, "", false);
    cmdline->focused = false;
    right_main->focused = false;
    right_aux->focused = false;
  };

  auto render_boot_overlay = [&](std::uint64_t now_ms) {
    auto *R = get_renderer();
    if (R == nullptr)
      return;

    const Rect left_area = content_rect(*left_main);
    if (left_area.w <= 0 || left_area.h <= 0) {
      return;
    }

    const BootVisualProgress visual_progress = boot_visual_progress(now_ms);
    const std::size_t actual_stage_index =
        (boot_stage == BootStage::Ready)
            ? kBootStages.size()
            : std::min<std::size_t>(boot_profile_index(boot_stage),
                                    kBootStages.size() - 1u);

    const short hero_green_pair = boot_pair("#4fcf6b");
    const short hero_green_soft_pair = boot_pair("#8fd49f");
    const short hero_muted_pair = boot_pair("#6f7b72");
    const short hero_active_pair = boot_pair("#d6b86a");
    const short hero_note_pair = boot_pair("#b6c7ba");
    const short hero_track_pair = boot_pair("#173122", "#173122");
    const short hero_fill_pair = boot_pair("#4fcf6b", "#4fcf6b");
    const short hero_head_pair = boot_pair("#8ef0a7", "#8ef0a7");
    const short hero_bar_text_fill_pair = boot_pair("#101014", "#4fcf6b");
    const short hero_bar_text_track_pair = boot_pair("#f1fff4", "#173122");
    const int wordmark_rows = std::clamp(left_area.h / 10, 3, 5);
    const Rect stage_area = home_showcase_stage_rect(
        left_area, 0.42, /*top_padding_rows=*/0,
        /*bottom_reserved_rows=*/
        static_cast<int>(kBootStageCount) + wordmark_rows + 6);

    if (boot_logo_ok) {
      render_home_showcase_static(boot_logo_image, stage_area,
                                  boot_logo_render_opt);
      const int wordmark_y = stage_area.y + stage_area.h + 1;
      if (wordmark_y < left_area.y + left_area.h) {
        render_art_text(
            "Cuwacunu", left_area.x, wordmark_y, left_area.w,
            std::min(wordmark_rows, left_area.y + left_area.h - wordmark_y),
            home_wordmark_text_opt, boot_logo_render_opt);
      }
    } else {
      const int fallback_y = stage_area.y + std::max(0, stage_area.h / 2 - 1);
      R->putText(fallback_y, stage_area.x, "waajacamaya logo unavailable",
                 stage_area.w, hero_muted_pair, true, false);
      if (fallback_y + 1 < stage_area.y + stage_area.h) {
        R->putText(fallback_y + 1, stage_area.x, loading_logo_path_cfg,
                   stage_area.w, hero_note_pair, false, false);
      }
    }

    const int pct_value =
        static_cast<int>(std::lround(visual_progress.ratio * 100.0));
    const std::string pct_text = std::to_string(pct_value) + "%";
    const int bar_pad = std::max(0, std::min(2, left_area.w / 10));
    const int bar_y = stage_area.y + stage_area.h + wordmark_rows + 2;
    const int bar_x = left_area.x + bar_pad;
    const int bar_w = std::max(1, left_area.w - 2 * bar_pad);
    if (bar_y < left_area.y + left_area.h) {
      R->fillRect(bar_y, bar_x, 1, bar_w, hero_track_pair);
      const int fill_w =
          std::clamp(static_cast<int>(std::lround(visual_progress.ratio *
                                                  static_cast<double>(bar_w))),
                     0, bar_w);
      if (fill_w > 0) {
        R->fillRect(bar_y, bar_x, 1, fill_w, hero_fill_pair);
        if (fill_w < bar_w) {
          R->fillRect(bar_y, bar_x + fill_w, 1, 1, hero_head_pair);
        }
      }
      const int pct_x =
          bar_x + std::max(0, (bar_w - static_cast<int>(pct_text.size())) / 2);
      for (std::size_t i = 0; i < pct_text.size(); ++i) {
        const int cell_x = pct_x + static_cast<int>(i);
        if (cell_x < bar_x || cell_x >= bar_x + bar_w) {
          continue;
        }
        const bool on_fill = (cell_x - bar_x) < fill_w;
        R->putGlyph(bar_y, cell_x, static_cast<wchar_t>(pct_text[i]),
                    on_fill ? hero_bar_text_fill_pair
                            : hero_bar_text_track_pair);
      }
    }

    int status_y = bar_y + 2;
    if (status_y < left_area.y + left_area.h) {
      const std::string step_line = "loading " + boot_actual_phase_label();
      R->putText(status_y, left_area.x, step_line, left_area.w,
                 hero_green_soft_pair, true, false);
      ++status_y;
    }
    if (status_y < left_area.y + left_area.h) {
      R->putText(status_y, left_area.x, boot_actual_phase_detail(), left_area.w,
                 hero_note_pair, false, false);
      ++status_y;
    }

    for (std::size_t i = 0; i < kBootStages.size(); ++i) {
      if (status_y >= left_area.y + left_area.h)
        break;
      const bool done = visual_progress.complete || i < actual_stage_index;
      const bool active = !visual_progress.complete &&
                          boot_stage != BootStage::Ready &&
                          i == actual_stage_index;
      const char *marker = done ? "[ok]" : (active ? "[..]" : "[  ]");
      const short marker_pair =
          done ? hero_green_pair
               : (active ? hero_active_pair : hero_muted_pair);
      const short text_pair =
          done ? hero_note_pair
               : (active ? hero_green_soft_pair : hero_muted_pair);
      R->putText(status_y, left_area.x, marker, 4, marker_pair, true, false);
      R->putText(status_y, left_area.x + 6, kBootStages[i].label,
                 std::max(0, left_area.w - 6), text_pair, true, false);
      ++status_y;
    }
  };

  auto render_home_overlay = [&](std::uint64_t now_ms) {
    auto *R = get_renderer();
    if (R == nullptr)
      return;

    const Rect left_area = content_rect(*left_main);
    if (left_area.w <= 0 || left_area.h <= 0) {
      return;
    }

    const short hero_green_pair = boot_pair("#4fcf6b");
    const short hero_green_soft_pair = boot_pair("#8fd49f");
    const short hero_note_pair = boot_pair("#b6c7ba");
    const bool use_static_home_fallback = !home_animation_ok && boot_logo_ok;
    const int static_wordmark_rows = std::clamp(left_area.h / 10, 3, 5);

    const Rect stage_area =
        use_static_home_fallback
            ? home_showcase_stage_rect(left_area, 0.48,
                                       /*top_padding_rows=*/0,
                                       /*bottom_reserved_rows=*/
                                       static_wordmark_rows + 8)
            : home_showcase_stage_rect(left_area, 0.56,
                                       /*top_padding_rows=*/0,
                                       /*bottom_reserved_rows=*/1);
    if (home_animation_ok) {
      if (home_animation_origin_ms == 0u) {
        home_animation_origin_ms = now_ms;
      }
      const rgba_image_t &frame = animation_detail::frame_for_elapsed_ms(
          home_animation, now_ms - home_animation_origin_ms);
      const rgba_image_t composite_frame = composite_home_wordmark_over_image(
          frame, home_showcase_assets.home_wordmark_image);
      render_image_grayscale(composite_frame, stage_area.x, stage_area.y,
                             stage_area.w, stage_area.h, boot_logo_render_opt);
    } else if (use_static_home_fallback) {
      render_home_showcase_static(boot_logo_image, stage_area,
                                  boot_logo_render_opt);
      const int wordmark_y = stage_area.y + stage_area.h + 1;
      if (wordmark_y < left_area.y + left_area.h) {
        render_art_text("Cuwacunu", left_area.x, wordmark_y, left_area.w,
                        std::min(static_wordmark_rows,
                                 left_area.y + left_area.h - wordmark_y),
                        home_wordmark_text_opt, boot_logo_render_opt);
      }
    } else {
      const int fallback_y = stage_area.y + std::max(0, stage_area.h / 2 - 1);
      R->putText(fallback_y, stage_area.x, "waajacamaya animation unavailable",
                 stage_area.w, hero_note_pair, true, false);
      if (fallback_y + 1 < stage_area.y + stage_area.h) {
        R->putText(fallback_y + 1, stage_area.x, home_animation_path_cfg,
                   stage_area.w, hero_note_pair, false, false);
      }
    }

    int info_y = stage_area.y + stage_area.h + 1;
    if (use_static_home_fallback) {
      info_y += static_wordmark_rows + 1;
    }
    if (info_y < left_area.y + left_area.h) {
      R->putText(info_y, left_area.x, "READY", left_area.w, hero_green_pair,
                 true, false);
      info_y += 2;
    }

    static constexpr std::array<std::pair<const char *, const char *>, 6>
        kHomeShortcuts{{
            {"F1", "Home"},
            {"F2", "Workbench"},
            {"F3", "Runtime"},
            {"F4", "Lattice"},
            {"F8", "Shell Logs"},
            {"F9", "Config"},
        }};
    for (const auto &shortcut : kHomeShortcuts) {
      if (info_y >= left_area.y + left_area.h - 1)
        break;
      R->putText(info_y, left_area.x, shortcut.first, 3, hero_green_pair, true,
                 false);
      R->putText(info_y, left_area.x + 5, shortcut.second,
                 std::max(0, left_area.w - 5), hero_green_soft_pair, true,
                 false);
      ++info_y;
    }

    if (left_area.h >= 2) {
      R->putText(left_area.y + left_area.h - 1, left_area.x, "waajacu.com",
                 left_area.w, hero_note_pair, true, false);
    }
  };

  auto render_farewell_ui = [&]() {
    left_main->visible = true;
    left_nav_panel->visible = false;
    left_worklist_panel->visible = false;
    right_main->visible = true;
    right_aux->visible = false;
    title->style.label_color = "#FFF3D9";
    title->style.background_color = "#2A1C11";
    title->style.border_color = "#8D5C33";
    status->visible = false;
    status->style.label_color = "#E5C89A";
    status->style.background_color = "#101014";
    status->style.border_color = "#101014";
    left_main->style.border_color = "#8D5C33";
    left_main->style.label_color = "#F3DFC3";
    left_main->style.title = " waajacamaya ";
    right_main->style.border_color = "#8D5C33";
    right_main->style.label_color = "#F0DEC4";
    right_main->style.title = " farewell ";
    bottom->style.border_color = "#8D5C33";
    bottom->style.label_color = "#E5C89A";
    bottom->style.title = " status ";
    cmdline->style.border_color = "#8D5C33";
    cmdline->style.label_color = "#FAEEDB";

    set_text_box(title, "Good luck!", false);
    set_text_box(status, "", false);
    set_text_box(left_main, "", true);
    set_text_box(right_main, "", true);
    set_text_box(bottom, "waajacu.com | closing terminal", false);
    set_text_box(cmdline, "", false);
    cmdline->focused = false;
    right_main->focused = false;
    right_aux->focused = false;
  };

  auto render_farewell_overlay = [&]() {
    auto *R = get_renderer();
    if (R == nullptr)
      return;

    const Rect left_area = content_rect(*left_main);
    const Rect right_area = content_rect(*right_main);
    const short hero_warm_pair = boot_pair("#F3C77C");
    const short hero_text_pair = boot_pair("#F7E7CE");
    const short hero_note_pair = boot_pair("#C8BBA8");
    const short hero_muted_pair = boot_pair("#7E7468");

    if (left_area.w > 0 && left_area.h > 0) {
      const int wordmark_rows = std::clamp(left_area.h / 10, 3, 5);
      const Rect stage_area =
          home_showcase_stage_rect(left_area, 0.48, /*top_padding_rows=*/0,
                                   /*bottom_reserved_rows=*/wordmark_rows + 6);
      if (farewell_logo_ok) {
        render_home_showcase_static(farewell_logo_image, stage_area,
                                    boot_logo_render_opt);
        const int wordmark_y = stage_area.y + stage_area.h + 1;
        if (wordmark_y < left_area.y + left_area.h) {
          render_art_text(
              "Cuwacunu", left_area.x, wordmark_y, left_area.w,
              std::min(wordmark_rows, left_area.y + left_area.h - wordmark_y),
              home_wordmark_text_opt, boot_logo_render_opt);
        }
      } else {
        const int fallback_y = stage_area.y + std::max(0, stage_area.h / 2 - 1);
        R->putText(fallback_y, stage_area.x,
                   "waajacamaya farewell logo unavailable", stage_area.w,
                   hero_muted_pair, true, false);
        if (fallback_y + 1 < stage_area.y + stage_area.h) {
          R->putText(fallback_y + 1, stage_area.x, closing_logo_path_cfg,
                     stage_area.w, hero_note_pair, false, false);
        }
      }

      if (left_area.h >= 2) {
        R->putText(left_area.y + left_area.h - 1, left_area.x, "waajacu.com",
                   left_area.w, hero_note_pair, true, false);
      }
    }

    if (right_area.w <= 0 || right_area.h <= 0)
      return;

    int y = right_area.y;
    R->putText(y, right_area.x, "Farewell...", right_area.w, hero_warm_pair,
               true, false);
    y += 2;
    if (y < right_area.y + right_area.h) {
      R->putText(y, right_area.x, "cuwacunu.cmd is shutting down", right_area.w,
                 hero_text_pair, false, false);
      ++y;
    }
    if (farewell_logo_uses_loading_fallback &&
        y < right_area.y + right_area.h) {
      R->putText(y, right_area.x,
                 "farewell image fell back to the loading logo", right_area.w,
                 hero_note_pair, false, false);
      ++y;
    }
    if (y < right_area.y + right_area.h) {
      R->putText(y, right_area.x, "See you soon.", right_area.w, hero_text_pair,
                 false, false);
    }
    if (right_area.h >= 1) {
      R->putText(right_area.y + right_area.h - 1, right_area.x, "waajacu.com",
                 right_area.w, hero_note_pair, true, false);
    }
  };

  auto render_farewell_screen = [&]() {
    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H <= 0 || W <= 0)
      return;

    if (workspace && workspace->grid) {
      workspace->grid->cols = {len_spec::frac(0.42), len_spec::frac(0.58)};
    }
    if (left && left->grid) {
      left->grid->rows = {len_spec::frac(0.34), len_spec::frac(0.66)};
      left->grid->gap_row = 0;
      left->grid->pad_bottom = 0;
    }
    if (right && right->grid) {
      right->grid->rows = {len_spec::frac(1.0)};
      right->grid->gap_row = 0;
    }
    render_farewell_ui();
    layout_tree(root, Rect{0, 0, W, H});
    ::erase();
    render_tree(root);
    render_farewell_overlay();
    ::refresh();
    ::napms(kFarewellHoldMs);
  };

  auto finish_boot = [&]() {
    set_mouse_capture(state.shell_logs.mouse_capture);
    log_info("[iinuji_cmd] cuwacunu Hero terminal ready\n");
    log_dbg("[iinuji_cmd] F1 home | F2 workbench | F3 runtime | F4 lattice | "
            "F8 shell logs | F9 config | type 'help' for commands\n");
    log_dbg("[iinuji_cmd] logs setting 'mouse capture' controls terminal "
            "select/copy mode\n");
    if (!state.workbench.ok) {
      log_warn("[iinuji_cmd] workbench screen invalid: %s\n",
               state.workbench.error.c_str());
    }
    if (!state.runtime.ok) {
      log_warn("[iinuji_cmd] runtime view invalid: %s\n",
               state.runtime.error.c_str());
    }
    if (!state.runtime.marshal_ok) {
      log_warn("[iinuji_cmd] marshal runtime defaults unavailable: %s\n",
               state.runtime.marshal_error.c_str());
    }
    if (!state.config.ok) {
      log_warn("[iinuji_cmd] config browser invalid: %s\n",
               state.config.error.c_str());
    }
  };

  constexpr int kVScrollStep = 6;
  constexpr int kHScrollStep = 16;

  auto dlog_tail_seq = []() -> std::uint64_t {
    const auto tail = cuwacunu::piaabo::dlog_snapshot(1);
    if (tail.empty())
      return 0;
    return tail.back().seq;
  };
  std::string last_status_panel_log{};
  std::string last_status_panel_scope{};
  UiEventfulStatusLogSeverity last_status_panel_log_severity =
      UiEventfulStatusLogSeverity::None;
  auto mirror_status_panel_line_to_shell_logs =
      [&](std::string scope, const std::string &line,
          UiEventfulStatusLogSeverity severity) {
    if (line.empty() || severity == UiEventfulStatusLogSeverity::None) {
      last_status_panel_log.clear();
      last_status_panel_scope.clear();
      last_status_panel_log_severity = UiEventfulStatusLogSeverity::None;
      return;
    }
    if (line == last_status_panel_log && scope == last_status_panel_scope &&
        severity == last_status_panel_log_severity) {
      return;
    }
    last_status_panel_log = line;
    last_status_panel_scope = scope;
    last_status_panel_log_severity = severity;
    switch (severity) {
    case UiEventfulStatusLogSeverity::Debug:
      log_dbg("[iinuji_cmd.%s.status] %s\n", scope.c_str(), line.c_str());
      break;
    case UiEventfulStatusLogSeverity::Info:
      log_info("[iinuji_cmd.%s.status] %s\n", scope.c_str(), line.c_str());
      break;
    case UiEventfulStatusLogSeverity::Warning:
      log_warn("[iinuji_cmd.%s.status] %s\n", scope.c_str(), line.c_str());
      break;
    case UiEventfulStatusLogSeverity::Error:
      log_err("[iinuji_cmd.%s.status] %s\n", scope.c_str(), line.c_str());
      break;
    case UiEventfulStatusLogSeverity::None:
      break;
    }
  };

  auto apply_workspace_split = [&](const CmdState &st) {
    if (!workspace || !workspace->grid || workspace->grid->cols.size() < 2)
      return;
    const ScreenMode screen = st.screen;
    if (workspace_is_current_screen_zoomed(st)) {
      workspace->grid->cols =
          workspace_current_screen_uses_left_panel_zoom(st)
              ? std::vector<len_spec>{len_spec::frac(1.0), len_spec::px(0)}
              : std::vector<len_spec>{len_spec::px(0), len_spec::frac(1.0)};
      return;
    }
    if (screen == ScreenMode::ShellLogs) {
      workspace->grid->cols = {len_spec::frac(0.68), len_spec::frac(0.32)};
      return;
    }
    if (screen == ScreenMode::Config) {
      workspace->grid->cols = {len_spec::frac(0.6), len_spec::frac(0.4)};
      return;
    }
    if (screen == ScreenMode::Home) {
      workspace->grid->cols = {len_spec::frac(0.42), len_spec::frac(0.58)};
      return;
    }
    workspace->grid->cols = {len_spec::frac(0.44), len_spec::frac(0.56)};
  };

  auto apply_left_panel_rows = [&](ScreenMode screen) {
    if (!left || !left->grid)
      return;
    if (screen == ScreenMode::Workbench) {
      left->grid->rows = {len_spec::px(10), len_spec::frac(1.0)};
      return;
    }
    if (screen == ScreenMode::Runtime) {
      left->grid->rows = {len_spec::px(10), len_spec::frac(1.0)};
      return;
    }
    if (screen == ScreenMode::Config) {
      left->grid->rows = {len_spec::px(7), len_spec::frac(1.0)};
      return;
    }
    if (screen == ScreenMode::Lattice) {
      left->grid->rows = {len_spec::px(7), len_spec::frac(1.0)};
      return;
    }
    left->grid->rows = {len_spec::frac(0.34), len_spec::frac(0.66)};
  };

  auto apply_left_panel_row_gap = [&](ScreenMode screen) {
    if (!left || !left->grid)
      return;
    // Human mode shows two stacked bordered boxes here, so keep them flush.
    left->grid->gap_row =
        (screen == ScreenMode::Home || screen == ScreenMode::Workbench ||
         screen == ScreenMode::Runtime || screen == ScreenMode::Config ||
         screen == ScreenMode::Lattice)
            ? 0
            : 1;
  };

  auto apply_left_panel_bottom_gap = [&](bool reserve_gap) {
    if (!left || !left->grid)
      return;
    left->grid->pad_bottom = reserve_gap ? 1 : 0;
  };

  auto apply_right_panel_rows = [&](const CmdState &st) {
    if (!right || !right->grid)
      return;
    if (st.screen == ScreenMode::Runtime &&
        runtime_show_secondary_panel(st)) {
      right->grid->rows = {len_spec::frac(0.66), len_spec::frac(0.34)};
      right->grid->gap_row = 0;
      return;
    }
    right->grid->rows = {len_spec::frac(1.0)};
    right->grid->gap_row = 0;
  };

  auto paint_current_ui = [&](int H, int W, bool boot_active,
                              std::uint64_t paint_now_ms) {
    if (boot_active) {
      apply_workspace_split(state);
      apply_left_panel_rows(ScreenMode::Home);
      apply_left_panel_row_gap(ScreenMode::Home);
      apply_left_panel_bottom_gap(false);
      apply_right_panel_rows(state);
      render_boot_ui();
      mirror_status_panel_line_to_shell_logs(
          "home", "waajacu.com | bootstrap in progress",
          UiEventfulStatusLogSeverity::Debug);
    } else {
      apply_workspace_split(state);
      apply_left_panel_rows(state.screen);
      apply_left_panel_row_gap(state.screen);
      apply_left_panel_bottom_gap(false);
      apply_right_panel_rows(state);
      if (state.screen == ScreenMode::Runtime) {
        runtime_keep_log_viewer_following(state);
      }
      refresh_ui(state, title, status, left, left_nav_panel,
                 left_worklist_panel, right_main, right_aux, bottom, cmdline);
      mirror_status_panel_line_to_shell_logs(
          ui_eventful_status_log_scope(state), ui_eventful_status_line(state),
          ui_eventful_status_log_severity(state));
      if (state.screen == ScreenMode::ShellLogs &&
          state.shell_logs.auto_follow) {
        jump_logs_to_bottom(state, left, right);
      }
    }
    layout_tree(root, Rect{0, 0, W, H});
    ::erase();
    render_tree(root);
    if (boot_active) {
      render_boot_overlay(paint_now_ms);
    } else {
      if (state.screen == ScreenMode::Home) {
        render_home_overlay(paint_now_ms);
      }
      render_help_overlay(state, left, right);
      render_logs_scroll_controls(state, left);
      render_workspace_zoom_controls(state, left, right);
    }
    ::refresh();
  };
  state.repaint_now = [&]() {
    int repaint_h = 0;
    int repaint_w = 0;
    getmaxyx(stdscr, repaint_h, repaint_w);
    const std::uint64_t paint_now_ms = monotonic_now_ms();
    const bool paint_boot_active =
        (boot_stage != BootStage::Ready) ||
        (boot_visual_hold_until_ms != 0u &&
         paint_now_ms < boot_visual_hold_until_ms);
    paint_current_ui(repaint_h, repaint_w, paint_boot_active, paint_now_ms);
  };

  bool dirty = true;
  int last_h = -1;
  int last_w = -1;
  std::uint64_t last_log_seq = dlog_tail_seq();
  int current_input_timeout = app_opts.input_timeout_ms;
  std::uint64_t last_idle_refresh_ms = monotonic_now_ms();
  while (state.running) {
    const std::uint64_t now_ms = monotonic_now_ms();
    const bool boot_active =
        (boot_stage != BootStage::Ready) ||
        (boot_visual_hold_until_ms != 0u && now_ms < boot_visual_hold_until_ms);

    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H != last_h || W != last_w) {
      last_h = H;
      last_w = W;
      dirty = true;
    }

    if (boot_active && boot_stage_needs_paint)
      dirty = true;

    if (dirty) {
      paint_current_ui(H, W, boot_active, now_ms);
      dirty = false;
    }

    if (!boot_active && poll_workbench_async_updates(state)) {
      dirty = true;
      last_idle_refresh_ms = monotonic_now_ms();
    }

    if (!boot_active && poll_lattice_async_updates(state)) {
      dirty = true;
      last_idle_refresh_ms = monotonic_now_ms();
    }

    if (!boot_active && poll_runtime_async_updates(state)) {
      dirty = true;
      last_idle_refresh_ms = monotonic_now_ms();
    }

    if (!boot_active && state.screen == ScreenMode::Runtime) {
      (void)runtime_poll_device_refresh(state, now_ms);
    }

    if (!boot_active && state.screen == ScreenMode::Runtime &&
        runtime_poll_live_log_viewer(state, now_ms)) {
      dirty = true;
    }

    if (!boot_active && poll_config_async_updates(state)) {
      dirty = true;
      last_idle_refresh_ms = monotonic_now_ms();
    }

    if (boot_stage != BootStage::Ready) {
      if (boot_stage_needs_paint) {
        boot_stage_needs_paint = false;
        continue;
      }

      switch (boot_stage) {
      case BootStage::LoadHumanDefaults: {
        log_boot_stage_begin(BootStage::LoadHumanDefaults);
        state.workbench.app.global_config_path = global_config_path;
        state.workbench.app.hero_config_path =
            cuwacunu::hero::human_mcp::resolve_human_hero_dsl_path(
                global_config_path);
        state.workbench.app.self_binary_path =
            cuwacunu::hero::human_mcp::current_executable_path();
        std::string error{};
        if (!state.workbench.app.hero_config_path.empty() &&
            cuwacunu::hero::human_mcp::load_human_defaults(
                state.workbench.app.hero_config_path, global_config_path,
                &state.workbench.app.defaults, &error)) {
          state.workbench.ok = true;
          set_workbench_status(state, "Human Hero defaults loaded.", false);
          log_boot_stage_finish(
              BootStage::LoadHumanDefaults, true,
              "hero_config_path=" +
                  state.workbench.app.hero_config_path.string() +
                  " marshal_root=" +
                  state.workbench.app.defaults.marshal_root.string() +
                  " operator_id=" + state.workbench.app.defaults.operator_id);
        } else {
          state.workbench.ok = false;
          state.workbench.error =
              error.empty() ? "missing Human Hero config" : error;
          set_workbench_status(state, state.workbench.error, true);
          log_boot_stage_finish(
              BootStage::LoadHumanDefaults, false,
              "hero_config_path=" +
                  state.workbench.app.hero_config_path.string() +
                  " error=" + state.workbench.error);
        }
        (void)advance_boot_stage(BootStage::LoadHumanDefaults,
                                 BootStage::LoadMarshalDefaults);
      } break;
      case BootStage::LoadMarshalDefaults: {
        log_boot_stage_begin(BootStage::LoadMarshalDefaults);
        state.runtime.marshal_app.global_config_path = global_config_path;
        state.runtime.marshal_app.hero_config_path =
            cuwacunu::hero::marshal_mcp::resolve_marshal_hero_dsl_path(
                global_config_path);
        state.runtime.marshal_app.self_binary_path.clear();
        std::string error{};
        if (!state.runtime.marshal_app.hero_config_path.empty() &&
            cuwacunu::hero::marshal_mcp::load_marshal_defaults(
                state.runtime.marshal_app.hero_config_path, global_config_path,
                &state.runtime.marshal_app.defaults, &error)) {
          const std::filesystem::path repo_root =
              state.runtime.marshal_app.defaults.repo_root;
          if (!repo_root.empty()) {
            state.runtime.marshal_app.self_binary_path =
                (repo_root / ".build/hero/hero_marshal.mcp").lexically_normal();
          } else {
            state.runtime.marshal_app.self_binary_path =
                "/cuwacunu/.build/hero/hero_marshal.mcp";
          }
          state.runtime.marshal_ok = true;
          state.runtime.marshal_error.clear();
          log_boot_stage_finish(
              BootStage::LoadMarshalDefaults, true,
              "hero_config_path=" +
                  state.runtime.marshal_app.hero_config_path.string() +
                  " marshal_root=" +
                  state.runtime.marshal_app.defaults.marshal_root.string() +
                  " repo_root=" +
                  state.runtime.marshal_app.defaults.repo_root.string());
        } else {
          state.runtime.marshal_ok = false;
          state.runtime.marshal_error =
              error.empty() ? "missing Marshal Hero config" : error;
          log_boot_stage_finish(
              BootStage::LoadMarshalDefaults, false,
              "hero_config_path=" +
                  state.runtime.marshal_app.hero_config_path.string() +
                  " error=" + state.runtime.marshal_error);
        }
        (void)advance_boot_stage(BootStage::LoadMarshalDefaults,
                                 BootStage::LoadRuntimeDefaults);
      } break;
      case BootStage::LoadRuntimeDefaults: {
        log_boot_stage_begin(BootStage::LoadRuntimeDefaults);
        state.runtime.app.global_config_path = global_config_path;
        state.runtime.app.hero_config_path =
            cuwacunu::hero::runtime_mcp::resolve_runtime_hero_dsl_path(
                global_config_path);
        state.runtime.app.self_binary_path =
            cuwacunu::hero::runtime_mcp::current_executable_path();
        std::string error{};
        if (!state.runtime.app.hero_config_path.empty() &&
            cuwacunu::hero::runtime_mcp::load_runtime_defaults(
                state.runtime.app.hero_config_path, global_config_path,
                &state.runtime.app.defaults, &error)) {
          state.runtime.ok = true;
          state.runtime.status = "Runtime Hero defaults loaded.";
          log_boot_stage_finish(
              BootStage::LoadRuntimeDefaults, true,
              "hero_config_path=" +
                  state.runtime.app.hero_config_path.string() +
                  " campaigns_root=" +
                  state.runtime.app.defaults.campaigns_root.string() +
                  " marshal_root=" +
                  state.runtime.app.defaults.marshal_root.string());
        } else {
          state.runtime.ok = false;
          state.runtime.error =
              error.empty() ? "missing Runtime Hero config" : error;
          state.runtime.status = state.runtime.error;
          state.runtime.status_is_error = true;
          log_boot_stage_finish(
              BootStage::LoadRuntimeDefaults, false,
              "hero_config_path=" +
                  state.runtime.app.hero_config_path.string() +
                  " error=" + state.runtime.error);
        }
        (void)advance_boot_stage(BootStage::LoadRuntimeDefaults,
                                 BootStage::LoadHumanWorkbench);
      } break;
      case BootStage::LoadHumanWorkbench:
        log_boot_stage_begin(BootStage::LoadHumanWorkbench);
        (void)refresh_workbench_state(state);
        if (state.workbench.ok) {
          log_boot_stage_finish(
              BootStage::LoadHumanWorkbench, true,
              "sessions=" +
                  std::to_string(count_tracked_operator_sessions(state)) +
                  " pending_requests=" +
                  std::to_string(count_pending_operator_requests(state)) +
                  " pending_reviews=" +
                  std::to_string(count_pending_operator_reviews(state)));
        } else {
          log_boot_stage_finish(BootStage::LoadHumanWorkbench, false,
                                "error=" + state.workbench.error);
        }
        (void)advance_boot_stage(BootStage::LoadHumanWorkbench,
                                 BootStage::LoadConfig);
        break;
      case BootStage::LoadConfig:
        log_boot_stage_begin(BootStage::LoadConfig);
        state.config = load_config_view_from_state(state);
        clamp_selected_config_file(state);
        if (state.config.ok) {
          log_boot_stage_finish(
              BootStage::LoadConfig, true,
              "policy_path=" + state.config.policy_path +
                  " write_policy_path=" +
                  config_effective_write_policy_path(state.config) + " files=" +
                  std::to_string(state.config.files.size()) + " defaults=" +
                  std::to_string(
                      count_config_family(state, ConfigFileFamily::Defaults)) +
                  " objectives=" +
                  std::to_string(count_config_family(
                      state, ConfigFileFamily::Objectives)) +
                  " optim=" +
                  std::to_string(
                      count_config_family(state, ConfigFileFamily::Optim)) +
                  " temp=" +
                  std::to_string(
                      count_config_family(state, ConfigFileFamily::Temp)));
        } else {
          log_boot_stage_finish(BootStage::LoadConfig, false,
                                "error=" + state.config.error);
        }
        (void)advance_boot_stage(BootStage::LoadConfig, BootStage::LoadRuntime);
        break;
      case BootStage::LoadRuntime:
        log_boot_stage_begin(BootStage::LoadRuntime);
        (void)refresh_runtime_state(state);
        if (state.runtime.ok) {
          log_boot_stage_finish(
              BootStage::LoadRuntime, true,
              "sessions=" + std::to_string(state.runtime.sessions.size()) +
                  " campaigns=" +
                  std::to_string(state.runtime.campaigns.size()) +
                  " jobs=" + std::to_string(state.runtime.jobs.size()) +
                  " gpus=" + std::to_string(state.runtime.device.gpus.size()));
        } else {
          log_boot_stage_finish(BootStage::LoadRuntime, false,
                                "error=" + state.runtime.error);
        }
        boot_visual_hold_until_ms = std::max<std::uint64_t>(
            advance_boot_stage(BootStage::LoadRuntime, BootStage::Ready),
            boot_visual_start_ms + boot_estimated_total_ms());
        persist_boot_stage_estimate_ms();
        finish_boot();
        break;
      case BootStage::Ready:
        break;
      }

      dirty = true;
      boot_stage_needs_paint = (boot_stage != BootStage::Ready);
      continue;
    }

    if (state.screen == ScreenMode::ShellLogs) {
      const std::uint64_t log_seq = dlog_tail_seq();
      if (log_seq != last_log_seq) {
        last_log_seq = log_seq;
        if (state.shell_logs.auto_follow)
          jump_logs_to_bottom(state, left, right);
        dirty = true;
      }
    }
    if (apply_logs_pending_actions(state, left, right))
      dirty = true;

    const std::uint64_t idle_refresh_period_ms =
        boot_active ? static_cast<std::uint64_t>(kAnimatedHomeFrameStepMs)
                    : idle_refresh_period_ms_for_screen(
                          state.screen, home_animation_is_dynamic);
    if ((now_ms - last_idle_refresh_ms) >= idle_refresh_period_ms) {
      if (boot_active ||
          refresh_visible_screen_on_idle(state, home_animation_is_dynamic))
        dirty = true;
      last_idle_refresh_ms = now_ms;
    }

    const int desired_timeout =
        boot_active ? kAnimatedHomeFrameStepMs
                    : desired_input_timeout_for_screen(
                          state.screen, home_animation_is_dynamic);
    if (desired_timeout != current_input_timeout) {
      ::wtimeout(stdscr, desired_timeout);
      current_input_timeout = desired_timeout;
    }

    const int ch = getch();
    if (ch == ERR) {
      continue;
    }
    if (ch == KEY_RESIZE) {
      dirty = true;
      continue;
    }
    const auto key_result = dispatch_app_key(
        state, ch, left, right, kVScrollStep, kHScrollStep, set_mouse_capture);
    if (key_result.handled) {
      if (key_result.dirty)
        dirty = true;
      continue;
    }
  }

  log_dbg("[iinuji_cmd] showing farewell screen\n");
  render_farewell_screen();
  return 0;
} catch (const std::exception &e) {
  endwin();
  log_err("[iinuji_cmd] exception: %s\n", e.what());
  return 1;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
