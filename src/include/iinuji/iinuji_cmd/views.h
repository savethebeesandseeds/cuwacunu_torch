#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/primitives/text_box.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string yes_no(bool value) { return value ? "yes" : "no"; }

inline std::string on_off(bool value) { return value ? "on" : "off"; }

inline std::string display_or(const std::string &value,
                              const std::string &fallback = "-") {
  return value.empty() ? fallback : value;
}

inline std::string mib(std::uint64_t kib) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(1)
      << (static_cast<double>(kib) / 1024.0) << " MiB";
  return out.str();
}

inline std::string ratio_mib(std::uint64_t used_mib, std::uint64_t total_mib) {
  std::ostringstream out;
  out << used_mib << "/" << total_mib << " MiB";
  return out.str();
}

inline std::string log_timestamp_text(std::uint64_t timestamp_ms) {
  const std::time_t seconds = static_cast<std::time_t>(timestamp_ms / 1000u);
  std::tm tm{};
  if (::localtime_r(&seconds, &tm) == nullptr)
    return std::to_string(timestamp_ms);
  std::array<char, 32> buffer{};
  if (std::strftime(buffer.data(), buffer.size(), "%H:%M:%S", &tm) == 0)
    return std::to_string(timestamp_ms);
  std::ostringstream out;
  out << buffer.data() << "." << std::setw(3) << std::setfill('0')
      << (timestamp_ms % 1000u);
  return out.str();
}

inline std::string pct_or_unknown(int pct) {
  return pct < 0 ? "n/a" : std::to_string(pct) + "%";
}

inline int pct_from_u64(std::uint64_t used, std::uint64_t total) {
  if (total == 0)
    return -1;
  return static_cast<int>((used * 100 + total / 2) / total);
}

inline void append_repeated(std::string &out, std::string_view token,
                            int count) {
  for (int i = 0; i < count; ++i)
    out += token;
}

inline std::string bar_pct(int pct, int width = 18) {
  if (pct < 0) {
    std::string out = "[";
    append_repeated(out, "░", width);
    out += "]";
    return out;
  }
  pct = std::clamp(pct, 0, 100);
  const int filled = (pct * width + 50) / 100;
  std::string out = "[";
  append_repeated(out, "█", filled);
  append_repeated(out, "░", width - filled);
  out += "]";
  return out;
}

inline char trend_char_for_pct(int pct) {
  if (pct < 0)
    return '?';
  if (pct < 10)
    return '.';
  if (pct < 35)
    return '-';
  if (pct < 60)
    return '=';
  if (pct < 85)
    return '+';
  return '#';
}

inline std::string trend_from_pcts(const std::vector<int> &values,
                                   std::size_t width = 32) {
  if (values.empty())
    return "n/a";
  const std::size_t start = values.size() > width ? values.size() - width : 0;
  std::string out{};
  out.reserve(values.size() - start);
  for (std::size_t i = start; i < values.size(); ++i)
    out.push_back(trend_char_for_pct(values[i]));
  return out;
}

inline int device_memory_pct(const RuntimeDeviceSnapshot &device) {
  if (device.mem_total_kib == 0)
    return -1;
  const std::uint64_t used =
      device.mem_total_kib > device.mem_available_kib
          ? device.mem_total_kib - device.mem_available_kib
          : 0;
  return pct_from_u64(used, device.mem_total_kib);
}

inline const RuntimeGpuSnapshot *
find_gpu_by_uuid(const RuntimeDeviceSnapshot &device, const std::string &uuid) {
  for (const auto &gpu : device.gpus) {
    if (gpu.uuid == uuid)
      return &gpu;
  }
  return nullptr;
}

inline std::vector<int>
host_cpu_history_values(const std::vector<RuntimeDeviceSnapshot> &history) {
  std::vector<int> values{};
  values.reserve(history.size());
  for (const auto &sample : history)
    values.push_back(sample.cpu_usage_pct);
  return values;
}

inline std::string
host_cpu_trend(const std::vector<RuntimeDeviceSnapshot> &history) {
  return trend_from_pcts(host_cpu_history_values(history));
}

inline std::vector<int>
host_memory_history_values(const std::vector<RuntimeDeviceSnapshot> &history) {
  std::vector<int> values{};
  values.reserve(history.size());
  for (const auto &sample : history)
    values.push_back(device_memory_pct(sample));
  return values;
}

inline std::string
host_memory_trend(const std::vector<RuntimeDeviceSnapshot> &history) {
  return trend_from_pcts(host_memory_history_values(history));
}

inline std::vector<int>
gpu_util_history_values(const std::vector<RuntimeDeviceSnapshot> &history,
                        const std::string &uuid) {
  std::vector<int> values{};
  values.reserve(history.size());
  for (const auto &sample : history) {
    if (const RuntimeGpuSnapshot *gpu = find_gpu_by_uuid(sample, uuid))
      values.push_back(gpu->utilization_gpu_pct);
  }
  return values;
}

inline std::string
gpu_util_trend(const std::vector<RuntimeDeviceSnapshot> &history,
               const std::string &uuid) {
  return trend_from_pcts(gpu_util_history_values(history, uuid));
}

inline std::vector<int>
gpu_memory_history_values(const std::vector<RuntimeDeviceSnapshot> &history,
                          const std::string &uuid) {
  std::vector<int> values{};
  values.reserve(history.size());
  for (const auto &sample : history) {
    if (const RuntimeGpuSnapshot *gpu = find_gpu_by_uuid(sample, uuid))
      values.push_back(
          pct_from_u64(gpu->memory_used_mib, gpu->memory_total_mib));
  }
  return values;
}

inline std::string
gpu_memory_trend(const std::vector<RuntimeDeviceSnapshot> &history,
                 const std::string &uuid) {
  return trend_from_pcts(gpu_memory_history_values(history, uuid));
}

inline std::string screen_tab_core_text(ScreenMode screen) {
  return screen_key_label(screen) + " " + screen_badge_label(screen);
}

inline std::string screen_tab_display_text(const CmdState &state,
                                           ScreenMode screen) {
  const std::string core = screen_tab_core_text(screen);
  if (state.screen == screen)
    return "[" + core + "]";
  return " " + core + " ";
}

inline std::string make_title_text(const CmdState &state) {
  std::ostringstream out;
  bool first = true;
  for (ScreenMode screen : screen_order()) {
    if (!first)
      out << " ";
    first = false;
    out << screen_tab_display_text(state, screen);
  }
  return out.str();
}

inline text_line_emphasis_t screen_tab_emphasis(const CmdState &state,
                                                ScreenMode screen) {
  if (state.screen == screen)
    return text_line_emphasis_t::Warning;
  if (screen == ScreenMode::Workbench)
    return text_line_emphasis_t::Info;
  return text_line_emphasis_t::None;
}

inline std::vector<styled_text_line_t>
make_tab_styled_lines(const CmdState &state) {
  std::vector<styled_text_segment_t> segments{};
  bool first = true;
  for (ScreenMode screen : screen_order()) {
    if (!first) {
      segments.push_back(styled_text_segment_t{
          .text = " ",
          .emphasis = text_line_emphasis_t::Debug,
      });
    }
    first = false;

    segments.push_back(styled_text_segment_t{
        .text = screen_tab_display_text(state, screen),
        .emphasis = screen_tab_emphasis(state, screen),
    });
  }
  return {make_segmented_styled_text_line(std::move(segments))};
}

inline std::string make_tab_text(const CmdState &state) {
  std::ostringstream out;
  bool first = true;
  for (ScreenMode screen : screen_order()) {
    if (!first)
      out << " ";
    first = false;
    out << screen_tab_display_text(state, screen);
  }
  return out.str();
}

inline std::string make_nav_current_tools_text(const CmdState &state) {
  std::ostringstream out;
  out << "\nCurrent\n";
  switch (state.screen) {
  case ScreenMode::Home:
    out << "  visual | waajacamaya\n";
    out << "  cuwacunu_cmd --visual / --home-visual\n";
    out << "  cuwacunu_cmd --image\n";
    out << "  cuwacunu_cmd --animation\n";
    out << "  cuwacunu_cmd --waajacamaya\n";
    out << "  splash | bootstrap\n";
    out << "  cuwacunu_cmd --splash\n";
    out << "  cuwacunu_cmd --splash loading\n";
    out << "  cuwacunu_cmd --bootstrap / --boot\n";
    out << "  farewell | closing\n";
    out << "  cuwacunu_cmd --farewell / --closing\n";
    out << "  cuwacunu_cmd --splash farewell\n";
    out << "  cuwacunu_cmd --splash close\n";
    out << "  cuwacunu_cmd --splash=farewell\n";
    out << "  cuwacunu_cmd --splash=good-luck\n";
    out << "  a / actions / palette / commands\n";
    break;
  case ScreenMode::Workbench:
    out << "  F1 home\n";
    out << "  F3 runtime\n";
    out << "  F4 lattice\n";
    out << "  F8 logs  F9 config\n";
    break;
  case ScreenMode::Runtime:
    out << "  focus " << runtime_focus_label(state.runtime.focus) << "\n";
    out << "  detail " << runtime_detail_mode_label(state.runtime.detail_mode)
        << "\n";
    out << "  Left/Right lanes\n";
    out << "  d devices  J jobs\n";
    out << "  k/j h/l rows\n";
    out << "  Enter menu  a actions\n";
    out << "  Tab/Shift-Tab detail\n";
    out << "  r refresh\n";
    out << "  f/z zoom  Esc split\n";
    break;
  case ScreenMode::Lattice:
    out << "  targets " << state.lattice.target_ids.size() << "\n";
    if (!state.lattice.target_ids.empty())
      out << "  row " << (state.lattice.selected_target + 1) << "/"
          << state.lattice.target_ids.size() << "\n";
    out << "  k/j h/l rows\n";
    out << "  Home/End edges\n";
    out << "  Enter show target\n";
    out << "  target n1/id\n";
    out << "  r refresh\n";
    out << "  f/z zoom  Esc split\n";
    break;
  case ScreenMode::Logs:
    out << "  source " << logs_source_filter_label(state.logs.source_filter)
        << "\n";
    out << "  level " << logs_severity_filter_label(state.logs.severity_filter)
        << "\n";
    out << "  s source  v level  m meta\n";
    out << "  t time    u thread e fields\n";
    out << "  o color   l follow p mouse\n";
    out << "  c clear   f/z zoom\n";
    out << "  Home/End logs\n";
    out << "  r refresh\n";
    out << "  Left/Right settings\n";
    out << "  Esc split\n";
    break;
  case ScreenMode::Config:
    out << "  files " << state.config.files.size() << "\n";
    if (!state.config.files.empty())
      out << "  row " << (state.config.selected_file + 1) << "/"
          << state.config.files.size() << "\n";
    out << "  k/j h/l rows\n";
    out << "  files | show file\n";
    out << "  Enter/e preview\n";
    out << "  Home/End edges\n";
    out << "  file n1/id\n";
    out << "  r reload\n";
    out << "  f/z zoom  Esc split\n";
    break;
  }
  return out.str();
}

inline bool lowercase_h_opens_help(ScreenMode screen) {
  return screen != ScreenMode::Runtime && screen != ScreenMode::Lattice &&
         screen != ScreenMode::Config;
}

inline std::string help_shortcut_label(ScreenMode screen) {
  return lowercase_h_opens_help(screen) ? "h/H / ? / help" : "H / ? / help";
}

inline std::string compact_help_shortcut_label(ScreenMode screen) {
  return lowercase_h_opens_help(screen) ? "h/H/? help" : "H/? help";
}

inline std::string nav_subtitle_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "home";
  case ScreenMode::Workbench:
    return "workbench";
  case ScreenMode::Runtime:
    return "runtime";
  case ScreenMode::Lattice:
    return "lattice";
  case ScreenMode::Logs:
    return "shell logs";
  case ScreenMode::Config:
    return "config";
  }
  return "home";
}

inline std::string make_nav_text(const CmdState &state) {
  std::ostringstream out;
  out << display_or(state.command_name, "cuwacunu_cmd") << "\n"
      << nav_subtitle_label(state.screen) << "\n\n";
  for (ScreenMode screen : screen_order()) {
    out << (state.screen == screen ? "> " : "  ") << std::left << std::setw(3)
        << screen_key_label(screen) << " " << screen_badge_label(screen)
        << "\n";
  }
  out << "\nMenu\n";
  out << "  " << help_shortcut_label(state.screen) << "\n";
  out << "  a / actions\n";
  out << "  type / : command\n";
  out << "  refresh | r\n";
  out << "  quit | exit | q | x\n";
  out << make_nav_current_tools_text(state);
  return out.str();
}

inline void append_snapshot_shell_chrome(std::ostringstream &out,
                                         const CmdState &state) {
  out << make_tab_text(state) << "\n";
  if (workspace_is_current_screen_zoomed(state))
    out << "workspace=full\n";
  out << "\n";
  out << make_nav_text(state) << "\n---\n";
}

inline const std::array<std::string_view, 5> &breathing_dot_glyphs() {
  static constexpr std::array<std::string_view, 5> glyphs{"●", "•", "·", "⋅",
                                                          "‧"};
  return glyphs;
}

inline std::string animated_rule(std::uint64_t tick, std::size_t width = 30) {
  (void)width;
  const auto &glyphs = breathing_dot_glyphs();
  return std::string(glyphs[static_cast<std::size_t>(tick % glyphs.size())]);
}

inline std::string make_screen_menu_text(ScreenMode screen) {
  std::ostringstream out;
  out << "Menu\n";
  switch (screen) {
  case ScreenMode::Home:
    out << "  F2 workbench  F3 runtime  F4 lattice\n";
    out << "  F8 shell logs  F9 config\n";
    out << "  a / actions  type/: command  "
        << compact_help_shortcut_label(screen) << "\n";
    out << "  r / refresh snapshots keeps runtime/config current\n";
    break;
  case ScreenMode::Workbench:
    out << "  F1 home  F3 runtime  F4 lattice\n";
    out << "  F8 logs  F9 config\n";
    out << "  a / actions  type/: command  "
        << compact_help_shortcut_label(screen) << "\n";
    out << "  r / refresh Workbench state\n";
    break;
  case ScreenMode::Runtime:
    out << "  Left/Right lanes  J jobs  d devices\n";
    out << "  Up/Down or k/j select  h/l row browse\n";
    out << "  Tab/Shift-Tab detail  f/z full  Esc split  "
        << compact_help_shortcut_label(screen) << "\n";
    out << "  a / actions  type/: command\n";
    out << "  PgUp/PgDn detail  r refresh\n";
    out << "  manifest/log/trace lenses follow selected rows\n";
    break;
  case ScreenMode::Lattice:
    out << "  targets and evidence facts\n";
    out << "  Up/Down or k/j select  h/l browse target\n";
    out << "  Home/End first-last  Enter show target\n";
    out << "  f/z full  Esc split  " << compact_help_shortcut_label(screen)
        << "\n";
    out << "  a / actions  type/: command\n";
    out << "  PgUp/PgDn proof  r refresh\n";
    out << "  read-only target proof preview\n";
    break;
  case ScreenMode::Logs:
    out << "  shell diagnostics and command feed\n";
    out << "  s source  v level  m meta  t time\n";
    out << "  u thread  e fields o color l follow p mouse\n";
    out << "  f/z full  Esc split\n";
    out << "  Left/Right settings\n";
    out << "  c clear  a / actions  type/: command  "
        << compact_help_shortcut_label(screen) << "\n";
    out << "  PgUp/PgDn stream  Home/End logs  r refresh\n";
    break;
  case ScreenMode::Config:
    out << "  managed config catalog\n";
    out << "  Up/Down or k/j select  h/l browse file\n";
    out << "  Home/End first-last  Enter/e preview\n";
    out << "  f/z full  Esc split  " << compact_help_shortcut_label(screen)
        << "\n";
    out << "  a / actions  type/: command\n";
    out << "  PgUp/PgDn preview  r reload\n";
    out << "  read-only managed Config file preview\n";
    break;
  }
  return out.str();
}

inline std::string runtime_detail_mode_selector(RuntimeDetailMode active) {
  static constexpr std::array<RuntimeDetailMode, 3> modes{
      RuntimeDetailMode::Manifest,
      RuntimeDetailMode::Log,
      RuntimeDetailMode::Trace,
  };
  std::ostringstream out;
  out << "Detail: ";
  for (std::size_t i = 0; i < modes.size(); ++i) {
    const RuntimeDetailMode mode = modes[i];
    if (i > 0)
      out << " ";
    if (active == mode)
      out << "[";
    out << runtime_detail_mode_label(mode);
    if (active == mode)
      out << "]";
  }
  return out.str();
}

inline std::string workspace_zoom_button_text(const CmdState &state) {
  return workspace_is_current_screen_zoomed(state) ? "[split]" : "[full]";
}

inline std::string workspace_zoom_state_text(const CmdState &state) {
  return workspace_is_current_screen_zoomed(state) ? "full" : "split";
}

inline std::string workspace_zoom_hint_text(const CmdState &state,
                                            std::string_view panel_label) {
  if (!workspace_screen_supports_zoom(state.screen))
    return {};
  std::ostringstream out;
  out << "Zoom: " << workspace_zoom_state_text(state) << " -> "
      << workspace_zoom_button_text(state) << " " << panel_label
      << "  f/z  Esc split  path=iinuji.workspace.zoom.toggle()";
  return out.str();
}

inline std::string home_visual_mode_text(const CmdState &state) {
  if (state.home_visual.presentation == HomePresentationMode::BootstrapSplash)
    return "visual=bootstrap-splash";
  if (state.home_visual.presentation == HomePresentationMode::FarewellSplash)
    return "visual=farewell-splash";
  if (state.home_visual.animation_ok) {
    std::ostringstream out;
    out << "visual="
        << (state.home_visual.animation_dynamic ? "animated" : "static");
    if (state.home_visual.animation_frames > 0u)
      out << " frames=" << state.home_visual.animation_frames;
    return out.str();
  }
  if (state.home_visual.loading_logo_ok)
    return "visual=loading-logo";
  return "visual=wordmark";
}

inline std::string detail_section_rule(std::size_t width = 56u) {
  return std::string(width, '-');
}

inline void append_detail_section_header(std::ostringstream &out,
                                         std::string_view title) {
  out << detail_section_rule() << "\n"
      << title << "\n"
      << detail_section_rule() << "\n";
}

inline std::string make_home_showcase_text() {
  std::ostringstream out;
  out << "  image panel: waajacamaya + Cuwacunu wordmark\n";
  out << "  animation: APNG frames on interactive F1 Home\n";
  out << "  splash: loading and closing logos from [GUI]\n";
  out << "  config keys: iinuji_loading_logo_path, iinuji_closing_logo_path, "
         "iinuji_home_animation_path\n";
  return out.str();
}

inline std::string home_shortcut_display_label(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "Home";
  case ScreenMode::Workbench:
    return "Workbench";
  case ScreenMode::Runtime:
    return "Runtime";
  case ScreenMode::Lattice:
    return "Lattice";
  case ScreenMode::Logs:
    return "Shell Logs";
  case ScreenMode::Config:
    return "Config";
  }
  return screen_badge_label(screen);
}

inline std::string make_home_text(const CmdState &state) {
  std::ostringstream out;
  (void)state;
  for (ScreenMode screen : screen_order()) {
    out << std::left << std::setw(5) << screen_key_label(screen)
        << home_shortcut_display_label(screen) << "\n";
  }
  out << "\n";
  out << "  h/H / ? / help  menu overlay\n";
  out << "  a / actions     action palette\n";
  out << "  r / refresh     refresh snapshots\n";
  out << "  type / :        command mode\n\n";
  out << "waajacu™ | waajacu.com\n";
  return out.str();
}

inline std::string
home_visual_asset_path_text(const std::filesystem::path &path) {
  return path.empty() ? "-" : path.string();
}

inline std::string home_visual_load_status_text(bool ok,
                                                const std::string &error) {
  if (ok)
    return "loaded";
  if (error.empty())
    return "unavailable";
  return "unavailable error=" + error;
}

inline void append_home_visual_asset_detail(std::ostringstream &out,
                                            const CmdState &state) {
  out << "  loading logo: "
      << home_visual_asset_path_text(state.home_visual.loading_logo_path)
      << " ["
      << home_visual_load_status_text(state.home_visual.loading_logo_ok,
                                      state.home_visual.loading_logo_error)
      << "]\n";
  out << "  closing logo: "
      << home_visual_asset_path_text(state.home_visual.closing_logo_path)
      << "\n";
  out << "  home animation: "
      << home_visual_asset_path_text(state.home_visual.home_animation_path)
      << " ["
      << home_visual_load_status_text(state.home_visual.animation_ok,
                                      state.home_visual.home_animation_error);
  if (state.home_visual.animation_ok) {
    out << " frames=" << state.home_visual.animation_frames
        << " dynamic=" << yes_no(state.home_visual.animation_dynamic);
    if (state.home_visual.animation_width > 0 &&
        state.home_visual.animation_height > 0) {
      out << " size=" << state.home_visual.animation_width << "x"
          << state.home_visual.animation_height;
    }
  }
  out << "]\n";
}

inline std::string make_home_detail_body_text(const CmdState &state) {
  const auto &policy = state.runtime.policy;
  const auto &wave = state.runtime.wave;
  std::ostringstream out;
  append_detail_section_header(out, "Home showcase");
  out << make_home_showcase_text() << "\n";

  append_detail_section_header(out, "Home visual");
  out << "Home visual: " << home_visual_mode_text(state) << "\n";
  append_detail_section_header(out, "Home assets");
  append_home_visual_asset_detail(out, state);

  append_detail_section_header(out, "Config");
  out << "  root: " << state.config.root.string() << "\n";
  out << "  files: " << state.config.files.size() << "\n\n";

  append_detail_section_header(out, "Runtime");
  out << "  exec: " << policy.runtime_exec_path.string() << "\n";
  out << "  exec exists: " << yes_no(policy.runtime_exec_exists)
      << " executable: " << yes_no(policy.runtime_exec_executable) << "\n";
  out << "  root: " << policy.runtime_root.string() << "\n";
  out << "  dry_run: " << display_or(policy.default_dry_run)
      << " execute: " << display_or(policy.allow_execute)
      << " train: " << display_or(policy.allow_train_execute) << "\n";
  out << "  jobs: " << state.runtime.jobs.size() << "\n\n";

  append_detail_section_header(out, "Active wave");
  out << "  " << display_or(wave.wave_id) << "\n";
  out << "  target=" << display_or(wave.target)
      << " mode=" << display_or(wave.mode) << "\n";
  out << "  range=" << display_or(wave.source_range) << " anchors=["
      << display_or(wave.anchor_index_begin) << ", "
      << display_or(wave.anchor_index_end) << ")\n\n";

  append_detail_section_header(out, "Lattice");
  out << "  targets: " << state.lattice.target_ids.size()
      << " exposure_facts: " << state.lattice.exposure_fact_count
      << " checkpoints: " << state.lattice.checkpoint_fact_count << "\n\n";
  out << "status: " << state.status << "\n";
  return out.str();
}

inline std::string make_device_text(const RuntimeState &runtime) {
  const RuntimeDeviceSnapshot &device = runtime.device;
  std::ostringstream out;
  out << (runtime.focus == RuntimeFocus::Devices ? "> " : "  ") << "Devices";
  out << "  host + " << device.gpus.size() << " gpu(s)\n";
  out << (runtime.focus == RuntimeFocus::Devices &&
                  runtime.selected_device_row == 0
              ? "  >> "
              : "    ");
  out << "host " << display_or(device.host_name)
      << " cpu=" << pct_or_unknown(device.cpu_usage_pct)
      << " ram=" << pct_or_unknown(device_memory_pct(device))
      << " load=" << std::fixed << std::setprecision(2) << device.load1
      << " cpus=" << device.cpu_count
      << " samples=" << runtime.device_history.size() << "\n";
  if (device.gpus.empty()) {
    out << "    gpu: " << display_or(device.gpu_error, "none") << "\n";
  } else {
    for (std::size_t i = 0; i < device.gpus.size(); ++i) {
      const auto &gpu = device.gpus[i];
      const int mem_pct =
          pct_from_u64(gpu.memory_used_mib, gpu.memory_total_mib);
      const std::size_t process_count = static_cast<std::size_t>(std::count_if(
          device.gpu_processes.begin(), device.gpu_processes.end(),
          [&](const RuntimeGpuProcessSnapshot &process) {
            return process.gpu_uuid == gpu.uuid;
          }));
      out << (runtime.focus == RuntimeFocus::Devices &&
                      runtime.selected_device_row == i + 1
                  ? "  >> "
                  : "    ");
      out << "gpu " << display_or(gpu.index) << " " << display_or(gpu.name)
          << " util=" << pct_or_unknown(gpu.utilization_gpu_pct)
          << " vram=" << pct_or_unknown(mem_pct) << " temp="
          << (gpu.temperature_c < 0 ? "n/a"
                                    : std::to_string(gpu.temperature_c) + "C")
          << " procs=" << process_count << "\n";
    }
  }
  return out.str();
}

inline std::string make_runtime_nav_text(const CmdState &state) {
  std::ostringstream out;
  out << "Runtime\n\n";
  out << "Focus: "
      << (state.runtime.focus == RuntimeFocus::Devices ? "[Devices]"
                                                       : "Devices")
      << " " << (state.runtime.focus == RuntimeFocus::Jobs ? "[Jobs]" : "Jobs")
      << "\n";
  if (state.runtime.focus == RuntimeFocus::Devices) {
    out << runtime_detail_mode_selector(state.runtime.detail_mode) << "\n";
  } else {
    out << "Details: selected job summary; Enter opens files\n";
  }
  out << "Active wave: " << display_or(state.runtime.wave.wave_id)
      << " mode=" << display_or(state.runtime.wave.mode) << "\n\n";
  out << make_screen_menu_text(ScreenMode::Runtime);
  return out.str();
}

inline std::string runtime_job_badge(std::string_view value,
                                     std::size_t width) {
  std::string text{value.empty() ? std::string_view{"-"} : value};
  if (text.size() > width) {
    if (width == 0u) {
      text.clear();
    } else if (width == 1u) {
      text = "~";
    } else {
      text.resize(width - 1u);
      text.push_back('~');
    }
  }

  std::ostringstream out;
  out << "[" << std::left << std::setw(static_cast<int>(width)) << text
      << std::right << "]";
  return out.str();
}

inline std::string
runtime_job_kind_or_target_text(const RuntimeJobSummary &job) {
  if (!job.job_kind.empty())
    return job.job_kind;
  if (!job.target_component_family_id.empty())
    return job.target_component_family_id;
  if (!job.wave_id.empty())
    return job.wave_id;
  return job.job_id;
}

inline std::string make_runtime_worklist_text(const CmdState &state) {
  std::ostringstream out;
  out << "Worklist\n";
  out << make_device_text(state.runtime);
  out << "\n"
      << (state.runtime.focus == RuntimeFocus::Jobs ? "> " : "  ") << "Jobs\n";
  if (state.runtime.jobs.empty()) {
    out << "    no runtime jobs found\n";
  } else {
    for (std::size_t i = 0; i < state.runtime.jobs.size(); ++i) {
      const auto &job = state.runtime.jobs[i];
      out << (state.runtime.focus == RuntimeFocus::Jobs &&
                      i == state.runtime.selected_job
                  ? "  >> "
                  : "    ")
          << runtime_job_badge(job.status, 9) << " "
          << runtime_job_badge(job.wave_action, 6) << " "
          << runtime_job_badge(runtime_job_kind_or_target_text(job), 29) << " "
          << display_or(job.job_id) << "\n";
    }
  }
  return out.str();
}

inline std::string make_runtime_text(const CmdState &state) {
  return make_runtime_nav_text(state) + make_runtime_worklist_text(state);
}

inline std::string make_workbench_text() { return {}; }

inline bool lattice_target_id_starts_with(std::string_view target_id,
                                          std::string_view prefix) {
  return target_id.size() >= prefix.size() &&
         target_id.substr(0, prefix.size()) == prefix;
}

inline bool lattice_target_id_contains(std::string_view target_id,
                                       std::string_view needle) {
  return target_id.find(needle) != std::string_view::npos;
}

inline std::string_view lattice_target_family_id(std::string_view target_id) {
  if (lattice_target_id_starts_with(target_id, "cwu_01v"))
    return "cwu_01v";
  if (lattice_target_id_starts_with(target_id, "cwu_02v"))
    return "cwu_02v";
  if (lattice_target_id_contains(target_id, "channel_mdn") ||
      lattice_target_id_contains(target_id, "_mdn_"))
    return "channel_mdn";
  if (lattice_target_id_contains(target_id, "mtf_jepa"))
    return "mtf_jepa";
  if (lattice_target_id_contains(target_id, "vicreg"))
    return "vicreg";
  return "other";
}

inline std::string_view lattice_target_family_title(std::string_view id) {
  if (id == "channel_mdn")
    return "channel_mdn";
  if (id == "vicreg")
    return "vicreg";
  if (id == "mtf_jepa")
    return "mtf_jepa";
  if (id == "cwu_01v")
    return "cwu_01v";
  if (id == "cwu_02v")
    return "cwu_02v";
  return "other";
}

inline std::string lattice_target_family_badge(std::string_view id) {
  std::ostringstream out;
  out << "[" << std::left << std::setw(11) << lattice_target_family_title(id)
      << std::right << "]";
  return out.str();
}

inline const std::array<std::string_view, 6> &lattice_target_family_order() {
  static const std::array<std::string_view, 6> order{
      "channel_mdn", "vicreg", "mtf_jepa", "cwu_01v", "cwu_02v", "other",
  };
  return order;
}

inline std::size_t lattice_target_family_count(const CmdState &state,
                                               std::string_view id) {
  return static_cast<std::size_t>(std::count_if(
      state.lattice.target_ids.begin(), state.lattice.target_ids.end(),
      [&](const std::string &target_id) {
        return lattice_target_family_id(target_id) == id;
      }));
}

inline std::string_view
selected_lattice_target_family_id(const CmdState &state) {
  const std::string *target_id = selected_lattice_target_id(state);
  return target_id == nullptr ? std::string_view{"other"}
                              : lattice_target_family_id(*target_id);
}

inline std::string make_lattice_family_summary_text(const CmdState &state) {
  std::ostringstream out;
  out << "Families\n";
  if (state.lattice.target_ids.empty()) {
    out << "  none\n";
    return out.str();
  }
  const std::string_view selected_id = selected_lattice_target_family_id(state);
  for (const std::string_view id : lattice_target_family_order()) {
    const std::size_t count = lattice_target_family_count(state, id);
    if (count == 0u)
      continue;
    out << (id == selected_id ? "> " : "  ") << lattice_target_family_badge(id)
        << " " << count << " target";
    if (count != 1u)
      out << "s";
    out << "\n";
  }
  return out.str();
}

inline std::string make_lattice_nav_text(const CmdState &state) {
  std::ostringstream out;
  out << "Lattice\n\n";
  out << "Configured targets: " << state.lattice.target_ids.size();
  if (!state.lattice.target_ids.empty()) {
    out << "  selected=" << (state.lattice.selected_target + 1) << "/"
        << state.lattice.target_ids.size();
  }
  out << "\n";
  if (state.lattice.exposure_fact_count == 0u &&
      state.lattice.checkpoint_fact_count == 0u) {
    out << "Runtime evidence: empty after dev_nuke (exposure=0 checkpoints=0)"
        << "\n";
  } else {
    out << "Runtime evidence: exposure=" << state.lattice.exposure_fact_count
        << " checkpoints=" << state.lattice.checkpoint_fact_count << "\n";
  }
  out << "Runtime root: " << state.lattice.runtime_root.string() << "\n\n";
  out << make_lattice_family_summary_text(state) << "\n";
  return out.str();
}

inline std::string make_lattice_worklist_text(const CmdState &state) {
  std::ostringstream out;
  out << "Worklist\n";
  if (state.lattice.target_ids.empty()) {
    out << "no lattice targets found\n";
  } else {
    const std::size_t index_width = std::max<std::size_t>(
        2, std::to_string(state.lattice.target_ids.size()).size());
    for (std::size_t i = 0; i < state.lattice.target_ids.size(); ++i) {
      const bool selected = i == state.lattice.selected_target;
      out << (selected ? "> " : "  ") << "["
          << std::setw(static_cast<int>(index_width)) << (i + 1) << "] "
          << lattice_target_family_badge(
                 lattice_target_family_id(state.lattice.target_ids[i]))
          << " " << state.lattice.target_ids[i] << "\n";
    }
  }
  out << "\n" << make_screen_menu_text(ScreenMode::Lattice);
  return out.str();
}

inline std::string make_lattice_text(const CmdState &state) {
  return make_lattice_nav_text(state) + make_lattice_worklist_text(state);
}

inline std::string lattice_target_value(std::string text) {
  std::size_t first = 0u;
  while (first < text.size() &&
         (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) {
    ++first;
  }
  std::size_t last = text.size();
  while (last > first && (text[last - 1u] == ' ' || text[last - 1u] == '\t' ||
                          text[last - 1u] == '\r' || text[last - 1u] == ';')) {
    --last;
  }
  return text.substr(first, last - first);
}

inline std::string truncate_lattice_preview_line(std::string line,
                                                 std::size_t width = 96) {
  if (line.size() <= width)
    return line;
  if (width == 0u)
    return {};
  if (width == 1u)
    return "~";
  line.resize(width - 1u);
  line.push_back('~');
  return line;
}

inline std::vector<std::string>
read_lattice_target_block(const std::filesystem::path &path,
                          const std::string &target_id) {
  std::vector<std::string> block{};
  if (target_id.empty())
    return block;

  std::ifstream in(path);
  if (!in)
    return block;

  std::vector<std::string> candidate{};
  std::string line;
  bool in_target = false;
  bool matches = false;
  while (std::getline(in, line)) {
    if (!in_target && line.find("LATTICE_TARGET") == std::string::npos)
      continue;
    if (!in_target) {
      in_target = true;
      matches = false;
      candidate.clear();
    }
    candidate.push_back(line);
    if (line.find("TARGET_ID") != std::string::npos) {
      const std::size_t eq = line.find('=');
      if (eq != std::string::npos)
        matches = lattice_target_value(line.substr(eq + 1)) == target_id;
    }
    if (line.find('}') != std::string::npos) {
      if (matches)
        return candidate;
      in_target = false;
      matches = false;
      candidate.clear();
    }
  }
  return {};
}

inline std::string
lattice_target_block_field(const std::vector<std::string> &lines,
                           std::string_view key) {
  for (const std::string &line : lines) {
    std::size_t first = 0u;
    while (first < line.size() &&
           (line[first] == ' ' || line[first] == '\t' || line[first] == '\r'))
      ++first;
    const std::size_t eq = line.find('=', first);
    if (eq == std::string::npos)
      continue;
    std::string_view lhs{line.data() + first, eq - first};
    while (!lhs.empty() && (lhs.back() == ' ' || lhs.back() == '\t'))
      lhs.remove_suffix(1u);
    if (lhs != key)
      continue;
    return lattice_target_value(line.substr(eq + 1u));
  }
  return {};
}

inline void append_lattice_target_summary_line(std::ostringstream &out,
                                               std::string_view label,
                                               std::string value) {
  out << "  " << label << ": " << display_or(value) << "\n";
}

inline std::string make_lattice_detail_text(const CmdState &state) {
  std::ostringstream out;
  out << "selected lattice target\n\n";
  out << "targets file: " << state.lattice.targets_path.string() << "\n";
  out << "runtime root: " << state.lattice.runtime_root.string() << "\n";
  if (state.lattice.exposure_fact_count == 0u &&
      state.lattice.checkpoint_fact_count == 0u) {
    out << "runtime evidence: empty after dev_nuke\n";
  }
  out << "exposure facts: " << state.lattice.exposure_fact_count << "\n";
  out << "checkpoint facts: " << state.lattice.checkpoint_fact_count << "\n\n";

  const std::string *target_id = selected_lattice_target_id(state);
  if (target_id == nullptr) {
    out << "no lattice target selected\n";
    return out.str();
  }

  out << "target: " << *target_id << "\n";
  out << "index: " << (state.lattice.selected_target + 1) << "/"
      << state.lattice.target_ids.size() << "\n\n";

  const auto lines =
      read_lattice_target_block(state.lattice.targets_path, *target_id);
  append_detail_section_header(out, "target summary");
  append_lattice_target_summary_line(
      out, "family",
      std::string(
          lattice_target_family_title(lattice_target_family_id(*target_id))));
  append_lattice_target_summary_line(
      out, "class", lattice_target_block_field(lines, "TARGET_CLASS"));
  append_lattice_target_summary_line(
      out, "kind", lattice_target_block_field(lines, "TARGET_KIND"));
  append_lattice_target_summary_line(
      out, "component", lattice_target_block_field(lines, "SUBJECT_COMPONENT"));
  append_lattice_target_summary_line(
      out, "source", lattice_target_block_field(lines, "CHECKPOINT_SOURCE"));
  append_lattice_target_summary_line(
      out, "range", lattice_target_block_field(lines, "SOURCE_RANGE"));
  append_lattice_target_summary_line(
      out, "split", lattice_target_block_field(lines, "PROTECT_SPLIT"));
  append_lattice_target_summary_line(
      out, "wave", lattice_target_block_field(lines, "WAVE_MODE"));
  out << "\n";

  append_detail_section_header(out, "target block");
  if (lines.empty()) {
    out << "  unavailable\n";
    return out.str();
  }
  constexpr std::size_t kMaxTargetBlockLines = 36;
  for (std::size_t i = 0; i < lines.size() && i < kMaxTargetBlockLines; ++i) {
    out << std::setw(3) << (i + 1) << " | "
        << truncate_lattice_preview_line(lines[i], 96) << "\n";
  }
  if (lines.size() > kMaxTargetBlockLines)
    out << "  ...\n";
  return out.str();
}

inline std::string config_file_extension(const ConfigFileSummary &file) {
  const std::string extension = file.path.extension().string();
  return extension.empty() ? "-" : extension;
}

inline bool config_relative_path_starts_with(std::string_view path,
                                             std::string_view prefix) {
  return path.size() >= prefix.size() &&
         path.substr(0, prefix.size()) == prefix;
}

inline std::string_view
config_catalog_family_id(const ConfigFileSummary &file) {
  const std::string_view path{file.relative_path};
  if (path == ".config" || path == "README.md")
    return "global";
  if (config_relative_path_starts_with(path, "grammar/"))
    return "grammar";
  if (config_relative_path_starts_with(path, "man/"))
    return "manuals";
  if (config_relative_path_starts_with(path, "hero."))
    return "hero";
  if (config_relative_path_starts_with(path, "kikijyeba."))
    return "kikijyeba";
  if (config_relative_path_starts_with(path, "ujcamei."))
    return "ujcamei";
  if (config_relative_path_starts_with(path, "wikimyei."))
    return "wikimyei";
  return "other";
}

inline std::string_view config_catalog_family_title(std::string_view id) {
  if (id == "global")
    return "global";
  if (id == "grammar")
    return "grammar";
  if (id == "manuals")
    return "manuals";
  if (id == "hero")
    return "hero";
  if (id == "kikijyeba")
    return "kikijyeba";
  if (id == "ujcamei")
    return "ujcamei";
  if (id == "wikimyei")
    return "wikimyei";
  return "other";
}

inline std::string config_catalog_family_badge(std::string_view id) {
  std::ostringstream out;
  out << "[" << std::left << std::setw(9) << config_catalog_family_title(id)
      << std::right << "]";
  return out.str();
}

inline const std::array<std::string_view, 8> &config_catalog_family_order() {
  static const std::array<std::string_view, 8> order{
      "global",  "hero",     "grammar", "kikijyeba",
      "ujcamei", "wikimyei", "manuals", "other",
  };
  return order;
}

inline std::size_t config_catalog_family_count(const CmdState &state,
                                               std::string_view id) {
  return static_cast<std::size_t>(
      std::count_if(state.config.files.begin(), state.config.files.end(),
                    [&](const ConfigFileSummary &file) {
                      return config_catalog_family_id(file) == id;
                    }));
}

inline std::string_view
selected_config_catalog_family_id(const CmdState &state) {
  const ConfigFileSummary *file = selected_config_file(state);
  return file == nullptr ? std::string_view{"other"}
                         : config_catalog_family_id(*file);
}

inline std::string make_config_family_summary_text(const CmdState &state) {
  std::ostringstream out;
  out << "Families\n";
  if (state.config.files.empty()) {
    out << "  none\n";
    return out.str();
  }
  const std::string_view selected_id = selected_config_catalog_family_id(state);
  for (const std::string_view id : config_catalog_family_order()) {
    const std::size_t count = config_catalog_family_count(state, id);
    if (count == 0u)
      continue;
    out << (id == selected_id ? "> " : "  ") << config_catalog_family_badge(id)
        << " " << count << " file";
    if (count != 1u)
      out << "s";
    out << "\n";
  }
  return out.str();
}

inline std::string truncate_config_preview_line(std::string line,
                                                std::size_t width = 96) {
  if (line.size() <= width)
    return line;
  if (width == 0u)
    return {};
  if (width == 1u)
    return "~";
  line.resize(width - 1u);
  line.push_back('~');
  return line;
}

inline std::string make_config_nav_text(const CmdState &state) {
  std::ostringstream out;
  out << "Config\n\n";
  out << "Root: " << state.config.root.string() << "\n";
  out << "Files: " << state.config.files.size();
  if (!state.config.files.empty()) {
    out << "  selected=" << (state.config.selected_file + 1) << "/"
        << state.config.files.size();
  }
  out << "\n\n";
  out << make_config_family_summary_text(state) << "\n";
  return out.str();
}

inline std::string make_config_worklist_text(const CmdState &state) {
  std::ostringstream out;
  out << "Worklist\n";
  if (state.config.files.empty()) {
    out << "no config files found\n";
  } else {
    const std::size_t index_width = std::max<std::size_t>(
        2, std::to_string(state.config.files.size()).size());
    for (std::size_t i = 0; i < state.config.files.size(); ++i) {
      const auto &file = state.config.files[i];
      const bool selected = i == state.config.selected_file;
      out << (selected ? "> " : "  ") << "["
          << std::setw(static_cast<int>(index_width)) << (i + 1) << "] "
          << config_catalog_family_badge(config_catalog_family_id(file)) << " "
          << std::setw(7) << file.size << "  " << std::left << std::setw(7)
          << config_file_extension(file) << " " << file.relative_path
          << std::right << "\n";
    }
  }
  out << "\n" << make_screen_menu_text(ScreenMode::Config);
  return out.str();
}

inline std::string make_config_text(const CmdState &state) {
  return make_config_nav_text(state) + make_config_worklist_text(state);
}

inline std::string make_config_detail_text(const CmdState &state) {
  std::ostringstream out;
  out << "selected config file\n\n";

  const ConfigFileSummary *file = selected_config_file(state);
  if (file == nullptr) {
    out << "no config file selected\n";
    out << "root: " << state.config.root.string() << "\n";
    return out.str();
  }

  out << "index: " << (state.config.selected_file + 1) << "/"
      << state.config.files.size() << "\n";
  out << "relative: " << file->relative_path << "\n";
  out << "path: " << file->path.string() << "\n";
  out << "size: " << file->size << " bytes\n";
  out << "extension: " << config_file_extension(*file) << "\n\n";

  append_detail_section_header(out, "preview");
  std::ifstream in(file->path);
  if (!in) {
    out << "  unavailable\n";
    return out.str();
  }

  constexpr std::size_t kMaxPreviewLines = 22;
  constexpr std::size_t kMaxPreviewBytes = 8192;
  std::string line;
  std::size_t line_count = 0;
  std::size_t byte_count = 0;
  while (line_count < kMaxPreviewLines && byte_count < kMaxPreviewBytes &&
         std::getline(in, line)) {
    byte_count += line.size() + 1u;
    out << std::setw(3) << (line_count + 1) << " | "
        << truncate_config_preview_line(line) << "\n";
    ++line_count;
  }
  if (line_count == 0) {
    out << "  empty\n";
  } else if (in.good()) {
    out << "  ...\n";
  }
  return out.str();
}

inline LogsSeverityFilter log_entry_severity(const LogEntry &entry) {
  const std::string text = entry.source + " " + entry.message;
  const std::string lowered = [&] {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
      return static_cast<char>(std::tolower(ch));
    });
    return out;
  }();
  if (lowered.find("fatal") != std::string::npos)
    return LogsSeverityFilter::FatalOnly;
  if (lowered.find("error") != std::string::npos ||
      lowered.find("failed") != std::string::npos ||
      lowered.find("missing") != std::string::npos ||
      lowered.find("unavailable") != std::string::npos) {
    return LogsSeverityFilter::ErrorOrHigher;
  }
  if (lowered.find("warn") != std::string::npos ||
      lowered.find("unknown") != std::string::npos ||
      lowered.find("reserved") != std::string::npos) {
    return LogsSeverityFilter::WarningOrHigher;
  }
  if (entry.source == "action")
    return LogsSeverityFilter::DebugOrHigher;
  if (entry.source == "refresh" || entry.source == "command" ||
      entry.source == "show" || entry.source == "status") {
    return LogsSeverityFilter::InfoOrHigher;
  }
  return LogsSeverityFilter::DebugOrHigher;
}

inline std::string log_entry_lowered_text(const LogEntry &entry) {
  std::string text = entry.source + " " + entry.message;
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

inline bool log_entry_has_path_metadata(const LogEntry &entry) {
  const std::string lowered = log_entry_lowered_text(entry);
  return lowered.find('/') != std::string::npos ||
         lowered.find('\\') != std::string::npos ||
         lowered.find(".cpp") != std::string::npos ||
         lowered.find(".h") != std::string::npos ||
         lowered.find(".dsl") != std::string::npos ||
         lowered.find(".config") != std::string::npos ||
         lowered.find("path=") != std::string::npos ||
         lowered.find("file=") != std::string::npos;
}

inline bool log_entry_has_function_metadata(const LogEntry &entry) {
  return entry.source == "action" || entry.source == "refresh";
}

inline bool log_entry_has_callsite_metadata(const LogEntry &entry) {
  return entry.source == "action" || entry.source == "command";
}

inline bool log_entry_has_any_metadata(const LogEntry &entry) {
  return !entry.source.empty() || entry.timestamp_ms != 0 ||
         log_entry_has_function_metadata(entry) ||
         log_entry_has_path_metadata(entry) ||
         log_entry_has_callsite_metadata(entry);
}

inline bool logs_metadata_filter_accepts(LogsMetadataFilter filter,
                                         const LogEntry &entry) {
  switch (filter) {
  case LogsMetadataFilter::Any:
    return true;
  case LogsMetadataFilter::WithAnyMetadata:
    return log_entry_has_any_metadata(entry);
  case LogsMetadataFilter::WithFunction:
    return log_entry_has_function_metadata(entry);
  case LogsMetadataFilter::WithPath:
    return log_entry_has_path_metadata(entry);
  case LogsMetadataFilter::WithCallsite:
    return log_entry_has_callsite_metadata(entry);
  }
  return true;
}

inline bool logs_accept_entry(const CmdState &state, const LogEntry &entry) {
  return logs_source_filter_accepts(state.logs.source_filter, entry.source) &&
         static_cast<unsigned>(log_entry_severity(entry)) >=
             static_cast<unsigned>(state.logs.severity_filter) &&
         logs_metadata_filter_accepts(state.logs.metadata_filter, entry);
}

inline std::string log_entry_metadata_text(const LogEntry &entry) {
  std::ostringstream out;
  out << " {sev=" << logs_severity_filter_label(log_entry_severity(entry))
      << " src=" << display_or(entry.source, "none")
      << " tick=" << entry.timestamp_ms;
  if (log_entry_has_function_metadata(entry))
    out << " fn=" << entry.source;
  if (log_entry_has_path_metadata(entry))
    out << " path=seen";
  if (log_entry_has_callsite_metadata(entry))
    out << " callsite=" << entry.source;
  out << "}";
  return out.str();
}

inline std::string format_log_entry(const CmdState &state,
                                    const LogEntry &entry) {
  std::ostringstream out;
  if (state.logs.show_timestamp)
    out << log_timestamp_text(entry.timestamp_ms) << " ";
  out << "[" << entry.source << "] ";
  if (state.logs.show_thread)
    out << "[main] ";
  out << entry.message;
  if (state.logs.show_metadata && log_entry_has_any_metadata(entry))
    out << log_entry_metadata_text(entry);
  return out.str();
}

inline std::string logs_scroll_rail_text(const CmdState &state,
                                         std::size_t visible_count) {
  std::ostringstream out;
  out << "scroll y=";
  if (state.logs.scroll_y == std::numeric_limits<int>::max())
    out << "end";
  else
    out << state.logs.scroll_y;
  out << "  visible=" << visible_count << "/" << state.log.size();
  out << "  zoom=" << workspace_zoom_state_text(state) << "->"
      << workspace_zoom_button_text(state);
  return out.str();
}

inline std::string make_logs_text(const CmdState &state) {
  std::ostringstream out;
  out << "Logs\n\n";
  out << "filter=" << logs_source_filter_label(state.logs.source_filter)
      << " level=" << logs_severity_filter_label(state.logs.severity_filter)
      << " meta=" << logs_metadata_filter_label(state.logs.metadata_filter)
      << " timestamp=" << on_off(state.logs.show_timestamp)
      << " thread=" << on_off(state.logs.show_thread)
      << " metadata=" << on_off(state.logs.show_metadata)
      << " color=" << on_off(state.logs.show_color)
      << " follow=" << on_off(state.logs.auto_follow)
      << " mouse=" << (state.logs.mouse_capture ? "capture" : "select/copy")
      << "\n\n";
  std::vector<const LogEntry *> visible{};
  visible.reserve(state.log.size());
  for (const auto &entry : state.log) {
    if (logs_accept_entry(state, entry))
      visible.push_back(&entry);
  }
  out << logs_scroll_rail_text(state, visible.size()) << "\n\n";
  const std::size_t start = visible.size() > 160 ? visible.size() - 160 : 0;
  for (std::size_t i = start; i < visible.size(); ++i) {
    out << format_log_entry(state, *visible[i]) << "\n";
  }
  if (visible.empty())
    out << "no visible log entries\n";
  out << "\n" << make_screen_menu_text(ScreenMode::Logs);
  return out.str();
}

inline const RuntimeJobSummary *selected_job(const CmdState &state) {
  if (state.runtime.jobs.empty() ||
      state.runtime.selected_job >= state.runtime.jobs.size())
    return nullptr;
  return &state.runtime.jobs[state.runtime.selected_job];
}

inline std::filesystem::path
runtime_resolved_artifact_path(const RuntimeJobSummary &job,
                               const std::string &raw_path) {
  if (raw_path.empty())
    return {};
  std::filesystem::path path{raw_path};
  if (path.is_absolute())
    return path;
  return job.job_dir / path;
}

inline std::string runtime_artifact_status(const std::filesystem::path &path) {
  if (path.empty())
    return "not recorded";
  std::error_code ec;
  if (!std::filesystem::exists(path, ec))
    return "missing";
  if (std::filesystem::is_directory(path, ec))
    return "directory";
  const auto size = std::filesystem::file_size(path, ec);
  if (ec)
    return "present";
  return "present " + std::to_string(size) + " bytes";
}

inline std::filesystem::path
runtime_report_preview_path(const RuntimeJobSummary &job) {
  const std::filesystem::path recorded =
      runtime_resolved_artifact_path(job, job.report_path);
  std::error_code ec;
  if (!recorded.empty() && std::filesystem::is_regular_file(recorded, ec))
    return recorded;

  if (job.job_dir.empty() || !std::filesystem::exists(job.job_dir, ec))
    return {};
  const auto opts = std::filesystem::directory_options::skip_permission_denied;
  for (std::filesystem::recursive_directory_iterator it(job.job_dir, opts, ec),
       end;
       it != end && !ec; it.increment(ec)) {
    if (!it->is_regular_file(ec))
      continue;
    if (it->path().extension() == ".report")
      return it->path();
  }
  return {};
}

inline std::filesystem::path
runtime_trace_preview_path(const RuntimeJobSummary &job) {
  if (job.job_dir.empty())
    return {};
  std::error_code ec;
  static constexpr std::array<std::string_view, 5> names{
      "trace.jsonl", "job.trace", "job.trace.jsonl", "runtime.trace",
      "runtime.trace.jsonl"};
  for (const std::string_view name : names) {
    const std::filesystem::path candidate = job.job_dir / std::string{name};
    if (std::filesystem::is_regular_file(candidate, ec))
      return candidate;
  }
  if (!std::filesystem::exists(job.job_dir, ec))
    return {};
  const auto opts = std::filesystem::directory_options::skip_permission_denied;
  for (std::filesystem::recursive_directory_iterator it(job.job_dir, opts, ec),
       end;
       it != end && !ec; it.increment(ec)) {
    if (!it->is_regular_file(ec))
      continue;
    const std::string filename = it->path().filename().string();
    if (filename.find("trace") != std::string::npos)
      return it->path();
  }
  return {};
}

inline std::string truncate_runtime_preview_line(std::string line,
                                                 std::size_t width = 96) {
  if (line.size() <= width)
    return line;
  if (width == 0u)
    return {};
  if (width == 1u)
    return "~";
  line.resize(width - 1u);
  line.push_back('~');
  return line;
}

inline void append_runtime_file_preview(std::ostringstream &out,
                                        const std::filesystem::path &path,
                                        std::size_t max_lines = 18) {
  if (path.empty()) {
    out << "  unavailable\n";
    return;
  }
  std::ifstream in(path);
  if (!in) {
    out << "  unavailable\n";
    return;
  }

  constexpr std::size_t kMaxPreviewBytes = 16384;
  std::string line;
  std::size_t line_count = 0;
  std::size_t byte_count = 0;
  while (line_count < max_lines && byte_count < kMaxPreviewBytes &&
         std::getline(in, line)) {
    byte_count += line.size() + 1u;
    out << std::setw(3) << (line_count + 1) << " | "
        << truncate_runtime_preview_line(line) << "\n";
    ++line_count;
  }
  if (line_count == 0)
    out << "  empty\n";
  else if (in.good())
    out << "  ...\n";
}

inline void append_runtime_compute_process_section(
    std::ostringstream &out, const RuntimeDeviceSnapshot &device,
    const RuntimeGpuSnapshot *selected_gpu = nullptr) {
  append_detail_section_header(out, "compute processes");

  bool any = false;
  for (const auto &process : device.gpu_processes) {
    if (selected_gpu != nullptr && process.gpu_uuid != selected_gpu->uuid)
      continue;

    any = true;
    out << "  pid=" << process.pid;
    if (selected_gpu == nullptr) {
      if (const RuntimeGpuSnapshot *gpu =
              find_gpu_by_uuid(device, process.gpu_uuid)) {
        out << "  gpu=" << display_or(gpu->index);
      } else {
        out << "  gpu=" << display_or(process.gpu_uuid);
      }
    }
    out << "  vram=" << process.used_memory_mib << " MiB\n";
  }

  if (!any)
    out << "  none\n";
}

inline std::string
make_runtime_device_detail_text(const RuntimeState &runtime) {
  const RuntimeDeviceSnapshot &device = runtime.device;
  std::ostringstream out;
  if (runtime.selected_device_row == 0 || device.gpus.empty()) {
    append_detail_section_header(out, "host device");
    out << "  host: " << display_or(device.host_name) << "\n";
    out << "  cpu_count: " << device.cpu_count << "\n";
    out << "  cpu_usage: " << pct_or_unknown(device.cpu_usage_pct) << " "
        << bar_pct(device.cpu_usage_pct) << "\n";
    out << "  cpu_iowait: " << pct_or_unknown(device.cpu_iowait_pct) << "\n";
    out << "  load: " << std::fixed << std::setprecision(2) << device.load1
        << " " << device.load5 << " " << device.load15 << "\n";
    if (device.mem_total_kib > 0) {
      const std::uint64_t used =
          device.mem_total_kib > device.mem_available_kib
              ? device.mem_total_kib - device.mem_available_kib
              : 0;
      const int mem_pct = device_memory_pct(device);
      out << "  memory: " << pct_or_unknown(mem_pct) << " " << bar_pct(mem_pct)
          << "\n";
      out << "  memory_used: " << mib(used) << "\n";
      out << "  memory_total: " << mib(device.mem_total_kib) << "\n";
    }
    out << "  gpus: " << device.gpus.size() << "\n";
    if (!device.gpu_error.empty())
      out << "  gpu_status: " << device.gpu_error << "\n";
    out << "\n";
    append_runtime_compute_process_section(out, device);
    return out.str();
  }

  const std::size_t gpu_index = runtime.selected_device_row - 1;
  if (gpu_index >= device.gpus.size())
    return out.str();
  const auto &gpu = device.gpus[gpu_index];
  const int mem_pct = pct_from_u64(gpu.memory_used_mib, gpu.memory_total_mib);
  append_detail_section_header(out, "gpu device");
  out << "  gpu: " << display_or(gpu.index) << "  " << display_or(gpu.name)
      << "\n";
  out << "  uuid: " << display_or(gpu.uuid) << "\n";
  out << "  utilization: " << pct_or_unknown(gpu.utilization_gpu_pct) << " "
      << bar_pct(gpu.utilization_gpu_pct) << "\n";
  out << "  vram_used: " << pct_or_unknown(mem_pct) << " " << bar_pct(mem_pct)
      << "\n";
  out << "  vram: " << ratio_mib(gpu.memory_used_mib, gpu.memory_total_mib)
      << "\n";
  out << "  memory_controller: " << pct_or_unknown(gpu.utilization_mem_pct)
      << "\n";
  out << "  temperature: "
      << (gpu.temperature_c < 0 ? std::string{"n/a"}
                                : std::to_string(gpu.temperature_c) + "C")
      << "\n";
  out << "\n";
  append_runtime_compute_process_section(out, device, &gpu);
  return out.str();
}

inline std::string make_runtime_job_detail_text(const CmdState &state) {
  std::ostringstream out;
  if (const RuntimeJobSummary *job = selected_job(state)) {
    append_detail_section_header(out, "job summary");
    out << "job_id\n" << display_or(job->job_id) << "\n\n";
    out << "status: " << display_or(job->status) << "\n";
    out << "kind: " << display_or(job->job_kind) << "\n";
    out << "wave: " << display_or(job->wave_id) << "\n";
    out << "action: " << display_or(job->wave_action) << "\n";
    out << "target: " << display_or(job->target_component_family_id) << "\n";
    out << "anchors: " << display_or(job->accepted_anchor_count) << "\n";
    out << "steps: " << display_or(job->steps_completed) << "\n";
    out << "last_loss: " << display_or(job->last_loss) << "\n\n";
    const std::filesystem::path report_path =
        runtime_resolved_artifact_path(*job, job->report_path);
    const std::filesystem::path checkpoint_path =
        runtime_resolved_artifact_path(*job, job->checkpoint_path);
    const std::filesystem::path job_state_path = job->job_dir / "job.state";
    const std::filesystem::path manifest_path = job->job_dir / "job.manifest";

    append_detail_section_header(out, "artifacts");
    out << "report: " << display_or(report_path.string()) << "\n";
    out << "report_status: " << runtime_artifact_status(report_path) << "\n";
    out << "checkpoint: " << display_or(checkpoint_path.string()) << "\n";
    out << "checkpoint_status: " << runtime_artifact_status(checkpoint_path)
        << "\n";
    out << "job_state: " << job_state_path.string() << "\n";
    out << "job_state_status: " << runtime_artifact_status(job_state_path)
        << "\n";
    out << "manifest: " << manifest_path.string() << "\n";
    out << "manifest_status: " << runtime_artifact_status(manifest_path)
        << "\n\n";

    const std::filesystem::path preview_path =
        runtime_report_preview_path(*job);
    if (!preview_path.empty()) {
      append_detail_section_header(out, "report preview");
      append_runtime_file_preview(out, preview_path);
    } else {
      append_detail_section_header(out, "job state preview");
      append_runtime_file_preview(out, job_state_path, 16);
    }
    out << "\n";
    append_detail_section_header(out, "job directory");
    out << "dir\n" << job->job_dir.string() << "\n";
  } else {
    out << "no runtime job selected\n";
  }
  return out.str();
}

inline std::string make_runtime_log_detail_text(const CmdState &state) {
  std::ostringstream out;
  if (state.runtime.focus == RuntimeFocus::Devices) {
    const RuntimeDeviceSnapshot &device = state.runtime.device;
    append_detail_section_header(out, "device telemetry");
    out << "samples: " << state.runtime.device_history.size() << "\n";
    out << "host: " << display_or(device.host_name) << "\n";
    out << "cpu: " << pct_or_unknown(device.cpu_usage_pct)
        << " iowait=" << pct_or_unknown(device.cpu_iowait_pct) << "\n";
    out << "load: " << std::fixed << std::setprecision(2) << device.load1 << " "
        << device.load5 << " " << device.load15 << "\n";
    out << "gpu_processes: " << device.gpu_processes.size() << "\n\n";
    append_detail_section_header(out, "history");
    out << "cpu_history: " << host_cpu_trend(state.runtime.device_history)
        << "\n";
    out << "memory_history: " << host_memory_trend(state.runtime.device_history)
        << "\n";
    return out.str();
  }

  if (const RuntimeJobSummary *job = selected_job(state)) {
    const std::filesystem::path job_state_path = job->job_dir / "job.state";
    append_detail_section_header(out, "job_state");
    out << job_state_path.string() << "\n";
    out << "status: " << runtime_artifact_status(job_state_path) << "\n\n";
    append_runtime_file_preview(out, job_state_path, 24);

    const std::filesystem::path report_path = runtime_report_preview_path(*job);
    if (!report_path.empty()) {
      out << "\n";
      append_detail_section_header(out, "report tail");
      out << report_path.string() << "\n";
      append_runtime_file_preview(out, report_path, 12);
    }
  } else {
    out << "no runtime job selected\n";
  }
  return out.str();
}

inline std::string make_runtime_trace_detail_text(const CmdState &state) {
  std::ostringstream out;
  if (state.runtime.focus == RuntimeFocus::Devices) {
    append_detail_section_header(out, "runtime policy");
    out << "policy: " << state.runtime.policy.policy_path.string() << "\n";
    out << "runtime_root: " << state.runtime.policy.runtime_root.string()
        << "\n";
    out << "protocol_layer: " << display_or(state.runtime.policy.protocol_layer)
        << "\n";
    out << "exec: " << state.runtime.policy.runtime_exec_path.string() << "\n";
    out << "exec_exists: " << yes_no(state.runtime.policy.runtime_exec_exists)
        << " executable="
        << yes_no(state.runtime.policy.runtime_exec_executable) << "\n\n";
    append_detail_section_header(out, "active wave");
    out << "wave: " << display_or(state.runtime.wave.wave_id) << "\n";
    out << "path: " << state.runtime.wave.wave_path.string() << "\n";
    out << "target: " << display_or(state.runtime.wave.target) << "\n";
    out << "mode: " << display_or(state.runtime.wave.mode) << "\n";
    out << "range: " << display_or(state.runtime.wave.source_range) << "\n";
    out << "anchors: [" << display_or(state.runtime.wave.anchor_index_begin)
        << ", " << display_or(state.runtime.wave.anchor_index_end) << ")\n";
    return out.str();
  }

  if (const RuntimeJobSummary *job = selected_job(state)) {
    const std::filesystem::path trace_path = runtime_trace_preview_path(*job);
    if (!trace_path.empty()) {
      append_detail_section_header(out, "trace preview");
      out << trace_path.string() << "\n";
      append_runtime_file_preview(out, trace_path, 32);
      return out.str();
    }

    const std::filesystem::path manifest_path = job->job_dir / "job.manifest";
    out << "trace artifact: not recorded\n\n";
    append_detail_section_header(out, "manifest fallback");
    out << manifest_path.string() << "\n";
    append_runtime_file_preview(out, manifest_path, 24);
  } else {
    out << "no runtime job selected\n";
  }
  return out.str();
}

inline std::string make_runtime_detail_text(const CmdState &state) {
  if (state.runtime.focus == RuntimeFocus::Jobs)
    return make_runtime_job_detail_text(state);

  switch (state.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    return make_runtime_device_detail_text(state.runtime);
  case RuntimeDetailMode::Log:
    return make_runtime_log_detail_text(state);
  case RuntimeDetailMode::Trace:
    return make_runtime_trace_detail_text(state);
  }
  return make_runtime_device_detail_text(state.runtime);
}

inline std::string make_help_text(const CmdState &state) {
  std::ostringstream out;
  out << "Help\n\n";
  out << "Screens\n";
  for (ScreenMode screen : screen_order()) {
    out << "  " << std::left << std::setw(3) << screen_key_label(screen) << " "
        << screen_badge_label(screen) << "\n";
  }
  out << "\nGlobal\n";
  out << "  h/H / ? / help / menu open this panel; lowercase h browses rows on "
         "browser screens\n";
  out << "  a / actions      open action menu\n";
  out << "  refresh / r      refresh snapshots\n";
  out << "  clear logs       clear shell log feed\n";
  out << "  zoom / f / z     full/split zoom when supported; Esc restores "
         "split\n";
  out << "  h/j/k/l          scroll overlays; move rows on browser screens\n";
  out << "  type / :         enter command mode\n";
  out << "  Enter            run command while editing\n";
  out << "  Ctrl-U           clear command input\n";
  out << "  Up/Down          recall history in command mode\n";
  out << "  quit / q / exit / x      exit\n";
  out << "  Esc              close help\n\n";
  out << "Shell utilities\n";
  out << "  cuwacunu_cmd --menu                 render menu overlay\n";
  out << "  cuwacunu.cmd --menu                 render menu overlay\n";
  out << "  cuwacunu_cmd --help / --version     menu or command identity\n";
  out << "  cuwacunu.cmd --help / --version     menu or command identity\n";
  out << "  iinuji_cmd --menu                   render menu overlay\n";
  out << "  cuwacunu_cmd --actions / --palette  render action palette\n";
  out << "  cuwacunu.cmd --actions / --palette  render action palette\n";
  out << "  cuwacunu_cmd --commands / --catalog print command catalog\n";
  out << "  cuwacunu.cmd --commands / --catalog print command catalog\n";
  out << "  iinuji_cmd --commands / --catalog   print command catalog\n";
  out << "  cuwacunu_cmd --visual / --home-visual render Home image/APNG "
         "preview\n";
  out << "  cuwacunu.cmd --visual / --home-visual render Home image/APNG "
         "preview\n";
  out << "  cuwacunu_cmd --image        render Home still image preview\n";
  out << "  cuwacunu.cmd --image        render Home still image preview\n";
  out << "  cuwacunu_cmd --animation    render Home APNG motion preview\n";
  out << "  cuwacunu.cmd --animation    render Home APNG motion preview\n";
  out << "  cuwacunu_cmd --waajacamaya     render Home waajacamaya visual\n";
  out << "  cuwacunu.cmd --waajacamaya     render Home waajacamaya visual\n";
  out << "  iinuji_cmd --visual / --home-visual render Home image/APNG "
         "preview\n";
  out << "  cuwacunu_cmd --bootstrap / --boot   render bootstrap splash "
         "report\n";
  out << "  cuwacunu.cmd --bootstrap / --boot   render bootstrap splash "
         "report\n";
  out << "  cuwacunu_cmd --splash loading       render bootstrap splash "
         "report\n";
  out << "  cuwacunu.cmd --splash loading       render bootstrap splash "
         "report\n";
  out << "  cuwacunu_cmd --farewell / --closing render closing splash report\n";
  out << "  cuwacunu.cmd --farewell / --closing render closing splash report\n";
  out << "  cuwacunu_cmd --splash farewell      render closing splash report\n";
  out << "  cuwacunu.cmd --splash farewell      render closing splash report\n";
  out << "  cuwacunu_cmd --splash close         render closing splash report\n";
  out << "  cuwacunu.cmd --splash close         render closing splash report\n";
  out << "  cuwacunu_cmd --splash=good-luck     render Good luck splash "
         "report\n";
  out << "  cuwacunu.cmd --splash=good-luck     render Good luck splash "
         "report\n";
  out << "  cuwacunu_cmd --splash good luck     render Good luck splash "
         "report\n";
  out << "  cuwacunu.cmd --splash good luck     render Good luck splash "
         "report\n";
  out << "  iinuji_cmd --splash=farewell        render closing splash report\n";
  out << "  cuwacunu_cmd runtime --snapshot     render Runtime snapshot\n";
  out << "  cuwacunu.cmd runtime --snapshot     render Runtime snapshot\n";
  out << "  cuwacunu_cmd --screen runtime       switch to Runtime screen\n";
  out << "  cuwacunu.cmd --screen runtime       switch to Runtime screen\n";
  out << "  cuwacunu_cmd src/config/hero.runtime.dsl --snapshot open Config "
         "file\n";
  out << "  cuwacunu.cmd src/config/hero.runtime.dsl --snapshot open Config "
         "file\n";
  out << "  cuwacunu_cmd --run \"show\" --snapshot  run command then render "
         "snapshot\n";
  out << "  cuwacunu.cmd --run \"show\" --snapshot  run command then render "
         "snapshot\n";
  out << "  cuwacunu_cmd --command \"show\" --snapshot alias for --run "
         "snapshot\n";
  out << "  cuwacunu.cmd --command \"show\" --snapshot alias for --run "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --menu   render menu overlay snapshot\n";
  out << "  cuwacunu.cmd --snapshot --menu   render menu overlay snapshot\n";
  out << "  cuwacunu_cmd --snapshot --actions render action palette snapshot\n";
  out << "  cuwacunu.cmd --snapshot --actions render action palette snapshot\n";
  out << "  cuwacunu_cmd --snapshot --catalog render command catalog "
         "snapshot\n";
  out << "  cuwacunu.cmd --snapshot --catalog render command catalog "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --visual render Home visual snapshot\n";
  out << "  cuwacunu.cmd --snapshot --visual render Home visual snapshot\n";
  out << "  cuwacunu_cmd --snapshot --splash loading render bootstrap visual "
         "snapshot\n";
  out << "  cuwacunu.cmd --snapshot --splash loading render bootstrap visual "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --splash close render closing visual "
         "snapshot\n";
  out << "  cuwacunu.cmd --snapshot --splash close render closing visual "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --splash good-luck render Good luck visual "
         "snapshot\n";
  out << "  cuwacunu.cmd --snapshot --splash good-luck render Good luck visual "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --screen runtime render Runtime screen "
         "snapshot\n";
  out << "  cuwacunu.cmd --snapshot --screen runtime render Runtime screen "
         "snapshot\n";
  out << "  cuwacunu_cmd --snapshot --full   render full-panel snapshot\n";
  out << "  cuwacunu.cmd --snapshot --full   render full-panel snapshot\n\n";
  out << "Natural utility aliases\n";
  out << "  h/j/k/l and arrows             scroll this help/menu overlay\n";
  out << "  help/menu up/down/left/right   scroll this help/menu overlay\n";
  out << "  help/menu page/home/end        page or edge-scroll this help "
         "overlay\n";
  out << "  actions down / palette down / commands down move action palette "
         "selection\n";
  out << "  actions page up/down           page action palette selection\n";
  out << "  actions home/end               jump action palette edge rows\n";
  out << "  home / f1 / 1                  switch to F1 Home\n";
  out << "  workbench / work / w / f2 / 2  switch to F2 Workbench\n";
  out << "  runtime / run / rt / f3 / 3    switch to F3 Runtime\n";
  out << "  lattice / lat / proof / f4 / 4 switch to F4 Lattice\n";
  out << "  logs / log / shell / events / f8 / 8 switch to F8 Shell Logs\n";
  out << "  config / cfg / f9 / 9          switch to F9 Config\n";
  out << "  splash / farewell              show Home visual diagnostics\n";
  out << "  logs down / logs page down     scroll Shell Logs stream\n";
  out << "  runtime down / runtime up      select Runtime browser rows\n";
  out << "  config down / config up        browse Config files\n";
  out << "  lattice down / lattice up      browse Lattice targets\n\n";
  out << "Screen-local utilities\n";
  out << "  Runtime Enter menu; a actions for the selected item\n";
  out << "  Lattice Enter                  show selected target proof\n";
  out << "  Config Enter / e               open selected file preview\n";
  out << "  Shell Logs m / p               metadata lens and mouse capture\n";
  out << "  Shell Logs Left / Right        change selected log setting\n";
  out << "  Shell Logs Home / End          jump oldest or newest log rows\n\n";
  out << "Canonical command paths\n";
  out << "  iinuji.screen.home()       F1 HOME\n";
  out << "  iinuji.screen.workbench()  F2 WORKBENCH\n";
  out << "  iinuji.screen.runtime()    F3 RUNTIME\n";
  out << "  iinuji.screen.lattice()    F4 LATTICE\n";
  out << "  iinuji.screen.logs()       F8 SHELL LOGS\n";
  out << "  iinuji.screen.config()     F9 CONFIG\n";
  out << "  iinuji.show()              show active screen summary\n";
  out << "  iinuji.show.home()         show F1 HOME summary\n";
  out << "  iinuji.show.workbench()    show F2 WORKBENCH summary\n";
  out << "  iinuji.show.runtime()      show F3 RUNTIME summary\n";
  out << "  iinuji.show.lattice()      show F4 LATTICE summary\n";
  out << "  iinuji.show.logs()         show F8 SHELL LOGS summary\n";
  out << "  iinuji.show.config()       show F9 CONFIG summary\n";
  out << "  iinuji.home.visual()       show F1 HOME visual showcase\n";
  out << "  iinuji.home.animation()    Home animation alias\n";
  out << "  iinuji.home.splash()       show bootstrap splash diagnostics\n";
  out << "  iinuji.splash()            bootstrap splash alias\n";
  out << "  iinuji.home.farewell()     show closing splash diagnostics\n";
  out << "  iinuji.farewell()          closing splash alias\n";
  out << "  iinuji.refresh()           refresh snapshots\n";
  out << "  iinuji.workbench.refresh() refresh Workbench state\n";
  out << "  iinuji.runtime.refresh()   refresh Runtime telemetry and jobs\n";
  out << "  iinuji.lattice.refresh()   refresh Lattice target/fact counters\n";
  out << "  iinuji.config.reload()     reload managed Config catalog\n";
  out << "  iinuji.help()              open help/menu overlay\n";
  out << "  iinuji.help.close()        close help/menu overlay\n";
  out << "  iinuji.help.scroll.*()     scroll help/menu overlay\n";
  out << "  iinuji.actions()           open action palette\n";
  out << "  iinuji.commands()          commands namespace for action palette\n";
  out << "  iinuji.actions.run()       run selected palette action\n";
  out << "  iinuji.commands.run.selected() run selected palette action\n";
  out << "  iinuji.actions.close()     close action palette\n";
  out << "  iinuji.commands.close()    close action palette via commands\n";
  out << "  iinuji.actions.select.prev() select previous palette action\n";
  out << "  iinuji.actions.select.next() select next palette action\n";
  out << "  iinuji.actions.select.page.up() page palette selection up\n";
  out << "  iinuji.actions.select.page.down() page palette selection down\n";
  out << "  iinuji.actions.select.home() jump to first palette action\n";
  out << "  iinuji.actions.select.end()  jump to final palette action\n";
  out << "  iinuji.commands.select.*() commands namespace movement utilities\n";
  out << "  iinuji.workspace.zoom.toggle() toggle full/split workspace\n";
  out << "  iinuji.workspace.split()   restore split workspace\n";
  out << "  iinuji.runtime.devices()   focus Runtime devices\n";
  out << "  iinuji.runtime.jobs()      focus Runtime jobs\n";
  out << "  iinuji.runtime.detail.next() cycle Runtime detail mode\n";
  out << "  iinuji.runtime.detail.manifest() set Runtime manifest detail\n";
  out << "  iinuji.runtime.detail.log() set Runtime log detail\n";
  out << "  iinuji.runtime.detail.trace() set Runtime trace detail\n";
  out << "  iinuji.runtime.row.prev()  select previous Runtime focus lane\n";
  out << "  iinuji.runtime.row.next()  select next Runtime focus lane\n";
  out << "  iinuji.runtime.item.prev() select previous runtime row\n";
  out << "  iinuji.runtime.item.next() select next runtime row\n";
  out << "  iinuji.logs.scroll.up()   scroll Shell Logs upward\n";
  out << "  iinuji.logs.scroll.down() scroll Shell Logs downward\n";
  out << "  iinuji.logs.scroll.page.up() page Shell Logs upward\n";
  out << "  iinuji.logs.scroll.page.down() page Shell Logs downward\n";
  out << "  iinuji.logs.scroll.home()  jump Shell Logs to oldest\n";
  out << "  iinuji.logs.scroll.end()   jump Shell Logs to newest\n";
  out << "  iinuji.logs.clear()        clear Shell Logs feed\n";
  out << "  iinuji.logs.settings.select.prev() select previous logs setting\n";
  out << "  iinuji.logs.settings.select.next() select next logs setting\n";
  out << "  iinuji.logs.settings.change.prev() change selected logs setting\n";
  out << "  iinuji.logs.settings.change.next() change selected logs setting\n";
  out << "  iinuji.logs.settings.source.next() cycle source lens\n";
  out << "  iinuji.logs.settings.source.any() set source lens to all\n";
  out << "  iinuji.logs.settings.source.all() set source lens to all\n";
  out << "  iinuji.logs.settings.source.refresh() set source lens to refresh\n";
  out << "  iinuji.logs.settings.source.action() set source lens to action\n";
  out << "  iinuji.logs.settings.source.command() set source lens to command\n";
  out << "  iinuji.logs.settings.source.show() set source lens to show\n";
  out << "  iinuji.logs.settings.source.status() set source lens to status\n";
  out << "  iinuji.logs.settings.date.toggle() toggle timestamps\n";
  out << "  iinuji.logs.settings.follow.toggle() toggle follow mode\n";
  out << "  iinuji.logs.settings.level.next() cycle log severity lens\n";
  out << "  iinuji.logs.settings.level.debug() set log severity lens\n";
  out << "  iinuji.logs.settings.level.info() set log severity lens\n";
  out << "  iinuji.logs.settings.level.warning() set log severity lens\n";
  out << "  iinuji.logs.settings.level.error() set log severity lens\n";
  out << "  iinuji.logs.settings.level.fatal() set log severity lens\n";
  out << "  iinuji.logs.settings.metadata.filter.next() cycle metadata lens\n";
  out << "  iinuji.logs.settings.metadata.filter.any() set metadata lens\n";
  out << "  iinuji.logs.settings.metadata.filter.any_meta() set metadata "
         "lens\n";
  out << "  iinuji.logs.settings.metadata.filter.function() set metadata "
         "lens\n";
  out << "  iinuji.logs.settings.metadata.filter.path() set metadata lens\n";
  out << "  iinuji.logs.settings.metadata.filter.callsite() set metadata "
         "lens\n";
  out << "  iinuji.logs.settings.metadata.toggle() toggle metadata fields\n";
  out << "  iinuji.logs.settings.thread.toggle() toggle thread column\n";
  out << "  iinuji.logs.settings.color.toggle() toggle log color emphasis\n";
  out << "  iinuji.logs.settings.mouse.capture.toggle() toggle mouse capture\n";
  out << "  iinuji.lattice.target.next() select next lattice target\n";
  out << "  iinuji.lattice.target.prev() select previous lattice target\n";
  out << "  iinuji.lattice.target.first() select first lattice target\n";
  out << "  iinuji.lattice.target.last() select last lattice target\n";
  out << "  iinuji.lattice.target.index.1() select lattice target by index\n";
  out << "  iinuji.lattice.target.index(n=1) select lattice target by arg\n";
  out << "  iinuji.lattice.target.id.default() select lattice target by id\n";
  out << "  lattice target 1 / target default natural lattice selectors\n";
  out << "  iinuji.config.files()     list Config catalog files\n";
  out << "  iinuji.config.show()      show selected Config summary\n";
  out << "  iinuji.config.file.show() show selected Config file preview\n";
  out << "  iinuji.config.file.next() select next config file\n";
  out << "  iinuji.config.file.prev() select previous config file\n";
  out << "  iinuji.config.file.first() select first config file\n";
  out << "  iinuji.config.file.last() select last config file\n";
  out << "  iinuji.config.file.index.1() select config file by index\n";
  out << "  iinuji.config.file.index(n=1) select config file by arg\n";
  out << "  iinuji.config.file.id.default() select config file by id\n";
  out << "  config file 1 / file default natural config selectors\n\n";
  out << "  iinuji.quit()              exit command shell\n";
  out << "  iinuji.exit()              exit command shell\n\n";
  out << make_screen_menu_text(state.screen);
  return out.str();
}

inline std::string logs_setting_cursor(const CmdState &state,
                                       std::size_t index) {
  return state.logs.selected_setting == index ? "> " : "  ";
}

inline std::string logs_setting_value_badge(std::string value) {
  return "[" + std::move(value) + "]";
}

inline std::string logs_selected_setting_hint(std::size_t index) {
  switch (index) {
  case 0:
    return "source selects the shell feed lane shown in the stream.";
  case 1:
    return "level raises or lowers the minimum severity lens.";
  case 2:
    return "meta filters entries by metadata payload availability.";
  case 3:
    return "timestamp toggles time labels for visible log rows.";
  case 4:
    return "thread toggles the captured thread column.";
  case 5:
    return "metadata toggles inline structured fields.";
  case 6:
    return "color toggles severity and metadata emphasis.";
  case 7:
    return "follow keeps the stream pinned to newest entries.";
  case 8:
    return "mouse switches between capture controls and select/copy.";
  }
  return "Up/Down selects a setting; Left/Right changes it.";
}

inline std::string make_logs_control_deck_text(const CmdState &state) {
  std::size_t shown = 0;
  std::size_t refresh_count = 0;
  std::size_t action_count = 0;
  std::size_t command_count = 0;
  std::size_t show_count = 0;
  std::size_t status_count = 0;
  std::size_t metadata_count = 0;
  std::size_t function_count = 0;
  std::size_t path_count = 0;
  std::size_t callsite_count = 0;
  std::size_t fatal_count = 0;
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  std::size_t info_count = 0;
  std::size_t debug_count = 0;
  for (const auto &entry : state.log) {
    if (entry.source == "refresh")
      ++refresh_count;
    else if (entry.source == "action")
      ++action_count;
    else if (entry.source == "command")
      ++command_count;
    else if (entry.source == "show")
      ++show_count;
    else if (entry.source == "status")
      ++status_count;
    if (log_entry_has_any_metadata(entry))
      ++metadata_count;
    if (log_entry_has_function_metadata(entry))
      ++function_count;
    if (log_entry_has_path_metadata(entry))
      ++path_count;
    if (log_entry_has_callsite_metadata(entry))
      ++callsite_count;
    switch (log_entry_severity(entry)) {
    case LogsSeverityFilter::FatalOnly:
      ++fatal_count;
      break;
    case LogsSeverityFilter::ErrorOrHigher:
      ++error_count;
      break;
    case LogsSeverityFilter::WarningOrHigher:
      ++warning_count;
      break;
    case LogsSeverityFilter::InfoOrHigher:
      ++info_count;
      break;
    case LogsSeverityFilter::DebugOrHigher:
      ++debug_count;
      break;
    }
    if (logs_accept_entry(state, entry))
      ++shown;
  }
  const std::size_t hidden =
      state.log.size() >= shown ? state.log.size() - shown : 0;

  std::ostringstream out;
  out << "Shell Logs Control Deck\n";
  out << "  tuned for live shell diagnostics\n\n";
  append_detail_section_header(out, "Capture");
  out << "  visible     " << shown << " / " << state.log.size() << "\n";
  out << "  hidden      " << hidden << "\n";
  out << "  capacity    " << state.logs.buffer_capacity << "\n";
  if (!state.log.empty()) {
    out << "  first seen  "
        << log_timestamp_text(state.log.front().timestamp_ms) << "\n";
    out << "  latest      " << log_timestamp_text(state.log.back().timestamp_ms)
        << "\n";
  }

  out << "\n";
  append_detail_section_header(out, "Lens");
  out << "  source      " << logs_source_filter_label(state.logs.source_filter)
      << "\n";
  out << "  level       "
      << logs_severity_filter_label(state.logs.severity_filter) << "\n";
  out << "  meta        "
      << logs_metadata_filter_label(state.logs.metadata_filter) << "\n";
  out << "  fields      timestamp " << on_off(state.logs.show_timestamp)
      << " | thread " << on_off(state.logs.show_thread) << "\n";
  out << "              meta " << on_off(state.logs.show_metadata)
      << " | color " << on_off(state.logs.show_color) << "\n";
  out << "  behavior    follow " << on_off(state.logs.auto_follow)
      << " | mouse " << (state.logs.mouse_capture ? "capture" : "select/copy")
      << "\n";

  out << "\n";
  append_detail_section_header(out, "Settings");
  out << "  Up/Down select | Left/Right change\n";
  out << logs_setting_cursor(state, 0) << "source      "
      << logs_setting_value_badge(
             logs_source_filter_label(state.logs.source_filter))
      << "\n";
  out << logs_setting_cursor(state, 1) << "level       "
      << logs_setting_value_badge(
             logs_severity_filter_label(state.logs.severity_filter))
      << "\n";
  out << logs_setting_cursor(state, 2) << "meta        "
      << logs_setting_value_badge(
             logs_metadata_filter_label(state.logs.metadata_filter))
      << "\n";
  out << logs_setting_cursor(state, 3) << "timestamp   "
      << logs_setting_value_badge(on_off(state.logs.show_timestamp)) << "\n";
  out << logs_setting_cursor(state, 4) << "thread      "
      << logs_setting_value_badge(on_off(state.logs.show_thread)) << "\n";
  out << logs_setting_cursor(state, 5) << "metadata    "
      << logs_setting_value_badge(on_off(state.logs.show_metadata)) << "\n";
  out << logs_setting_cursor(state, 6) << "color       "
      << logs_setting_value_badge(on_off(state.logs.show_color)) << "\n";
  out << logs_setting_cursor(state, 7) << "follow      "
      << logs_setting_value_badge(on_off(state.logs.auto_follow)) << "\n";
  out << logs_setting_cursor(state, 8) << "mouse       "
      << logs_setting_value_badge(state.logs.mouse_capture ? "capture"
                                                           : "select/copy")
      << "\n";

  out << "\n";
  append_detail_section_header(out, "Selected");
  out << "  " << logs_selected_setting_hint(state.logs.selected_setting)
      << "\n";

  out << "\n";
  append_detail_section_header(out, "Signal Mix");
  out << "  refresh     " << refresh_count << "\n";
  out << "  action      " << action_count << "\n";
  out << "  command     " << command_count << "\n";
  out << "  show        " << show_count << "\n";
  out << "  status      " << status_count << "\n";
  out << "\n";
  append_detail_section_header(out, "Metadata Mix");
  out << "  any         " << metadata_count << "\n";
  out << "  fn          " << function_count << "\n";
  out << "  path        " << path_count << "\n";
  out << "  callsite    " << callsite_count << "\n";
  out << "\n";
  append_detail_section_header(out, "Severity Mix");
  out << "  fatal       " << fatal_count << "\n";
  out << "  error       " << error_count << "\n";
  out << "  warning     " << warning_count << "\n";
  out << "  info        " << info_count << "\n";
  out << "  debug       " << debug_count << "\n";

  out << "\n";
  append_detail_section_header(out, "Quick Keys");
  out << "  Up/Down     select setting\n";
  out << "  Left/Right  change selected setting\n";
  out << "  s           cycle source filter\n";
  out << "  v           cycle severity level\n";
  out << "  m           cycle metadata filter\n";
  out << "  t           toggle timestamps\n";
  out << "  u           toggle thread column\n";
  out << "  e           toggle metadata fields\n";
  out << "  o           toggle color emphasis\n";
  out << "  l           toggle follow\n";
  out << "  p           toggle mouse capture\n";
  out << "  f / Esc     full screen / split\n";
  out << "  z           toggle full screen\n";
  out << "  c           clear logs\n";
  out << "  Home/End    jump oldest/newest\n";
  out << "  wheel       scroll panels\n";
  out << "  h/H/?       menu/help\n";
  return out.str();
}

struct MenuOverlayColumns {
  std::vector<styled_text_line_t> commands{};
  std::vector<styled_text_line_t> comments{};
};

inline constexpr std::size_t kMenuOverlayCommandWidth = 46u;
inline constexpr std::size_t kMenuOverlayCommentWidth = 58u;

inline styled_text_line_t
menu_overlay_line(std::string text,
                  text_line_emphasis_t emphasis = text_line_emphasis_t::None,
                  std::string background_color = {}) {
  return styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
      .background_color = std::move(background_color),
  };
}

inline std::string menu_overlay_fit(std::string text, std::size_t max_size) {
  if (text.size() <= max_size)
    return text;
  if (max_size == 0u)
    return {};
  if (max_size == 1u)
    return "~";
  text.resize(max_size - 1u);
  text.push_back('~');
  return text;
}

inline void push_menu_row(
    MenuOverlayColumns &columns, const std::string &command,
    const std::string &comment,
    text_line_emphasis_t command_emphasis = text_line_emphasis_t::None,
    text_line_emphasis_t comment_emphasis = text_line_emphasis_t::Info,
    std::string background_color = {}) {
  columns.commands.push_back(
      menu_overlay_line(menu_overlay_fit(command, kMenuOverlayCommandWidth),
                        command_emphasis, background_color));
  columns.comments.push_back(
      menu_overlay_line(menu_overlay_fit(comment, kMenuOverlayCommentWidth),
                        comment_emphasis, background_color));
}

inline void push_menu_header(MenuOverlayColumns &columns,
                             const std::string &title) {
  push_menu_row(columns, title, "", text_line_emphasis_t::Accent,
                text_line_emphasis_t::Debug);
}

inline void append_menu_current_screen_rows(MenuOverlayColumns &columns,
                                            const CmdState &state) {
  switch (state.screen) {
  case ScreenMode::Home:
    push_menu_row(columns, "F2 / F3 / F4 / F8 / F9",
                  "jump to Workbench, Runtime, Lattice, Logs, Config");
    push_menu_row(columns, "Home showcase",
                  "waajacamaya APNG + Cuwacunu wordmark");
    push_menu_row(columns, "visual / waajacamaya",
                  "return to animated Home showcase");
    push_menu_row(columns, "cuwacunu_cmd --visual / --home-visual",
                  "inspect rendered animation frames");
    push_menu_row(columns, "cuwacunu_cmd --image",
                  "inspect rendered image frame");
    push_menu_row(columns, "cuwacunu_cmd --animation",
                  "inspect rendered animation frames");
    push_menu_row(columns, "cuwacunu_cmd --waajacamaya",
                  "inspect rendered waajacamaya visual");
    push_menu_row(columns, "splash / bootstrap", "show splash assets");
    push_menu_row(columns, "farewell / closing", "show closing splash assets");
    push_menu_row(columns, "cuwacunu_cmd --splash",
                  "inspect splash asset report");
    push_menu_row(columns, "cuwacunu_cmd --splash loading",
                  "inspect splash asset report");
    push_menu_row(columns, "cuwacunu_cmd --bootstrap / --boot",
                  "inspect splash asset report");
    push_menu_row(columns, "cuwacunu_cmd --farewell / --closing",
                  "inspect closing splash report");
    push_menu_row(columns, "cuwacunu_cmd --splash farewell",
                  "inspect closing splash report");
    push_menu_row(columns, "cuwacunu_cmd --splash close",
                  "inspect closing splash report");
    push_menu_row(columns, "cuwacunu_cmd --splash=farewell",
                  "inspect closing splash report");
    push_menu_row(columns, "cuwacunu_cmd --splash=good-luck",
                  "inspect Good luck closing splash report");
    break;
  case ScreenMode::Workbench:
    push_menu_row(columns, "F3 / F4 / F8 / F9",
                  "jump to Runtime, Lattice, Logs, or Config");
    break;
  case ScreenMode::Runtime:
    push_menu_row(columns, "Left / Right", "switch Devices and Jobs focus");
    push_menu_row(columns, "d / J", "focus Devices or Jobs directly");
    push_menu_row(columns, "k / j / h / l", "row selection keys");
    push_menu_row(columns, "Enter menu | a actions",
                  "Enter opens selected item menu; a opens actions");
    push_menu_row(columns, "Tab / Shift-Tab",
                  "cycle manifest, log, trace detail");
    push_menu_row(columns, "manifest / log / trace",
                  "choose Runtime detail lens directly");
    push_menu_row(columns, "Up / Down",
                  "select host, gpu, or runtime job rows");
    push_menu_row(columns, "Runtime detail",
                  "selected row follows active detail mode");
    push_menu_row(columns, "PgUp / PgDn",
                  "scroll Runtime context/detail panel");
    push_menu_row(columns, "f / z", "toggle full screen; Esc restores split");
    push_menu_row(columns, "r", "refresh runtime snapshots");
    break;
  case ScreenMode::Lattice:
    push_menu_row(columns, "Up / Down / k / j",
                  "select previous or next target");
    push_menu_row(columns, "h / l / [ / ]", "browse previous or next target");
    push_menu_row(columns, "Home / End", "jump first or last target");
    push_menu_row(columns, "Enter", "show selected target proof");
    push_menu_row(columns, "PgUp / PgDn", "scroll selected target evidence");
    push_menu_row(columns, "detail panel", "read-only selected target block");
    push_menu_row(columns, "r", "refresh target/fact counters");
    push_menu_row(columns, "read-only",
                  "inspect target evidence without edits");
    break;
  case ScreenMode::Logs:
    push_menu_row(columns, "PgUp / PgDn", "scroll Shell Logs stream");
    push_menu_row(columns, "Home / End", "jump to first or newest log rows");
    push_menu_row(columns, "s / v / m / t / l",
                  "source, severity, metadata, timestamps, follow");
    push_menu_row(columns, "u / e / o / p",
                  "thread, metadata fields, color, mouse capture");
    push_menu_row(columns, "f / z", "toggle full screen; Esc restores split");
    push_menu_row(columns, "clear logs", "clear the current in-memory feed");
    push_menu_row(columns, "Up / Down settings",
                  "select Shell Logs setting row");
    push_menu_row(columns, "Left / Right settings",
                  "change selected Shell Logs setting");
    break;
  case ScreenMode::Config:
    push_menu_row(columns, "Up / Down / k / j",
                  "select previous or next config file");
    push_menu_row(columns, "h / l / [ / ]", "browse previous or next file");
    push_menu_row(columns, "Home / End", "jump first or last config file");
    push_menu_row(columns, "Enter / e", "open selected file preview");
    push_menu_row(columns, "PgUp / PgDn", "scroll selected file preview");
    push_menu_row(columns, "files / show config",
                  "list catalog or show selected summary");
    push_menu_row(columns, "show file", "show selected file preview utility");
    push_menu_row(columns, "detail panel", "read-only selected file preview");
    push_menu_row(columns, "r", "refresh managed config catalog");
    push_menu_row(columns, "read-only", "inspect managed files without edits");
    break;
  }
}

inline MenuOverlayColumns make_menu_overlay_columns(const CmdState &state) {
  MenuOverlayColumns columns{};

  push_menu_header(columns, "MENU OVERLAY");
  push_menu_row(columns, "Esc / [x]", "close menu",
                text_line_emphasis_t::Warning);
  push_menu_row(columns, "a / actions", "open navigable action menu");
  push_menu_row(columns, "Arrows or h/j/k/l", "scroll this overlay");
  push_menu_row(columns, "PgUp / PgDn", "page overlay scroll");
  push_menu_row(columns, "Home / End", "jump overlay scroll");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "SCREENS");
  for (ScreenMode screen : screen_order()) {
    std::ostringstream cmd;
    cmd << screen_key_label(screen) << "  " << screen_badge_label(screen);
    const bool current = state.screen == screen;
    push_menu_row(
        columns, cmd.str(), current ? "current screen" : "switch screen",
        current ? text_line_emphasis_t::Warning : text_line_emphasis_t::None,
        current ? text_line_emphasis_t::Warning : text_line_emphasis_t::Info,
        current ? "#2A2418" : std::string{});
  }
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "CONTEXT");
  push_menu_row(columns, "current",
                screen_key_label(state.screen) + " " +
                    screen_badge_label(state.screen));
  push_menu_row(columns, "screen tools", "full list below in CURRENT SCREEN");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "COMMAND ALIASES");
  push_menu_row(columns, "help | h | ? | menu", "open command and screen help");
  push_menu_row(columns, "home | f1 | 1", "switch to F1 Home");
  push_menu_row(columns, "home | visual | waajacamaya",
                "show animated Home showcase");
  push_menu_row(columns, "cuwacunu_cmd --visual / --home-visual",
                "render Home image/APNG snapshot");
  push_menu_row(columns, "cuwacunu.cmd --visual / --home-visual",
                "render Home image/APNG snapshot");
  push_menu_row(columns, "cuwacunu_cmd --image", "render Home image snapshot");
  push_menu_row(columns, "cuwacunu.cmd --image", "render Home image snapshot");
  push_menu_row(columns, "cuwacunu_cmd --animation",
                "render Home APNG frame snapshot");
  push_menu_row(columns, "cuwacunu.cmd --animation",
                "render Home APNG frame snapshot");
  push_menu_row(columns, "cuwacunu_cmd --waajacamaya",
                "render Home waajacamaya visual snapshot");
  push_menu_row(columns, "cuwacunu.cmd --waajacamaya",
                "render Home waajacamaya visual snapshot");
  push_menu_row(columns, "splash | bootstrap", "show Home splash diagnostics");
  push_menu_row(columns, "farewell | closing",
                "show closing splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash loading",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash loading",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --bootstrap / --boot",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --bootstrap / --boot",
                "render bootstrap splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --farewell / --closing",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --farewell / --closing",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash farewell",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash farewell",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash close",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash close",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash=farewell",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash=farewell",
                "render closing splash diagnostics");
  push_menu_row(columns, "cuwacunu_cmd --splash=good-luck",
                "render Good luck closing splash diagnostics");
  push_menu_row(columns, "cuwacunu.cmd --splash=good-luck",
                "render Good luck closing splash diagnostics");
  push_menu_row(columns, "workbench | work | w | f2 | 2",
                "switch to Workbench");
  push_menu_row(columns, "runtime | run | rt | f3 | 3",
                "switch to Runtime device/job view");
  push_menu_row(columns, "lattice | lat | proof | f4 | 4",
                "switch to Lattice evidence view");
  push_menu_row(columns, "logs | log | shell logs | events | f8 | 8",
                "switch to F8 Shell Logs");
  push_menu_row(columns, "config | cfg | f9 | 9", "switch to Config catalog");
  push_menu_row(columns, "show | iinuji.show()", "show active screen summary");
  push_menu_row(columns, "show home | iinuji.show.home()",
                "switch to Home and show summary");
  push_menu_row(columns, "home show | show home", "natural Home show alias");
  push_menu_row(columns, "show workbench | iinuji.show.workbench()",
                "switch to Workbench and show summary");
  push_menu_row(columns, "workbench show | show workbench",
                "natural Workbench show alias");
  push_menu_row(columns, "show runtime | iinuji.show.runtime()",
                "switch to Runtime and show summary");
  push_menu_row(columns, "runtime show | show runtime",
                "natural Runtime show alias");
  push_menu_row(columns, "show lattice | iinuji.show.lattice()",
                "switch to Lattice and show summary");
  push_menu_row(columns, "show target | show proof",
                "selected Lattice target proof");
  push_menu_row(columns, "lattice show | proof show",
                "natural Lattice proof alias");
  push_menu_row(columns, "show shell logs | iinuji.show.logs()",
                "switch to Shell Logs and show summary");
  push_menu_row(columns, "logs show | shell logs show",
                "natural Shell Logs show alias");
  push_menu_row(columns, "show config | iinuji.show.config()",
                "switch to Config and show summary");
  push_menu_row(columns, "config show | show config",
                "natural Config show alias");
  push_menu_row(columns, "refresh | reload | r",
                "refresh snapshots and telemetry");
  push_menu_row(columns, "help down | help page down",
                "scroll help overlay with natural aliases");
  push_menu_row(columns, "menu down | menu page down",
                "scroll menu overlay with natural aliases");
  push_menu_row(columns, "clear logs | logs clear | clear",
                "clear shell log feed; iinuji.logs.clear()");
  push_menu_row(columns, "logs down | logs page down",
                "scroll Shell Logs stream with natural aliases");
  push_menu_row(columns, "logs filter | source next",
                "iinuji.logs.settings.source.next()");
  push_menu_row(columns, "logs level | severity",
                "cycle Shell Logs severity lens");
  push_menu_row(columns, "logs meta | metadata filter",
                "cycle Shell Logs metadata lens");
  push_menu_row(columns, "logs metadata | thread",
                "toggle metadata fields or thread column");
  push_menu_row(columns, "logs color | mouse",
                "toggle color emphasis or mouse capture");
  push_menu_row(columns, "logs time | timestamp",
                "toggle Shell Logs timestamps");
  push_menu_row(columns, "logs follow | follow",
                "toggle Shell Logs follow mode");
  push_menu_row(columns, "runtime down / up", "select Runtime browser rows");
  push_menu_row(columns, "runtime right / left",
                "switch Runtime Devices/Jobs focus");
  push_menu_row(columns, "lattice down / up", "browse Lattice target rows");
  push_menu_row(columns, "lattice target n1 / id",
                "select Lattice target by token");
  push_menu_row(columns, "lattice index(n=1)",
                "iinuji.lattice.target.index(n=1)");
  push_menu_row(columns, "config down / up", "browse Config catalog rows");
  push_menu_row(columns, "files | show file", "Config catalog/show utilities");
  push_menu_row(columns, "config file n1 / id", "select Config rows by token");
  push_menu_row(columns, "config index(n=1)", "iinuji.config.file.index(n=1)");
  push_menu_row(columns, "zoom | full | split",
                "toggle full/split panel when the screen supports it");
  push_menu_row(columns, "a | actions | palette | commands",
                "open navigable action menu");
  push_menu_row(columns, "actions down / palette down / commands down",
                "move action menu selection");
  push_menu_row(columns, "actions run",
                "open palette; run selected row when palette is open");
  push_menu_row(columns, "quit | exit | q | x", "exit command shell");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "CANONICAL PATHS");
  push_menu_row(columns, "iinuji.screen.*()", "screen command paths");
  push_menu_row(columns, "iinuji.show.*()",
                "show Home, Workbench, Runtime, Lattice, Logs, Config");
  push_menu_row(columns, "iinuji.home.visual()",
                "alias iinuji.home.animation()");
  push_menu_row(columns, "iinuji.home.splash()", "alias iinuji.splash()");
  push_menu_row(columns, "iinuji.home.farewell()", "alias iinuji.farewell()");
  push_menu_row(columns, "iinuji.refresh()", "refresh current snapshots");
  push_menu_row(columns, "iinuji.workbench.refresh()",
                "Workbench refresh path");
  push_menu_row(columns, "iinuji.runtime.refresh()",
                "refresh Runtime telemetry and jobs");
  push_menu_row(columns, "iinuji.lattice.refresh()",
                "refresh Lattice target/fact counters");
  push_menu_row(columns, "iinuji.config.reload()",
                "reload managed Config catalog");
  push_menu_row(columns, "iinuji.actions.*()",
                "open, close, move, and run action palette");
  push_menu_row(columns, "iinuji.commands.*()",
                "commands namespace for action palette");
  push_menu_row(columns, "iinuji.workspace.*()",
                "toggle full/split panel layouts");
  push_menu_row(columns, "iinuji.workspace.zoom.toggle()",
                "toggle full/split panel layout");
  push_menu_row(columns, "iinuji.workspace.split()",
                "restore split panel layout");
  push_menu_row(columns, "iinuji.runtime.devices()",
                "focus Runtime device telemetry");
  push_menu_row(columns, "iinuji.runtime.jobs()", "focus Runtime jobs");
  push_menu_row(columns, "iinuji.runtime.detail.next()",
                "cycle Runtime manifest/log/trace detail");
  push_menu_row(columns, "iinuji.runtime.row.*()",
                "switch Runtime Devices/Jobs focus");
  push_menu_row(columns, "iinuji.runtime.item.*()",
                "select Runtime host/gpu/job rows");
  push_menu_row(columns, "iinuji.logs.settings.*()",
                "move and toggle Shell Logs controls");
  push_menu_row(columns, "iinuji.logs.settings.source.next()",
                "cycle Shell Logs source lens");
  push_menu_row(columns, "iinuji.logs.settings.source.*()",
                "set Shell Logs source lens directly");
  push_menu_row(columns, "iinuji.logs.settings.level.*()",
                "set Shell Logs severity lens directly");
  push_menu_row(columns, "iinuji.logs.settings.date.toggle()",
                "toggle Shell Logs timestamps");
  push_menu_row(columns, "iinuji.logs.settings.follow.toggle()",
                "toggle Shell Logs follow mode");
  push_menu_row(columns, "iinuji.logs.settings.metadata.*()",
                "metadata filters and display toggles");
  push_menu_row(columns, "iinuji.logs.settings.metadata.toggle()",
                "toggle Shell Logs metadata fields");
  push_menu_row(columns, "iinuji.logs.settings.thread.toggle()",
                "toggle Shell Logs thread column");
  push_menu_row(columns, "iinuji.logs.settings.color.toggle()",
                "toggle Shell Logs color emphasis");
  push_menu_row(columns, "iinuji.logs.settings.mouse.capture.toggle()",
                "toggle Shell Logs mouse capture");
  push_menu_row(columns, "iinuji.lattice.target.*()",
                "browse Lattice targets read-only");
  push_menu_row(columns, "iinuji.config.files()",
                "list Config catalog files read-only");
  push_menu_row(columns, "iinuji.config.show()",
                "show selected Config summary");
  push_menu_row(columns, "iinuji.config.file.*()",
                "browse Config files read-only");
  push_menu_row(columns, "iinuji.help.close()", "explicitly close help menu");
  push_menu_row(columns, "iinuji.quit() / iinuji.exit()", "exit command shell");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "COMMAND LINE");
  push_menu_row(columns, "type / :", "enter command mode",
                text_line_emphasis_t::Warning);
  push_menu_row(columns, "cuwacunu_cmd --menu", "open menu overlay");
  push_menu_row(columns, "cuwacunu.cmd --menu", "open menu overlay");
  push_menu_row(columns, "cuwacunu_cmd --help / --version",
                "menu or command identity");
  push_menu_row(columns, "cuwacunu.cmd --help / --version",
                "menu or command identity");
  push_menu_row(columns, "cuwacunu_cmd --actions / --palette",
                "open the action palette from the in-app prompt");
  push_menu_row(columns, "cuwacunu.cmd --actions / --palette",
                "open the action palette from the in-app prompt");
  push_menu_row(columns, "cuwacunu_cmd --commands / --catalog",
                "print the active command catalog outside the TUI");
  push_menu_row(columns, "cuwacunu.cmd --commands / --catalog",
                "print the active command catalog outside the TUI");
  push_menu_row(columns, "cuwacunu_cmd runtime --snapshot",
                "switch to Runtime before rendering");
  push_menu_row(columns, "cuwacunu.cmd runtime --snapshot",
                "switch to Runtime before rendering");
  push_menu_row(columns, "iinuji_cmd runtime --snapshot",
                "switch to Runtime before rendering");
  push_menu_row(columns, "cuwacunu_cmd --screen runtime",
                "switch screens by name");
  push_menu_row(columns, "cuwacunu.cmd --screen runtime",
                "switch screens by name");
  push_menu_row(columns, "cuwacunu_cmd --run/--command \"show\"",
                "run a command before snapshot or TUI start");
  push_menu_row(columns, "cuwacunu.cmd --run/--command \"show\"",
                "run a command before snapshot or TUI start");
  push_menu_row(columns, ":runtime",
                "prefix aliases that collide with quick keys");
  push_menu_row(columns, "Enter", "run typed command");
  push_menu_row(columns, "Ctrl-U / Backspace",
                "clear typed command / delete last character");
  push_menu_row(columns, "Up / Down", "recall command history in command mode");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "CURRENT SCREEN");
  append_menu_current_screen_rows(columns, state);
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);

  push_menu_header(columns, "MENU SCROLL UTILITIES");
  push_menu_row(columns, "help/menu up | down",
                "scroll menu/help overlay one step");
  push_menu_row(columns, "help/menu page up | page down",
                "page menu/help overlay");
  push_menu_row(columns, "help/menu left | right",
                "horizontal menu/help overlay scroll");
  push_menu_row(columns, "help/menu home | end",
                "jump menu/help overlay to first or final rows");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);
  push_menu_header(columns, "ACTION PALETTE UTILITIES");
  push_menu_row(columns, "actions up | down",
                "move action palette selection directly");
  push_menu_row(columns, "actions page up | page down",
                "page action palette selection directly");
  push_menu_row(columns, "actions home | end",
                "jump action palette selection directly");
  push_menu_row(columns, "actions close | run",
                "close palette or run selected row directly");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);
  push_menu_header(columns, "RUNTIME UTILITIES");
  push_menu_row(columns, "runtime devices | jobs",
                "focus Runtime lanes directly");
  push_menu_row(columns, "runtime manifest | log | trace",
                "choose Runtime detail lens directly");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);
  push_menu_header(columns, "SHELL LOG UTILITIES");
  push_menu_row(columns, "logs setting prev | next",
                "move Shell Logs setting cursor directly");
  push_menu_row(columns, "logs setting left | right",
                "change selected Shell Logs setting directly");
  push_menu_row(columns, "logs up | down", "scroll Shell Logs stream directly");
  push_menu_row(columns, "logs page up | page down",
                "page Shell Logs stream directly");
  push_menu_row(columns, "logs home | end", "jump Shell Logs stream directly");
  push_menu_row(columns, "logs source all | refresh",
                "set Shell Logs source lens directly");
  push_menu_row(columns, "logs source action | command",
                "set Shell Logs source lens directly");
  push_menu_row(columns, "logs source show | status",
                "set Shell Logs source lens directly");
  push_menu_row(columns, "logs debug | info | warning",
                "set Shell Logs severity lens directly");
  push_menu_row(columns, "logs error | fatal",
                "set Shell Logs severity lens directly");
  push_menu_row(columns, "logs meta any | any_meta",
                "set Shell Logs metadata lens directly");
  push_menu_row(columns, "logs meta function | path",
                "set Shell Logs metadata lens directly");
  push_menu_row(columns, "logs meta callsite",
                "set Shell Logs metadata lens directly");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);
  push_menu_header(columns, "SHELL SNAPSHOTS");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --menu",
                "render menu overlay snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --menu",
                "render menu overlay snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --actions",
                "render action palette snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --actions",
                "render action palette snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --catalog",
                "render command catalog snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --catalog",
                "render command catalog snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --visual",
                "render Home visual snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --visual",
                "render Home visual snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --image",
                "render Home still image snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --image",
                "render Home still image snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --animation",
                "render Home APNG motion snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --animation",
                "render Home APNG motion snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --splash loading",
                "render bootstrap splash snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --splash loading",
                "render bootstrap splash snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --splash close",
                "render closing splash snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --splash close",
                "render closing splash snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --splash good-luck",
                "render Good luck splash snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --splash good-luck",
                "render Good luck splash snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --screen runtime",
                "render Runtime screen snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --screen runtime",
                "render Runtime screen snapshot");
  push_menu_row(columns, "cuwacunu_cmd --snapshot --full",
                "render full-panel snapshot");
  push_menu_row(columns, "cuwacunu.cmd --snapshot --full",
                "render full-panel snapshot");
  push_menu_row(columns, "cuwacunu_cmd hero.runtime.dsl --snapshot",
                "open managed Config path through shell snapshot");
  push_menu_row(columns, "cuwacunu.cmd hero.runtime.dsl --snapshot",
                "open managed Config path through shell snapshot");
  push_menu_row(columns, "", "", text_line_emphasis_t::None,
                text_line_emphasis_t::None);
  push_menu_header(columns, "IDENTITY");
  push_menu_row(columns, "version | about",
                "iinuji.version() / cuwacunu_cmd --version");
  push_menu_row(columns, "cuwacunu.cmd --version", "show command identity");

  return columns;
}

inline std::string make_detail_text(const CmdState &state) {
  if (state.help_view)
    return make_help_text(state);
  if (state.screen == ScreenMode::Workbench)
    return {};
  std::ostringstream out;
  if (state.screen == ScreenMode::Home) {
    out << "disclosures\n\n";
  } else {
    out << screen_label(state.screen) << " detail\n\n";
  }
  switch (state.screen) {
  case ScreenMode::Runtime: {
    out << make_runtime_detail_text(state);
    break;
  }
  case ScreenMode::Config:
    out << make_config_detail_text(state);
    break;
  case ScreenMode::Lattice:
    out << make_lattice_detail_text(state);
    break;
  case ScreenMode::Workbench:
    break;
  case ScreenMode::Home:
    out << make_home_detail_body_text(state);
    break;
  case ScreenMode::Logs:
    out << make_logs_control_deck_text(state);
    break;
  }
  return out.str();
}

inline std::vector<std::string> split_styled_content_lines(std::string text) {
  std::vector<std::string> lines{};
  std::size_t start = 0u;
  while (start <= text.size()) {
    const std::size_t end = text.find('\n', start);
    if (end == std::string::npos) {
      lines.push_back(text.substr(start));
      break;
    }
    lines.push_back(text.substr(start, end - start));
    start = end + 1u;
  }
  if (lines.empty())
    lines.emplace_back();
  return lines;
}

inline std::string trim_for_style(std::string_view text) {
  std::size_t first = 0u;
  while (first < text.size() &&
         (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) {
    ++first;
  }
  std::size_t last = text.size();
  while (last > first && (text[last - 1u] == ' ' || text[last - 1u] == '\t' ||
                          text[last - 1u] == '\r')) {
    --last;
  }
  return std::string(text.substr(first, last - first));
}

inline bool style_starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0u, prefix.size()) == prefix;
}

inline bool style_contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

inline bool is_animated_rule_line(std::string_view trimmed) {
  for (const std::string_view glyph : breathing_dot_glyphs()) {
    if (trimmed == glyph)
      return true;
  }
  if (trimmed.size() < 6u)
    return false;
  for (const char ch : trimmed) {
    if (std::string_view{".:-=+*#"}.find(ch) == std::string_view::npos)
      return false;
  }
  return true;
}

inline bool is_content_section_heading(std::string_view trimmed) {
  if (trimmed.empty())
    return false;
  static constexpr std::string_view kHeadings[]{
      "Home",
      "waajacamaya",
      "disclosures",
      "Worklist",
      "Runtime",
      "Active wave",
      "Lattice",
      "Workbench",
      "Config",
      "Logs",
      "Menu",
      "Global",
      "Screens",
      "Canonical command paths",
      "COMMAND PATHS",
      "Shell Logs Control Deck",
      "Capture",
      "Lens",
      "Settings",
      "Selected",
      "Signal Mix",
      "Metadata Mix",
      "Severity Mix",
      "Quick Keys",
      "host",
      "gpus",
      "compute processes",
      "job_id",
      "artifacts",
      "report preview",
      "job state preview",
      "report",
      "checkpoint",
      "dir",
      "device telemetry",
      "job_state",
      "report tail",
      "trace preview",
      "runtime policy",
      "active wave",
      "manifest fallback",
      "selected config file",
      "selected lattice target",
      "preview",
      "target block",
  };
  for (const auto heading : kHeadings) {
    if (trimmed == heading)
      return true;
  }
  return style_contains(trimmed, " detail");
}

inline std::string trim_selection_marker_for_style(std::string_view line) {
  std::string trimmed = trim_for_style(line);
  while (!trimmed.empty() && trimmed.front() == '>')
    trimmed.erase(trimmed.begin());
  return trim_for_style(trimmed);
}

inline text_line_emphasis_t styled_content_emphasis(std::string_view line) {
  const std::string trimmed = trim_selection_marker_for_style(line);
  if (trimmed.empty())
    return text_line_emphasis_t::None;
  if (style_starts_with(trimmed, "[F"))
    return text_line_emphasis_t::Warning;
  if (is_animated_rule_line(trimmed))
    return text_line_emphasis_t::Debug;
  if (is_content_section_heading(trimmed))
    return text_line_emphasis_t::Accent;
  if (style_starts_with(trimmed, "[refresh]"))
    return text_line_emphasis_t::Info;
  if (style_starts_with(trimmed, "[action]"))
    return text_line_emphasis_t::Accent;
  if (style_starts_with(trimmed, "[command]"))
    return text_line_emphasis_t::Warning;
  if (style_contains(trimmed, "fatal") || style_contains(trimmed, "failed") ||
      style_contains(trimmed, "error") ||
      style_contains(trimmed, "unavailable") ||
      style_contains(trimmed, "missing"))
    return text_line_emphasis_t::Error;
  if (style_contains(trimmed, "completed") ||
      style_contains(trimmed, "exec exists: yes") ||
      style_contains(trimmed, " executable: yes"))
    return text_line_emphasis_t::Success;
  if (style_contains(trimmed, "reserved") || style_contains(trimmed, "n/a") ||
      style_contains(trimmed, "none"))
    return text_line_emphasis_t::MutedWarning;
  if (style_contains(trimmed, ":") || style_contains(trimmed, "="))
    return text_line_emphasis_t::Info;
  return text_line_emphasis_t::None;
}

inline std::string styled_content_background(std::string_view line) {
  const std::string trimmed = trim_for_style(line);
  if (style_starts_with(trimmed, "[F"))
    return "#171D22";
  if (style_contains(trimmed, "fatal") || style_contains(trimmed, "failed") ||
      style_contains(trimmed, "error") || style_contains(trimmed, "missing"))
    return "#24141A";
  if (style_contains(trimmed, "reserved"))
    return "#201B13";
  return {};
}

inline bool styled_selection_marker_span(std::string_view line,
                                         std::size_t &offset,
                                         std::size_t &length) {
  const std::size_t first = line.find_first_not_of(" \t\r");
  if (first == std::string_view::npos || line[first] != '>')
    return false;

  std::size_t end = first;
  while (end < line.size() && line[end] == '>')
    ++end;

  offset = first;
  length = end - first;
  return length > 0u;
}

inline std::vector<styled_text_line_t> style_content_text(std::string text) {
  std::vector<styled_text_line_t> styled{};
  const auto lines = split_styled_content_lines(std::move(text));
  styled.reserve(lines.size());
  for (const auto &line : lines) {
    const text_line_emphasis_t emphasis = styled_content_emphasis(line);
    const std::string background = styled_content_background(line);
    std::size_t marker_offset = 0u;
    std::size_t marker_length = 0u;
    if (styled_selection_marker_span(line, marker_offset, marker_length)) {
      std::vector<styled_text_segment_t> segments{};
      segments.reserve(3);
      if (marker_offset > 0u) {
        segments.push_back(styled_text_segment_t{
            .text = line.substr(0u, marker_offset),
            .emphasis = text_line_emphasis_t::None,
        });
      }
      segments.push_back(styled_text_segment_t{
          .text = line.substr(marker_offset, marker_length),
          .emphasis = text_line_emphasis_t::Warning,
      });
      const std::size_t rest_offset = marker_offset + marker_length;
      if (rest_offset < line.size()) {
        segments.push_back(styled_text_segment_t{
            .text = line.substr(rest_offset),
            .emphasis = emphasis,
        });
      }
      styled_text_line_t styled_line =
          make_segmented_styled_text_line(std::move(segments), emphasis);
      styled_line.background_color = background;
      styled.push_back(std::move(styled_line));
      continue;
    }
    styled.push_back(styled_text_line_t{
        .text = line,
        .emphasis = emphasis,
        .background_color = background,
    });
  }
  return styled;
}

inline std::string make_main_text(const CmdState &state) {
  switch (state.screen) {
  case ScreenMode::Home:
    return make_home_text(state);
  case ScreenMode::Workbench:
    return make_workbench_text();
  case ScreenMode::Runtime:
    return make_runtime_text(state);
  case ScreenMode::Lattice:
    return make_lattice_text(state);
  case ScreenMode::Config:
    return make_config_text(state);
  case ScreenMode::Logs:
    return make_logs_text(state);
  }
  return {};
}

inline std::vector<styled_text_line_t>
make_main_styled_lines(const CmdState &state) {
  if (state.screen == ScreenMode::Logs && !state.logs.show_color) {
    std::vector<styled_text_line_t> styled{};
    const auto lines = split_styled_content_lines(make_logs_text(state));
    styled.reserve(lines.size());
    for (const auto &line : lines)
      styled.push_back(styled_text_line_t{.text = line});
    return styled;
  }
  return style_content_text(make_main_text(state));
}

inline std::vector<styled_text_line_t>
make_detail_styled_lines(const CmdState &state) {
  return style_content_text(make_detail_text(state));
}

inline std::string command_prompt_status(const CmdState &) { return "cmd>"; }

inline std::string make_command_line_snapshot_text(const CmdState &state) {
  std::ostringstream out;
  append_detail_section_header(out, "Command");
  out << " " << command_prompt_status(state) << " ";
  if (state.command_mode) {
    out << "editing";
  } else {
    out << "ready";
  }
  if (!state.command_history.empty())
    out << " | history=" << state.command_history.size();
  out << "\n";
  if (!state.command_history.empty())
    out << " last: " << state.command_history.back() << "\n";
  out << " type command or : mode | Enter run | Ctrl-U clear | Up/Down history "
         "| ";
  if (state.command_mode)
    out << "Esc cancel command\n";
  else if (state.help_view || state.action_menu_open ||
           cmd_choice_menu_open(state) || cmd_content_popup_open(state))
    out << "Esc/[x] close overlay\n";
  else if (workspace_is_current_screen_zoomed(state))
    out << "Esc restore split\n";
  else
    out << "Esc/[x] close overlay when open\n";
  return out.str();
}

inline std::string workspace_mode_text(const CmdState &state) {
  if (!workspace_screen_supports_zoom(state.screen))
    return "fixed";
  return workspace_is_current_screen_zoomed(state) ? "full" : "split";
}

inline std::size_t visible_log_count(const CmdState &state) {
  std::size_t shown = 0;
  for (const auto &entry : state.log) {
    if (logs_accept_entry(state, entry))
      ++shown;
  }
  return shown;
}

inline std::string compact_text(std::string value, std::size_t max_size) {
  if (value.size() <= max_size)
    return value;
  if (max_size == 0)
    return {};
  if (max_size == 1)
    return "~";
  value.resize(max_size - 1);
  value.push_back('~');
  return value;
}

inline std::string compact_status_text(const CmdState &state) {
  std::string status = display_or(state.status, "ready");
  if (state.help_view && status.rfind("help overlay=open", 0) == 0)
    status = "help overlay";
  return compact_text(std::move(status), 32);
}

inline text_line_emphasis_t operator_status_emphasis(const CmdState &state) {
  const std::string status = display_or(state.status, "ready");
  if (status.find("unknown") != std::string::npos ||
      status.find("failed") != std::string::npos ||
      status.find("error") != std::string::npos) {
    return text_line_emphasis_t::Error;
  }
  if (status.find("queued") != std::string::npos ||
      status.find("refreshed") != std::string::npos ||
      status.find("split") != std::string::npos ||
      status.find("full") != std::string::npos ||
      status.find("cleared") != std::string::npos) {
    return text_line_emphasis_t::Success;
  }
  if (state.action_menu_open || state.help_view ||
      cmd_choice_menu_open(state) || cmd_content_popup_open(state) ||
      state.command_mode)
    return text_line_emphasis_t::Accent;
  return text_line_emphasis_t::Info;
}

inline std::string make_operator_state_line(const CmdState &state) {
  std::ostringstream out;
  out << " " << screen_key_label(state.screen) << " "
      << screen_badge_label(state.screen) << " | "
      << compact_status_text(state);

  if (state.help_view)
    out << " | menu";
  if (state.action_menu_open)
    out << " | actions";
  if (cmd_choice_menu_open(state))
    out << " | menu";
  if (cmd_content_popup_open(state))
    out << " | popup";
  if (state.command_mode)
    out << " | edit";

  switch (state.screen) {
  case ScreenMode::Home:
    out << " | jobs=" << state.runtime.jobs.size()
        << " cfg=" << state.config.files.size() << " "
        << home_visual_mode_text(state);
    break;
  case ScreenMode::Workbench:
    break;
  case ScreenMode::Runtime:
    out << " | focus=" << runtime_focus_label(state.runtime.focus)
        << " detail=" << runtime_detail_mode_label(state.runtime.detail_mode)
        << " | jobs=" << state.runtime.jobs.size()
        << " gpu=" << state.runtime.device.gpus.size()
        << " samples=" << state.runtime.device_history.size() << " "
        << workspace_mode_text(state);
    break;
  case ScreenMode::Lattice:
    out << " | configured_targets=" << state.lattice.target_ids.size()
        << " runtime_facts=" << state.lattice.exposure_fact_count << "/"
        << state.lattice.checkpoint_fact_count;
    if (state.lattice.exposure_fact_count == 0u &&
        state.lattice.checkpoint_fact_count == 0u)
      out << " empty";
    if (!state.lattice.target_ids.empty()) {
      out << " selected=" << (state.lattice.selected_target + 1) << "/"
          << state.lattice.target_ids.size() << " family="
          << lattice_target_family_title(
                 selected_lattice_target_family_id(state));
    }
    out << " " << workspace_mode_text(state);
    break;
  case ScreenMode::Logs:
    out << " | log=" << visible_log_count(state) << "/" << state.log.size()
        << " src=" << logs_source_filter_label(state.logs.source_filter)
        << " level=" << logs_severity_filter_label(state.logs.severity_filter)
        << " meta=" << logs_metadata_filter_label(state.logs.metadata_filter)
        << " | follow=" << on_off(state.logs.auto_follow)
        << " time=" << on_off(state.logs.show_timestamp) << " "
        << workspace_mode_text(state);
    break;
  case ScreenMode::Config:
    out << " | files=" << state.config.files.size();
    if (!state.config.files.empty()) {
      out << " selected=" << (state.config.selected_file + 1) << "/"
          << state.config.files.size() << " family="
          << config_catalog_family_title(
                 selected_config_catalog_family_id(state));
    }
    out << " " << workspace_mode_text(state);
    break;
  }
  return out.str();
}

inline std::string home_operator_hint_line() {
  return " home | ready | F2 workbench | F3 runtime | F4 lattice | F8 shell "
         "logs | F9 config | waajacu™";
}

inline std::string workbench_operator_hint_line() {
  return " workbench | F3 runtime | F4 lattice | F8 shell logs | F9 config | "
         "h/H/? menu | a / actions | type/: command";
}

inline std::string runtime_operator_hint_line(const CmdState &state) {
  if (state.runtime.focus == RuntimeFocus::Devices) {
    return " lane=Devices | Up/Down browses host and gpu rows. Tab/Shift-Tab "
           "cycles manifest/log/trace. Enter opens menu. lower panel shows "
           "live history. f/z widens detail panel; Esc restores split. r "
           "refreshes runtime inventory.";
  }
  return " lane=Jobs | Up/Down browses jobs. Enter opens file menu; a opens "
         "actions. f/z widens details panel; Esc restores split. r refreshes "
         "runtime inventory.";
}

inline std::string lattice_operator_hint_line(const CmdState &state) {
  std::ostringstream out;
  out << " section=targets";
  if (!state.lattice.target_ids.empty()) {
    out << " | row=" << (state.lattice.selected_target + 1) << "/"
        << state.lattice.target_ids.size();
  } else {
    out << " | row=0/0";
  }
  out << " | Up/Down browse targets. Home/End jump edges. Enter shows target "
         "proof. PgUp/PgDn scroll evidence. f/z toggles full screen. Esc "
         "restores split. r refreshes catalogs.";
  return out.str();
}

inline std::string logs_operator_hint_line(const CmdState &state) {
  return std::string(" logs | filter=") +
         logs_severity_filter_label(state.logs.severity_filter) +
         " | metadata=" +
         logs_metadata_filter_label(state.logs.metadata_filter) +
         (state.logs.auto_follow ? " | follow=on" : " | follow=off") +
         " | Up/Down settings. Left/Right changes. f/z toggles full screen. "
         "Esc restores split. Home/End jump oldest/newest.";
}

inline std::string config_operator_hint_line(const CmdState &state) {
  std::ostringstream out;
  out << " config | files focus";
  if (!state.config.files.empty()) {
    out << " | row=" << (state.config.selected_file + 1) << "/"
        << state.config.files.size();
  } else {
    out << " | row=0/0";
  }
  out << " | Up/Down browse files. Home/End jump edges. Enter/e opens "
         "read-only "
         "preview. f/z toggles full screen. Esc restores split. r reloads "
         "active policy.";
  return out.str();
}

inline std::string make_operator_hint_line(const CmdState &state) {
  if (state.action_menu_open) {
    return " actions: Enter/run | Up/Down/j/k move | Pg/Home/End edge | "
           "Esc/[x] close";
  }
  if (cmd_choice_menu_open(state)) {
    return " menu: Enter choose | Up/Down/j/k move | Pg/Home/End edge | "
           "Esc/[x] close";
  }
  if (cmd_content_popup_open(state)) {
    return " popup: arrows/hjkl scroll | Pg/Home/End jump | Esc/[x] close";
  }
  if (state.help_view) {
    return " menu: arrows/hjkl scroll | Pg/Home/End jump | a / actions | "
           "Esc/[x] close";
  }
  if (state.command_mode) {
    return " command: Enter run | Esc cancel | Ctrl-U clear | Up/Down history";
  }

  switch (state.screen) {
  case ScreenMode::Home:
    return home_operator_hint_line();
  case ScreenMode::Workbench:
    return workbench_operator_hint_line();
  case ScreenMode::Runtime:
    return runtime_operator_hint_line(state);
  case ScreenMode::Lattice:
    return lattice_operator_hint_line(state);
  case ScreenMode::Logs:
    return logs_operator_hint_line(state);
  case ScreenMode::Config:
    return config_operator_hint_line(state);
  }
  return " ";
}

inline std::string make_bottom_hint_text(const CmdState &state) {
  std::ostringstream out;
  out << make_operator_state_line(state) << "\n"
      << make_operator_hint_line(state);
  return out.str();
}

inline std::vector<styled_text_line_t>
make_operator_status_styled_lines(const CmdState &state) {
  return {styled_text_line_t{
              .text = make_operator_state_line(state),
              .emphasis = operator_status_emphasis(state),
          },
          styled_text_line_t{
              .text = make_operator_hint_line(state),
              .emphasis = text_line_emphasis_t::Info,
          }};
}

inline std::vector<styled_text_line_t>
make_bottom_status_styled_lines(const CmdState &state) {
  text_line_emphasis_t emphasis = text_line_emphasis_t::Info;
  if (operator_status_emphasis(state) == text_line_emphasis_t::Error)
    emphasis = text_line_emphasis_t::Error;
  else if (state.action_menu_open || state.help_view ||
           cmd_choice_menu_open(state) || cmd_content_popup_open(state) ||
           state.command_mode)
    emphasis = text_line_emphasis_t::Accent;
  return {styled_text_line_t{
      .text = make_operator_hint_line(state),
      .emphasis = emphasis,
  }};
}

inline void append_snapshot_status(std::ostringstream &out,
                                   const CmdState &state) {
  append_detail_section_header(out, "Status");
  out << make_bottom_hint_text(state) << "\n";
}

inline bool snapshot_should_render_main_panel(const CmdState &state) {
  return !workspace_is_current_screen_zoomed(state) ||
         workspace_current_screen_uses_left_panel_zoom(state);
}

inline bool snapshot_should_render_detail_panel(const CmdState &state) {
  return !workspace_is_current_screen_zoomed(state) ||
         !workspace_current_screen_uses_left_panel_zoom(state);
}

inline void append_snapshot_workspace(std::ostringstream &out,
                                      const CmdState &state) {
  if (snapshot_should_render_main_panel(state)) {
    const std::string main_text = make_main_text(state);
    if (!main_text.empty())
      out << main_text << "\n---\n";
  }
  if (snapshot_should_render_detail_panel(state)) {
    const std::string detail_text = make_detail_text(state);
    if (!detail_text.empty())
      out << detail_text << "\n---\n";
  }
}

inline std::string make_snapshot_text(const CmdState &state) {
  std::ostringstream out;
  append_snapshot_shell_chrome(out, state);
  append_snapshot_status(out, state);
  out << "---\n";
  append_snapshot_workspace(out, state);
  out << make_command_line_snapshot_text(state);
  return out.str();
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
