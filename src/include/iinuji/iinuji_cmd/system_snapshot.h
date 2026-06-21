#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "hero/config_path_defaults.h"
#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::uint64_t now_ms() {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

inline std::string trim(std::string text) {
  auto not_space = [](unsigned char ch) {
    return ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n';
  };
  text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
  text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(),
             text.end());
  return text;
}

inline std::string strip_trailing_semicolon(std::string text) {
  text = trim(std::move(text));
  if (!text.empty() && text.back() == ';')
    text.pop_back();
  return trim(std::move(text));
}

inline std::map<std::string, std::string>
read_assignment_file(const std::filesystem::path &path) {
  std::map<std::string, std::string> out{};
  std::ifstream in(path);
  if (!in)
    return out;
  std::string line;
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string cleaned;
    for (std::size_t i = 0; i < line.size();) {
      if (in_block_comment) {
        const std::size_t end = line.find("*/", i);
        if (end == std::string::npos)
          break;
        in_block_comment = false;
        i = end + 2;
        continue;
      }
      if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      cleaned.push_back(line[i++]);
    }
    const std::size_t hash = cleaned.find('#');
    if (hash != std::string::npos)
      cleaned.resize(hash);
    const std::size_t eq = cleaned.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = trim(cleaned.substr(0, eq));
    const std::size_t bracket_pos = key.find('[');
    if (bracket_pos != std::string::npos)
      key.resize(bracket_pos);
    const std::size_t type_pos = key.find(':');
    if (type_pos != std::string::npos)
      key.resize(type_pos);
    key = trim(std::move(key));
    if (key.empty())
      continue;
    out[key] = strip_trailing_semicolon(cleaned.substr(eq + 1));
  }
  return out;
}

inline std::string
assignment_value(const std::map<std::string, std::string> &kv,
                 const std::string &key, const std::string &fallback = {}) {
  const auto it = kv.find(key);
  return it == kv.end() ? fallback : it->second;
}

inline std::vector<std::string> split_csv_line(const std::string &line) {
  std::vector<std::string> parts{};
  std::stringstream ss(line);
  std::string part;
  while (std::getline(ss, part, ','))
    parts.push_back(trim(part));
  return parts;
}

inline int parse_int_or(const std::string &text, int fallback) {
  try {
    return std::stoi(trim(text));
  } catch (...) {
    return fallback;
  }
}

inline std::uint64_t parse_u64_or(const std::string &text,
                                  std::uint64_t fallback) {
  try {
    return static_cast<std::uint64_t>(std::stoull(trim(text)));
  } catch (...) {
    return fallback;
  }
}

inline std::string popen_capture(const std::string &command, int *status) {
  if (status)
    *status = -1;
  FILE *pipe = ::popen(command.c_str(), "r");
  if (!pipe)
    return {};
  std::array<char, 512> buffer{};
  std::string output{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    output += buffer.data();
  const int rc = ::pclose(pipe);
  if (status)
    *status = rc;
  return output;
}

struct CpuSample {
  std::uint64_t user{0};
  std::uint64_t nice{0};
  std::uint64_t system{0};
  std::uint64_t idle{0};
  std::uint64_t iowait{0};
  std::uint64_t irq{0};
  std::uint64_t softirq{0};
  std::uint64_t steal{0};

  std::uint64_t total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
  }

  std::uint64_t idle_all() const { return idle + iowait; }
};

inline std::optional<CpuSample> read_cpu_sample() {
  std::ifstream in("/proc/stat");
  std::string cpu;
  CpuSample sample{};
  if (!(in >> cpu >> sample.user >> sample.nice >> sample.system >>
        sample.idle >> sample.iowait >> sample.irq >> sample.softirq >>
        sample.steal)) {
    return std::nullopt;
  }
  if (cpu != "cpu")
    return std::nullopt;
  return sample;
}

inline int percent_from_delta(std::uint64_t value, std::uint64_t total) {
  if (total == 0)
    return -1;
  return static_cast<int>((value * 100 + total / 2) / total);
}

inline void apply_cpu_sample(RuntimeDeviceSnapshot &snapshot,
                             const CpuSample &sample) {
  snapshot.cpu_user_ticks = sample.user;
  snapshot.cpu_nice_ticks = sample.nice;
  snapshot.cpu_system_ticks = sample.system;
  snapshot.cpu_idle_ticks = sample.idle;
  snapshot.cpu_iowait_ticks = sample.iowait;
  snapshot.cpu_irq_ticks = sample.irq;
  snapshot.cpu_softirq_ticks = sample.softirq;
  snapshot.cpu_steal_ticks = sample.steal;
}

inline std::uint64_t cpu_total_ticks(const RuntimeDeviceSnapshot &snapshot) {
  return snapshot.cpu_user_ticks + snapshot.cpu_nice_ticks +
         snapshot.cpu_system_ticks + snapshot.cpu_idle_ticks +
         snapshot.cpu_iowait_ticks + snapshot.cpu_irq_ticks +
         snapshot.cpu_softirq_ticks + snapshot.cpu_steal_ticks;
}

inline std::uint64_t cpu_idle_ticks(const RuntimeDeviceSnapshot &snapshot) {
  return snapshot.cpu_idle_ticks + snapshot.cpu_iowait_ticks;
}

inline void compute_cpu_usage_from_previous(RuntimeDeviceSnapshot &current,
                                            const RuntimeDeviceSnapshot &prev) {
  const std::uint64_t prev_total = cpu_total_ticks(prev);
  const std::uint64_t current_total = cpu_total_ticks(current);
  if (current_total <= prev_total)
    return;

  const std::uint64_t total_delta = current_total - prev_total;
  const std::uint64_t prev_idle = cpu_idle_ticks(prev);
  const std::uint64_t current_idle = cpu_idle_ticks(current);
  const std::uint64_t idle_delta =
      current_idle >= prev_idle ? current_idle - prev_idle : 0;
  current.cpu_usage_pct = std::clamp(
      percent_from_delta(
          total_delta > idle_delta ? total_delta - idle_delta : 0, total_delta),
      0, 100);

  const std::uint64_t iowait_delta =
      current.cpu_iowait_ticks >= prev.cpu_iowait_ticks
          ? current.cpu_iowait_ticks - prev.cpu_iowait_ticks
          : 0;
  current.cpu_iowait_pct =
      std::clamp(percent_from_delta(iowait_delta, total_delta), 0, 100);
}

inline void read_memory(RuntimeDeviceSnapshot &snapshot) {
  std::ifstream in("/proc/meminfo");
  std::string key;
  std::uint64_t value = 0;
  std::string unit;
  while (in >> key >> value >> unit) {
    if (key == "MemTotal:")
      snapshot.mem_total_kib = value;
    else if (key == "MemAvailable:")
      snapshot.mem_available_kib = value;
  }
}

inline void read_load(RuntimeDeviceSnapshot &snapshot) {
  std::ifstream in("/proc/loadavg");
  in >> snapshot.load1 >> snapshot.load5 >> snapshot.load15;
}

inline std::vector<RuntimeGpuProcessSnapshot> collect_gpu_processes() {
  int status = -1;
  const std::string output =
      popen_capture("nvidia-smi --query-compute-apps=pid,gpu_uuid,used_memory "
                    "--format=csv,noheader,nounits 2>/dev/null",
                    &status);
  std::vector<RuntimeGpuProcessSnapshot> processes{};
  std::stringstream ss(output);
  std::string line;
  while (std::getline(ss, line)) {
    const auto parts = split_csv_line(line);
    if (parts.size() < 3)
      continue;
    RuntimeGpuProcessSnapshot process{};
    process.pid = parse_u64_or(parts[0], 0);
    process.gpu_uuid = parts[1];
    process.used_memory_mib = parse_u64_or(parts[2], 0);
    if (process.pid != 0)
      processes.push_back(std::move(process));
  }
  return processes;
}

inline void collect_gpus(RuntimeDeviceSnapshot &snapshot) {
  snapshot.gpu_query_attempted = true;
  int status = -1;
  const std::string output = popen_capture(
      "nvidia-smi --query-gpu=index,name,uuid,utilization.gpu,"
      "utilization.memory,memory.used,memory.total,temperature.gpu "
      "--format=csv,noheader,nounits 2>/dev/null",
      &status);
  std::stringstream ss(output);
  std::string line;
  while (std::getline(ss, line)) {
    const auto parts = split_csv_line(line);
    if (parts.size() < 8)
      continue;
    RuntimeGpuSnapshot gpu{};
    gpu.index = parts[0];
    gpu.name = parts[1];
    gpu.uuid = parts[2];
    gpu.utilization_gpu_pct = parse_int_or(parts[3], -1);
    gpu.utilization_mem_pct = parse_int_or(parts[4], -1);
    gpu.memory_used_mib = parse_u64_or(parts[5], 0);
    gpu.memory_total_mib = parse_u64_or(parts[6], 0);
    gpu.temperature_c = parse_int_or(parts[7], -1);
    snapshot.gpus.push_back(std::move(gpu));
  }
  if (snapshot.gpus.empty()) {
    snapshot.gpu_error =
        status == 0 ? "no GPUs reported" : "nvidia-smi unavailable";
    return;
  }
  snapshot.gpu_processes = collect_gpu_processes();
}

inline RuntimeDeviceSnapshot collect_runtime_device_snapshot() {
  RuntimeDeviceSnapshot snapshot{};
  snapshot.collected_at_ms = now_ms();

  char host[256]{};
  if (::gethostname(host, sizeof(host) - 1) == 0)
    snapshot.host_name = host;

  snapshot.cpu_count = std::thread::hardware_concurrency();
  read_load(snapshot);
  read_memory(snapshot);

  if (const auto cpu = read_cpu_sample())
    apply_cpu_sample(snapshot, *cpu);

  collect_gpus(snapshot);
  snapshot.ok = true;
  return snapshot;
}

inline std::size_t
runtime_device_row_count(const RuntimeDeviceSnapshot &device) {
  return device.gpus.size() + 1;
}

inline void clamp_runtime_device_selection(RuntimeState &runtime) {
  const std::size_t row_count = runtime_device_row_count(runtime.device);
  if (runtime.selected_device_row >= row_count)
    runtime.selected_device_row = row_count == 0 ? 0 : row_count - 1;
}

inline void push_runtime_device_history(RuntimeState &runtime,
                                        RuntimeDeviceSnapshot snapshot) {
  if (snapshot.collected_at_ms == 0)
    snapshot.collected_at_ms = now_ms();
  if (runtime.device.collected_at_ms != 0)
    compute_cpu_usage_from_previous(snapshot, runtime.device);
  runtime.device = std::move(snapshot);
  runtime.device_last_refresh_ms = runtime.device.collected_at_ms;
  runtime.device_history.push_back(runtime.device);
  constexpr std::size_t kMaxDeviceHistory = 96;
  if (runtime.device_history.size() > kMaxDeviceHistory) {
    runtime.device_history.erase(
        runtime.device_history.begin(),
        runtime.device_history.begin() +
            static_cast<std::ptrdiff_t>(runtime.device_history.size() -
                                        kMaxDeviceHistory));
  }
  clamp_runtime_device_selection(runtime);
}

inline void refresh_runtime_device_snapshot(RuntimeState &runtime) {
  push_runtime_device_history(runtime, collect_runtime_device_snapshot());
}

inline bool maybe_refresh_runtime_device_snapshot(CmdState &state,
                                                  std::uint64_t interval_ms) {
  if (state.screen != ScreenMode::Runtime)
    return false;
  const std::uint64_t now = now_ms();
  if (state.runtime.device_last_refresh_ms != 0 &&
      now - state.runtime.device_last_refresh_ms < interval_ms)
    return false;
  refresh_runtime_device_snapshot(state.runtime);
  return true;
}

inline RuntimePolicySummary
collect_runtime_policy(const std::filesystem::path &config_root) {
  RuntimePolicySummary summary{};
  summary.policy_path = config_root / "hero.runtime.dsl";
  const auto kv = read_assignment_file(summary.policy_path);
  summary.protocol_layer = assignment_value(kv, "protocol_layer");
  const auto config_path = config_root / ".config";
  summary.runtime_exec_path = assignment_value(
      kv, "runtime_exec_path",
      cuwacunu::hero::config_paths::default_runtime_exec_path(config_path)
          .string());
  summary.runtime_root = assignment_value(
      kv, "runtime_root",
      cuwacunu::hero::config_paths::default_runtime_root_path(config_path)
          .string());
  summary.default_dry_run = assignment_value(kv, "default_dry_run");
  summary.allow_execute = assignment_value(kv, "allow_execute");
  summary.allow_train_execute = assignment_value(kv, "allow_train_execute");
  std::error_code ec;
  summary.runtime_exec_exists =
      std::filesystem::exists(summary.runtime_exec_path, ec);
  summary.runtime_exec_executable =
      summary.runtime_exec_exists &&
      (::access(summary.runtime_exec_path.c_str(), X_OK) == 0);
  return summary;
}

inline RuntimeWaveSummary
collect_runtime_wave(const std::filesystem::path &config_root) {
  RuntimeWaveSummary summary{};
  summary.wave_path = config_root / "hero.runtime.wave.dsl";
  const auto kv = read_assignment_file(summary.wave_path);
  summary.wave_id = assignment_value(kv, "WAVE_ID");
  summary.target = assignment_value(kv, "TARGET");
  summary.mode = assignment_value(kv, "MODE");
  summary.source_range = assignment_value(kv, "SOURCE_RANGE");
  summary.source_order = assignment_value(kv, "SOURCE_ORDER");
  summary.anchor_index_begin = assignment_value(kv, "ANCHOR_INDEX_BEGIN");
  summary.anchor_index_end = assignment_value(kv, "ANCHOR_INDEX_END");
  return summary;
}

inline RuntimeJobSummary
read_runtime_job(const std::filesystem::path &state_path) {
  RuntimeJobSummary job{};
  job.job_dir = state_path.parent_path();
  std::error_code ec;
  job.last_write_time = std::filesystem::last_write_time(state_path, ec);

  const auto state = read_assignment_file(state_path);
  const auto manifest = read_assignment_file(job.job_dir / "job.manifest");
  auto pick = [&](const std::string &key) {
    std::string value = assignment_value(state, key);
    if (value.empty())
      value = assignment_value(manifest, key);
    return value;
  };
  job.job_id = pick("job_id");
  job.status = pick("status");
  job.job_kind = pick("job_kind");
  job.wave_id = pick("wave_id");
  job.wave_action = pick("wave_action");
  job.target_component_family_id = pick("target_component_family_id");
  job.accepted_anchor_count = pick("accepted_anchor_count");
  job.steps_completed = pick("steps_completed");
  job.last_loss = pick("last_loss");
  job.report_path = pick("report_path");
  job.checkpoint_path = pick("checkpoint_path");
  return job;
}

inline std::vector<RuntimeJobSummary>
scan_runtime_jobs(const std::filesystem::path &runtime_root,
                  std::size_t limit = 50) {
  std::vector<RuntimeJobSummary> jobs{};
  std::error_code ec;
  if (!std::filesystem::exists(runtime_root, ec))
    return jobs;
  const auto opts = std::filesystem::directory_options::skip_permission_denied;
  for (std::filesystem::recursive_directory_iterator it(runtime_root, opts, ec),
       end;
       it != end && !ec; it.increment(ec)) {
    if (!it->is_regular_file(ec))
      continue;
    if (it->path().filename() == "job.state")
      jobs.push_back(read_runtime_job(it->path()));
  }
  std::sort(jobs.begin(), jobs.end(),
            [](const RuntimeJobSummary &lhs, const RuntimeJobSummary &rhs) {
              return lhs.last_write_time > rhs.last_write_time;
            });
  if (jobs.size() > limit)
    jobs.resize(limit);
  return jobs;
}

inline ConfigState scan_config_files(const std::filesystem::path &config_root) {
  ConfigState state{};
  state.root = config_root;
  std::error_code ec;
  if (!std::filesystem::exists(config_root, ec))
    return state;
  const auto opts = std::filesystem::directory_options::skip_permission_denied;
  for (std::filesystem::recursive_directory_iterator it(config_root, opts, ec),
       end;
       it != end && !ec; it.increment(ec)) {
    if (ec)
      break;
    if (!it->is_regular_file(ec))
      continue;
    ConfigFileSummary file{};
    file.path = it->path();
    file.relative_path =
        std::filesystem::relative(it->path(), config_root, ec).string();
    file.size = it->file_size(ec);
    state.files.push_back(std::move(file));
  }
  std::sort(state.files.begin(), state.files.end(),
            [](const ConfigFileSummary &lhs, const ConfigFileSummary &rhs) {
              return lhs.relative_path < rhs.relative_path;
            });
  return state;
}

inline std::vector<std::string>
collect_lattice_target_ids(const std::filesystem::path &targets_path) {
  std::vector<std::string> targets{};
  std::ifstream in(targets_path);
  if (!in)
    return targets;
  std::string line;
  bool in_block_comment = false;
  bool in_target = false;
  while (std::getline(in, line)) {
    std::string cleaned;
    for (std::size_t i = 0; i < line.size();) {
      if (in_block_comment) {
        const std::size_t end = line.find("*/", i);
        if (end == std::string::npos)
          break;
        in_block_comment = false;
        i = end + 2;
        continue;
      }
      if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      cleaned.push_back(line[i++]);
    }
    if (cleaned.find("LATTICE_TARGET") != std::string::npos)
      in_target = true;
    if (in_target && cleaned.find("TARGET_ID") != std::string::npos) {
      const std::size_t eq = cleaned.find('=');
      if (eq != std::string::npos)
        targets.push_back(strip_trailing_semicolon(cleaned.substr(eq + 1)));
    }
    if (in_target && cleaned.find('}') != std::string::npos)
      in_target = false;
  }
  std::sort(targets.begin(), targets.end());
  targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
  return targets;
}

inline std::size_t count_named_files(const std::filesystem::path &root,
                                     const std::string &filename) {
  std::size_t count = 0;
  std::error_code ec;
  if (!std::filesystem::exists(root, ec))
    return 0;
  const auto opts = std::filesystem::directory_options::skip_permission_denied;
  for (std::filesystem::recursive_directory_iterator it(root, opts, ec), end;
       it != end && !ec; it.increment(ec)) {
    if (it->is_regular_file(ec) && it->path().filename() == filename)
      ++count;
  }
  return count;
}

inline LatticeState
scan_lattice_state(const std::filesystem::path &config_root,
                   const std::filesystem::path &runtime_root) {
  LatticeState state{};
  state.targets_path = config_root / "hero.lattice.targets.dsl";
  state.runtime_root = runtime_root;
  state.target_ids = collect_lattice_target_ids(state.targets_path);
  state.exposure_fact_count =
      count_named_files(runtime_root, "lattice.exposure.fact");
  state.checkpoint_fact_count =
      count_named_files(runtime_root, "lattice.checkpoint.fact");
  return state;
}

inline bool refresh_lattice_runtime_evidence_counts(CmdState &state) {
  std::filesystem::path runtime_root = state.lattice.runtime_root;
  if (runtime_root.empty())
    runtime_root = state.runtime.policy.runtime_root;
  const std::size_t exposure_count =
      count_named_files(runtime_root, "lattice.exposure.fact");
  const std::size_t checkpoint_count =
      count_named_files(runtime_root, "lattice.checkpoint.fact");
  const bool changed = state.lattice.runtime_root != runtime_root ||
                       state.lattice.exposure_fact_count != exposure_count ||
                       state.lattice.checkpoint_fact_count != checkpoint_count;
  state.lattice.runtime_root = runtime_root;
  state.lattice.exposure_fact_count = exposure_count;
  state.lattice.checkpoint_fact_count = checkpoint_count;
  return changed;
}

inline void append_log(CmdState &state, std::string source,
                       std::string message) {
  state.log.push_back(
      LogEntry{now_ms(), std::move(source), std::move(message)});
  const std::size_t capacity = std::max<std::size_t>(
      1u, state.logs.buffer_capacity == 0u ? 1u : state.logs.buffer_capacity);
  if (state.log.size() > capacity) {
    const auto over = static_cast<std::vector<LogEntry>::difference_type>(
        state.log.size() - capacity);
    state.log.erase(state.log.begin(), state.log.begin() + over);
  }
}

struct SnapshotRefreshStageDescriptor {
  std::string_view label{};
  std::string_view detail{};
};

inline const std::array<SnapshotRefreshStageDescriptor, 6> &
snapshot_refresh_stages() {
  static constexpr std::array<SnapshotRefreshStageDescriptor, 6> stages{{
      {"Config root", "resolve active global config and config tree"},
      {"Runtime policy",
       "read Runtime policy, active wave, and execution root"},
      {"Runtime inventory", "collect device telemetry, jobs, and artifacts"},
      {"Config catalog", "scan managed config files for the browser"},
      {"Lattice targets", "collect target ids and readiness fact counters"},
      {"Workbench", "prepare F2 Workbench, publish status, and enter TUI"},
  }};
  return stages;
}

struct SnapshotRefreshProgress {
  std::size_t completed_stages{0};
  std::size_t total_stages{0};
  std::string_view label{};
  std::string_view detail{};
  double ratio{0.0};
  bool complete{false};
};

using SnapshotRefreshProgressCallback =
    std::function<void(const SnapshotRefreshProgress &)>;

inline void
emit_snapshot_refresh_progress(const SnapshotRefreshProgressCallback &callback,
                               std::size_t completed_stages,
                               bool complete = false) {
  if (!callback)
    return;

  const auto &stages = snapshot_refresh_stages();
  const std::size_t total = stages.size();
  const std::size_t stage_index =
      std::min(completed_stages, total == 0u ? 0u : total - 1u);
  const double ratio = total == 0u
                           ? 1.0
                           : std::clamp(static_cast<double>(completed_stages) /
                                            static_cast<double>(total),
                                        0.0, 1.0);
  callback(SnapshotRefreshProgress{
      .completed_stages = completed_stages,
      .total_stages = total,
      .label = stages[stage_index].label,
      .detail = stages[stage_index].detail,
      .ratio = complete ? 1.0 : ratio,
      .complete = complete,
  });
}

inline void refresh_snapshots_with_progress(
    CmdState &state,
    const SnapshotRefreshProgressCallback &progress_callback = {}) {
  emit_snapshot_refresh_progress(progress_callback, 0u);

  const std::string selected_config =
      selected_config_file(state) == nullptr
          ? std::string{}
          : selected_config_file(state)->relative_path;
  const std::string selected_lattice_target =
      selected_lattice_target_id(state) == nullptr
          ? std::string{}
          : *selected_lattice_target_id(state);

  if (!state.global_config_path.empty())
    state.config_root = state.global_config_path.parent_path();
  if (state.config_root.empty())
    state.config_root =
        cuwacunu::hero::config_paths::default_global_config_path()
            .parent_path();
  emit_snapshot_refresh_progress(progress_callback, 1u);

  state.runtime.policy = collect_runtime_policy(state.config_root);
  state.runtime.wave = collect_runtime_wave(state.config_root);
  emit_snapshot_refresh_progress(progress_callback, 2u);

  refresh_runtime_device_snapshot(state.runtime);
  state.runtime.jobs = scan_runtime_jobs(state.runtime.policy.runtime_root);
  if (state.runtime.selected_job >= state.runtime.jobs.size())
    state.runtime.selected_job =
        state.runtime.jobs.empty() ? 0 : state.runtime.jobs.size() - 1;
  emit_snapshot_refresh_progress(progress_callback, 3u);

  state.config = scan_config_files(state.config_root);
  if (!selected_config.empty()) {
    for (std::size_t i = 0; i < state.config.files.size(); ++i) {
      if (state.config.files[i].relative_path == selected_config) {
        state.config.selected_file = i;
        break;
      }
    }
  }
  clamp_config_selection(state.config);
  emit_snapshot_refresh_progress(progress_callback, 4u);

  state.lattice =
      scan_lattice_state(state.config_root, state.runtime.policy.runtime_root);
  if (!selected_lattice_target.empty()) {
    for (std::size_t i = 0; i < state.lattice.target_ids.size(); ++i) {
      if (state.lattice.target_ids[i] == selected_lattice_target) {
        state.lattice.selected_target = i;
        break;
      }
    }
  }
  clamp_lattice_selection(state.lattice);
  emit_snapshot_refresh_progress(progress_callback, 5u);

  state.status = "refreshed";
  append_log(
      state, "refresh",
      "snapshots updated: jobs=" + std::to_string(state.runtime.jobs.size()) +
          " gpus=" + std::to_string(state.runtime.device.gpus.size()));
  emit_snapshot_refresh_progress(progress_callback, 6u, true);
}

inline void refresh_snapshots(CmdState &state) {
  refresh_snapshots_with_progress(state);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
