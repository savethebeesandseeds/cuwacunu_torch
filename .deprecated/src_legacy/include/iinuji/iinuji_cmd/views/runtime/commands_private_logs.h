inline bool runtime_read_structured_log_entries(
    const std::string &path, std::vector<RuntimeMarshalEventViewerEntry> *out,
    RuntimeLogViewerKind kind, std::string *error) {
  if (out == nullptr)
    return false;
  out->clear();
  std::string raw{};
  if (!read_text_file_safe(path, &raw, error))
    return false;
  const auto lines = runtime_split_jsonl_records(raw);
  out->reserve(lines.size());
  for (const auto &line : lines) {
    if (trim_copy(line).empty())
      continue;
    if (kind == RuntimeLogViewerKind::MarshalEvents) {
      out->push_back(runtime_build_marshaled_event_entry(line));
    } else {
      out->push_back(runtime_build_codex_stream_entry(
          line, runtime_codex_stream_fallback_label(kind)));
    }
  }
  return true;
}

inline std::size_t runtime_event_viewer_entry_count(const CmdState &st) {
  return st.runtime.marshal_event_viewer.entries.size();
}

inline bool runtime_event_viewer_has_entries(const CmdState &st) {
  return runtime_event_viewer_entry_count(st) != 0;
}

inline bool runtime_event_viewer_is_at_end(const CmdState &st) {
  return !runtime_event_viewer_has_entries(st) ||
         st.runtime.marshal_event_viewer.selected_entry + 1 >=
             runtime_event_viewer_entry_count(st);
}

inline void runtime_snap_event_viewer_to_end(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st)) {
    st.runtime.marshal_event_viewer.selected_entry = 0;
    return;
  }
  st.runtime.marshal_event_viewer.selected_entry =
      runtime_event_viewer_entry_count(st) - 1;
}

inline bool runtime_open_log_viewer(CmdState &st, const std::string &path,
                                    RuntimeLogViewerKind kind,
                                    std::string title,
                                    bool announce_status = true) {
  const bool supports_follow = runtime_log_viewer_kind_supports_follow(kind);
  if (runtime_log_viewer_kind_uses_structured_view(kind)) {
    const bool preserve_selection = runtime_event_viewer_is_open(st) &&
                                    st.runtime.log_viewer_path == path &&
                                    st.runtime.log_viewer_kind == kind;
    const bool previous_follow = st.runtime.log_viewer_live_follow;
    const std::size_t previous_selected =
        st.runtime.marshal_event_viewer.selected_entry;
    std::string error{};
    std::vector<RuntimeMarshalEventViewerEntry> entries{};
    if (!runtime_read_structured_log_entries(path, &entries, kind, &error)) {
      set_runtime_status(st,
                         error.empty()
                             ? "failed to open " +
                                   runtime_log_viewer_kind_label(kind) + " file"
                             : error,
                         true);
      return true;
    }

    st.runtime.log_viewer.reset();
    st.runtime.log_viewer_open = true;
    st.runtime.log_viewer_kind = kind;
    st.runtime.log_viewer_path = path;
    st.runtime.log_viewer_title = std::move(title);
    st.runtime.log_viewer_last_poll_ms = 0;
    st.runtime.marshal_event_viewer.entries = std::move(entries);
    st.runtime.marshal_event_viewer.selected_entry = 0;

    if (preserve_selection && !previous_follow) {
      st.runtime.log_viewer_live_follow = false;
      if (runtime_event_viewer_has_entries(st)) {
        st.runtime.marshal_event_viewer.selected_entry = std::min(
            previous_selected, runtime_event_viewer_entry_count(st) - 1);
      }
    } else {
      st.runtime.log_viewer_live_follow = supports_follow;
      if (supports_follow)
        runtime_snap_event_viewer_to_end(st);
    }

    runtime_record_log_viewer_stamp(st, runtime_probe_log_viewer_file(path));
    if (announce_status) {
      set_runtime_status(
          st,
          "Opened " +
              runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
              " in the right panel.",
          false);
    }
    return true;
  }

  const auto previous = st.runtime.log_viewer;
  const bool preserve_viewport = runtime_log_viewer_is_open(st) && previous &&
                                 st.runtime.log_viewer_path == path &&
                                 st.runtime.log_viewer_kind == kind;
  std::string error{};
  auto editor = runtime_build_log_viewer_editor(path, &error);
  if (!editor) {
    set_runtime_status(
        st, error.empty() ? "failed to open runtime log file" : error, true);
    return true;
  }
  if (preserve_viewport && !st.runtime.log_viewer_live_follow) {
    runtime_restore_log_viewer_viewport(*editor, *previous);
  } else if (supports_follow) {
    st.runtime.log_viewer_live_follow = true;
    if (previous)
      editor->last_body_h =
          std::max(editor->last_body_h, previous->last_body_h);
    runtime_snap_log_viewer_to_end(*editor);
  } else {
    st.runtime.log_viewer_live_follow = false;
    if (previous)
      editor->last_body_h =
          std::max(editor->last_body_h, previous->last_body_h);
  }
  st.runtime.marshal_event_viewer.entries.clear();
  st.runtime.marshal_event_viewer.selected_entry = 0;
  st.runtime.log_viewer = std::move(editor);
  st.runtime.log_viewer_open = true;
  st.runtime.log_viewer_kind = kind;
  st.runtime.log_viewer_path = path;
  st.runtime.log_viewer_title = std::move(title);
  st.runtime.log_viewer_last_poll_ms = 0;
  runtime_record_log_viewer_stamp(st, runtime_probe_log_viewer_file(path));
  if (announce_status) {
    set_runtime_status(
        st,
        "Opened " + runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
            " in the right panel.",
        false);
  }
  return true;
}

inline bool runtime_reload_log_viewer(CmdState &st,
                                      bool announce_status = true) {
  if (!runtime_log_viewer_is_open(st) || st.runtime.log_viewer_path.empty()) {
    return false;
  }
  const std::string title = st.runtime.log_viewer_title;
  const RuntimeLogViewerKind kind = st.runtime.log_viewer_kind;
  const bool ok = runtime_open_log_viewer(st, st.runtime.log_viewer_path, kind,
                                          title, false);
  if (ok && announce_status) {
    set_runtime_status(
        st,
        "Reloaded " +
            runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) + ".",
        false);
  }
  return ok;
}

inline bool runtime_toggle_log_viewer_follow(CmdState &st) {
  if (!runtime_log_viewer_is_open(st))
    return false;
  if (!runtime_log_viewer_supports_follow(st)) {
    set_runtime_status(st, "This viewer does not support live-follow.", false);
    return true;
  }
  st.runtime.log_viewer_live_follow = !st.runtime.log_viewer_live_follow;
  if (st.runtime.log_viewer_live_follow) {
    if (runtime_event_viewer_is_open(st)) {
      runtime_snap_event_viewer_to_end(st);
    } else if (st.runtime.log_viewer) {
      runtime_snap_log_viewer_to_end(*st.runtime.log_viewer);
    }
  }
  set_runtime_status(st,
                     st.runtime.log_viewer_live_follow
                         ? "Runtime log live-follow enabled."
                         : "Runtime log live-follow paused.",
                     false);
  return true;
}

inline bool runtime_poll_live_log_viewer(CmdState &st, std::uint64_t now_ms) {
  if (!runtime_log_viewer_is_open(st) || st.runtime.log_viewer_path.empty()) {
    return false;
  }
  if (!runtime_log_viewer_supports_follow(st))
    return false;
  constexpr std::uint64_t kRuntimeLogViewerPollPeriodMs = 700;
  if (st.runtime.log_viewer_last_poll_ms != 0 &&
      now_ms <
          st.runtime.log_viewer_last_poll_ms + kRuntimeLogViewerPollPeriodMs) {
    return false;
  }
  st.runtime.log_viewer_last_poll_ms = now_ms;
  const auto stamp = runtime_probe_log_viewer_file(st.runtime.log_viewer_path);
  if (!stamp.ok || runtime_log_viewer_stamp_matches(st, stamp))
    return false;
  return runtime_reload_log_viewer(st, false);
}

inline bool runtime_select_prev_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st) ||
      st.runtime.marshal_event_viewer.selected_entry == 0) {
    return false;
  }
  --st.runtime.marshal_event_viewer.selected_entry;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_next_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st) ||
      st.runtime.marshal_event_viewer.selected_entry + 1 >=
          runtime_event_viewer_entry_count(st)) {
    return false;
  }
  ++st.runtime.marshal_event_viewer.selected_entry;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_first_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st))
    return false;
  st.runtime.marshal_event_viewer.selected_entry = 0;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_last_event_viewer_entry(CmdState &st,
                                                   bool enable_follow) {
  if (!runtime_event_viewer_has_entries(st))
    return false;
  runtime_snap_event_viewer_to_end(st);
  st.runtime.log_viewer_live_follow = enable_follow;
  return true;
}

inline bool runtime_page_event_viewer_entries(CmdState &st, int delta) {
  if (!runtime_event_viewer_has_entries(st) || delta == 0)
    return false;
  const auto max_index =
      static_cast<std::ptrdiff_t>(runtime_event_viewer_entry_count(st) - 1);
  const auto current = static_cast<std::ptrdiff_t>(
      st.runtime.marshal_event_viewer.selected_entry);
  const auto next = std::clamp(current + static_cast<std::ptrdiff_t>(delta),
                               static_cast<std::ptrdiff_t>(0), max_index);
  if (next == current)
    return false;
  st.runtime.marshal_event_viewer.selected_entry =
      static_cast<std::size_t>(next);
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline std::uint64_t runtime_monotonic_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline std::uint64_t runtime_wall_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

inline std::vector<std::string>
runtime_split_csv_fields(const std::string &line) {
  std::vector<std::string> fields{};
  std::string current{};
  bool in_quotes = false;
  for (char c : line) {
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (c == ',' && !in_quotes) {
      fields.push_back(trim_copy(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  fields.push_back(trim_copy(current));
  return fields;
}

inline bool runtime_parse_u64_token(std::string token, std::uint64_t *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  std::uint64_t value = 0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_parse_int_token(std::string token, int *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  int value = 0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_parse_double_token(std::string token, double *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  double value = 0.0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_capture_command_output(const std::string &command,
                                           std::string *out, int *status_code) {
  if (out)
    out->clear();
  if (status_code)
    *status_code = -1;

  FILE *pipe = ::popen(command.c_str(), "r");
  if (pipe == nullptr)
    return false;

  char buffer[4096];
  std::ostringstream oss{};
  while (::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    oss << buffer;
  }
  const int status = ::pclose(pipe);
  if (out)
    *out = oss.str();
  if (status_code)
    *status_code = status;
  return status == 0;
}

inline void runtime_parse_meminfo_text(const std::string &text,
                                       RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  for (const auto &line : split_lines_keep_empty(text)) {
    std::istringstream iss(line);
    std::string key{};
    std::uint64_t value = 0;
    iss >> key >> value;
    if (!iss)
      continue;
    if (key == "MemTotal:") {
      out->mem_total_kib = value;
    } else if (key == "MemAvailable:") {
      out->mem_available_kib = value;
    }
  }
}

inline void runtime_parse_proc_stat_text(const std::string &text,
                                         RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  for (const auto &line : split_lines_keep_empty(text)) {
    if (line.rfind("cpu ", 0) != 0)
      continue;
    std::istringstream iss(line.substr(4));
    (void)(iss >> out->cpu_user_ticks >> out->cpu_nice_ticks >>
           out->cpu_system_ticks >> out->cpu_idle_ticks >>
           out->cpu_iowait_ticks >> out->cpu_irq_ticks >>
           out->cpu_softirq_ticks >> out->cpu_steal_ticks);
    break;
  }
}

inline void runtime_compute_cpu_usage(RuntimeDeviceSnapshot *current,
                                      const RuntimeDeviceSnapshot *previous) {
  if (current == nullptr || previous == nullptr)
    return;
  const std::uint64_t prev_total =
      previous->cpu_user_ticks + previous->cpu_nice_ticks +
      previous->cpu_system_ticks + previous->cpu_idle_ticks +
      previous->cpu_iowait_ticks + previous->cpu_irq_ticks +
      previous->cpu_softirq_ticks + previous->cpu_steal_ticks;
  const std::uint64_t curr_total =
      current->cpu_user_ticks + current->cpu_nice_ticks +
      current->cpu_system_ticks + current->cpu_idle_ticks +
      current->cpu_iowait_ticks + current->cpu_irq_ticks +
      current->cpu_softirq_ticks + current->cpu_steal_ticks;
  if (curr_total <= prev_total)
    return;

  const std::uint64_t total_delta = curr_total - prev_total;
  const std::uint64_t idle_prev =
      previous->cpu_idle_ticks + previous->cpu_iowait_ticks;
  const std::uint64_t idle_curr =
      current->cpu_idle_ticks + current->cpu_iowait_ticks;
  const std::uint64_t idle_delta =
      idle_curr >= idle_prev ? (idle_curr - idle_prev) : 0;
  const std::uint64_t busy_delta =
      total_delta >= idle_delta ? (total_delta - idle_delta) : 0;
  current->cpu_usage_pct =
      std::clamp(static_cast<int>((100.0 * static_cast<double>(busy_delta)) /
                                      static_cast<double>(total_delta) +
                                  0.5),
                 0, 100);

  const std::uint64_t iowait_delta =
      current->cpu_iowait_ticks >= previous->cpu_iowait_ticks
          ? (current->cpu_iowait_ticks - previous->cpu_iowait_ticks)
          : 0;
  current->cpu_iowait_pct =
      std::clamp(static_cast<int>((100.0 * static_cast<double>(iowait_delta)) /
                                      static_cast<double>(total_delta) +
                                  0.5),
                 0, 100);
}

inline void runtime_collect_host_device_snapshot(RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  out->host_name.clear();
  out->cpu_count = std::max(1u, std::thread::hardware_concurrency());

  std::string hostname{};
  std::string ignored_error{};
  if (read_text_file_safe("/proc/sys/kernel/hostname", &hostname,
                          &ignored_error)) {
    out->host_name = trim_copy(hostname);
  }

  std::string loadavg{};
  if (read_text_file_safe("/proc/loadavg", &loadavg, &ignored_error)) {
    std::istringstream iss(loadavg);
    (void)(iss >> out->load1 >> out->load5 >> out->load15);
  }

  std::string proc_stat{};
  if (read_text_file_safe("/proc/stat", &proc_stat, &ignored_error)) {
    runtime_parse_proc_stat_text(proc_stat, out);
  }

  std::string meminfo{};
  if (read_text_file_safe("/proc/meminfo", &meminfo, &ignored_error)) {
    runtime_parse_meminfo_text(meminfo, out);
  }
}

inline void runtime_collect_gpu_device_snapshot(RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;

  out->gpu_query_attempted = true;
  out->gpus.clear();
  out->gpu_processes.clear();
  out->gpu_error.clear();

  std::string gpu_output{};
  int gpu_status = -1;
  const bool gpu_ok = runtime_capture_command_output(
      "LC_ALL=C nvidia-smi "
      "--query-gpu=index,name,uuid,utilization.gpu,utilization.memory,"
      "memory.used,memory.total,temperature.gpu "
      "--format=csv,noheader,nounits 2>&1",
      &gpu_output, &gpu_status);
  if (!gpu_ok) {
    const std::string error_text = trim_copy(gpu_output);
    if (to_lower_copy(error_text).find("not found") != std::string::npos) {
      out->gpu_query_attempted = false;
      out->gpu_error = "nvidia-smi unavailable";
      return;
    }
    out->gpu_error =
        error_text.empty() ? "nvidia-smi query failed" : error_text;
    return;
  }

  for (const auto &raw_line : split_lines_keep_empty(gpu_output)) {
    const std::string line = trim_copy(raw_line);
    if (line.empty())
      continue;
    const auto fields = runtime_split_csv_fields(line);
    if (fields.size() < 8)
      continue;

    RuntimeDeviceGpuSnapshot gpu{};
    gpu.index = fields[0];
    gpu.name = fields[1];
    gpu.uuid = fields[2];
    (void)runtime_parse_int_token(fields[3], &gpu.util_gpu_pct);
    (void)runtime_parse_int_token(fields[4], &gpu.util_mem_pct);
    (void)runtime_parse_u64_token(fields[5], &gpu.memory_used_mib);
    (void)runtime_parse_u64_token(fields[6], &gpu.memory_total_mib);
    (void)runtime_parse_int_token(fields[7], &gpu.temperature_c);
    out->gpus.push_back(std::move(gpu));
  }

  if (out->gpus.empty()) {
    out->gpu_error = "no NVIDIA GPU detected";
    return;
  }

  std::string proc_output{};
  int proc_status = -1;
  const bool proc_ok = runtime_capture_command_output(
      "LC_ALL=C nvidia-smi "
      "--query-compute-apps=pid,gpu_uuid,used_gpu_memory "
      "--format=csv,noheader,nounits 2>/dev/null",
      &proc_output, &proc_status);
  (void)proc_status;
  if (!proc_ok)
    return;

  for (const auto &raw_line : split_lines_keep_empty(proc_output)) {
    const std::string line = trim_copy(raw_line);
    if (line.empty())
      continue;
    if (to_lower_copy(line).find("no running processes") != std::string::npos) {
      continue;
    }
    const auto fields = runtime_split_csv_fields(line);
    if (fields.size() < 3)
      continue;
    RuntimeDeviceGpuProcessSnapshot proc{};
    if (!runtime_parse_u64_token(fields[0], &proc.pid))
      continue;
    proc.gpu_uuid = fields[1];
    (void)runtime_parse_u64_token(fields[2], &proc.used_memory_mib);
    out->gpu_processes.push_back(std::move(proc));
  }
}

inline RuntimeDeviceSnapshot runtime_collect_device_snapshot() {
  RuntimeDeviceSnapshot snapshot{};
  snapshot.collected_at_ms = runtime_wall_now_ms();
  runtime_collect_host_device_snapshot(&snapshot);
  runtime_collect_gpu_device_snapshot(&snapshot);
  snapshot.ok = true;

  if (snapshot.host_name.empty() && snapshot.mem_total_kib == 0 &&
      snapshot.gpus.empty() && !snapshot.gpu_query_attempted) {
    snapshot.error = "device snapshot unavailable";
  }
  return snapshot;
}

struct RuntimePidResourceSnapshot {
  bool alive{false};
  std::string name{};
  std::string state{};
  std::uint64_t rss_kib{0};
};

inline RuntimePidResourceSnapshot
runtime_probe_pid_resources(std::uint64_t pid) {
  RuntimePidResourceSnapshot snapshot{};
  if (pid == 0)
    return snapshot;

  snapshot.alive = cuwacunu::hero::runtime::pid_alive(pid);
  std::string status_text{};
  std::string ignored_error{};
  if (!read_text_file_safe("/proc/" + std::to_string(pid) + "/status",
                           &status_text, &ignored_error)) {
    return snapshot;
  }

  snapshot.alive = true;
  for (const auto &line : split_lines_keep_empty(status_text)) {
    if (line.rfind("Name:", 0) == 0) {
      snapshot.name = trim_copy(line.substr(5));
      continue;
    }
    if (line.rfind("State:", 0) == 0) {
      snapshot.state = trim_copy(line.substr(6));
      continue;
    }
    if (line.rfind("VmRSS:", 0) == 0) {
      std::istringstream iss(line.substr(6));
      std::uint64_t rss_kib = 0;
      if (iss >> rss_kib)
        snapshot.rss_kib = rss_kib;
    }
  }
  return snapshot;
}

inline std::size_t runtime_device_item_count(const CmdState &st) {
  return std::size_t{1} + st.runtime.device.gpus.size();
}

inline std::optional<std::size_t>
runtime_selected_device_index(const CmdState &st) {
  const std::size_t total = runtime_device_item_count(st);
  if (total == 0)
    return std::nullopt;
  return std::min(st.runtime.selected_device_index, total - 1);
}

inline bool runtime_apply_device_snapshot(CmdState &st,
                                          RuntimeDeviceSnapshot snapshot) {
  if (st.runtime.device.collected_at_ms != 0) {
    runtime_compute_cpu_usage(&snapshot, &st.runtime.device);
  }
  st.runtime.device = std::move(snapshot);
  st.runtime.device_history.push_back(st.runtime.device);
  constexpr std::size_t kRuntimeDeviceHistoryLimit = 96;
  if (st.runtime.device_history.size() > kRuntimeDeviceHistoryLimit) {
    st.runtime.device_history.erase(
        st.runtime.device_history.begin(),
        st.runtime.device_history.begin() +
            static_cast<std::ptrdiff_t>(st.runtime.device_history.size() -
                                        kRuntimeDeviceHistoryLimit));
  }
  const std::size_t total = runtime_device_item_count(st);
  if (total == 0) {
    st.runtime.selected_device_index = 0;
  } else {
    st.runtime.selected_device_index =
        std::min(st.runtime.selected_device_index, total - 1);
  }
  return true;
}

inline bool queue_runtime_device_refresh(CmdState &st) {
  if (st.runtime.device_pending) {
    st.runtime.device_requeue = true;
    return false;
  }
  st.runtime.device_pending = true;
  st.runtime.device_requeue = false;
  st.runtime.device_last_refresh_ms = runtime_monotonic_now_ms();
  st.runtime.device_future =
      std::async(std::launch::async, []() -> RuntimeDeviceSnapshot {
        return runtime_collect_device_snapshot();
      });
  return true;
}

inline bool runtime_poll_device_refresh(CmdState &st, std::uint64_t now_ms) {
  constexpr std::uint64_t kRuntimeDeviceRefreshPeriodMs = 700;
  if (st.screen != ScreenMode::Runtime)
    return false;
  if (st.runtime.device_pending)
    return false;
  if (st.runtime.device_last_refresh_ms != 0 &&
      now_ms <
          st.runtime.device_last_refresh_ms + kRuntimeDeviceRefreshPeriodMs) {
    return false;
  }
  return queue_runtime_device_refresh(st);
}

inline bool runtime_open_job_log_viewer(CmdState &st,
                                        RuntimeLogViewerKind kind) {
  if (!st.runtime.job.has_value())
    return false;
  const auto &job = *st.runtime.job;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::JobStdout:
    path = job.stdout_path;
    break;
  case RuntimeLogViewerKind::JobStderr:
    path = job.stderr_path;
    break;
  case RuntimeLogViewerKind::JobTrace:
    path = job.trace_path;
    break;
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected job does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(st, path, kind,
                                 runtime_log_viewer_kind_label(kind) + " | " +
                                     trim_to_width(job.job_cursor, 28));
}

inline bool runtime_open_campaign_log_viewer(CmdState &st,
                                             RuntimeLogViewerKind kind) {
  if (!st.runtime.campaign.has_value())
    return false;
  const auto &campaign = *st.runtime.campaign;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::CampaignStdout:
    path = campaign.stdout_path;
    break;
  case RuntimeLogViewerKind::CampaignStderr:
    path = campaign.stderr_path;
    break;
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected campaign does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(
      st, path, kind,
      runtime_log_viewer_kind_label(kind) + " | " +
      trim_to_width(campaign.campaign_cursor, 28));
}

inline std::string runtime_session_summary_path(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.marshal_session_id.empty())
    return {};
  if (!st.runtime.marshal_app.defaults.marshal_root.empty()) {
    return cuwacunu::hero::marshal::marshal_session_human_summary_path(
               st.runtime.marshal_app.defaults.marshal_root,
               session.marshal_session_id)
        .string();
  }
  if (!session.session_root.empty()) {
    return (std::filesystem::path(session.session_root) /
            std::string(cuwacunu::hero::marshal::kMarshalSessionHumanDirname) /
            std::string(
                cuwacunu::hero::marshal::kMarshalSessionHumanSummaryFilename))
        .lexically_normal()
        .string();
  }
  return {};
}

inline bool runtime_open_session_log_viewer(CmdState &st,
                                            RuntimeLogViewerKind kind) {
  if (!st.runtime.session.has_value())
    return false;
  const auto &session = *st.runtime.session;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::MarshalEvents:
    path = session.events_path;
    break;
  case RuntimeLogViewerKind::MarshalCodexStdout:
    path = session.codex_stdout_path;
    break;
  case RuntimeLogViewerKind::MarshalCodexStderr:
    path = session.codex_stderr_path;
    break;
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected marshal does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(
      st, path, kind,
      runtime_log_viewer_kind_label(kind) + " | " +
          trim_to_width(session.marshal_session_id, 28));
}

inline bool runtime_open_artifact_viewer(CmdState &st, const std::string &path,
                                         std::string title,
                                         std::string missing_status) {
  if (path.empty()) {
    set_runtime_status(st,
                       missing_status.empty() ? "artifact path is not recorded"
                                              : std::move(missing_status),
                       true);
    return true;
  }
  return runtime_open_log_viewer(st, path, RuntimeLogViewerKind::ArtifactFile,
                                 std::move(title));
}

inline bool runtime_parse_marshal_session_json(
    const cmd_json_value_t *value,
    cuwacunu::hero::marshal::marshal_session_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid marshal session payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing marshal session output";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  session.schema =
      cmd_json_string(cmd_json_field(value, "schema"), session.schema);
  session.marshal_session_id =
      cmd_json_string(cmd_json_field(value, "marshal_session_id"));
  session.lifecycle =
      cmd_json_string(cmd_json_field(value, "lifecycle"), session.lifecycle);
  session.status_detail =
      cmd_json_string(cmd_json_field(value, "status_detail"));
  session.work_gate =
      cmd_json_string(cmd_json_field(value, "work_gate"), session.work_gate);
  session.finish_reason = cmd_json_string(
      cmd_json_field(value, "finish_reason"), session.finish_reason);
  session.activity =
      cmd_json_string(cmd_json_field(value, "activity"), session.activity);
  session.objective_name =
      cmd_json_string(cmd_json_field(value, "objective_name"));
  session.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  session.source_marshal_objective_dsl_path = cmd_json_string(
      cmd_json_field(value, "source_marshal_objective_dsl_path"));
  session.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  session.source_marshal_objective_md_path = cmd_json_string(
      cmd_json_field(value, "source_marshal_objective_md_path"));
  session.source_marshal_guidance_md_path =
      cmd_json_string(cmd_json_field(value, "source_marshal_guidance_md_path"));
  session.session_root = cmd_json_string(cmd_json_field(value, "session_root"));
  session.objective_root =
      cmd_json_string(cmd_json_field(value, "objective_root"));
  session.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  session.marshal_objective_dsl_path =
      cmd_json_string(cmd_json_field(value, "marshal_objective_dsl_path"));
  session.marshal_objective_md_path =
      cmd_json_string(cmd_json_field(value, "marshal_objective_md_path"));
  session.marshal_guidance_md_path =
      cmd_json_string(cmd_json_field(value, "marshal_guidance_md_path"));
  session.hero_marshal_dsl_path =
      cmd_json_string(cmd_json_field(value, "hero_marshal_dsl_path"));
  session.config_policy_path =
      cmd_json_string(cmd_json_field(value, "config_policy_path"));
  session.briefing_path =
      cmd_json_string(cmd_json_field(value, "briefing_path"));
  session.memory_path = cmd_json_string(cmd_json_field(value, "memory_path"));
  session.human_request_path =
      cmd_json_string(cmd_json_field(value, "human_request_path"));
  session.events_path = cmd_json_string(cmd_json_field(value, "events_path"));
  session.codex_stdout_path =
      cmd_json_string(cmd_json_field(value, "codex_stdout_path"));
  session.codex_stderr_path =
      cmd_json_string(cmd_json_field(value, "codex_stderr_path"));
  session.current_thread_id =
      cmd_json_string(cmd_json_field(value, "current_thread_id"));
  session.resolved_marshal_codex_binary =
      cmd_json_string(cmd_json_field(value, "resolved_marshal_codex_binary"));
  session.resolved_marshal_codex_model =
      cmd_json_string(cmd_json_field(value, "resolved_marshal_codex_model"));
  session.resolved_marshal_codex_reasoning_effort = cmd_json_string(
      cmd_json_field(value, "resolved_marshal_codex_reasoning_effort"));
  session.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  session.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  session.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  session.checkpoint_count =
      cmd_json_u64(cmd_json_field(value, "checkpoint_count"));
  session.launch_count = cmd_json_u64(cmd_json_field(value, "launch_count"));
  session.max_campaign_launches =
      cmd_json_u64(cmd_json_field(value, "max_campaign_launches"));
  session.remaining_campaign_launches =
      cmd_json_u64(cmd_json_field(value, "remaining_campaign_launches"));
  session.authority_scope = cmd_json_string(
      cmd_json_field(value, "authority_scope"), session.authority_scope);
  session.campaign_status = cmd_json_string(
      cmd_json_field(value, "campaign_status"), session.campaign_status);
  session.campaign_cursor =
      cmd_json_string(cmd_json_field(value, "campaign_cursor"));
  session.last_codex_action =
      cmd_json_string(cmd_json_field(value, "last_codex_action"));
  session.last_warning = cmd_json_string(cmd_json_field(value, "last_warning"));
  session.last_input_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_input_checkpoint_path"));
  session.last_intent_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_intent_checkpoint_path"));
  session.last_mutation_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_mutation_checkpoint_path"));
  session.campaign_cursors =
      cmd_json_string_array(cmd_json_field(value, "campaign_cursors"));
  if (session.marshal_session_id.empty()) {
    if (error)
      *error = "marshal session payload missing marshal_session_id";
    return false;
  }
  *out = std::move(session);
  return true;
}

inline bool runtime_parse_campaign_json(
    const cmd_json_value_t *value,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid runtime campaign payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing runtime campaign output";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.schema =
      cmd_json_string(cmd_json_field(value, "schema"), campaign.schema);
  campaign.campaign_cursor =
      cmd_json_string(cmd_json_field(value, "campaign_cursor"));
  campaign.boot_id = cmd_json_string(cmd_json_field(value, "boot_id"));
  campaign.state =
      cmd_json_string(cmd_json_field(value, "state"), campaign.state);
  campaign.state_detail =
      cmd_json_string(cmd_json_field(value, "state_detail"));
  campaign.marshal_session_id =
      cmd_json_string(cmd_json_field(value, "marshal_session_id"));
  campaign.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  campaign.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  campaign.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  campaign.reset_runtime_state =
      cmd_json_bool(cmd_json_field(value, "reset_runtime_state"), false);
  campaign.stdout_path = cmd_json_string(cmd_json_field(value, "stdout_path"));
  campaign.stderr_path = cmd_json_string(cmd_json_field(value, "stderr_path"));
  campaign.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  campaign.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  campaign.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  campaign.runner_pid =
      cmd_json_optional_u64(cmd_json_field(value, "runner_pid"));
  campaign.runner_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "runner_start_ticks"));
  campaign.current_run_index =
      cmd_json_optional_u64(cmd_json_field(value, "current_run_index"));
  campaign.run_bind_ids =
      cmd_json_string_array(cmd_json_field(value, "run_bind_ids"));
  campaign.job_cursors =
      cmd_json_string_array(cmd_json_field(value, "job_cursors"));
  if (campaign.campaign_cursor.empty()) {
    if (error)
      *error = "runtime campaign payload missing campaign_cursor";
    return false;
  }
  *out = std::move(campaign);
  return true;
}

inline bool
runtime_parse_job_json(const cmd_json_value_t *value,
                       cuwacunu::hero::runtime::runtime_job_record_t *out,
                       std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid runtime job payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing runtime job output";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t job{};
  job.schema = cmd_json_string(cmd_json_field(value, "schema"), job.schema);
  job.job_cursor = cmd_json_string(cmd_json_field(value, "job_cursor"));
  job.job_kind =
      cmd_json_string(cmd_json_field(value, "job_kind"), job.job_kind);
  job.boot_id = cmd_json_string(cmd_json_field(value, "boot_id"));
  job.state = cmd_json_string(cmd_json_field(value, "state"), job.state);
  job.state_detail = cmd_json_string(cmd_json_field(value, "state_detail"));
  job.worker_binary = cmd_json_string(cmd_json_field(value, "worker_binary"));
  job.worker_command = cmd_json_string(cmd_json_field(value, "worker_command"));
  job.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  job.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  job.source_contract_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_contract_dsl_path"));
  job.source_wave_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_wave_dsl_path"));
  job.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  job.contract_dsl_path =
      cmd_json_string(cmd_json_field(value, "contract_dsl_path"));
  job.wave_dsl_path = cmd_json_string(cmd_json_field(value, "wave_dsl_path"));
  job.binding_id = cmd_json_string(cmd_json_field(value, "binding_id"));
  job.requested_device =
      cmd_json_string(cmd_json_field(value, "requested_device"));
  job.resolved_device =
      cmd_json_string(cmd_json_field(value, "resolved_device"));
  job.device_source_section =
      cmd_json_string(cmd_json_field(value, "device_source_section"));
  job.device_contract_hash =
      cmd_json_string(cmd_json_field(value, "device_contract_hash"));
  job.device_error = cmd_json_string(cmd_json_field(value, "device_error"));
  job.cuda_required =
      cmd_json_bool(cmd_json_field(value, "cuda_required"), false);
  job.reset_runtime_state =
      cmd_json_bool(cmd_json_field(value, "reset_runtime_state"), false);
  job.stdout_path = cmd_json_string(cmd_json_field(value, "stdout_path"));
  job.stderr_path = cmd_json_string(cmd_json_field(value, "stderr_path"));
  job.trace_path = cmd_json_string(cmd_json_field(value, "trace_path"));
  job.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  job.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  job.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  job.runner_pid = cmd_json_optional_u64(cmd_json_field(value, "runner_pid"));
  job.runner_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "runner_start_ticks"));
  job.target_pid = cmd_json_optional_u64(cmd_json_field(value, "target_pid"));
  job.target_pgid = cmd_json_optional_u64(cmd_json_field(value, "target_pgid"));
  job.target_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "target_start_ticks"));
  job.exit_code = cmd_json_optional_i64(cmd_json_field(value, "exit_code"));
  job.term_signal = cmd_json_optional_i64(cmd_json_field(value, "term_signal"));
  if (job.job_cursor.empty()) {
    if (error)
      *error = "runtime job payload missing job_cursor";
    return false;
  }
  *out = std::move(job);
  return true;
}

inline bool runtime_invoke_runtime_tool(
    cuwacunu::hero::runtime_mcp::app_context_t *app,
    const std::string &tool_name, const std::string &arguments_json,
    cmd_json_value_t *out_structured, std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing Runtime Hero app context";
    return false;
  }
  if (!cuwacunu::hero::runtime_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return cmd_parse_tool_structured_content(result_json, out_structured, error);
}

inline bool runtime_invoke_runtime_tool(CmdState &st,
                                        const std::string &tool_name,
                                        const std::string &arguments_json,
                                        cmd_json_value_t *out_structured,
                                        std::string *error) {
  return runtime_invoke_runtime_tool(&st.runtime.app, tool_name, arguments_json,
                                     out_structured, error);
}

inline bool runtime_invoke_marshal_tool(
    cuwacunu::hero::marshal_mcp::app_context_t *app,
    const std::string &tool_name, const std::string &arguments_json,
    cmd_json_value_t *out_structured, std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing Marshal Hero app context";
    return false;
  }
  if (!cuwacunu::hero::marshal_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return cmd_parse_tool_structured_content(result_json, out_structured, error);
}

inline bool runtime_invoke_marshal_tool(CmdState &st,
                                        const std::string &tool_name,
                                        const std::string &arguments_json,
                                        cmd_json_value_t *out_structured,
                                        std::string *error) {
  return runtime_invoke_marshal_tool(&st.runtime.marshal_app, tool_name,
                                     arguments_json, out_structured, error);
}

inline bool runtime_load_sessions_via_hero(
    bool marshal_ok, const std::string &marshal_error,
    cuwacunu::hero::marshal_mcp::app_context_t *marshal_app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime sessions output";
    return false;
  }
  out->clear();
  if (!marshal_ok) {
    if (error) {
      *error = marshal_error.empty() ? "Marshal Hero defaults unavailable"
                                     : marshal_error;
    }
    return false;
  }

  cmd_json_value_t structured{};
  if (!runtime_invoke_marshal_tool(marshal_app, "hero.marshal.list_sessions",
                                   "{}", &structured, error)) {
    return false;
  }
  const auto *sessions = cmd_json_field(&structured, "sessions");
  if (sessions == nullptr || sessions->type != cmd_json_type_t::ARRAY ||
      !sessions->arrayValue) {
    if (error)
      *error = "hero.marshal.list_sessions missing sessions array";
    return false;
  }
  out->reserve(sessions->arrayValue->size());
  for (const auto &entry : *sessions->arrayValue) {
    cuwacunu::hero::marshal::marshal_session_record_t session{};
    if (!runtime_parse_marshal_session_json(&entry, &session, error))
      return false;
    out->push_back(std::move(session));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.marshal_session_id > b.marshal_session_id;
  });
  return true;
}

inline bool runtime_load_campaigns_via_hero(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime campaigns output";
    return false;
  }
  out->clear();

  cmd_json_value_t structured{};
  if (!runtime_invoke_runtime_tool(runtime_app, "hero.runtime.list_campaigns",
                                   "{}", &structured, error)) {
    return false;
  }
  const auto *campaigns = cmd_json_field(&structured, "campaigns");
  if (campaigns == nullptr || campaigns->type != cmd_json_type_t::ARRAY ||
      !campaigns->arrayValue) {
    if (error)
      *error = "hero.runtime.list_campaigns missing campaigns array";
    return false;
  }
  out->reserve(campaigns->arrayValue->size());
  for (const auto &entry : *campaigns->arrayValue) {
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (!runtime_parse_campaign_json(&entry, &campaign, error))
      return false;
    out->push_back(std::move(campaign));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.campaign_cursor > b.campaign_cursor;
  });
  return true;
}

inline bool runtime_load_jobs_via_hero(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime jobs output";
    return false;
  }
  out->clear();

  cmd_json_value_t structured{};
  if (!runtime_invoke_runtime_tool(runtime_app, "hero.runtime.list_jobs",
                                   "{\"include_full\":true}", &structured,
                                   error)) {
    return false;
  }
  const auto *jobs = cmd_json_field(&structured, "jobs");
  if (jobs == nullptr || jobs->type != cmd_json_type_t::ARRAY ||
      !jobs->arrayValue) {
    if (error)
      *error = "hero.runtime.list_jobs missing jobs array";
    return false;
  }
  out->reserve(jobs->arrayValue->size());
  for (const auto &entry : *jobs->arrayValue) {
    cuwacunu::hero::runtime::runtime_job_record_t job{};
    if (!runtime_parse_job_json(&entry, &job, error))
      return false;
    out->push_back(std::move(job));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.job_cursor > b.job_cursor;
  });
  return true;
}

inline bool runtime_load_sessions_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  return runtime_load_sessions_via_hero(st.runtime.marshal_ok,
                                        st.runtime.marshal_error,
                                        &st.runtime.marshal_app, out, error);
}

inline bool runtime_load_campaigns_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> *out,
    std::string *error) {
  return runtime_load_campaigns_via_hero(&st.runtime.app, out, error);
}

inline bool runtime_load_jobs_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t> *out,
    std::string *error) {
  return runtime_load_jobs_via_hero(&st.runtime.app, out, error);
}

inline void runtime_assign_excerpt_from_tool(CmdState &st,
                                             const std::string &tool_name,
                                             const std::string &arguments_json,
                                             std::string *out_excerpt) {
  if (out_excerpt == nullptr)
    return;
  out_excerpt->clear();

  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_runtime_tool(st, tool_name, arguments_json, &structured,
                                   &error)) {
    *out_excerpt = "<" + tool_name + " unavailable: " + error + ">";
    return;
  }
  *out_excerpt = cmd_json_string(cmd_json_field(&structured, "text"));
  if (out_excerpt->empty())
    *out_excerpt = "<empty>";
}

inline void runtime_assign_excerpt_from_tool(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    const std::string &tool_name, const std::string &arguments_json,
    std::string *out_excerpt) {
  if (out_excerpt == nullptr)
    return;
  out_excerpt->clear();

  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_runtime_tool(runtime_app, tool_name, arguments_json,
                                   &structured, &error)) {
    *out_excerpt = "<" + tool_name + " unavailable: " + error + ">";
    return;
  }
  *out_excerpt = cmd_json_string(cmd_json_field(&structured, "text"));
  if (out_excerpt->empty())
    *out_excerpt = "<empty>";
}

