#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "hero/marshal_hero/hero_marshal_tools.h"
#include "hero/marshal_hero/marshal_session.h"
#include "hero/runtime_hero/hero_runtime_tools.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iinuji/iinuji_types.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class RuntimeDetailMode : std::uint8_t {
  Manifest = 0,
  Log = 1,
  Trace = 2
};
enum class RuntimeLogViewerKind : std::uint8_t {
  None = 0,
  JobStdout = 1,
  JobStderr = 2,
  MarshalEvents = 3,
  MarshalCodexStdout = 4,
  MarshalCodexStderr = 5,
  CampaignStdout = 6,
  CampaignStderr = 7,
  JobTrace = 8,
  ArtifactFile = 9,
};

enum class RuntimeLane : std::uint8_t {
  Device = 0,
  Session = 1,
  Campaign = 2,
  Job = 3
};
using RuntimeRowKind = RuntimeLane;

inline constexpr RuntimeLane kRuntimeDeviceLane = RuntimeLane::Device;
inline constexpr RuntimeLane kRuntimeSessionLane = RuntimeLane::Session;
inline constexpr RuntimeLane kRuntimeCampaignLane = RuntimeLane::Campaign;
inline constexpr RuntimeLane kRuntimeJobLane = RuntimeLane::Job;

enum class RuntimePanelFocus : std::uint8_t { Menu = 0, Worklist = 1 };

inline constexpr RuntimePanelFocus kRuntimeMenuFocus = RuntimePanelFocus::Menu;
inline constexpr RuntimePanelFocus kRuntimeWorklistFocus =
    RuntimePanelFocus::Worklist;

inline bool runtime_is_device_lane(RuntimeLane lane) {
  return lane == kRuntimeDeviceLane;
}

inline bool runtime_is_session_lane(RuntimeLane lane) {
  return lane == kRuntimeSessionLane;
}

inline bool runtime_is_campaign_lane(RuntimeLane lane) {
  return lane == kRuntimeCampaignLane;
}

inline bool runtime_is_job_lane(RuntimeLane lane) {
  return lane == kRuntimeJobLane;
}

inline bool runtime_is_menu_focus(RuntimePanelFocus focus) {
  return focus == kRuntimeMenuFocus;
}

inline bool runtime_is_worklist_focus(RuntimePanelFocus focus) {
  return focus == kRuntimeWorklistFocus;
}

inline std::string runtime_lane_label(RuntimeLane lane) {
  switch (lane) {
  case RuntimeLane::Device:
    return "Devices";
  case RuntimeLane::Session:
    return "Marshals";
  case RuntimeLane::Campaign:
    return "Campaigns";
  case RuntimeLane::Job:
    return "Jobs";
  }
  return "Marshals";
}

inline std::string runtime_focus_label(RuntimePanelFocus focus) {
  return runtime_is_menu_focus(focus) ? "menu" : "worklist";
}

struct RuntimeRefreshSnapshot {
  bool ok{false};
  std::string error{};
  std::string session_list_error{};
  std::string job_list_error{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
};

struct RuntimeExcerptSnapshot {
  std::string job_cursor{};
  RuntimeDetailMode detail_mode{RuntimeDetailMode::Manifest};
  std::string stdout_excerpt{};
  std::string stderr_excerpt{};
  std::string trace_excerpt{};
};

struct RuntimeDeviceGpuSnapshot {
  std::string index{};
  std::string name{};
  std::string uuid{};
  int util_gpu_pct{-1};
  int util_mem_pct{-1};
  std::uint64_t memory_used_mib{0};
  std::uint64_t memory_total_mib{0};
  int temperature_c{-1};
};

struct RuntimeDeviceGpuProcessSnapshot {
  std::uint64_t pid{0};
  std::string gpu_uuid{};
  std::uint64_t used_memory_mib{0};
};

struct RuntimeMarshalEventViewerEntry {
  std::uint64_t timestamp_ms{0};
  std::string event_name{};
  std::string summary{};
  std::vector<std::pair<std::string, std::string>> metadata{};
  std::vector<std::pair<std::string, std::string>> payload{};
  cuwacunu::iinuji::text_line_emphasis_t emphasis{
      cuwacunu::iinuji::text_line_emphasis_t::Info};
  std::string raw_line{};
};

struct RuntimeMarshalEventViewerState {
  std::vector<RuntimeMarshalEventViewerEntry> entries{};
  std::size_t selected_entry{0};
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
  std::vector<RuntimeDeviceGpuSnapshot> gpus{};
  std::vector<RuntimeDeviceGpuProcessSnapshot> gpu_processes{};
  std::uint64_t collected_at_ms{0};
};

struct RuntimeState {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::runtime_mcp::app_context_t app{};
  cuwacunu::hero::marshal_mcp::app_context_t marshal_app{};
  bool marshal_ok{false};
  std::string marshal_error{};
  RuntimeLane lane{kRuntimeSessionLane};
  RuntimeLane family_anchor_lane{kRuntimeSessionLane};
  RuntimePanelFocus focus{kRuntimeMenuFocus};
  bool campaign_filter_active{false};
  bool job_filter_active{false};
  std::size_t selected_device_index{0};
  std::string anchor_session_id{};
  std::string selected_session_id{};
  std::string selected_campaign_cursor{};
  std::string selected_job_cursor{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  std::unordered_map<std::string, std::string> campaign_session_by_cursor{};
  std::unordered_map<std::string, std::string> job_campaign_by_cursor{};
  std::unordered_map<std::string, std::string> job_session_by_cursor{};
  std::unordered_map<std::string, std::vector<std::size_t>>
      campaign_indices_by_session{};
  std::unordered_map<std::string, std::vector<std::size_t>>
      job_indices_by_campaign{};
  std::unordered_map<std::string, std::vector<std::size_t>>
      job_indices_by_session{};
  std::unordered_map<std::uint64_t, std::size_t> runner_job_index_by_pid{};
  std::unordered_map<std::uint64_t, std::size_t> target_job_index_by_pid{};
  RuntimeDetailMode detail_mode{RuntimeDetailMode::Manifest};
  std::optional<cuwacunu::hero::marshal::marshal_session_record_t> session{};
  std::optional<cuwacunu::hero::runtime::runtime_campaign_record_t> campaign{};
  std::optional<cuwacunu::hero::runtime::runtime_job_record_t> job{};
  bool log_viewer_open{false};
  RuntimeLogViewerKind log_viewer_kind{RuntimeLogViewerKind::None};
  std::string log_viewer_path{};
  std::string log_viewer_title{};
  std::shared_ptr<cuwacunu::iinuji::editorBox_data_t> log_viewer{};
  RuntimeMarshalEventViewerState marshal_event_viewer{};
  bool log_viewer_live_follow{true};
  std::uint64_t log_viewer_last_poll_ms{0};
  std::uintmax_t log_viewer_last_size{0};
  std::int64_t log_viewer_last_write_tick{0};
  std::string job_stdout_excerpt{};
  std::string job_stderr_excerpt{};
  std::string job_trace_excerpt{};
  std::string status{};
  bool status_is_error{false};
  bool refresh_pending{false};
  bool refresh_requeue{false};
  std::future<RuntimeRefreshSnapshot> refresh_future{};
  bool excerpt_pending{false};
  bool excerpt_requeue{false};
  std::future<RuntimeExcerptSnapshot> excerpt_future{};
  RuntimeDeviceSnapshot device{};
  std::vector<RuntimeDeviceSnapshot> device_history{};
  bool device_pending{false};
  bool device_requeue{false};
  std::uint64_t device_last_refresh_ms{0};
  std::future<RuntimeDeviceSnapshot> device_future{};
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
