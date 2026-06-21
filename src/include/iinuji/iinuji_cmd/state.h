#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#include "hero/config_path_defaults.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class ScreenMode : std::uint8_t {
  Home = 0,
  Workbench = 1,
  Runtime = 2,
  Lattice = 3,
  Logs = 4,
  Config = 5,
};

enum class RuntimeFocus : std::uint8_t {
  Devices = 0,
  Jobs = 1,
};

enum class RuntimeDetailMode : std::uint8_t {
  Manifest = 0,
  Log = 1,
  Trace = 2,
};

enum class WorkspaceZoomSlot : std::uint8_t {
  None = 0,
  RuntimeDetail = 1,
  LogsStream = 2,
  LatticeDetail = 3,
  ConfigDetail = 4,
};

enum class LogsSourceFilter : std::uint8_t {
  All = 0,
  Refresh = 1,
  Action = 2,
  Command = 3,
  Show = 4,
  Status = 5,
};

inline constexpr unsigned kLogsSourceFilterCount = 6u;

enum class LogsSeverityFilter : std::uint8_t {
  DebugOrHigher = 0,
  InfoOrHigher = 1,
  WarningOrHigher = 2,
  ErrorOrHigher = 3,
  FatalOnly = 4,
};

inline constexpr unsigned kLogsSeverityFilterCount = 5u;

enum class LogsMetadataFilter : std::uint8_t {
  Any = 0,
  WithAnyMetadata = 1,
  WithFunction = 2,
  WithPath = 3,
  WithCallsite = 4,
};

inline constexpr unsigned kLogsMetadataFilterCount = 5u;

enum class HomePresentationMode : std::uint8_t {
  Showcase = 0,
  BootstrapSplash = 1,
  FarewellSplash = 2,
};

enum class CmdChoiceMenuKind : std::uint8_t {
  None = 0,
  RuntimeSelection = 1,
};

enum class CmdContentPopupKind : std::uint8_t {
  None = 0,
  RuntimePolicy = 1,
  RuntimeJobManifest = 2,
  RuntimeJobState = 3,
  RuntimeJobReport = 4,
  RuntimeJobTrace = 5,
};

struct WorkspaceState {
  std::uint32_t zoom_flags{0};
};

inline constexpr std::uint32_t workspace_zoom_bit(WorkspaceZoomSlot slot) {
  if (slot == WorkspaceZoomSlot::None)
    return 0u;
  return 1u << (static_cast<std::uint32_t>(slot) - 1u);
}

inline WorkspaceZoomSlot workspace_zoom_slot_for_screen(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Runtime:
    return WorkspaceZoomSlot::RuntimeDetail;
  case ScreenMode::Logs:
    return WorkspaceZoomSlot::LogsStream;
  case ScreenMode::Lattice:
    return WorkspaceZoomSlot::LatticeDetail;
  case ScreenMode::Config:
    return WorkspaceZoomSlot::ConfigDetail;
  case ScreenMode::Home:
  case ScreenMode::Workbench:
    break;
  }
  return WorkspaceZoomSlot::None;
}

inline bool workspace_zoom_slot_uses_left_panel(WorkspaceZoomSlot slot) {
  return slot == WorkspaceZoomSlot::LogsStream;
}

inline bool workspace_screen_supports_zoom(ScreenMode screen) {
  return workspace_zoom_slot_for_screen(screen) != WorkspaceZoomSlot::None;
}

inline const std::vector<ScreenMode> &screen_order() {
  static const std::vector<ScreenMode> order{
      ScreenMode::Home,    ScreenMode::Workbench, ScreenMode::Runtime,
      ScreenMode::Lattice, ScreenMode::Logs,      ScreenMode::Config,
  };
  return order;
}

inline std::string screen_key_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "F1";
  case ScreenMode::Workbench:
    return "F2";
  case ScreenMode::Runtime:
    return "F3";
  case ScreenMode::Lattice:
    return "F4";
  case ScreenMode::Logs:
    return "F8";
  case ScreenMode::Config:
    return "F9";
  }
  return "F1";
}

inline std::string screen_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "Home";
  case ScreenMode::Workbench:
    return "Workbench";
  case ScreenMode::Runtime:
    return "Runtime";
  case ScreenMode::Lattice:
    return "Lattice";
  case ScreenMode::Config:
    return "Config";
  case ScreenMode::Logs:
    return "Shell Logs";
  }
  return "Home";
}

inline std::string screen_status_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "screen=home";
  case ScreenMode::Workbench:
    return "screen=workbench";
  case ScreenMode::Runtime:
    return "screen=runtime";
  case ScreenMode::Lattice:
    return "screen=lattice";
  case ScreenMode::Config:
    return "screen=config";
  case ScreenMode::Logs:
    return "screen=shell_logs";
  }
  return "screen=home";
}

inline std::string screen_badge_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "HOME";
  case ScreenMode::Workbench:
    return "WORKBENCH";
  case ScreenMode::Runtime:
    return "RUNTIME";
  case ScreenMode::Lattice:
    return "LATTICE";
  case ScreenMode::Config:
    return "CONFIG";
  case ScreenMode::Logs:
    return "SHELL LOGS";
  }
  return "HOME";
}

inline std::string runtime_focus_label(RuntimeFocus focus) {
  switch (focus) {
  case RuntimeFocus::Devices:
    return "Devices";
  case RuntimeFocus::Jobs:
    return "Jobs";
  }
  return "Devices";
}

inline std::string runtime_lane_label(RuntimeFocus focus) {
  return runtime_focus_label(focus);
}

inline std::string runtime_row_kind_label(RuntimeFocus focus) {
  switch (focus) {
  case RuntimeFocus::Devices:
    return "device";
  case RuntimeFocus::Jobs:
    return "job";
  }
  return "device";
}

inline std::string runtime_detail_mode_label(RuntimeDetailMode mode) {
  switch (mode) {
  case RuntimeDetailMode::Manifest:
    return "manifest";
  case RuntimeDetailMode::Log:
    return "log";
  case RuntimeDetailMode::Trace:
    return "trace";
  }
  return "manifest";
}

inline std::string logs_source_filter_label(LogsSourceFilter filter) {
  switch (filter) {
  case LogsSourceFilter::All:
    return "any";
  case LogsSourceFilter::Refresh:
    return "refresh";
  case LogsSourceFilter::Action:
    return "action";
  case LogsSourceFilter::Command:
    return "command";
  case LogsSourceFilter::Show:
    return "show";
  case LogsSourceFilter::Status:
    return "status";
  }
  return "any";
}

inline bool logs_source_filter_accepts(LogsSourceFilter filter,
                                       const std::string &source) {
  switch (filter) {
  case LogsSourceFilter::All:
    return true;
  case LogsSourceFilter::Refresh:
    return source == "refresh";
  case LogsSourceFilter::Action:
    return source == "action";
  case LogsSourceFilter::Command:
    return source == "command";
  case LogsSourceFilter::Show:
    return source == "show";
  case LogsSourceFilter::Status:
    return source == "status";
  }
  return true;
}

inline std::string logs_severity_filter_label(LogsSeverityFilter filter) {
  switch (filter) {
  case LogsSeverityFilter::DebugOrHigher:
    return "DEBUG+";
  case LogsSeverityFilter::InfoOrHigher:
    return "INFO+";
  case LogsSeverityFilter::WarningOrHigher:
    return "WARNING+";
  case LogsSeverityFilter::ErrorOrHigher:
    return "ERROR+";
  case LogsSeverityFilter::FatalOnly:
    return "FATAL";
  }
  return "INFO+";
}

inline std::string logs_metadata_filter_label(LogsMetadataFilter filter) {
  switch (filter) {
  case LogsMetadataFilter::Any:
    return "ANY";
  case LogsMetadataFilter::WithAnyMetadata:
    return "META+";
  case LogsMetadataFilter::WithFunction:
    return "FN+";
  case LogsMetadataFilter::WithPath:
    return "PATH+";
  case LogsMetadataFilter::WithCallsite:
    return "CALLSITE+";
  }
  return "ANY";
}

struct RuntimeGpuProcessSnapshot {
  std::uint64_t pid{0};
  std::string gpu_uuid{};
  std::uint64_t used_memory_mib{0};
};

struct RuntimeGpuSnapshot {
  std::string index{};
  std::string name{};
  std::string uuid{};
  int utilization_gpu_pct{-1};
  int utilization_mem_pct{-1};
  std::uint64_t memory_used_mib{0};
  std::uint64_t memory_total_mib{0};
  int temperature_c{-1};
};

struct RuntimeDeviceSnapshot {
  bool ok{false};
  std::string error{};
  std::string host_name{};
  std::uint32_t cpu_count{0};
  std::uint64_t cpu_user_ticks{0};
  std::uint64_t cpu_nice_ticks{0};
  std::uint64_t cpu_system_ticks{0};
  std::uint64_t cpu_idle_ticks{0};
  std::uint64_t cpu_iowait_ticks{0};
  std::uint64_t cpu_irq_ticks{0};
  std::uint64_t cpu_softirq_ticks{0};
  std::uint64_t cpu_steal_ticks{0};
  int cpu_usage_pct{-1};
  int cpu_iowait_pct{-1};
  double load1{0.0};
  double load5{0.0};
  double load15{0.0};
  std::uint64_t mem_total_kib{0};
  std::uint64_t mem_available_kib{0};
  bool gpu_query_attempted{false};
  std::string gpu_error{};
  std::vector<RuntimeGpuSnapshot> gpus{};
  std::vector<RuntimeGpuProcessSnapshot> gpu_processes{};
  std::uint64_t collected_at_ms{0};
};

struct RuntimePolicySummary {
  std::filesystem::path policy_path{};
  std::filesystem::path runtime_exec_path{};
  std::filesystem::path runtime_root{};
  std::string protocol_layer{};
  std::string default_dry_run{};
  std::string allow_execute{};
  std::string allow_train_execute{};
  bool runtime_exec_exists{false};
  bool runtime_exec_executable{false};
};

struct RuntimeWaveSummary {
  std::filesystem::path wave_path{};
  std::string wave_id{};
  std::string target{};
  std::string mode{};
  std::string source_range{};
  std::string source_order{};
  std::string anchor_index_begin{};
  std::string anchor_index_end{};
};

struct RuntimeJobSummary {
  std::filesystem::path job_dir{};
  std::filesystem::file_time_type last_write_time{};
  std::string job_id{};
  std::string status{};
  std::string job_kind{};
  std::string wave_id{};
  std::string wave_action{};
  std::string target_component_family_id{};
  std::string accepted_anchor_count{};
  std::string steps_completed{};
  std::string last_loss{};
  std::string report_path{};
  std::string checkpoint_path{};
};

struct RuntimeState {
  RuntimePolicySummary policy{};
  RuntimeWaveSummary wave{};
  RuntimeDeviceSnapshot device{};
  std::vector<RuntimeDeviceSnapshot> device_history{};
  std::uint64_t device_last_refresh_ms{0};
  RuntimeFocus focus{RuntimeFocus::Devices};
  RuntimeDetailMode detail_mode{RuntimeDetailMode::Manifest};
  std::size_t selected_device_row{0};
  std::vector<RuntimeJobSummary> jobs{};
  std::size_t selected_job{0};
};

struct ConfigFileSummary {
  std::filesystem::path path{};
  std::string relative_path{};
  std::uintmax_t size{0};
};

struct ConfigState {
  std::filesystem::path root{};
  std::vector<ConfigFileSummary> files{};
  std::size_t selected_file{0};
  bool file_editor_open{false};
};

struct LatticeState {
  std::filesystem::path targets_path{};
  std::filesystem::path runtime_root{};
  std::vector<std::string> target_ids{};
  std::size_t selected_target{0};
  std::size_t exposure_fact_count{0};
  std::size_t checkpoint_fact_count{0};
};

struct HomeVisualState {
  std::filesystem::path loading_logo_path{};
  std::filesystem::path closing_logo_path{};
  std::filesystem::path home_animation_path{};
  std::string loading_logo_error{};
  std::string home_animation_error{};
  bool loading_logo_ok{false};
  bool animation_ok{false};
  bool animation_dynamic{false};
  std::size_t animation_frames{0};
  int animation_width{0};
  int animation_height{0};
  HomePresentationMode presentation{HomePresentationMode::Showcase};
};

struct LogEntry {
  std::uint64_t timestamp_ms{0};
  std::string source{};
  std::string message{};
};

struct LogsState {
  std::size_t buffer_capacity{6000};
  LogsSourceFilter source_filter{LogsSourceFilter::All};
  LogsSeverityFilter severity_filter{LogsSeverityFilter::InfoOrHigher};
  LogsMetadataFilter metadata_filter{LogsMetadataFilter::Any};
  bool show_timestamp{false};
  bool show_thread{false};
  bool show_metadata{true};
  bool show_color{true};
  bool auto_follow{true};
  bool mouse_capture{true};
  std::size_t selected_setting{0};
  int scroll_y{0};
  int scroll_x{0};
};

struct CmdChoiceMenuState {
  CmdChoiceMenuKind kind{CmdChoiceMenuKind::None};
  std::size_t selected{0};
  int scroll_y{0};
};

struct CmdContentPopupState {
  CmdContentPopupKind kind{CmdContentPopupKind::None};
  int scroll_y{0};
  int scroll_x{0};
};

struct CmdState {
  std::string command_name{"cuwacunu_cmd"};
  std::filesystem::path global_config_path{
      cuwacunu::hero::config_paths::default_global_config_path()};
  std::filesystem::path config_root{
      cuwacunu::hero::config_paths::default_global_config_path().parent_path()};
  ScreenMode screen{ScreenMode::Home};
  WorkspaceState workspace{};
  HomeVisualState home_visual{};
  RuntimeState runtime{};
  ConfigState config{};
  LatticeState lattice{};
  LogsState logs{};
  std::vector<LogEntry> log{};
  std::uint64_t animation_tick{0};
  bool help_view{false};
  int help_scroll_y{0};
  int help_scroll_x{0};
  bool action_menu_open{false};
  std::size_t action_menu_selected{0};
  int action_menu_scroll_y{0};
  CmdChoiceMenuState choice_menu{};
  CmdContentPopupState content_popup{};
  bool command_mode{false};
  bool command_focused{false};
  std::vector<std::string> command_history{};
  int command_history_cursor{-1};
  std::string command_history_draft{};
  std::string status{"ready"};
  std::string last_status_log_scope{};
  std::string last_status_log_text{};
  bool quit{false};
};

inline bool cmd_choice_menu_open(const CmdState &state) {
  return state.choice_menu.kind != CmdChoiceMenuKind::None;
}

inline bool cmd_content_popup_open(const CmdState &state) {
  return state.content_popup.kind != CmdContentPopupKind::None;
}

inline void close_choice_menu(CmdState &state) {
  state.choice_menu.kind = CmdChoiceMenuKind::None;
  state.choice_menu.selected = 0u;
  state.choice_menu.scroll_y = 0;
}

inline void close_content_popup(CmdState &state) {
  state.content_popup.kind = CmdContentPopupKind::None;
  state.content_popup.scroll_y = 0;
  state.content_popup.scroll_x = 0;
}

inline void open_choice_menu(CmdState &state, CmdChoiceMenuKind kind) {
  state.help_view = false;
  state.action_menu_open = false;
  state.action_menu_scroll_y = 0;
  close_content_popup(state);
  state.choice_menu.kind = kind;
  state.choice_menu.selected = 0u;
  state.choice_menu.scroll_y = 0;
}

inline void open_content_popup(CmdState &state, CmdContentPopupKind kind,
                               bool preserve_choice_menu = false) {
  state.help_view = false;
  state.action_menu_open = false;
  state.action_menu_scroll_y = 0;
  if (!preserve_choice_menu)
    close_choice_menu(state);
  state.content_popup.kind = kind;
  state.content_popup.scroll_y = 0;
  state.content_popup.scroll_x = 0;
}

inline bool workspace_zoom_slot_enabled(const CmdState &state,
                                        WorkspaceZoomSlot slot) {
  const std::uint32_t bit = workspace_zoom_bit(slot);
  return bit != 0u && (state.workspace.zoom_flags & bit) != 0u;
}

inline bool workspace_is_current_screen_zoomed(const CmdState &state) {
  return workspace_zoom_slot_enabled(
      state, workspace_zoom_slot_for_screen(state.screen));
}

inline bool
workspace_current_screen_uses_left_panel_zoom(const CmdState &state) {
  return workspace_zoom_slot_uses_left_panel(
      workspace_zoom_slot_for_screen(state.screen));
}

inline bool workspace_toggle_current_screen_zoom(CmdState &state) {
  const WorkspaceZoomSlot slot = workspace_zoom_slot_for_screen(state.screen);
  const std::uint32_t bit = workspace_zoom_bit(slot);
  if (bit == 0u)
    return false;
  state.workspace.zoom_flags ^= bit;
  return true;
}

inline bool workspace_restore_current_screen_split(CmdState &state) {
  const WorkspaceZoomSlot slot = workspace_zoom_slot_for_screen(state.screen);
  const std::uint32_t bit = workspace_zoom_bit(slot);
  if (bit == 0u || (state.workspace.zoom_flags & bit) == 0u)
    return false;
  state.workspace.zoom_flags &= ~bit;
  return true;
}

inline void select_relative_action_menu_item(CmdState &state, int delta) {
  if (delta < 0) {
    const std::size_t amount = static_cast<std::size_t>(-delta);
    state.action_menu_selected = amount > state.action_menu_selected
                                     ? 0u
                                     : state.action_menu_selected - amount;
  } else if (delta > 0) {
    const std::size_t amount = static_cast<std::size_t>(delta);
    const std::size_t max_value = std::numeric_limits<std::size_t>::max();
    state.action_menu_selected = state.action_menu_selected > max_value - amount
                                     ? max_value
                                     : state.action_menu_selected + amount;
  }
  state.action_menu_open = true;
  state.help_view = false;
  close_content_popup(state);
  state.status = "action menu";
}

inline void select_first_action_menu_item(CmdState &state) {
  state.action_menu_selected = 0u;
  state.action_menu_open = true;
  state.help_view = false;
  close_content_popup(state);
  state.status = "action menu";
}

inline void select_last_action_menu_item(CmdState &state) {
  state.action_menu_selected = std::numeric_limits<std::size_t>::max();
  state.action_menu_open = true;
  state.help_view = false;
  close_content_popup(state);
  state.status = "action menu";
}

inline void set_runtime_focus(CmdState &state, RuntimeFocus focus) {
  state.runtime.focus = focus;
  state.status = "runtime lane=" + runtime_lane_label(focus);
}

inline std::size_t wrap_relative_index(std::size_t value, std::size_t count,
                                       int delta) {
  if (count == 0u)
    return 0u;

  const std::size_t current = value % count;
  if (delta == 0)
    return current;

  const std::size_t magnitude =
      delta < 0 ? static_cast<std::size_t>(-(delta + 1)) + 1u
                : static_cast<std::size_t>(delta);
  const std::size_t step = magnitude % count;
  if (delta > 0)
    return (current + step) % count;
  return (current + count - step) % count;
}

inline void select_relative_runtime_focus(CmdState &state, int delta) {
  if (delta == 0)
    return;
  const std::size_t current =
      state.runtime.focus == RuntimeFocus::Devices ? 0u : 1u;
  const std::size_t next = wrap_relative_index(current, 2u, delta);
  set_runtime_focus(state,
                    next == 0 ? RuntimeFocus::Devices : RuntimeFocus::Jobs);
}

inline void cycle_runtime_detail_mode(CmdState &state) {
  switch (state.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    state.runtime.detail_mode = RuntimeDetailMode::Log;
    break;
  case RuntimeDetailMode::Log:
    state.runtime.detail_mode = RuntimeDetailMode::Trace;
    break;
  case RuntimeDetailMode::Trace:
    state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    break;
  }
  state.status =
      "runtime detail=" + runtime_detail_mode_label(state.runtime.detail_mode);
}

inline void set_runtime_detail_mode(CmdState &state, RuntimeDetailMode mode) {
  state.runtime.detail_mode = mode;
  state.status =
      "runtime detail=" + runtime_detail_mode_label(state.runtime.detail_mode);
}

inline bool select_relative_runtime_job(CmdState &state, int delta) {
  if (state.runtime.jobs.empty())
    return false;
  state.runtime.selected_job = wrap_relative_index(
      state.runtime.selected_job, state.runtime.jobs.size(), delta);
  return true;
}

inline bool select_relative_runtime_device(CmdState &state, int delta) {
  const std::size_t size = state.runtime.device.gpus.size() + 1u;
  state.runtime.selected_device_row =
      wrap_relative_index(state.runtime.selected_device_row, size, delta);
  return true;
}

inline void select_relative_runtime_item(CmdState &state, int delta) {
  if (delta == 0)
    return;

  const bool moved = state.runtime.focus == RuntimeFocus::Devices
                         ? select_relative_runtime_device(state, delta)
                         : select_relative_runtime_job(state, delta);
  if (!moved) {
    state.status = "runtime " + runtime_row_kind_label(state.runtime.focus) +
                   " list empty";
    return;
  }

  state.status = "runtime " + runtime_row_kind_label(state.runtime.focus) +
                 (delta < 0 ? "=prev" : "=next");
}

inline constexpr std::size_t logs_settings_count() { return 9u; }

inline unsigned wrap_relative_logs_filter(unsigned value, unsigned count,
                                          int delta) {
  if (count == 0u)
    return 0u;

  int next = static_cast<int>(value) + delta;
  const int size = static_cast<int>(count);
  while (next < 0)
    next += size;
  while (next >= size)
    next -= size;
  return static_cast<unsigned>(next);
}

inline void cycle_logs_source_filter(CmdState &state, int delta) {
  const auto next =
      wrap_relative_logs_filter(static_cast<unsigned>(state.logs.source_filter),
                                kLogsSourceFilterCount, delta);
  state.logs.source_filter = static_cast<LogsSourceFilter>(next);
  state.screen = ScreenMode::Logs;
  state.status =
      "logs.source=" + logs_source_filter_label(state.logs.source_filter);
}

inline void cycle_logs_severity_filter(CmdState &state, int delta) {
  const auto next = wrap_relative_logs_filter(
      static_cast<unsigned>(state.logs.severity_filter),
      kLogsSeverityFilterCount, delta);
  state.logs.severity_filter = static_cast<LogsSeverityFilter>(next);
  state.screen = ScreenMode::Logs;
  state.status =
      "logs.level=" + logs_severity_filter_label(state.logs.severity_filter);
}

inline void cycle_logs_metadata_filter(CmdState &state, int delta) {
  const auto next = wrap_relative_logs_filter(
      static_cast<unsigned>(state.logs.metadata_filter),
      kLogsMetadataFilterCount, delta);
  state.logs.metadata_filter = static_cast<LogsMetadataFilter>(next);
  state.screen = ScreenMode::Logs;
  state.status = "logs.metadata_filter=" +
                 logs_metadata_filter_label(state.logs.metadata_filter);
}

inline void toggle_logs_follow(CmdState &state) {
  state.logs.auto_follow = !state.logs.auto_follow;
  state.screen = ScreenMode::Logs;
  state.status =
      std::string("logs.follow=") + (state.logs.auto_follow ? "on" : "off");
}

inline void toggle_logs_timestamp(CmdState &state) {
  state.logs.show_timestamp = !state.logs.show_timestamp;
  state.screen = ScreenMode::Logs;
  state.status =
      std::string("logs.date=") + (state.logs.show_timestamp ? "on" : "off");
}

inline void toggle_logs_thread(CmdState &state) {
  state.logs.show_thread = !state.logs.show_thread;
  state.screen = ScreenMode::Logs;
  state.status =
      std::string("logs.thread=") + (state.logs.show_thread ? "on" : "off");
}

inline void toggle_logs_metadata(CmdState &state) {
  state.logs.show_metadata = !state.logs.show_metadata;
  state.screen = ScreenMode::Logs;
  state.status =
      std::string("logs.metadata=") + (state.logs.show_metadata ? "on" : "off");
}

inline void toggle_logs_color(CmdState &state) {
  state.logs.show_color = !state.logs.show_color;
  state.screen = ScreenMode::Logs;
  state.status =
      std::string("logs.color=") + (state.logs.show_color ? "on" : "off");
}

inline void toggle_logs_mouse_capture(CmdState &state) {
  state.logs.mouse_capture = !state.logs.mouse_capture;
  state.screen = ScreenMode::Logs;
  state.status = std::string("logs.mouse_capture=") +
                 (state.logs.mouse_capture ? "on" : "off");
}

inline bool adjust_selected_logs_setting(CmdState &state, int delta) {
  switch (state.logs.selected_setting) {
  case 0:
    cycle_logs_source_filter(state, delta);
    return true;
  case 1:
    cycle_logs_severity_filter(state, delta);
    return true;
  case 2:
    cycle_logs_metadata_filter(state, delta);
    return true;
  case 3:
    toggle_logs_timestamp(state);
    return true;
  case 4:
    toggle_logs_thread(state);
    return true;
  case 5:
    toggle_logs_metadata(state);
    return true;
  case 6:
    toggle_logs_color(state);
    return true;
  case 7:
    toggle_logs_follow(state);
    return true;
  case 8:
    toggle_logs_mouse_capture(state);
    return true;
  default:
    return false;
  }
}

inline void select_relative_logs_setting(CmdState &state, int delta) {
  const int size = static_cast<int>(logs_settings_count());
  int next = static_cast<int>(state.logs.selected_setting) + delta;
  if (next < 0)
    next = size - 1;
  if (next >= size)
    next = 0;
  state.logs.selected_setting = static_cast<std::size_t>(next);
  state.status =
      delta < 0 ? "logs.settings.cursor=prev" : "logs.settings.cursor=next";
}

inline void clamp_config_selection(ConfigState &config) {
  if (config.files.empty()) {
    config.selected_file = 0;
    return;
  }
  if (config.selected_file >= config.files.size())
    config.selected_file = config.files.size() - 1;
}

inline const ConfigFileSummary *selected_config_file(const CmdState &state) {
  if (state.config.files.empty() ||
      state.config.selected_file >= state.config.files.size())
    return nullptr;
  return &state.config.files[state.config.selected_file];
}

inline std::string selected_config_file_status(const CmdState &state) {
  return "selected file=" + std::to_string(state.config.selected_file + 1u);
}

inline void select_relative_config_file(CmdState &state, int delta) {
  if (state.config.files.empty()) {
    state.config.file_editor_open = false;
    state.status = "no config files";
    return;
  }
  state.config.selected_file = wrap_relative_index(
      state.config.selected_file, state.config.files.size(), delta);
  state.config.file_editor_open = false;
  state.status = selected_config_file_status(state);
}

inline void select_first_config_file(CmdState &state) {
  if (state.config.files.empty()) {
    state.config.selected_file = 0;
    state.config.file_editor_open = false;
    state.status = "no config files";
    return;
  }
  state.config.selected_file = 0;
  state.config.file_editor_open = false;
  state.status = selected_config_file_status(state);
}

inline void select_last_config_file(CmdState &state) {
  if (state.config.files.empty()) {
    state.config.selected_file = 0;
    state.config.file_editor_open = false;
    state.status = "no config files";
    return;
  }
  state.config.selected_file = state.config.files.size() - 1;
  state.config.file_editor_open = false;
  state.status = selected_config_file_status(state);
}

inline void clamp_lattice_selection(LatticeState &lattice) {
  if (lattice.target_ids.empty()) {
    lattice.selected_target = 0;
    return;
  }
  if (lattice.selected_target >= lattice.target_ids.size())
    lattice.selected_target = lattice.target_ids.size() - 1;
}

inline const std::string *selected_lattice_target_id(const CmdState &state) {
  if (state.lattice.target_ids.empty() ||
      state.lattice.selected_target >= state.lattice.target_ids.size())
    return nullptr;
  return &state.lattice.target_ids[state.lattice.selected_target];
}

inline void select_relative_lattice_target(CmdState &state, int delta) {
  if (state.lattice.target_ids.empty())
    return;
  state.lattice.selected_target = wrap_relative_index(
      state.lattice.selected_target, state.lattice.target_ids.size(), delta);
  state.status = "lattice target";
}

inline void select_first_lattice_target(CmdState &state) {
  state.lattice.selected_target = 0;
  state.status = "lattice target";
}

inline void select_last_lattice_target(CmdState &state) {
  state.lattice.selected_target = state.lattice.target_ids.empty()
                                      ? 0
                                      : state.lattice.target_ids.size() - 1;
  state.status = "lattice target";
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
